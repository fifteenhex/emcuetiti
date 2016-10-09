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

#include <stdbool.h>
#include <stdarg.h>

#include "emcuetiti_config.h"
#include "libmqtt.h"
#include "buffers_types.h"

typedef EMCUETITI_CONFIG_TIMESTAMPTYPE emcuetiti_timestamp;

// forward typedefs

typedef struct emcuetiti_clienthandle emcuetiti_clienthandle;
typedef struct emcuetiti_clientstate emcuetiti_clientstate;
typedef struct emcuetiti_brokerhandle emcuetiti_brokerhandle;
typedef struct emcuetiti_topichandle emcuetiti_topichandle;

// function prototypes

typedef int (*emcuetiti_publishreadyfunc)(emcuetiti_brokerhandle* broker,
		emcuetiti_topichandle* topic, buffers_buffer* buffer, void* portdata);
typedef int (*emcuetiti_clientconnectedfunc)(emcuetiti_brokerhandle* broker,
		void* portdata);
typedef int (*emcuetiti_clientdisconnectedfunc)(emcuetiti_brokerhandle* broker,
		void* portdata);

typedef bool (*emcuetiti_authenticateclientfunc)(const char* clientid);
typedef bool (*emcuetiti_isconnected)(void* userdata);

typedef bool (*emcuetiti_readytoreadfunc)(void* userdata);

typedef libmqtt_writefunc emcuetiti_writefunc;
typedef libmqtt_readfunc emcuetiti_readfunc;

typedef void (*emcuetiti_freefunc)(void* userdata);
typedef int (*emcuetiti_allocfunc)(void* userdata, size_t size);
typedef void (*emcuetiti_resetfunc)(void* userdata);
typedef void (*emcuetiti_pollfunc)(emcuetiti_timestamp now, void* userdata);

typedef void (*emcuetiti_disconnectfunc)(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* client);

typedef emcuetiti_timestamp (*emcuetiti_timstampfunc)(void);

typedef void (*emcuetiti_logfunc)(const emcuetiti_brokerhandle* broker,
		const char* msg, va_list args);

// shared structures
struct emcuetiti_topichandle {
	const char* topicpart;
	size_t topicpartln;
	emcuetiti_topichandle* child;
	emcuetiti_topichandle* sibling;
	emcuetiti_topichandle* parent;
	bool targetable;
};

// client structures

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

typedef enum {
	CLIENTSTATE_NEW, CLIENTSTATE_CONNECTED, CLIENTSTATE_DISCONNECTED
} emcuetiti_clientconnectionstate;

typedef enum {
	ONLYTHIS, THISANDABOVE
} emcuetiti_subscription_level;

typedef struct {
	emcuetiti_topichandle* topic;
	uint8_t qos;
	emcuetiti_subscription_level level;

} emcuetiti_subscription;

typedef struct {
	emcuetiti_topichandle* topic;

	size_t* buffsz;
	uint8_t* payloadbuff;
} clientregisters_publish;

typedef struct {
	unsigned subscriptionspending;
	emcuetiti_topichandle* pendingtopics[EMCUETITI_CONFIG_MAXSUBSPERCLIENT];
	uint8_t pendingqos[EMCUETITI_CONFIG_MAXSUBSPERCLIENT];
	emcuetiti_subscription_level pendinglevels[EMCUETITI_CONFIG_MAXSUBSPERCLIENT];
} clientregisters_subunsub;

typedef union {
	clientregisters_publish publish;
	clientregisters_subunsub subunsub;
} clientregisters;

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

// port structures

typedef struct {
	const emcuetiti_publishreadyfunc publishreadycallback;
	const emcuetiti_pollfunc pollfunc;
#if EMCUETITI_CONFIG_PORT_CALLBACK_CLIENTCONNECT
	const emcuetiti_clientconnectedfunc connected;
#endif
#if EMCUETITI_CONFIG_PORT_CALLBACK_CLIENTDISCONNECT
const emcuetiti_clientdisconnectedfunc disconnected;
#endif
} emcuetiti_port_callbacks;

typedef struct {
const emcuetiti_port_callbacks* callbacks;
void* portdata;
} emcuetiti_porthandle;

// broker structures
typedef struct {
emcuetiti_topichandle* topic;
emcuetiti_writefunc writefunc;
emcuetiti_readfunc readfunc;
emcuetiti_freefunc freefunc;
emcuetiti_resetfunc resetfunc;
void* userdata;
size_t payloadln;
} emcuetiti_publish;

typedef struct {

emcuetiti_authenticateclientfunc authenticatecallback;
emcuetiti_timstampfunc timestamp;

// optional
emcuetiti_logfunc logx;

// client funcs
emcuetiti_isconnected isconnectedfunc;
libmqtt_writefunc writefunc;
emcuetiti_readytoreadfunc readytoread;
libmqtt_readfunc readfunc;
emcuetiti_disconnectfunc disconnectfunc;
} emcuetiti_brokerhandle_callbacks;

struct emcuetiti_brokerhandle {
unsigned registeredclients;
emcuetiti_topichandle* root;
emcuetiti_porthandle* ports[EMCUETITI_CONFIG_MAXPORTS];
emcuetiti_clientstate clients[EMCUETITI_CONFIG_MAXCLIENTS];
BUFFERS_STATICBUFFERPOOL(inflightpayloads, EMCUETITI_CONFIG_MAXPAYLOADLEN, EMCUETITI_CONFIG_MAXINFLIGHTPAYLOADS);
const emcuetiti_brokerhandle_callbacks* callbacks;
void* userdata;
};
