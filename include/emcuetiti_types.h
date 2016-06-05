#pragma once

#include <stdbool.h>

#include "emcuetiti_config.h"
#include "libmqtt.h"

// forward typedefs

typedef struct emcuetiti_clienthandle emcuetiti_clienthandle;
typedef struct emcuetiti_clientstate emcuetiti_clientstate;

// function prototypes

typedef int (*emcuetiti_publishreadyfunc)(emcuetiti_clienthandle* client,
		size_t payloadlen);
typedef bool (*emcuetiti_authenticateclientfunc)(const char* clientid);
typedef bool (*emcuetiti_isconnected)(void* userdata);
typedef int (*emcuetiti_writefunc)(void* userdata, uint8_t* buffer,
		size_t offset, size_t len);
typedef bool (*emcuetiti_readytoreadfunc)(void* userdata);
typedef int (*emcuetiti_readfunc)(void* userdata, uint8_t* buffer,
		size_t offset, size_t len);
typedef void (*emcuetiti_freefunc)(void* userdata);
typedef int (*emcuetiti_allocfunc)(void* userdata, size_t size);

typedef void (*emcuetiti_disconnectfunc)(emcuetiti_clienthandle* client,
		void* userdata);
typedef EMCUETITI_CONFIG_TIMESTAMPTYPE (*emcuetiti_timstampfunc)(void);

// shared structures
typedef struct emcuetiti_topichandle {
	const char* topicpart;
	struct emucutiti_topichandle* child;
	struct emcuetiti_topichandle* sibling;
	struct emcuetiti_topichandle* parent;
	bool targetable;
} emcuetiti_topichandle;

// client structures
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

struct emcuetiti_clientstate {
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
};

// broker structures
typedef struct {
	emcuetiti_topichandle* topic;
	emcuetiti_writefunc writefunc;
	emcuetiti_readfunc readfunc;
	emcuetiti_freefunc freefunc;
	void* userdata;
} emcuetiti_publish;

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
