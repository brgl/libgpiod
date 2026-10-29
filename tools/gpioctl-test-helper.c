// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Bartosz Golaszewski <bartosz.golaszewski@oss.qualcomm.com>
// SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

/*
 * Test helper for forging messages to the gpioctl server in the gpio-tools
 * test suite. Two modes:
 *
 *   zero-msg <socket-path> <num-bytes>
 *     Send a single message of exactly <num-bytes> zero bytes.
 *
 *   reconfigure <socket-path> <request-id> <offset>=<value>...
 *     Send a reconfigure message, switching the request to output mode, with
 *     the offsets and values exactly as given, bypassing the checks done by
 *     the client. Print "ok" or "error <errno>" depending on the reply.
 */

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

#define PACKED			__attribute__((__packed__))

/*
 * Just the parts of the gpioctl wire protocol needed to forge a reconfigure
 * message: keep them in sync with tools/gpioctl.c. The server only accepts
 * messages of exactly the size of their type so the reconfigure payload must
 * be mirrored in full.
 */
#define MSG_TYPE_OK		0
#define MSG_TYPE_ERROR		1
#define MSG_TYPE_RECONFIGURE	14
#define MAX_REQ_LINES		64

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

struct message {
	unsigned int type;
	union {
		struct {
			unsigned int errnum;
		} PACKED err;
		struct {
			unsigned int id;
			struct request_config cfg;
		} PACKED reconfigure;
	};
} PACKED;

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

static int zero_msg_main(int argc, char **argv)
{
	unsigned long num_bytes;
	char *buf;
	int sock;

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
	char *cmd;

	set_prog_name(argv[0]);

	if (argc < 1)
		die("Usage: gpioctl-test-helper <command> ...");

	cmd = argv[1];

	argc -= 1;
	argv += 1;

	if (strcmp(cmd, "reconfigure") == 0)
		return reconfigure_main(argc, argv);
	else if (strcmp(cmd, "zero-msg") == 0)
		return zero_msg_main(argc, argv);

	die("Invalid command: '%s'", cmd);
}
