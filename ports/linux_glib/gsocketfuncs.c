#include <gio/gio.h>
#include "gsocketfuncs.h"
#include "libmqtt.h"

bool gsocket_readytoread(void* userdata) {
	GSocket* socket = (GSocket*) userdata;
	return g_socket_condition_check(socket, G_IO_IN) == G_IO_IN;
}

int gsocket_read(void* connectiondata, uint8_t* buffer, size_t len) {
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
