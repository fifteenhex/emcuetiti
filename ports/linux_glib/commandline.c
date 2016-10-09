/*	This file is part of emcuetiti.
 *
 * emcuetiti is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * emcuetiti is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with emcuetiti.  If not, see <http://www.gnu.org/licenses/>.
 */

#define GETTEXT_PACKAGE "gtk20"
#include <glib.h>

#include "commandline.h"

// broker
int commandline_port;
gchar** commandline_topics;
gboolean commandline_sys;

// remote
char* commandline_remote_host = NULL;
int commandline_remote_port = 1883;
gchar** commandline_remote_topics = NULL;
int commandline_remote_keepalive = 0;

static const GOptionEntry entries[] = { //
		{ "port", 'p', 0, G_OPTION_ARG_INT, &commandline_port, "broker port",
				"port" }, //
				{ "topic", 't', 0, G_OPTION_ARG_STRING_ARRAY,
						&commandline_topics, "topic", "topic" }, //
				{ "sys", 's', 0, G_OPTION_ARG_INT, &commandline_sys, "sys",
						"sys" }, //
				NULL };

static const GOptionEntry remote_entries[] = { //
		{ "remotehost", 'H', 0, G_OPTION_ARG_STRING, &commandline_remote_host,
				"remote host", "hostname" }, //
				{ "remoteport", 'P', 0, G_OPTION_ARG_INT,
						&commandline_remote_port, "remote port", "port" }, //
				{ "remotekeepalive", 'K', 0, G_OPTION_ARG_INT,
						&commandline_remote_keepalive, "remote keepalive",
						"seconds" }, //
				{ "remotetopic", 'T', 0, G_OPTION_ARG_STRING_ARRAY,
						&commandline_remote_topics, "remote topic", "topic" }, //
				{ NULL } };

int commandline_parse(int argc, char** argv) {

	int ret = -1;

	GError *error = NULL;
	GOptionContext *context = g_option_context_new(NULL);

	g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);

	GOptionGroup* remotegroup = g_option_group_new("remote",
			"remote broker options", "remote broker options", NULL, NULL);
	g_option_group_add_entries(remotegroup, remote_entries);
	g_option_context_add_group(context, remotegroup);

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("option parsing failed: %s\n", error->message);
	} else
		ret = 0;

	g_option_context_free(context);

	return ret;
}
