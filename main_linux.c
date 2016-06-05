#include <gio/gio.h>
#include <stdio.h>
#include <stdbool.h>

#include "emcuetiti.h"
#include "emcuetiti_port_router.h"

emcuetiti_brokerhandle broker;
static int publishreadycallback(emcuetiti_clienthandle* client,
		size_t publishlen) {
	printf("publish ready\n");
	uint8_t* buffer = g_malloc(publishlen + 1);
	int read = emcuetiti_client_readpublish(&broker, client, buffer,
			publishlen);
	buffer[read] = '\0';
	printf("publish %s\n", (char *) buffer);
	g_free(buffer);
	return 0;
}

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

int gsocket_read(void* userdata, uint8_t* buffer, size_t offset, size_t len) {
	GSocket* socket = (GSocket*) userdata;
	int ret = g_socket_receive(socket, buffer, len, NULL, NULL);
	return ret;

}

int gsocket_write(void* userdata, uint8_t* buffer, size_t len) {
	GSocket* socket = (GSocket*) userdata;
	int ret = g_socket_send(socket, buffer, len, NULL, NULL);
	return ret;
}

static bool gsocket_isconnected(void* userdata) {
	GSocket* socket = (GSocket*) userdata;
	return g_socket_is_connected(socket);
}

void gsocket_disconnect(emcuetiti_clienthandle* client, void* userdata) {
	GSocket* socket = (GSocket*) userdata;
	g_socket_close(socket, NULL);
}

static emcuetiti_clientops gsocketclientops = { //
		.isconnectedfunc = gsocket_isconnected, //
				.readytoread = readytoread, //
				.readfunc = gsocket_read };

static uint32_t timestamp() {
	return 0;
}

static emcuetiti_brokerhandle_callbacks brokerops = { //
		.authenticatecallback = NULL, //
				.publishreadycallback = publishreadycallback, //
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

		emcuetiti_init(&broker);

		emcuetiti_addtopicpart(&broker, NULL, &topic1, "topic1", true);
		emcuetiti_addtopicpart(&broker, &topic1, &subtopic1, "subtopic1", true);

		emcuetiti_addtopicpart(&broker, NULL, &topic2, "topic2", true);
		emcuetiti_addtopicpart(&broker, &topic2, &topic2subtopic1,
				"topic1subtopic2", true);
		emcuetiti_addtopicpart(&broker, &topic2, &topic2subtopic2,
				"topic2subtopic2", true);
		emcuetiti_addtopicpart(&broker, &topic2subtopic2,
				&topic2subtopic2subtopic1, "subtopic2subtopic1", true);

		emcuetiti_dumpstate(&broker);

		emcuetiti_client_router(&broker);

		while (true) {
			GSocket* clientsocket = g_socket_accept(serversocket, NULL, NULL);
			if (clientsocket != NULL) {
				g_message("incoming connection");
				emcuetiti_clienthandle* client = g_malloc(
						sizeof(emcuetiti_clienthandle));

				client->userdata = clientsocket;
				client->ops = &gsocketclientops;
				emcuetiti_client_register(&broker, client);

			}

			emcuetiti_poll(&broker);
		}
		g_socket_close(serversocket, NULL);
	} else
		g_message("failed to create server socket");

	return 0;
}
