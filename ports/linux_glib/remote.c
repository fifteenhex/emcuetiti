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

#include <gio/gio.h>
#include <emcuetiti_port_remote.h>

#include "gsocketfuncs.h"
#include "commandline.h"
#include "remote.h"
#include "broker.h"

static gboolean remote_socketcallback(GSocket *socket, GIOCondition condition,
		gpointer data) {

}

static void remote_wireupsocketcallback(GSocket *socket) {
	//GSource* clientsocketsource = g_socket_create_source(socket, G_IO_IN,
	//NULL);
	//g_source_attach(clientsocketsource, NULL);
	//g_source_set_callback(clientsocketsource, , &broker,
	//NULL);
}

static int remote_connect(const char* host, unsigned port,
		void** connectiondata) {
	g_message("connecting to remote: %s on port %u", host, port);

	GSocket* sock = g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_STREAM,
			G_SOCKET_PROTOCOL_TCP, NULL);

	GSocketConnectable* addr = g_network_address_new(host, port);
	GSocketAddressEnumerator* enumerator = g_socket_connectable_enumerate(addr);

	int ret = EMCUETITI_PORT_REMOTE_TRYAGAIN;

	GSocketAddress* sockaddr;
	while ((sockaddr = g_socket_address_enumerator_next(enumerator, NULL, NULL))) {
		g_message("connecting...");
		GError* error = NULL;
		if (g_socket_connect(sock, sockaddr, NULL, &error)) {
			g_message("connected");
			g_socket_set_blocking(sock, FALSE);
			remote_wireupsocketcallback(sock);
			*connectiondata = sock;
			ret = EMCUETITI_PORT_REMOTE_OK;
			break;
		} else {
			g_message("failed to connect %d:%s", error->code, error->message);
		}
	}

	if (ret != EMCUETITI_PORT_REMOTE_OK)
		g_object_unref(sock);

	g_object_unref(addr);
	g_object_unref(enumerator);
	return ret;
}

static int remote_disconnect(void* connectiondata) {
	g_message("disconnecting remote socket");
	GSocket* sock = (GSocket*) connectiondata;
	g_socket_close(sock, NULL);
	g_object_unref(sock);
	return 0;
}

static const emcuetiti_port_remote_hostops remotehostops = { //
		.connect = remote_connect, //
				.disconnect = remote_disconnect, //
				.datawaiting = gsocket_readytoread, //
				.read = gsocket_read, //
				.write = gsocket_write };

static emcuetiti_port_remoteconfig remoteconfig = { //
		.clientid = "remoteclient", //
				.hostops = &remotehostops };

static emcuetiti_porthandle remoteport;
static emcuetiti_port_remote_portdata remoteportdata;

int remote_init(emcuetiti_brokerhandle* broker) {
	if (commandline_remote_host != NULL) {
		remoteconfig.host = commandline_remote_host;
		remoteconfig.port = commandline_remote_port;
		remoteconfig.keepalive = commandline_remote_keepalive;

		remoteconfig.numtopics = 0;
		if (commandline_remote_topics != NULL) {
			for (gchar** t = commandline_remote_topics; *t != NULL; t++)
				remoteconfig.numtopics++;
			remoteconfig.topics = g_malloc(
					sizeof(libmqtt_subscription) * remoteconfig.numtopics);

			for (int i = 0; i < remoteconfig.numtopics; i++) {
				libmqtt_subscription* sub = &remoteconfig.topics[i];
				sub->topic = commandline_remote_topics[i];
				sub->qos = LIBMQTT_QOS0_ATMOSTONCE;
			}
		}

		emcuetiti_port_remote_new(broker, &remoteconfig, &remoteport,
				&remoteportdata);
	}
	return 0;
}
