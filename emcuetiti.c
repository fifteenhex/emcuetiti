#include <string.h>

#include <stdio.h>

#include "libmqtt.h"

#include "emcuetiti_priv.h"
#include "emcuetiti.h"

static size_t min(size_t a, size_t b) {
	if (a < b)
		return a;
	else
		return b;
}

void emcuetiti_client_register(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* handle) {
	if (broker->registeredclients < EMCUETITI_CONFIG_MAXCLIENTS) {
		broker->registeredclients++;
		// search for a free slot to put this client
		for (int i = 0; i < ARRAY_ELEMENTS(broker->clients); i++) {
			if (broker->clients[i].client == NULL) {
				emcuetiti_clientstate* cs = broker->clients + i;
				cs->client = handle;
				cs->state = CLIENTSTATE_NEW;
				cs->readstate = CLIENTREADSTATE_IDLE;
				break;
			}
		}
	}
}

void emcuetiti_client_unregister(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* handle) {
	if (broker->registeredclients > 0) {
		broker->registeredclients--;
		for (int i = 0; i < ARRAY_ELEMENTS(broker->clients); i++) {
			if (broker->clients[i].client == handle) {
				emcuetiti_clientstate* cs = broker->clients + i;

				// clear the client and it's subscriptions
				cs->client = NULL;
				break;
			}
		}
	}
}

int emcuetiti_client_readpublish(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* client, uint8_t* buffer, size_t len) {

	emcuetiti_clientstate* cs;
	for (int i = 0; i < ARRAY_ELEMENTS(broker->clients); i++) {
		if (broker->clients[i].client == client) {
			cs = &(broker->clients[i]);
		}
	}

	len = min(len, cs->publishpayloadlen);

	memcpy(buffer, cs->buffer + cs->bufferpos, len);

	cs->readstate = CLIENTREADSTATE_IDLE;

	return len;
}

static void emcuetiti_poll_read(emcuetiti_clientstate* cs) {
	uint8_t* offsetbuffer = cs->buffer + cs->bufferpos;
	void* userdata = cs->client->userdata;
	int read;
	switch (cs->readstate) {
	case CLIENTREADSTATE_IDLE:
		if (cs->client->ops->readytoread(cs->client->userdata)) {
			cs->readstate = CLIENTREADSTATE_TYPE;
			cs->bufferpos = 0;
		}
		break;
	case CLIENTREADSTATE_TYPE:
		if (cs->client->ops->readfunc(userdata, &(cs->packettype), 0, 1) == 1) {
			cs->readstate = CLIENTREADSTATE_REMAININGLEN;
		}
		break;
	case CLIENTREADSTATE_REMAININGLEN:
		if (cs->client->ops->readfunc(userdata, offsetbuffer, 0, 1) == 1) {
			if ((*offsetbuffer & (1 << 7)) == 0) {
				libmqtt_decodelength(cs->buffer, &cs->varheaderandpayloadlen);
				if (cs->varheaderandpayloadlen > 0) {
					cs->readstate = CLIENTREADSTATE_PAYLOAD;
					cs->remainingbytes = cs->varheaderandpayloadlen;
				} else
					cs->readstate = CLIENTREADSTATE_COMPLETE;
			}
			cs->bufferpos = 0;
		}
		break;
	case CLIENTREADSTATE_PAYLOAD:
		read = cs->client->ops->readfunc(userdata, offsetbuffer, 0,
				cs->remainingbytes);
		if (read > 0) {
			cs->remainingbytes -= read;
			if (cs->remainingbytes == 0)
				cs->readstate = CLIENTREADSTATE_COMPLETE;
		}
		break;
	}
}

static emcuetiti_topichandle* emcuetiti_findtopic(
		emcuetiti_brokerhandle* broker, emcuetiti_topichandle* root,
		const char* topicpart) {

	printf("looking for %s\n", topicpart);
	if (root == NULL)
		root = broker->root;
	else
		root = root->child;

	for (; root != NULL; root = root->sibling) {
		if (strcmp(root->topicpart, topicpart) == 0) {
			printf("found %s\n", topicpart);
			return root;
		} else
			printf("not %s\n", root->topicpart);
	}

	return NULL;
}

static libmqtt_writefunc emcuetiti_resolvewritefunc(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs) {
	libmqtt_writefunc writefunc = broker->callbacks->writefunc;
#if EMCUETITI_CONFIG_PERCLIENTCALLBACK_WRITE
	if(cs->client->ops->writefunc != NULL)
	writefunc = cs->client->ops->writefunc;
#endif
	return writefunc;
}

static void emcuetiti_handleinboundpacket_pingreq(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs) {
	printf("pingreq from %s\n", cs->clientid);
	libmqtt_construct_pingresp(emcuetiti_resolvewritefunc(broker, cs),
			cs->client->userdata);
	cs->readstate = CLIENTREADSTATE_IDLE;
}

static emcuetiti_topichandle* emcuetiti_readtopicstringandfindtopic(
		emcuetiti_brokerhandle* broker, uint8_t* buffer, uint16_t* topiclen) {

	int topicpartpos = 0;
	char topicpart[32];

	uint16_t len = (*(buffer++) << 8) | *(buffer++);

	printf("part len is %d\n", len);

	emcuetiti_topichandle* t = NULL;
	for (uint16_t i = 0; i < len; i++) {
		uint8_t byte = *(buffer++);
		if (i + 1 == len) {
			topicpart[topicpartpos++] = byte;
			topicpart[topicpartpos] = '\0';
			printf("%s\n", topicpart);
			t = emcuetiti_findtopic(broker, t, topicpart);
		} else if (byte == '/') {
			topicpart[topicpartpos] = '\0';
			topicpartpos = 0;
			printf("%s\n", topicpart);
			t = emcuetiti_findtopic(broker, t, topicpart);
		} else
			topicpart[topicpartpos++] = byte;

	}

	if (topiclen != NULL)
		*topiclen = len;

	return t;
}

static void emcuetiti_disconnectclient(emcuetiti_brokerhandle* broker,
		emcuetiti_clientstate* cs) {
#if EMCUETITI_CONFIG_PERCLIENTCALLBACK_DISCONNECT
	if (cs->client->ops->disconnectfunc != NULL)
	cs->client->ops->disconnectfunc(cs->client, cs->client->userdata);
	else {
#endif
	if (broker->callbacks->disconnectfunc != NULL) {
		broker->callbacks->disconnectfunc(cs->client, cs->client->userdata);
	}
#if EMCUETITI_CONFIG_PERCLIENTCALLBACK_DISCONNECT
}
#endif

	cs->state = CLIENTSTATE_DISCONNECTED;
	cs->readstate = CLIENTREADSTATE_IDLE;
}

static void emcuetiti_handleinboundpacket_publish(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs,
		uint8_t flags) {
	printf("handling publish\n");

	libmqtt_qos qos = (flags >> 1) & 0x3;
	uint16_t messageid;

	uint16_t topiclen;
	emcuetiti_topichandle* t = emcuetiti_readtopicstringandfindtopic(broker,
			cs->buffer, &topiclen);

	if (t != NULL) {
		cs->publishtopic = t;
		cs->publishpayloadlen = cs->varheaderandpayloadlen - (topiclen + 2);
		cs->readstate = CLIENTREADSTATE_PUBLISHREADY;
		cs->bufferpos = topiclen + 2;

		if (qos > LIBMQTT_QOS0_ATMOSTONCE) {
			messageid = (cs->buffer[cs->bufferpos] << 8)
					| cs->buffer[cs->bufferpos + 1];
			cs->bufferpos += 2;
		}

		if (broker->callbacks->publishreadycallback != NULL)
			broker->callbacks->publishreadycallback(cs->client,
					cs->publishpayloadlen);

		if (qos > LIBMQTT_QOS0_ATMOSTONCE) {
			libmqtt_construct_puback(emcuetiti_resolvewritefunc(broker, cs),
					cs->client->userdata, messageid);
		}
	} else {
		printf("publish to invalid topic\n");
		emcuetiti_disconnectclient(broker, cs);
	}
}

static void emcuetiti_handleinboardpacket_connect(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs) {
	void* userdata = cs->client->userdata;

	uint8_t* buff = cs->buffer;
	uint16_t protnamelen = (*(buff++) << 8) | *(buff++);
	char protocolname[5];
	memcpy(protocolname, buff, protnamelen);
	protocolname[protnamelen] = '\0';
	buff += protnamelen;

	uint8_t level = *(buff++);
	uint8_t flags = *(buff++);

	cs->keepalive = (*(buff++) << 8) | *(buff++);

	uint16_t idlen = (*(buff++) << 8) | *(buff++);
	memcpy(cs->clientid, buff, idlen);
	cs->clientid[idlen] = '\0';
	buff += idlen;

	if (flags & LIBMQTT_CONNECTFLAG_WILL) {

	}

	if (flags & LIBMQTT_CONNECTFLAG_USERNAME) {

	}

	if (flags & LIBMQTT_CONNECTFLAG_PASSWORD) {

	}

	printf("protoname %s, level %d, keepalive %d, clientid %s \n", protocolname,
			(int) level, (int) cs->keepalive, cs->clientid);

	if (broker->callbacks->authenticatecallback == NULL
			|| broker->callbacks->authenticatecallback(cs->clientid)) {
		libmqtt_construct_connack(emcuetiti_resolvewritefunc(broker, cs),
				userdata);
	}
	cs->readstate = CLIENTREADSTATE_IDLE;
}

static void emcuetiti_handleinboundpacket_subscribe(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs) {
	void* userdata = cs->client->userdata;
	uint8_t returncodes[] = { 0 };

	uint8_t* buffer = cs->buffer;
	uint16_t messageid = (*(buffer++) << 8) | *(buffer++);

	uint16_t topiclen;
	emcuetiti_topichandle* t = emcuetiti_readtopicstringandfindtopic(broker,
			buffer, &topiclen);
	buffer += 2 + topiclen;

	uint8_t qos = *(buffer++);

	printf("sub from %s, messageid %d, qos %d\n", cs->clientid, (int) messageid,
			(int) qos);

	if (t == NULL) {
		printf("client tried to sub to nonexisting topic\n");
		returncodes[0] = LIBMQTT_SUBSCRIBERETURNCODE_FAILURE;
	} else {
		cs->subscriptions[cs->numsubscriptions++] = t;
	}

	libmqtt_construct_suback(emcuetiti_resolvewritefunc(broker, cs), userdata,
			messageid, returncodes, 1);
	cs->readstate = CLIENTREADSTATE_IDLE;
}

static void emcuetiti_handleinboundpacket_unsubscribe(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs) {
	void* userdata = cs->client->userdata;

	uint8_t* buffer = cs->buffer;
	uint16_t messageid = (*(buffer++) << 8) | *(buffer++);

	uint16_t topiclen;
	emcuetiti_topichandle* t = emcuetiti_readtopicstringandfindtopic(broker,
			buffer, &topiclen);
	buffer += 2 + topiclen;

	uint8_t qos = *(buffer++);

	printf("unsub from %s, messageid %d, qos %d\n", cs->clientid,
			(int) messageid, (int) qos);

	if (t == NULL) {
		printf("client tried to unsub from nonexisting topic\n");
	} else {
		//cs->subscriptions[cs->numsubscriptions++] = t;
	}

	libmqtt_construct_unsuback(emcuetiti_resolvewritefunc(broker, cs), userdata,
			messageid);
	cs->readstate = CLIENTREADSTATE_IDLE;
}

static void emcuetiti_handleinboundpacket_disconnect(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs) {
	printf("client %s has requested to disconnect\n", cs->clientid);
	emcuetiti_disconnectclient(broker, cs);
}

static void emcuetiti_handleinboundpacket(emcuetiti_brokerhandle* broker,
		emcuetiti_clientstate* cs) {
	uint8_t packettype = LIBMQTT_PACKETTYPEFROMPACKETTYPEANDFLAGS(
			cs->packettype);
	uint8_t packetflags = LIBMQTT_PACKETFLAGSFROMPACKETTYPEANDFLAGS(
			cs->packettype);

	switch (packettype) {
	case LIBMQTT_PACKETTYPE_CONNECT:
		emcuetiti_handleinboardpacket_connect(broker, cs);
		break;
	case LIBMQTT_PACKETTYPE_SUBSCRIBE:
		emcuetiti_handleinboundpacket_subscribe(broker, cs);
		break;
	case LIBMQTT_PACKETTYPE_UNSUBSCRIBE:
		emcuetiti_handleinboundpacket_unsubscribe(broker, cs);
		break;
	case LIBMQTT_PACKETTYPE_PUBLISH:
		emcuetiti_handleinboundpacket_publish(broker, cs, packetflags);
		break;
	case LIBMQTT_PACKETTYPE_PINGREQ:
		emcuetiti_handleinboundpacket_pingreq(broker, cs);
		break;
	case LIBMQTT_PACKETTYPE_DISCONNECT:
		emcuetiti_handleinboundpacket_disconnect(broker, cs);
		break;
	default:
		printf("unhandled packet type %d from client %s\n", (int) packettype,
				cs->clientid);
		break;
	}

}

void emcuetiti_broker_poll(emcuetiti_brokerhandle* broker) {
	EMCUETITI_CONFIG_TIMESTAMPTYPE now = broker->callbacks->timestamp();
	if (broker->registeredclients > 0) {
		for (int i = 0; i < ARRAY_ELEMENTS(broker->clients); i++) {
			emcuetiti_clientstate* cs = broker->clients + i;
			if (cs->client != NULL) {
				switch (cs->state) {
				case CLIENTSTATE_NEW:
				case CLIENTSTATE_CONNECTED:
					if (cs->client->ops->isconnectedfunc(
							cs->client->userdata)) {
						emcuetiti_poll_read(cs);
						if (cs->readstate == CLIENTREADSTATE_COMPLETE)
							emcuetiti_handleinboundpacket(broker, cs);
					} else {
						printf("client %s disconnected\n", cs->clientid);
					}
					break;
				}
			}
		}
	}
}

static emcuetiti_topichandle* emcuetiti_findparent(
		emcuetiti_topichandle* sibling) {
	for (; sibling->sibling != NULL; sibling = sibling->sibling) {

	}
#ifdef EMCUETITI_CONFIG_DEBUG
	printf("attaching to %s\n", sibling->topicpart);
#endif
	return sibling;
}

void emcuetiti_broker_addtopicpart(emcuetiti_brokerhandle* broker,
		emcuetiti_topichandle* root, emcuetiti_topichandle* part,
		const char* topicpart, bool targetable) {

	// clear pointers
	part->sibling = NULL;
	part->child = NULL;
	part->parent = NULL;

	part->topicpart = topicpart;
	part->targetable = targetable;

	if (root == NULL) {
		if (broker->root == NULL)
			broker->root = part;
		else {
			emcuetiti_topichandle* parent = emcuetiti_findparent(broker->root);
			parent->sibling = part;
		}
	} else {
		// if this root doesn't have a child yet become that child
		if (root->child == NULL)
			root->child = part;
		else {
			emcuetiti_topichandle* parent = emcuetiti_findparent(root->child);
			parent->sibling = part;
		}
		part->parent = root;
	}
}

void emcuetiti_broker_init(emcuetiti_brokerhandle* broker) {
	// clear out state
	broker->root = NULL;
	broker->registeredclients = 0;
	memset(broker->clients, 0, sizeof(broker->clients));
}

static void emcuetiti_broker_dumpstate_printtopic(emcuetiti_topichandle* node) {
	bool first = node->parent == NULL;
	if (!first) {
		emcuetiti_broker_dumpstate_printtopic(node->parent);
		printf("/");
	}

	printf("%s", node->topicpart);
}

static void emcuetiti_broker_dumpstate_child(emcuetiti_topichandle* node) {
	for (; node != NULL; node = node->sibling) {
		if (node->child != NULL)
			emcuetiti_broker_dumpstate_child(node->child);
		else {
			emcuetiti_broker_dumpstate_printtopic(node);
			printf("\n");
		}
	}
}

void emcuetiti_broker_dumpstate(emcuetiti_brokerhandle* broker) {
	printf("--Topic hierachy--\n");
	emcuetiti_topichandle* th = broker->root;
	emcuetiti_broker_dumpstate_child(th);

	printf("-- Clients --\n");
	for (int i = 0; i < ARRAY_ELEMENTS(broker->clients); i++) {

	}
}
