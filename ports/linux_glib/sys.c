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
#include <json-glib/json-glib.h>

#include "sys.h"
#include "commandline.h"
#include "emcuetiti_port.h"
#include "emcuetiti_port_sys.h"

#define JSONFIELD_CLIENTID "clientid"
#define JSONFIELD_KEEPALIVE "keepalive"

static void* sys_encode_client_connect(
		emcuetiti_port_sys_connectdata* connectdata, size_t* len) {
	JsonBuilder* builder = json_builder_new();

	json_builder_begin_object(builder);

	json_builder_set_member_name(builder, JSONFIELD_CLIENTID);
	json_builder_add_string_value(builder, connectdata->clientid);

	json_builder_set_member_name(builder, JSONFIELD_KEEPALIVE);
	json_builder_add_int_value(builder, connectdata->keepalive);

	json_builder_end_object(builder);

	JsonGenerator* gen = json_generator_new();
	JsonNode* root = json_builder_get_root(builder);
	json_generator_set_root(gen, root);
	gchar* str = json_generator_to_data(gen, len);

	json_node_free(root);
	g_object_unref(gen);
	g_object_unref(builder);

	g_message(str);

	return str;
}

static void* sys_encode_client_disconnect(
		emcuetiti_port_sys_disconnectdata* disconnectdata, size_t* len) {
	return NULL;
}

static void sys_encode_client_free(void* buffer) {
	g_free(buffer);
}

static emcuetiti_port_sys_ops sys_ops = { //
		.encode_client_connect = sys_encode_client_connect, //
				.encode_client_disconnect = sys_encode_client_disconnect, //
				.free = sys_encode_client_free };

int sys_init(emcuetiti_brokerhandle* broker) {
	int ret = 0;

	if (commandline_sys) {
		emcuetiti_porthandle* porthandle = g_malloc(
				sizeof(emcuetiti_porthandle));
		emcuetiti_port_sys_data* portdata = g_malloc(
				sizeof(emcuetiti_port_sys_data));

		emcuetiti_port_sys_new(broker, porthandle, portdata, &sys_ops);
	}

	return ret;
}
