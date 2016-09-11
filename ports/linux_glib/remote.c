#include <gio/gio.h>
#include <emcuetiti_port_remote.h>

#include "remote.h"

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
			*connectiondata = sock;
			ret = EMCUETITI_PORT_REMOTE_OK;
			break;
		} else {
			g_message("failed to connect %d:%s\n", error->code, error->message);
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

static int remote_read(void* connectiondata, uint8_t* buffer, size_t len) {
	GError* error = NULL;
	GSocket* sock = (GSocket*) connectiondata;
	int ret = g_socket_receive(sock, buffer, len, NULL, &error);

	if (ret == 0)
		ret = LIBMQTT_EREMOTEDISCONNECTED;
	else if (error != NULL) {
		if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK))
			ret = LIBMQTT_EWOULDBLOCK;
		else
			ret = LIBMQTT_EFATAL;
		g_error_free(error);
	}

	return ret;
}

static int remote_write(void* connectiondata, const uint8_t* buffer, size_t len) {
	GSocket* sock = (GSocket*) connectiondata;
	return g_socket_send(sock, buffer, len, NULL, NULL);
}

libmqtt_subscription remotesubs[] = { { .topic = "/remote", .qos =
		LIBMQTT_QOS0_ATMOSTONCE } };

static const emcuetiti_port_remote_hostops remotehostops = { //
		.connect = remote_connect, //
				.disconnect = remote_disconnect, //
				.read = remote_read, //
				.write = remote_write };

static emcuetiti_port_remoteconfig remoteconfig = { //
		.host = "localhost", //
				.port = 8992, //
				.clientid = "remoteclient", //
				.keepalive = 10, //
				.hostops = &remotehostops, //
				.topics = remotesubs, .numtopics = 1 };
static emcuetiti_porthandle remoteport;
static emcuetiti_port_remote_portdata remoteportdata;

int remote_init(emcuetiti_brokerhandle* broker) {
	emcuetiti_port_remote_new(broker, &remoteconfig, &remoteport,
			&remoteportdata);
	return 0;
}
