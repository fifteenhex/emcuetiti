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
				emcuetiti_port_remote_movetostate(data,
						REMOTEPORTSTATE_CONNECTING);
			else if (ret == EMCUETITI_PORT_REMOTE_ERR)
				emcuetiti_port_remote_movetostate(data, REMOTEPORTSTATE_ERROR);
		}
	}
		break;
	case REMOTEPORTSTATE_CONNECTING: {
		if (data->statedata.connecting.connreqsent) {
			if (emcuetiti_port_remote_readpacket(data,
					&data->statedata.connecting.pktread)) {

				emcuetiti_port_remote_movetostate(data,
						REMOTEPORTSTATE_CONNECTED);

				data->statedata.connected.datalastsent = now;
				data->statedata.connected.datalastreceived = now;
			}
		} else {
			libmqtt_construct_connect(writefunc, data->connectiondata,
					config->keepalive, config->clientid, NULL, NULL, NULL,
					NULL);
			data->statedata.connecting.connreqsent = true;
		}
	}
		break;
	case REMOTEPORTSTATE_CONNECTED: {
		if (emcuetiti_port_remote_readpacket(data,
				&data->statedata.connected.pktread)) {

			if (data->statedata.connected.pktread.state
					== LIBMQTT_PACKETREADSTATE_FINISHED)
				data->statedata.connected.datalastreceived = now;
			else if (data->statedata.connected.pktread.state
					== LIBMQTT_PACKETREADSTATE_ERROR) {
				emcuetiti_port_remote_movetostate(data, REMOTEPORTSTATE_ERROR);
				break;
			}

		}

		if (config->keepalive > 0) {
			emcuetiti_timestamp timesincelastrecv = now
					- data->statedata.connected.datalastreceived;
			emcuetiti_timestamp timesincelastsend = now
					- data->statedata.connected.datalastsent;

			if (timesincelastrecv >= (data->config->keepalive * 2))
				emcuetiti_port_remote_movetostate(data, REMOTEPORTSTATE_ERROR);
			else {
				bool sendping = timesincelastsend >= data->config->keepalive
						|| timesincelastrecv >= data->config->keepalive;
				if (sendping) {
					libmqtt_construct_pingreq(writefunc, data->connectiondata);
					data->statedata.connected.datalastsent = now;
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

