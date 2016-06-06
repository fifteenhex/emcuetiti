#include <gio/gio.h>
#include <stdio.h>
#include <stdbool.h>

#include "emcuetiti.h"
#include "emcuetiti_port_router.h"

emcuetiti_brokerhandle broker;

/*
 typedef int (*emcuetiti_writefunc)(void* userdata, uint8_t* buffer,
 size_t offset, size_t len);
 typedef bool (*emcuetiti_readytoreadfunc)(void* userdata);
 typedef int (*emcuetiti_readfunc)(void* userdata, uint8_t* buffer,
 size_t offset, size_t len);
 typedef void (*emcuetiti_freefunc)(void* userdata);
 typedef int (*emcuetiti_allocfunc)(void* userdata, size_t size);
 typedef void (*emcuetiti_disconnectfunc)(void* userdata);*/

bool readytoread(void* userdata) {
	GSocket* socket = (GSocket*) userdata;
	return g_socket_condition_check(socket, G_IO_IN) == G_IO_IN;
}

int gsocket_read(void* userdata, uint8_t* buffer, size_t len) {
	GSocket* socket = (GSocket*) userdata;
	int ret = g_socket_receive(socket, buffer, len, NULL, NULL);
	if (ret != len)
		printf("wanted %d, read %d\n", len, ret);
	return ret;

}

int gsocket_write(void* userdata, const uint8_t* buffer, size_t len) {
	GSocket* socket = (GSocket*) userdata;
	int ret = g_socket_send(socket, buffer, len, NULL, NULL);
	return ret;
}

static bool gsocket_isconnected(void* userdata) {
	GSocket* socket = (GSocket*) userdata;
	return g_socket_is_connected(socket);
}

void gsocket_disconnect(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* client) {
	emcuetiti_client_unregister(broker, client);
	GSocket* socket = (GSocket*) client->userdata;
	g_socket_close(socket, NULL);
	g_free(client);
}

static emcuetiti_clientops gsocketclientops = { //
		.isconnectedfunc = gsocket_isconnected, //
				.readytoread = readytoread, //
				.readfunc = gsocket_read };

static uint32_t timestamp() {
	gint64 now = g_get_monotonic_time() / 1000000;
	return (uint32_t) now;
}

static emcuetiti_brokerhandle_callbacks brokerops = { //
		.authenticatecallback = NULL, //
				.timestamp = timestamp, //
				.writefunc = gsocket_write, //
				.disconnectfunc = gsocket_disconnect };

int main(int argc, char** argv) {

	GSocketAddress* serversockaddr = g_inet_socket_address_new_from_string(
			"127.0.0.1", 8991);

	GSocket* serversocket = g_socket_new(G_SOCKET_FAMILY_IPV4,
			G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_DEFAULT, NULL);

	if (serversocket == NULL)
		return 1;

	if (!g_socket_bind(serversocket, serversockaddr, FALSE, NULL))
		return 1;

	g_socket_set_blocking(serversocket, FALSE);

	if (g_socket_listen(serversocket, NULL)) {

		broker.callbacks = &brokerops;

		emcuetiti_topichandle topic1, subtopic1;

		emcuetiti_topichandle topic2, topic2subtopic1, topic2subtopic2,
				topic2subtopic2subtopic1;

		emcuetiti_broker_init(&broker);

		emcuetiti_broker_addtopicpart(&broker, NULL, &topic1, "topic1", true);
		emcuetiti_broker_addtopicpart(&broker, &topic1, &subtopic1, "subtopic1",
		true);

		emcuetiti_broker_addtopicpart(&broker, NULL, &topic2, "topic2", true);
		emcuetiti_broker_addtopicpart(&broker, &topic2, &topic2subtopic1,
				"topic1subtopic2", true);
		emcuetiti_broker_addtopicpart(&broker, &topic2, &topic2subtopic2,
				"topic2subtopic2", true);
		emcuetiti_broker_addtopicpart(&broker, &topic2subtopic2,
				&topic2subtopic2subtopic1, "subtopic2subtopic1", true);

		emcuetiti_broker_dumpstate(&broker);

		emcuetiti_porthandle routerport;
		emcuetiti_port_router(&broker, &routerport);

		while (true) {
			//static int loop = 0;
			GSocket* clientsocket = g_socket_accept(serversocket, NULL, NULL);
			if (clientsocket != NULL) {
				if (emcuetiti_broker_canacceptmoreclients(&broker)) {
					g_message("incoming connection");
					emcuetiti_clienthandle* client = g_malloc(
							sizeof(emcuetiti_clienthandle));

					client->userdata = clientsocket;
					client->ops = &gsocketclientops;
					emcuetiti_client_register(&broker, client);
				} else {
					g_message(
							"cannot accept any more clients, disconnecting client");
					g_socket_close(clientsocket, NULL);
				}
			}

			emcuetiti_broker_poll(&broker);
			//loop++;
			//if ((loop % 100) == 0)
			//	emcuetiti_broker_dumpstate(&broker);
			g_usleep(5000);
		}
		g_socket_close(serversocket, NULL);
	} else
		g_message("failed to create server socket");

	return 0;
}
