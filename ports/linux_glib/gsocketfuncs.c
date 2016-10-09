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
