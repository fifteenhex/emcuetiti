#include <gio/gio.h>
#include <stdio.h>
#include <stdbool.h>

#include "emcuetiti.h"
#include "emcuetiti_port_router.h"
#include "emcuetiti_port_remote.h"

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

gboolean brokerpoll(gpointer data) {
	emcuetiti_brokerhandle* broker = (emcuetiti_brokerhandle*) data;
	emcuetiti_broker_poll(broker);
	return TRUE;
}

gboolean serversocket_callback(gpointer data) {
	GSocket* serversocket = (GSocket*) data;
	GSocket* clientsocket = g_socket_accept(serversocket, NULL, NULL);
	if (clientsocket != NULL) {

		if (emcuetiti_broker_canacceptmoreclients(&broker)) {
			g_message("incoming connection");
			emcuetiti_clienthandle* client = g_malloc(
					sizeof(emcuetiti_clienthandle));

			client->userdata = clientsocket;
			client->ops = &gsocketclientops;
			emcuetiti_client_register(&broker, client);

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

int remote_connect(const char* host, unsigned port, void** connectiondata) {
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
			*connectiondata = sock;
			ret = EMCUETITI_PORT_REMOTE_OK;
			break;
		} else {
			g_message("failed to connect %d:%s\n", error->code, error->message);
		}
	}

	g_object_unref(addr);
	g_object_unref(enumerator);
	return ret;
}

int remote_disconnect(void* connectiondata) {
	return 0;
}

int remote_read(void* connectiondata, uint8_t* buffer, size_t len) {
	GSocket* sock = (GSocket*) connectiondata;
	int ret = g_socket_receive(sock, buffer, len, NULL, NULL);

	if (ret == G_IO_ERROR_WOULD_BLOCK)
		ret = LIBMQTT_EWOULDBLOCK;

	return ret;
}

int remote_write(void* connectiondata, const uint8_t* buffer, size_t len) {
	GSocket* sock = (GSocket*) connectiondata;
	return g_socket_send(sock, buffer, len, NULL, NULL);
}

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

		emcuetiti_broker_addtopicpart(&broker, NULL, &topic1, "topic1",
		true);
		emcuetiti_broker_addtopicpart(&broker, &topic1, &subtopic1, "subtopic1",
		true);

		emcuetiti_broker_addtopicpart(&broker, NULL, &topic2, "topic2",
		true);
		emcuetiti_broker_addtopicpart(&broker, &topic2, &topic2subtopic1,
				"topic1subtopic2", true);
		emcuetiti_broker_addtopicpart(&broker, &topic2, &topic2subtopic2,
				"topic2subtopic2", true);
		emcuetiti_broker_addtopicpart(&broker, &topic2subtopic2,
				&topic2subtopic2subtopic1, "subtopic2subtopic1", true);

		emcuetiti_broker_dumpstate(&broker);

		emcuetiti_porthandle routerport;
		emcuetiti_port_router(&broker, &routerport);

		emcuetiti_port_remote_hostops remotehostops = { //
				.connect = remote_connect, //
						.disconnect = remote_disconnect, //
						.read = remote_read, //
						.write = remote_write };
		emcuetiti_port_remoteconfig remoteconfig = { //
				.host = "localhost", //
						.port = 8992, //
						.clientid = "remoteclient", //
						.keepalive = 10, //
						.hostops = &remotehostops };
		emcuetiti_porthandle remoteport;
		emcuetiti_port_remote_portdata remoteportdata;

		emcuetiti_port_remote_new(&broker, &remoteconfig, &remoteport,
				&remoteportdata);

		GSource* serversocketsource = g_socket_create_source(serversocket,
				G_IO_IN, NULL);

		g_source_attach(serversocketsource, NULL);
		g_source_set_callback(serversocketsource, serversocket_callback,
				serversocket, NULL);

		g_timeout_add(10, brokerpoll, &broker);

		GMainLoop* mainloop = g_main_loop_new(NULL, FALSE);
		g_main_loop_run(mainloop);

		g_socket_close(serversocket, NULL);

	} else
		g_message("failed to create server socket");

	return 0;
}
