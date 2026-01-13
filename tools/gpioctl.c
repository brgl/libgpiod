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
	MSG_TYPE_LIST,
	MSG_TYPE_REQ_INFO,
	MSG_TYPE_GET,
	MSG_TYPE_GET_OK,
	MSG_TYPE_SET,
	MSG_TYPE_MONITOR,
	MSG_TYPE_EDGE_EVENT,
	MSG_TYPE_RECONFIGURE,
	NUM_MSG_TYPES,
};

#define MAX_EPOLL_EVENTS	16
#define MAX_REQ_NAME_SIZE	32
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
	char consumer[MAX_REQ_NAME_SIZE];
	struct request_config cfg;
} PACKED;

struct request_ok {
	char req_name[MAX_REQ_NAME_SIZE];
} PACKED;

struct release {
	unsigned int id;
} PACKED;

struct request_info {
	unsigned int id;
	char chip_name[32];
	char consumer[MAX_REQ_NAME_SIZE];
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
	bool strict;
	const char *chip_id;
	const char *consumer;
	bool by_name;
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
	char consumer[MAX_REQ_NAME_SIZE];
	char line_names[MAX_REQ_LINES][32];
	unsigned int num_lines;
	struct client *monitor_client;
	struct receiver monitor_rcv;
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
	char socket_path[sizeof(((struct sockaddr_un *)NULL)->sun_path)];
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
		srv_log(srv, "Failed to add the receiver to the epoll set: %s\n",
			strerror(errno));

	return ret;
}

static void srv_remove_receiver(struct server *srv, struct receiver *rcv)
{
	epoll_ctl(srv->epollfd, EPOLL_CTL_DEL, rcv->fd, NULL);
}

static void srv_drop_client(struct server *srv, struct client *cli)
{
	struct line_request *line_req;

	list_del(&cli->list);
	srv_remove_receiver(srv, &cli->rcv);
	close(cli->rcv.fd);

	/* Cancel any active monitor subscription this client holds. */
	list_for_each_entry(line_req, &srv->requests, list) {
		if (line_req->monitor_client == cli) {
			srv_remove_receiver(srv, &line_req->monitor_rcv);
			line_req->monitor_client = NULL;
		}
	}

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

static int build_line_config(struct gpiod_line_config *line_cfg,
			     struct gpiod_line_settings *settings,
			     const struct request_config *cfg)
{
	unsigned int offsets[MAX_REQ_LINES];

	if (cfg->direction)
		gpiod_line_settings_set_direction(settings, cfg->direction);
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

	return gpiod_line_config_add_line_settings(line_cfg, offsets,
						   cfg->num_offsets, settings);
}

static struct line_request *request_lines(const struct request *req)
{
	struct gpiod_request_config *req_cfg;
	struct gpiod_line_settings *settings;
	struct gpiod_line_config *line_cfg;
	struct line_request *line_req;
	struct gpiod_chip *chip;
	int ret;

	line_req = calloc(1, sizeof(*line_req));
	req_cfg = gpiod_request_config_new();
	line_cfg = gpiod_line_config_new();
	settings = gpiod_line_settings_new();
	if (!line_req || !req_cfg || !line_cfg || !settings)
		die_oom();

	ret = build_line_config(line_cfg, settings, &req->cfg);
	if (ret)
		goto err_out;

	gpiod_request_config_set_consumer(req_cfg, req->consumer);

	chip = gpiod_chip_open(req->chip_path);
	if (!chip)
		goto err_out;

	line_req->handle = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
	gpiod_chip_close(chip);
	if (!line_req->handle)
		goto err_out;

	if (req->cfg.direction == GPIOD_LINE_DIRECTION_OUTPUT &&
	    req->cfg.values[0] != GPIOD_LINE_VALUE_ERROR) {
		enum gpiod_line_value values[MAX_REQ_LINES];

		memcpy(values, req->cfg.values, req->cfg.num_offsets * sizeof(*values));
		gpiod_line_request_set_values(line_req->handle, values);
	}

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

static void handle_line_request(struct client *cli, struct server *srv,
				const struct request *req)
{
	struct gpiod_line_info *info;
	struct line_request *line_req;
	struct gpiod_chip *chip;
	struct message resp = { };
	unsigned int i;
	const char *name;

	srv_log(srv, "Handling line request from process %d\n", cli->creds.pid);

	line_req = request_lines(req);
	if (!line_req) {
		srv_log(srv, "Line request failed (%s), sending back error response\n",
			strerror(errno));
		send_error_response(cli, srv, errno);
		return;
	}

	line_req->id = get_request_id(srv);
	snprintf(line_req->consumer, sizeof(line_req->consumer), "%s",
		 req->consumer);
	line_req->num_lines = req->cfg.num_offsets;

	chip = gpiod_chip_open(req->chip_path);
	if (chip) {
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
	}

	resp.type = MSG_TYPE_REQ_OK;
	snprintf(resp.req_ok.req_name, MAX_REQ_NAME_SIZE, "%s", req->consumer);

	send_message(&cli->rcv, srv, &resp);

	list_add(&line_req->list, &srv->requests);
	srv->usecnt++;
}

static void handle_list_request(struct client *cli, struct server *srv)
{
	struct line_request *line_req;
	unsigned int offsets[MAX_REQ_LINES];
	const char *chip_name;
	struct message resp;
	size_t num_offsets;
	int ret;

	srv_log(srv, "Handling list request from process %d\n", cli->creds.pid);

	list_for_each_entry(line_req, &srv->requests, list) {
		memset(&resp, 0, sizeof(resp));
		resp.type = MSG_TYPE_REQ_INFO;
		resp.req_info.id = line_req->id;
		snprintf(resp.req_info.consumer, sizeof(resp.req_info.consumer),
			 "%s", line_req->consumer);

		chip_name = gpiod_line_request_get_chip_name(line_req->handle);
		snprintf(resp.req_info.chip_name, sizeof(resp.req_info.chip_name),
			 "%s", chip_name);

		num_offsets = gpiod_line_request_get_requested_offsets(
				line_req->handle, offsets, MAX_REQ_LINES);
		memcpy(resp.req_info.offsets, offsets,
		       num_offsets * sizeof(*offsets));
		resp.req_info.num_offsets = num_offsets;
		memcpy(resp.req_info.line_names, line_req->line_names,
		       num_offsets * sizeof(*line_req->line_names));


		ret = send_message(&cli->rcv, srv, &resp);
		if (ret) {
			srv_drop_client(srv, cli);
			return;
		}
	}

	memset(&resp, 0, sizeof(resp));
	resp.type = MSG_TYPE_OK;
	ret = send_message(&cli->rcv, srv, &resp);
	if (ret)
		srv_drop_client(srv, cli);
}

static void handle_release_request(struct client *cli, struct server *srv,
				   const struct release *rel)
{
	struct line_request *line_req, *tmp;
	struct message resp = { };
	int ret;

	srv_log(srv, "Handling release request for id %u from process %d\n",
		rel->id, cli->creds.pid);

	list_for_each_entry_safe(line_req, tmp, &srv->requests, list) {
		if (line_req->id != rel->id)
			continue;

		list_del(&line_req->list);
		gpiod_line_request_release(line_req->handle);
		if (line_req->monitor_client)
			srv_remove_receiver(srv, &line_req->monitor_rcv);
		free(line_req);
		srv->usecnt--;

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
	enum gpiod_line_value values[MAX_REQ_LINES];
	struct line_request *line_req;
	struct message resp = { };
	unsigned int offsets[MAX_REQ_LINES];
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

	resp.type = MSG_TYPE_GET_OK;
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
	struct line_request *line_req;
	struct message resp = { };
	unsigned int offsets[MAX_REQ_LINES];
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

	resp.type = MSG_TYPE_OK;
	ret = send_message(&cli->rcv, srv, &resp);
	if (ret)
		srv_drop_client(srv, cli);
}

static void handle_reconfigure_request(struct client *cli, struct server *srv,
					const struct reconfigure *rcfg)
{
	struct gpiod_line_settings *settings;
	struct gpiod_line_config *line_cfg;
	struct request_config cfg;
	struct line_request *line_req;
	struct message resp = { };
	size_t num_lines;
	int ret;

	srv_log(srv, "Handling reconfigure request for id %u from process %d\n",
		rcfg->id, cli->creds.pid);

	line_req = find_request(srv, rcfg->id);
	if (!line_req) {
		send_error_response(cli, srv, ENOENT);
		return;
	}

	/*
	 * Fetch the current offsets from the handle and splice them into a
	 * copy of the config so build_line_config applies the new settings
	 * to the right lines.
	 */
	cfg = rcfg->cfg;
	{
		unsigned int offsets[MAX_REQ_LINES];

		num_lines = gpiod_line_request_get_requested_offsets(
				line_req->handle, offsets, MAX_REQ_LINES);
		memcpy(cfg.offsets, offsets, num_lines * sizeof(*offsets));
	}
	cfg.num_offsets = num_lines;

	line_cfg = gpiod_line_config_new();
	settings = gpiod_line_settings_new();
	if (!line_cfg || !settings)
		die_oom();

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

	if (rcfg->cfg.direction == GPIOD_LINE_DIRECTION_OUTPUT &&
	    rcfg->cfg.values[0] != GPIOD_LINE_VALUE_ERROR) {
		enum gpiod_line_value values[MAX_REQ_LINES];

		memcpy(values, rcfg->cfg.values, num_lines * sizeof(*values));
		gpiod_line_request_set_values(line_req->handle, values);
	}

	resp.type = MSG_TYPE_OK;
	ret = send_message(&cli->rcv, srv, &resp);
	if (ret)
		srv_drop_client(srv, cli);
}

static void monitor_receive(uint32_t events UNUSED, struct receiver *rcv,
			    struct server *srv)
{
	struct gpiod_edge_event_buffer *buf;
	struct gpiod_edge_event *event;
	struct line_request *line_req =
		container_of(rcv, struct line_request, monitor_rcv);
	struct message msg = { };
	int num_events, i, ret;

	buf = gpiod_edge_event_buffer_new(MONITOR_EVENT_BUF_SIZE);
	if (!buf)
		die_oom();

	num_events = gpiod_line_request_read_edge_events(line_req->handle, buf,
							 MONITOR_EVENT_BUF_SIZE);
	if (num_events < 0) {
		gpiod_edge_event_buffer_free(buf);
		return;
	}

	for (i = 0; i < num_events; i++) {
		event = gpiod_edge_event_buffer_get_event(buf, i);

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

	gpiod_edge_event_buffer_free(buf);
}

static void handle_monitor_request(struct client *cli, struct server *srv,
				   const struct monitor_req *mon)
{
	struct line_request *line_req;
	struct message resp = { };
	int ret;

	srv_log(srv, "Handling monitor request for id %u from process %d\n",
		mon->id, cli->creds.pid);

	line_req = find_request(srv, mon->id);
	if (!line_req) {
		send_error_response(cli, srv, ENOENT);
		return;
	}

	if (line_req->monitor_client) {
		/* Already monitored. */
		send_error_response(cli, srv, EBUSY);
		return;
	}

	line_req->monitor_client = cli;
	line_req->monitor_rcv.fd = gpiod_line_request_get_fd(line_req->handle);
	line_req->monitor_rcv.receive = monitor_receive;

	ret = srv_add_receiver(srv, &line_req->monitor_rcv);
	if (ret) {
		line_req->monitor_client = NULL;
		send_error_response(cli, srv, errno);
		return;
	}

	resp.type = MSG_TYPE_OK;
	ret = send_message(&cli->rcv, srv, &resp);
	if (ret)
		srv_drop_client(srv, cli);
}

static void client_receive(uint32_t events, struct receiver *rcv,
			   struct server *srv)
{
	struct message msg = { };
	struct client *cli = to_client(rcv);
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
	case MSG_TYPE_RELEASE:
		handle_release_request(cli, srv, &msg.rel);
		break;
	case MSG_TYPE_LIST:
		handle_list_request(cli, srv);
		break;
	case MSG_TYPE_GET:
		handle_get_request(cli, srv, &msg.get_req);
		break;
	case MSG_TYPE_SET:
		handle_set_request(cli, srv, &msg.set_req);
		break;
	case MSG_TYPE_RECONFIGURE:
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
		die_oom();

	len = sizeof(cli->creds);
	ret = getsockopt(sock, SOL_SOCKET, SO_PEERCRED, &cli->creds, &len);
	if (ret) {
		close(sock);
		free(cli);
		return;
	}

	pwd = getpwuid(cli->creds.uid);
	if (!pwd) {
		srv_log(srv, "Failed to get credentials from connected client: %s\n",
			strerror(errno));
		close(sock);
		free(cli);
		return;
	}

	cli->rcv.fd = sock;
	cli->rcv.receive = client_receive;

	ret = srv_add_receiver(srv, &cli->rcv);
	if (ret) {
		close(sock);
		free(cli);
		return;
	}

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

	running = curr.it_value.tv_sec || curr.it_value.tv_nsec;

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
		die_oom();

	memset(srv, 0, sizeof(*srv));

	memcpy(srv->socket_path, addr->sun_path, sizeof(srv->socket_path));

	srv->epollfd = epoll_create1(0);
	if (srv->epollfd < 0)
		die_perror("Failed to create the epoll handle");

	list_init(&srv->clients);
	list_init(&srv->requests);
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
	/*
	 * If we returned, we're the client. The daemon owns the socket file;
	 * clear the path so free_server() doesn't unlink it from the client.
	 */
	srv->socket_path[0] = '\0';
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

static void make_addr(struct sockaddr_un *addr, const char *socket_path)
{
	memset(addr, 0, sizeof(*addr));
	addr->sun_family = AF_UNIX;
	if (socket_path) {
		strncpy(addr->sun_path, socket_path, sizeof(addr->sun_path) - 1);
	} else {
		snprintf(addr->sun_path + 1, sizeof(addr->sun_path) - 1,
			 "gpioctl-%u", getuid());
	}
}

static int get_connection(const char *socket_path)
{
	struct sockaddr_un addr;
	int sock;

	make_addr(&addr, socket_path);

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
		      const struct config *cfg)
{
	struct sockaddr_un addr;
	struct server *srv;

	make_addr(&addr, cfg->socket_path);
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

static void client_recv_msg_wait(int sock, struct message *msg)
{
	ssize_t recvd;

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
	printf("  -l, --active-low\t\ttreat the lines as active low\n");
	printf("      --bias-disabled\t\tset the line bias to disabled\n");
	printf("      --both-edges\t\tmonitor rising and falling edges (implies --input)\n");
	printf("      --clock-hte\t\tuse HTE clock for edge event timestamps\n");
	printf("      --clock-monotonic\t\tuse monotonic clock for edge event timestamps\n");
	printf("      --clock-realtime\t\tuse realtime clock for edge event timestamps\n");
	printf("  -p, --debounce-period <period>\tdebounce the lines with the specified period\n");
	printf("      --falling-edge\t\tmonitor falling edges (implies --input)\n");
	printf("  -h, --help\t\t\tdisplay this help and exit\n");
	printf("  -i, --input\t\t\tset direction to input\n");
	printf("      --open-drain\t\tset drive mode to open-drain (output only)\n");
	printf("      --open-source\t\tset drive mode to open-source (output only)\n");
	printf("  -o, --output\t\t\tset direction to output\n");
	printf("      --pull-down\t\tset the line bias to pull-down\n");
	printf("      --pull-up\t\t\tset the line bias to pull-up\n");
	printf("      --push-pull\t\tset drive mode to push-pull (output only)\n");
	printf("      --rising-edge\t\tmonitor rising edges (implies --input)\n");
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
	int clock_count = 0, drive_count = 0, bias_count = 0;

	if (opts->input && opts->output)
		die("--input and --output are mutually exclusive");

	if ((opts->rising_edge || opts->falling_edge) && opts->output)
		die("edge detection is only available in input mode");

	drive_count += opts->push_pull;
	drive_count += opts->open_drain;
	drive_count += opts->open_source;
	if (drive_count > 1)
		die("--push-pull, --open-drain and --open-source are mutually exclusive");
	if (drive_count > 0 && !opts->output)
		die("--push-pull, --open-drain and --open-source are only available in output mode");

	bias_count += opts->pull_up;
	bias_count += opts->pull_down;
	bias_count += opts->bias_disabled;
	if (bias_count > 1)
		die("--pull-up, --pull-down and --bias-disabled are mutually exclusive");

	if (opts->clock_monotonic)
		clock_count++;
	if (opts->clock_realtime)
		clock_count++;
	if (opts->clock_hte)
		clock_count++;

	if (clock_count > 1)
		die("--clock-monotonic, --clock-realtime and --clock-hte are mutually exclusive");

	if (clock_count > 0 &&
	    !opts->rising_edge && !opts->falling_edge)
		die("--clock-* options require edge detection");

	if (opts->debounce_period_us &&
	    !opts->rising_edge && !opts->falling_edge)
		die("--debounce-period requires edge detection");
}

static void parse_request_cmdline(int *argc, char ***argv,
				  struct request_opts *opts)
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
	static const char *const shortopts = "+lp:hioh";

	int optc, opti;


	/* Reset getopt state after the global command line was parsed. */
	optind = 1;

	for (;;) {
		optc = getopt_long(*argc, *argv, shortopts, longopts, &opti);
		if (optc < 0)
			break;

		switch (optc) {
		case 'l':
			opts->active_low = true;
			break;
		case 'D':
			opts->bias_disabled = true;
			break;
		case 'E':
			opts->rising_edge = opts->falling_edge = true;
			break;
		case 'H':
			opts->clock_hte = true;
			break;
		case 'M':
			opts->clock_monotonic = true;
			break;
		case 'R':
			opts->clock_realtime = true;
			break;
		case 'p':
			opts->debounce_period_us = parse_period_or_die(optarg);
			break;
		case 'F':
			opts->falling_edge = true;
			break;
		case 'h':
			print_request_help();
			exit(EXIT_SUCCESS);
		case 'i':
			opts->input = true;
			break;
		case 'N':
			opts->open_drain = true;
			break;
		case 'S':
			opts->open_source = true;
			break;
		case 'o':
			opts->output = true;
			break;
		case 'P':
			opts->pull_down = true;
			break;
		case 'U':
			opts->pull_up = true;
			break;
		case 'X':
			opts->push_pull = true;
			break;
		case 'G':
			opts->rising_edge = true;
			break;
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

	if ((opts->rising_edge || opts->falling_edge) && !opts->output)
		opts->input = true;

	/* Initialize values to sentinel (no value given). */
	for (i = 0; i < MAX_REQ_LINES; i++)
		cfg->values[i] = GPIOD_LINE_VALUE_ERROR;

	/* Strip =value suffixes from line arguments. */
	for (i = 0; i < argc; i++) {
		char *eq = strchr(argv[i], '=');

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

	if (opts->debounce_period_us) {
		if (opts->debounce_period_us > ULONG_MAX)
			die("debounce period too large");
		cfg->debounce_period_us = (unsigned long)opts->debounce_period_us;
	}
}

static int request_main(int argc, char **argv, const struct config *cfg)
{
	struct message msg = { .type = MSG_TYPE_REQUEST };
	struct line_resolver *resolver;
	struct request *req = &msg.req;
	struct request_opts opts = {};
	int sock, i;

	parse_request_cmdline(&argc, &argv, &opts);
	validate_request_opts(&opts);

	apply_opts_to_config(&req->cfg, &opts, argc, argv);

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

	snprintf(req->consumer, sizeof(req->consumer), "%s", cfg->consumer);

	for (i = 0; i < resolver->num_lines; i++)
		req->cfg.offsets[i] = resolver->lines[i].offset;
	req->cfg.num_offsets = resolver->num_lines;

	sock = get_connection(cfg->socket_path);

	client_exchange(sock, &msg);
	client_validate_response(&msg, MSG_TYPE_REQ_OK);

	free_line_resolver(resolver);
	close(sock);

	return EXIT_SUCCESS;
}

static void print_release_help(void)
{
	printf("Usage: %s [GLOBAL OPTIONS] release <request>\n", get_prog_name());
	printf("\n");
	printf("Release a GPIO line request held by the daemon.\n");
	printf("\n");
	printf("<request> is the name of a request as reported by the 'requests'\n");
	printf("sub-command (e.g. 'request0'); the bare id ('0') is also accepted.\n");
	printf("\n");
	printf("Options:\n");
	printf("  -h, --help\t\tdisplay this help and exit\n");
}

static void parse_release_cmdline(int *argc, char ***argv)
{
	static const struct option longopts[] = {
		{ "help",		no_argument,	NULL,	'h' },
		{ GETOPT_NULL_LONGOPT }
	};
	static const char *const shortopts = "+h";

	int optc, opti;

	/* Reset getopt state after the global command line was parsed. */
	optind = 1;

	for (;;) {
		optc = getopt_long(*argc, *argv, shortopts, longopts, &opti);
		if (optc < 0)
			break;

		switch (optc) {
		case 'h':
			print_release_help();
			exit(EXIT_SUCCESS);
		case '?':
			die("try %s release --help", get_prog_name());
		default:
			abort();
		}
	}

	*argc -= optind;
	*argv += optind;
}

static bool is_request_id(const char *arg)
{
	const char *str = arg;
	char *end;

	if (strncmp(str, "request", 7) == 0)
		str += 7;

	if (*str == '\0')
		return false;

	errno = 0;
	strtoul(str, &end, 10);
	return !errno && *end == '\0';
}

static unsigned int parse_request_id(const char *arg)
{
	const char *str = arg;
	unsigned long id;
	char *end;

	/* Accept both the 'request<id>' name and a bare '<id>'. */
	if (strncmp(str, "request", 7) == 0)
		str += 7;

	errno = 0;
	id = strtoul(str, &end, 10);
	if (errno || *str == '\0' || *end != '\0' || id > UINT_MAX)
		die("invalid request name: '%s'", arg);

	return id;
}

static int release_main(int argc, char **argv, const struct config *cfg UNUSED)
{
	struct message msg = { .type = MSG_TYPE_RELEASE };
	int sock;

	parse_release_cmdline(&argc, &argv);

	if (argc < 1)
		die("exactly one request must be specified, try %s release --help",
		    get_prog_name());
	if (argc > 1)
		die("only one request can be released at a time");

	msg.rel.id = parse_request_id(argv[0]);

	sock = get_connection(cfg->socket_path);

	client_exchange(sock, &msg);
	client_validate_response(&msg, MSG_TYPE_OK);
	close(sock);

	return EXIT_SUCCESS;
}

static int stop_main(int argc UNUSED, char **argv UNUSED,
		     const struct config *cfg)
{
	struct message msg = { .type = MSG_TYPE_STOP };
	int sock = get_connection(cfg->socket_path);

	client_exchange(sock, &msg);
	client_validate_response(&msg, MSG_TYPE_OK);
	close(sock);

	return EXIT_SUCCESS;
}

static int ping_main(int argc UNUSED, char **argv UNUSED,
		     const struct config *cfg)
{
	struct message msg = { .type = MSG_TYPE_PING };
	int sock = get_connection(cfg->socket_path);

	client_exchange(sock, &msg);
	client_validate_response(&msg, MSG_TYPE_OK);
	close(sock);

	return EXIT_SUCCESS;
}

static void print_request_info(const struct request_info *info)
{
	unsigned int i;

	printf("request%u (%s) consumer=\"%s\" offsets: [",
	       info->id, info->chip_name, info->consumer);

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
}

static void parse_requests_cmdline(int *argc, char ***argv)
{
	static const struct option longopts[] = {
		{ "help",	no_argument,	NULL,	'h' },
		{ GETOPT_NULL_LONGOPT }
	};
	static const char *const shortopts = "+h";

	int optc, opti;

	/* Reset getopt state after the global command line was parsed. */
	optind = 1;

	for (;;) {
		optc = getopt_long(*argc, *argv, shortopts, longopts, &opti);
		if (optc < 0)
			break;

		switch (optc) {
		case 'h':
			print_requests_help();
			exit(EXIT_SUCCESS);
		case '?':
			die("try %s requests --help", get_prog_name());
		default:
			abort();
		}
	}

	*argc -= optind;
	*argv += optind;
}

static int requests_main(int argc, char **argv, const struct config *cfg UNUSED)
{
	struct message msg = { .type = MSG_TYPE_LIST };
	int sock;

	parse_requests_cmdline(&argc, &argv);

	sock = get_connection(cfg->socket_path);

	client_send_msg(sock, &msg);

	for (;;) {
		client_recv_msg(sock, &msg);

		if (msg.type == MSG_TYPE_REQ_INFO) {
			print_request_info(&msg.req_info);
			continue;
		}

		client_validate_response(&msg, MSG_TYPE_OK);
		break;
	}

	close(sock);

	return EXIT_SUCCESS;
}

static struct request_info *collect_req_infos(int sock, unsigned int *num_out)
{
	struct message msg = { .type = MSG_TYPE_LIST };
	struct request_info *infos = NULL;
	unsigned int count = 0;

	client_send_msg(sock, &msg);

	for (;;) {
		client_recv_msg(sock, &msg);

		if (msg.type == MSG_TYPE_OK)
			break;
		if (msg.type != MSG_TYPE_REQ_INFO)
			die("unexpected message type %d from daemon", msg.type);

		infos = realloc(infos, (count + 1) * sizeof(*infos));
		if (!infos)
			die_oom();
		infos[count++] = msg.req_info;
	}

	*num_out = count;
	return infos;
}

struct resolved_name {
	unsigned int req_id;
	unsigned int offset;
};

static void resolve_names(const struct request_info *infos, unsigned int num_infos,
			   int argc, char **argv, struct resolved_name *resolved)
{
	unsigned int i, j, k;

	for (i = 0; i < (unsigned int)argc; i++) {
		bool found = false;

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
			die("no held request contains a line named '%s'", argv[i]);
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
}

static void parse_get_cmdline(int *argc, char ***argv)
{
	static const struct option longopts[] = {
		{ "help",	no_argument,	NULL,	'h' },
		{ GETOPT_NULL_LONGOPT }
	};
	static const char *const shortopts = "+h";

	int optc, opti;

	optind = 1;

	for (;;) {
		optc = getopt_long(*argc, *argv, shortopts, longopts, &opti);
		if (optc < 0)
			break;

		switch (optc) {
		case 'h':
			print_get_help();
			exit(EXIT_SUCCESS);
		case '?':
			die("try %s get --help", get_prog_name());
		default:
			abort();
		}
	}

	*argc -= optind;
	*argv += optind;
}

static int get_main(int argc, char **argv, const struct config *cfg)
{
	unsigned int group_idx[MAX_REQ_LINES], num_infos, id, i, j, cnt;
	struct resolved_name *resolved;
	enum gpiod_line_value *values;
	struct request_info *infos;
	struct message msg = { };
	bool *done;
	int sock;

	parse_get_cmdline(&argc, &argv);

	if (argc < 1)
		die("request or line name must be specified, try %s get --help",
		    get_prog_name());

	sock = get_connection(cfg->socket_path);

	if (is_request_id(argv[0])) {
		if (argc > 1)
			die("only one request can be queried at a time");

		id = parse_request_id(argv[0]);
		msg.type = MSG_TYPE_GET;
		msg.get_req.id = id;
		msg.get_req.num_offsets = 0;

		client_exchange(sock, &msg);
		client_validate_response(&msg, MSG_TYPE_GET_OK);

		for (i = 0; i < msg.get_ok.num_values; i++) {
			if (i)
				printf(" ");
			printf("%s", msg.get_ok.values[i] ==
			       GPIOD_LINE_VALUE_ACTIVE ? "active" : "inactive");
		}
		printf("\n");
	} else {
		infos = collect_req_infos(sock, &num_infos);
		resolved = malloc((unsigned int)argc * sizeof(*resolved));
		if (!resolved)
			die_oom();

		resolve_names(infos, num_infos, argc, argv, resolved);
		free(infos);

		/*
		 * Group by request_id and send one GET per group. Results are
		 * placed back into a values[] array indexed by original argv
		 * position so output order matches the command line.
		 */
		values = malloc((unsigned int)argc * sizeof(*values));
		if (!values)
			die_oom();

		done = calloc((unsigned int)argc, sizeof(*done));
		if (!done)
			die_oom();

		for (i = 0; i < (unsigned int)argc; i++) {
			if (done[i])
				continue;

			memset(&msg, 0, sizeof(msg));
			msg.type = MSG_TYPE_GET;
			msg.get_req.id = resolved[i].req_id;
			cnt = 0;

			for (j = i; j < (unsigned int)argc; j++) {
				if (resolved[j].req_id == resolved[i].req_id) {
					msg.get_req.offsets[cnt] = resolved[j].offset;
					group_idx[cnt] = j;
					cnt++;
					done[j] = true;
				}
			}
			msg.get_req.num_offsets = cnt;

			client_exchange(sock, &msg);
			client_validate_response(&msg, MSG_TYPE_GET_OK);

			for (j = 0; j < cnt; j++)
				values[group_idx[j]] = msg.get_ok.values[j];
		}

		for (i = 0; i < (unsigned int)argc; i++) {
			if (i)
				printf(" ");
			printf("\"%s\"=%s", argv[i],
			       values[i] == GPIOD_LINE_VALUE_ACTIVE ?
			       "active" : "inactive");
		}
		printf("\n");

		free(done);
		free(values);
		free(resolved);
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

static void parse_set_cmdline(int *argc, char ***argv)
{
	static const struct option longopts[] = {
		{ "help",	no_argument,	NULL,	'h' },
		{ GETOPT_NULL_LONGOPT }
	};
	static const char *const shortopts = "+h";

	int optc, opti;

	optind = 1;

	for (;;) {
		optc = getopt_long(*argc, *argv, shortopts, longopts, &opti);
		if (optc < 0)
			break;

		switch (optc) {
		case 'h':
			print_set_help();
			exit(EXIT_SUCCESS);
		case '?':
			die("try %s set --help", get_prog_name());
		default:
			abort();
		}
	}

	*argc -= optind;
	*argv += optind;
}

static int set_main(int argc, char **argv, const struct config *cfg)
{
	unsigned int num_infos, i, j, cnt;
	bool offset_mode, has_eq, *done;
	struct resolved_name *resolved;
	enum gpiod_line_value *values;
	struct request_info *infos;
	struct message msg = { };
	char **names, *end;
	unsigned long off;
	const char *eq;
	int sock;

	parse_set_cmdline(&argc, &argv);

	if (argc < 1)
		die("request and value or line=value required, try %s set --help",
		    get_prog_name());

	/*
	 * Determine mode: if the first argument contains '=' and is not a
	 * request id, treat all arguments as "line=value" name-based form.
	 * Otherwise the first argument must be a request id followed by values.
	 */
	eq = strchr(argv[0], '=');
	if (eq && !is_request_id(argv[0])) {
		names = malloc((unsigned int)argc * sizeof(*names));
		values = malloc((unsigned int)argc * sizeof(*values));
		resolved = malloc((unsigned int)argc * sizeof(*resolved));
		if (!names || !values || !resolved)
			die_oom();

		for (i = 0; i < (unsigned int)argc; i++) {
			eq = strchr(argv[i], '=');
			if (!eq)
				die("expected name=value, got '%s'", argv[i]);
			names[i] = strndup(argv[i], eq - argv[i]);
			if (!names[i])
				die_oom();
			values[i] = parse_line_value_or_die(eq + 1);
		}

		sock = get_connection(cfg->socket_path);
		infos = collect_req_infos(sock, &num_infos);
		resolve_names(infos, num_infos, argc, (char **)names, resolved);
		free(infos);

		done = calloc((unsigned int)argc, sizeof(*done));
		if (!done)
			die_oom();

		for (i = 0; i < (unsigned int)argc; i++) {
			if (done[i])
				continue;

			memset(&msg, 0, sizeof(msg));
			msg.type = MSG_TYPE_SET;
			msg.set_req.id = resolved[i].req_id;
			cnt = 0;

			for (j = i; j < (unsigned int)argc; j++) {
				if (resolved[j].req_id == resolved[i].req_id) {
					msg.set_req.offsets[cnt] = resolved[j].offset;
					msg.set_req.values[cnt] = values[j];
					cnt++;
					done[j] = true;
				}
			}
			msg.set_req.num_offsets = cnt;
			msg.set_req.num_values = cnt;

			client_exchange(sock, &msg);
			client_validate_response(&msg, MSG_TYPE_OK);
		}

		for (i = 0; i < (unsigned int)argc; i++)
			free(names[i]);
		free(done);
		free(names);
		free(values);
		free(resolved);
		close(sock);
	} else {
		if (argc < 2)
			die("request and at least one value required, try %s set --help",
			    get_prog_name());

		/*
		 * Request-id mode: detect positional vs offset=value sub-mode.
		 * All value args must be the same form; mixing is rejected.
		 */
		eq = strchr(argv[1], '=');
		offset_mode = (eq != NULL);

		for (i = 1; i < (unsigned int)argc; i++) {
			has_eq = (strchr(argv[i], '=') != NULL);
			if (has_eq != offset_mode)
				die("cannot mix positional values and offset=value pairs");
		}

		memset(&msg, 0, sizeof(msg));
		msg.type = MSG_TYPE_SET;
		msg.set_req.id = parse_request_id(argv[0]);

		if (offset_mode) {
			cnt = argc - 1;
			if (cnt > MAX_REQ_LINES)
				die("too many offset=value pairs specified");

			for (i = 0; i < cnt; i++) {
				eq = strchr(argv[i + 1], '=');
				if (!eq)
					die("expected offset=value, got '%s'", argv[i + 1]);
				off = strtoul(argv[i + 1], &end, 10);
				if (end != eq || off > UINT_MAX)
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
				msg.set_req.values[i] = parse_line_value_or_die(argv[i + 1]);
			msg.set_req.num_offsets = 0;
			msg.set_req.num_values = cnt;
		}

		sock = get_connection(cfg->socket_path);
		client_exchange(sock, &msg);
		client_validate_response(&msg, MSG_TYPE_OK);
		close(sock);
	}

	return EXIT_SUCCESS;
}

static void print_monitor_help(void)
{
	printf("Usage: %s [GLOBAL OPTIONS] monitor [OPTIONS] <request>\n",
	       get_prog_name());
	printf("\n");
	printf("Monitor edge events on a GPIO line request held by the daemon.\n");
	printf("\n");
	printf("<request> is the name or bare id as reported by the 'requests' sub-command.\n");
	printf("The request must have been made with edge detection enabled.\n");
	printf("\n");
	printf("Options:\n");
	printf("      --banner\t\t\tdisplay a banner on successful startup\n");
	printf("  -h, --help\t\t\tdisplay this help and exit\n");
	printf("  -n, --num-events <num>\texit after receiving num events\n");
}

static void parse_monitor_cmdline(int *argc, char ***argv, int *num_events,
				  bool *banner)
{
	static const struct option longopts[] = {
		{ "banner",	no_argument,		NULL,	'-' },
		{ "help",	no_argument,		NULL,	'h' },
		{ "num-events",	required_argument,	NULL,	'n' },
		{ GETOPT_NULL_LONGOPT }
	};
	static const char *const shortopts = "+hn:";

	int optc, opti;

	optind = 1;

	for (;;) {
		optc = getopt_long(*argc, *argv, shortopts, longopts, &opti);
		if (optc < 0)
			break;

		switch (optc) {
		case '-':
			*banner = true;
			break;
		case 'h':
			print_monitor_help();
			exit(EXIT_SUCCESS);
		case 'n':
			*num_events = parse_uint_or_die(optarg);
			break;
		case '?':
			die("try %s monitor --help", get_prog_name());
		default:
			abort();
		}
	}

	*argc -= optind;
	*argv += optind;
}

static const char *edge_type_str(unsigned int type)
{
	if (type == GPIOD_EDGE_EVENT_RISING_EDGE)
		return "rising";
	return "falling";
}

static int monitor_main(int argc, char **argv, const struct config *cfg)
{
	struct message msg = { .type = MSG_TYPE_MONITOR };
	int num_events = 0, events_done = 0, sock;
	bool banner = false;
	unsigned int id;

	parse_monitor_cmdline(&argc, &argv, &num_events, &banner);

	if (argc < 1)
		die("exactly one request must be specified, try %s monitor --help",
		    get_prog_name());
	if (argc > 1)
		die("only one request can be monitored at a time");

	id = parse_request_id(argv[0]);
	msg.monitor_req.id = id;

	sock = get_connection(cfg->socket_path);

	client_send_msg(sock, &msg);
	client_recv_msg(sock, &msg);
	client_validate_response(&msg, MSG_TYPE_OK);

	if (banner) {
		printf("Monitoring request%u...\n", id);
		fflush(stdout);
	}

	for (;;) {
		client_recv_msg_wait(sock, &msg);
		if (msg.type != MSG_TYPE_EDGE_EVENT)
			break;

		printf("%llu.%09llu %s %u\n",
		       (unsigned long long)(msg.edge_ev.timestamp_ns / 1000000000ULL),
		       (unsigned long long)(msg.edge_ev.timestamp_ns % 1000000000ULL),
		       edge_type_str(msg.edge_ev.event_type),
		       msg.edge_ev.offset);
		fflush(stdout);

		events_done++;
		if (num_events && events_done >= num_events)
			break;
	}

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
	printf("sub-command.  Options are the same as for the 'request' sub-command.\n");
}

static int reconfigure_main(int argc, char **argv, const struct config *cfg)
{
	struct message msg = { .type = MSG_TYPE_RECONFIGURE };
	struct request_opts opts = {};
	int sock, i, id_pos = -1;

	/*
	 * Find the request id: the first positional argument (doesn't start
	 * with '-').  Shift it out so parse_request_cmdline can process the
	 * remaining options and value specs cleanly.
	 */
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			id_pos = i;
			break;
		}
		/* Skip the argument of an option that takes one. */
		if (argv[i][1] != '-' && strchr("p", argv[i][1]) &&
		    argv[i][2] == '\0')
			i++;
	}

	if (id_pos < 0) {
		print_reconfigure_help();
		die("request id required, try %s reconfigure --help",
		    get_prog_name());
	}

	msg.reconfigure.id = parse_request_id(argv[id_pos]);

	/* Remove the id from argv so getopt only sees options and value specs. */
	memmove(&argv[id_pos], &argv[id_pos + 1],
		(argc - id_pos - 1) * sizeof(*argv));
	argc--;

	parse_request_cmdline(&argc, &argv, &opts);
	validate_request_opts(&opts);

	apply_opts_to_config(&msg.reconfigure.cfg, &opts, argc, argv);

	sock = get_connection(cfg->socket_path);

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
	printf("      --by-name\t\tTreat lines as names even if they would parse as an offset.\n");
	printf("  -c, --chip <chip>\tRestrict scope to a particular chip.\n");
	printf("  -C, --consumer <name>\tConsumer name applied to requested lines\n");
	printf("\t\t\t(default is 'gpioctl').\n");
	printf("  -h, --help\t\tDisplay this help and exit.\n");
	printf("  -p, --socket-path\tPath to the unix-domain socket used for\n");
	printf("\t\t\tcommunication between client and server.\n");
	printf("  -s, --strict\t\tAbort if requested line names are not unique.\n");
	printf("  -v, --version\t\tOutput version information and exit.\n");
}

static void parse_global_cmdline(int *argc, char ***argv, struct config *cfg)
{
	static const struct option longopts[] = {
		{ "help",		no_argument,		NULL,	'h' },
		{ "version",		no_argument,		NULL,	'v' },
		{ "chip",		required_argument,	NULL,	'c' },
		{ "strict",		no_argument,		NULL,	's' },
		{ "consumer",		required_argument,	NULL,	'C' },
		{ "by-name",		no_argument,		NULL,	'B' },
		{ "socket-path",	required_argument,	NULL,	'p' },
		{ GETOPT_NULL_LONGOPT },
	};
	static const char *const shortopts = "+hvc:sC:Bp:";

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
		case 'p':
			cfg->socket_path = optarg;
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
	struct config cfg = { .consumer = "gpioctl" };
	const struct sub_command *subcmd;

	set_prog_name(argv[0]);
	parse_global_cmdline(&argc, &argv, &cfg);

	subcmd = find_sub_cmd(argv[0]);
	if (!subcmd)
		die("invalid sub-command, try %s --help", get_prog_name());

	return subcmd->sub_main(argc, argv, &cfg);
}
