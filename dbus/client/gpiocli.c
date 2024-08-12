// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2022-2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <glib/gstdio.h>

#include "common.h"

typedef struct {
	gchar *name;
	int (*main_func)(int argc, char **argv);
	gchar *descr;
} GPIOCliCmd;

int gpiocli_detect_main(int argc, char **argv);
int gpiocli_find_main(int argc, char **argv);
int gpiocli_info_main(int argc, char **argv);
int gpiocli_get_main(int argc, char **argv);
int gpiocli_monitor_main(int argc, char **argv);
int gpiocli_notify_main(int argc, char **argv);
int gpiocli_reconfigure_main(int argc, char **argv);
int gpiocli_release_main(int argc, char **argv);
int gpiocli_request_main(int argc, char **argv);
int gpiocli_requests_main(int argc, char **argv);
int gpiocli_set_main(int argc, char **argv);
int gpiocli_wait_main(int argc, char **argv);

static const GPIOCliCmd cli_cmds[] = {
	{
		.name = "detect",
		.main_func = gpiocli_detect_main,
		.descr = "list GPIO chips and print their properties",
	},
	{
		.name = "find",
		.main_func = gpiocli_find_main,
		.descr = "take a line name and find its parent chip's name and offset within it",
	},
	{
		.name = "info",
		.main_func = gpiocli_info_main,
		.descr = "print information about GPIO lines",
	},
	{
		.name = "get",
		.main_func = gpiocli_get_main,
		.descr = "get values of GPIO lines",
	},
	{
		.name = "monitor",
		.main_func = gpiocli_monitor_main,
		.descr = "notify the user about edge events",
	},
	{
		.name = "notify",
		.main_func = gpiocli_notify_main,
		.descr = "notify the user about line property changes",
	},
	{
		.name = "reconfigure",
		.main_func = gpiocli_reconfigure_main,
		.descr = "change the line configuration for an existing request",
	},
	{
		.name = "release",
		.main_func = gpiocli_release_main,
		.descr = "release one of the line requests controlled by the manager",
	},
	{
		.name = "request",
		.main_func = gpiocli_request_main,
		.descr = "request a set of GPIO lines for exclusive usage by the manager",
	},
	{
		.name = "requests",
		.main_func = gpiocli_requests_main,
		.descr = "list all line requests controlled by the manager",
	},
	{
		.name = "set",
		.main_func = gpiocli_set_main,
		.descr = "set values of GPIO lines",
	},
	{
		.name = "wait",
		.main_func = gpiocli_wait_main,
		.descr = "wait for the gpio-manager interface to appear",
	},
	{ }
};

static GHashTable *make_cmd_table(void)
{
	GHashTable *cmd_table = g_hash_table_new_full(g_str_hash, g_str_equal,
						      NULL, NULL);
	const GPIOCliCmd *cmd;

	for (cmd = &cli_cmds[0]; cmd->name; cmd++)
		g_hash_table_insert(cmd_table, cmd->name, cmd->main_func);

	return cmd_table;
}

static gchar *make_description(void)
{
	g_autoptr(GString) descr = g_string_new("Available commands:\n");
	const GPIOCliCmd *cmd;

	for (cmd = &cli_cmds[0]; cmd->name; cmd++)
		g_string_append_printf(descr, "  %s - %s\n",
				       cmd->name, cmd->descr);

	g_string_truncate(descr, descr->len - 1);
	return g_strdup(descr->str);
}

static void show_version_and_exit(void)
{
	g_print("gpiocli v%s\n", GPIOD_VERSION_STR);

	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	static const gchar *const summary =
"Simple command-line client for controlling gpio-manager.";

	g_autoptr(GHashTable) cmd_table = make_cmd_table();
	g_autofree gchar *description = make_description();
	g_autofree gchar *basename = NULL;
	g_autofree gchar *cmd_name = NULL;
	gint (*cmd_func)(gint, gchar **);
	g_auto(GStrv) cmd_args = NULL;
	gboolean show_version = FALSE;

	const GOptionEntry opts[] = {
		{
			.long_name		= "version",
			.short_name		= 'v',
			.flags			= G_OPTION_FLAG_NONE,
			.arg			= G_OPTION_ARG_NONE,
			.arg_data		= &show_version,
			.description		= "Show version and exit.",
		},
		{
			.long_name		= G_OPTION_REMAINING,
			.flags			= G_OPTION_FLAG_NONE,
			.arg			= G_OPTION_ARG_STRING_ARRAY,
			.arg_data		= &cmd_args,
			.arg_description	= "CMD [ARGS?] ...",
		},
		{ }
	};

	basename = g_path_get_basename(argv[0]);
	g_set_prgname(basename);

	parse_options(opts, summary, description, &argc, &argv);

	if (show_version)
		show_version_and_exit();

	if (!cmd_args)
		die_parsing_opts("Command must be specified.");

	cmd_func = g_hash_table_lookup(cmd_table, cmd_args[0]);
	if (!cmd_func)
		die_parsing_opts("Unknown command: %s.", cmd_args[0]);

	cmd_name = g_strdup_printf("%s %s", basename, cmd_args[0]);
	g_set_prgname(cmd_name);

	return cmd_func(g_strv_length(cmd_args), cmd_args);
}
