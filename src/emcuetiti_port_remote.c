#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "emcuetiti_port_remote.h"
#include "emcuetiti.h"

#define TIMEOUT 30

static int emcuetiti_port_remote_publishready(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* client, emcuetiti_topichandle* topic,
		size_t payloadlen) {
	return 0;
}

static void emcuetiti_port_remote_movetostate(
		emcuetiti_port_remote_portdata* portdata,
		emcuetiti_port_remote_state newstate) {
	memset(&portdata->statedata, 0, sizeof(portdata->statedata));
	portdata->state = newstate;
	printf("s:%d\n", newstate);
}

static void processtopicpart(emcuetiti_port_remote_portdata* data,
		buffers_buffer* topicbuffer) {
	uint8_t term = '\0';
	buffers_buffer_append(topicbuffer, &term, 1);
	printf("toppart: %s\n", topicbuffer->buffer);

	data->topic = emcuetiti_findtopic(data->broker, data->topic,
			topicbuffer->buffer);

	buffers_buffer_reset(topicbuffer);
}

static int emcuetiti_port_remote_munchtopicpart(
		emcuetiti_port_remote_portdata* portdata, const uint8_t* buffer,
		size_t len, buffers_buffer* topicbuffer) {

	size_t consumed;
	bool partcomplete = false;
	for (consumed = 0; consumed < len; consumed++) {
		if (buffer[consumed] == '/') {
			partcomplete = true;
			break;
		}
	}

	buffers_buffer_append(topicbuffer, buffer, consumed);
	if (partcomplete && buffers_buffer_available(topicbuffer) > 0)
		processtopicpart(portdata, topicbuffer);

	return consumed;
}

static int emcuetiti_port_remote_readpacket_writer(void* userdata,
		const uint8_t* buffer, size_t len) {

	int ret;
	emcuetiti_port_remote_portdata* portdata =
			(emcuetiti_port_remote_portdata*) userdata;

	if (portdata->statedata.ready.pktread.type == LIBMQTT_PACKETTYPE_PUBLISH) {
		switch (portdata->statedata.ready.pktread.state) {
		case LIBMQTT_PACKETREADSTATE_TOPIC: {
			BUFFERS_STATICBUFFER_TO_BUFFER(portdata->topicbuffer, topbuf);
			ret = emcuetiti_port_remote_munchtopicpart(portdata, buffer, len,
					&topbuf);
		}
			break;
		case LIBMQTT_PACKETREADSTATE_PAYLOAD: {
			BUFFERS_STATICBUFFER_TO_BUFFER(portdata->publishbuffer, pubbuff);
			ret = buffers_buffer_writefunc(&pubbuff, buffer, len);
		}
			break;
		}
	} else {
		printf("wr: %c[%02x]\n", buffer[0], buffer[0]);
		ret = 1;
	}
	return ret;
}

static int emcuetiti_port_remote_readpacket_statechange(libmqtt_packetread* pkt,
		void* userdata) {

	emcuetiti_port_remote_portdata* portdata =
			(emcuetiti_port_remote_portdata*) userdata;

	printf("ps:%d\n", (int) pkt->state);

	BUFFERS_STATICBUFFER_TO_BUFFER(portdata->topicbuffer, topbuff);

	switch (pkt->state) {
	case LIBMQTT_PACKETREADSTATE_TYPE: {
		// reset everything
		buffers_buffer_reset(&topbuff);
		portdata->topic = NULL;
		BUFFERS_STATICBUFFER_TO_BUFFER(portdata->publishbuffer, pubbuff);
		buffers_buffer_reset(&pubbuff);
	}
		break;
	case LIBMQTT_PACKETREADSTATE_TOPIC:
		printf("tl:%u\n", pkt->varhdr.publish.topiclen);
		break;
	case LIBMQTT_PACKETREADSTATE_PAYLOAD:
		printf("pl:%u\n", pkt->length - pkt->pos);
		processtopicpart(portdata, &topbuff);
		break;
	case LIBMQTT_PACKETREADSTATE_FINISHED:
		if (pkt->type == LIBMQTT_PACKETTYPE_PUBLISH)
			portdata->publishwaiting = true;
		break;
	}

	return LIBMQTT_EWOULDBLOCK;
}

static bool emcuetiti_port_remote_readpacket(
		emcuetiti_port_remote_portdata* portdata, libmqtt_packetread* pkt) {
	libmqtt_readpkt(pkt, emcuetiti_port_remote_readpacket_statechange, portdata,
			portdata->config->hostops->read, portdata->connectiondata,
			emcuetiti_port_remote_readpacket_writer, portdata);

	bool ret = false;

	bool packetfinished = pkt->state >= LIBMQTT_PACKETREADSTATE_FINISHED;

	if (packetfinished) {
		switch (pkt->state) {
		case LIBMQTT_PACKETREADSTATE_ERROR:
			printf("error\n");
			emcuetiti_port_remote_movetostate(portdata, REMOTEPORTSTATE_ERROR);
			break;
		case LIBMQTT_PACKETREADSTATE_FINISHED:
			printf("type %d\n", pkt->type);
			ret = true;
			break;
		}
	}

	return ret;
}

static void emcuetiti_port_remote_nextstate(
		emcuetiti_port_remote_portdata* portdata) {
	emcuetiti_port_remote_state newstate = portdata->state + 1;
	emcuetiti_port_remote_movetostate(portdata, newstate);
}

static bool emcuetiti_port_remote_timeoutexpired(emcuetiti_timestamp then,
		emcuetiti_timestamp now, unsigned timeout) {
	emcuetiti_timestamp elapsedtime = now - then;
	bool timedout = elapsedtime > timeout;
	if (timedout)
		printf("%u %u timeout\n", then, now);
	return timedout;
}

static void emcuetiti_port_remote_errorontimeout(emcuetiti_timestamp then,
		emcuetiti_timestamp now, unsigned timeout,
		emcuetiti_port_remote_portdata* portdata) {
	if (emcuetiti_port_remote_timeoutexpired(then, now, timeout))
		emcuetiti_port_remote_movetostate(portdata, REMOTEPORTSTATE_ERROR);
}

static void emcuetiti_port_remote_state_notconnected(emcuetiti_timestamp now,
		emcuetiti_port_remote_portdata* data) {
	const emcuetiti_port_remoteconfig* config = data->config;

	emcuetiti_timestamp timesincelastattempt = now
			- data->statedata.notconnected.lastattempt;
	if (timesincelastattempt >= 10) {
		data->statedata.notconnected.lastattempt = now;
		int ret = config->hostops->connect(config->host, config->port,
				&data->connectiondata);
		if (ret == EMCUETITI_PORT_REMOTE_OK)
			emcuetiti_port_remote_nextstate(data);
		else if (ret == EMCUETITI_PORT_REMOTE_ERR)
			emcuetiti_port_remote_movetostate(data, REMOTEPORTSTATE_ERROR);
	}
}

static void emcuetiti_port_remote_state_connecting(emcuetiti_timestamp now,
		emcuetiti_port_remote_portdata* data) {
	const emcuetiti_port_remoteconfig* config = data->config;
	emcuetiti_port_remote_statedata_connecting* statedata =
			&data->statedata.connecting;

	if (statedata->connreqsent) {
		if (emcuetiti_port_remote_readpacket(data, &statedata->pktread)) {
			emcuetiti_port_remote_nextstate(data);
		} else
			emcuetiti_port_remote_errorontimeout(statedata->connsentat, now,
			TIMEOUT, data);
	} else {
		libmqtt_construct_connect(config->hostops->write, data->connectiondata,
				config->keepalive, config->clientid, NULL, NULL, NULL,
				NULL, true);
		statedata->connsentat = now;
		statedata->connreqsent = true;
	}
}

static void emcuetiti_port_remote_state_subscribing(emcuetiti_timestamp now,
		emcuetiti_port_remote_portdata* data) {

	if (data->config->numtopics > 0) {
		emcuetiti_port_remote_statedata_subscribing* statedata =
				&data->statedata.subscribing;

		if (statedata->subreqsent) {
			if (emcuetiti_port_remote_readpacket(data, &statedata->pktread)) {
				if (statedata->pktread.type == LIBMQTT_PACKETTYPE_SUBACK
						&& statedata->msgid
								== statedata->pktread.varhdr.msgid) {
					emcuetiti_port_remote_nextstate(data);
					data->statedata.ready.datalastsent = now;
					data->statedata.ready.datalastreceived = now;
				}
			} else
				emcuetiti_port_remote_errorontimeout(statedata->subreqsentat,
						now, 10, data);
		} else {
			libmqtt_construct_subscribe(data->config->hostops->write,
					data->connectiondata, data->config->topics,
					data->config->numtopics, data->msgid);
			statedata->msgid = data->msgid;
			statedata->subreqsent = true;
			statedata->subreqsentat = now;

			data->msgid++;
		}
	} else
		emcuetiti_port_remote_nextstate(data);
}

static void emcuetiti_port_remote_state_ready(emcuetiti_timestamp now,
		emcuetiti_port_remote_portdata* data) {
	switch (data->statedata.ready.state) {
	case REMOTEPORTSTATE_READY_READ:
		if (emcuetiti_port_remote_readpacket(data,
				&data->statedata.ready.pktread)) {
			data->statedata.ready.datalastreceived = now;

			// if we have a keep alive set error out if no data has come in
		} else if (data->config->keepalive > 0)
			emcuetiti_port_remote_errorontimeout(
					data->statedata.ready.datalastreceived, now,
					data->config->keepalive * 2, data);
		break;
	case REMOTEPORTSTATE_READY_WRITE:

		if (data->publishwaiting) {

			BUFFERS_STATICBUFFER_TO_BUFFER(data->publishbuffer, pb);

			emcuetiti_publish pub = { .topic = data->topic,
			/*emcuetiti_writefunc writefunc;*/
			.readfunc = buffers_buffer_readfunc,
			/*
			 emcuetiti_freefunc freefunc;
			 emcuetiti_resetfunc resetfunc;*/
			.userdata = &pb, //
					.payloadln = buffers_buffer_available(&pb) };

			emcuetiti_broker_publish(data->broker, &pub);

			data->publishwaiting = false;
		}

		break;
	case REMOTEPORTSTATE_READY_KEEPALIVE:
		if (data->config->keepalive > 0) {
			emcuetiti_timestamp timesincelastrecv = now
					- data->statedata.ready.datalastreceived;
			emcuetiti_timestamp timesincelastsend = now
					- data->statedata.ready.datalastsent;

			bool sendping = timesincelastsend >= data->config->keepalive
					|| timesincelastrecv >= data->config->keepalive;
			if (sendping) {
				libmqtt_construct_pingreq(data->config->hostops->write,
						data->connectiondata);
				data->statedata.ready.datalastsent = now;
			}
		}
		break;
	}

	data->statedata.ready.state = (data->statedata.ready.state + 1)
			% REMOTEPORTSTATE_READY_END;
}

static void emcuetiti_port_remote_state_error(emcuetiti_timestamp now,
		emcuetiti_port_remote_portdata* data) {
	const emcuetiti_port_remoteconfig* config = data->config;

	if (data->connectiondata != NULL) {
		config->hostops->disconnect(data->connectiondata);
		data->connectiondata = NULL;
	}
}

static void emcuetiti_port_remote_poll(emcuetiti_timestamp now, void* portdata) {
	emcuetiti_port_remote_portdata* data =
			(emcuetiti_port_remote_portdata*) portdata;
	switch (data->state) {
	case REMOTEPORTSTATE_NOTCONNECTED:
		emcuetiti_port_remote_state_notconnected(now, data);
		break;
	case REMOTEPORTSTATE_CONNECTING:
		emcuetiti_port_remote_state_connecting(now, data);
		break;
	case REMOTEPORTSTATE_SUBSCRIBING:
		emcuetiti_port_remote_state_subscribing(now, data);
		break;
	case REMOTEPORTSTATE_READY:
		emcuetiti_port_remote_state_ready(now, data);
		break;
	case REMOTEPORTSTATE_DISCONNECTED:
		break;
	case REMOTEPORTSTATE_ERROR:
		emcuetiti_port_remote_state_error(now, data);
		break;
	}
}

void emcuetiti_port_remote_new(emcuetiti_brokerhandle* broker,
		emcuetiti_port_remoteconfig* config, emcuetiti_porthandle* port,
		emcuetiti_port_remote_portdata* portdata) {

	port->pollfunc = emcuetiti_port_remote_poll;
	port->publishreadycallback = emcuetiti_port_remote_publishready;

	portdata->broker = broker;
	portdata->config = config;
	portdata->msgid = 0;
	emcuetiti_port_remote_movetostate(portdata, REMOTEPORTSTATE_NOTCONNECTED);
	port->portdata = portdata;

	emcuetiti_port_register(broker, port);
}

