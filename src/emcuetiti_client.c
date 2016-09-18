#include <string.h>

#include <stdio.h>

#include "emcuetiti_topic.h"
#include "emcuetiti_client.h"
#include "emcuetiti_error.h"

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
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs) {
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

	emcuetiti_timestamp expires = client->lastseen + keepalive;

	if (expires > now) {
		EMCUETITI_CONFIG_TIMESTAMPTYPE timebeforeexpiry = expires - now;
#if EMCUETITI_CONFIG_DEBUG_KEEPALIVE
		broker->callbacks->log(broker, "client %s expires in %ds", client->clientid,
				timebeforeexpiry);
#endif
	} else {
		broker->callbacks->log(broker, "client %s has expired",
				client->clientid);
		emcuetiti_disconnectclient(broker, client);
	}
}

static void emcuetiti_handleinboundpacket_publish(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs) {

	broker->callbacks->log(broker, "handling publish");

	/*
	 libmqtt_qos qos = (flags >> 1) & 0x3;
	 uint16_t messageid;

	 uint16_t topiclen;
	 emcuetiti_topichandle* t = emcuetiti_readtopicstringandfindtopic(broker,
	 cs->buffer, &topiclen, NULL);

	 if (t != NULL) {
	 cs->publishtopic = t;
	 cs->publishpayloadlen = cs->varheaderandpayloadlen - (topiclen + 2);
	 //cs->readstate = CLIENTREADSTATE_PUBLISHREADY;
	 cs->bufferpos = topiclen + 2;

	 if (qos > LIBMQTT_QOS0_ATMOSTONCE) {
	 messageid = (cs->buffer[cs->bufferpos] << 8)
	 | cs->buffer[cs->bufferpos + 1];
	 cs->bufferpos += 2;
	 }

	 for (int p = 0; p < ARRAY_ELEMENTS(broker->ports); p++) {
	 if (broker->ports[p] != NULL) {
	 broker->callbacks->log(broker, "dispatching publish to port %d",
	 p);
	 if (broker->ports[p]->publishreadycallback != NULL)
	 broker->ports[p]->publishreadycallback(broker, cs->client,
	 t, cs->publishpayloadlen);
	 }
	 }

	 if (qos > LIBMQTT_QOS0_ATMOSTONCE) {
	 libmqtt_construct_puback(
	 emcuetiti_client_resolvewritefunc(broker, cs),
	 cs->client->userdata, messageid);
	 }
	 } else {
	 broker->callbacks->log(broker, "publish to invalid topic");
	 emcuetiti_disconnectclient(broker, cs);
	 }*/
}

static void emcuetiti_handleinboardpacket_connect(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs) {
	void* userdata = cs->client->userdata;

	cs->keepalive = cs->incomingpacket.varhdr.connect.keepalive;
	int level = cs->incomingpacket.varhdr.connect.level;

	broker->callbacks->log(broker,
			"protoname %s, level %d, keepalive %d, clientid %s ", NULL,
			(int) level, (int) cs->keepalive, cs->clientid);

	if (broker->callbacks->authenticatecallback == NULL
			|| broker->callbacks->authenticatecallback(cs->clientid)) {
		libmqtt_construct_connack(emcuetiti_client_resolvewritefunc(broker, cs),
				userdata);
	}
	cs->state = CLIENTSTATE_CONNECTED;
	//cs->readstate = CLIENTREADSTATE_IDLE;
}

static void emcuetiti_handleinboundpacket_subscribe(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs) {
	void* userdata = cs->client->userdata;
	uint8_t returncodes[] = { LIBMQTT_SUBSCRIBERETURNCODE_FAILURE };

	uint16_t messageid = cs->incomingpacket.varhdr.msgid;

	/*
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
	 broker->callbacks->log(broker, "probably have more topics");

	 broker->callbacks->log(broker, "sub from %s, messageid %d, qos %d",
	 cs->clientid, (int) messageid, (int) qos);

	 bool qosisvalid = qos >= LIBMQTT_QOS0_ATMOSTONCE
	 && qos <= LIBMQTT_QOS2_EXACTLYONCE;
	 if (!qosisvalid)
	 broker->callbacks->log(broker,
	 "client %s requested invalid qos in subreq", cs->clientid);

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
	 }*/

	libmqtt_construct_suback(emcuetiti_client_resolvewritefunc(broker, cs),
			userdata, messageid, returncodes, 1);
}

static void emcuetiti_handleinboundpacket_unsubscribe(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs) {
	void* userdata = cs->client->userdata;

	uint16_t messageid = cs->incomingpacket.varhdr.msgid;
	/*
	 uint8_t* buffer = cs->buffer;
	 uint8_t* bufferend = cs->buffer + cs->varheaderandpayloadlen;

	 uint16_t messageid = (*(buffer++) << 8) | *(buffer++);

	 uint16_t topiclen;
	 emcuetiti_subscription_level level;

	 emcuetiti_topichandle* t = emcuetiti_readtopicstringandfindtopic(broker,
	 buffer, &topiclen, &level);
	 buffer += 2 + topiclen;

	 if (buffer != bufferend)
	 broker->callbacks->log(broker, "probably more topics");

	 broker->callbacks->log(broker, "unsub from %s, messageid %d", cs->clientid,
	 (int) messageid);

	 if (t == NULL) {
	 broker->callbacks->log(broker,
	 "client tried to unsub from nonexisting topic");
	 } else {

	 for (int i = 0; i < ARRAY_ELEMENTS(cs->subscriptions); i++) {
	 if (cs->subscriptions[i].topic == t) {
	 cs->subscriptions[i].topic = NULL;
	 cs->numsubscriptions--;
	 break;
	 }
	 }
	 }*/

	libmqtt_construct_unsuback(emcuetiti_client_resolvewritefunc(broker, cs),
			userdata, messageid);

}

static void emcuetiti_handleinboundpacket_disconnect(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs) {
	broker->callbacks->log(broker, "client %s has requested to disconnect",
			cs->clientid);
	emcuetiti_disconnectclient(broker, cs);
}

static void emcuetiti_handleinboundpacket_pingreq(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs) {
	broker->callbacks->log(broker, "pingreq from %s", cs->clientid);
	libmqtt_construct_pingresp(emcuetiti_client_resolvewritefunc(broker, cs),
			cs->client->userdata);
}

static void emcuetiti_handleinboundpacket(emcuetiti_brokerhandle* broker,
		emcuetiti_clientstate* cs) {
	switch (cs->incomingpacket.type) {
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
		broker->callbacks->log(broker,
				"unhandled packet type %d from client %s",
				(int) cs->incomingpacket.type, cs->clientid);
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
				//cs->readstate = CLIENTREADSTATE_IDLE;
				cs->lastseen = broker->callbacks->timestamp();

				memset(cs->subscriptions, 0, sizeof(cs->subscriptions));
				cs->numsubscriptions = 0;

				broker->registeredclients++;
				broker->callbacks->log(broker,
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
			broker->callbacks->log(broker,
					"unregistered client, %d clients left",
					broker->registeredclients);
		}
	}
}

int emcuetiti_client_readpublish(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* client, uint8_t* buffer, size_t len) {

	/*emcuetiti_clientstate* cs;
	 for (int i = 0; i < ARRAY_ELEMENTS(broker->clients); i++) {
	 if (broker->clients[i].client == client) {
	 cs = &(broker->clients[i]);
	 }
	 }

	 len = size_min(len, cs->publishpayloadlen);

	 memcpy(buffer, cs->buffer + cs->bufferpos, len);

	 //cs->readstate = CLIENTREADSTATE_IDLE;*/

	return 0;
}

static int emcuetiti_client_writefunc(void* userdata, const uint8_t* buffer,
		size_t len) {

	emcuetiti_clientstate* cs = (emcuetiti_clientstate*) userdata;

	int ret = len;

	switch (cs->incomingpacket.type) {
	case LIBMQTT_PACKETTYPE_PUBLISH:
		switch (cs->incomingpacket.state) {
		case LIBMQTT_PACKETREADSTATE_PUBLISH_PAYLOAD:
			printf("wr: %c[%02x]\n", buffer[0], buffer[0]);
			ret = 1;
			break;
		}
		break;
	case LIBMQTT_PACKETTYPE_CONNECT: {
		BUFFERS_STATICBUFFER_TO_BUFFER(cs->buffer, cidb);
		ret = buffers_buffer_append(&cidb, buffer, len);
	}
		break;
	}

	return ret;
}

static int statechange(libmqtt_packetread* pkt,
		libmqtt_packetread_state previousstate, void* userdata) {

	emcuetiti_clientstate* cs = (emcuetiti_clientstate*) userdata;

	switch (previousstate) {
	case LIBMQTT_PACKETREADSTATE_CONNECT_CLIENTID: {
		BUFFERS_STATICBUFFER_TO_BUFFER(cs->buffer, clientbuffer);
		size_t end = buffers_buffer_emptyinto(&clientbuffer, cs->clientid,
				sizeof(cs->clientid) - 1);
		cs->clientid[end] = '\0';
	}
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
						if (readytoreadfunc(cs->client->userdata)) {
							libmqtt_readpkt(&cs->incomingpacket, statechange,
									cs,
									emcuetiti_client_resolvereadfunc(broker,
											cs), cs->client->userdata,
									emcuetiti_client_writefunc, cs);
							if (cs->incomingpacket.state
									== LIBMQTT_PACKETREADSTATE_FINISHED) {
								emcuetiti_handleinboundpacket(broker, cs);
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
		broker->callbacks->log(broker, "%s\t%s\t%d - subs %d", client->clientid,
				state, (int) client->incomingpacket.state,
				(int) client->numsubscriptions);
}
