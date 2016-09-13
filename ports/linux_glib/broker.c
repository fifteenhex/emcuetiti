#include <gio/gio.h>
#include <stdio.h>
#include <stdbool.h>

#include "broker.h"
#include "commandline.h"
#include "emcuetiti_topic.h"
#include "emcuetiti_port_router.h"
#include "emcuetiti_port_remote.h"

typedef struct {
	GSocket* serversocket;
	emcuetiti_porthandle routerport;
	emcuetiti_topichandle topic1, subtopic1, topic2, topic2subtopic1,
			topic2subtopic2, topic2subtopic2subtopic1;
} brokerdata;

static brokerdata data;

static bool readytoread(void* userdata) {
	GSocket* socket = (GSocket*) userdata;
	return g_socket_condition_check(socket, G_IO_IN) == G_IO_IN;
}

static int gsocket_read(void* userdata, uint8_t* buffer, size_t len) {
	GSocket* socket = (GSocket*) userdata;
	int ret = g_socket_receive(socket, buffer, len, NULL, NULL);
	if (ret != len)
		printf("wanted %d, read %d\n", len, ret);
	return ret;

}

static int gsocket_write(void* userdata, const uint8_t* buffer, size_t len) {
	GSocket* socket = (GSocket*) userdata;
	int ret = g_socket_send(socket, buffer, len, NULL, NULL);
	return ret;
}

static bool gsocket_isconnected(void* userdata) {
	GSocket* socket = (GSocket*) userdata;
	return g_socket_is_connected(socket);
}

static void gsocket_disconnect(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* client) {
	emcuetiti_client_unregister(broker, client);
	GSocket* socket = (GSocket*) client->userdata;
	g_socket_close(socket, NULL);
	g_free(client);
}

static const emcuetiti_clientops gsocketclientops = { //
		.isconnectedfunc = gsocket_isconnected, //
				.readytoread = readytoread, //
				.readfunc = gsocket_read };

static uint32_t broker_timestamp() {
	uint32_t now = (uint32_t) (g_get_monotonic_time() / 1000000);
	return now;
}

static const emcuetiti_brokerhandle_callbacks brokerops = { //
		.authenticatecallback = NULL, //
				.timestamp = broker_timestamp, //
				.writefunc = gsocket_write, //
				.disconnectfunc = gsocket_disconnect };

static gboolean brokerpoll(gpointer data) {
	emcuetiti_brokerhandle* broker = (emcuetiti_brokerhandle*) data;
	emcuetiti_broker_poll(broker);
	return TRUE;
}

static gboolean serversocket_callback(GSocket *socket, GIOCondition condition,
		gpointer data) {

	emcuetiti_brokerhandle* broker = (emcuetiti_brokerhandle*) data;
	brokerdata* bdata = (brokerdata*) broker->userdata;

	GSocket* clientsocket = g_socket_accept(bdata->serversocket, NULL,
	NULL);
	if (clientsocket != NULL) {

		if (emcuetiti_broker_canacceptmoreclients(broker)) {
			g_message("incoming connection");
			emcuetiti_clienthandle* client = g_malloc(
					sizeof(emcuetiti_clienthandle));

			client->userdata = clientsocket;
			client->ops = &gsocketclientops;
			emcuetiti_client_register(broker, client);

			/*
			 GSource* clientsocketsource = g_socket_create_source(clientsocket,
			 G_IO_IN, NULL);
			 g_source_attach(clientsocketsource, NULL);
			 g_source_set_callback(clientsocketsource, brokerpoll, &broker,
			 NULL);*/

		} else {
			g_message("cannot accept any more clients, disconnecting client");
			g_socket_close(clientsocket, NULL);
		}
	}

	return TRUE;
}

static void broker_init_setup(emcuetiti_brokerhandle* broker) {

	broker->userdata = &data;
	broker->callbacks = &brokerops;

	emcuetiti_broker_init(broker);

	for (gchar** topic = commandline_topics; *topic != NULL; topic++) {
		gchar** topicparts = g_strsplit(*topic, "/", -1);
		emcuetiti_topichandle* l = NULL;
		for (gchar** topicpart = topicparts; *topicpart != NULL; topicpart++) {
			bool last = *(topicpart + 1) == NULL;
			emcuetiti_topichandle* t = emcuetiti_findtopic(broker, l,
					*topicpart);
			if (t == NULL) {
				t = g_malloc(sizeof(emcuetiti_topichandle));
				gchar* topicpartcopy = g_strdup(*topicpart);
				emcuetiti_broker_addtopicpart(broker, l, t, topicpartcopy,
						last);
			}
			l = t;
		}
		g_strfreev(topicparts);
	}

	emcuetiti_port_router(broker, &data.routerport);

	emcuetiti_broker_dumpstate(broker);
}

int broker_init(GMainLoop* mainloop, emcuetiti_brokerhandle* broker) {

	int ret = -1;
	broker_init_setup(broker);

	GSocketAddress* serversockaddr = g_inet_socket_address_new_from_string(
			"127.0.0.1", 8991);

	data.serversocket = g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_STREAM,
			G_SOCKET_PROTOCOL_DEFAULT, NULL);

	if (data.serversocket == NULL)
		goto exit;

	if (!g_socket_bind(data.serversocket, serversockaddr, FALSE, NULL))
		goto exit;

	g_socket_set_blocking(data.serversocket, FALSE);
	if (g_socket_listen(data.serversocket, NULL)) {
		GSource* serversocketsource = g_socket_create_source(data.serversocket,
				G_IO_IN, NULL);
		g_source_attach(serversocketsource, NULL);
		g_source_set_callback(serversocketsource, serversocket_callback, broker,
		NULL);
		g_timeout_add(2, brokerpoll, broker);

		ret = 0;
	} else
		g_message("failed to create server socket");

	exit: return ret;
}

void broker_cleanup(emcuetiti_brokerhandle* broker) {
	//g_socket_close(serversocket, NULL);
}
