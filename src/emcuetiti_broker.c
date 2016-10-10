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

#include "libmqtt.h"
#include "libmqtt_priv.h"

#include "emcuetiti_broker.h"
#include "emcuetiti_log.h"
#include "emcuetiti_port.h"
#include "emcuetiti_topic.h"

#include "buffers.h"

#include "util.h"

static void emcuetiti_broker_dumpstate_printtopic(
		const emcuetiti_brokerhandle* broker, emcuetiti_topichandle* node) {
	bool first = node->parent == NULL;
	if (!first) {
		emcuetiti_broker_dumpstate_printtopic(broker, node->parent);
		emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG, "/");
	}
	emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG, "%s", node->topicpart);
}

void emcuetiti_broker_poll(emcuetiti_brokerhandle* broker) {
	emcuetiti_timestamp now = broker->callbacks->timestamp();
	emcuetiti_port_poll(broker, now);
	emcuetiti_client_poll(broker, now);
}

void emcuetiti_broker_publish(const emcuetiti_brokerhandle* broker,
		emcuetiti_publish* publish) {

	emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG, "outgoing publish to");
	emcuetiti_broker_dumpstate_printtopic(broker, publish->topic);

	for (int c = 0; c < ARRAY_ELEMENTS(broker->clients); c++) {
		const emcuetiti_clientstate* cs = &broker->clients[c];
		if (cs->state == CLIENTSTATE_CONNECTED) {
			emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG,
					"checking client %s, has %d subscriptions", cs->clientid,
					cs->numsubscriptions);
			bool subbed = false;
			for (int i = 0; i < ARRAY_ELEMENTS(cs->subscriptions); i++) {
				const emcuetiti_subscription* sub = &(cs->subscriptions[i]);
				const emcuetiti_topichandle* topic = sub->topic;
				if (topic != NULL) {

					emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG, "topic %s",
							topic->topicpart);

					if (topic == publish->topic) {
						subbed = true;
					} else if (sub->level == THISANDABOVE) {
						emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG,
								"looking for wildcard subscription below this level");
						emcuetiti_topichandle* t = publish->topic;
						while (t->parent != NULL) {
							t = t->parent;
							if (t == topic) {
								emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG,
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
				emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG,
						"sending publish to %s", cs->clientid);

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

	emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG, "finished");
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
	emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG, "-- Topic hierachy --");
	emcuetiti_topichandle* th = broker->root;
	emcuetiti_broker_dumpstate_child(broker, th);

	emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG, "-- Clients --");
	for (int i = 0; i < ARRAY_ELEMENTS(broker->clients); i++) {
		emcuetiti_clientstate* cs = &(broker->clients[i]);
		emcuetiti_client_dumpstate(broker, cs);
	}
}

bool emcuetiti_broker_canacceptmoreclients(emcuetiti_brokerhandle* broker) {
	return broker->registeredclients < EMCUETITI_CONFIG_MAXCLIENTS;
}

uint8_t* emcuetiti_broker_getpayloadbuffer(const emcuetiti_brokerhandle* broker,
		size_t* size) {
	for (int i = 0; i < EMCUETITI_CONFIG_MAXINFLIGHTPAYLOADS; i++) {
		BUFFERS_STATICBUFFER_TO_BUFFER(broker->inflightpayloads[i],
				payloadbuffer);
		*size = *payloadbuffer.size;
		if (!buffers_buffer_inuse(&payloadbuffer)) {
			buffers_buffer_ref(&payloadbuffer);
			return (uint8_t*) &broker->inflightpayloads[i];
		}
	}
	return NULL;
}
