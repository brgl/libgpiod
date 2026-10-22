// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Bartosz Golaszewski <bartosz.golaszewski@oss.qualcomm.com>
// SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

#include <errno.h>
#include <gpiod.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "tools-common.h"

#define PACKED	__attribute__((__packed__))

/*
 * Just the parts of the gpioctl wire protocol needed to forge a reconfigure
 * message: keep them in sync with tools/gpioctl.c. The server only accepts
 * messages of exactly the size of its struct message so pad ours to that.
 */
#define MSG_TYPE_OK		0
#define MSG_TYPE_ERROR		1
#define MSG_TYPE_RECONFIGURE	14
#define MSG_SIZE		2380
#define MAX_REQ_LINES		64

struct request_config {
	unsigned int offsets[MAX_REQ_LINES];
	unsigned int num_offsets;
	enum gpiod_line_value values[MAX_REQ_LINES];
	bool active_low;
	enum gpiod_line_direction direction;
} PACKED;

struct message {
	unsigned int type;
	union {
		struct {
			unsigned int errnum;
		} err;
		struct {
			unsigned int id;
			struct request_config cfg;
		} reconfigure;
		char pad[MSG_SIZE - sizeof(unsigned int)];
	};
} PACKED;

_Static_assert(sizeof(struct message) == MSG_SIZE, "bad message size");

/*
 * Test helper for forging messages to the gpioctl server in the gpio-tools
 * test suite. Two modes:
 *
 *   <socket-path> <num-bytes>
 *     Send a single message of exactly <num-bytes> zero bytes.
 *
 *   reconfigure <socket-path> <request-id> <offset>=<value>...
 *     Send a reconfigure message, switching the request to output mode, with
 *     the offsets and values exactly as given, bypassing the checks done by
 *     the client. Print "ok" or "error <errno>" depending on the reply.
 */

static int connect_to_server(const char *path)
{
	struct sockaddr_un addr;
	int sock;

	sock = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if (sock < 0)
		die_perror("socket");

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		die_perror("connect");

	return sock;
}

static unsigned long parse_ulong_or_die(const char *str)
{
	unsigned long val;
	char *end;

	errno = 0;
	val = strtoul(str, &end, 10);
	if (errno || *end != '\0' || end == str)
		die("invalid number: %s", str);

	return val;
}

static int reconfigure_main(int argc, char **argv)
{
	struct message msg;
	int i, sock;
	char *end;

	if (argc < 4 || argc - 3 > MAX_REQ_LINES)
		die("usage: %s reconfigure <socket-path> <request-id> [<offset>=<value>]...",
		    get_prog_name());

	memset(&msg, 0, sizeof(msg));
	msg.type = MSG_TYPE_RECONFIGURE;
	msg.reconfigure.id = parse_ulong_or_die(argv[2]);
	msg.reconfigure.cfg.direction = GPIOD_LINE_DIRECTION_OUTPUT;
	msg.reconfigure.cfg.num_offsets = argc - 3;

	for (i = 3; i < argc; i++) {
		msg.reconfigure.cfg.offsets[i - 3] = strtoul(argv[i], &end, 10);
		if (*end != '=')
			die("invalid offset=value pair: %s", argv[i]);

		msg.reconfigure.cfg.values[i - 3] =
				parse_ulong_or_die(end + 1) ?
				GPIOD_LINE_VALUE_ACTIVE :
				GPIOD_LINE_VALUE_INACTIVE;
	}

	sock = connect_to_server(argv[1]);

	if (send(sock, &msg, sizeof(msg), 0) < 0)
		die_perror("send");

	if (recv(sock, &msg, sizeof(msg), 0) < 0)
		die_perror("recv");

	if (msg.type == MSG_TYPE_OK)
		printf("ok\n");
	else if (msg.type == MSG_TYPE_ERROR)
		printf("error %u\n", msg.err.errnum);
	else
		die("unexpected reply of type %u", msg.type);

	close(sock);

	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
	unsigned long num_bytes;
	char *buf;
	int sock;

	set_prog_name(argv[0]);

	if (argc > 1 && strcmp(argv[1], "reconfigure") == 0)
		return reconfigure_main(argc - 1, argv + 1);

	if (argc != 3)
		die("usage: %s <socket-path> <num-bytes>", get_prog_name());

	num_bytes = parse_ulong_or_die(argv[2]);

	buf = calloc(1, num_bytes ? num_bytes : 1);
	if (!buf)
		die_oom();

	sock = connect_to_server(argv[1]);

	if (send(sock, buf, num_bytes, 0) < 0)
		die_perror("send");

	close(sock);
	free(buf);

	return EXIT_SUCCESS;
}
