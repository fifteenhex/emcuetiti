#include <stdio.h>
#include <string.h>

#include "emcuetiti_port_router.h"
#include "emcuetiti_client.h"
#include "emcuetiti_broker.h"
#include "buffers.h"

static int emcuetiti_port_publishreadycallback(emcuetiti_brokerhandle* broker,
		emcuetiti_topichandle* topic, buffers_buffer* buffer, void* portdata) {

	buffers_buffer_reference bufferreference;
	buffers_buffer_createreference(buffer, &bufferreference);
	buffers_buffer_ref(buffer);

	size_t payloadlen = buffers_buffer_available(&bufferreference.buffer);

	broker->callbacks->log(broker,
			"routing publish to clients, have %d payload bytes %d %d",
			payloadlen, *buffer->refs, *bufferreference.buffer.refs);

	emcuetiti_publish pub = { .topic = topic, //
			.readfunc = buffers_buffer_readfunc, //
			.resetfunc = buffers_buffer_resetfunc, //
			.userdata = &bufferreference.buffer, //
			.payloadln = payloadlen };

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
