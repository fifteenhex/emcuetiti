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

#include "emcuetiti_topic.h"
#include "emcuetiti_client.h"
#include "emcuetiti_error.h"
#include "emcuetiti_port.h"
#include "emcuetiti_broker.h"
#include "emcuetiti_log.h"

#include "buffers.h"
#include "libmqtt_priv.h"
#include "util.h"

emcuetiti_isconnected emcuetiti_client_resolvefunc_isconnected(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs) {
	emcuetiti_isconnected isconnectedfunc = broker->callbacks->isconnectedfunc;
#if EMCUETITI_CONFIG_PERCLIENTCALLBACKS
#if EMCUETITI_CONFIG_PERCLIENTCALLBACK_ISCONNECTED
	if(cs->client->ops->isconnected != NULL)
	isconnectedfunc = cs->client->ops->isconnected;
#endif
#endif
	return isconnectedfunc;
}

libmqtt_writefunc emcuetiti_client_resolvewritefunc(
		const emcuetiti_brokerhandle* broker, const emcuetiti_clientstate* cs) {
	libmqtt_writefunc writefunc = broker->callbacks->writefunc;
#if EMCUETITI_CONFIG_PERCLIENTCALLBACKS
#if EMCUETITI_CONFIG_PERCLIENTCALLBACK_WRITE
	if(cs->client->ops->writefunc != NULL)
	writefunc = cs->client->ops->writefunc;
#endif
#endif
	return writefunc;
}

static emcuetiti_readytoreadfunc emcuetiti_client_resolvefunc_readytoread(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs) {
	emcuetiti_readytoreadfunc readytoreadfunc = broker->callbacks->readytoread;
#if EMCUETITI_CONFIG_PERCLIENTCALLBACKS
#if EMCUETITI_CONFIG_PERCLIENTCALLBACK_READTOREAD
	if(cs->client->ops->readytoread != NULL)
	readfunc = cs->client->ops->readytoread;
#endif
#endif
	return readytoreadfunc;
}

libmqtt_readfunc emcuetiti_client_resolvereadfunc(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs) {
	libmqtt_readfunc readfunc = broker->callbacks->readfunc;
#if EMCUETITI_CONFIG_PERCLIENTCALLBACKS
#if EMCUETITI_CONFIG_PERCLIENTCALLBACK_READ
	if(cs->client->ops->readfuc != NULL)
	readfunc = cs->client->ops->readfunc;
#endif
#endif
	return readfunc;
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
}

static void emcuetiti_broker_poll_checkkeepalive(emcuetiti_brokerhandle* broker,
		emcuetiti_clientstate* client, emcuetiti_timestamp now) {

	uint16_t keepalive = EMCUETITI_CONFIG_DEFAULTKEEPALIVE;
	if (client->keepalive != 0)
		keepalive = client->keepalive;

	emcuetiti_timestamp expires = client->lastseen
			+ (keepalive + (keepalive / 2));

	if (expires > now) {
		EMCUETITI_CONFIG_TIMESTAMPTYPE timebeforeexpiry = expires - now;
#if EMCUETITI_CONFIG_DEBUG_KEEPALIVE
		emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG, "client %s expires in %ds",
				client->clientid, timebeforeexpiry);
#endif
	} else {
		emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG,
				"client %s has expired", client->clientid);
		emcuetiti_disconnectclient(broker, client);
	}
}

static void emcuetiti_handleinboundpacket_publish(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs) {

	BUFFERS_STATICBUFFER_TO_BUFFER_SIZE(cs->registers.publish.payloadbuff,
			payloadbuffer, cs->registers.publish.buffsz);

	if (cs->registers.publish.topic != NULL) {
		emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG,
				"handling publish, have %d bytes",
				buffers_buffer_available(&payloadbuffer));

		emcuetiti_port_onpublishready(broker, cs->registers.publish.topic,
				&payloadbuffer);
	} else
		emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG, "dafuq?");

	buffers_buffer_unref(&payloadbuffer);

	/*if (qos > LIBMQTT_QOS0_ATMOSTONCE) {
	 libmqtt_construct_puback(
	 emcuetiti_client_resolvewritefunc(broker, cs),
	 cs->client->userdata, messageid);
	 }
	 } else {
	 emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG, "publish to invalid topic");
	 emcuetiti_disconnectclient(broker, cs);
	 }*/
}

static void emcuetiti_handleinboardpacket_connect(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs) {
	void* userdata = cs->client->userdata;

	cs->keepalive = cs->incomingpacket.varhdr.connect.keepalive;
	int level = cs->incomingpacket.varhdr.connect.level;

	emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG,
			"protoname %s, level %d, keepalive %d, clientid %s ", NULL,
			(int) level, (int) cs->keepalive, cs->clientid);

	if (broker->callbacks->authenticatecallback == NULL
			|| broker->callbacks->authenticatecallback(cs->clientid)) {
		libmqtt_construct_connack(emcuetiti_client_resolvewritefunc(broker, cs),
				userdata);
	}
	cs->state = CLIENTSTATE_CONNECTED;
	emcuetiti_port_onclientconnected(broker, cs);
}

static void emcuetiti_handleinboundpacket_subscribe(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs) {
	void* userdata = cs->client->userdata;

	uint8_t returncodes[EMCUETITI_CONFIG_MAXSUBSPERCLIENT];

	uint16_t messageid = cs->incomingpacket.varhdr.subscribe.msgid;
	uint16_t num = cs->registers.subunsub.subscriptionspending;

	emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG,
			"processing %d subscriptions from message %d for %s", num,
			(int) messageid, cs->clientid);

	for (int i = 0; i < num; i++) {
		returncodes[i] = LIBMQTT_SUBSCRIBERETURNCODE_FAILURE;
		emcuetiti_topichandle* topic = cs->registers.subunsub.pendingtopics[i];
		if (topic != NULL) {
			if (cs->numsubscriptions < EMCUETITI_CONFIG_MAXSUBSPERCLIENT) {
				for (int j = 0; j < ARRAY_ELEMENTS(cs->subscriptions); j++) {
					if (cs->subscriptions[j].topic == NULL) {
						cs->subscriptions[j].topic = topic;
						cs->subscriptions[j].qos =
								cs->registers.subunsub.pendingqos[i];
						cs->subscriptions[j].level =
								cs->registers.subunsub.pendinglevels[i];
						cs->numsubscriptions++;
						returncodes[i] = 0;
						emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG,
								"inserted subscription to %s into %s, now have %d",
								topic->topicpart, cs->clientid,
								cs->numsubscriptions);
						break;
					}
				}
			}
		}
	}

	libmqtt_construct_suback(emcuetiti_client_resolvewritefunc(broker, cs),
			userdata, messageid, returncodes, num);
}

static void emcuetiti_handleinboundpacket_unsubscribe(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs) {
	void* userdata = cs->client->userdata;
	uint16_t messageid = cs->incomingpacket.varhdr.unsubscribe.msgid;

	emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG,
			"handling %d unsubscriptions",
			cs->registers.subunsub.subscriptionspending);

	for (int i = 0; i < cs->registers.subunsub.subscriptionspending; i++) {
		for (int j = 0; j < ARRAY_ELEMENTS(cs->subscriptions); j++) {
			if (cs->subscriptions[j].topic
					== cs->registers.subunsub.pendingtopics[i]) {
				cs->subscriptions[j].topic = NULL;
				cs->numsubscriptions--;
				emcuetiti_log(cs->broker, EMCUETITI_LOG_LEVEL_DEBUG,
						"cleared subscription from %s, now have %d",
						cs->clientid, cs->numsubscriptions);
				break;
			}
		}
	}

	libmqtt_writepkt_unsuback(&cs->outgoingpacket, messageid);
}

static void emcuetiti_handleinboundpacket_disconnect(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs) {
	emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG,
			"client %s has requested to disconnect", cs->clientid);
	emcuetiti_disconnectclient(broker, cs);
}

static void emcuetiti_handleinboundpacket_pingreq(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs) {
	EMCUETITI_LOG_DEBUG("pingreq from %s", cs->clientid);
	libmqtt_writepkt_pingresp(&cs->outgoingpacket);
}

static void emcuetiti_handleinboundpacket(emcuetiti_brokerhandle* broker,
		emcuetiti_clientstate* cs) {
	switch (cs->incomingpacket.varhdr.common.type) {
// common
	case LIBMQTT_PACKETTYPE_PINGREQ:
		emcuetiti_handleinboundpacket_pingreq(broker, cs);
		break;
	case LIBMQTT_PACKETTYPE_PUBLISH:
		emcuetiti_handleinboundpacket_publish(broker, cs);
		break;
// uncommon
	case LIBMQTT_PACKETTYPE_SUBSCRIBE:
		emcuetiti_handleinboundpacket_subscribe(broker, cs);
		break;
	case LIBMQTT_PACKETTYPE_UNSUBSCRIBE:
		emcuetiti_handleinboundpacket_unsubscribe(broker, cs);
		break;
		// very uncommon
	case LIBMQTT_PACKETTYPE_CONNECT:
		emcuetiti_handleinboardpacket_connect(broker, cs);
		break;
	case LIBMQTT_PACKETTYPE_DISCONNECT:
		emcuetiti_handleinboundpacket_disconnect(broker, cs);
		break;
	default:
		emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG,
				"unhandled packet type %d from client %s",
				(int) cs->incomingpacket.varhdr.common.type, cs->clientid);
		break;
	}
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
				cs->lastseen = broker->callbacks->timestamp();

				memset(cs->subscriptions, 0, sizeof(cs->subscriptions));
				cs->numsubscriptions = 0;
				memset(&cs->registers, 0, sizeof(cs->registers));

				libmqtt_writepkt_reset(&cs->outgoingpacket);

				cs->broker = broker;

				broker->registeredclients++;
				emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG,
						"registered client, now have %d clients",
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
			emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG,
					"unregistered client, %d clients left",
					broker->registeredclients);
		}
	}
}

static void processtopicpart(buffers_buffer* topicpart, void* userdata) {
	emcuetiti_clientstate* cs = (emcuetiti_clientstate*) userdata;

	if (cs->incomingpacket.varhdr.common.type == LIBMQTT_PACKETTYPE_PUBLISH) {
		cs->registers.publish.topic = emcuetiti_findtopic(cs->broker,
				cs->registers.publish.topic, topicpart->buffer);
	} else {
		cs->registers.subunsub.pendingtopics[cs->registers.subunsub.subscriptionspending] =
				emcuetiti_findtopic(cs->broker,
						cs->registers.subunsub.pendingtopics[cs->registers.subunsub.subscriptionspending],
						topicpart->buffer);
	}
}

static int emcuetiti_client_writefunc(void* userdata, const uint8_t* buffer,
		size_t len) {

	emcuetiti_clientstate* cs = (emcuetiti_clientstate*) userdata;

	int ret = len;

	BUFFERS_STATICBUFFER_TO_BUFFER(cs->buffer, cidb);

	switch (cs->incomingpacket.varhdr.common.type) {
	case LIBMQTT_PACKETTYPE_PUBLISH:
		switch (cs->incomingpacket.state) {
		case LIBMQTT_PACKETREADSTATE_PUBLISH_TOPIC:
			ret = emcuetiti_topic_munchtopicpart(buffer, len, &cidb,
					processtopicpart, NULL, cs);
			break;
		case LIBMQTT_PACKETREADSTATE_PUBLISH_PAYLOAD: {
			if (cs->registers.publish.payloadbuff == NULL)
				cs->registers.publish.payloadbuff =
						emcuetiti_broker_getpayloadbuffer(cs->broker,
								&cs->registers.publish.buffsz);

			if (cs->registers.publish.payloadbuff != NULL) {
				BUFFERS_STATICBUFFER_TO_BUFFER_SIZE(
						cs->registers.publish.payloadbuff, payloadbuff,
						cs->registers.publish.buffsz);
				ret = buffers_buffer_append(&payloadbuff, buffer, len);
			} else
				ret = LIBMQTT_EWOULDBLOCK;
			break;
		}
		}
		break;
	case LIBMQTT_PACKETTYPE_SUBSCRIBE:
	case LIBMQTT_PACKETTYPE_UNSUBSCRIBE: {
		ret = emcuetiti_topic_munchtopicpart(buffer, len, &cidb,
				processtopicpart, NULL, cs);
	}
		break;
	case LIBMQTT_PACKETTYPE_CONNECT: {
		ret = buffers_buffer_append(&cidb, buffer, len);
	}
		break;
	}

	return ret;
}

static int statechange(libmqtt_packetread* pkt,
		libmqtt_packetread_state previousstate, void* userdata) {

	emcuetiti_clientstate* cs = (emcuetiti_clientstate*) userdata;

	BUFFERS_STATICBUFFER_TO_BUFFER(cs->buffer, clientbuffer);

	switch (previousstate) {
	case LIBMQTT_PACKETREADSTATE_CONNECT_CLIENTID: {

		size_t end = buffers_buffer_emptyinto(&clientbuffer, cs->clientid,
				sizeof(cs->clientid) - 1);
		cs->clientid[end] = '\0';
		buffers_buffer_reset(&clientbuffer);
	}
		break;
	case LIBMQTT_PACKETREADSTATE_PUBLISH_TOPIC:
		if (buffers_buffer_available(&clientbuffer) > 0) {
			buffers_buffer_terminate(&clientbuffer);
			processtopicpart(&clientbuffer, cs);
		}
		break;
	case LIBMQTT_PACKETREADSTATE_SUBSCRIBE_QOS:
	case LIBMQTT_PACKETREADSTATE_UNSUBSCRIBE_TOPICFILTER:
		if (buffers_buffer_available(&clientbuffer) > 0) {
			buffers_buffer_terminate(&clientbuffer);
			processtopicpart(&clientbuffer, cs);
		}
		cs->registers.subunsub.pendingqos[cs->registers.subunsub.subscriptionspending] =
				pkt->varhdr.subscribe.topicfilterqos;
		cs->registers.subunsub.pendinglevels[cs->registers.subunsub.subscriptionspending] =
				THISANDABOVE;
		cs->registers.subunsub.subscriptionspending++;
		break;
	}

	return 0;
}

void emcuetiti_client_poll(emcuetiti_brokerhandle* broker,
		emcuetiti_timestamp now) {

	if (broker->registeredclients > 0) {
		for (int i = 0; i < ARRAY_ELEMENTS(broker->clients); i++) {
			emcuetiti_clientstate* cs = broker->clients + i;
			if (cs->client != NULL) {

				emcuetiti_isconnected isconnectedfunc =
						emcuetiti_client_resolvefunc_isconnected(broker, cs);
				emcuetiti_readytoreadfunc readytoreadfunc =
						emcuetiti_client_resolvefunc_readytoread(broker, cs);

				switch (cs->state) {
				case CLIENTSTATE_NEW:
				case CLIENTSTATE_CONNECTED:
					if (isconnectedfunc(cs->client->userdata)) {

						libmqtt_writepkt(&cs->outgoingpacket, NULL, NULL,
								emcuetiti_client_resolvewritefunc(broker, cs),
								cs->client->userdata, NULL, NULL);
						if (cs->outgoingpacket.state
								== LIBMQTT_PACKETREADSTATE_FINISHED) {
							libmqtt_writepkt_reset(&cs->outgoingpacket);
						}

						if (readytoreadfunc(cs->client->userdata)) {
							libmqtt_readpkt(&cs->incomingpacket, statechange,
									cs,
									emcuetiti_client_resolvereadfunc(broker,
											cs), cs->client->userdata,
									emcuetiti_client_writefunc, cs);
							if (cs->incomingpacket.state
									== LIBMQTT_PACKETREADSTATE_FINISHED) {
								emcuetiti_handleinboundpacket(broker, cs);

								BUFFERS_STATICBUFFER_TO_BUFFER(cs->buffer, cb);
								buffers_buffer_reset(&cb);

								memset(&cs->registers, 0,
										sizeof(cs->registers));

								cs->lastseen = now;
							}
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

void emcuetiti_client_dumpstate(emcuetiti_brokerhandle* broker,
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
		emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG, "%s\t%s\t%d - subs %d",
				client->clientid, state, (int) client->incomingpacket.state,
				(int) client->numsubscriptions);
}
