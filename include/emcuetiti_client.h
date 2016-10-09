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

#pragma once

#include "emcuetiti_config.h"
#include "emcuetiti_types.h"

typedef enum {
	CLIENTSTATE_NEW, // client has just connected to us
	CLIENTSTATE_CONNECTED, // client has completed MQTT handshake
	CLIENTSTATE_DISCONNECTED
} emcuetiti_clientconnectionstate;

#if EMCUETITI_CONFIG_CLIENTCALLBACKS
typedef struct {
#if EMCUETITI_CONFIG_CLIENTCALLBACKS_ISCONNECTED
	emcuetiti_isconnected isconnectedfunc;
#endif
#if EMCUETITI_CONFIG_CLIENTCALLBACKS_WRITE
	libmqtt_writefunc writefunc; // function pointer to the function used to write data to the client
#endif
#if EMCUETITI_CONFIG_PERCLIENTCALLBACKS_READYTOREAD
	emcuetiti_readytoreadfunc readytoread;
#endif
#if EMCUETITI_CONFIG_CLIENTCALLBACKS_READ
	emcuetiti_readfunc readfunc; // function pointer to the function user to read data from the client
#endif
#if EMCUETITI_CONFIG_CLIENTCALLBACKS_DISCONNECT
	emcuetiti_disconnectfunc disconnectfunc; //
#endif
}emcuetiti_clientops;
#endif

struct emcuetiti_clienthandle {
#if EMCUETITI_CONFIG_CLIENTCALLBACKS
	const emcuetiti_clientops* ops;
#endif
	void* userdata; // use this to stash whatever is needed to write/read the right client
// in the write/read functions
};

struct emcuetiti_clientstate {
	emcuetiti_clienthandle* client;
	char clientid[LIBMQTT_CLIENTID_MAXLENGTH + 1];

	uint16_t keepalive;
	EMCUETITI_CONFIG_TIMESTAMPTYPE lastseen;

	unsigned numsubscriptions;
	emcuetiti_subscription subscriptions[EMCUETITI_CONFIG_MAXSUBSPERCLIENT];

	BUFFERS_STATICBUFFER(buffer, EMCUETITI_CONFIG_CLIENTBUFFERSZ + 1);

	emcuetiti_clientconnectionstate state;

	libmqtt_packetread incomingpacket;

	uint8_t packettype;
	size_t varheaderandpayloadlen;
	size_t remainingbytes;

	emcuetiti_topichandle* publishtopic;
	size_t publishpayloadlen;

	clientregisters registers;
	emcuetiti_brokerhandle* broker;
};

int emcuetiti_client_register(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* handle);
void emcuetiti_client_unregister(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* handle);
void emcuetiti_client_poll(emcuetiti_brokerhandle* broker,
		emcuetiti_timestamp now);
libmqtt_writefunc emcuetiti_client_resolvewritefunc(
		const emcuetiti_brokerhandle* broker, const emcuetiti_clientstate* cs);

void emcuetiti_client_dumpstate(emcuetiti_brokerhandle* broker,
		emcuetiti_clientstate* client);
