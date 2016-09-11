#include <string.h>

#include <stdio.h>

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

static void libmqtt_appendlengthandstring_writer(libmqtt_writefunc writefunc,
		void* userdata, libmqtt_topicwriter writer, void* topicdata, size_t len) {
	uint8_t strlen[2] = { ((len >> 8) & 0xff), (len & 0xff) };
	writefunc(userdata, strlen, sizeof(strlen));
	writer(writefunc, userdata, topicdata);
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

int libmqtt_readpkt_changestate(libmqtt_packetread* pkt,
		libmqtt_packetreadchange changefunc, libmqtt_packetread_state newstate) {
	int ret = 0;

	pkt->counter = 0;
	pkt->state = newstate;

// if all of the packet has been read move to finished instead of the next state
	if (pkt->state > LIBMQTT_PACKETREADSTATE_LEN
			&& pkt->state < LIBMQTT_PACKETREADSTATE_FINISHED)
		if (pkt->pos == pkt->length)
			pkt->state = LIBMQTT_PACKETREADSTATE_FINISHED;

	if (changefunc != NULL)
		ret = changefunc(pkt);

	return ret;
}

static void libmqtt_readpkt_incposandcounter(libmqtt_packetread* pkt, int by) {
	pkt->pos += by;
	pkt->counter += by;
}

int libmqtt_readpkt_read(libmqtt_packetread* pkt, libmqtt_readfunc readfunc,
		void* readuserdata, void* buff, size_t len) {
	int ret = readfunc(readuserdata, buff, len);
	if (ret > 0)
		libmqtt_readpkt_incposandcounter(pkt, ret);
	return ret;
}

int libmqtt_buffer_flush(libmqtt_bufferhandle* buffer,
		libmqtt_writefunc writefunc, void* userdata) {

	int ret = 0;
	if (buffer->writepos != 0) {
		if (writefunc != NULL) {
			size_t writesz = buffer->writepos - buffer->readpos;
			ret = writefunc(userdata, buffer->buffer + buffer->readpos,
					writesz);
			if (ret > 0)
				buffer->readpos += ret;
		} else
			buffer->readpos = buffer->writepos;

		if (buffer->readpos == buffer->writepos) {
			buffer->readpos = 0;
			buffer->writepos = 0;
		}
	}
	return ret;
}

int libmqtt_buffer_fill(libmqtt_bufferhandle* buffer, size_t waiting,
		libmqtt_readfunc readfunc, void* userdata) {
	int ret = 0;
	if (buffer->writepos == 0) {
		size_t readsz =
				waiting < sizeof(buffer->buffer) ?
						waiting : sizeof(buffer->buffer);
		ret = readfunc(userdata, buffer->buffer, readsz);
		if (ret > 0)
			buffer->writepos = ret;
	}
	return ret;
}

int libmqtt_readpkt(libmqtt_packetread* pkt,
		libmqtt_packetreadchange changefunc, libmqtt_readfunc readfunc,
		void* readuserdata, libmqtt_writefunc payloadwritefunc,
		void* payloadwriteuserdata) {

	int ret = 0;

// reset a finished or error'd packet
	if (pkt->state >= LIBMQTT_PACKETREADSTATE_FINISHED)
		memset(pkt, 0, sizeof(*pkt));

	while (pkt->state < LIBMQTT_PACKETREADSTATE_FINISHED && ret >= 0) {
		//printf("s %d - %d %d %d\n", pkt->state, pkt->type, pkt->length,
		//		pkt->pos);
		switch (pkt->state) {
		case LIBMQTT_PACKETREADSTATE_TYPE: {
			uint8_t typeandflags;
			ret = libmqtt_readpkt_read(pkt, readfunc, readuserdata,
					&typeandflags, 1);
			if (ret == 1) {
				pkt->type = LIBMQTT_PACKETTYPEFROMPACKETTYPEANDFLAGS(
						typeandflags);

				// check for valid packet type
				if (pkt->type >= LIBMQTT_PACKETTYPE_CONNECT
						&& pkt->type <= LIBMQTT_PACKETTYPE_DISCONNECT) {
					pkt->flags = LIBMQTT_PACKETFLAGSFROMPACKETTYPEANDFLAGS(
							typeandflags);
					pkt->length = 1;
					pkt->lenmultiplier = 1;
					libmqtt_readpkt_changestate(pkt, changefunc,
							LIBMQTT_PACKETREADSTATE_LEN);
				} else
					libmqtt_readpkt_changestate(pkt, changefunc,
							LIBMQTT_PACKETREADSTATE_ERROR);
			}
		}
			break;
		case LIBMQTT_PACKETREADSTATE_LEN: {
			uint8_t lenbyte;
			ret = libmqtt_readpkt_read(pkt, readfunc, readuserdata, &lenbyte,
					1);
			if (ret == 1) {
				pkt->length += 1 + (LIBMQTT_LEN(lenbyte) * pkt->lenmultiplier);
				pkt->lenmultiplier *= 128;
				// last byte of the length has been read
				if (LIBMQTT_ISLASTLENBYTE(lenbyte)) {
					bool hasmessageid = (pkt->type == LIBMQTT_PACKETTYPE_PUBACK)
							|| (pkt->type == LIBMQTT_PACKETTYPE_PUBREC)
							|| (pkt->type == LIBMQTT_PACKETTYPE_PUBREL)
							|| (pkt->type == LIBMQTT_PACKETTYPE_PUBCOMP)
							|| (pkt->type == LIBMQTT_PACKETTYPE_SUBSCRIBE)
							|| (pkt->type == LIBMQTT_PACKETTYPE_SUBACK)
							|| (pkt->type == LIBMQTT_PACKETTYPE_UNSUBSCRIBE)
							|| (pkt->type == LIBMQTT_PACKETTYPE_UNSUBACK);

					// connacks has a flags byte and a return code byte
					if (pkt->type == LIBMQTT_PACKETTYPE_CONNACK)
						libmqtt_readpkt_changestate(pkt, changefunc,
								LIBMQTT_PACKETREADSTATE_CONNFLAGS);
					// publishes always have a topic
					else if (pkt->type == LIBMQTT_PACKETTYPE_PUBLISH)
						libmqtt_readpkt_changestate(pkt, changefunc,
								LIBMQTT_PACKETREADSTATE_TOPICLEN);
					// some packets have a message id
					else if (hasmessageid)
						libmqtt_readpkt_changestate(pkt, changefunc,
								LIBMQTT_PACKETREADSTATE_MSGID);
					else
						libmqtt_readpkt_changestate(pkt, changefunc,
								LIBMQTT_PACKETREADSTATE_PAYLOAD);
				}
			}
		}
			break;
		case LIBMQTT_PACKETREADSTATE_TOPICLEN: {
			uint8_t topiclenbyte;
			ret = libmqtt_readpkt_read(pkt, readfunc, readuserdata,
					&topiclenbyte, 1);
			if (ret == 1) {
				pkt->varhdr.publish.topiclen = (pkt->varhdr.publish.topiclen
						<< 8) | topiclenbyte;
				if (pkt->counter == 2)
					libmqtt_readpkt_changestate(pkt, changefunc,
							LIBMQTT_PACKETREADSTATE_TOPIC);
			};
		}
			break;
		case LIBMQTT_PACKETREADSTATE_TOPIC: {
			uint8_t topicbyte;
			ret = libmqtt_readpkt_read(pkt, readfunc, readuserdata, &topicbyte,
					1);
			if (ret == 1) {
				if (pkt->counter == pkt->varhdr.publish.topiclen) {
					bool publishhasmsgid = ((pkt->flags >> 1) & 0x3);
					libmqtt_readpkt_changestate(pkt, changefunc,
							publishhasmsgid ?
									LIBMQTT_PACKETREADSTATE_MSGID :
									LIBMQTT_PACKETREADSTATE_PAYLOAD);
				}
			};
		}
			break;
		case LIBMQTT_PACKETREADSTATE_MSGID: {
			uint8_t msgidbyte;
			ret = libmqtt_readpkt_read(pkt, readfunc, readuserdata, &msgidbyte,
					1);
			if (ret == 1) {
				pkt->varhdr.msgid = (pkt->varhdr.msgid << 8) | msgidbyte;
				if (pkt->counter == 2)
					libmqtt_readpkt_changestate(pkt, changefunc,
							LIBMQTT_PACKETREADSTATE_PAYLOAD);
			};

		}
			break;
		case LIBMQTT_PACKETREADSTATE_CONNFLAGS: {
			uint8_t flags;
			ret = libmqtt_readpkt_read(pkt, readfunc, readuserdata, &flags, 1);
			if (ret == 1) {
				pkt->varhdr.connack.ackflags = flags;
				libmqtt_readpkt_changestate(pkt, changefunc,
						LIBMQTT_PACKETREADSTATE_CONNRET);
			}
		}
			break;
		case LIBMQTT_PACKETREADSTATE_CONNRET: {
			uint8_t returncode;
			ret = libmqtt_readpkt_read(pkt, readfunc, readuserdata, &returncode,
					1);
			if (ret == 1) {
				pkt->varhdr.connack.returncode = returncode;
				libmqtt_readpkt_changestate(pkt, changefunc,
						LIBMQTT_PACKETREADSTATE_FINISHED);
			}
		}
			break;
		case LIBMQTT_PACKETREADSTATE_PAYLOAD: {
			// try to read
			size_t remaining = pkt->length - pkt->pos;
			ret = libmqtt_buffer_fill(&pkt->buffer, remaining, readfunc,
					readuserdata);
			if (ret < 0)
				break;

			// try to flush
			ret = libmqtt_buffer_flush(&pkt->buffer, payloadwritefunc,
					payloadwriteuserdata);
			if (ret > 0)
				libmqtt_readpkt_incposandcounter(pkt, ret);

			libmqtt_readpkt_changestate(pkt, changefunc,
					LIBMQTT_PACKETREADSTATE_PAYLOAD);
		}
			break;
		}

	}

	if (ret <= LIBMQTT_EFATAL)
		libmqtt_readpkt_changestate(pkt, changefunc,
				LIBMQTT_PACKETREADSTATE_ERROR);

	return ret;
}

