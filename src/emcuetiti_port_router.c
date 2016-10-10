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

#include "emcuetiti_port_router.h"
#include "emcuetiti_log.h"
#include "buffers.h"

static int emcuetiti_port_publishreadycallback(emcuetiti_brokerhandle* broker,
		emcuetiti_topichandle* topic, buffers_buffer* buffer, void* portdata) {

	buffers_buffer_reference bufferreference;
	buffers_buffer_createreference(buffer, &bufferreference);
	buffers_buffer_ref(buffer);

	size_t payloadlen = buffers_buffer_available(&bufferreference.buffer);

	emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG,
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

static const emcuetiti_port_callbacks callbacks = { //
		.publishreadycallback = emcuetiti_port_publishreadycallback };

void emcuetiti_port_router(emcuetiti_brokerhandle* broker,
		emcuetiti_porthandle* port) {
	port->callbacks = &callbacks;
	port->portdata = NULL;
	emcuetiti_port_register(broker, port);
}
