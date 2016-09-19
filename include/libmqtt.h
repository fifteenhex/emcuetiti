#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "buffers_types.h"

typedef enum {
	LIBMQTT_QOS0_ATMOSTONCE, LIBMQTT_QOS1_ATLEASTONCE, LIBMQTT_QOS2_EXACTLYONCE
} libmqtt_qos;

#define LIBMQTT_CLIENTID_MINLEGNTH		1
#define LIBMQTT_CLIENTID_MAXLENGTH		23

#define LIBMQTT_PACKETTYPE_CONNECT		1
#define LIBMQTT_PACKETTYPE_CONNACK		2
#define LIBMQTT_PACKETTYPE_PUBLISH		3
#define LIBMQTT_PACKETTYPE_PUBACK		4
#define LIBMQTT_PACKETTYPE_PUBREC		5
#define LIBMQTT_PACKETTYPE_PUBREL		6
#define LIBMQTT_PACKETTYPE_PUBCOMP		7
#define LIBMQTT_PACKETTYPE_SUBSCRIBE	8
#define LIBMQTT_PACKETTYPE_SUBACK		9
#define LIBMQTT_PACKETTYPE_UNSUBSCRIBE	10
#define LIBMQTT_PACKETTYPE_UNSUBACK		11
#define LIBMQTT_PACKETTYPE_PINGREQ		12
#define LIBMQTT_PACKETTYPE_PINGRESP		13
#define LIBMQTT_PACKETTYPE_DISCONNECT	14

#define LIBMQTT_PACKETTYPE_SHIFT		4
#define LIBMQTT_PACKETTYPEFROMPACKETTYPEANDFLAGS(tf) ((tf >> LIBMQTT_PACKETTYPE_SHIFT) & 0xf)
#define LIBMQTT_PACKETFLAGSFROMPACKETTYPEANDFLAGS(tf) (tf & 0xf)

#define LIBMQTT_CONNECTRETURNCODE_ACCEPTED						0
#define LIBMQTT_CONNECTRETURNCODE_UNACCEPTABLEPROTOCOLVERSION	1
#define LIBMQTT_CONNECTRETURNCODE_IDENTIFIERREJECTED			2
#define LIBMQTT_CONNECTRETURNCODE_SERVERUNAVAILABLE				3
#define LIBMQTT_CONNECTRETURNCODE_BADUSERNAMEORPASSWORD			4
#define LIBMQTT_CONNECTRETURNCODE_NOTAUTHORISED					5

#define LIBMQTT_SUBSCRIBERETURNCODE_QOS0GRANTED					0x0
#define LIBMQTT_SUBSCRIBERETURNCODE_QOS1GRANTED					0x1
#define LIBMQTT_SUBSCRIBERETURNCODE_QOS2GRANTED					0x2
#define LIBMQTT_SUBSCRIBERETURNCODE_FAILURE						0x80

#define LIBMQTT_LEN(b) (b & ~(1 << 7))
#define LIBMQTT_ISLASTLENBYTE(b) ((b & (1 << 7)) == 0)

// errors
// non-fatal
#define LIBMQTT_EWOULDBLOCK			-1
// fatal
#define LIBMQTT_EFATAL				-2
#define LIBMQTT_EREMOTEDISCONNECTED	-3
#define LIBMQTT_ETOOBIG				-4

// types
typedef struct {
	const char* topic;
	uint8_t qos;
} libmqtt_subscription;

typedef struct {
	const libmqtt_subscription* subs;
	uint8_t* results;
	unsigned numsubs;
} libmqtt_subtransaction;

typedef enum {
	LIBMQTT_PACKETREADSTATE_TYPE,		// packet type
	LIBMQTT_PACKETREADSTATE_LEN,		// packet remaining length
	LIBMQTT_PACKETREADSTATE_MSGID,// packet message id, only valid for packets that need it
	LIBMQTT_PACKETREADSTATE_COMMON_END,

	LIBMQTT_PACKETREADSTATE_CONNECT_START = 10,
	LIBMQTT_PACKETREADSTATE_CONNECT_PROTOLEN = 10,	//
	LIBMQTT_PACKETREADSTATE_CONNECT_PROTO,			//
	LIBMQTT_PACKETREADSTATE_CONNECT_PROTOLEVEL, 	//
	LIBMQTT_PACKETREADSTATE_CONNECT_FLAGS,			//
	LIBMQTT_PACKETREADSTATE_CONNECT_KEEPALIVE,		//
	LIBMQTT_PACKETREADSTATE_CONNECT_CLIENTIDLEN,	//
	LIBMQTT_PACKETREADSTATE_CONNECT_CLIENTID,		//
	LIBMQTT_PACKETREADSTATE_CONNECT_END,

	LIBMQTT_PACKETREADSTATE_CONNACK_START = 20,
	LIBMQTT_PACKETREADSTATE_CONNACK_FLAGS = 20,	// connection flags, only valid for connack
	LIBMQTT_PACKETREADSTATE_CONNACK_RETCODE,// connection return code, only valid for connack
	LIBMQTT_PACKETREADSTATE_CONNACK_END,

	LIBMQTT_PACKETREADSTATE_SUBSCRIBE_START = 30,
	LIBMQTT_PACKETREADSTATE_SUBSCRIBE_MSGID = 30,
	LIBMQTT_PACKETREADSTATE_SUBSCRIBE_TOPICLEN, //
	LIBMQTT_PACKETREADSTATE_SUBSCRIBE_TOPICFILTER, //
	LIBMQTT_PACKETREADSTATE_SUBSCRIBE_QOS, //
	LIBMQTT_PACKETREADSTATE_SUBSCRIBE_END,

	LIBMQTT_PACKETREADSTATE_SUBACK_START = 40,
	LIBMQTT_PACKETREADSTATE_SUBACK_MSGID = 40,
	LIBMQTT_PACKETREADSTATE_SUBACK_RESULT, //
	LIBMQTT_PACKETREADSTATE_SUBACK_END,

	LIBMQTT_PACKETREADSTATE_UNSUBSCRIBE_START = 50,
	LIBMQTT_PACKETREADSTATE_UNSUBSCRIBE_MSGID = 50,
	LIBMQTT_PACKETREADSTATE_UNSUBSCRIBE_TOPICLEN, //
	LIBMQTT_PACKETREADSTATE_UNSUBSCRIBE_TOPICFILTER, //
	LIBMQTT_PACKETREADSTATE_UNSUBSCRIBE_END,

	LIBMQTT_PACKETREADSTATE_PUBLISH_START = 60,
	LIBMQTT_PACKETREADSTATE_PUBLISH_TOPICLEN = 60, // packet topic length, only valid for publish
	LIBMQTT_PACKETREADSTATE_PUBLISH_TOPIC, // packet topic, only valid for publish
	LIBMQTT_PACKETREADSTATE_PUBLISH_MSGID,
	LIBMQTT_PACKETREADSTATE_PUBLISH_PAYLOAD, // packet payload, only valid for packets that have a payload
	LIBMQTT_PACKETREADSTATE_PUBLISH_END,

	LIBMQTT_PACKETREADSTATE_FINISHED = 100,	// packet has been read completely
	LIBMQTT_PACKETREADSTATE_ERROR
} libmqtt_packetread_state;

typedef struct {
	uint8_t level;
	uint8_t flags;
	uint16_t keepalive;
	uint16_t clientidlen;
} libmqtt_packet_connect;

typedef struct {
	uint8_t ackflags;
	uint8_t returncode;
} libmqtt_packet_connack;

typedef struct {
	uint16_t msgid;
	uint16_t topicfilterlen;
	uint8_t topicfilterqos;
} libmqtt_packet_subscribe;

typedef struct {
	uint16_t msgid;
	uint8_t result;
} libmqtt_packet_suback;

typedef struct {
	uint16_t msgid;
	uint16_t topicfilterlen;
} libmqtt_packet_unsubscribe;

typedef struct {
	uint16_t msgid;
	uint16_t topiclen;
} libmqtt_packet_publish;

typedef union {
	uint16_t msgid;
	libmqtt_packet_connect connect;
	libmqtt_packet_connack connack;
	libmqtt_packet_publish publish;
	libmqtt_packet_subscribe subscribe;
	libmqtt_packet_suback suback;
	libmqtt_packet_unsubscribe unsubscribe;
} libmqtt_packetread_registers;

typedef struct {
	libmqtt_packetread_state state;
	uint8_t type;
	uint8_t flags;
	size_t length;
	libmqtt_packetread_registers varhdr;
	// secret, don't touch
	size_t pos;
	unsigned lenmultiplier;
	uint8_t counter;
	BUFFERS_STATICBUFFER(buffer, 64);
} libmqtt_packetread;

// callbacks

typedef int (*libmqtt_writefunc)(void* userdata, const uint8_t* buffer,
		size_t len);
typedef int (*libmqtt_readfunc)(void* userdata, uint8_t* buffer, size_t len);
typedef int (*libmqtt_topicwriter)(libmqtt_writefunc writefunc,
		void* writefuncuserdata, void* userdata);

typedef int (*libmqtt_packetreadchange)(libmqtt_packetread* pkt,
		libmqtt_packetread_state previousstate, void* userdata);

// User API

int libmqtt_encodelength(uint8_t* buffer, size_t bufferlen, size_t len,
		size_t* fieldlen);

int libmqtt_construct_connect(libmqtt_writefunc writefunc, void* userdata,
		uint16_t keepalive, const char* clientid, const char* willtopic,
		const char* willmessage, const char* username, const char* password,
		bool cleansession);

int libmqtt_construct_connack(libmqtt_writefunc writefunc, void* userdata);

int libmqtt_construct_publish(
		libmqtt_writefunc writefunc, //
		void* writeuserdata, //
		libmqtt_readfunc readfunc, //
		void* readuserdata, //
		libmqtt_topicwriter topicwriter, //
		void* topicdata, //
		size_t topicln, //
		size_t payloadlen, libmqtt_qos qos, bool duplicate, bool retain,
		uint16_t id);

int libmqtt_construct_puback(libmqtt_writefunc writefunc, void* userdata,
		uint16_t messageid);

int libmqtt_construct_subscribe(libmqtt_writefunc writefunc, void* userdata,
		libmqtt_subscription* subscriptions, int numsubscriptions,
		uint16_t messageid);

int libmqtt_construct_suback(libmqtt_writefunc writefunc, void* userdata,
		uint16_t id, uint8_t* returncodes, int numreturncodes);

int libmqtt_construct_unsuback(libmqtt_writefunc writefunc, void* userdata,
		uint16_t messageid);

int libmqtt_construct_pingreq(libmqtt_writefunc writefunc, void* userdata);

int libmqtt_construct_pingresp(libmqtt_writefunc writefunc, void* userdata);

int libmqtt_extractmqttstring(uint8_t* mqttstring, uint8_t* buffer,
		size_t bufferlen);

int libmqtt_decodelength(uint8_t* buffer, size_t* len);

int libmqtt_readpkt(libmqtt_packetread* pkt, // pkt being read
		libmqtt_packetreadchange changefunc, void* changeuserdata, // pkt state change callback
		libmqtt_readfunc readfunc, void* readuserdata, // func and data for reading the packet in
		libmqtt_writefunc payloadwritefunc, void* payloadwriteuserdata); // func and data for writing the payload out
