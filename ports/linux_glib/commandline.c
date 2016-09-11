#define GETTEXT_PACKAGE "gtk20"
#include <glib.h>

#include "commandline.h"

// broker
static int port;

// remote
static char* remote_host = NULL;
static int remote_port;

static GOptionEntry entries[] = { //
		{ "port", 'p', 0, G_OPTION_ARG_INT, &port, "broker port", "port" }, //
				{ "remotehost", 'H', 0, G_OPTION_ARG_STRING, &remote_host,
						"remote host", "hostname" }, //
				{ "remoteport", 'P', 0, G_OPTION_ARG_INT, &remote_port,
						"remote port", "port" }, //
				{ NULL } };

int commandline_parse(int argc, char** argv) {

	int ret = -1;

	GError *error = NULL;
	GOptionContext *context = g_option_context_new(NULL);

	g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("option parsing failed: %s\n", error->message);
	} else
		ret = 0;

	g_option_context_free(context);

	return ret;
}
