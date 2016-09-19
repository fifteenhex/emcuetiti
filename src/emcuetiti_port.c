#include <stdio.h>

#include "emcuetiti.h"
#include "emcuetiti_priv.h"
#include "util.h"

void emcuetiti_port_onpublishready(emcuetiti_brokerhandle* broker,
		emcuetiti_topichandle* topic, buffers_buffer* payload) {
	for (int p = 0; p < ARRAY_ELEMENTS(broker->ports); p++) {
		if (broker->ports[p] != NULL) {
			broker->callbacks->log(broker, "dispatching publish to port %d", p);
			/*if (broker->ports[p]->publishreadycallback != NULL)
			 broker->ports[p]->publishreadycallback(broker, cs->client, t,
			 cs->publishpayloadlen);*/
		}
	}
}

void emcuetiti_port_poll(emcuetiti_brokerhandle* broker,
		emcuetiti_timestamp now) {
	for (int i = 0; i < ARRAY_ELEMENTS(broker->ports); i++) {
		emcuetiti_porthandle* port = broker->ports[i];
		if (port != NULL && port->pollfunc != NULL)
			port->pollfunc(now, port->portdata);
	}
}

void emcuetiti_port_register(emcuetiti_brokerhandle* broker,
		emcuetiti_porthandle* port) {

	bool registered = false;

	for (int i = 0; i < ARRAY_ELEMENTS(broker->ports); i++) {
		if (broker->ports[i] == NULL) {
			broker->ports[i] = port;
			registered = true;
			break;
		}
	}

	if (registered)
		printf("port registered\n");
	else
		printf("failed to register port\n");
}
