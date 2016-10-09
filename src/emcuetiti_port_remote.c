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

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "emcuetiti_port_remote.h"
#include "emcuetiti.h"

#define TIMEOUT 30

static int emcuetiti_port_remote_publishready(emcuetiti_brokerhandle* broker,
		emcuetiti_topichandle* topic, buffers_buffer* buffer, void* portdata) {

	emcuetiti_port_remote_portdata* pd =
			(emcuetiti_port_remote_portdata*) portdata;

	if (pd->state == REMOTEPORTSTATE_READY) {
		buffers_buffer_reference bufferreference;
		buffers_buffer_createreference(buffer, &bufferreference);
		buffers_buffer_ref(buffer);

		size_t payloadlen = buffers_buffer_available(&bufferreference.buffer);

		emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG,
				"sending publish to remote, %d payload bytes %d %d", payloadlen,
				*buffer->refs, *bufferreference.buffer.refs);

		libmqtt_construct_publish(
				//
				pd->config->hostops->write,
				pd->connectiondata, //
				buffers_buffer_readfunc,
				&bufferreference.buffer, //
				emcuetiti_topic_topichandlewriter, topic,
				emcuetiti_topic_len(topic), //
				payloadlen, //
				0, //
				false, //
				false, //
				0);

		buffers_buffer_unref(&bufferreference.buffer);

		emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG, "finished");
	}

	return 0;
}

static void emcuetiti_port_remote_movetostate(
		emcuetiti_port_remote_portdata* portdata,
		emcuetiti_port_remote_state newstate) {
	memset(&portdata->statedata, 0, sizeof(portdata->statedata));
	portdata->state = newstate;
	emcuetiti_log(portdata->broker, EMCUETITI_LOG_LEVEL_DEBUG, "s:%d",
			newstate);
}

static void processtopicpart(buffers_buffer* topicbuffer, void* data) {
	emcuetiti_port_remote_portdata* portdata =
			(emcuetiti_port_remote_portdata*) data;
	emcuetiti_log(portdata->broker, EMCUETITI_LOG_LEVEL_DEBUG, "toppart: %s",
			topicbuffer->buffer);
	portdata->topic = emcuetiti_findtopic(portdata->broker, portdata->topic,
			topicbuffer->buffer);
}

static int emcuetiti_port_remote_readpacket_writer(void* userdata,
		const uint8_t* buffer, size_t len) {

	int ret;
	emcuetiti_port_remote_portdata* portdata =
			(emcuetiti_port_remote_portdata*) userdata;

	if (portdata->statedata.ready.pktread.varhdr.common.type
			== LIBMQTT_PACKETTYPE_PUBLISH) {
		switch (portdata->statedata.ready.pktread.state) {
		case LIBMQTT_PACKETREADSTATE_PUBLISH_TOPIC: {
			BUFFERS_STATICBUFFER_TO_BUFFER(portdata->topicbuffer, topbuf);
			ret = emcuetiti_topic_munchtopicpart(buffer, len, &topbuf,
					processtopicpart, NULL, portdata);
		}
			break;
		case LIBMQTT_PACKETREADSTATE_PUBLISH_PAYLOAD: {
			if (portdata->publish == NULL) {
				portdata->publish = emcuetiti_broker_getpayloadbuffer(
						portdata->broker, &portdata->buffsz);
				emcuetiti_log(portdata->broker, EMCUETITI_LOG_LEVEL_DEBUG,
						"here xx");
			}
			if (portdata->publish != NULL) {

				BUFFERS_STATICBUFFER_TO_BUFFER_SIZE(portdata->publish, pubbuff,
						portdata->buffsz);
				ret = buffers_buffer_writefunc(&pubbuff, buffer, len);
			} else
				ret = LIBMQTT_EWOULDBLOCK;
		}
			break;
		}
	} else {
		emcuetiti_log(portdata->broker, EMCUETITI_LOG_LEVEL_DEBUG,
				"wr: %c[%02x]", buffer[0], buffer[0]);
		ret = 1;
	}
	return ret;
}

static int emcuetiti_port_remote_readpacket_statechange(libmqtt_packetread* pkt,
		libmqtt_packetread_state previousstate, void* userdata) {

	emcuetiti_port_remote_portdata* portdata =
			(emcuetiti_port_remote_portdata*) userdata;

	emcuetiti_log(portdata->broker, EMCUETITI_LOG_LEVEL_DEBUG, "ps:%d",
			(int) pkt->state);

	BUFFERS_STATICBUFFER_TO_BUFFER(portdata->topicbuffer, topbuff);

	switch (pkt->state) {
	case LIBMQTT_PACKETREADSTATE_TYPE: {
		// reset everything
		buffers_buffer_reset(&topbuff);
		portdata->topic = NULL;
	}
		break;
	case LIBMQTT_PACKETREADSTATE_PUBLISH_TOPIC:
		emcuetiti_log(portdata->broker, EMCUETITI_LOG_LEVEL_DEBUG, "tl:%u",
				pkt->varhdr.publish.topiclen);
		break;
	case LIBMQTT_PACKETREADSTATE_PUBLISH_PAYLOAD:
		if (previousstate != LIBMQTT_PACKETREADSTATE_PUBLISH_PAYLOAD) {
			emcuetiti_log(portdata->broker, EMCUETITI_LOG_LEVEL_DEBUG, "pl:%u",
					pkt->varhdr.common.length - pkt->pos);
			processtopicpart(&topbuff, portdata);
		}
		break;
	case LIBMQTT_PACKETREADSTATE_FINISHED:
		if (pkt->varhdr.common.type == LIBMQTT_PACKETTYPE_PUBLISH)
			portdata->publishwaiting = true;
		break;
	}

	return LIBMQTT_EWOULDBLOCK;
}

static bool emcuetiti_port_remote_readpacket(
		emcuetiti_port_remote_portdata* portdata, libmqtt_packetread* pkt) {
	bool ret = false;
	if (portdata->config->hostops->datawaiting == NULL
			|| portdata->config->hostops->datawaiting(
					portdata->connectiondata)) {

		libmqtt_readpkt(pkt, emcuetiti_port_remote_readpacket_statechange,
				portdata, portdata->config->hostops->read,
				portdata->connectiondata,
				emcuetiti_port_remote_readpacket_writer, portdata);

		bool packetfinished = pkt->state >= LIBMQTT_PACKETREADSTATE_FINISHED;

		if (packetfinished) {
			switch (pkt->state) {
			case LIBMQTT_PACKETREADSTATE_ERROR:
				emcuetiti_log(portdata->broker, EMCUETITI_LOG_LEVEL_DEBUG,
						"error");
				emcuetiti_port_remote_movetostate(portdata,
						REMOTEPORTSTATE_ERROR);
				break;
			case LIBMQTT_PACKETREADSTATE_FINISHED:
				emcuetiti_log(portdata->broker, EMCUETITI_LOG_LEVEL_DEBUG,
						"type %d", pkt->varhdr.common.type);
				ret = true;
				break;
			}
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
				if (statedata->pktread.varhdr.common.type
						== LIBMQTT_PACKETTYPE_SUBACK
						&& statedata->msgid
								== statedata->pktread.varhdr.suback.msgid) {
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

			BUFFERS_STATICBUFFER_TO_BUFFER_SIZE(data->publish, pb, data->buffsz);

			emcuetiti_publish pub = { .topic = data->topic,
			/*emcuetiti_writefunc writefunc;*/
			.readfunc = buffers_buffer_readfunc,
			/*
			 emcuetiti_freefunc freefunc;
			 emcuetiti_resetfunc resetfunc;*/
			.userdata = &pb, //
					.payloadln = buffers_buffer_available(&pb) };

			emcuetiti_broker_publish(data->broker, &pub);

			buffers_buffer_unref(&pb);

			data->publishwaiting = false;
			data->publish = NULL;
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

static const emcuetiti_port_callbacks callbacks = { //
		.pollfunc = emcuetiti_port_remote_poll, //
				.publishreadycallback = emcuetiti_port_remote_publishready, //
		};

void emcuetiti_port_remote_new(emcuetiti_brokerhandle* broker,
		emcuetiti_port_remoteconfig* config, emcuetiti_porthandle* port,
		emcuetiti_port_remote_portdata* portdata) {

	portdata->broker = broker;
	portdata->config = config;
	portdata->msgid = 0;
	portdata->publish = NULL;
	emcuetiti_port_remote_movetostate(portdata, REMOTEPORTSTATE_NOTCONNECTED);

	port->callbacks = &callbacks;
	port->portdata = portdata;

	emcuetiti_port_register(broker, port);
}

