// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <gpiod.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
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

#define MSG_TYPE_STOP		1
#define MSG_TYPE_ERROR		2
#define MSG_TYPE_REQUEST	3

#define MSG_ERROR_TYPE_SYSTEM	1

#define MAX_EVENTS		16
#define MAX_LINES		64

#define container_of(ptr, type, member) \
({ \
	void *__mptr = (void *)(ptr); \
	((type *)(__mptr - offsetof(type, member))); \
})

struct error {
	unsigned int type;
	unsigned int errnum;
};

struct request {
	unsigned int chip_num;
	unsigned int offsets[MAX_LINES];
	unsigned int num_offsets;
	int values[MAX_LINES];
	bool active_low;
	bool output;
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
	struct receiver rcv;
	struct ucred creds;
};

static struct client *to_client(struct receiver *rcv)
{
	return container_of(rcv, struct client, rcv);
}

struct server {
	bool stop;
	bool logging;
	struct receiver rcv;
	int epollfd;
	unsigned int usecnt;
	struct receiver idle_timer;
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

static int add_receiver(struct server *srv, struct receiver *rcv)
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

static void remove_receiver(struct server *srv, struct receiver *rcv)
{
	epoll_ctl(srv->epollfd, EPOLL_CTL_DEL, rcv->fd, NULL);
}

static void drop_client(struct server *srv, struct client *cli)
{
	remove_receiver(srv, &cli->rcv);
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
		srv_log(srv, "Failed to receive client data: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

static struct gpiod_line_request *request_lines(const struct request *req)
{
	return NULL;
}

static void handle_line_request(struct client *cli, struct server *srv,
				const struct request *req)
{
	struct gpiod_line_request *request;
	struct message resp = { };
	ssize_t ret;

	srv_log(srv, "Handling line request from process %d\n", cli->creds.pid);

	request = request_lines(req);
	if (!request) {
		srv_log(srv, "Line request failed (%s), sending back error response\n",
			strerror(errno));

		resp.type = MSG_TYPE_ERROR;
		resp.err.type = MSG_ERROR_TYPE_SYSTEM;
		resp.err.errnum = errno;

		ret = send(cli->rcv.fd, &resp, sizeof(resp), 0);
		if (ret < 0) {
			srv_log(srv, "Failed to send response to client: %s\n",
				strerror(errno));
			drop_client(srv, cli);
			return;
		}
	}
}

static void client_receive(uint32_t events, struct receiver *rcv,
			   struct server *srv)
{
	struct client *cli = to_client(rcv);
	struct message msg = { };
	int ret;

	if (events & EPOLLHUP) {
		srv_log(srv, "Process %d hang up\n", cli->creds.pid);
		drop_client(srv, cli);
		return;
	}

	ret = receive_message(rcv, srv, &msg);
	if (ret)
		return;

	switch (msg.type) {
	case MSG_TYPE_STOP:
		srv_log(srv, "Stop request received from client, exiting\n");
		srv->stop = true;
		break;
	case MSG_TYPE_REQUEST:
		handle_line_request(cli, srv, &msg.req);
		break;
	default:
		srv_log(srv, "Unexpected message type %d received from client\n",
			msg.type);
		drop_client(srv, cli);
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

	ret = add_receiver(srv, &cli->rcv);
	if (ret)
		return;

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
		srv_log(srv, "Signal %d received, exiting\n",
			siginfo.ssi_signo);
		srv->stop = true;
	default:
		break;
	}
}

static void add_signalfd(struct server *srv)
{
	struct receiver *rcv;
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

	rcv = malloc(sizeof(*rcv));
	if (!rcv)
		die("out of memory");

	rcv->fd = sigfd;
	rcv->receive = signal_receive;

	ret = add_receiver(srv, rcv);
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

	srv_log(srv, "Idle timer expired, exiting\n");
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

		remove_receiver(srv, timer);

		ret = timerfd_settime(timer->fd, 0, &ts, NULL);
		if (ret)
			die_perror("Failed to disarm the idle timer");
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

		ret = add_receiver(srv, timer);
		if (ret)
			exit(EXIT_FAILURE);
	}
}

static NORETURN void run_server(struct server *srv)
{
	struct epoll_event events[MAX_EVENTS], *event;
	struct receiver *rcv;
	int numev, i;

	add_signalfd(srv);
	add_timerfd(srv);

	srv_log(srv, "gpioctl server started\n");

	while (!srv->stop) {
		setup_idle_timer(srv);

		numev = epoll_wait(srv->epollfd, events, MAX_EVENTS, 60 * 1000);
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

	srv->rcv.fd = sock;
	srv->rcv.receive = server_receive;
	event.data.ptr = &srv->rcv;

	ret = epoll_ctl(srv->epollfd, EPOLL_CTL_ADD, sock, &event);
	if (ret)
		die_perror("Failed to add the server socket to the epoll set");

	return srv;
}

static void free_server(struct server *srv)
{
	close(srv->epollfd);
	close(srv->rcv.fd);
	free(srv);
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
	struct line_resolver *resolver;
	struct message msg;
	int sock, i;
	ssize_t ret;

	parse_request_cmdline(&argc, &argv);

	resolver = resolve_lines(argc, argv, cfg->chip_id, cfg->strict,
				 cfg->by_name);
	validate_resolution(resolver, cfg->chip_id);

	/* FIXME should be able to just send multiple messages */
	if (resolver->num_chips > 1)
		die("Can only manipulate lines from one chip at a time");

	if (!resolver->num_lines)
		die("At least one line must be specified");

	if (resolver->num_lines > MAX_LINES)
		die("Can only handle of up to %u lines", MAX_LINES);

	memset(&msg, 0, sizeof(msg));

	msg.type = MSG_TYPE_REQUEST;

	msg.req.chip_num = resolver->lines[0].chip_num;
	
	for (i = 0; i < resolver->num_lines; i++)
		msg.req.offsets[i] = resolver->lines[i].offset;

	sock = get_connection();

	ret = send(sock, &msg, sizeof(msg), 0);
	if (ret < 0)
		die_perror("Failed to send data to server");

	memset(&msg, 0, sizeof(msg));
	ret = recv(sock, &msg, sizeof(msg), 0);
	if (ret < 0)
		die_perror("Failed to receive data from server");

	free_line_resolver(resolver);

	return EXIT_SUCCESS;
}

static int stop_main(int argc UNUSED, char **UNUSED,
		     const struct config *cfg UNUSED)
{
	int sock = get_connection();
	struct message msg = { };
	ssize_t written;

	msg.type = MSG_TYPE_STOP;

	written = send(sock, &msg, sizeof(msg), 0);
	if (written < 0)
		die_perror("Failed to send the stop request to server");

	/* FIXME should poll with a timeout. */
	recv(sock, &msg, sizeof(msg), 0);
	close(sock);
	return 0;
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
		.name = "stop",
		.sub_main = stop_main,
	},
	{
		.name = "request",
		.sub_main = request_main,
		.desc = "Request a set of GPIO lines.",
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
