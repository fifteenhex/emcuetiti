#include <glib.h>

#include "commandline.h"
#include "broker.h"
#include "remote.h"

static emcuetiti_brokerhandle broker;

int main(int argc, char** argv) {

	GMainLoop* mainloop = g_main_loop_new(NULL, FALSE);

	int ret = commandline_parse(argc, argv);
	if (ret != 0)
		goto exit;

	ret = broker_init(mainloop, &broker);
	if (ret != 0)
		goto exit;

	ret = remote_init(&broker);
	if (ret != 0)
		goto exit;

	g_main_loop_run(mainloop);

	exit: return ret;
}
