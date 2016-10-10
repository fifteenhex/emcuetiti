#pragma once

#include "libmqtt.h"

#include "buffers.h"

typedef struct {
	libmqtt_packetread_state state;
	libmqtt_packetread_registers varhdr;
	// secret, don't touch
	size_t pos;
	unsigned lenmultiplier;
	uint8_t counter;
	BUFFERS_STATICBUFFER(buffer, 64);
} libmqtt_packetwrite;

typedef int (*libmqtt_packetwritechange)(libmqtt_packetwrite* pkt,
		libmqtt_packetread_state previousstate, void* userdata);

void libmqtt_writepkt_reset(libmqtt_packetwrite* pkt);

void libmqtt_writepkt_puback(libmqtt_packetwrite* pkt, uint16_t messageid);
void libmqtt_writepkt_pubrec(libmqtt_packetwrite* pkt, uint16_t messageid);
void libmqtt_writepkt_pubrel(libmqtt_packetwrite* pkt, uint16_t messageid);
void libmqtt_writepkt_pubcomp(libmqtt_packetwrite* pkt, uint16_t messageid);
void libmqtt_writepkt_unsuback(libmqtt_packetwrite* pkt, uint16_t messageid);
void libmqtt_writepkt_pingreq(libmqtt_packetwrite* pkt);
void libmqtt_writepkt_pingresp(libmqtt_packetwrite* pkt);
void libmqtt_writepkt_disconnect(libmqtt_packetwrite* pkt);

int libmqtt_writepkt(libmqtt_packetwrite* pkt, // packet being written
		libmqtt_packetwritechange changefunc, void* changeuserdata, // packet state change callback
		libmqtt_writefunc writefunc, void* writeuserdata, // func and data for writing the packet out
		libmqtt_readfunc payloadreadfunc, void* payloadreaduserdata); // func and data for reading the payload in
