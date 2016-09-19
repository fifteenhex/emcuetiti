#include <stdio.h>
#include <string.h>

#include "emcuetiti_port_router.h"
#include "emcuetiti_client.h"
#include "emcuetiti_broker.h"
#include "buffers.h"

static int emcuetiti_port_publishreadycallback(emcuetiti_brokerhandle* broker,
		emcuetiti_topichandle* topic, buffers_buffer* buffer) {

	broker->callbacks->log(broker, "publish ready");

	buffers_buffer_reference bufferreference;
	buffers_buffer_createreference(buffer, &bufferreference);

	emcuetiti_publish pub = { .topic = topic, //
			.readfunc = buffers_buffer_readfunc, //
			.resetfunc = buffers_buffer_resetfunc, //
			.userdata = &bufferreference.buffer, //
			.payloadln = buffers_buffer_available(buffer) };

	emcuetiti_broker_publish(broker, &pub);

	buffers_buffer_unref(&bufferreference.buffer);

	return 0;
}

void emcuetiti_port_router(emcuetiti_brokerhandle* broker,
		emcuetiti_porthandle* port) {

	port->pollfunc = NULL;
	port->publishreadycallback = emcuetiti_port_publishreadycallback;
	port->portdata = NULL;

	emcuetiti_port_register(broker, port);
}
