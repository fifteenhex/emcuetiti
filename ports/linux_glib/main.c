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

#include <glib.h>

#include "commandline.h"
#include "broker.h"
#include "remote.h"
#include "sys.h"

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

	ret = sys_init(&broker);
	if (ret != 0)
		goto exit;

	g_main_loop_run(mainloop);

	exit: return ret;
}
