#include <string.h>
#include <stdbool.h>

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
}__attribute__ ((packed)) libmqtt_subscribe_variableheader;

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
	size_t tmp = 0;
	uint8_t byte;
	unsigned multiplier = 1;

	do {
		byte = *(buffer++);
		tmp += (byte & ~(1 << 7)) * multiplier;
		multiplier *= 128;
	} while ((byte & (1 << 7)) != 0);

	*len = tmp;

	return 0;
}

static void libmqtt_appendlengthandstring(libmqtt_writefunc writefunc,
		void* userdata, const char* string, size_t len) {
	uint8_t strlen[2] = { ((len >> 8) & 0xff), (len & 0xff) };
	writefunc(userdata, strlen, sizeof(strlen));
	writefunc(userdata, string, len);
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
	uint8_t fixedheader[8];
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

	uint8_t fixedheader[2];
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
		size_t payloadlen) {

	size_t topiclen = strlen(topic);

	uint8_t fixedheader[2];
	fixedheader[0] = LIBMQTT_PACKETYPEANDFLAGS(LIBMQTT_PACKETTYPE_PUBLISH, 0);
	size_t remainingbytesfiedlen;
	libmqtt_encodelength(fixedheader + 1, sizeof(fixedheader) - 1,
	LIBMQTT_MQTTSTRLEN(topiclen) + payloadlen, &remainingbytesfiedlen);

	write(writeuserdata, fixedheader, 1 + remainingbytesfiedlen);
	libmqtt_appendlengthandstring(writefunc, writeuserdata, topic, topiclen);

	size_t payloadremaining = payloadlen;
	uint8_t buffer[32];
	while (payloadremaining > 0) {
		int read = readfunc(readuserdata, buffer, sizeof(buffer));
		writefunc(readuserdata, buffer, sizeof(buffer));
		payloadremaining -= read;
	}

	return 0;
}

int libmqtt_construct_puback(uint8_t* buffer) {
	return 0;
}

int libmqtt_construct_pubrec(uint8_t* buffer) {
	return 0;
}

int libmqtt_construct_pubrel(uint8_t* buffer) {
	return 0;
}

int libmqtt_construct_pubcomp(uint8_t* buffer) {
	return 0;
}

int libmqtt_construct_subscribe(libmqtt_writefunc writefunc, void* userdata,
		libmqtt_subscription* subscriptions, int numsubscriptions) {

	libmqtt_subscribe_variableheader varheader = {

	};

	size_t payloadsz = 0;
	for (int i = 0; i < numsubscriptions; i++)
		payloadsz += 2 + strlen(subscriptions[i].topic) + 1;

	uint8_t fixedheader[2];
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
		uint8_t* returncodes, int numreturncodes) {

	libmqtt_subscribe_variableheader varheader = {

	};

	uint8_t fixedheader[2];
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

int libmqtt_construct_unsubscribe(uint8_t* buffer) {
	return 0;
}

int libmqtt_construct_unsuback(uint8_t* buffer) {
	return 0;
}

int libmqtt_construct_pingreq(uint8_t* buffer) {
	return 0;
}

int libmqtt_construct_pingresp(uint8_t* buffer) {
	return 0;
}

int libmqtt_construct_disconnect(uint8_t* buffer) {
	return 0;
}

int libmqtt_extractmqttstring(uint8_t* mqttstring, uint8_t* buffer,
		size_t bufferlen) {
	uint16_t strlen = (mqttstring[0] << 8) | mqttstring[1];
	buffer[strlen] = '\0';
	memcpy(buffer, mqttstring + 2, strlen);
	return 0;
}
