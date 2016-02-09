#include <string.h>

#include "libmqtt.h"

#include "emcuetiti_config.h"
#include "emcuetiti_priv.h"
#include "emcuetiti.h"

static unsigned registeredclients;
static unsigned subscribedtopics;
static emcuetiti_topichandle* root;
static emcuetiti_clientstate clients[EMCUETITI_CONFIG_MAXCLIENTS];
static emcuetiti_subscriptionhandle subscriptions[EMCUETITI_CONFIG_MAXCLIENTS
		* EMCUETITI_CONFIG_MAXSUBSPERCLIENT];

void emcuetiti_client_register(emcuetiti_clienthandle* handle) {
	if (registeredclients < EMCUETITI_CONFIG_MAXCLIENTS) {
		registeredclients++;
		// search for a free slot to put this client
		for (int i = 0; i < ARRAY_ELEMENTS(clients); i++) {
			if (clients[i].client == NULL) {
				emcuetiti_clientstate* cs = clients + i;
				cs->client = handle;
				cs->readstate = CLIENTREADSTATE_IDLE;
				break;
			}
		}
	}
}

void emcuetiti_client_unregister(emcuetiti_clienthandle* handle) {
	if (registeredclients > 0) {
		registeredclients--;
		for (int i = 0; i < ARRAY_ELEMENTS(clients); i++) {
			if (clients[i].client == handle) {
				emcuetiti_clientstate* cs = clients + i;

				// clear the client and it's subscriptions
				cs->client = NULL;
				subscribedtopics -= cs->subscriptions;

				for (int j = 0; j < ARRAY_ELEMENTS(subscriptions); j++) {
					if (cs->client == handle) {
						subscriptions[j].client = NULL;
						subscriptions[j].topic = NULL;
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
		if (cs->client->readfunc(userdata, offsetbuffer, 0, 1) == 1) {
			cs->readstate = CLIENTREADSTATE_REMAININGLEN;
			cs->bufferpos++;
		}
		break;
	case CLIENTREADSTATE_REMAININGLEN:
		if (cs->client->readfunc(userdata, offsetbuffer, 0, 1) == 1) {
			if ((*offsetbuffer & (1 << 7)) == 0) {
				libmqtt_decodelength(cs->buffer + 1, &cs->remainingbytes);
				if (cs->remainingbytes > 0)
					cs->readstate = CLIENTREADSTATE_PAYLOAD;
				else
					cs->readstate = CLIENTREADSTATE_COMPLETE;
			}
			cs->bufferpos++;
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
	uint8_t packetype = LIBMQTT_PACKETTYPEFROMPACKETTYPEANDFLAGS(cs->buffer[0]);
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
	}
	cs->readstate = CLIENTREADSTATE_IDLE;
}

void emcuetiti_poll() {
	if (registeredclients > 0) {
		for (int i = 0; i < ARRAY_ELEMENTS(clients); i++) {
			emcuetiti_clientstate* cs = clients + i;
			if (cs->client != NULL) {
				emcuetiti_poll_read(cs);
				if (cs->readstate == CLIENTREADSTATE_COMPLETE)
					emcuetiti_handleinboundpacket(cs);
			}
		}
	}
}

void emcuetiti_addtopicpart(emcuetiti_topichandle* root,
		emcuetiti_topichandle* part, const char* topicpart) {
	part->parent = root;
	part->topicpart = topicpart;
}

void emcuetiti_init(emcuetiti_topichandle* topicroot) {
	root = topicroot;
	registeredclients = 0;
	subscribedtopics = 0;
	memset(clients, 0, sizeof(clients));
	memset(subscriptions, 0, sizeof(subscriptions));
}
