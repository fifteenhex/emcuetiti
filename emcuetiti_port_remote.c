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

	if (pkt->state == LIBMQTT_PACKETREADSTATE_FINISHED)
		printf("type %d\n", pkt->type);

	return pkt->state == LIBMQTT_PACKETREADSTATE_FINISHED;
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
		int ret = config->hostops->connect(config->host, config->port,
				&data->connectiondata);
		if (ret == EMCUETITI_PORT_REMOTE_OK)
			emcuetiti_port_remote_movetostate(data, REMOTEPORTSTATE_CONNECTING);
		else if (ret == EMCUETITI_PORT_REMOTE_ERR)
			emcuetiti_port_remote_movetostate(data, REMOTEPORTSTATE_ERROR);
	}
		break;
	case REMOTEPORTSTATE_CONNECTING: {
		if (data->statedata.connecting.connreqsent) {
			if (emcuetiti_port_remote_readpacket(data,
					&data->statedata.connecting.pktread)) {

				emcuetiti_port_remote_movetostate(data,
						REMOTEPORTSTATE_CONNECTED);
				data->statedata.connected.datalastsent = now;
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

		}

		if (config->keepalive > 0) {
			emcuetiti_timestamp timesincelastsend = now
					- data->statedata.connected.datalastsent;
			if (timesincelastsend >= data->config->keepalive) {
				libmqtt_construct_pingreq(writefunc, data->connectiondata);
				data->statedata.connected.datalastsent = now;
			}
		}
	}
		break;
	case REMOTEPORTSTATE_DISCONNECTED:
		break;
	case REMOTEPORTSTATE_ERROR:
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

