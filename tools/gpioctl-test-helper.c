// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Bartosz Golaszewski <bartosz.golaszewski@oss.qualcomm.com>
// SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "tools-common.h"

/*
 * Test helper: connect to an AF_UNIX SOCK_SEQPACKET socket and send a single
 * message of exactly <num-bytes> zero bytes. Used by the gpio-tools test
 * suite to forge malformed messages to the gpioctl server.
 */

int main(int argc, char **argv)
{
	struct sockaddr_un addr;
	unsigned long num_bytes;
	char *end, *buf;
	int sock;

	set_prog_name(argv[0]);

	if (argc != 3)
		die("usage: %s <socket-path> <num-bytes>", get_prog_name());

	num_bytes = strtoul(argv[2], &end, 10);
	if (errno || *end != '\0' || end == argv[2])
		die("invalid byte count: %s", argv[2]);

	buf = calloc(1, num_bytes ? num_bytes : 1);
	if (!buf)
		die_oom();

	sock = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if (sock < 0)
		die_perror("socket");

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, argv[1], sizeof(addr.sun_path) - 1);

	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		die_perror("connect");

	if (send(sock, buf, num_bytes, 0) < 0)
		die_perror("send");

	close(sock);
	free(buf);

	return EXIT_SUCCESS;
}
