#include <stdio.h>
#include <glib.h>
#include <string.h>

#include "emcuetiti_port_router.h"
#include "emcuetiti_client.h"
#include "emcuetiti_broker.h"

static int emcuetiti_router_readfunc(void* userdata, uint8_t* buffer,
		size_t len) {
	emcuetiti_bufferholder* buffholder = (emcuetiti_bufferholder*) userdata;
	memcpy(buffer, buffholder->buffer + buffholder->readpos, len);
	return len;
}

static void emcuetiti_router_resetfunc(void* userdata) {
	emcuetiti_bufferholder* buffholder = (emcuetiti_bufferholder*) userdata;
	buffholder->readpos = 0;
}

static int emcuetiti_port_publishreadycallback(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* client, emcuetiti_topichandle* topic,
		size_t publishlen) {
	printf("publish ready\n");
	uint8_t* buffer = g_malloc(publishlen + 1);
	int read = emcuetiti_client_readpublish(broker, client, buffer, publishlen);
	buffer[read] = '\0';
	printf("publish %s\n", (char *) buffer);

	emcuetiti_bufferholder bh = { .buffer = buffer, .len = publishlen,
			.readpos = 0 };

	emcuetiti_publish pub = { .topic = topic, //
			.readfunc = emcuetiti_router_readfunc, //
			.resetfunc = emcuetiti_router_resetfunc, //
			.userdata = &bh, //
			.payloadln = publishlen };

	emcuetiti_broker_publish(broker, &pub);

	g_free(buffer);

	return 0;
}

void emcuetiti_port_router(emcuetiti_brokerhandle* broker,
		emcuetiti_porthandle* port) {

	port->publishreadycallback = emcuetiti_port_publishreadycallback;

	emcuetiti_port_register(broker, port);
}
