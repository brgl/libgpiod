// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>
// SPDX-FileCopyrightText: 2026 Bartosz Golaszewski <bartosz.golaszewski@oss.qualcomm.com>
// SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <gpiod.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/file.h>
#include <sys/prctl.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include "tools-common.h"

#define PACKED	__attribute__((__packed__))
#define UNUSED	__attribute__((unused))

enum {
	MSG_TYPE_OK,
	MSG_TYPE_ERROR,
	MSG_TYPE_PING,
	MSG_TYPE_STOP,
	MSG_TYPE_REQUEST,
	MSG_TYPE_REQ_OK,
	MSG_TYPE_RELEASE,
	MSG_TYPE_REQ_LIST,
	MSG_TYPE_REQ_INFO,
	MSG_TYPE_GET,
	MSG_TYPE_GET_OK,
	MSG_TYPE_SET,
	MSG_TYPE_MONITOR,
	MSG_TYPE_EDGE_EVENT,
	MSG_TYPE_RECONFIGURE,
	NUM_MSG_TYPES,
};

static const char *const msg_type_str[] = {
	[MSG_TYPE_OK] = "Operation Successful",
	[MSG_TYPE_ERROR] = "Operation Failed",
	[MSG_TYPE_PING] = "Ping Server",
	[MSG_TYPE_STOP] = "Stop Server",
	[MSG_TYPE_REQUEST] = "Line Request",
	[MSG_TYPE_REQ_OK] = "Line Request Successful",
	[MSG_TYPE_RELEASE] = "Release",
	[MSG_TYPE_REQ_LIST] = "List Requests",
	[MSG_TYPE_REQ_INFO] = "Request Info",
	[MSG_TYPE_GET] = "Get Values",
	[MSG_TYPE_GET_OK] = "Get Values Response",
	[MSG_TYPE_SET] = "Set Values",
	[MSG_TYPE_MONITOR] = "Monitor",
	[MSG_TYPE_EDGE_EVENT] = "Edge Event",
	[MSG_TYPE_RECONFIGURE] = "Reconfigure Request",
	NULL
};

#define MAX_EPOLL_EVENTS	16
#define SERVER_START_TIMEOUT_MS	2000
#define SEND_WAIT_TIMEOUT_MS	1000
#define CONSUMER_NAME_SIZE	32
#define MAX_REQ_LINES		64
#define MONITOR_EVENT_BUF_SIZE	16

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

#define list_for_each_entry(pos, head, member) \
	for (pos = list_first_entry(head, typeof(*pos), member); \
	     !list_entry_is_head(pos, head, member); \
	     pos = list_next_entry(pos, member))

#define list_for_each_entry_safe(pos, next, head, member) \
	for (pos = list_first_entry(head, typeof(*pos), member), \
	     next = list_next_entry(pos, member); \
	     !list_entry_is_head(pos, head, member); \
	     pos = next, next = list_next_entry(next, member))

struct error {
	unsigned int errnum;
} PACKED;

struct request_config {
	unsigned int offsets[MAX_REQ_LINES];
	unsigned int num_offsets;
	enum gpiod_line_value values[MAX_REQ_LINES];
	bool active_low;
	enum gpiod_line_direction direction;
	enum gpiod_line_bias bias;
	enum gpiod_line_drive drive;
	enum gpiod_line_edge edge;
	enum gpiod_line_clock event_clock;
	unsigned long debounce_period_us;
} PACKED;

struct request {
	char chip_path[128];
	char consumer[CONSUMER_NAME_SIZE];
	struct request_config cfg;
} PACKED;

struct request_ok {
	char req_name[CONSUMER_NAME_SIZE];
} PACKED;

struct release {
	unsigned int id;
} PACKED;

struct request_info {
	unsigned int id;
	char chip_name[32];
	char consumer[CONSUMER_NAME_SIZE];
	unsigned int offsets[MAX_REQ_LINES];
	unsigned int num_offsets;
	char line_names[MAX_REQ_LINES][32];
} PACKED;

struct get_req {
	unsigned int id;
	unsigned int offsets[MAX_REQ_LINES];
	unsigned int num_offsets;
} PACKED;

struct get_ok {
	enum gpiod_line_value values[MAX_REQ_LINES];
	unsigned int num_values;
} PACKED;

struct set_req {
	unsigned int id;
	unsigned int num_offsets;
	unsigned int offsets[MAX_REQ_LINES];
	unsigned int num_values;
	enum gpiod_line_value values[MAX_REQ_LINES];
} PACKED;

struct monitor_req {
	unsigned int id;
} PACKED;

struct reconfigure {
	unsigned int id;
	struct request_config cfg;
} PACKED;

struct edge_event {
	uint64_t timestamp_ns;
	unsigned int offset;
	unsigned int event_type;
} PACKED;

/*
 * The protocol is not stable, it is subject to change at any moment without
 * any notice and meant only to be used by this program. Don't make it part
 * of any library.
 *
 * Messages are sent and received as only msg_size() bytes: the type field plus
 * the union member matching it. Otherwise a flood of small messages (edge
 * events are 20 bytes) would each cost as much as the largest one.
 */
struct message {
	unsigned int type;
	union {
		struct error err;
		struct request req;
		struct request_ok req_ok;
		struct release rel;
		struct request_info req_info;
		struct get_req get_req;
		struct get_ok get_ok;
		struct set_req set_req;
		struct monitor_req monitor_req;
		struct reconfigure reconfigure;
		struct edge_event edge_ev;
	};
} PACKED;

struct config {
	bool by_name;
	bool numeric;
	bool strict;
	bool unquoted;
	const char *chip_id;
	const char *consumer;
	const char *socket_path;
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
	unsigned int id;
	char consumer[CONSUMER_NAME_SIZE];
	char line_names[MAX_REQ_LINES][32];
	unsigned int num_lines;
	struct client *monitor_client;
	struct receiver monitor_rcv;
	struct gpiod_edge_event_buffer *ev_buf;
};

struct server {
	bool stop;
	bool logging;
	bool timer_armed;
	struct receiver rcv;
	int epollfd;
	/*
	 * While the detached server is starting up this is the write end of
	 * the pipe used to tell the client about failures, -1 otherwise.
	 */
	int startup_fd;
	struct epoll_event *pending;
	int num_pending;
	unsigned int usecnt;
	struct receiver idle_timer;
	struct receiver signal_handler;
	struct list_head clients;
	struct list_head requests;
	char socket_path[sizeof(((struct sockaddr_un *)NULL)->sun_path)];
};

static void srv_vlog(struct server *srv, const char *fmt, va_list va)
{
	if (!srv->logging)
		return;

	vfprintf(stderr, fmt, va);
}

static PRINTF(2, 3) void srv_log(struct server *srv, const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	srv_vlog(srv, fmt, va);
	va_end(va);
}

static PRINTF(2, 3) NORETURN void
srv_die(struct server *srv, const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	srv_vlog(srv, fmt, va);
	va_end(va);
	srv_log(srv, "\n");

	exit(EXIT_FAILURE);
}

/*
 * The detached server has no stderr to complain on, so if it fails while
 * starting up, pass the error number to the client waiting on the other end of
 * the startup pipe. Does nothing once the server is up.
 */
static void srv_report_startup_failure(struct server *srv, int errnum)
{
	ssize_t ret;

	if (srv->startup_fd < 0)
		return;

	ret = write(srv->startup_fd, &errnum, sizeof(errnum));
	(void)ret;
}

static PRINTF(2, 3) NORETURN void
srv_die_perror(struct server *srv, const char *fmt, ...)
{
	va_list va;

	srv_report_startup_failure(srv, errno);

	va_start(va, fmt);
	srv_vlog(srv, fmt, va);
	srv_log(srv, ": %s\n", strerror(errno));
	va_end(va);

	exit(EXIT_FAILURE);
}

static NORETURN void srv_die_oom(struct server *srv)
{
	srv_report_startup_failure(srv, ENOMEM);
	srv_die(srv, "Out of memory");
}

static int srv_add_receiver(struct server *srv, struct receiver *rcv,
			    bool fatal)
{
	struct epoll_event event = {
		.events = EPOLLIN | EPOLLPRI,
		.data.ptr = rcv,
	};
	int ret;

	ret = epoll_ctl(srv->epollfd, EPOLL_CTL_ADD, rcv->fd, &event);
	if (ret) {
		srv_log(srv, "Failed to add the receiver to the epoll set: %s\n",
			strerror(errno));
		if (fatal)
			exit(EXIT_FAILURE);
	}

	return ret;
}

static void srv_remove_receiver(struct server *srv, struct receiver *rcv)
{
	int i;

	epoll_ctl(srv->epollfd, EPOLL_CTL_DEL, rcv->fd, NULL);

	/*
	 * The receiver may already have an event queued in the batch returned
	 * by the current epoll_wait() call and its owner may be freed right
	 * after this. Make sure we don't dispatch to it.
	 */
	for (i = 0; i < srv->num_pending; i++) {
		if (srv->pending[i].data.ptr == rcv)
			srv->pending[i].data.ptr = NULL;
	}
}

static void arm_idle_timer(struct server *srv)
{
	struct receiver *timer = &srv->idle_timer;
	struct itimerspec ts = { };
	struct timespec now;
	int ret;

	ret = clock_gettime(CLOCK_MONOTONIC, &now);
	if (ret)
		srv_die_perror(srv, "failed to get current time");

	ts.it_value.tv_sec = now.tv_sec + 60;
	ts.it_value.tv_nsec = now.tv_nsec;

	ret = timerfd_settime(timer->fd, TFD_TIMER_ABSTIME, &ts, NULL);
	if (ret)
		srv_die_perror(srv, "failed to arm the idle timer");

	srv_add_receiver(srv, timer, true);

	srv->timer_armed = true;
	srv_log(srv, "Server is now idle, arming the idle timer\n");
}

static void disarm_idle_timer(struct server *srv)
{
	struct receiver *timer = &srv->idle_timer;
	struct itimerspec ts = { };
	int ret;

	ret = timerfd_settime(timer->fd, 0, &ts, NULL);
	if (ret)
		srv_die_perror(srv, "failed to disarm the idle timer");

	srv_remove_receiver(srv, timer);
	srv->timer_armed = false;
	srv_log(srv, "Server is now active, disarming the idle timer\n");
}

static void srv_inc_usecnt(struct server *srv)
{
	srv->usecnt++;
	if (srv->usecnt == 1 && srv->timer_armed)
		disarm_idle_timer(srv);
}

static void srv_dec_usecnt(struct server *srv)
{
	srv->usecnt--;
	if (srv->usecnt == 0)
		arm_idle_timer(srv);
}

static void srv_drop_client(struct server *srv, struct client *cli)
{
	struct line_request *line_req;

	list_del(&cli->list);
	srv_remove_receiver(srv, &cli->rcv);
	close(cli->rcv.fd);

	list_for_each_entry(line_req, &srv->requests, list) {
		if (line_req->monitor_client == cli) {
			srv_remove_receiver(srv, &line_req->monitor_rcv);
			gpiod_edge_event_buffer_free(line_req->ev_buf);
			line_req->ev_buf = NULL;
			line_req->monitor_client = NULL;
		}
	}

	srv_dec_usecnt(srv);
	free(cli);
}

static const char *msg_type_to_str(unsigned int type)
{
	if (type >= NUM_MSG_TYPES)
		return "Invalid";

	return msg_type_str[type];
}

#define MSG_SIZE(member) \
	(offsetof(struct message, member) + \
	 sizeof(((struct message *)0)->member))

/* The type must have already been checked to be less than NUM_MSG_TYPES. */
static size_t msg_size(unsigned int type)
{
	switch (type) {
	case MSG_TYPE_ERROR:
		return MSG_SIZE(err);
	case MSG_TYPE_REQUEST:
		return MSG_SIZE(req);
	case MSG_TYPE_REQ_OK:
		return MSG_SIZE(req_ok);
	case MSG_TYPE_RELEASE:
		return MSG_SIZE(rel);
	case MSG_TYPE_REQ_INFO:
		return MSG_SIZE(req_info);
	case MSG_TYPE_GET:
		return MSG_SIZE(get_req);
	case MSG_TYPE_GET_OK:
		return MSG_SIZE(get_ok);
	case MSG_TYPE_SET:
		return MSG_SIZE(set_req);
	case MSG_TYPE_MONITOR:
		return MSG_SIZE(monitor_req);
	case MSG_TYPE_EDGE_EVENT:
		return MSG_SIZE(edge_ev);
	case MSG_TYPE_RECONFIGURE:
		return MSG_SIZE(reconfigure);
	default:
		/* OK, ping, stop and list: no payload. */
		return offsetof(struct message, err);
	}
}

/* Is a received datagram of len bytes a well-formed message? */
static bool msg_size_valid(const struct message *msg, ssize_t len)
{
	return len >= (ssize_t)sizeof(msg->type) &&
	       msg->type < NUM_MSG_TYPES &&
	       (size_t)len == msg_size(msg->type);
}

static int receive_message(struct receiver *rcv, struct server *srv,
			   struct message *msg)
{
	ssize_t ret;

	ret = recv(rcv->fd, msg, sizeof(*msg), MSG_TRUNC);
	if (ret < 0) {
		/* Nothing to read after all, wait for the next event. */
		if (errno == EAGAIN)
			return 1;

		srv_log(srv, "Failed to receive client data: %s\n",
			strerror(errno));
		return -1;
	}

	if (!msg_size_valid(msg, ret)) {
		srv_log(srv, "Invalid message received from client\n");
		return -1;
	}

	srv_log(srv, "Client message of type '%s' received\n",
		msg_type_to_str(msg->type));

	return 0;
}

static int send_message(struct receiver *rcv, struct server *srv,
			struct message *msg)
{
	ssize_t ret;

	ret = send(rcv->fd, msg, msg_size(msg->type), MSG_NOSIGNAL);
	if (ret < 0) {
		srv_log(srv, "Failed to send data to client: %s\n",
			strerror(errno));
		return -1;
	}

	srv_log(srv, "Server message of type '%s' sent\n",
		msg_type_to_str(msg->type));

	return 0;
}

/*
 * Like send_message() but if the client's socket buffer is full wait for it
 * to drain until the deadline instead of failing right away. For replies made
 * of many messages sent in a row that a client which does read them would
 * otherwise miss. The deadline is for the whole reply so a client that
 * doesn't read, or reads too slowly, can't stall the server for longer.
 */
static void deadline_from_now(struct timespec *ts, unsigned int ms)
{
	clock_gettime(CLOCK_MONOTONIC, ts);

	ts->tv_sec += ms / 1000;
	ts->tv_nsec += (ms % 1000) * 1000000L;

	if (ts->tv_nsec >= 1000000000L) {
		ts->tv_sec++;
		ts->tv_nsec -= 1000000000L;
	}
}

static int ms_until(const struct timespec *deadline)
{
	struct timespec now;
	long left;

	clock_gettime(CLOCK_MONOTONIC, &now);

	left = (deadline->tv_sec - now.tv_sec) * 1000 +
	       (deadline->tv_nsec - now.tv_nsec) / 1000000;

	return left > 0 ? left : 0;
}

static int send_message_wait(struct receiver *rcv, struct server *srv,
			     struct message *msg,
			     const struct timespec *deadline)
{
	struct pollfd pfd = {
		.fd = rcv->fd,
		.events = POLLOUT,
	};
	ssize_t ret;
	int left;

	for (;;) {
		ret = send(rcv->fd, msg, msg_size(msg->type), MSG_NOSIGNAL);
		if (ret >= 0)
			break;

		if (errno != EAGAIN) {
			srv_log(srv, "Failed to send data to client: %s\n",
				strerror(errno));
			return -1;
		}

		left = ms_until(deadline);
		if (!left) {
			srv_log(srv, "Client is not reading, giving up\n");
			return -1;
		}

		ret = poll(&pfd, 1, left);
		if (ret < 0) {
			if (errno == EINTR)
				continue;

			srv_log(srv, "Failed to poll the client socket: %s\n",
				strerror(errno));
			return -1;
		}
		if (ret == 0) {
			srv_log(srv, "Client is not reading, giving up\n");
			return -1;
		}
	}

	srv_log(srv, "Server message of type '%s' sent\n",
		msg_type_to_str(msg->type));

	return 0;
}

static void send_error_response(struct client *cli, struct server *srv,
				int errnum)
{
	struct message msg = {
		.type = MSG_TYPE_ERROR,
		.err.errnum = errnum,
	};
	int ret;

	ret = send_message(&cli->rcv, srv, &msg);
	if (ret)
		srv_drop_client(srv, cli);
}

static bool check_num_offsets(struct client *cli, struct server *srv,
			      unsigned int num_offsets)
{
	if (num_offsets > MAX_REQ_LINES) {
		send_error_response(cli, srv, EINVAL);
		return false;
	}

	return true;
}

static int build_line_config(struct gpiod_line_config *line_cfg,
			     struct gpiod_line_settings *settings,
			     const struct request_config *cfg)
{
	unsigned int offsets[MAX_REQ_LINES], i;
	int ret;

	if (cfg->direction)
		gpiod_line_settings_set_direction(settings, cfg->direction);
	/* These imply input. */
	else if (cfg->bias || cfg->edge || cfg->debounce_period_us)
		gpiod_line_settings_set_direction(settings,
						  GPIOD_LINE_DIRECTION_INPUT);

	if (cfg->active_low)
		gpiod_line_settings_set_active_low(settings, true);

	if (cfg->bias)
		gpiod_line_settings_set_bias(settings, cfg->bias);

	if (cfg->drive)
		gpiod_line_settings_set_drive(settings, cfg->drive);

	if (cfg->edge)
		gpiod_line_settings_set_edge_detection(settings, cfg->edge);

	if (cfg->event_clock)
		gpiod_line_settings_set_event_clock(settings, cfg->event_clock);

	if (cfg->debounce_period_us)
		gpiod_line_settings_set_debounce_period_us(settings,
						cfg->debounce_period_us);

	memcpy(offsets, cfg->offsets, cfg->num_offsets * sizeof(*offsets));

	/*
	 * Output values are part of the line settings: that way the lines are
	 * driven to the right level as soon as they become outputs.
	 */
	if (cfg->direction == GPIOD_LINE_DIRECTION_OUTPUT &&
	    cfg->values[0] != GPIOD_LINE_VALUE_ERROR) {
		for (i = 0; i < cfg->num_offsets; i++) {
			ret = gpiod_line_settings_set_output_value(settings,
							cfg->values[i]);
			if (ret)
				return ret;

			ret = gpiod_line_config_add_line_settings(line_cfg,
							&offsets[i], 1,
							settings);
			if (ret)
				return ret;
		}

		return 0;
	}

	return gpiod_line_config_add_line_settings(line_cfg, offsets,
						   cfg->num_offsets, settings);
}

static struct line_request *request_lines(struct server *srv,
					  const struct request *req)
{
	struct gpiod_request_config *req_cfg;
	struct gpiod_line_settings *settings;
	struct gpiod_line_config *line_cfg;
	struct line_request *line_req;
	struct gpiod_line_info *info;
	struct gpiod_chip *chip;
	const char *name;
	unsigned int i;
	int ret;

	line_req = malloc(sizeof(*line_req));
	req_cfg = gpiod_request_config_new();
	line_cfg = gpiod_line_config_new();
	settings = gpiod_line_settings_new();
	if (!line_req || !req_cfg || !line_cfg || !settings)
		srv_die_oom(srv);

	memset(line_req, 0, sizeof(*line_req));

	ret = build_line_config(line_cfg, settings, &req->cfg);
	if (ret)
		goto err_out;

	gpiod_request_config_set_consumer(req_cfg, req->consumer);

	chip = gpiod_chip_open(req->chip_path);
	if (!chip)
		goto err_out;

	line_req->handle = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
	if (!line_req->handle) {
		gpiod_chip_close(chip);
		goto err_out;
	}

	for (i = 0; i < req->cfg.num_offsets; i++) {
		info = gpiod_chip_get_line_info(chip, req->cfg.offsets[i]);
		if (info) {
			name = gpiod_line_info_get_name(info);
			if (name)
				snprintf(line_req->line_names[i],
					 sizeof(line_req->line_names[i]),
					 "%s", name);
			gpiod_line_info_free(info);
		}
	}

	gpiod_chip_close(chip);

	line_req->num_lines = req->cfg.num_offsets;

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

static bool request_id_taken(struct server *srv, unsigned int id)
{
	struct line_request *req;

	list_for_each_entry(req, &srv->requests, list) {
		if (req->id == id)
			return true;
	}

	return false;
}

static unsigned int get_request_id(struct server *srv)
{
	unsigned int id;

	for (id = 0; request_id_taken(srv, id); id++)
		;

	return id;
}

static void free_line_request(struct line_request *line_req)
{
	gpiod_line_request_release(line_req->handle);
	gpiod_edge_event_buffer_free(line_req->ev_buf);
	free(line_req);
}

static void handle_line_request(struct client *cli, struct server *srv,
				const struct request *req)
{
	struct message resp = { .type = MSG_TYPE_REQ_OK };
	struct line_request *line_req;
	int ret;

	srv_log(srv, "Handling line request from process %d\n", cli->creds.pid);

	line_req = request_lines(srv, req);
	if (!line_req) {
		srv_log(srv, "Line request failed (%s), sending back error response\n",
			strerror(errno));
		send_error_response(cli, srv, errno);
		return;
	}

	line_req->id = get_request_id(srv);
	snprintf(line_req->consumer, sizeof(line_req->consumer), "%s",
		 req->consumer);
	snprintf(resp.req_ok.req_name, CONSUMER_NAME_SIZE, "request%u",
		 line_req->id);

	ret = send_message(&cli->rcv, srv, &resp);
	if (ret) {
		/* The client never learned the id so nobody can release it. */
		free_line_request(line_req);
		srv_drop_client(srv, cli);
		return;
	}

	list_add(&line_req->list, &srv->requests);
	srv_inc_usecnt(srv);
}

static void handle_list_request(struct client *cli, struct server *srv)
{
	unsigned int offsets[MAX_REQ_LINES];
	struct line_request *line_req;
	struct timespec deadline;
	const char *chip_name;
	struct message resp;
	size_t num_offsets;
	int ret;

	srv_log(srv, "Handling list request from process %d\n", cli->creds.pid);

	deadline_from_now(&deadline, SEND_WAIT_TIMEOUT_MS);

	list_for_each_entry(line_req, &srv->requests, list) {
		memset(&resp, 0, sizeof(resp));
		resp.type = MSG_TYPE_REQ_INFO;
		resp.req_info.id = line_req->id;
		snprintf(resp.req_info.consumer, sizeof(resp.req_info.consumer),
			 "%s", line_req->consumer);

		chip_name = gpiod_line_request_get_chip_name(line_req->handle);
		snprintf(resp.req_info.chip_name,
			 sizeof(resp.req_info.chip_name),
			 "%s", chip_name);

		num_offsets = gpiod_line_request_get_requested_offsets(
				line_req->handle, offsets, MAX_REQ_LINES);
		memcpy(resp.req_info.offsets, offsets,
		       num_offsets * sizeof(*offsets));
		resp.req_info.num_offsets = num_offsets;
		memcpy(resp.req_info.line_names, line_req->line_names,
		       num_offsets * sizeof(*line_req->line_names));

		ret = send_message_wait(&cli->rcv, srv, &resp, &deadline);
		if (ret) {
			srv_drop_client(srv, cli);
			return;
		}
	}

	memset(&resp, 0, sizeof(resp));
	resp.type = MSG_TYPE_OK;

	ret = send_message_wait(&cli->rcv, srv, &resp, &deadline);
	if (ret)
		srv_drop_client(srv, cli);
}

static void handle_release_request(struct client *cli, struct server *srv,
				   const struct release *rel)
{
	struct line_request *line_req, *tmp;
	struct message resp;
	int ret;

	srv_log(srv, "Handling release request for id %u from process %d\n",
		rel->id, cli->creds.pid);

	list_for_each_entry_safe(line_req, tmp, &srv->requests, list) {
		if (line_req->id != rel->id)
			continue;

		if (line_req->monitor_client) {
			srv_remove_receiver(srv, &line_req->monitor_rcv);

			memset(&resp, 0, sizeof(resp));
			resp.type = MSG_TYPE_OK;
			send_message(&line_req->monitor_client->rcv, srv,
				     &resp);
		}

		list_del(&line_req->list);
		free_line_request(line_req);
		srv_dec_usecnt(srv);

		memset(&resp, 0, sizeof(resp));
		resp.type = MSG_TYPE_OK;

		ret = send_message(&cli->rcv, srv, &resp);
		if (ret)
			srv_drop_client(srv, cli);

		return;
	}

	srv_log(srv, "No request with id %u to release\n", rel->id);
	send_error_response(cli, srv, ENOENT);
}

static struct line_request *find_request(struct server *srv, unsigned int id)
{
	struct line_request *line_req;

	list_for_each_entry(line_req, &srv->requests, list) {
		if (line_req->id == id)
			return line_req;
	}

	return NULL;
}

static void handle_get_request(struct client *cli, struct server *srv,
				const struct get_req *get)
{
	struct message resp = { .type = MSG_TYPE_GET_OK };
	enum gpiod_line_value values[MAX_REQ_LINES];
	unsigned int offsets[MAX_REQ_LINES];
	struct line_request *line_req;
	size_t num_lines;
	int ret;

	srv_log(srv, "Handling get request for id %u from process %d\n",
		get->id, cli->creds.pid);

	line_req = find_request(srv, get->id);
	if (!line_req) {
		send_error_response(cli, srv, ENOENT);
		return;
	}

	if (get->num_offsets == 0) {
		num_lines = gpiod_line_request_get_num_requested_lines(
							line_req->handle);
		ret = gpiod_line_request_get_values(line_req->handle, values);
	} else {
		num_lines = get->num_offsets;
		memcpy(offsets, get->offsets, num_lines * sizeof(*offsets));
		ret = gpiod_line_request_get_values_subset(line_req->handle,
							   num_lines,
							   offsets,
							   values);
	}
	if (ret) {
		send_error_response(cli, srv, errno);
		return;
	}

	memcpy(resp.get_ok.values, values, num_lines * sizeof(*values));
	resp.get_ok.num_values = num_lines;

	ret = send_message(&cli->rcv, srv, &resp);
	if (ret)
		srv_drop_client(srv, cli);
}

static void handle_set_request(struct client *cli, struct server *srv,
				const struct set_req *set)
{
	enum gpiod_line_value values[MAX_REQ_LINES], setvals[MAX_REQ_LINES];
	struct message resp = { .type = MSG_TYPE_OK };
	unsigned int offsets[MAX_REQ_LINES];
	struct line_request *line_req;
	size_t num_lines;
	int ret;

	srv_log(srv, "Handling set request for id %u from process %d\n",
		set->id, cli->creds.pid);

	line_req = find_request(srv, set->id);
	if (!line_req) {
		send_error_response(cli, srv, ENOENT);
		return;
	}

	if (set->num_offsets == 0) {
		num_lines = gpiod_line_request_get_num_requested_lines(
							line_req->handle);
		if (set->num_values != num_lines) {
			send_error_response(cli, srv, EINVAL);
			return;
		}
		memcpy(values, set->values, num_lines * sizeof(*values));
		ret = gpiod_line_request_set_values(line_req->handle, values);
	} else {
		memcpy(offsets, set->offsets,
		       set->num_offsets * sizeof(*offsets));
		memcpy(setvals, set->values,
		       set->num_offsets * sizeof(*setvals));
		ret = gpiod_line_request_set_values_subset(line_req->handle,
							   set->num_offsets,
							   offsets,
							   setvals);
	}
	if (ret) {
		send_error_response(cli, srv, errno);
		return;
	}

	ret = send_message(&cli->rcv, srv, &resp);
	if (ret)
		srv_drop_client(srv, cli);
}

static bool offsets_match(const unsigned int *req_offsets, size_t num_req,
			  const unsigned int *offsets, size_t num)
{
	size_t i, j;

	if (num != num_req)
		return false;

	for (i = 0; i < num; i++) {
		for (j = 0; j < num_req; j++) {
			if (offsets[i] == req_offsets[j])
				break;
		}

		if (j == num_req)
			return false;

		for (j = i + 1; j < num; j++) {
			if (offsets[j] == offsets[i])
				return false;
		}
	}

	return true;
}

/*
 * libgpiod wants the offsets of a reconfigure in the order of the request.
 * Arrange the values, given for offsets in any order, accordingly. The
 * offsets must be known to match: see offsets_match().
 */
static void order_values(const unsigned int *req_offsets, size_t num,
			 const unsigned int *offsets,
			 const enum gpiod_line_value *values,
			 enum gpiod_line_value *ordered)
{
	size_t i, j;

	for (i = 0; i < num; i++) {
		for (j = 0; j < num; j++) {
			if (offsets[j] == req_offsets[i])
				break;
		}

		ordered[i] = values[j];
	}
}

static void handle_reconfigure_request(struct client *cli, struct server *srv,
					const struct reconfigure *rcfg)
{
	enum gpiod_line_value values[MAX_REQ_LINES], ordered[MAX_REQ_LINES];
	unsigned int offsets[MAX_REQ_LINES], new_offsets[MAX_REQ_LINES];
	struct message resp = { .type = MSG_TYPE_OK };
	struct gpiod_line_settings *settings;
	struct gpiod_line_config *line_cfg;
	struct line_request *line_req;
	struct request_config cfg;
	size_t num_lines;
	int ret;

	srv_log(srv, "Handling reconfigure request for id %u from process %d\n",
		rcfg->id, cli->creds.pid);

	line_req = find_request(srv, rcfg->id);
	if (!line_req) {
		send_error_response(cli, srv, ENOENT);
		return;
	}

	cfg = rcfg->cfg;
	num_lines = gpiod_line_request_get_requested_offsets(
			line_req->handle, offsets, MAX_REQ_LINES);

	/*
	 * Without offsets the values, if any, are in the order of the lines
	 * in the request. Otherwise they belong to the offsets given.
	 */
	if (!cfg.num_offsets) {
		memcpy(cfg.offsets, offsets, num_lines * sizeof(*offsets));
		cfg.num_offsets = num_lines;
	} else {
		memcpy(new_offsets, cfg.offsets,
		       cfg.num_offsets * sizeof(*new_offsets));

		if (!offsets_match(offsets, num_lines, new_offsets,
				   cfg.num_offsets)) {
			send_error_response(cli, srv, EINVAL);
			return;
		}

		memcpy(values, cfg.values, num_lines * sizeof(*values));
		order_values(offsets, num_lines, new_offsets, values, ordered);
		memcpy(cfg.offsets, offsets, num_lines * sizeof(*offsets));
		memcpy(cfg.values, ordered, num_lines * sizeof(*ordered));
	}

	line_cfg = gpiod_line_config_new();
	settings = gpiod_line_settings_new();
	if (!line_cfg || !settings)
		srv_die_oom(srv);

	ret = build_line_config(line_cfg, settings, &cfg);
	if (ret) {
		gpiod_line_settings_free(settings);
		gpiod_line_config_free(line_cfg);
		send_error_response(cli, srv, errno);
		return;
	}

	ret = gpiod_line_request_reconfigure_lines(line_req->handle, line_cfg);
	gpiod_line_settings_free(settings);
	gpiod_line_config_free(line_cfg);
	if (ret) {
		send_error_response(cli, srv, errno);
		return;
	}

	ret = send_message(&cli->rcv, srv, &resp);
	if (ret)
		srv_drop_client(srv, cli);
}

static void monitor_receive(uint32_t events UNUSED, struct receiver *rcv,
			    struct server *srv)
{
	struct line_request *line_req =
			container_of(rcv, struct line_request, monitor_rcv);
	struct gpiod_edge_event *event;
	struct message msg;
	int num_events, i, ret;

	num_events = gpiod_line_request_read_edge_events(line_req->handle,
							 line_req->ev_buf,
							 MONITOR_EVENT_BUF_SIZE);
	if (num_events < 0)
		return;

	for (i = 0; i < num_events; i++) {
		event = gpiod_edge_event_buffer_get_event(line_req->ev_buf, i);

		memset(&msg, 0, sizeof(msg));
		msg.type = MSG_TYPE_EDGE_EVENT;
		msg.edge_ev.timestamp_ns = gpiod_edge_event_get_timestamp_ns(event);
		msg.edge_ev.offset = gpiod_edge_event_get_line_offset(event);
		msg.edge_ev.event_type = gpiod_edge_event_get_event_type(event);

		ret = send_message(&line_req->monitor_client->rcv, srv, &msg);
		if (ret) {
			srv_drop_client(srv, line_req->monitor_client);
			break;
		}
	}
}

static void handle_monitor_request(struct client *cli, struct server *srv,
				   const struct monitor_req *mon)
{
	struct message resp = { .type = MSG_TYPE_OK };
	struct line_request *line_req;
	int ret;

	srv_log(srv, "Handling monitor request for id %u from process %d\n",
		mon->id, cli->creds.pid);

	line_req = find_request(srv, mon->id);
	if (!line_req) {
		send_error_response(cli, srv, ENOENT);
		return;
	}

	if (line_req->monitor_client) {
		send_error_response(cli, srv, EBUSY);
		return;
	}

	line_req->ev_buf = gpiod_edge_event_buffer_new(MONITOR_EVENT_BUF_SIZE);
	if (!line_req->ev_buf) {
		send_error_response(cli, srv, ENOMEM);
		return;
	}

	line_req->monitor_client = cli;
	line_req->monitor_rcv.fd = gpiod_line_request_get_fd(line_req->handle);
	line_req->monitor_rcv.receive = monitor_receive;

	ret = srv_add_receiver(srv, &line_req->monitor_rcv, false);
	if (ret) {
		gpiod_edge_event_buffer_free(line_req->ev_buf);
		line_req->ev_buf = NULL;
		line_req->monitor_client = NULL;
		send_error_response(cli, srv, errno);
		return;
	}

	ret = send_message(&cli->rcv, srv, &resp);
	if (ret)
		srv_drop_client(srv, cli);
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
	if (ret > 0)
		return;
	if (ret < 0) {
		srv_drop_client(srv, cli);
		return;
	}

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
		if (!check_num_offsets(cli, srv, msg.req.cfg.num_offsets))
			break;
		handle_line_request(cli, srv, &msg.req);
		break;
	case MSG_TYPE_RELEASE:
		handle_release_request(cli, srv, &msg.rel);
		break;
	case MSG_TYPE_REQ_LIST:
		handle_list_request(cli, srv);
		break;
	case MSG_TYPE_GET:
		if (!check_num_offsets(cli, srv, msg.get_req.num_offsets))
			break;
		handle_get_request(cli, srv, &msg.get_req);
		break;
	case MSG_TYPE_SET:
		if (!check_num_offsets(cli, srv, msg.set_req.num_offsets))
			break;
		handle_set_request(cli, srv, &msg.set_req);
		break;
	case MSG_TYPE_RECONFIGURE:
		if (!check_num_offsets(cli, srv,
				       msg.reconfigure.cfg.num_offsets))
			break;
		handle_reconfigure_request(cli, srv, &msg.reconfigure);
		break;
	case MSG_TYPE_MONITOR:
		handle_monitor_request(cli, srv, &msg.monitor_req);
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
	socklen_t len;
	int sock, ret;

	sock = accept4(rcv->fd, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
	if (sock < 0) {
		srv_log(srv, "Failed to accept client connection: %s\n",
			strerror(errno));
		return;
	}

	cli = malloc(sizeof(*cli));
	if (!cli)
		srv_die_oom(srv);

	len = sizeof(cli->creds);
	ret = getsockopt(sock, SOL_SOCKET, SO_PEERCRED, &cli->creds, &len);
	if (ret) {
		close(sock);
		free(cli);
		return;
	}

	/*
	 * Abstract sockets are not subject to filesystem permissions. Only
	 * serve the user the server runs as (and root).
	 */
	if (cli->creds.uid != geteuid() && cli->creds.uid != 0) {
		srv_log(srv, "Rejecting connection from process %d of user %u\n",
			cli->creds.pid, cli->creds.uid);
		close(sock);
		free(cli);
		return;
	}

	cli->rcv.fd = sock;
	cli->rcv.receive = client_receive;

	ret = srv_add_receiver(srv, &cli->rcv, false);
	if (ret) {
		close(sock);
		free(cli);
		return;
	}

	list_add(&cli->list, &srv->clients);

	srv_log(srv, "Accepted connection from process %d (uid: %u)\n",
		cli->creds.pid, cli->creds.uid);

	srv_inc_usecnt(srv);
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
		srv_die_perror(srv, "failed to mask signals");

	sigfd = signalfd(-1, &mask, 0);
	if (sigfd < 0)
		srv_die_perror(srv, "failed to create signalfd");

	srv->signal_handler.fd = sigfd;
	srv->signal_handler.receive = signal_receive;

	srv_add_receiver(srv, &srv->signal_handler, true);
}

static void timer_receive(uint32_t events UNUSED, struct receiver *rcv,
			  struct server *srv)
{
	uint64_t exp;
	ssize_t ret;

	ret = read(rcv->fd, &exp, sizeof(exp));
	if (ret < 0)
		srv_die_perror(srv, "failed to read timer data");

	srv_log(srv, "Idle timer expired\n");
	srv->stop = true;
}

static void add_timerfd(struct server *srv)
{
	struct receiver *timer = &srv->idle_timer;

	timer->fd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (timer->fd < 0)
		srv_die_perror(srv, "failed to create the timer file descriptor");

	timer->receive = timer_receive;
}

static void free_server(struct server *srv)
{
	close(srv->epollfd);
	if (srv->rcv.fd >= 0)
		close(srv->rcv.fd);
	/* Filesystem sockets (non-abstract) need to be unlinked. */
	if (srv->socket_path[0] != '\0')
		unlink(srv->socket_path);
	free(srv);
}

static NORETURN void run_server(struct server *srv)
{
	struct epoll_event events[MAX_EPOLL_EVENTS], *event;
	struct line_request *line_req, *req_tmp;
	struct client *cli, *tmp;
	struct receiver *rcv;
	int numev, i;

	add_signalfd(srv);
	add_timerfd(srv);
	arm_idle_timer(srv);

	/*
	 * Closing the write end of the startup pipe without having written
	 * anything tells the client that the server is up and running.
	 */
	if (srv->startup_fd >= 0) {
		close(srv->startup_fd);
		srv->startup_fd = -1;
	}

	srv_log(srv, "gpioctl server started\n");

	while (!srv->stop) {
		numev = epoll_wait(srv->epollfd, events,
				   MAX_EPOLL_EVENTS, 60 * 1000);
		if (numev < 0) {
			if (errno == EINTR)
				continue;

			srv_die_perror(srv, "Failed to poll file descriptors");
		}

		if (numev == 0)
			continue;

		for (i = 0; i < numev; i++) {
			event = &events[i];
			rcv = event->data.ptr;
			if (!rcv)
				continue;

			srv->pending = &events[i + 1];
			srv->num_pending = numev - i - 1;
			rcv->receive(event->events, rcv, srv);
		}

		srv->pending = NULL;
		srv->num_pending = 0;
	}

	srv_log(srv, "gpioctl server exiting\n");

	/*
	 * Close the listening socket before dropping clients so that any
	 * new connection attempt races get ECONNREFUSED rather than
	 * connecting to a server that won't serve them.
	 */
	epoll_ctl(srv->epollfd, EPOLL_CTL_DEL, srv->rcv.fd, NULL);
	close(srv->rcv.fd);
	srv->rcv.fd = -1;
	if (srv->socket_path[0] != '\0') {
		unlink(srv->socket_path);
		srv->socket_path[0] = '\0';
	}

	list_for_each_entry_safe(cli, tmp, &srv->clients, list)
		srv_drop_client(srv, cli);

	list_for_each_entry_safe(line_req, req_tmp, &srv->requests, list) {
		list_del(&line_req->list);
		free_line_request(line_req);
	}

	close(srv->signal_handler.fd);
	close(srv->idle_timer.fd);
	free_server(srv);

	exit(EXIT_SUCCESS);
}

/*
 * Returns NULL if the address is an abstract one that is already in use: some
 * other process was faster to start the server.
 */
static struct server *make_server(const struct sockaddr_un *addr)
{
	struct epoll_event event = {
		.events = EPOLLIN | EPOLLERR,
	};
	struct server *srv;
	int ret, sock;

	sock = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if (sock < 0)
		die_perror("failed to create the server socket");

	ret = bind(sock, (struct sockaddr *)addr, sizeof(*addr));
	if (ret) {
		if (errno == EADDRINUSE && addr->sun_path[0] == '\0') {
			close(sock);
			return NULL;
		}

		die_perror("failed to bind to the unix socket");
	}

	ret = listen(sock, 128);
	if (ret < 0)
		die_perror("failed to start listening on the socket");

	srv = malloc(sizeof(*srv));
	if (!srv)
		die_oom();

	memset(srv, 0, sizeof(*srv));
	srv->startup_fd = -1;
	memcpy(srv->socket_path, addr->sun_path, sizeof(srv->socket_path));

	srv->epollfd = epoll_create1(0);
	if (srv->epollfd < 0)
		die_perror("failed to create the epoll handle");

	list_init(&srv->clients);
	list_init(&srv->requests);
	srv->rcv.fd = sock;
	srv->rcv.receive = server_receive;
	event.data.ptr = &srv->rcv;

	ret = epoll_ctl(srv->epollfd, EPOLL_CTL_ADD, sock, &event);
	if (ret)
		die_perror("failed to add the server socket to the epoll set");

	return srv;
}

static void redirect_fds(struct server *srv)
{
	int fd, ret;

	fd = open("/dev/null", O_RDWR);
	if (fd < 0)
		srv_die_perror(srv, "failed to open /dev/null");

	ret = dup2(fd, STDIN_FILENO);
	if (ret < 0)
		srv_die_perror(srv, "failed to redirect stdin");

	ret = dup2(fd, STDOUT_FILENO);
	if (ret < 0)
		srv_die_perror(srv, "failed to redirect stdout");

	ret = dup2(fd, STDERR_FILENO);
	if (ret < 0)
		srv_die_perror(srv, "failed to redirect stderr");

	if (fd > STDERR_FILENO)
		close(fd);
}

/*
 * Wait for the detached server to either come up, which it signals by closing
 * the write end of the startup pipe, or to report why it can't.
 */
static void wait_for_startup(int rd)
{
	struct pollfd pfd = {
		.fd = rd,
		.events = POLLIN,
	};
	int ret, errnum;
	ssize_t rdlen;

	do {
		ret = poll(&pfd, 1, SERVER_START_TIMEOUT_MS);
	} while (ret < 0 && errno == EINTR);
	if (ret < 0)
		die_perror("failed to poll the startup pipe");
	if (ret == 0)
		die("timed out waiting for the server to start");

	do {
		rdlen = read(rd, &errnum, sizeof(errnum));
	} while (rdlen < 0 && errno == EINTR);
	if (rdlen < 0)
		die_perror("failed to read from the startup pipe");

	/* End of file: the server is up. */
	if (rdlen == 0)
		return;

	errno = errnum;
	die_perror("the server failed to start");
}

static void spawn_server(struct server *srv, int lock_fd)
{
	int startup_pipe[2], ret;
	pid_t pid;

	ret = pipe(startup_pipe);
	if (ret)
		die_perror("failed to create the startup pipe");

	pid = fork();
	if (pid < 0) {
		die_perror("failed to spawn the server process");
	} else if (pid == 0) {
		/*
		 * Server process. Start a new session and fork again to be
		 * reparented to init.
		 */
		close(startup_pipe[0]);
		srv->startup_fd = startup_pipe[1];

		setsid();

		pid = fork();
		if (pid < 0)
			srv_die_perror(srv, "failed to spawn the final server process");
		else if (pid > 0)
			exit(EXIT_SUCCESS);

		prctl(PR_SET_NAME, "gpioctl server");
		redirect_fds(srv);
		if (lock_fd >= 0)
			close(lock_fd);

		run_server(srv);
	} else {
		/*
		 * Client process. Drop our write end of the startup pipe or
		 * we'd never see the end of file, wait for the first fork to
		 * exit and for the server to report back.
		 */
		close(startup_pipe[1]);
		waitpid(pid, NULL, 0);
		wait_for_startup(startup_pipe[0]);
		close(startup_pipe[0]);
	}
}

static void start_server(const struct sockaddr_un *addr, int lock_fd)
{
	struct server *srv;

	srv = make_server(addr);
	if (!srv)
		return;

	spawn_server(srv, lock_fd);
	/*
	 * If we returned, we're the client. The daemon owns the socket file,
	 * so clear the path so that free_server() doesn't unlink it from the
	 * client.
	 */
	srv->socket_path[0] = '\0';
	free_server(srv);
}

/* Returns -1 with errno set to ENOENT or ECONNREFUSED if there's no server. */
static int try_connect(const struct sockaddr_un *addr)
{
	struct ucred creds;
	socklen_t len;
	int sock, ret;

	sock = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if (sock < 0)
		die_perror("failed to create the client socket");

	ret = connect(sock, (struct sockaddr *)addr, sizeof(*addr));
	if (ret) {
		if (errno == ENOENT || errno == ECONNREFUSED) {
			/*
			 * The socket file does not exist or is there but
			 * there's no server bound to it behind.
			 */
			ret = errno;
			close(sock);
			errno = ret;
			return -1;
		}

		die_perror("failed to connect to server");
	}

	/*
	 * Abstract socket names are not owned by anyone so make sure we're
	 * talking to our own server and not to one started by another user.
	 */
	len = sizeof(creds);
	ret = getsockopt(sock, SOL_SOCKET, SO_PEERCRED, &creds, &len);
	if (ret)
		die_perror("failed to get server credentials");

	if (creds.uid != geteuid() && creds.uid != 0)
		die("refusing to talk to a server owned by another user");

	return sock;
}

static void make_addr(struct sockaddr_un *addr, const char *socket_path)
{
	memset(addr, 0, sizeof(*addr));
	addr->sun_family = AF_UNIX;
	if (socket_path) {
		strncpy(addr->sun_path, socket_path,
			sizeof(addr->sun_path) - 1);
	} else {
		snprintf(addr->sun_path + 1, sizeof(addr->sun_path) - 1,
			 "gpioctl-%u", getuid());
	}
}

/*
 * Starting a server on a filesystem socket must be serialized between
 * clients as it involves removing a stale socket file. Take an exclusive
 * lock on a file next to the socket. Abstract sockets need no locking, there
 * are no stale ones. Returns the descriptor of the lock file or -1. Dies if
 * the lock can't be taken in a reasonable time.
 */
static int lock_startup(const struct sockaddr_un *addr)
{
	struct timespec ts = { .tv_nsec = 10 * 1000 * 1000 };
	char path[sizeof(addr->sun_path) + 8];
	int fd, i;

	if (addr->sun_path[0] == '\0')
		return -1;

	snprintf(path, sizeof(path), "%s.lock", addr->sun_path);

	fd = open(path, O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600);
	if (fd < 0)
		die_perror("failed to open the lock file '%s'", path);

	/* Don't wait forever for a client that hangs or a file we can't win. */
	for (i = 0; i < SERVER_START_TIMEOUT_MS / 10; i++) {
		if (i)
			nanosleep(&ts, NULL);

		if (!flock(fd, LOCK_EX | LOCK_NB))
			return fd;

		if (errno != EWOULDBLOCK && errno != EINTR)
			die_perror("failed to lock '%s'", path);
	}

	die("timed out waiting for the lock '%s'", path);
}

static void unlock_startup(int fd)
{
	if (fd < 0)
		return;

	/*
	 * The server process inherits the descriptor from us. Unlock
	 * explicitly as closing ours wouldn't release the lock.
	 */
	flock(fd, LOCK_UN);
	close(fd);
}

/*
 * A server that didn't exit cleanly leaves its socket file behind. It would
 * make bind() fail for the new server so remove it. Only remove actual
 * sockets: connect() also fails with ECONNREFUSED for other file types.
 */
static void remove_stale_socket(const struct sockaddr_un *addr)
{
	struct stat st;

	if (addr->sun_path[0] != '\0' && !lstat(addr->sun_path, &st) &&
	    S_ISSOCK(st.st_mode))
		unlink(addr->sun_path);
}

/*
 * Wait for a server that's being started, possibly by another process, to
 * become available.
 */
static int wait_for_server(const struct sockaddr_un *addr)
{
	struct timespec ts = { .tv_nsec = 10 * 1000 * 1000 };
	int sock, i;

	for (i = 0; i < SERVER_START_TIMEOUT_MS / 10; i++) {
		sock = try_connect(addr);
		if (sock >= 0)
			return sock;

		nanosleep(&ts, NULL);
	}

	return -1;
}

static int get_connection(const char *socket_path)
{
	struct sockaddr_un addr;
	int sock, lock_fd;

	make_addr(&addr, socket_path);

	sock = try_connect(&addr);
	if (sock >= 0)
		return sock;

	lock_fd = lock_startup(&addr);
	if (lock_fd >= 0) {
		/* Another client may have started the server meanwhile. */
		sock = try_connect(&addr);
		if (sock >= 0) {
			unlock_startup(lock_fd);
			return sock;
		}

		if (errno == ECONNREFUSED)
			remove_stale_socket(&addr);
	}

	start_server(&addr, lock_fd);
	unlock_startup(lock_fd);

	sock = wait_for_server(&addr);
	if (sock < 0)
		die("timed out waiting for the server to start");

	return sock;
}

static int debug_main(int argc UNUSED, char **argv UNUSED,
		      const struct config *cfg)
{
	struct sockaddr_un addr;
	struct server *srv;

	make_addr(&addr, cfg->socket_path);
	srv = make_server(&addr);
	if (!srv)
		die("a server is already running");

	srv->logging = true;
	run_server(srv);
}

static void client_send_msg(int sock, const struct message *msg)
{
	ssize_t ret;

	ret = send(sock, msg, msg_size(msg->type), MSG_NOSIGNAL);
	if (ret < 0)
		die_perror("failed to send data to server");
}

static void client_recv_reply(int sock, struct message *msg)
{
	struct pollfd fd = {
		.fd = sock,
		.events = POLLIN | POLLPRI,
	};
	ssize_t recvd;
	int ret;

	memset(msg, 0, sizeof(*msg));

	ret = poll(&fd, 1, 10 * 1000);
	if (ret < 0)
		die_perror("failed while polling server for response");
	if (ret == 0)
		die("timeout while waiting for server to respond");

	recvd = recv(sock, msg, sizeof(*msg), MSG_TRUNC);
	if (recvd < 0)
		die_perror("failed to receive data from server");
	if (recvd == 0)
		die("server disconnected unexpectedly");
	if (!msg_size_valid(msg, recvd))
		die("unexpected message size received from server");
}

static void client_recv_event(int sock, struct message *msg)
{
	ssize_t recvd;

	recvd = recv(sock, msg, sizeof(*msg), MSG_TRUNC);
	if (recvd < 0)
		die_perror("failed to receive data from server");
	if (recvd == 0)
		die("server disconnected unexpectedly");
	if (!msg_size_valid(msg, recvd))
		die("unexpected message size received from server");
}

static void client_exchange(int sock, struct message *msg)
{
	client_send_msg(sock, msg);
	client_recv_reply(sock, msg);
}

static void PRINTF(3, 4) client_validate_response(const struct message *msg,
						  unsigned int expected,
						  const char *fmt, ...)
{
	va_list va;

	if (msg->type == MSG_TYPE_ERROR) {
		va_start(va, fmt);
		fprintf(stderr, "%s: ", get_prog_name());
		vfprintf(stderr, fmt, va);
		fprintf(stderr, ": %s\n", strerror(msg->err.errnum));
		va_end(va);
		exit(EXIT_FAILURE);
	}

	if (msg->type != expected)
		die("server responded with unexpected message type: %d",
		    msg->type);
}

static void print_request_help(void)
{
	printf("Usage: %s [GLOBAL OPTIONS] request [OPTIONS] <line>[=value]...\n",
	       get_prog_name());
	printf("\n");
	printf("Request one or more GPIO lines, keeping them held by the daemon.\n");
	printf("\n");
	printf("Lines are specified by offset or by name if --by-name is set globally.\n");
	printf("The chip must be identified with the global --chip option when lines are\n");
	printf("specified by offset.\n");
	printf("\n");
	printf("In output mode, values may optionally be set per-line using the\n");
	printf("'line=value' syntax. Values: '0'/'1', 'active'/'inactive', 'on'/'off'.\n");
	printf("\n");
	printf("Options:\n");
	printf("  -i, --input\t\t\tset direction to input\n");
	printf("  -o, --output\t\t\tset direction to output\n");
	printf("      --open-drain\t\tset drive mode to open-drain (output only)\n");
	printf("      --open-source\t\tset drive mode to open-source (output only)\n");
	printf("      --push-pull\t\tset drive mode to push-pull (output only)\n");
	printf("  -l, --active-low\t\ttreat the lines as active low\n");
	printf("      --bias-disabled\t\tset the line bias to disabled\n");
	printf("      --pull-down\t\tset the line bias to pull-down\n");
	printf("      --pull-up\t\t\tset the line bias to pull-up\n");
	printf("      --rising-edge\t\tmonitor rising edges (implies --input)\n");
	printf("      --falling-edge\t\tmonitor falling edges (implies --input)\n");
	printf("      --both-edges\t\tmonitor rising and falling edges (implies --input)\n");
	printf("      --clock-hte\t\tuse HTE clock for edge event timestamps\n");
	printf("      --clock-monotonic\t\tuse monotonic clock for edge event timestamps\n");
	printf("      --clock-realtime\t\tuse realtime clock for edge event timestamps\n");
	printf("  -p, --debounce-period <period>\n");
	printf("\t\t\tdebounce the lines with the specified period\n");
	printf("  -h, --help\t\t\tdisplay this help and exit\n");
	print_period_help();
}

struct request_opts {
	bool input;
	bool output;
	bool active_low;
	bool pull_up;
	bool pull_down;
	bool bias_disabled;
	bool push_pull;
	bool open_drain;
	bool open_source;
	bool rising_edge;
	bool falling_edge;
	bool clock_monotonic;
	bool clock_realtime;
	bool clock_hte;
	unsigned long long debounce_period_us;
};

static void validate_request_opts(const struct request_opts *opts)
{
	int count;

	if (opts->input && opts->output)
		die("--input and --output are mutually exclusive");

	if ((opts->rising_edge || opts->falling_edge) && opts->output)
		die("edge detection is only available in input mode");

	count = 0;
	count += opts->push_pull;
	count += opts->open_drain;
	count += opts->open_source;
	if (count > 1)
		die("--push-pull, --open-drain and --open-source are mutually exclusive");
	if (count > 0 && !opts->output)
		die("--push-pull, --open-drain and --open-source are only available in output mode");

	count = 0;
	count += opts->pull_up;
	count += opts->pull_down;
	count += opts->bias_disabled;
	if (count > 1)
		die("--pull-up, --pull-down and --bias-disabled are mutually exclusive");

	count = 0;
	if (opts->clock_monotonic)
		count++;
	if (opts->clock_realtime)
		count++;
	if (opts->clock_hte)
		count++;

	if (count > 1)
		die("--clock-monotonic, --clock-realtime and --clock-hte are mutually exclusive");

	if (count > 0 &&
	    !opts->rising_edge && !opts->falling_edge)
		die("--clock-* options require edge detection");

	if (opts->debounce_period_us &&
	    !opts->rising_edge && !opts->falling_edge)
		die("--debounce-period requires edge detection");
}

static void parse_request_cmdline(int *argc, char ***argv,
				  struct request_opts *opts, bool permute,
				  void (*help_fn)(void))
{
	static const struct option longopts[] = {
		{ "active-low",		no_argument,		NULL,	'l' },
		{ "bias-disabled",	no_argument,		NULL,	'D' },
		{ "both-edges",		no_argument,		NULL,	'E' },
		{ "clock-hte",		no_argument,		NULL,	'H' },
		{ "clock-monotonic",	no_argument,		NULL,	'M' },
		{ "clock-realtime",	no_argument,		NULL,	'R' },
		{ "debounce-period",	required_argument,	NULL,	'p' },
		{ "falling-edge",	no_argument,		NULL,	'F' },
		{ "help",		no_argument,		NULL,	'h' },
		{ "input",		no_argument,		NULL,	'i' },
		{ "open-drain",		no_argument,		NULL,	'N' },
		{ "open-source",	no_argument,		NULL,	'S' },
		{ "output",		no_argument,		NULL,	'o' },
		{ "pull-down",		no_argument,		NULL,	'P' },
		{ "pull-up",		no_argument,		NULL,	'U' },
		{ "push-pull",		no_argument,		NULL,	'X' },
		{ "rising-edge",	no_argument,		NULL,	'G' },
		{ GETOPT_NULL_LONGOPT }
	};
	const char *shortopts = permute ? "lp:hio" : "+lp:hio";

	int opti, optc;

	optind = 0;

	for (;;) {
		optc = getopt_long(*argc, *argv, shortopts, longopts, &opti);
		if (optc < 0)
			break;

		switch (optc) {
		case 'D':
			opts->bias_disabled = true;
			break;
		case 'E':
			opts->rising_edge = opts->falling_edge = true;
			break;
		case 'F':
			opts->falling_edge = true;
			break;
		case 'G':
			opts->rising_edge = true;
			break;
		case 'H':
			opts->clock_hte = true;
			break;
		case 'i':
			opts->input = true;
			break;
		case 'l':
			opts->active_low = true;
			break;
		case 'M':
			opts->clock_monotonic = true;
			break;
		case 'N':
			opts->open_drain = true;
			break;
		case 'o':
			opts->output = true;
			break;
		case 'p':
			opts->debounce_period_us = parse_period_or_die(optarg);
			break;
		case 'P':
			opts->pull_down = true;
			break;
		case 'R':
			opts->clock_realtime = true;
			break;
		case 'S':
			opts->open_source = true;
			break;
		case 'U':
			opts->pull_up = true;
			break;
		case 'X':
			opts->push_pull = true;
			break;
		case 'h':
			help_fn ? help_fn() : print_request_help();
			exit(EXIT_SUCCESS);
		case '?':
			die("try %s request --help", get_prog_name());
		default:
			abort();
		}
	}

	*argc -= optind;
	*argv += optind;
}

static void apply_opts_to_config(struct request_config *cfg,
				 struct request_opts *opts,
				 int argc, char **argv)
{
	int i, num_values = 0;
	char *eq;

	if ((opts->rising_edge || opts->falling_edge) && !opts->output)
		opts->input = true;

	/* GPIOD_LINE_VALUE_ERROR marks a line for which no value was given. */
	for (i = 0; i < MAX_REQ_LINES; i++)
		cfg->values[i] = GPIOD_LINE_VALUE_ERROR;

	/* argv entries must be bare line names/offsets for line resolution. */
	for (i = 0; i < argc; i++) {
		eq = strchr(argv[i], '=');

		if (eq) {
			if (!opts->output)
				die("output values can only be set in output mode");
			cfg->values[i] = parse_line_value_or_die(eq + 1);
			*eq = '\0';
			num_values++;
		}
	}

	if (num_values && num_values != argc)
		die("if values are set, they must be set for all lines");

	cfg->active_low = opts->active_low;

	if (opts->pull_up)
		cfg->bias = GPIOD_LINE_BIAS_PULL_UP;
	else if (opts->pull_down)
		cfg->bias = GPIOD_LINE_BIAS_PULL_DOWN;
	else if (opts->bias_disabled)
		cfg->bias = GPIOD_LINE_BIAS_DISABLED;

	if (opts->push_pull)
		cfg->drive = GPIOD_LINE_DRIVE_PUSH_PULL;
	else if (opts->open_drain)
		cfg->drive = GPIOD_LINE_DRIVE_OPEN_DRAIN;
	else if (opts->open_source)
		cfg->drive = GPIOD_LINE_DRIVE_OPEN_SOURCE;

	if (opts->output)
		cfg->direction = GPIOD_LINE_DIRECTION_OUTPUT;
	else if (opts->input)
		cfg->direction = GPIOD_LINE_DIRECTION_INPUT;

	if (opts->rising_edge && opts->falling_edge)
		cfg->edge = GPIOD_LINE_EDGE_BOTH;
	else if (opts->rising_edge)
		cfg->edge = GPIOD_LINE_EDGE_RISING;
	else if (opts->falling_edge)
		cfg->edge = GPIOD_LINE_EDGE_FALLING;

	if (opts->clock_monotonic)
		cfg->event_clock = GPIOD_LINE_CLOCK_MONOTONIC;
	else if (opts->clock_realtime)
		cfg->event_clock = GPIOD_LINE_CLOCK_REALTIME;
	else if (opts->clock_hte)
		cfg->event_clock = GPIOD_LINE_CLOCK_HTE;

	if (opts->debounce_period_us)
		cfg->debounce_period_us = opts->debounce_period_us;
}

static int request_main(int argc, char **argv, const struct config *cfg)
{
	struct message msg = { .type = MSG_TYPE_REQUEST };
	struct line_resolver *resolver;
	struct request *req = &msg.req;
	struct request_opts opts = { };
	int sock, i;

	parse_request_cmdline(&argc, &argv, &opts, false, NULL);
	validate_request_opts(&opts);

	apply_opts_to_config(&req->cfg, &opts, argc, argv);

	resolver = resolve_lines(argc, argv, cfg->chip_id, cfg->strict,
				 cfg->by_name);
	validate_resolution(resolver, cfg->chip_id);

	if (resolver->num_chips > 1)
		die("can only manipulate lines from one chip at a time");

	if (!resolver->num_lines)
		die("at least one line must be specified");

	if (resolver->num_lines > MAX_REQ_LINES)
		die("can only handle up to %u lines", MAX_REQ_LINES);

	snprintf(req->chip_path, sizeof(req->chip_path), "%s",
		 resolver->chips[resolver->lines[0].chip_num].path);

	snprintf(req->consumer, sizeof(req->consumer), "%s", cfg->consumer);

	for (i = 0; i < resolver->num_lines; i++)
		req->cfg.offsets[i] = resolver->lines[i].offset;
	req->cfg.num_offsets = resolver->num_lines;

	sock = get_connection(cfg->socket_path);

	client_exchange(sock, &msg);
	client_validate_response(&msg, MSG_TYPE_REQ_OK, "unable to request lines");
	printf("%s\n", msg.req_ok.req_name);

	free_line_resolver(resolver);
	close(sock);

	return EXIT_SUCCESS;
}

static void parse_help_only(int *argc, char ***argv,
			    void (*help)(void), const char *cmd)
{
	static const struct option longopts[] = {
		{ "help", no_argument, NULL, 'h' },
		{ GETOPT_NULL_LONGOPT }
	};

	int optc;

	optind = 0;
	while ((optc = getopt_long(*argc, *argv, "+h", longopts, NULL)) >= 0) {
		switch (optc) {
		case 'h':
			help();
			exit(EXIT_SUCCESS);
		case '?':
			die("try %s %s --help", get_prog_name(), cmd);
		default:
			abort();
		}
	}
	*argc -= optind;
	*argv += optind;
}

static void print_release_help(void)
{
	printf("Usage: %s [GLOBAL OPTIONS] release <request>\n",
	       get_prog_name());
	printf("\n");
	printf("Release a GPIO line request held by the daemon.\n");
	printf("\n");
	printf("<request> is the name of a request as reported by the 'requests'\n");
	printf("sub-command (e.g. 'request0'); the bare id ('0') is also accepted.\n");
	printf("\n");
	printf("Options:\n");
	printf("  -h, --help\t\tdisplay this help and exit\n");
}

static bool try_parse_request_id(const char *arg, unsigned int *out)
{
	const char *s = arg;
	unsigned long id;
	char *end;

	if (strncmp(s, "request", 7) == 0)
		s += 7;
	if (*s == '\0')
		return false;

	errno = 0;
	id = strtoul(s, &end, 10);
	if (errno || *end != '\0' || id > UINT_MAX)
		return false;

	*out = id;

	return true;
}

static bool is_request_id(const char *arg)
{
	unsigned int id;

	return try_parse_request_id(arg, &id);
}

static unsigned int parse_request_id(const char *arg)
{
	unsigned int id;

	if (!try_parse_request_id(arg, &id))
		die("invalid request name: '%s'", arg);

	return id;
}

static int release_main(int argc, char **argv, const struct config *cfg UNUSED)
{
	struct message msg = { .type = MSG_TYPE_RELEASE };
	unsigned int id;
	int sock;

	parse_help_only(&argc, &argv, print_release_help, "release");

	if (argc < 1)
		die("exactly one request must be specified, try %s release --help",
		    get_prog_name());
	if (argc > 1)
		die("only one request can be released at a time");

	id = parse_request_id(argv[0]);
	msg.rel.id = id;

	sock = get_connection(cfg->socket_path);

	client_exchange(sock, &msg);
	client_validate_response(&msg, MSG_TYPE_OK,
				 "unable to release request %u", id);
	close(sock);

	return EXIT_SUCCESS;
}

static int stop_main(int argc UNUSED, char **argv UNUSED,
		     const struct config *cfg)
{
	struct message msg = { .type = MSG_TYPE_STOP };
	int sock = get_connection(cfg->socket_path);

	client_exchange(sock, &msg);
	client_validate_response(&msg, MSG_TYPE_OK, "unable to stop daemon");
	close(sock);

	return EXIT_SUCCESS;
}

static int ping_main(int argc UNUSED, char **argv UNUSED,
		     const struct config *cfg)
{
	struct message msg = { .type = MSG_TYPE_PING };
	int sock = get_connection(cfg->socket_path);

	client_exchange(sock, &msg);
	client_validate_response(&msg, MSG_TYPE_OK, "unable to ping daemon");
	close(sock);

	return EXIT_SUCCESS;
}

static void print_request_info(const struct request_info *info, bool unquoted)
{
	unsigned int i;

	printf("request%u (%s) consumer=%s%s%s offsets: [",
	       info->id, info->chip_name,
	       unquoted ? "" : "\"",
	       info->consumer,
	       unquoted ? "" : "\"");

	for (i = 0; i < info->num_offsets; i++)
		printf("%s%u", i ? ", " : "", info->offsets[i]);

	printf("]\n");
}

static void print_requests_help(void)
{
	printf("Usage: %s [GLOBAL OPTIONS] requests\n", get_prog_name());
	printf("\n");
	printf("List all GPIO line requests currently held by the daemon.\n");
	printf("\n");
	printf("Options:\n");
	printf("  -h, --help\t\tdisplay this help and exit\n");
	printf("Use the global --unquoted option to suppress quoting of consumer names.\n");
}

static int requests_main(int argc, char **argv, const struct config *cfg)
{
	struct message msg = { .type = MSG_TYPE_REQ_LIST };
	int sock;

	parse_help_only(&argc, &argv, print_requests_help, "requests");

	sock = get_connection(cfg->socket_path);

	client_send_msg(sock, &msg);

	for (;;) {
		client_recv_reply(sock, &msg);

		if (msg.type == MSG_TYPE_REQ_INFO) {
			print_request_info(&msg.req_info, cfg->unquoted);
			continue;
		}

		client_validate_response(&msg, MSG_TYPE_OK, "unable to list requests");
		break;
	}

	close(sock);

	return EXIT_SUCCESS;
}

static struct request_info *collect_req_infos(int sock, unsigned int *num_out)
{
	struct message msg = { .type = MSG_TYPE_REQ_LIST };
	struct request_info *infos = NULL, *tmp;
	unsigned int count = 0;

	client_send_msg(sock, &msg);

	for (;;) {
		client_recv_reply(sock, &msg);

		if (msg.type == MSG_TYPE_OK)
			break;
		if (msg.type != MSG_TYPE_REQ_INFO)
			die("unexpected message type %d from daemon", msg.type);

		tmp = realloc(infos, (count + 1) * sizeof(*infos));
		if (!tmp)
			die_oom();
		infos = tmp;
		infos[count++] = msg.req_info;
	}

	*num_out = count;
	return infos;
}

struct resolved_name {
	unsigned int req_id;
	unsigned int offset;
};

static void resolve_names(const struct request_info *infos,
			  unsigned int num_infos, int argc, char **argv,
			  struct resolved_name *resolved)
{
	unsigned int i, j, k, num_names = argc;
	bool found;

	for (i = 0; i < num_names; i++) {
		found = false;

		for (j = 0; j < num_infos; j++) {
			for (k = 0; k < infos[j].num_offsets; k++) {
				if (infos[j].line_names[k][0] == '\0')
					continue;
				if (strcmp(infos[j].line_names[k], argv[i]) != 0)
					continue;
				if (found)
					die("line '%s' is held by multiple requests",
					    argv[i]);
				found = true;
				resolved[i].req_id = infos[j].id;
				resolved[i].offset = infos[j].offsets[k];
			}
		}

		if (!found)
			die("no held request contains a line named '%s'",
			    argv[i]);
	}
}

static void print_get_help(void)
{
	printf("Usage: %s [GLOBAL OPTIONS] get <request-or-line>...\n",
	       get_prog_name());
	printf("\n");
	printf("Read the current value of one or more GPIO lines or a request held by the daemon.\n");
	printf("\n");
	printf("Each argument is either:\n");
	printf("  - a request name or bare id as reported by the 'requests' sub-command\n");
	printf("    (all values are printed in offset order), or\n");
	printf("  - one or more GPIO line names; each value is printed as \"name\"=value.\n");
	printf("\n");
	printf("Options:\n");
	printf("  -h, --help\t\tdisplay this help and exit\n");
	printf("\n");
	printf("Use the global --unquoted option to suppress quoting of line names.\n");
	printf("Use the global --numeric option to print bare 0/1 values instead of names.\n");
}

typedef void (*group_fn)(unsigned int, const unsigned int *,
			 unsigned int, void *);

static void for_each_request_group(const struct resolved_name *resolved,
				   unsigned int n, group_fn fn, void *ctx)
{
	unsigned int pos[MAX_REQ_LINES], i, j, cnt;
	bool done[MAX_REQ_LINES];

	/* A line may be repeated so this can exceed the size of any request. */
	if (n > MAX_REQ_LINES)
		die("can only handle up to %u lines", MAX_REQ_LINES);

	memset(done, 0, sizeof(done));

	for (i = 0; i < n; i++) {
		if (done[i])
			continue;

		cnt = 0;
		for (j = i; j < n; j++) {
			if (resolved[j].req_id == resolved[i].req_id) {
				pos[cnt++] = j;
				done[j] = true;
			}
		}
		fn(resolved[i].req_id, pos, cnt, ctx);
	}
}

struct get_group_ctx {
	int sock;
	const struct resolved_name *resolved;
	enum gpiod_line_value *values;
};

static void get_group(unsigned int req_id, const unsigned int *pos,
		      unsigned int cnt, void *data)
{
	struct message msg = {
		.type = MSG_TYPE_GET,
		.get_req.id = req_id,
		.get_req.num_offsets = cnt,
	};

	struct get_group_ctx *ctx = data;
	unsigned int j;

	for (j = 0; j < cnt; j++)
		msg.get_req.offsets[j] = ctx->resolved[pos[j]].offset;

	client_exchange(ctx->sock, &msg);
	client_validate_response(&msg, MSG_TYPE_GET_OK,
				 "unable to get line values for request %u",
				 req_id);

	for (j = 0; j < cnt; j++)
		ctx->values[pos[j]] = msg.get_ok.values[j];
}

static void get_request(int sock, char **argv, const struct config *cfg)
{
	struct message msg = { .type = MSG_TYPE_GET };
	unsigned int id, i;

	id = parse_request_id(argv[0]);
	msg.get_req.id = id;
	msg.get_req.num_offsets = 0;

	client_exchange(sock, &msg);
	client_validate_response(&msg, MSG_TYPE_GET_OK,
				 "unable to get line values for request %u",
				 id);

	for (i = 0; i < msg.get_ok.num_values; i++) {
		if (i)
			putchar(' ');
		if (cfg->numeric)
			printf("%d", msg.get_ok.values[i]);
		else
			printf("%s", msg.get_ok.values[i] ==
			       GPIOD_LINE_VALUE_ACTIVE ? "active" : "inactive");
	}
	printf("\n");
}

static void get_by_name(int sock, int argc, char **argv,
			const struct config *cfg)
{
	unsigned int num_infos, num_names = argc, i;
	struct get_group_ctx ctx;
	struct resolved_name *resolved;
	enum gpiod_line_value *values;
	struct request_info *infos;

	infos = collect_req_infos(sock, &num_infos);
	resolved = malloc(num_names * sizeof(*resolved));
	if (!resolved)
		die_oom();

	resolve_names(infos, num_infos, argc, argv, resolved);
	free(infos);

	values = malloc(num_names * sizeof(*values));
	if (!values)
		die_oom();

	ctx.sock = sock;
	ctx.resolved = resolved;
	ctx.values = values;
	for_each_request_group(resolved, num_names, get_group, &ctx);

	for (i = 0; i < num_names; i++) {
		if (i)
			printf(" ");
		if (cfg->numeric)
			printf("%d", values[i]);
		else
			printf(cfg->unquoted ? "%s=%s" : "\"%s\"=%s", argv[i],
			       values[i] == GPIOD_LINE_VALUE_ACTIVE ?
			       "active" : "inactive");
	}
	printf("\n");

	free(values);
	free(resolved);
}

static int get_main(int argc, char **argv, const struct config *cfg)
{
	int sock;

	parse_help_only(&argc, &argv, print_get_help, "get");

	if (argc < 1)
		die("request or line name must be specified, try %s get --help",
		    get_prog_name());

	sock = get_connection(cfg->socket_path);

	if (is_request_id(argv[0])) {
		if (argc > 1)
			die("only one request can be queried at a time");
		get_request(sock, argv, cfg);
	} else {
		get_by_name(sock, argc, argv, cfg);
	}

	close(sock);
	return EXIT_SUCCESS;
}

static void print_set_help(void)
{
	printf("Usage: %s [GLOBAL OPTIONS] set <request> <value>...\n",
	       get_prog_name());
	printf("       %s [GLOBAL OPTIONS] set <request> <offset>=<value>...\n",
	       get_prog_name());
	printf("       %s [GLOBAL OPTIONS] set <line>=<value>...\n",
	       get_prog_name());
	printf("\n");
	printf("Update the output values of GPIO lines or a request held by the daemon.\n");
	printf("\n");
	printf("The first form takes a request name or bare id and values for all lines\n");
	printf("in the request in offset order (count must match).\n");
	printf("The second form takes a request name or bare id and offset=value pairs\n");
	printf("to update a subset of lines.\n");
	printf("The third form takes one or more GPIO line names with values in\n");
	printf("'name=value' syntax; name resolution is done by the client.\n");
	printf("Accepted values: '0'/'1', 'active'/'inactive', 'on'/'off'.\n");
	printf("\n");
	printf("Options:\n");
	printf("  -h, --help\t\tdisplay this help and exit\n");
}

struct set_group_ctx {
	int sock;
	const struct resolved_name *resolved;
	const enum gpiod_line_value *values;
};

static void set_group(unsigned int req_id, const unsigned int *pos,
		      unsigned int cnt, void *data)
{
	struct set_group_ctx *ctx = data;
	struct message msg = {
		.type = MSG_TYPE_SET,
		.set_req.id = req_id,
		.set_req.num_offsets = cnt,
		.set_req.num_values = cnt,
	};
	unsigned int j;

	for (j = 0; j < cnt; j++) {
		msg.set_req.offsets[j] = ctx->resolved[pos[j]].offset;
		msg.set_req.values[j] = ctx->values[pos[j]];
	}

	client_exchange(ctx->sock, &msg);
	client_validate_response(&msg, MSG_TYPE_OK,
				 "unable to set line values for request %u",
				 req_id);
}

static void set_by_name(int sock, int argc, char **argv)
{
	unsigned int num_infos, num_names = argc;
	struct set_group_ctx ctx;
	struct resolved_name *resolved;
	enum gpiod_line_value *values;
	struct request_info *infos;
	char **names;

	names = malloc(num_names * sizeof(*names));
	values = malloc(num_names * sizeof(*values));
	resolved = malloc(num_names * sizeof(*resolved));
	if (!names || !values || !resolved)
		die_oom();

	parse_line_values_or_die(argc, argv, names, values);

	infos = collect_req_infos(sock, &num_infos);
	resolve_names(infos, num_infos, argc, (char **)names, resolved);
	free(infos);

	ctx.sock = sock;
	ctx.resolved = resolved;
	ctx.values = values;
	for_each_request_group(resolved, num_names, set_group, &ctx);

	free(names);
	free(values);
	free(resolved);
}

static void set_request(int sock, int argc, char **argv)
{
	struct message msg = { };
	unsigned int id, i, cnt, num_args = argc;
	bool offset_mode, has_eq;
	unsigned long off;
	const char *eq;
	char *end;

	if (argc < 2)
		die("request and at least one value required, try %s set --help",
		    get_prog_name());

	eq = strchr(argv[1], '=');
	offset_mode = (eq != NULL);

	for (i = 1; i < num_args; i++) {
		has_eq = (strchr(argv[i], '=') != NULL);
		if (has_eq != offset_mode)
			die("cannot mix positional values and offset=value pairs");
	}

	msg.type = MSG_TYPE_SET;
	id = parse_request_id(argv[0]);
	msg.set_req.id = id;

	if (offset_mode) {
		cnt = argc - 1;
		if (cnt > MAX_REQ_LINES)
			die("too many offset=value pairs specified");

		for (i = 0; i < cnt; i++) {
			eq = strchr(argv[i + 1], '=');
			if (!eq)
				die("expected offset=value, got '%s'",
				    argv[i + 1]);
			errno = 0;
			off = strtoul(argv[i + 1], &end, 10);
			if (errno || end == argv[i + 1] || end != eq ||
			    off > UINT_MAX)
				die("invalid offset in '%s'", argv[i + 1]);
			msg.set_req.offsets[i] = (unsigned int)off;
			msg.set_req.values[i] = parse_line_value_or_die(eq + 1);
		}
		msg.set_req.num_offsets = cnt;
		msg.set_req.num_values = cnt;
	} else {
		cnt = argc - 1;
		if (cnt > MAX_REQ_LINES)
			die("too many values specified");

		for (i = 0; i < cnt; i++)
			msg.set_req.values[i] =
				parse_line_value_or_die(argv[i + 1]);

		msg.set_req.num_offsets = 0;
		msg.set_req.num_values = cnt;
	}

	client_exchange(sock, &msg);
	client_validate_response(&msg, MSG_TYPE_OK,
				 "unable to set line values for request %u",
				 id);
}

static int set_main(int argc, char **argv, const struct config *cfg)
{
	const char *eq;
	int sock;

	parse_help_only(&argc, &argv, print_set_help, "set");

	if (argc < 1)
		die("request and value or line=value required, try %s set --help",
		    get_prog_name());

	sock = get_connection(cfg->socket_path);

	eq = strchr(argv[0], '=');
	if (eq && !is_request_id(argv[0]))
		set_by_name(sock, argc, argv);
	else
		set_request(sock, argc, argv);

	close(sock);
	return EXIT_SUCCESS;
}

static void print_monitor_help(void)
{
	printf("Usage: %s [GLOBAL OPTIONS] monitor [OPTIONS] <request-or-line>...\n",
	       get_prog_name());
	printf("\n");
	printf("Monitor edge events on a GPIO line request held by the daemon.\n");
	printf("\n");
	printf("Each argument is either:\n");
	printf("  - a request name or bare id as reported by the 'requests' sub-command\n");
	printf("    (all edge events from that request are printed), or\n");
	printf("  - one or more GPIO line names; events are filtered to those lines only.\n");
	printf("    All named lines must belong to the same request.\n");
	printf("The request must have been made with edge detection enabled.\n");
	printf("\n");
	printf("Options:\n");
	printf("      --banner\t\t\tdisplay a banner on successful startup\n");
	printf("  -F, --format <fmt>\t\tspecify a custom output format\n");
	printf("  -h, --help\t\t\tdisplay this help and exit\n");
	printf("  -I, --initial-state\t\tprint initial states of the lines\n");
	printf("      --localtime\t\tformat timestamps as local time\n");
	printf("  -n, --num-events <num>\texit after receiving num events\n");
	printf("      --utc\t\t\tformat timestamps as UTC\n");
	print_event_format_help();
	printf("\n");
	printf("The initial-state printout honors the global --numeric and --unquoted\n");
	printf("options, the same as the 'get' sub-command.\n");
}

struct monitor_opts {
	bool banner;
	bool initial_state;
	int num_events;
	int timestamp_fmt;
	const char *fmt;
};

static void parse_monitor_cmdline(int *argc, char ***argv,
				  struct monitor_opts *opts)
{
	static const struct option longopts[] = {
		{ "banner",		no_argument,		NULL,	'-' },
		{ "format",		required_argument,	NULL,	'F' },
		{ "help",		no_argument,		NULL,	'h' },
		{ "initial-state",	no_argument,		NULL,	'I' },
		{ "localtime",		no_argument,		NULL,	'L' },
		{ "num-events",		required_argument,	NULL,	'n' },
		{ "utc",		no_argument,		NULL,	'U' },
		{ GETOPT_NULL_LONGOPT }
	};
	static const char *const shortopts = "+F:hn:I";

	int opti, optc;

	optind = 0;

	for (;;) {
		optc = getopt_long(*argc, *argv, shortopts, longopts, &opti);
		if (optc < 0)
			break;

		switch (optc) {
		case '-':
			opts->banner = true;
			break;
		case 'F':
			opts->fmt = optarg;
			break;
		case 'I':
			opts->initial_state = true;
			break;
		case 'L':
			opts->timestamp_fmt = 2;
			break;
		case 'n':
			opts->num_events = parse_uint_or_die(optarg);
			break;
		case 'U':
			opts->timestamp_fmt = 1;
			break;
		case 'h':
			print_monitor_help();
			exit(EXIT_SUCCESS);
		case '?':
			die("try %s monitor --help", get_prog_name());
		default:
			abort();
		}
	}

	*argc -= optind;
	*argv += optind;
}

static unsigned int monitor_resolve(int sock, int argc, char **argv,
				    struct resolved_name **resolved_out,
				    unsigned int *num_filter_out,
				    struct request_info **info_out)
{
	unsigned int num_infos, id, i, num_names = argc;
	struct resolved_name *resolved = NULL;
	struct request_info *infos;

	infos = collect_req_infos(sock, &num_infos);

	if (is_request_id(argv[0])) {
		if (argc > 1)
			die("only one request can be monitored at a time");

		id = parse_request_id(argv[0]);
		num_names = 0;
	} else {
		resolved = malloc(num_names * sizeof(*resolved));
		if (!resolved)
			die_oom();

		resolve_names(infos, num_infos, argc, argv, resolved);
		id = resolved[0].req_id;
		for (i = 1; i < num_names; i++) {
			if (resolved[i].req_id != id)
				die("lines belong to different requests");
		}
	}

	/* Keep a copy of the request's info for line and chip names. */
	*info_out = NULL;
	for (i = 0; i < num_infos; i++) {
		if (infos[i].id != id)
			continue;

		*info_out = malloc(sizeof(**info_out));
		if (!*info_out)
			die_oom();
		**info_out = infos[i];
		break;
	}
	free(infos);

	*resolved_out = resolved;
	*num_filter_out = num_names;
	return id;
}

static const char *monitor_line_name(const struct request_info *info,
				     unsigned int offset)
{
	unsigned int i;

	if (!info)
		return NULL;

	for (i = 0; i < info->num_offsets; i++) {
		if (info->offsets[i] != offset)
			continue;

		return info->line_names[i][0] ? info->line_names[i] : NULL;
	}

	return NULL;
}

static int monitor_main(int argc, char **argv, const struct config *cfg)
{
	struct message msg = { .type = MSG_TYPE_MONITOR };
	struct event_format_opts fmt_opts = { };
	struct monitor_opts opts = { };
	struct resolved_name *resolved;
	struct request_info *info;
	struct event_info ev = { };
	unsigned int num_filter, id, i;
	int events_done = 0, sock;
	bool filter;

	parse_monitor_cmdline(&argc, &argv, &opts);

	fmt_opts.timestamp_fmt = opts.timestamp_fmt;
	fmt_opts.fmt = opts.fmt;
	fmt_opts.show_chip = true;
	fmt_opts.unquoted = cfg->unquoted;

	if (argc < 1)
		die("at least one request or line must be specified, try %s monitor --help",
		    get_prog_name());

	sock = get_connection(cfg->socket_path);

	id = monitor_resolve(sock, argc, argv, &resolved, &num_filter, &info);
	filter = (num_filter > 0);

	if (opts.initial_state) {
		if (is_request_id(argv[0]))
			get_request(sock, argv, cfg);
		else
			get_by_name(sock, argc, argv, cfg);
		fflush(stdout);
	}

	msg.monitor_req.id = id;
	client_send_msg(sock, &msg);
	client_recv_reply(sock, &msg);
	client_validate_response(&msg, MSG_TYPE_OK,
				 "unable to monitor request %u", id);

	if (opts.banner) {
		printf("Monitoring request%u...\n", id);
		fflush(stdout);
	}

	for (;;) {
		client_recv_event(sock, &msg);
		if (msg.type != MSG_TYPE_EDGE_EVENT)
			break;

		if (filter) {
			for (i = 0; i < num_filter; i++) {
				if (resolved[i].offset == msg.edge_ev.offset)
					break;
			}
			if (i == num_filter)
				continue;
		}

		ev.timestamp_ns = msg.edge_ev.timestamp_ns;
		ev.offset = msg.edge_ev.offset;
		ev.event_type = msg.edge_ev.event_type;
		ev.line_name = monitor_line_name(info, msg.edge_ev.offset);
		ev.chip_name = info ? info->chip_name : NULL;

		print_event(&ev, &fmt_opts);
		fflush(stdout);

		events_done++;
		if (opts.num_events && events_done >= opts.num_events)
			break;
	}

	free(info);
	free(resolved);
	close(sock);

	return EXIT_SUCCESS;
}

static void print_reconfigure_help(void)
{
	printf("Usage: %s [GLOBAL OPTIONS] reconfigure [OPTIONS] <request> [line=value]...\n",
	       get_prog_name());
	printf("\n");
	printf("Reconfigure an existing GPIO line request held by the daemon.\n");
	printf("\n");
	printf("<request> is the name or bare id as reported by the 'requests'\n");
	printf("sub-command.\n");
	printf("\n");
	printf("In output mode, values may optionally be set. They must be given for\n");
	printf("all lines of the request, either one per line in the order listed by\n");
	printf("'requests' or as 'line=value' pairs, where line is the name or the\n");
	printf("offset of a line in the request. Values: '0'/'1', 'active'/'inactive',\n");
	printf("'on'/'off'.\n");
	printf("\n");
	printf("Options:\n");
	printf("  -i, --input\t\t\tset direction to input\n");
	printf("  -o, --output\t\t\tset direction to output\n");
	printf("      --open-drain\t\tset drive mode to open-drain (output only)\n");
	printf("      --open-source\t\tset drive mode to open-source (output only)\n");
	printf("      --push-pull\t\tset drive mode to push-pull (output only)\n");
	printf("  -l, --active-low\t\ttreat the lines as active low\n");
	printf("      --bias-disabled\t\tset the line bias to disabled\n");
	printf("      --pull-down\t\tset the line bias to pull-down\n");
	printf("      --pull-up\t\t\tset the line bias to pull-up\n");
	printf("      --rising-edge\t\tmonitor rising edges (implies --input)\n");
	printf("      --falling-edge\t\tmonitor falling edges (implies --input)\n");
	printf("      --both-edges\t\tmonitor rising and falling edges (implies --input)\n");
	printf("      --clock-hte\t\tuse HTE clock for edge event timestamps\n");
	printf("      --clock-monotonic\t\tuse monotonic clock for edge event timestamps\n");
	printf("      --clock-realtime\t\tuse realtime clock for edge event timestamps\n");
	printf("  -p, --debounce-period <period>\n");
	printf("\t\t\tdebounce the lines with the specified period\n");
	printf("  -h, --help\t\t\tdisplay this help and exit\n");
	print_period_help();
}

static const struct request_info *
find_req_info(const struct request_info *infos, unsigned int num_infos,
	      unsigned int id)
{
	unsigned int i;

	for (i = 0; i < num_infos; i++) {
		if (infos[i].id == id)
			return &infos[i];
	}

	return NULL;
}

/* Find a line of the request by its name or offset. Returns its index. */
static unsigned int find_info_line(const struct request_info *info,
				   const char *line)
{
	unsigned int i, found = 0;
	bool match = false;
	unsigned long off;
	char *end;

	for (i = 0; i < info->num_offsets; i++) {
		if (info->line_names[i][0] == '\0' ||
		    strcmp(info->line_names[i], line) != 0)
			continue;

		if (match)
			die("line '%s' is ambiguous in request %u", line,
			    info->id);

		match = true;
		found = i;
	}
	if (match)
		return found;

	errno = 0;
	off = strtoul(line, &end, 10);
	if (!errno && end != line && *end == '\0') {
		for (i = 0; i < info->num_offsets; i++) {
			if (info->offsets[i] == off)
				return i;
		}
	}

	die("no line '%s' in request %u", line, info->id);
}

/*
 * Values are either given for all lines of the request in the order they are
 * listed by 'requests' or as 'line=value' pairs, where line is the name or
 * the offset of a line in the request, for all of its lines.
 */
static void reconfigure_values(int sock, unsigned int id,
			       struct request_config *cfg,
			       const struct request_opts *opts,
			       int argc, char **argv)
{
	enum gpiod_line_value values[MAX_REQ_LINES];
	const struct request_info *info;
	struct request_info *infos;
	char *lines[MAX_REQ_LINES];
	unsigned int num_infos, k;
	bool used[MAX_REQ_LINES];
	int i, num_eq = 0;

	if (!opts->output)
		die("output values can only be set in output mode");

	infos = collect_req_infos(sock, &num_infos);
	info = find_req_info(infos, num_infos, id);
	if (!info)
		die("no request with id %u", id);

	if ((unsigned int)argc != info->num_offsets)
		die("values must be specified for all %u lines of request %u",
		    info->num_offsets, id);

	for (i = 0; i < argc; i++) {
		if (strchr(argv[i], '='))
			num_eq++;
	}
	if (num_eq && num_eq != argc)
		die("cannot mix positional values and line=value pairs");

	if (!num_eq) {
		for (i = 0; i < argc; i++) {
			cfg->offsets[i] = info->offsets[i];
			cfg->values[i] = parse_line_value_or_die(argv[i]);
		}
	} else {
		parse_line_values_or_die(argc, argv, lines, values);
		memset(used, 0, sizeof(used));

		for (i = 0; i < argc; i++) {
			k = find_info_line(info, lines[i]);
			if (used[k])
				die("line '%s' specified more than once",
				    lines[i]);

			used[k] = true;
			cfg->offsets[i] = info->offsets[k];
			cfg->values[i] = values[i];
		}
	}

	cfg->num_offsets = argc;
	free(infos);
}

static int reconfigure_main(int argc, char **argv, const struct config *cfg)
{
	struct message msg = { .type = MSG_TYPE_RECONFIGURE };
	struct request_opts opts = { };
	unsigned int id;
	int sock;

	parse_request_cmdline(&argc, &argv, &opts, true,
			      print_reconfigure_help);
	validate_request_opts(&opts);

	if (!argc)
		die("request id required, try %s reconfigure --help",
		    get_prog_name());

	id = parse_request_id(argv[0]);
	msg.reconfigure.id = id;
	argc--;
	argv++;

	apply_opts_to_config(&msg.reconfigure.cfg, &opts, 0, NULL);

	sock = get_connection(cfg->socket_path);
	if (argc)
		reconfigure_values(sock, id, &msg.reconfigure.cfg, &opts, argc,
				   argv);

	client_exchange(sock, &msg);
	client_validate_response(&msg, MSG_TYPE_OK,
				 "unable to reconfigure request %u", id);
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
	{
		.name = "requests",
		.sub_main = requests_main,
		.desc = "List the GPIO line requests held by the daemon.",
	},
	{
		.name = "get",
		.sub_main = get_main,
		.desc = "Read the values of a held GPIO line request.",
	},
	{
		.name = "set",
		.sub_main = set_main,
		.desc = "Update the values of a held GPIO line request.",
	},
	{
		.name = "monitor",
		.sub_main = monitor_main,
		.desc = "Monitor edge events on a held GPIO line request.",
	},
	{
		.name = "reconfigure",
		.sub_main = reconfigure_main,
		.desc = "Reconfigure the settings of a held GPIO line request.",
	},
	{ }
};

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
	printf("      --by-name\t\ttreat lines as names even if they would parse as an offset\n");
	printf("  -c, --chip <chip>\trestrict scope to a particular chip\n");
	printf("  -C, --consumer <name>\tconsumer name applied to requested lines\n");
	printf("\t\t\t(default is 'gpioctl')\n");
	printf("  -h, --help\t\tdisplay this help and exit\n");
	printf("  -N, --numeric\t\tdisplay line values as '0' (inactive) or '1' (active)\n");
	printf("  -p, --socket-path <path>\n");
	printf("\t\t\tpath to the unix-domain socket used for\n");
	printf("\t\t\tcommunication between client and server\n");
	printf("  -s, --strict\t\tabort if requested line names are not unique\n");
	printf("      --unquoted\tdon't quote line or consumer names\n");
	printf("  -v, --version\t\toutput version information and exit\n");
	print_chip_help();
}

static void parse_global_cmdline(int *argc, char ***argv, struct config *cfg)
{
	static const struct option longopts[] = {
		{ "by-name",		no_argument,		NULL,	'B' },
		{ "chip",		required_argument,	NULL,	'c' },
		{ "consumer",		required_argument,	NULL,	'C' },
		{ "help",		no_argument,		NULL,	'h' },
		{ "numeric",		no_argument,		NULL,	'N' },
		{ "socket-path",	required_argument,	NULL,	'p' },
		{ "strict",		no_argument,		NULL,	's' },
		{ "unquoted",		no_argument,		NULL,	'Q' },
		{ "version",		no_argument,		NULL,	'v' },
		{ GETOPT_NULL_LONGOPT },
	};
	static const char *const shortopts = "+hvNc:sC:Bp:Q";

	int opti, optc;

	for (;;) {
		optc = getopt_long(*argc, *argv, shortopts, longopts, &opti);
		if (optc < 0)
			break;

		switch (optc) {
		case 'B':
			cfg->by_name = true;
			break;
		case 'c':
			cfg->chip_id = optarg;
			break;
		case 'C':
			cfg->consumer = optarg;
			break;
		case 'N':
			cfg->numeric = true;
			break;
		case 'p':
			cfg->socket_path = optarg;
			break;
		case 'Q':
			cfg->unquoted = true;
			break;
		case 's':
			cfg->strict = true;
			break;
		case 'h':
			print_help();
			exit(EXIT_SUCCESS);
		case 'v':
			print_version();
			exit(EXIT_SUCCESS);
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

static const struct sub_command *find_sub_cmd(const char *name)
{
	const struct sub_command *cmd;

	for (cmd = sub_cmds; cmd->name; cmd++) {
		if (strcmp(name, cmd->name) == 0)
			return cmd;
	}

	return NULL;
}

int main(int argc, char **argv)
{
	struct config cfg = { .consumer = "gpioctl" };
	const struct sub_command *subcmd;

	set_prog_name(argv[0]);
	parse_global_cmdline(&argc, &argv, &cfg);

	subcmd = find_sub_cmd(argv[0]);
	if (!subcmd)
		die("invalid sub-command, try %s --help", get_prog_name());

	return subcmd->sub_main(argc, argv, &cfg);
}
