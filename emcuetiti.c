#include <string.h>

#include <stdio.h>

#include "libmqtt.h"
#include "libmqtt_priv.h"

#include "emcuetiti_priv.h"
#include "emcuetiti.h"

static void emcuetiti_broker_dumpstate_printtopic(emcuetiti_topichandle* node) {
	bool first = node->parent == NULL;
	if (!first) {
		emcuetiti_broker_dumpstate_printtopic(node->parent);
		printf("/");
	}
	printf("%s", node->topicpart);
}

static size_t min(size_t a, size_t b) {
	if (a < b)
		return a;
	else
		return b;
}

int emcuetiti_client_register(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* handle) {

	int ret = 0;

	if (broker->registeredclients < EMCUETITI_CONFIG_MAXCLIENTS) {
		// search for a free slot to put this client
		for (int i = 0; i < ARRAY_ELEMENTS(broker->clients); i++) {
			if (broker->clients[i].client == NULL) {
				emcuetiti_clientstate* cs = broker->clients + i;
				cs->client = handle;
				cs->state = CLIENTSTATE_NEW;
				cs->readstate = CLIENTREADSTATE_IDLE;
				cs->lastseen = broker->callbacks->timestamp();

				memset(cs->subscriptions, 0, sizeof(cs->subscriptions));
				cs->numsubscriptions = 0;

				broker->registeredclients++;
				printf("registered client, now have %d clients\n",
						broker->registeredclients);

				break;
			}
		}
	} else
		ret = EMCUETITI_ERROR_NOMORECLIENTS;

	return ret;
}

void emcuetiti_client_unregister(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* handle) {
	if (broker->registeredclients > 0) {
		bool unregistered = false;
		for (int i = 0; i < ARRAY_ELEMENTS(broker->clients); i++) {
			emcuetiti_clientstate* cs = &(broker->clients[i]);
			if (cs->client == handle) {
				// clear the client and it's subscriptions
				cs->client = NULL;
				unregistered = true;
				break;
			}
		}

		if (unregistered) {
			broker->registeredclients--;
			printf("unregistered client, %d clients left\n",
					broker->registeredclients);
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

static void emcuetiti_poll_read(emcuetiti_clientstate* cs,
EMCUETITI_CONFIG_TIMESTAMPTYPE now) {

	int bufferremaining = EMCUETITI_CONFIG_CLIENTBUFFERSZ - cs->bufferpos;

	if (bufferremaining == 0) {
		printf("client buffer is full\n");
		return;
	}

	uint8_t* offsetbuffer = cs->buffer + cs->bufferpos;
	void* userdata = cs->client->userdata;
	int read;

	emcuetiti_readfunc readfunc = cs->client->ops->readfunc;

	switch (cs->readstate) {
	case CLIENTREADSTATE_IDLE:
		if (cs->client->ops->readytoread(cs->client->userdata)) {
			cs->readstate = CLIENTREADSTATE_TYPE;
			cs->bufferpos = 0;
		}
		break;
	case CLIENTREADSTATE_TYPE:
		if (readfunc(userdata, &(cs->packettype), 1) == 1) {
			cs->readstate = CLIENTREADSTATE_REMAININGLEN;
		}
		break;
	case CLIENTREADSTATE_REMAININGLEN:
		if (readfunc(userdata, offsetbuffer, 1) == 1) {
			if (LIBMQTT_ISLASTLENBYTE(*offsetbuffer)) {
				libmqtt_decodelength(cs->buffer, &cs->varheaderandpayloadlen);
				if (cs->varheaderandpayloadlen > 0) {
					cs->readstate = CLIENTREADSTATE_PAYLOAD;
					cs->remainingbytes = cs->varheaderandpayloadlen;
				} else
					cs->readstate = CLIENTREADSTATE_COMPLETE;

				printf("%d bytes remaining\n", cs->remainingbytes);
				// length is stashed, reset buffer
				cs->bufferpos = 0;
			} else
				cs->bufferpos++;
		}
		break;
	case CLIENTREADSTATE_PAYLOAD:
		if (cs->remainingbytes > bufferremaining) {
			printf("not enough space in buffer for remaining bytes\n");
		} else {
			read = readfunc(userdata, offsetbuffer, cs->remainingbytes);
			if (read > 0) {
				cs->remainingbytes -= read;
				if (cs->remainingbytes == 0) {
					cs->readstate = CLIENTREADSTATE_COMPLETE;
					cs->lastseen = now;
				}
			}
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
		emcuetiti_brokerhandle* broker, uint8_t* buffer, uint16_t* topiclen,
		emcuetiti_subscription_level* level) {

	int topicpartpos = 0;
	char topicpart[32];

	uint16_t len = (*(buffer++) << 8) | *(buffer++);
	emcuetiti_subscription_level sublevel = ONLYTHIS;

	printf("part len is %d\n", len);

	emcuetiti_topichandle* t = NULL;
	for (uint16_t i = 0; i < len; i++) {
		uint8_t byte = *(buffer++);
		if (i + 1 == len) {
			topicpart[topicpartpos++] = byte;
			topicpart[topicpartpos] = '\0';

			if (strcmp(topicpart, "#") == 0) {
				printf("have multilevel wildcard\n");
				sublevel = THISANDABOVE;
			} else {
				printf("%s\n", topicpart);
				t = emcuetiti_findtopic(broker, t, topicpart);
			}
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

	if (level != NULL)
		*level = sublevel;

	return t;
}

static void emcuetiti_disconnectclient(emcuetiti_brokerhandle* broker,
		emcuetiti_clientstate* cs) {
#if EMCUETITI_CONFIG_PERCLIENTCALLBACK_DISCONNECT
	if (cs->client->ops->disconnectfunc != NULL)
	cs->client->ops->disconnectfunc(broker, cs->client);
	else {
#endif
	if (broker->callbacks->disconnectfunc != NULL) {
		broker->callbacks->disconnectfunc(broker, cs->client);
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
			cs->buffer, &topiclen, NULL);

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

		for (int p = 0; p < ARRAY_ELEMENTS(broker->ports); p++) {
			if (broker->ports[p] != NULL) {
				printf("dispatching publish to port %d\n", p);
				if (broker->ports[p]->publishreadycallback != NULL)
					broker->ports[p]->publishreadycallback(broker, cs->client,
							t, cs->publishpayloadlen);
			}
		}

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

	if (flags & LIBMQTT_FLAGS_CONNECT_WILLFLAG) {

	}

	if (flags & LIBMQTT_FLAGS_CONNECT_USERNAMEFLAG) {

	}

	if (flags & LIBMQTT_FLAGS_CONNECT_PASSWORDFLAG) {

	}

	printf("protoname %s, level %d, keepalive %d, clientid %s \n", protocolname,
			(int) level, (int) cs->keepalive, cs->clientid);

	if (broker->callbacks->authenticatecallback == NULL
			|| broker->callbacks->authenticatecallback(cs->clientid)) {
		libmqtt_construct_connack(emcuetiti_resolvewritefunc(broker, cs),
				userdata);
	}
	cs->state = CLIENTSTATE_CONNECTED;
	cs->readstate = CLIENTREADSTATE_IDLE;
}

static void emcuetiti_handleinboundpacket_subscribe(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs) {
	void* userdata = cs->client->userdata;
	uint8_t returncodes[] = { LIBMQTT_SUBSCRIBERETURNCODE_FAILURE };

	uint8_t* buffer = cs->buffer;
	uint8_t* bufferend = cs->buffer + cs->varheaderandpayloadlen;

	uint16_t messageid = (*(buffer++) << 8) | *(buffer++);

	uint16_t topiclen;
	emcuetiti_subscription_level level;

	emcuetiti_topichandle* t = emcuetiti_readtopicstringandfindtopic(broker,
			buffer, &topiclen, &level);
	buffer += 2 + topiclen;
	uint8_t qos = *(buffer++);

	if (buffer != bufferend)
		printf("probably have more topics\n");

	printf("sub from %s, messageid %d, qos %d\n", cs->clientid, (int) messageid,
			(int) qos);

	bool qosisvalid = qos >= LIBMQTT_QOS0_ATMOSTONCE
			&& qos <= LIBMQTT_QOS2_EXACTLYONCE;
	if (!qosisvalid)
		printf("client %s requested invalid qos in subreq\n", cs->clientid);

	if (qosisvalid && t != NULL) {
		bool added = false;
		for (int i = 0; i < ARRAY_ELEMENTS(cs->subscriptions); i++) {
			if (cs->subscriptions[i].topic == NULL) {
				cs->subscriptions[i].topic = t;
				cs->subscriptions[i].qos = qos;
				cs->subscriptions[i].level = level;
				cs->numsubscriptions++;
				returncodes[0] = 0;
				break;
			}
		}
	}

	libmqtt_construct_suback(emcuetiti_resolvewritefunc(broker, cs), userdata,
			messageid, returncodes, 1);

	cs->readstate = CLIENTREADSTATE_IDLE;
}

static void emcuetiti_handleinboundpacket_unsubscribe(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs) {
	void* userdata = cs->client->userdata;

	uint8_t* buffer = cs->buffer;
	uint8_t* bufferend = cs->buffer + cs->varheaderandpayloadlen;

	uint16_t messageid = (*(buffer++) << 8) | *(buffer++);

	uint16_t topiclen;
	emcuetiti_subscription_level level;

	emcuetiti_topichandle* t = emcuetiti_readtopicstringandfindtopic(broker,
			buffer, &topiclen, &level);
	buffer += 2 + topiclen;

	if (buffer != bufferend)
		printf("probably more topics\n");

	printf("unsub from %s, messageid %d\n", cs->clientid, (int) messageid);

	if (t == NULL) {
		printf("client tried to unsub from nonexisting topic\n");
	} else {

		for (int i = 0; i < ARRAY_ELEMENTS(cs->subscriptions); i++) {
			if (cs->subscriptions[i].topic == t) {
				cs->subscriptions[i].topic = NULL;
				cs->numsubscriptions--;
				break;
			}
		}
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

static void emcuetiti_broker_poll_checkkeepalive(emcuetiti_brokerhandle* broker,
		emcuetiti_clientstate* client,
		EMCUETITI_CONFIG_TIMESTAMPTYPE now) {

	uint16_t keepalive = EMCUETITI_CONFIG_DEFAULTKEEPALIVE;
	if (client->keepalive != 0)
		keepalive = client->keepalive;

	EMCUETITI_CONFIG_TIMESTAMPTYPE expires = client->lastseen + keepalive;

	if (expires > now) {
		EMCUETITI_CONFIG_TIMESTAMPTYPE timebeforeexpiry = expires - now;
#if EMCUETITI_CONFIG_DEBUG_KEEPALIVE
		printf("client %s expires in %ds\n", client->clientid,
				timebeforeexpiry);
#endif
	} else {
		printf("client %s has expired\n", client->clientid);
		emcuetiti_disconnectclient(broker, client);
	}
}

void emcuetiti_broker_poll(emcuetiti_brokerhandle* broker) {
	EMCUETITI_CONFIG_TIMESTAMPTYPE now = broker->callbacks->timestamp();

	emcuetiti_port_poll(broker, now);

	if (broker->registeredclients > 0) {
		for (int i = 0; i < ARRAY_ELEMENTS(broker->clients); i++) {
			emcuetiti_clientstate* cs = broker->clients + i;
			if (cs->client != NULL) {
				switch (cs->state) {
				case CLIENTSTATE_NEW:
				case CLIENTSTATE_CONNECTED:

					if (cs->client->ops->isconnectedfunc(
							cs->client->userdata)) {

						switch (cs->readstate) {
						case CLIENTREADSTATE_COMPLETE:
							emcuetiti_handleinboundpacket(broker, cs);
							break;
						default:
							emcuetiti_poll_read(cs, now);
							break;
						}

						emcuetiti_broker_poll_checkkeepalive(broker, cs, now);

					} else
						emcuetiti_disconnectclient(broker, cs);

					break;
				}
			}
		}
	}
}

static int emcuetiti_topic_topichandlewrite(libmqtt_writefunc writefunc,
		void *writefuncuserdata, void* userdata) {

	int len = 0;
	emcuetiti_topichandle* node = (emcuetiti_topichandle*) userdata;

	bool first = node->parent == NULL;
	if (!first) {
		len += emcuetiti_topic_topichandlewrite(writefunc, writefuncuserdata,
				node->parent) + 1;
		writefunc(writefuncuserdata, "/", 1);
	}

	writefunc(writefuncuserdata, node->topicpart, node->topicpartln);
	return len;
}

static int emcuetiti_topic_len(emcuetiti_topichandle* node) {
	int len = 0;
	bool first = node->parent == NULL;
	if (!first) {
		len += (emcuetiti_topic_len(node->parent) + 1);
	}
	return len + node->topicpartln;
}

void emcuetiti_broker_publish(emcuetiti_brokerhandle* broker,
		emcuetiti_publish* publish) {

	printf("outgoing publish to ");
	emcuetiti_broker_dumpstate_printtopic(publish->topic);
	printf("\n");

	for (int c = 0; c < ARRAY_ELEMENTS(broker->clients); c++) {

		emcuetiti_clientstate* cs = &broker->clients[c];

		if (cs->state == CLIENTSTATE_CONNECTED) {

			bool subbed = false;
			for (int i = 0; i < ARRAY_ELEMENTS(cs->subscriptions); i++) {
				emcuetiti_subscription* sub = &(cs->subscriptions[i]);
				emcuetiti_topichandle* topic = sub->topic;
				if (topic != NULL) {
					if (topic == publish->topic) {
						subbed = true;
					} else if (sub->level == THISANDABOVE) {
						printf(
								"looking for wildcard subscription below this level\n");
						emcuetiti_topichandle* t = publish->topic;
						while (t->parent != NULL) {
							t = t->parent;
							if (t == topic) {
								printf("found wild card sub below target\n");
								subbed = true;
								break;
							}
						}
					}

					if (subbed)
						break;
				}
			}

			if (subbed) {
				printf("sending publish to %s\n", cs->clientid);

				libmqtt_writefunc clientwritefunc = emcuetiti_resolvewritefunc(
						broker, &broker->clients[c]);

				libmqtt_construct_publish(clientwritefunc, //
						broker->clients[c].client->userdata, //
						publish->readfunc, //
						publish->userdata, //
						emcuetiti_topic_topichandlewrite, //
						publish->topic, //
						emcuetiti_topic_len(publish->topic), //
						publish->payloadln, //
						LIBMQTT_QOS0_ATMOSTONCE, //
						false, //
						false, 0);

				if (publish->resetfunc != NULL)
					publish->resetfunc(publish->userdata);
			}
		}
	}

	printf("finished\n");

//if (publish->freefunc != NULL)
//	publish->freefunc(publish->userdata);
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
	part->topicpartln = strlen(topicpart);
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
	memset(broker->ports, 0, sizeof(broker->ports));
	memset(broker->clients, 0, sizeof(broker->clients));
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

void emcuetiti_broker_dumpstate_client(emcuetiti_brokerhandle* broker,
		emcuetiti_clientstate* client) {

	char* state = "invalid";
	switch (client->state) {
	case CLIENTSTATE_NEW:
		state = "new";
		break;
	case CLIENTSTATE_CONNECTED:
		state = "connected";
		break;
	case CLIENTSTATE_DISCONNECTED:
		state = "disconnected";
		break;
	}

	if (client->client != NULL)
		printf("%s\t%s\t%d - subs %d\n", client->clientid, state,
				(int) client->readstate, (int) client->numsubscriptions);
}

void emcuetiti_broker_dumpstate(emcuetiti_brokerhandle* broker) {
	printf("-- Topic hierachy --\n");
	emcuetiti_topichandle* th = broker->root;
	emcuetiti_broker_dumpstate_child(th);

	printf("-- Clients --\n");
	for (int i = 0; i < ARRAY_ELEMENTS(broker->clients); i++) {
		emcuetiti_clientstate* cs = &(broker->clients[i]);
		emcuetiti_broker_dumpstate_client(broker, cs);
	}
}

bool emcuetiti_broker_canacceptmoreclients(emcuetiti_brokerhandle* broker) {
	return broker->registeredclients < EMCUETITI_CONFIG_MAXCLIENTS;
}
