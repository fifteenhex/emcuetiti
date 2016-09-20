#pragma once

#include "emcuetiti_types.h"
#include "buffers.h"

#define EMCUETITI_PORT_REMOTE_OK		0
#define EMCUETITI_PORT_REMOTE_TRYAGAIN	-1
#define EMCUETITI_PORT_REMOTE_ERR		-2

typedef int (*emcuetiti_port_remote_connect)(const char* host, unsigned port,
		void** connectiondata);
typedef int (*emcuetiti_port_remote_disconnect)(void* connectiondata);
typedef libmqtt_readfunc emcuetiti_port_remote_read;
typedef libmqtt_writefunc emcuetiti_port_remote_write;
typedef bool (*emcuetiti_port_remote_datawaiting)(void* connectiondata);

typedef struct {
	emcuetiti_port_remote_connect connect;
	emcuetiti_port_remote_disconnect disconnect;
	emcuetiti_port_remote_read read;
	emcuetiti_port_remote_write write;
	emcuetiti_port_remote_datawaiting datawaiting;
#if EMCUETITI_CONFIG_HAVETLS
	emcuetiti_port_remote_connect connect_tls;
	emcuetiti_port_remote_disconnect disconnect_tls;
	emcuetiti_port_remote_read read_tls;
	emcuetiti_port_remote_write write_tls;
	emcuetiti_port_remote_datawaiting datawaiting_tls;
#endif

} emcuetiti_port_remote_hostops;

typedef enum {
	REMOTEPORTSTATE_NOTCONNECTED,	// socket is not yet connected
	REMOTEPORTSTATE_CONNECTING,		// mqtt is connecting
	REMOTEPORTSTATE_SUBSCRIBING,	// subscribing to topics
	REMOTEPORTSTATE_READY,			// connected, subscribed, normal state
	REMOTEPORTSTATE_DISCONNECTED,	//
	REMOTEPORTSTATE_ERROR			// something bad happened
} emcuetiti_port_remote_state;

typedef enum {
	REMOTEPORTSTATE_READY_READ,
	REMOTEPORTSTATE_READY_WRITE,
	REMOTEPORTSTATE_READY_KEEPALIVE,
	REMOTEPORTSTATE_READY_END
} emcuetiti_port_remote_readystate;

typedef struct {
	char* host; 			// host to connect to
	unsigned port;		// port on host
	char* clientid;		// clientid
	unsigned keepalive;			// keepalive timeout
	libmqtt_subscription* topics;		// topics to subscribe to
	unsigned numtopics;			// number of topics pointed to by the above
#if EMCUETITI_CONFIG_HAVETLS
	void* tlsconfig; // if using tls this should point to a tls config, else null
#endif
	const emcuetiti_port_remote_hostops* hostops; //
} emcuetiti_port_remoteconfig;

typedef struct {
	emcuetiti_timestamp lastattempt;
} emcuetiti_port_remote_statedata_notconnected;

typedef struct {
	libmqtt_packetread pktread;
	emcuetiti_timestamp connsentat;
	bool connreqsent;
} emcuetiti_port_remote_statedata_connecting;

typedef struct {
	libmqtt_packetread pktread;
	bool subreqsent;
	emcuetiti_timestamp subreqsentat;
	uint16_t msgid;
} emcuetiti_port_remote_statedata_subscribing;

typedef struct {
	emcuetiti_port_remote_readystate state;
	libmqtt_packetread pktread;
	emcuetiti_timestamp datalastsent;
	emcuetiti_timestamp datalastreceived;
} emcuetiti_port_remote_statedata_ready;

typedef union {
	emcuetiti_port_remote_statedata_notconnected notconnected;
	emcuetiti_port_remote_statedata_connecting connecting;
	emcuetiti_port_remote_statedata_subscribing subscribing;
	emcuetiti_port_remote_statedata_ready ready;
} emcuetiti_port_remote_statedata;

typedef struct {
	const emcuetiti_brokerhandle* broker;
	const emcuetiti_port_remoteconfig* config;
	emcuetiti_port_remote_state state;
	emcuetiti_port_remote_statedata statedata;
	void* connectiondata;
	uint16_t msgid;

// incoming publish tracking
	BUFFERS_STATICBUFFER(topicbuffer, EMCUETITI_CONFIG_MAXTOPICPARTLEN);
	emcuetiti_topichandle* topic;
	uint8_t* publish;
	bool publishwaiting;
} emcuetiti_port_remote_portdata;

void emcuetiti_port_remote_new(emcuetiti_brokerhandle* broker,
		emcuetiti_port_remoteconfig* config, emcuetiti_porthandle* port,
		emcuetiti_port_remote_portdata* portdata);
