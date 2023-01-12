// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>
// SPDX-FileCopyrightText: 2022 Kent Gibson <warthog618@gmail.com>

#include <ctype.h>
#include <gpiod.h>
#include <getopt.h>
#include <limits.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#if GPIOSET_INTERACTIVE
#include <editline/readline.h>
#endif

#include "tools-common.h"

struct config {
	bool active_low;
	bool banner;
	bool by_name;
	bool daemonize;
	bool interactive;
	bool strict;
	bool unquoted;
	enum gpiod_line_bias bias;
	enum gpiod_line_drive drive;
	int toggles;
	unsigned int *toggle_periods;
	unsigned int hold_period_us;
	const char *chip_id;
	const char *consumer;
};

static void print_help(void)
{
	printf("Usage: %s [OPTIONS] <line=value>...\n", get_progname());
	printf("\n");
	printf("Set values of GPIO lines.\n");
	printf("\n");
	printf("Lines are specified by name, or optionally by offset if the chip option\n");
	printf("is provided.\n");
	printf("Values may be '1' or '0', or equivalently 'active'/'inactive' or 'on'/'off'.\n");
	printf("\n");
	printf("The line output state is maintained until the process exits, but after that\n");
	printf("is not guaranteed.\n");
	printf("\n");
	printf("Options:\n");
	printf("      --banner\t\tdisplay a banner on successful startup\n");
	print_bias_help();
	printf("      --by-name\t\ttreat lines as names even if they would parse as an offset\n");
	printf("  -c, --chip <chip>\trestrict scope to a particular chip\n");
	printf("  -C, --consumer <name>\tconsumer name applied to requested lines (default is 'gpioset')\n");
	printf("  -d, --drive <drive>\tspecify the line drive mode\n");
	printf("\t\t\tPossible values: 'push-pull', 'open-drain', 'open-source'.\n");
	printf("\t\t\t(default is 'push-pull')\n");
	printf("  -h, --help\t\tdisplay this help and exit\n");
#ifdef GPIOSET_INTERACTIVE
	printf("  -i, --interactive\tset the lines then wait for additional set commands\n");
	printf("\t\t\tUse the 'help' command at the interactive prompt to get help\n");
	printf("\t\t\tfor the supported commands.\n");
#endif
	printf("  -l, --active-low\ttreat the line as active low\n");
	printf("  -p, --hold-period <period>\n");
	printf("\t\t\tthe minimum time period to hold lines at the requested values\n");
	printf("  -s, --strict\t\tabort if requested line names are not unique\n");
	printf("  -t, --toggle <period>[,period]...\n");
	printf("\t\t\ttoggle the line(s) after the specified period(s)\n");
	printf("\t\t\tIf the last period is non-zero then the sequence repeats.\n");
	printf("      --unquoted\tdon't quote line names\n");
	printf("  -v, --version\t\toutput version information and exit\n");
	printf("  -z, --daemonize\tset values then detach from the controlling terminal\n");
	print_chip_help();
	print_period_help();
	printf("\n");
	printf("*Note*\n");
	printf("    The state of a GPIO line controlled over the character device reverts to default\n");
	printf("    when the last process referencing the file descriptor representing the device file exits.\n");
	printf("    This means that it's wrong to run gpioset, have it exit and expect the line to continue\n");
	printf("    being driven high or low. It may happen if given pin is floating but it must be interpreted\n");
	printf("    as undefined behavior.\n");
}

static int parse_drive_or_die(const char *option)
{
	if (strcmp(option, "open-drain") == 0)
		return GPIOD_LINE_DRIVE_OPEN_DRAIN;
	if (strcmp(option, "open-source") == 0)
		return GPIOD_LINE_DRIVE_OPEN_SOURCE;
	if (strcmp(option, "push-pull") != 0)
		die("invalid drive: %s", option);

	return 0;
}

static int parse_periods_or_die(char *option, unsigned int **periods)
{
	int i, num_periods = 1;
	unsigned int *pp;
	char *end;

	for (i = 0; option[i] != '\0'; i++)
		if (option[i] == ',')
			num_periods++;

	pp = calloc(num_periods, sizeof(*pp));
	if (pp == NULL)
		die("out of memory");

	for (i = 0; i < num_periods - 1; i++) {
		for (end = option; *end != ','; end++)
			;

		*end = '\0';
		pp[i] = parse_period_or_die(option);
		option = end + 1;
	}
	pp[i] = parse_period_or_die(option);
	*periods = pp;

	return num_periods;
}

static int parse_config(int argc, char **argv, struct config *cfg)
{
	static const struct option longopts[] = {
		{ "active-low",	no_argument,		NULL,	'l' },
		{ "banner",	no_argument,		NULL,	'-'},
		{ "bias",	required_argument,	NULL,	'b' },
		{ "by-name",	no_argument,		NULL,	'B' },
		{ "chip",	required_argument,	NULL,	'c' },
		{ "consumer",	required_argument,	NULL,	'C' },
		{ "daemonize",	no_argument,		NULL,	'z' },
		{ "drive",	required_argument,	NULL,	'd' },
		{ "help",	no_argument,		NULL,	'h' },
		{ "hold-period", required_argument,	NULL,	'p' },
#ifdef GPIOSET_INTERACTIVE
		{ "interactive", no_argument,		NULL,	'i' },
#endif
		{ "strict",	no_argument,		NULL,	's' },
		{ "toggle",	required_argument,	NULL,	't' },
		{ "unquoted",	no_argument,		NULL,	'Q' },
		{ "version",	no_argument,		NULL,	'v' },
		{ GETOPT_NULL_LONGOPT },
	};

#ifdef GPIOSET_INTERACTIVE
	static const char *const shortopts = "+b:c:C:d:hilp:st:vz";
#else
	static const char *const shortopts = "+b:c:C:d:hlp:st:vz";
#endif

	int opti, optc;

	memset(cfg, 0, sizeof(*cfg));
	cfg->consumer = "gpioset";

	for (;;) {
		optc = getopt_long(argc, argv, shortopts, longopts, &opti);
		if (optc < 0)
			break;

		switch (optc) {
		case '-':
			cfg->banner = true;
			break;
		case 'b':
			cfg->bias = parse_bias_or_die(optarg);
			break;
		case 'B':
			cfg->by_name = true;
			break;
		case 'c':
			cfg->chip_id = optarg;
			break;
		case 'C':
			cfg->consumer = optarg;
			break;
		case 'd':
			cfg->drive = parse_drive_or_die(optarg);
			break;
#ifdef GPIOSET_INTERACTIVE
		case 'i':
			cfg->interactive = true;
			break;
#endif
		case 'l':
			cfg->active_low = true;
			break;
		case 'p':
			cfg->hold_period_us = parse_period_or_die(optarg);
			break;
		case 'Q':
			cfg->unquoted = true;
			break;
		case 's':
			cfg->strict = true;
			break;
		case 't':
			cfg->toggles = parse_periods_or_die(optarg,
						 &cfg->toggle_periods);
			break;
		case 'z':
			cfg->daemonize = true;
			break;
		case 'h':
			print_help();
			exit(EXIT_SUCCESS);
		case 'v':
			print_version();
			exit(EXIT_SUCCESS);
		case '?':
			die("try %s --help", get_progname());
		case 0:
			break;
		default:
			abort();
		}
	}

#ifdef GPIOSET_INTERACTIVE
	if (cfg->toggles && cfg->interactive)
		die("can't combine interactive with toggle");
#endif

	return optind;
}

static enum gpiod_line_value parse_value(const char *option)
{
	if (strcmp(option, "0") == 0)
		return GPIOD_LINE_VALUE_INACTIVE;
	if (strcmp(option, "1") == 0)
		return GPIOD_LINE_VALUE_ACTIVE;
	if (strcmp(option, "inactive") == 0)
		return GPIOD_LINE_VALUE_INACTIVE;
	if (strcmp(option, "active") == 0)
		return GPIOD_LINE_VALUE_ACTIVE;
	if (strcmp(option, "off") == 0)
		return GPIOD_LINE_VALUE_INACTIVE;
	if (strcmp(option, "on") == 0)
		return GPIOD_LINE_VALUE_ACTIVE;
	if (strcmp(option, "false") == 0)
		return GPIOD_LINE_VALUE_INACTIVE;
	if (strcmp(option, "true") == 0)
		return GPIOD_LINE_VALUE_ACTIVE;

	return GPIOD_LINE_VALUE_ERROR;
}

/*
 * Parse line id and values from lvs into lines and values.
 *
 * Accepted forms:
 *	'line=value'
 *	'"line"=value'
 *
 * If line id is quoted then it is returned unquoted.
 */
static bool parse_line_values(int num_lines, char **lvs, char **lines,
			      enum gpiod_line_value *values, bool interactive)
{
	char *value, *line;
	int i;

	for (i = 0; i < num_lines; i++) {
		line = lvs[i];

		if (*line != '"') {
			value = strchr(line, '=');
		} else {
			line++;
			value = strstr(line, "\"=");
			if (value) {
				*value = '\0';
				value++;
			}
		}

		if (!value) {
			if (interactive)
				printf("invalid line value: '%s'\n", lvs[i]);
			else
				print_error("invalid line value: '%s'", lvs[i]);

			return false;
		}

		*value = '\0';
		value++;
		values[i] = parse_value(value);

		if (values[i] == GPIOD_LINE_VALUE_ERROR) {
			if (interactive)
				printf("invalid line value: '%s'\n", value);
			else
				print_error("invalid line value: '%s'", value);

			return false;
		}

		lines[i] = line;
	}

	return true;
}

/*
 * Parse line id and values from lvs into lines and values, or die trying.
 */
static void parse_line_values_or_die(int num_lines, char **lvs, char **lines,
				     enum gpiod_line_value *values)
{
	if (!parse_line_values(num_lines, lvs, lines, values, false))
		exit(EXIT_FAILURE);
}

static void print_banner(int num_lines, char **lines)
{
	int i;

	if (num_lines > 1) {
		printf("Setting lines ");

		for (i = 0; i < num_lines - 1; i++)
			printf("'%s', ", lines[i]);

		printf("and '%s'...\n", lines[i]);
	} else {
		printf("Setting line '%s'...\n", lines[0]);
	}
	fflush(stdout);
}

static void wait_fd(int fd)
{
	struct pollfd pfd;

	pfd.fd = fd;
	pfd.events = POLLERR;

	if (poll(&pfd, 1, -1) < 0)
		die_perror("error waiting on request");
}

/*
 * Apply values from the resolver to the requests.
 * offset and values are scratch pads for working.
 */
static void apply_values(struct gpiod_line_request **requests,
			 struct line_resolver *resolver, unsigned int *offsets,
			 enum gpiod_line_value *values)
{
	int i;

	for (i = 0; i < resolver->num_chips; i++) {
		get_line_offsets_and_values(resolver, i, offsets, values);
		if (gpiod_line_request_set_values(requests[i], values))
			print_perror("unable to set values on '%s'",
				     get_chip_name(resolver, i));
	}
}

/* Toggle the values of all lines in the resolver */
static void toggle_all_lines(struct line_resolver *resolver)
{
	int i;

	for (i = 0; i < resolver->num_lines; i++)
		resolver->lines[i].value = !resolver->lines[i].value;
}

/*
 * Toggle the resolved lines as specified by the toggle_periods,
 * and apply the values to the requests.
 * offset and values are scratch pads for working.
 */
static void toggle_sequence(int toggles, unsigned int *toggle_periods,
			    struct gpiod_line_request **requests,
			    struct line_resolver *resolver,
			    unsigned int *offsets,
			    enum gpiod_line_value *values)
{
	int i = 0;

	if ((toggles == 1) && (toggle_periods[0] == 0))
		return;

	for (;;) {
		usleep(toggle_periods[i]);
		toggle_all_lines(resolver);
		apply_values(requests, resolver, offsets, values);

		i++;
		if ((i == toggles - 1) && (toggle_periods[i] == 0))
			return;

		if (i == toggles)
			i = 0;
	}
}

#ifdef GPIOSET_INTERACTIVE

/*
 * Parse line id from words into lines.
 *
 * If line id is quoted then it is returned unquoted.
 */
static bool parse_line_ids(int num_lines, char **words, char **lines)
{
	int i, len;
	char *line;

	for (i = 0; i < num_lines; i++) {
		line = words[i];
		if (*line == '"') {
			line++;
			len = strlen(line);
			if ((len == 0) || line[len - 1] != '"') {
				printf("invalid line id: '%s'\n", words[i]);
				return false;
			}
			line[len - 1] = '\0';
		}
		lines[i] = line;
	}

	return true;
}

/*
 * Set the values in the resolver for the line values specified by
 * the remaining parameters.
 */
static void set_line_values_subset(struct line_resolver *resolver,
				   int num_lines, char **lines,
				   enum gpiod_line_value *values)
{
	int l, i;

	for (l = 0; l < num_lines; l++) {
		for (i = 0; i < resolver->num_lines; i++) {
			if (strcmp(lines[l], resolver->lines[i].id) == 0) {
				resolver->lines[i].value = values[l];
				break;
			}
		}
	}
}

static void print_all_line_values(struct line_resolver *resolver, bool unquoted)
{
	char *fmt = unquoted ? "%s=%s " : "\"%s\"=%s ";
	int i;

	for (i = 0; i < resolver->num_lines; i++) {
		if (i == resolver->num_lines - 1)
			fmt = unquoted ? "%s=%s\n" : "\"%s\"=%s\n";

		printf(fmt, resolver->lines[i].id,
		       resolver->lines[i].value ? "active" : "inactive");
	}
}

/*
 * Print the resovler line values for a subset of lines, specified by
 * num_lines and lines.
 */
static void print_line_values(struct line_resolver *resolver, int num_lines,
			      char **lines, bool unquoted)
{
	char *fmt = unquoted ? "%s=%s " : "\"%s\"=%s ";
	struct resolved_line *line;
	int i, j;

	for (i = 0; i < num_lines; i++) {
		if (i == num_lines - 1)
			fmt = unquoted ? "%s=%s\n" : "\"%s\"=%s\n";

		for (j = 0; j < resolver->num_lines; j++) {
			line = &resolver->lines[j];
			if (strcmp(lines[i], line->id) == 0) {
				printf(fmt, line->id,
				       line->value ? "active" : "inactive");
				break;
			}
		}
	}
}

/*
 * Toggle a subset of lines, specified by num_lines and lines, in the resolver.
 */
static void toggle_lines(struct line_resolver *resolver, int num_lines,
			 char **lines)
{
	struct resolved_line *line;
	int i, j;

	for (i = 0; i < num_lines; i++)
		for (j = 0; j < resolver->num_lines; j++) {
			line = &resolver->lines[j];
			if (strcmp(lines[i], line->id) == 0) {
				line->value = !line->value;
				break;
			}
		}
}

/*
 * Check that a set of lines, specified by num_lines and lines, are all
 * resolved lines.
 */
static bool valid_lines(struct line_resolver *resolver, int num_lines,
			char **lines)
{
	bool ret = true, found;
	int i, l;

	for (l = 0; l < num_lines; l++) {
		found = false;

		for (i = 0; i < resolver->num_lines; i++) {
			if (strcmp(lines[l], resolver->lines[i].id) == 0) {
				found = true;
				break;
			}
		}

		if (!found) {
			printf("unknown line: '%s'\n", lines[l]);
			ret = false;
		}
	}

	return ret;
}

static void print_interactive_help(void)
{
	printf("COMMANDS:\n\n");
	printf("    exit\n");
	printf("        Exit the program\n");
	printf("    get [line]...\n");
	printf("        Display the output values of the given requested lines\n\n");
	printf("        If no lines are specified then all requested lines are displayed\n\n");
	printf("    help\n");
	printf("        Print this help\n\n");
	printf("    set <line=value>...\n");
	printf("        Update the output values of the given requested lines\n\n");
	printf("    sleep <period>\n");
	printf("        Sleep for the specified period\n\n");
	printf("    toggle [line]...\n");
	printf("        Toggle the output values of the given requested lines\n\n");
	printf("        If no lines are specified then all requested lines are toggled\n\n");
}

/*
 * Split a line into words, returning the each of the words and the count.
 *
 * max_words specifies the max number of words that may be returned in words.
 *
 * Any escaping is ignored, on the assumption that the only escaped
 * character of consequence is '"', and that names won't include quotes.
 */
static int split_words(char *line, int max_words, char **words)
{
	bool in_quotes = false, in_word = false;
	int num_words = 0;

	for (; (*line != '\0'); line++) {
		if (!in_word) {
			if (isspace(*line))
				continue;

			in_word = true;
			in_quotes = (*line == '"');

			/* count all words, but only store max_words */
			if (num_words < max_words)
				words[num_words] = line;
		} else {
			if (in_quotes) {
				if (*line == '"')
					in_quotes = false;
				continue;
			}
			if (isspace(*line)) {
				num_words++;
				in_word = false;
				*line = '\0';
			}
		}
	}

	if (in_word)
		num_words++;

	return num_words;
}

/* check if a line is specified somewhere in the rl_line_buffer */
static bool in_line_buffer(const char *id)
{
	char *match = rl_line_buffer;
	int len = strlen(id);

	while ((match = strstr(match, id))) {
		if ((match > rl_line_buffer && isspace(match[-1])) &&
		    ((match[len] == '=') || isspace(match[len])))
			return true;

		match += len;
	}

	return false;
}

/* context for complete_line_id, so it can provide valid line ids */
static struct line_resolver *completion_context;

/* tab completion helper for line ids */
static char *complete_line_id(const char *text, int state)
{
	static int idx, len;
	const char *id;

	if (!state) {
		idx = 0;
		len = strlen(text);
	}

	while (idx < completion_context->num_lines) {
		id = completion_context->lines[idx].id;
		idx++;

		if ((strncmp(id, text, len) == 0) && (!in_line_buffer(id)))
			return strdup(id);
	}

	return NULL;
}

/* tab completion helper for line values (just the value component) */
static char *complete_value(const char *text, int state)
{
	static const char * const values[] = {
		"1", "0", "active", "inactive", "on", "off", "true", "false",
		NULL
	};

	static int idx, len;

	const char *value;

	if (!state) {
		idx = 0;
		len = strlen(text);
	}

	while ((value = values[idx])) {
		idx++;
		if (strncmp(value, text, len) == 0)
			return strdup(value);
	}

	return NULL;
}

/* tab completion help for interactive commands */
static char *complete_command(const char *text, int state)
{
	static const char * const commands[] = {
		"get", "set", "toggle", "sleep", "help", "exit", NULL
	};

	static int idx, len;

	const char *cmd;

	if (!state) {
		idx = 0;
		len = strlen(text);
	}

	while ((cmd = commands[idx])) {
		idx++;
		if (strncmp(cmd, text, len) == 0)
			return strdup(cmd);
	}
	return NULL;
}

/* tab completion for interactive command lines */
static char **tab_completion(const char *text, int start, int end)
{
	int cmd_start, cmd_end, len;
	char **matches = NULL;

	rl_attempted_completion_over = true;
	rl_completion_type = '@';
	rl_completion_append_character = ' ';

	for (cmd_start = 0;
	     isspace(rl_line_buffer[cmd_start]) && cmd_start < end; cmd_start++)
		;

	if (cmd_start == start)
		matches = rl_completion_matches(text, complete_command);

	for (cmd_end = cmd_start + 1;
	     !isspace(rl_line_buffer[cmd_end]) && cmd_end < end; cmd_end++)
		;

	len = cmd_end - cmd_start;
	if (len == 3 && strncmp("set ", &rl_line_buffer[cmd_start], 4) == 0) {
		if (rl_line_buffer[start - 1] == '=') {
			matches = rl_completion_matches(text, complete_value);
		} else {
			rl_completion_append_character = '=';
			matches = rl_completion_matches(text, complete_line_id);
		}
	}

	if ((len == 3 && strncmp("get ", &rl_line_buffer[cmd_start], 4) == 0) ||
	    (len == 6 &&
	     strncmp("toggle ", &rl_line_buffer[cmd_start], 7) == 0))
		matches = rl_completion_matches(text, complete_line_id);

	return matches;
}

#define PROMPT "gpioset> "

static void interact(struct gpiod_line_request **requests,
		     struct line_resolver *resolver, char **lines,
		     unsigned int *offsets, enum gpiod_line_value *values,
		     bool unquoted)
{
	int num_words, num_lines, max_words, period_us, i;
	char *line, **words, *line_buf;
	bool done, stdout_is_tty;

	stifle_history(20);
	rl_attempted_completion_function = tab_completion;
	rl_basic_word_break_characters = " =\"";
	completion_context = resolver;
	stdout_is_tty = isatty(1);

	max_words = resolver->num_lines + 1;
	words = calloc(max_words, sizeof(*words));
	if (!words)
		die("out of memory");

	for (done = false; !done;) {
		/*
		 * manually print the prompt, as libedit doesn't if stdout
		 * is not a tty.  And fflush to ensure the prompt and any
		 * output buffered from the previous command is sent.
		 */
		if (!stdout_is_tty)
			printf(PROMPT);
		fflush(stdout);

		line = readline(PROMPT);
		if (!line || line[0] == '\0') {
			free(line);
			continue;
		}

		for (i = strlen(line) - 1; (i > 0) && isspace(line[i]); i--)
			line[i] = '\0';

		line_buf = strdup(line);
		num_words = split_words(line_buf, max_words, words);
		if (num_words > max_words) {
			printf("too many command parameters provided\n");
			goto cmd_done;
		}
		num_lines = num_words - 1;
		if (strcmp(words[0], "get") == 0) {
			if (num_lines == 0)
				print_all_line_values(resolver, unquoted);
			else if (parse_line_ids(num_lines, &words[1], lines) &&
				 valid_lines(resolver, num_lines, lines))
				print_line_values(resolver, num_lines, lines,
						  unquoted);
			goto cmd_ok;
		}
		if (strcmp(words[0], "set") == 0) {
			if (num_lines == 0)
				printf("at least one GPIO line value must be specified\n");
			else if (parse_line_values(num_lines, &words[1], lines,
						   values, true) &&
				 valid_lines(resolver, num_lines, lines)) {
				set_line_values_subset(resolver, num_lines,
						       lines, values);
				apply_values(requests, resolver, offsets,
					     values);
			}
			goto cmd_ok;
		}
		if (strcmp(words[0], "toggle") == 0) {
			if (num_lines == 0)
				toggle_all_lines(resolver);
			else if (parse_line_ids(num_lines, &words[1], lines) &&
				 valid_lines(resolver, num_lines, lines))
				toggle_lines(resolver, num_lines, lines);

			apply_values(requests, resolver, offsets, values);
			goto cmd_ok;
		}
		if (strcmp(words[0], "sleep") == 0) {
			if (num_lines == 0) {
				printf("a period must be specified\n");
				goto cmd_ok;
			}
			if (num_lines > 1) {
				printf("only one period can be specified\n");
				goto cmd_ok;
			}
			period_us = parse_period(words[1]);
			if (period_us < 0) {
				printf("invalid period: '%s'\n", words[1]);
				goto cmd_ok;
			}
			usleep(period_us);
			goto cmd_ok;
		}

		if (strcmp(words[0], "exit") == 0) {
			done = true;
			goto cmd_done;
		}

		if (strcmp(words[0], "help") == 0) {
			print_interactive_help();
			goto cmd_done;
		}

		printf("unknown command: '%s'\n", words[0]);
		printf("Try the 'help' command\n");

cmd_ok:
		for (i = 0; isspace(line[i]); i++)
			;

		if ((history_length) == 0 ||
		    (strcmp(history_list()[history_length - 1]->line,
			    &line[i]) != 0))
			add_history(&line[i]);

cmd_done:
		free(line);
		free(line_buf);
	}
	free(words);
}

#endif /* GPIOSET_INTERACTIVE */

int main(int argc, char **argv)
{
	struct gpiod_line_settings *settings;
	struct gpiod_request_config *req_cfg;
	struct gpiod_line_request **requests;
	struct gpiod_line_config *line_cfg;
	struct line_resolver *resolver;
	enum gpiod_line_value *values;
	struct gpiod_chip *chip;
	unsigned int *offsets;
	int i, num_lines, ret;
	struct config cfg;
	char **lines;

	i = parse_config(argc, argv, &cfg);
	argc -= i;
	argv += i;

	if (argc < 1)
		die("at least one GPIO line value must be specified");

	num_lines = argc;

	lines = calloc(num_lines, sizeof(*lines));
	values = calloc(num_lines, sizeof(*values));
	if (!lines || !values)
		die("out of memory");

	parse_line_values_or_die(argc, argv, lines, values);

	settings = gpiod_line_settings_new();
	if (!settings)
		die_perror("unable to allocate line settings");

	if (cfg.bias)
		gpiod_line_settings_set_bias(settings, cfg.bias);

	if (cfg.drive)
		gpiod_line_settings_set_drive(settings, cfg.drive);

	if (cfg.active_low)
		gpiod_line_settings_set_active_low(settings, true);

	gpiod_line_settings_set_direction(settings,
					  GPIOD_LINE_DIRECTION_OUTPUT);

	req_cfg = gpiod_request_config_new();
	if (!req_cfg)
		die_perror("unable to allocate the request config structure");

	gpiod_request_config_set_consumer(req_cfg, cfg.consumer);
	resolver = resolve_lines(num_lines, lines, cfg.chip_id, cfg.strict,
				 cfg.by_name);
	validate_resolution(resolver, cfg.chip_id);
	for (i = 0; i < num_lines; i++)
		resolver->lines[i].value = values[i];

	requests = calloc(resolver->num_chips, sizeof(*requests));
	offsets = calloc(num_lines, sizeof(*offsets));
	if (!requests || !offsets)
		die("out of memory");

	line_cfg = gpiod_line_config_new();
	if (!line_cfg)
		die_perror("unable to allocate the line config structure");

	for (i = 0; i < resolver->num_chips; i++) {
		num_lines = get_line_offsets_and_values(resolver, i, offsets,
							values);

		gpiod_line_config_reset(line_cfg);

		ret = gpiod_line_config_add_line_settings(line_cfg, offsets,
							  num_lines, settings);
		if (ret)
			die_perror("unable to add line settings");

		ret = gpiod_line_config_set_output_values(line_cfg,
							  values, num_lines);
		if (ret)
			die_perror("unable to set output values");

		chip = gpiod_chip_open(resolver->chips[i].path);
		if (!chip)
			die_perror("unable to open chip '%s'",
				   resolver->chips[i].path);

		requests[i] = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
		if (!requests[i])
			die_perror("unable to request lines on chip '%s'",
				   resolver->chips[i].path);

		gpiod_chip_close(chip);
	}

	gpiod_request_config_free(req_cfg);
	gpiod_line_config_free(line_cfg);
	gpiod_line_settings_free(settings);

	if (cfg.banner)
		print_banner(argc, lines);

	if (cfg.daemonize)
		if (daemon(0, cfg.interactive) < 0)
			die_perror("unable to daemonize");

	if (cfg.toggles) {
		for (i = 0; i < cfg.toggles; i++)
			if ((cfg.hold_period_us > cfg.toggle_periods[i]) &&
			    ((i != cfg.toggles - 1) ||
			     cfg.toggle_periods[i] != 0))
				cfg.toggle_periods[i] = cfg.hold_period_us;

		toggle_sequence(cfg.toggles, cfg.toggle_periods, requests,
				resolver, offsets, values);
		free(cfg.toggle_periods);
	}

	if (cfg.hold_period_us)
		usleep(cfg.hold_period_us);

#ifdef GPIOSET_INTERACTIVE
	if (cfg.interactive)
		interact(requests, resolver, lines, offsets, values,
			 cfg.unquoted);
	else if (!cfg.toggles)
		wait_fd(gpiod_line_request_get_fd(requests[0]));
#else
	if (!cfg.toggles)
		wait_fd(gpiod_line_request_get_fd(requests[0]));
#endif

	for (i = 0; i < resolver->num_chips; i++)
		gpiod_line_request_release(requests[i]);

	free(requests);
	free_line_resolver(resolver);
	free(lines);
	free(values);
	free(offsets);

	return EXIT_SUCCESS;
}
