#include <string.h>

#include <stdio.h>

#include "libmqtt.h"

#include "emcuetiti_priv.h"
#include "emcuetiti.h"

void emcuetiti_client_register(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* handle) {
	if (broker->registeredclients < EMCUETITI_CONFIG_MAXCLIENTS) {
		broker->registeredclients++;
		// search for a free slot to put this client
		for (int i = 0; i < ARRAY_ELEMENTS(broker->clients); i++) {
			if (broker->clients[i].client == NULL) {
				emcuetiti_clientstate* cs = broker->clients + i;
				cs->client = handle;
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
				broker->subscribedtopics -= cs->subscriptions;

				for (int j = 0; j < ARRAY_ELEMENTS(broker->subscriptions);
						j++) {
					if (cs->client == handle) {
						broker->subscriptions[j].client = NULL;
						broker->subscriptions[j].topic = NULL;
						cs->subscriptions--;
						if (cs->subscriptions == 0)
							break;
					}
				}

				break;
			}
		}
	}
}

static void emcuetiti_poll_read(emcuetiti_clientstate* cs) {
	uint8_t* offsetbuffer = cs->buffer + cs->bufferpos;
	void* userdata = cs->client->userdata;
	int read;
	switch (cs->readstate) {
	case CLIENTREADSTATE_IDLE:
		if (cs->client->readytoread(cs->client->userdata)) {
			cs->readstate = CLIENTREADSTATE_TYPE;
			cs->bufferpos = 0;
		}
		break;
	case CLIENTREADSTATE_TYPE:
		if (cs->client->readfunc(userdata, &(cs->packettype), 0, 1) == 1) {
			cs->readstate = CLIENTREADSTATE_REMAININGLEN;
		}
		break;
	case CLIENTREADSTATE_REMAININGLEN:
		if (cs->client->readfunc(userdata, offsetbuffer, 0, 1) == 1) {
			if ((*offsetbuffer & (1 << 7)) == 0) {
				libmqtt_decodelength(cs->buffer, &cs->remainingbytes);
				if (cs->remainingbytes > 0)
					cs->readstate = CLIENTREADSTATE_PAYLOAD;
				else
					cs->readstate = CLIENTREADSTATE_COMPLETE;
			}
			cs->bufferpos = 0;
		}
		break;
	case CLIENTREADSTATE_PAYLOAD:
		read = cs->client->readfunc(userdata, offsetbuffer, 0,
				cs->remainingbytes);
		if (read > 0) {
			cs->remainingbytes -= read;
			if (cs->remainingbytes == 0)
				cs->readstate = CLIENTREADSTATE_COMPLETE;
		}
		break;
	}
}

static void emcuetiti_handleinboundpacket(emcuetiti_clientstate* cs) {
	uint8_t packetype = LIBMQTT_PACKETTYPEFROMPACKETTYPEANDFLAGS(
			cs->packettype);
	libmqtt_writefunc writefunc = cs->client->writefunc;

	uint8_t returncodes[] = { 0 };

	void* userdata = cs->client->userdata;
	switch (packetype) {
	case LIBMQTT_PACKETTYPE_CONNECT:
		libmqtt_construct_connack(writefunc, userdata);
		break;
	case LIBMQTT_PACKETTYPE_SUBSCRIBE:
		libmqtt_construct_suback(writefunc, userdata, returncodes, 1);
		break;
	case LIBMQTT_PACKETTYPE_PUBLISH:

		break;
	}
	cs->readstate = CLIENTREADSTATE_IDLE;
}

void emcuetiti_poll(emcuetiti_brokerhandle* broker) {
	if (broker->registeredclients > 0) {
		for (int i = 0; i < ARRAY_ELEMENTS(broker->clients); i++) {
			emcuetiti_clientstate* cs = broker->clients + i;
			if (cs->client != NULL) {
				emcuetiti_poll_read(cs);
				if (cs->readstate == CLIENTREADSTATE_COMPLETE)
					emcuetiti_handleinboundpacket(cs);
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

void emcuetiti_addtopicpart(emcuetiti_brokerhandle* broker,
		emcuetiti_topichandle* root, emcuetiti_topichandle* part,
		const char* topicpart) {

	part->topicpart = topicpart;
	part->sibling = NULL;
	part->child = NULL;
	part->parent = NULL;

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

void emcuetiti_init(emcuetiti_brokerhandle* broker) {
	broker->root = NULL;
	broker->registeredclients = 0;
	broker->subscribedtopics = 0;
	memset(broker->clients, 0, sizeof(broker->clients));
	memset(broker->subscriptions, 0, sizeof(broker->subscriptions));
}

static void emcuetiti_dumpstate_printtopic(emcuetiti_topichandle* node) {
	bool first = node->parent == NULL;
	if (!first) {
		emcuetiti_dumpstate_printtopic(node->parent);
		printf("/");
	}

	printf("%s", node->topicpart);
}

static void emcuetiti_dumpstate_child(emcuetiti_topichandle* node) {
	for (; node != NULL; node = node->sibling) {
		if (node->child != NULL)
			emcuetiti_dumpstate_child(node->child);
		else {
			emcuetiti_dumpstate_printtopic(node);
			printf("\n");
		}
	}
}

void emcuetiti_dumpstate(emcuetiti_brokerhandle* broker) {
	printf("Topic hierachy\n");
	emcuetiti_topichandle* th = broker->root;
	emcuetiti_dumpstate_child(th);
}
