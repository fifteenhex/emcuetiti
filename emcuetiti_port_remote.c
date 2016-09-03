#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "emcuetiti_port_remote.h"
#include "emcuetiti.h"

static int emcuetiti_port_remote_publishready(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* client, emcuetiti_topichandle* topic,
		size_t payloadlen) {
	return 0;
}

static bool emcuetiti_port_remote_readpacket(
		emcuetiti_port_remote_portdata* portdata, libmqtt_packetread* pkt) {
	libmqtt_readpkt(pkt, NULL, portdata->config->hostops->read,
			portdata->connectiondata, NULL, NULL);

	bool packetfinished = pkt->state >= LIBMQTT_PACKETREADSTATE_FINISHED;

	if (packetfinished) {
		if (pkt->state == LIBMQTT_PACKETREADSTATE_FINISHED)
			printf("type %d\n", pkt->type);
		else if (pkt->state == LIBMQTT_PACKETREADSTATE_ERROR)
			printf("error\n");
	}
	return packetfinished;
}

static void emcuetiti_port_remote_movetostate(
		emcuetiti_port_remote_portdata* portdata,
		emcuetiti_port_remote_state newstate) {
	memset(&portdata->statedata, 0, sizeof(portdata->statedata));
	portdata->state = newstate;
	printf("s:%d\n", newstate);
}

static void emcuetiti_port_remote_nextstate(
		emcuetiti_port_remote_portdata* portdata) {
	emcuetiti_port_remote_state newstate = portdata->state + 1;
	emcuetiti_port_remote_movetostate(portdata, newstate);
}

static bool emcuetiti_port_remote_timeoutexpired(emcuetiti_timestamp then,
		emcuetiti_timestamp now, unsigned timeout) {
	emcuetiti_timestamp elapsedtime = now - then;
	return elapsedtime > timeout;
}

static void emcuetiti_port_remote_errorontimeout(emcuetiti_timestamp then,
		emcuetiti_timestamp now, unsigned timeout,
		emcuetiti_port_remote_portdata* portdata) {
	if (emcuetiti_port_remote_timeoutexpired(then, now, timeout))
		emcuetiti_port_remote_movetostate(portdata, REMOTEPORTSTATE_ERROR);
}

static void emcuetiti_port_remote_poll(emcuetiti_timestamp now, void* portdata) {

	emcuetiti_port_remote_portdata* data =
			(emcuetiti_port_remote_portdata*) portdata;
	const emcuetiti_port_remoteconfig* config = data->config;
	emcuetiti_port_remote_write writefunc = data->config->hostops->write;

	switch (data->state) {
	case REMOTEPORTSTATE_NOTCONNECTED: {
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
		break;
	case REMOTEPORTSTATE_CONNECTING: {
		if (data->statedata.connecting.connreqsent) {
			if (emcuetiti_port_remote_readpacket(data,
					&data->statedata.connecting.pktread)) {
				emcuetiti_port_remote_nextstate(data);
			} else
				emcuetiti_port_remote_errorontimeout(
						data->statedata.connecting.connreqsent, now, 10, data);
		} else {
			libmqtt_construct_connect(writefunc, data->connectiondata,
					config->keepalive, config->clientid, NULL, NULL, NULL,
					NULL);
			data->statedata.connecting.connsentat = now;
			data->statedata.connecting.connreqsent = true;
		}
	}
		break;

	case REMOTEPORTSTATE_SUBSCRIBING:
		if (data->statedata.subscribing.subreqsent) {
			if (emcuetiti_port_remote_readpacket(data,
					&data->statedata.subscribing.pktread)) {
				emcuetiti_port_remote_nextstate(data);
				data->statedata.ready.datalastsent = now;
				data->statedata.ready.datalastreceived = now;
			} else
				emcuetiti_port_remote_errorontimeout(
						data->statedata.subscribing.subreqsentat, now, 10,
						data);
		} else {
			libmqtt_construct_subscribe(writefunc, data->connectiondata,
					config->topics, config->numtopics);
			data->statedata.subscribing.subreqsent = true;
			data->statedata.subscribing.subreqsentat = now;
		}
		break;
	case REMOTEPORTSTATE_READY: {
		if (emcuetiti_port_remote_readpacket(data,
				&data->statedata.ready.pktread)) {

			if (data->statedata.ready.pktread.state
					== LIBMQTT_PACKETREADSTATE_FINISHED)
				data->statedata.ready.datalastreceived = now;
			else if (data->statedata.ready.pktread.state
					== LIBMQTT_PACKETREADSTATE_ERROR) {
				emcuetiti_port_remote_movetostate(data, REMOTEPORTSTATE_ERROR);
				break;
			}

		}

		if (config->keepalive > 0) {
			emcuetiti_timestamp timesincelastrecv = now
					- data->statedata.ready.datalastreceived;
			emcuetiti_timestamp timesincelastsend = now
					- data->statedata.ready.datalastsent;

			if (timesincelastrecv >= (data->config->keepalive * 2))
				emcuetiti_port_remote_movetostate(data, REMOTEPORTSTATE_ERROR);
			else {
				bool sendping = timesincelastsend >= data->config->keepalive
						|| timesincelastrecv >= data->config->keepalive;
				if (sendping) {
					libmqtt_construct_pingreq(writefunc, data->connectiondata);
					data->statedata.ready.datalastsent = now;
				}
			}
		}
	}
		break;
	case REMOTEPORTSTATE_DISCONNECTED:
		break;
	case REMOTEPORTSTATE_ERROR:
		if (data->connectiondata != NULL) {
			config->hostops->disconnect(data->connectiondata);
			data->connectiondata = NULL;
		}
		break;
	}

}

void emcuetiti_port_remote_new(emcuetiti_brokerhandle* broker,
		emcuetiti_port_remoteconfig* config, emcuetiti_porthandle* port,
		emcuetiti_port_remote_portdata* portdata) {

	port->pollfunc = emcuetiti_port_remote_poll;
	port->publishreadycallback = emcuetiti_port_remote_publishready;

	portdata->config = config;
	emcuetiti_port_remote_movetostate(portdata, REMOTEPORTSTATE_NOTCONNECTED);
	port->portdata = portdata;

	emcuetiti_port_register(broker, port);
}

