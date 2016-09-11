#pragma once

#include <stdbool.h>

#include "emcuetiti_config.h"
#include "libmqtt.h"

typedef EMCUETITI_CONFIG_TIMESTAMPTYPE emcuetiti_timestamp;

// forward typedefs

typedef struct emcuetiti_clienthandle emcuetiti_clienthandle;
typedef struct emcuetiti_clientstate emcuetiti_clientstate;
typedef struct emcuetiti_brokerhandle emcuetiti_brokerhandle;
typedef struct emcuetiti_topichandle emcuetiti_topichandle;

// function prototypes

typedef int (*emcuetiti_publishreadyfunc)(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* client, emcuetiti_topichandle* topic,
		size_t payloadlen);
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

// shared structures
struct emcuetiti_topichandle {
	const char* topicpart;
	size_t topicpartln;
	struct emucutiti_topichandle* child;
	struct emcuetiti_topichandle* sibling;
	struct emcuetiti_topichandle* parent;
	bool targetable;
};

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
	const emcuetiti_clientops* ops;
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

typedef enum {
	ONLYTHIS, THISANDABOVE
} emcuetiti_subscription_level;

typedef struct {
	emcuetiti_topichandle* topic;
	uint8_t qos;
	emcuetiti_subscription_level level;

} emcuetiti_subscription;

struct emcuetiti_clientstate {
	emcuetiti_clienthandle* client;
	char clientid[LIBMQTT_CLIENTID_MAXLENGTH + 1];
	uint16_t keepalive;
	EMCUETITI_CONFIG_TIMESTAMPTYPE lastseen;

	unsigned numsubscriptions;
	emcuetiti_subscription subscriptions[EMCUETITI_CONFIG_MAXSUBSPERCLIENT];

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
// port structures

typedef struct {
	emcuetiti_publishreadyfunc publishreadycallback;
	emcuetiti_pollfunc pollfunc;
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
	uint8_t* buffer;
	size_t len;
	size_t writepos;
	size_t readpos;
} emcuetiti_bufferholder;

typedef struct {
	emcuetiti_authenticateclientfunc authenticatecallback;
	emcuetiti_timstampfunc timestamp;

	// client funcs
	libmqtt_writefunc writefunc;
	emcuetiti_disconnectfunc disconnectfunc;
} emcuetiti_brokerhandle_callbacks;

struct emcuetiti_brokerhandle {
	unsigned registeredclients;
	emcuetiti_topichandle* root;
	emcuetiti_porthandle* ports[EMCUETITI_CONFIG_MAXPORTS];
	emcuetiti_clientstate clients[EMCUETITI_CONFIG_MAXCLIENTS];
	const emcuetiti_brokerhandle_callbacks* callbacks;
	void* userdata;
};
