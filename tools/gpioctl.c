// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>
// SPDX-FileCopyrightText: 2026 Bartosz Golaszewski <bartosz.golaszewski@oss.qualcomm.com>
// SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <gpiod.h>
#include <poll.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/prctl.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include "tools-common.h"

#define PACKED	__attribute__((__packed__));
#define UNUSED	__attribute__((unused))

enum {
	MSG_TYPE_OK,
	MSG_TYPE_ERROR,
	MSG_TYPE_PING,
	MSG_TYPE_STOP,
	MSG_TYPE_REQUEST,
	MSG_TYPE_REQ_OK,
	MSG_TYPE_RELEASE,
	NUM_MSG_TYPES,
};

#define MAX_EPOLL_EVENTS	16
#define MAX_REQ_NAME_SIZE	32
#define MAX_REQ_LINES		64

struct list_head {
	struct list_head *prev;
	struct list_head *next;
};

#define container_of(ptr, type, member) \
({ \
	void *__mptr = (void *)(ptr); \
	((type *)(__mptr - offsetof(type, member))); \
})

static void list_init(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

static void list_add(struct list_head *new, struct list_head *head)
{
	struct list_head *prev = head->prev;

	head->prev = new;
	new->next = head;
	new->prev = prev;
	prev->next = new;
}

static void list_del(struct list_head *entry)
{
	struct list_head *prev = entry->prev, *next = entry->next;

	prev->next = next;
	next->prev = prev;
}

#define list_entry(ptr, type, member) container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

#define list_next_entry(pos, member) \
	list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_entry_is_head(pos, head, member) (&pos->member == (head))

#define list_for_each_entry_safe(pos, next, head, member) \
	for (pos = list_first_entry(head, typeof(*pos), member), \
	     next = list_next_entry(pos, member); \
	     !list_entry_is_head(pos, head, member); \
	     pos = next, next = list_next_entry(next, member))

struct error {
	unsigned int errnum;
} PACKED;

struct request {
	char chip_path[128];
	unsigned int offsets[MAX_REQ_LINES];
	unsigned int num_offsets;
	int values[MAX_REQ_LINES];
	bool active_low;
	bool output;
} PACKED;

struct request_ok {
	char req_name[MAX_REQ_NAME_SIZE];
} PACKED;

/*
 * The protocol is not stable, it is subject to change at any moment without
 * any notice and meant only to be used by this program. Don't make it part
 * of any library.
 */
struct message {
	unsigned int type;
	union {
		struct error err;
		struct request req;
		struct request_ok req_ok;
	};
} PACKED;

struct config {
	bool active_low;
	bool strict;
	const char *chip_id;
	const char *consumer;
	bool by_name;
};

struct server;

struct receiver {
	int fd;
	void (*receive)(uint32_t, struct receiver *, struct server *);
};

struct client {
	struct list_head list;
	struct receiver rcv;
	struct ucred creds;
};

static struct client *to_client(struct receiver *rcv)
{
	return container_of(rcv, struct client, rcv);
}

struct line_request {
	struct list_head list;
	struct gpiod_line_request *handle;
};

struct server {
	bool stop;
	bool logging;
	struct receiver rcv;
	int epollfd;
	unsigned int usecnt;
	struct receiver idle_timer;
	struct receiver signal_handler;
	struct list_head clients;
	struct list_head requests;
};

static PRINTF(2, 3) void srv_log(struct server *srv, const char *fmt, ...)
{
	va_list va;

	if (!srv->logging)
		return;

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
}

static int srv_add_receiver(struct server *srv, struct receiver *rcv)
{
	struct epoll_event event = {
		.events = EPOLLIN | EPOLLPRI,
		.data.ptr = rcv,
	};
	int ret;

	ret = epoll_ctl(srv->epollfd, EPOLL_CTL_ADD, rcv->fd, &event);
	if (ret)
		srv_log(srv, "Failed to add the reciever to the epoll set: %s\n",
			strerror(errno));

	return 0;
}

static void srv_remove_receiver(struct server *srv, struct receiver *rcv)
{
	epoll_ctl(srv->epollfd, EPOLL_CTL_DEL, rcv->fd, NULL);
}

static void srv_drop_client(struct server *srv, struct client *cli)
{
	list_del(&cli->list);
	srv_remove_receiver(srv, &cli->rcv);
	close(cli->rcv.fd);
	free(cli);
	srv->usecnt--;
}

static int receive_message(struct receiver *rcv, struct server *srv,
			   struct message *msg)
{
	ssize_t ret;

	ret = recv(rcv->fd, msg, sizeof(*msg), 0);
	if (ret < 0) {
		srv_log(srv, "Failed to receive client data: %s\n",
			strerror(errno));
		return -1;
	}

	return 0;
}

static int send_message(struct receiver *rcv, struct server *srv,
			struct message *msg)
{
	ssize_t ret;

	ret = send(rcv->fd, msg, sizeof(*msg), 0);
	if (ret < 0) {
		srv_log(srv, "Failed to send data to client: %s\n",
			strerror(errno));
		return -1;
	}

	return 0;
}

static void send_error_response(struct client *cli, struct server *srv,
				int errnum)
{
	struct message msg = { .type = MSG_TYPE_ERROR, .err.errnum = errnum };

	send_message(&cli->rcv, srv, &msg);
}

static struct line_request *request_lines(const struct request *req)
{
	struct gpiod_request_config *req_cfg;
	struct gpiod_line_settings *settings;
	unsigned int offsets[MAX_REQ_LINES];
	struct gpiod_line_config *line_cfg;
	struct line_request *line_req;
	enum gpiod_line_direction dir;
	struct gpiod_chip *chip;
	int ret;

	line_req = malloc(sizeof(*line_req));
	req_cfg = gpiod_request_config_new();
	line_cfg = gpiod_line_config_new();
	settings = gpiod_line_settings_new();
	if (!line_req || !req_cfg || !line_cfg || !settings)
		die("out of memory");

	dir = req->output ? GPIOD_LINE_DIRECTION_OUTPUT : GPIOD_LINE_DIRECTION_INPUT;

	ret = gpiod_line_settings_set_direction(settings, dir);
	if (ret)
		goto err_out;

	memcpy(offsets, req->offsets, sizeof(offsets));

	ret = gpiod_line_config_add_line_settings(line_cfg, offsets,
						  req->num_offsets, settings);
	if (ret)
		goto err_out;

	gpiod_request_config_set_consumer(req_cfg, "dupa");

	chip = gpiod_chip_open(req->chip_path);
	if (!chip)
		goto err_out;

	line_req->handle = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
	if (!line_req->handle)
		goto err_out;

	gpiod_line_settings_free(settings);
	gpiod_line_config_free(line_cfg);
	gpiod_request_config_free(req_cfg);

	return line_req;

err_out:
	gpiod_line_settings_free(settings);
	gpiod_line_config_free(line_cfg);
	gpiod_request_config_free(req_cfg);
	free(line_req);
	return NULL;
}

static void handle_line_request(struct client *cli, struct server *srv,
				const struct request *req)
{
	struct line_request *line_req;
	struct message resp = { };

	srv_log(srv, "Handling line request from process %d\n", cli->creds.pid);

	line_req = request_lines(req);
	if (!line_req) {
		srv_log(srv, "Line request failed (%s), sending back error response\n",
			strerror(errno));
		send_error_response(cli, srv, errno);
		return;
	}

	resp.type = MSG_TYPE_REQ_OK;
	snprintf(resp.req_ok.req_name, MAX_REQ_NAME_SIZE, "dupa");

	send_message(&cli->rcv, srv, &resp);

	list_add(&srv->requests, &line_req->list);
	srv->usecnt++;
}

static void client_receive(uint32_t events, struct receiver *rcv,
			   struct server *srv)
{
	struct client *cli = to_client(rcv);
	struct message msg = { };
	int ret;

	if (events & EPOLLHUP) {
		srv_log(srv, "Process %d hang up\n", cli->creds.pid);
		srv_drop_client(srv, cli);
		return;
	}

	ret = receive_message(rcv, srv, &msg);
	if (ret)
		return;

	switch (msg.type) {
	case MSG_TYPE_STOP:
		srv_log(srv, "Stop request received from client, exiting\n");
		msg.type = MSG_TYPE_OK;
		ret = send_message(rcv, srv, &msg);
		if (ret)
			srv_drop_client(srv, cli);

		srv->stop = true;
		break;
	case MSG_TYPE_REQUEST:
		handle_line_request(cli, srv, &msg.req);
		break;
	case MSG_TYPE_PING:
		srv_log(srv, "Ping request received from client\n");
		msg.type = MSG_TYPE_OK;
		ret = send_message(rcv, srv, &msg);
		if (ret)
			srv_drop_client(srv, cli);

		break;
	default:
		srv_log(srv, "Unexpected message type %d received from client\n",
			msg.type);
		srv_drop_client(srv, cli);
		return;
	}
}

static void server_receive(uint32_t events UNUSED, struct receiver *rcv,
			   struct server *srv)
{
	struct client *cli;
	struct passwd *pwd;
	socklen_t len;
	int sock, ret;

	sock = accept(rcv->fd, NULL, NULL);
	if (sock < 0) {
		srv_log(srv, "Failed to accept client connection: %s\n",
			strerror(errno));
		return;
	}

	cli = malloc(sizeof(*cli));
	if (!cli)
		die("out of memory");

	len = sizeof(cli->creds);
	ret = getsockopt(sock, SOL_SOCKET, SO_PEERCRED, &cli->creds, &len);
	if (ret) {
		free(cli);
		return;
	}

	pwd = getpwuid(cli->creds.uid);
	if (!pwd) {
		srv_log(srv, "Failed to get credentials from connected client: %s\n",
			strerror(errno));
		free(cli);
		return;
	}

	cli->rcv.fd = sock;
	cli->rcv.receive = client_receive;

	ret = srv_add_receiver(srv, &cli->rcv);
	if (ret)
		return;

	list_add(&cli->list, &srv->clients);

	srv_log(srv, "Accepted connection from process %d (user: %s)\n",
		cli->creds.pid, pwd->pw_name);

	srv->usecnt++;
}

static void signal_receive(uint32_t events UNUSED, struct receiver *rcv,
			   struct server *srv)
{
	struct signalfd_siginfo siginfo;
	ssize_t ret;

	ret = read(rcv->fd, &siginfo, sizeof(siginfo));
	if (ret < 0) {
		srv_log(srv, "Failed to receive signal info\n");
		return;
	}

	switch (siginfo.ssi_signo) {
	case SIGINT:
	case SIGTERM:
	case SIGQUIT:
		srv_log(srv, "Signal %d received\n", siginfo.ssi_signo);
		srv->stop = true;
		break;
	default:
		break;
	}
}

static void add_signalfd(struct server *srv)
{
	int sigfd, ret;
	sigset_t mask;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGQUIT);

	ret = sigprocmask(SIG_BLOCK, &mask, NULL);
	if (ret)
		die_perror("Failed to mask signals");

	sigfd = signalfd(-1, &mask, 0);
	if (sigfd < 0)
		die_perror("Failed to create signalfd");

	srv->signal_handler.fd = sigfd;
	srv->signal_handler.receive = signal_receive;

	ret = srv_add_receiver(srv, &srv->signal_handler);
	if (ret)
		exit(EXIT_FAILURE);
}

static void timer_receive(uint32_t events UNUSED, struct receiver *rcv,
			  struct server *srv)
{
	uint64_t exp;
	ssize_t ret;

	ret = read(rcv->fd, &exp, sizeof(exp));
	if (ret < 0)
		die_perror("Failed to read timer data");

	srv_log(srv, "Idle timer expired\n");
	srv->stop = true;
}

static void add_timerfd(struct server *srv)
{
	struct receiver *timer = &srv->idle_timer;

	timer->fd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (timer->fd < 0)
		die_perror("Failed to create the timer file descriptor");

	timer->receive = timer_receive;
}

static void setup_idle_timer(struct server *srv)
{
	struct receiver *timer = &srv->idle_timer;
	struct itimerspec ts = { }, curr = { };
	struct timespec now;
	bool running;
	int ret;

	ret = timerfd_gettime(timer->fd, &curr);
	if (ret < 0)
		die_perror("Failed to read current idle timer setting");

	running = !curr.it_value.tv_sec && !curr.it_value.tv_nsec ? false : true;

	if (srv->usecnt && running) {
		srv_log(srv, "Server is now active, disarming the idle timer\n");

		ret = timerfd_settime(timer->fd, 0, &ts, NULL);
		if (ret)
			die_perror("Failed to disarm the idle timer");

		srv_remove_receiver(srv, timer);
	} else if (!srv->usecnt && !running) {
		srv_log(srv, "Server is now idle, arming the idle timer\n");

		ret = clock_gettime(CLOCK_MONOTONIC, &now);
		if (ret)
			die_perror("Failed to get current time");

		ts.it_value.tv_sec = now.tv_sec + 60;
		ts.it_value.tv_nsec = now.tv_nsec;

		ret = timerfd_settime(timer->fd, TFD_TIMER_ABSTIME, &ts, NULL);
		if (ret)
			die_perror("Failed to arm the idle timer");

		ret = srv_add_receiver(srv, timer);
		if (ret)
			exit(EXIT_FAILURE);
	}
}

static void free_server(struct server *srv)
{
	close(srv->epollfd);
	close(srv->rcv.fd);
	free(srv);
}

static NORETURN void run_server(struct server *srv)
{
	struct epoll_event events[MAX_EPOLL_EVENTS], *event;
	struct client *cli, *tmp;
	struct receiver *rcv;
	int numev, i;

	add_signalfd(srv);
	add_timerfd(srv);

	srv_log(srv, "gpioctl server started\n");

	while (!srv->stop) {
		setup_idle_timer(srv);

		numev = epoll_wait(srv->epollfd, events, MAX_EPOLL_EVENTS, 60 * 1000);
		if (numev < 0) {
			if (errno == EINTR)
				continue;

			srv_log(srv, "Failed to poll file descriptors: %s\n",
				strerror(errno));
			exit(EXIT_FAILURE);
		}

		if (numev == 0)
			/* Timeout. */
			continue;

		for (i = 0; i < numev; i++) {
			event = &events[i];
			rcv = event->data.ptr;
			rcv->receive(event->events, rcv, srv);
		}
	}

	srv_log(srv, "gpioctl server exiting\n");

	list_for_each_entry_safe(cli, tmp, &srv->clients, list)
		srv_drop_client(srv, cli);
	close(srv->signal_handler.fd);
	close(srv->idle_timer.fd);
	free_server(srv);

	exit(EXIT_SUCCESS);
}

static struct server *make_server(const struct sockaddr_un *addr)
{
	struct epoll_event event = { .events = EPOLLIN | EPOLLERR };
	struct server *srv;
	int ret, sock;

	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0)
		die_perror("Failed to create the server socket");

	ret = bind(sock, (struct sockaddr *)addr, sizeof(*addr));
	if (ret)
		die_perror("Failed to bind to the unix socket");

	ret = listen(sock, 128);
	if (ret < 0)
		die_perror("Failed to start listening on the socket");

	srv = malloc(sizeof(*srv));
	if (!srv)
		die("out of memory");

	srv->epollfd = epoll_create1(0);
	if (srv->epollfd < 0)
		die_perror("Failed to create the epoll handle");

	list_init(&srv->clients);
	srv->rcv.fd = sock;
	srv->rcv.receive = server_receive;
	event.data.ptr = &srv->rcv;

	ret = epoll_ctl(srv->epollfd, EPOLL_CTL_ADD, sock, &event);
	if (ret)
		die_perror("Failed to add the server socket to the epoll set");

	return srv;
}

static void redirect_fds(void)
{
	int fd;
	
	fd = open("/dev/null", O_RDONLY);
	dup2(fd, STDIN_FILENO);
	close(fd);

	fd = open("/dev/null", O_WRONLY);
	dup2(fd, STDOUT_FILENO);
	dup2(fd, STDERR_FILENO);
	close(fd);
}

static void spawn_server(struct server *srv)
{
	pid_t pid;

	pid = fork();
	if (pid < 0) {
		die_perror("Failed to spawn the server process");
	} else if (pid == 0) {
		/*
		 * Server process. Start a new session and fork again to be
		 * reparented to init.
		 */
		setsid();

		pid = fork();
		if (pid < 0)
			die_perror("Failed to spawn the final server process");
		else if (pid > 0)
			exit(EXIT_SUCCESS);

		prctl(PR_SET_NAME, "gpioctl server");
		redirect_fds();

		run_server(srv);
	} else {
		/* Client process, wait for the first fork to exit. */
		waitpid(pid, NULL, 0);
	}
}

static void start_server(const struct sockaddr_un *addr)
{
	struct server *srv;

	srv = make_server(addr);
	spawn_server(srv);
	/* If we returned, we're the client. Free server resources. */
	free_server(srv);
}

static int try_connect(const struct sockaddr_un *addr)
{
	int sock, ret;

	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0)
		die_perror("Failed to create the client socket");

	ret = connect(sock, (struct sockaddr *)addr, sizeof(*addr));
	if (ret) {
		if (errno == ENOENT || errno == ECONNREFUSED) {
			/*
			 * The socket file does not exist or is there but
			 * there's no server bound to it behind.
			 */
			close(sock);
			return -1;
		}

		die_perror("Failed to connect to server");
	}

	return sock;
}

static void make_addr(struct sockaddr_un *addr)
{
	memset(addr, 0, sizeof(*addr));
	addr->sun_family = AF_UNIX;
	snprintf(addr->sun_path + 1, sizeof(addr->sun_path) - 1,
		 "gpioctl-%u", getuid());
}

static int get_connection(void)
{
	struct sockaddr_un addr;
	int sock;

	make_addr(&addr);

	sock = try_connect(&addr);
	if (sock >= 0)
		return sock;

	/* Start the server if client failed to connect. */
	start_server(&addr);

	/* Try again to connect as client. */
	sock = try_connect(&addr);
	if (sock < 0)
		/* Still failing, give up. */
		die_perror("Failed to connect to server");

	return sock;
}

static int debug_main(int argc UNUSED, char **argv UNUSED,
		      const struct config *cfg UNUSED)
{
	struct sockaddr_un addr;
	struct server *srv;

	make_addr(&addr);
	srv = make_server(&addr);
	srv->logging = true;
	run_server(srv);
}

static void client_send_msg(int sock, const struct message *msg)
{
	ssize_t ret;

	ret = send(sock, msg, sizeof(*msg), 0);
	if (ret < 0)
		die_perror("Failed to send data to server");
}

static void client_recv_msg(int sock, struct message *msg)
{
	struct pollfd fd = { .fd = sock, .events = POLLIN | POLLPRI };
	ssize_t recvd;
	int ret;

	memset(msg, 0, sizeof(*msg));

	/* 10-second timeout for server to respond. */
	ret = poll(&fd, 1, 10 * 1000);
	if (ret < 0)
		die_perror("Failed while polling server for response");
	if (ret == 0)
		die("Timeout while waiting for server to respond");

	recvd = recv(sock, msg, sizeof(*msg), 0);
	if (recvd < 0)
		die_perror("Failed to receive data from server");
}

static void client_exchange(int sock, struct message *msg)
{
	client_send_msg(sock, msg);
	client_recv_msg(sock, msg);
}

static void client_validate_response(const struct message *msg,
				     unsigned int expected)
{
	if (msg->type == MSG_TYPE_ERROR)
		die("Internal server error: %s", strerror(msg->err.errnum));

	if (msg->type != expected)
		die("Server responded with unexpected message type: %d",
		    msg->type);
}

static void print_request_help(void)
{
	printf("Usage: %s [GLOBAL OPTIONS] request ...\n", get_prog_name());
}

static void parse_request_cmdline(int *argc, char ***argv)
{
	static const struct option longopts[] = {
		{ "help",		no_argument,	NULL,	'h' },
		{ GETOPT_NULL_LONGOPT }
	};

	static const char *const shortopts = "+h";

	int optc, opti;

	for (;;) {
		optc = getopt_long(*argc, *argv, shortopts, longopts, &opti);
		if (optc < 0)
			break;

		switch (optc) {
		case 'h':
			print_request_help();
			exit(EXIT_SUCCESS);
		case '?':
			die("try %s request --help", get_prog_name());
		default:
			abort();
		};
	}

	*argc -= optind;
	*argv += optind;
}

static int request_main(int argc, char **argv, const struct config *cfg)
{
	struct message msg = { .type = MSG_TYPE_REQUEST };
	struct line_resolver *resolver;
	struct request *req = &msg.req;
	int sock, i;

	parse_request_cmdline(&argc, &argv);

	resolver = resolve_lines(argc, argv, cfg->chip_id, cfg->strict,
				 cfg->by_name);
	validate_resolution(resolver, cfg->chip_id);

	/* FIXME should be able to just send multiple messages */
	if (resolver->num_chips > 1)
		die("Can only manipulate lines from one chip at a time");

	if (!resolver->num_lines)
		die("At least one line must be specified");

	if (resolver->num_lines > MAX_REQ_LINES)
		die("Can only handle of up to %u lines", MAX_REQ_LINES);

	strcpy(req->chip_path,
		resolver->chips[resolver->lines[0].chip_num].path);
	
	for (i = 0; i < resolver->num_lines; i++)
		req->offsets[i] = resolver->lines[i].offset;
	req->num_offsets = resolver->num_lines;

	sock = get_connection();

	client_exchange(sock, &msg);
	client_validate_response(&msg, MSG_TYPE_REQ_OK);

	free_line_resolver(resolver);
	close(sock);

	return EXIT_SUCCESS;
}

static int release_main(int argc UNUSED, char **argv UNUSED,
			const struct config *cfg UNUSED)
{
	return EXIT_SUCCESS;
}

static int stop_main(int argc UNUSED, char **argv UNUSED,
		     const struct config *cfg UNUSED)
{
	struct message msg = { .type = MSG_TYPE_STOP };
	int sock = get_connection();

	client_exchange(sock, &msg);
	client_validate_response(&msg, MSG_TYPE_OK);
	close(sock);

	return EXIT_SUCCESS;
}

static int ping_main(int argc UNUSED, char **argv UNUSED,
		     const struct config *cfg UNUSED)
{
	struct message msg = { .type = MSG_TYPE_PING };
	int sock = get_connection();

	client_exchange(sock, &msg);
	client_validate_response(&msg, MSG_TYPE_OK);
	close(sock);

	return EXIT_SUCCESS;
}

struct sub_command {
	const char *name;
	int (*sub_main)(int, char **, const struct config *);
	const char *desc;
};

static const struct sub_command sub_cmds[] = {
	{
		.name = "debug",
		.sub_main = debug_main,
	},
	{
		.name = "ping",
		.sub_main = ping_main,
	},
	{
		.name = "stop",
		.sub_main = stop_main,
	},
	{
		.name = "request",
		.sub_main = request_main,
		.desc = "Request a set of GPIO lines.",
	},
	{
		.name = "release",
		.sub_main = release_main,
		.desc = "Release a set of requested GPIO lines.",
	},
	{ }
};

static const struct sub_command *find_sub_cmd(const char *name)
{
	const struct sub_command *cmd;

	for (cmd = sub_cmds; cmd->name; cmd++) {
		if (strcmp(name, cmd->name) == 0)
			return cmd;
	}

	return NULL;
}

static void print_help(void)
{
	const struct sub_command *cmd;

	printf("Usage: %s [GLOBAL OPTIONS] [SUB-COMMAND] [COMMAND OPTIONS] ...\n",
	       get_prog_name());
	printf("\n");
	printf("Simple swiss-army knife for controlling GPIOs with persistence\n");
	printf("support.\n");
	printf("\n");
	printf("Commands:\n");
	for (cmd = sub_cmds; cmd->name; cmd++) {
		if (cmd->desc)
			printf("  %s - %s\n", cmd->name, cmd->desc);
	}
	printf("\n");
	printf("Options:\n");
	printf("  -h, --help\t\tDisplay this help and exit.\n");
	printf("  -v, --version\t\tOutput version information and exit.\n");
	printf("  -p, --socket-path\tPath to the unix-domain socket used for\n");
	printf("\t\t\tcommunication between client and server.\n");
}

static void parse_global_cmdline(int *argc, char ***argv, struct config *cfg)
{
	static const struct option longopts[] = {
		{ "help",		no_argument,		NULL,	'h' },
		{ "version",		no_argument,		NULL,	'v' },
		{ "active-low",		no_argument,		NULL,	'l' },
		{ "chip",		required_argument,	NULL,	'c' },
		{ "strict",		no_argument,		NULL,	's' },
		{ "consumer",		required_argument,	NULL,	'C' },
		{ "by-name",		no_argument,		NULL,	'B' },
		{ GETOPT_NULL_LONGOPT },
	};

	static const char *const shortopts = "+hvlcCsn";

	int optc, opti;

	for (;;) {
		optc = getopt_long(*argc, *argv, shortopts, longopts, &opti);
		if (optc < 0)
			break;

		switch (optc) {
		case 'h':
			print_help();
			exit(EXIT_SUCCESS);
		case 'v':
			print_version();
			exit(EXIT_SUCCESS);
		case 'l':
			cfg->active_low = true;
			break;
		case 'c':
			cfg->chip_id = optarg;
			break;
		case 's':
			cfg->strict = true;
			break;
		case 'C':
			cfg->consumer = optarg;
			break;
		case 'B':
			cfg->by_name = true;
			break;
		case '?':
			die("try %s --help", get_prog_name());
		default:
			abort();
		}
	}

	*argc -= optind;
	*argv += optind;

	if (!*argc)
		die("sub-command is required, try %s --help", get_prog_name());
}

int main(int argc, char **argv)
{
	const struct sub_command *subcmd;
	struct config cfg = { };

	set_prog_name(argv[0]);
	parse_global_cmdline(&argc, &argv, &cfg);

	subcmd = find_sub_cmd(argv[0]);
	if (!subcmd)
		die("invalid sub-command, try %s --help", get_prog_name());

	return subcmd->sub_main(argc, argv, &cfg);
}
