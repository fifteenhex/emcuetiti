#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

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

	// varheader processing
	LIBMQTT_PACKETREADSTATE_TOPICLEN,// packet topic length, only valid for publish
	LIBMQTT_PACKETREADSTATE_TOPIC,		// packet topic, only valid for publish
	LIBMQTT_PACKETREADSTATE_MSGID,// packet message id, only valid for packets that need it
	LIBMQTT_PACKETREADSTATE_CONNFLAGS,// connection flags, only valid for connack
	LIBMQTT_PACKETREADSTATE_CONNRET,// connection return code, only valid for connack

	LIBMQTT_PACKETREADSTATE_PAYLOAD,// packet payload, only valid for packets that have a payload
	LIBMQTT_PACKETREADSTATE_FINISHED,	// packet has been read completely
	LIBMQTT_PACKETREADSTATE_ERROR
} libmqtt_packetread_state;

typedef struct {
	uint8_t ackflags;
	uint8_t returncode;
} libmqtt_packet_connack;

typedef struct {
	uint16_t msgid;
	uint16_t topiclen;
} libmqtt_packet_publish;

typedef union {
	uint16_t msgid;
	libmqtt_packet_connack connack;
	libmqtt_packet_publish publish;
} libmqtt_packetread_varhdr;

typedef struct {
	size_t writepos;
	size_t readpos;
	uint8_t buffer[64];
} libmqtt_bufferhandle;

typedef struct {
	libmqtt_packetread_state state;
	uint8_t type;
	uint8_t flags;
	size_t length;
	libmqtt_packetread_varhdr varhdr;
	// secret, don't touch
	size_t pos;
	unsigned lenmultiplier;
	uint8_t counter;
	libmqtt_bufferhandle buffer;
} libmqtt_packetread;

// callbacks

typedef int (*libmqtt_writefunc)(void* userdata, const uint8_t* buffer,
		size_t len);
typedef int (*libmqtt_readfunc)(void* userdata, uint8_t* buffer, size_t len);
typedef int (*libmqtt_topicwriter)(libmqtt_writefunc writefunc,
		void* writefuncuserdata, void* userdata);

typedef int (*libmqtt_packetreadchange)(libmqtt_packetread* pkt);

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

int libmqtt_readpkt(libmqtt_packetread* pkt,
		libmqtt_packetreadchange changefunc, // pkt and change callback
		libmqtt_readfunc readfunc, void* readuserdata, // func and data for reading the packet in
		libmqtt_writefunc payloadwritefunc, void* payloadwriteuserdata); // func and data for writing the payload out
