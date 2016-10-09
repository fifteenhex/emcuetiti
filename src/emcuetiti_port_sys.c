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

#include "emcuetiti_broker.h"
#include "emcuetiti_port_sys.h"
#include "emcuetiti_topic.h"
#include "buffers.h"

static void emcuetiti_port_sys_onclientconnected(emcuetiti_brokerhandle* broker,
		emcuetiti_clientstate* clientstate, void* portdata) {

	emcuetiti_port_sys_connectdata connectdata = { //
			.clientid = clientstate->clientid, //
					.keepalive = clientstate->keepalive, //
			};

	emcuetiti_port_sys_data* data = (emcuetiti_port_sys_data*) portdata;
	if (data->ops != NULL && data->ops->encode_client_connect != NULL) {
		size_t payloadlen;
		void* payload = data->ops->encode_client_connect(&connectdata,
				&payloadlen);
		if (payload != NULL) {

			buffers_bufferhead bufferhead;
			buffers_buffer buffer;

			buffers_wrap(&buffer, &bufferhead, payload, &payloadlen);

			emcuetiti_publish publish = { //
					.topic = &data->topic_sys_client_connect, //
							.readfunc = buffers_buffer_readfunc, //
							.resetfunc = buffers_buffer_resetfunc, //
							.userdata = &buffer, //
							.payloadln = payloadlen };
			emcuetiti_broker_publish(broker, &publish);
			data->ops->free(payload);
		}
	}
}

static void emcuetiti_port_sys_onclientdisconnected(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* clientstate,
		void* portdata) {

	emcuetiti_port_sys_disconnectdata disconnectdata = {

	};

	emcuetiti_port_sys_data* data = (emcuetiti_port_sys_data*) portdata;
	if (data->ops != NULL && data->ops->encode_client_disconnect != NULL) {
		size_t payloadlen;
		void* payload = data->ops->encode_client_disconnect(&disconnectdata,
				&payloadlen);
		if (payload != NULL) {
			data->ops->free(payload);
		}
	}
}

static void emcuetiti_port_sys_poll(emcuetiti_timestamp now, void* portdata) {
}

static emcuetiti_port_callbacks callbacks = { //
		.pollfunc = emcuetiti_port_sys_poll, //
				.connected = emcuetiti_port_sys_onclientconnected, //
				.disconnected = emcuetiti_port_sys_onclientdisconnected //
		};

void emcuetiti_port_sys_new(emcuetiti_brokerhandle* broker,
		emcuetiti_porthandle* port, emcuetiti_port_sys_data* data,
		emcuetiti_port_sys_ops* ops) {

	data->ops = ops;

	port->portdata = data;
	port->callbacks = &callbacks;
	emcuetiti_port_register(broker, port);

	emcuetiti_broker_addtopicpart(broker, NULL, &data->topic_sys, "$sys",
	false);
	emcuetiti_broker_addtopicpart(broker, &data->topic_sys,
			&data->topic_sys_client, "client",
			false);
	emcuetiti_broker_addtopicpart(broker, &data->topic_sys_client,
			&data->topic_sys_client_connect, "connect",
			false);
	emcuetiti_broker_addtopicpart(broker, &data->topic_sys_client,
			&data->topic_sys_client_disconnect, "disconnect",
			false);
}
