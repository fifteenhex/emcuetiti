#pragma once

#include <stdint.h>
#include <stddef.h>

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
#define LIBMQTT_PACKETTYPE_DISCONNECT	15

#define LIBMQTT_PACKETTYPE_SHIFT		4
#define LIBMQTT_PACKETTYPEFROMPACKETTYPEANDFLAGS(tf) ((tf >> LIBMQTT_PACKETTYPE_SHIFT) & 0xf)

typedef int (*libmqtt_writefunc)(void* userdata, const uint8_t* buffer,
		size_t len);
typedef int (*libmqtt_readfunc)(void* userdata, uint8_t* buffer, size_t len);

typedef struct {
	const char* topic;
	uint8_t qos;
} libmqtt_subscription;

int libmqtt_construct_connect(libmqtt_writefunc writefunc, void* userdata,
		const char* clientid, const char* willtopic, const char* willmessage,
		const char* username, const char* password);

int libmqtt_construct_connack(libmqtt_writefunc writefunc, void* userdata);

int libmqtt_construct_publish(libmqtt_writefunc writefunc, void* writeuserdata,
		libmqtt_readfunc readfunc, void* readuserdata, const char* topic,
		size_t payloadlen);

int libmqtt_construct_subscribe(libmqtt_writefunc writefunc, void* userdata,
		libmqtt_subscription* subscriptions, int numsubscriptions);

int libmqtt_construct_suback(libmqtt_writefunc writefunc, void* userdata,
		uint8_t* returncodes, int numreturncodes);

int libmqtt_extractmqttstring(uint8_t* mqttstring, uint8_t* buffer,
		size_t bufferlen);
