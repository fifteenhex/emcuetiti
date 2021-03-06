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

#include <string.h>

#include <stdio.h>
#include <assert.h>

#include "libmqtt_priv.h"
#include "libmqtt.h"
#include "buffers.h"

typedef struct {
	const uint8_t lengthmsb;
	const uint8_t lengthlsb;
	const uint8_t protocolname[4];
	const uint8_t protocollevel;
	const uint8_t flags;
	const uint8_t keepalivemsb;
	const uint8_t keepalivelsb;
}__attribute__ ((packed)) libmqtt_connect_variableheader;

typedef struct {
	const uint8_t flags;
	const uint8_t returncode;
}__attribute__ ((packed)) libmqtt_connack_variableheader;

typedef struct {
	const uint8_t packetidmsb;
	const uint8_t packetidlsb;
}__attribute__ ((packed)) libmqtt_messageid_variableheader;

static void libmqtt_appendlengthandstring(libmqtt_writefunc writefunc,
		void* userdata, const char* string, size_t len) {
	uint8_t strlen[2] = { ((len >> 8) & 0xff), (len & 0xff) };
	writefunc(userdata, strlen, sizeof(strlen));
	writefunc(userdata, string, len);
}

static void libmqtt_appendlengthandstring_writer(libmqtt_writefunc writefunc,
		void* userdata, libmqtt_topicwriter writer, void* topicdata, size_t len) {
	uint8_t strlen[2] = { ((len >> 8) & 0xff), (len & 0xff) };
	writefunc(userdata, strlen, sizeof(strlen));
	writer(writefunc, userdata, topicdata);
}

int libmqtt_encodelength(uint8_t* buffer, size_t bufferlen, size_t len,
		size_t* fieldlen) {
	size_t written = 0;
	do {
		uint8_t byte = len % 128;
		len = (len - byte) / 128;
		if (len != 0)
			byte |= (1 << 7);
		*buffer++ = byte;
		written++;
	} while (len > 0);

	*fieldlen = written;

	return 0;
}

int libmqtt_construct_connect(libmqtt_writefunc writefunc, void* userdata,
		uint16_t keepalive, const char* clientid, const char* willtopic,
		const char* willmessage, const char* username, const char* password,
		bool cleansession) {

	uint8_t flags = 0;
	size_t clientidlen = strlen(clientid);
	size_t payloadsz = clientidlen + 2;

	bool havewill = willtopic != NULL && willmessage != NULL;
	size_t willtopiclen;
	size_t willmessagelen;
	if (havewill) {
		flags |= LIBMQTT_FLAGS_CONNECT_WILLFLAG;
		willtopiclen = strlen(willtopic);
		willmessagelen = strlen(willmessage);
		payloadsz += willtopiclen + willmessagelen + 4;
	}

	size_t usernamelen;
	if (username != NULL) {
		flags |= LIBMQTT_FLAGS_CONNECT_USERNAMEFLAG;
		usernamelen = strlen(username);
		payloadsz += usernamelen + 2;
	}

	size_t passwordlen;
	if (password != NULL) {
		flags |= LIBMQTT_FLAGS_CONNECT_PASSWORDFLAG;
		passwordlen = strlen(password);
		payloadsz += passwordlen;
	}

	if (cleansession) {
		flags |= LIBMQTT_FLAGS_CONNECT_CLEANSESSION;
	}

	libmqtt_connect_variableheader varhead = {
			.lengthmsb = 0, //
			.lengthlsb = 4, //
			.protocolname = { 'M', 'Q', 'T', 'T' }, .protocollevel = 0x04,
			.flags = flags, .keepalivemsb = LIBMQTT_MSB(keepalive),
			.keepalivelsb = LIBMQTT_MSB(keepalive) };

// now we know how much var header and payload we have we can calculate the
// length of the fixed header
	uint8_t fixedheader[LIBMQTT_MAXIMUMFIXEDHEADERBYTES];
	fixedheader[0] = LIBMQTT_PACKETYPEANDFLAGS(LIBMQTT_PACKETTYPE_CONNECT, 0);
	size_t remainingbytensfieldlen;
	libmqtt_encodelength(fixedheader + 1, sizeof(fixedheader) - 1,
			sizeof(varhead) + payloadsz, &remainingbytensfieldlen);
	size_t fixedheadersz = 1 + remainingbytensfieldlen;

// write out the packet

	writefunc(userdata, fixedheader, fixedheadersz);

	writefunc(userdata, (uint8_t*) &varhead, sizeof(varhead));

	libmqtt_appendlengthandstring(writefunc, userdata, clientid, clientidlen);

	if (havewill) {
		libmqtt_appendlengthandstring(writefunc, userdata, willtopic,
				willtopiclen);
		libmqtt_appendlengthandstring(writefunc, userdata, willmessage,
				willmessagelen);
	}

	if (username != NULL)
		libmqtt_appendlengthandstring(writefunc, userdata, username,
				usernamelen);

	if (password != NULL)
		libmqtt_appendlengthandstring(writefunc, userdata, password,
				passwordlen);

	return 0;
}

int libmqtt_construct_connack(libmqtt_writefunc writefunc, void* userdata) {

	libmqtt_connack_variableheader varheader = {

	};

	uint8_t fixedheader[LIBMQTT_MINIMUMFIXEDHEADERBYTES];
	fixedheader[0] = LIBMQTT_PACKETYPEANDFLAGS(LIBMQTT_PACKETTYPE_CONNACK, 0);
	size_t remainingbytesfiedlen;
	libmqtt_encodelength(fixedheader + 1, sizeof(fixedheader) - 1,
			sizeof(varheader), &remainingbytesfiedlen);

	writefunc(userdata, fixedheader, 1 + remainingbytesfiedlen);
	writefunc(userdata, (uint8_t*) &varheader, sizeof(varheader));
	return 0;
}

int libmqtt_construct_publish(
		libmqtt_writefunc writefunc,// function to write data into the client or buffer
		void* writeuserdata,		// pointer to the data needed for the above
		libmqtt_readfunc readfunc,		// function to read data for the payload
		void* readuserdata,			// pointer to the data needed for the above
		libmqtt_topicwriter topicwriter,// function to write the topic into the client or buffer
		void* topicdata, 			// point to the data needed for the above
		size_t topiclen,				// length the topic that will be written
		size_t payloadlen, libmqtt_qos qos, bool duplicate, bool retain,
		uint16_t id) {

	bool needsid = qos > LIBMQTT_QOS0_ATMOSTONCE;

	libmqtt_messageid_variableheader messageid = { .packetidmsb = (id >> 8)
			& 0xff, .packetidlsb = id & 0xff };

	uint8_t fixedheader[LIBMQTT_MAXIMUMFIXEDHEADERBYTES];
	fixedheader[0] = LIBMQTT_PACKETYPEANDFLAGS(LIBMQTT_PACKETTYPE_PUBLISH, 0);
	size_t remainingbytesfiedlen;
	libmqtt_encodelength(fixedheader + 1, sizeof(fixedheader) - 1,
			LIBMQTT_MQTTSTRLEN(topiclen) + (needsid ? sizeof(messageid) : 0)
					+ payloadlen, &remainingbytesfiedlen);

// fixed header
	writefunc(writeuserdata, fixedheader, 1 + remainingbytesfiedlen);

// var header
	libmqtt_appendlengthandstring_writer(writefunc, writeuserdata, topicwriter,
			topicdata, topiclen);
	if (needsid)
		writefunc(writeuserdata, (uint8_t*) &messageid, sizeof(messageid));

// payload
	size_t payloadremaining = payloadlen;
	uint8_t buffer[32];
	while (payloadremaining > 0) {
		int want = sizeof(buffer);
		if (payloadremaining < want)
			want = payloadremaining;

		int read = readfunc(readuserdata, buffer, want);
		writefunc(writeuserdata, buffer, want);
		payloadremaining -= read;
	}

	return 0;
}

int libmqtt_construct_subscribe(libmqtt_writefunc writefunc, void* userdata,
		libmqtt_subscription* subscriptions, int numsubscriptions,
		uint16_t messageid) {

	libmqtt_messageid_variableheader varheader = { //
			.packetidmsb = (messageid >> 8) & 0xff, //
					.packetidlsb = messageid & 0xff };

	size_t payloadsz = 0;
	for (int i = 0; i < numsubscriptions; i++)
		payloadsz += 2 + strlen(subscriptions[i].topic) + 1;

	uint8_t fixedheader[LIBMQTT_MAXIMUMFIXEDHEADERBYTES];
	fixedheader[0] = LIBMQTT_PACKETYPEANDFLAGS(LIBMQTT_PACKETTYPE_SUBSCRIBE, 2);

	size_t remainingbytesfiedlen;
	libmqtt_encodelength(fixedheader + 1, sizeof(fixedheader) - 1,
			sizeof(varheader) + payloadsz, &remainingbytesfiedlen);

	writefunc(userdata, fixedheader, 1 + remainingbytesfiedlen);
	writefunc(userdata, (uint8_t*) &varheader, sizeof(varheader));

	for (int i = 0; i < numsubscriptions; i++) {
		libmqtt_appendlengthandstring(writefunc, userdata,
				subscriptions[i].topic, strlen(subscriptions[i].topic));
		writefunc(userdata, &(subscriptions[i].qos), 1);
	}

	return 0;
}

int libmqtt_construct_suback(libmqtt_writefunc writefunc, void* userdata,
		uint16_t id, uint8_t* returncodes, int numreturncodes) {

	libmqtt_messageid_variableheader varheader = { .packetidmsb = LIBMQTT_MSB(
			id), .packetidlsb = LIBMQTT_LSB(id) };

	uint8_t fixedheader[LIBMQTT_MAXIMUMFIXEDHEADERBYTES];
	fixedheader[0] = LIBMQTT_PACKETYPEANDFLAGS(LIBMQTT_PACKETTYPE_SUBACK, 0);
	size_t remainingbytesfiedlen;
	libmqtt_encodelength(fixedheader + 1, sizeof(fixedheader) - 1,
			sizeof(varheader) + numreturncodes, &remainingbytesfiedlen);

	writefunc(userdata, fixedheader, 1 + remainingbytesfiedlen);
	writefunc(userdata, (uint8_t*) &varheader, sizeof(varheader));

	for (int i = 0; i < numreturncodes; i++)
		writefunc(userdata, returncodes++, 1);

	return 0;
}

int libmqtt_construct_unsubscribe(libmqtt_writefunc writefunc, void* userdata,
		const char** topics, int numtopics) {

	libmqtt_messageid_variableheader varheader = {

	};

	size_t payloadsz = 0;
	for (int i = 0; i < numtopics; i++)
		payloadsz += 2 + strlen(topics[i]);

	uint8_t fixedheader[LIBMQTT_MAXIMUMFIXEDHEADERBYTES];
	fixedheader[0] = LIBMQTT_PACKETYPEANDFLAGS(LIBMQTT_PACKETTYPE_SUBSCRIBE, 0);

	size_t remainingbytesfiedlen;
	libmqtt_encodelength(fixedheader + 1, sizeof(fixedheader) - 1,
			sizeof(varheader) + payloadsz, &remainingbytesfiedlen);

	writefunc(userdata, fixedheader, 1 + remainingbytesfiedlen);
	writefunc(userdata, (uint8_t*) &varheader, sizeof(varheader));

	for (int i = 0; i < numtopics; i++)
		libmqtt_appendlengthandstring(writefunc, userdata, topics[i],
				strlen(topics[i]));

	return 0;
}

int libmqtt_extractmqttstring(uint8_t* mqttstring, uint8_t* buffer,
		size_t bufferlen) {
	uint16_t strlen = (mqttstring[0] << 8) | mqttstring[1];
	buffer[strlen] = '\0';
	memcpy(buffer, mqttstring + 2, strlen);
	return 0;
}

libmqtt_packetread_state libmqtt_nextstateafterlen(uint8_t type, size_t length,
		size_t pos) {
	libmqtt_packetread_state nextstate = LIBMQTT_PACKETREADSTATE_ERROR;
	switch (type) {
	case LIBMQTT_PACKETTYPE_CONNECT:
		nextstate = LIBMQTT_PACKETREADSTATE_CONNECT_START;
		break;
	case LIBMQTT_PACKETTYPE_CONNACK:
		nextstate = LIBMQTT_PACKETREADSTATE_CONNACK_START;
		break;
	case LIBMQTT_PACKETTYPE_SUBSCRIBE:
		nextstate = LIBMQTT_PACKETREADSTATE_SUBSCRIBE_START;
		break;
	case LIBMQTT_PACKETTYPE_SUBACK:
		nextstate = LIBMQTT_PACKETREADSTATE_SUBACK_START;
		break;
	case LIBMQTT_PACKETTYPE_UNSUBSCRIBE:
		nextstate = LIBMQTT_PACKETREADSTATE_UNSUBSCRIBE_START;
		break;
	case LIBMQTT_PACKETTYPE_PUBLISH:
		nextstate = LIBMQTT_PACKETREADSTATE_PUBLISH_START;
		break;
	default: {
		bool hasmessageid = (type == LIBMQTT_PACKETTYPE_PUBACK)
				|| (type == LIBMQTT_PACKETTYPE_PUBREC)
				|| (type == LIBMQTT_PACKETTYPE_PUBREL)
				|| (type == LIBMQTT_PACKETTYPE_PUBCOMP)
				|| (type == LIBMQTT_PACKETTYPE_UNSUBACK);
		if (hasmessageid)
			nextstate = LIBMQTT_PACKETREADSTATE_MSGID;
		else if (length == pos)
			nextstate = LIBMQTT_PACKETREADSTATE_FINISHED;
	}
		break;
	}
	return nextstate;
}
