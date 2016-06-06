#include <string.h>

#include "libmqtt_priv.h"
#include "libmqtt.h"

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

static int libmqtt_construct_genericack(libmqtt_writefunc writefunc,
		void* userdata, uint8_t typeandflags, uint16_t messageid) {
	libmqtt_messageid_variableheader varheader = { //
			.packetidmsb = (messageid >> 8) & 0xff, //
					.packetidlsb = messageid & 0xff };

	uint8_t fixedheader[LIBMQTT_MINIMUMFIXEDHEADERBYTES];
	fixedheader[0] = typeandflags;
	size_t remainingbytesfiedlen;
	libmqtt_encodelength(fixedheader + 1, sizeof(fixedheader) - 1,
			sizeof(varheader), &remainingbytesfiedlen);

	writefunc(userdata, fixedheader, 1 + remainingbytesfiedlen);
	writefunc(userdata, (uint8_t*) &varheader, sizeof(varheader));
	return 0;
}

static int libmqtt_construct_fixedheaderonly(libmqtt_writefunc writefunc,
		void* userdata, uint8_t typeandflags) {
	uint8_t fixedheader[LIBMQTT_MINIMUMFIXEDHEADERBYTES] = { typeandflags, 0 };
	writefunc(userdata, fixedheader, sizeof(fixedheader));
	return 0;
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

int libmqtt_decodelength(uint8_t* buffer, size_t* len) {
	static const uint8_t continuationmask = (1 << 7);

	size_t tmp = 0;
	uint8_t byte;
	unsigned multiplier = 1;

	do {
		byte = *(buffer++);
		tmp += (byte & ~continuationmask) * multiplier;
		multiplier *= 128;
	} while ((byte & continuationmask) != 0);

	*len = tmp;

	return 0;
}

int libmqtt_construct_connect(libmqtt_writefunc writefunc, void* userdata,
		const char* clientid, const char* willtopic, const char* willmessage,
		const char* username, const char* password) {

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

	libmqtt_connect_variableheader varhead = {
			.lengthmsb = 0, //
			.lengthlsb = 4, //
			.protocolname = { 'M', 'Q', 'T', 'T' }, .protocollevel = 0x04,
			.flags = flags, .keepalivemsb = 0, .keepalivelsb = 0 };

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

int libmqtt_construct_publish(libmqtt_writefunc writefunc, void* writeuserdata,
		libmqtt_readfunc readfunc, void* readuserdata, const char* topic,
		size_t payloadlen, libmqtt_qos qos, bool duplicate, bool retain,
		uint16_t id) {

	bool needsid = qos > LIBMQTT_QOS0_ATMOSTONCE;

	size_t topiclen = strlen(topic);
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
	libmqtt_appendlengthandstring(writefunc, writeuserdata, topic, topiclen);
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

int libmqtt_construct_puback(libmqtt_writefunc writefunc, void* userdata,
		uint16_t messageid) {
	return libmqtt_construct_genericack(writefunc, userdata,
			LIBMQTT_PACKETYPEANDFLAGS(LIBMQTT_PACKETTYPE_PUBACK, 0), messageid);
}

int libmqtt_construct_pubrec(libmqtt_writefunc writefunc, void* userdata) {
	return libmqtt_construct_genericack(writefunc, userdata,
			LIBMQTT_PACKETYPEANDFLAGS(LIBMQTT_PACKETTYPE_PUBREC, 0), 0);
}

int libmqtt_construct_pubrel(libmqtt_writefunc writefunc, void* userdata) {
	return libmqtt_construct_genericack(writefunc, userdata,
			LIBMQTT_PACKETYPEANDFLAGS(LIBMQTT_PACKETTYPE_PUBREL, 0), 0);
}

int libmqtt_construct_pubcomp(libmqtt_writefunc writefunc, void* userdata) {
	return libmqtt_construct_genericack(writefunc, userdata,
			LIBMQTT_PACKETYPEANDFLAGS(LIBMQTT_PACKETTYPE_PUBCOMP, 0), 0);
}

int libmqtt_construct_subscribe(libmqtt_writefunc writefunc, void* userdata,
		libmqtt_subscription* subscriptions, int numsubscriptions) {

	libmqtt_messageid_variableheader varheader = {

	};

	size_t payloadsz = 0;
	for (int i = 0; i < numsubscriptions; i++)
		payloadsz += 2 + strlen(subscriptions[i].topic) + 1;

	uint8_t fixedheader[LIBMQTT_MAXIMUMFIXEDHEADERBYTES];
	fixedheader[0] = LIBMQTT_PACKETYPEANDFLAGS(LIBMQTT_PACKETTYPE_SUBSCRIBE, 0);

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

int libmqtt_construct_unsuback(libmqtt_writefunc writefunc, void* userdata,
		uint16_t messageid) {
	return libmqtt_construct_genericack(writefunc, userdata,
			LIBMQTT_PACKETYPEANDFLAGS(LIBMQTT_PACKETTYPE_UNSUBACK, 0),
			messageid);
}

int libmqtt_construct_pingreq(libmqtt_writefunc writefunc, void* userdata) {
	return libmqtt_construct_fixedheaderonly(writefunc, userdata,
			LIBMQTT_PACKETYPEANDFLAGS(LIBMQTT_PACKETTYPE_PINGREQ, 0));
}

int libmqtt_construct_pingresp(libmqtt_writefunc writefunc, void* userdata) {
	return libmqtt_construct_fixedheaderonly(writefunc, userdata,
			LIBMQTT_PACKETYPEANDFLAGS(LIBMQTT_PACKETTYPE_PINGRESP, 0));
}

int libmqtt_construct_disconnect(libmqtt_writefunc writefunc, void* userdata) {
	return libmqtt_construct_fixedheaderonly(writefunc, userdata,
			LIBMQTT_PACKETYPEANDFLAGS(LIBMQTT_PACKETTYPE_DISCONNECT, 0));
}

int libmqtt_extractmqttstring(uint8_t* mqttstring, uint8_t* buffer,
		size_t bufferlen) {
	uint16_t strlen = (mqttstring[0] << 8) | mqttstring[1];
	buffer[strlen] = '\0';
	memcpy(buffer, mqttstring + 2, strlen);
	return 0;
}
