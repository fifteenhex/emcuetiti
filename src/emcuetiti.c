#include <string.h>

#include "libmqtt.h"
#include "libmqtt_priv.h"

#include "emcuetiti_priv.h"
#include "emcuetiti.h"
#include "buffers.h"

#include "util.h"

static void emcuetiti_broker_dumpstate_printtopic(
		emcuetiti_brokerhandle* broker, emcuetiti_topichandle* node) {
	bool first = node->parent == NULL;
	if (!first) {
		emcuetiti_broker_dumpstate_printtopic(broker, node->parent);
		broker->callbacks->log(broker, "/");
	}
	broker->callbacks->log(broker, "%s", node->topicpart);
}

void emcuetiti_broker_poll(emcuetiti_brokerhandle* broker) {
	emcuetiti_timestamp now = broker->callbacks->timestamp();
	emcuetiti_port_poll(broker, now);
	emcuetiti_client_poll(broker, now);
}

void emcuetiti_broker_publish(emcuetiti_brokerhandle* broker,
		emcuetiti_publish* publish) {

	broker->callbacks->log(broker, "outgoing publish to");
	emcuetiti_broker_dumpstate_printtopic(broker, publish->topic);

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
						broker->callbacks->log(broker,
								"looking for wildcard subscription below this level");
						emcuetiti_topichandle* t = publish->topic;
						while (t->parent != NULL) {
							t = t->parent;
							if (t == topic) {
								broker->callbacks->log(broker,
										"found wild card sub below target");
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
				broker->callbacks->log(broker, "sending publish to %s",
						cs->clientid);

				libmqtt_writefunc clientwritefunc =
						emcuetiti_client_resolvewritefunc(broker,
								&broker->clients[c]);

				libmqtt_construct_publish(clientwritefunc, //
						broker->clients[c].client->userdata, //
						publish->readfunc, //
						publish->userdata, //
						emcuetiti_topic_topichandlewriter, //
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

	broker->callbacks->log(broker, "finished");

//if (publish->freefunc != NULL)
//	publish->freefunc(publish->userdata);
}

void emcuetiti_broker_init(emcuetiti_brokerhandle* broker) {
// clear out state
	broker->root = NULL;
	broker->registeredclients = 0;
	memset(broker->ports, 0, sizeof(broker->ports));
	memset(broker->clients, 0, sizeof(broker->clients));
}

static void emcuetiti_broker_dumpstate_child(emcuetiti_brokerhandle* broker,
		emcuetiti_topichandle* node) {
	for (; node != NULL; node = node->sibling) {
		if (node->child != NULL)
			emcuetiti_broker_dumpstate_child(broker, node->child);
		else {
			emcuetiti_broker_dumpstate_printtopic(broker, node);
		}
	}
}

void emcuetiti_broker_dumpstate(emcuetiti_brokerhandle* broker) {
	broker->callbacks->log(broker, "-- Topic hierachy --");
	emcuetiti_topichandle* th = broker->root;
	emcuetiti_broker_dumpstate_child(broker, th);

	broker->callbacks->log(broker, "-- Clients --");
	for (int i = 0; i < ARRAY_ELEMENTS(broker->clients); i++) {
		emcuetiti_clientstate* cs = &(broker->clients[i]);
		emcuetiti_client_dumpstate(broker, cs);
	}
}

bool emcuetiti_broker_canacceptmoreclients(emcuetiti_brokerhandle* broker) {
	return broker->registeredclients < EMCUETITI_CONFIG_MAXCLIENTS;
}

uint8_t* emcuetiti_broker_getpayloadbuffer(emcuetiti_brokerhandle* broker) {
	for (int i = 0; i < EMCUETITI_CONFIG_MAXINFLIGHTPAYLOADS; i++) {
		uint8_t* bufferaddress = &broker->inflightpayloads[i];
		BUFFERS_STATICBUFFER_TO_BUFFER(bufferaddress, payloadbuffer);
		if (!buffers_buffer_inuse(&payloadbuffer)) {
			buffers_buffer_ref(&payloadbuffer);
			return bufferaddress;
		}
	}
	return NULL;
}
