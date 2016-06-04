#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "libmqtt.h"
#include "emcuetiti_config.h"

typedef struct emcuetiti_clienthandle emcuetiti_clienthandle;

typedef int (*emcuetiti_writefunc)(void* userdata, uint8_t* buffer,
		size_t offset, size_t len);
typedef bool (*emcuetiti_readytoreadfunc)(void* userdata);
typedef int (*emcuetiti_readfunc)(void* userdata, uint8_t* buffer,
		size_t offset, size_t len);
typedef void (*emcuetiti_freefunc)(void* userdata);
typedef int (*emcuetiti_allocfunc)(void* userdata, size_t size);

typedef void (*emcuetiti_disconnectfunc)(emcuetiti_clienthandle* client,
		void* userdata);

typedef bool (*emcuetiti_isconnected)(void* userdata);

typedef struct {
	emcuetiti_isconnected isconnectedfunc;
#if EMCUETITI_CONFIG_CLIENTCALLBACKS_WRITE
	libmqtt_writefunc writefunc; // function pointer to the function used to write data to the client
#endif
	emcuetiti_readytoreadfunc readytoread;
	emcuetiti_readfunc readfunc; // function pointer to the function user to read data from the client
#if EMCUETITIT_CONFIG_CLIENTCALLBACKS_DISCONNECT
	emcuetiti_disconnectfunc disconnectfunc; //
#endif
} emcuetiti_clientops;

struct emcuetiti_clienthandle {
	emcuetiti_clientops* ops;
	void* userdata; // use this to stash whatever is needed to write/read the right client
// in the write/read functions
};

typedef enum {
	CLIENTSTATE_NEW, CLIENTSTATE_CONNECTED, CLIENTSTATE_DISCONNECTED
} emcuetiti_clientconnectionstate;

typedef enum {
	CLIENTREADSTATE_IDLE,
	CLIENTREADSTATE_TYPE,
	CLIENTREADSTATE_REMAININGLEN,
	CLIENTREADSTATE_PAYLOAD,
	CLIENTREADSTATE_COMPLETE,

	CLIENTREADSTATE_PUBLISHREADY
} emcuetiti_clientreadstate;

typedef struct emcuetiti_topichandle {
	const char* topicpart;
	struct emucutiti_topichandle* child;
	struct emcuetiti_topichandle* sibling;
	struct emcuetiti_topichandle* parent;
	bool targetable;
} emcuetiti_topichandle;

typedef struct {
	emcuetiti_clienthandle* client;
	char clientid[LIBMQTT_CLIENTID_MAXLENGTH + 1];
	uint16_t keepalive;
	EMCUETITI_CONFIG_TIMESTAMPTYPE lastseen;

	unsigned numsubscriptions;
	emcuetiti_topichandle* subscriptions[EMCUETITI_CONFIG_MAXSUBSPERCLIENT];

	uint8_t buffer[EMCUETITI_CONFIG_CLIENTBUFFERSZ];
	unsigned bufferpos;

	emcuetiti_clientconnectionstate state;
	emcuetiti_clientreadstate readstate;
	uint8_t packettype;
	size_t varheaderandpayloadlen;
	size_t remainingbytes;

	emcuetiti_topichandle* publishtopic;
	size_t publishpayloadlen;
} emcuetiti_clientstate;

typedef int (*emcuetiti_publishreadyfunc)(emcuetiti_clienthandle* client,
		size_t payloadlen);

typedef bool (*emcuetiti_authenticateclientfunc)(const char* clientid);

typedef EMCUETITI_CONFIG_TIMESTAMPTYPE (*emcuetiti_timstampfunc)(void);

typedef struct {
	emcuetiti_publishreadyfunc publishreadycallback;
	emcuetiti_authenticateclientfunc authenticatecallback;
	emcuetiti_timstampfunc timestamp;

	// client funcs
	libmqtt_writefunc writefunc;
	emcuetiti_disconnectfunc disconnectfunc;
} emcuetiti_brokerhandle_callbacks;

typedef struct {
	unsigned registeredclients;
	emcuetiti_topichandle* root;
	emcuetiti_clientstate clients[EMCUETITI_CONFIG_MAXCLIENTS];
	emcuetiti_brokerhandle_callbacks* callbacks;
} emcuetiti_brokerhandle;

typedef struct {
	emcuetiti_topichandle* topic;
	emcuetiti_writefunc writefunc;
	emcuetiti_readfunc readfunc;
	emcuetiti_freefunc freefunc;
	void* userdata;
} emcuetiti_publish;

// These functions are to be driven by the code running on the broker
// to publish to clients
void emcuetiti_broker_publish(emcuetiti_brokerhandle* broker,
		emcuetiti_publish* publish);

//
void emcuetiti_client_register(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* handle);
void emcuetiti_client_unregister(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* handle);
int emcuetiti_client_readpublish(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* client, uint8_t* buffer, size_t len);

//
void emcuetiti_poll(emcuetiti_brokerhandle* broker);
void emcuetiti_addtopicpart(emcuetiti_brokerhandle* broker,
		emcuetiti_topichandle* root, emcuetiti_topichandle* part,
		const char* topicpart, bool targetable);
void emcuetiti_init(emcuetiti_brokerhandle* broker);
void emcuetiti_dumpstate(emcuetiti_brokerhandle* broker);
