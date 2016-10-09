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

#include "emcuetiti_port_sys.h"
#include "emcuetiti_topic.h"

// topic parts used
static emcuetiti_topichandle topic_sys;
static emcuetiti_topichandle topic_sys_client;
static emcuetiti_topichandle topic_sys_client_connect;
static emcuetiti_topichandle topic_sys_client_disconnect;

static void emcuetiti_port_sys_onclientconnected(emcuetiti_brokerhandle* broker,
		void* portdata) {

}

static void emcuetiti_port_sys_onclientdisconnected(
		emcuetiti_brokerhandle* broker, void* portdata) {

}

static void emcuetiti_port_sys_poll(emcuetiti_timestamp now, void* portdata) {

}

static emcuetiti_port_callbacks callbacks = { //
		.pollfunc = emcuetiti_port_sys_poll, //
				.connected = emcuetiti_port_sys_onclientconnected, //
				.disconnected = emcuetiti_port_sys_onclientdisconnected //
		};

void emcuetiti_port_sys_new(emcuetiti_brokerhandle* broker,
		emcuetiti_porthandle* port) {
	port->callbacks = &callbacks;
	emcuetiti_port_register(broker, port);

	emcuetiti_broker_addtopicpart(broker, NULL, &topic_sys, "$sys", false);
	emcuetiti_broker_addtopicpart(broker, &topic_sys, &topic_sys_client,
			"client",
			false);
	emcuetiti_broker_addtopicpart(broker, &topic_sys_client,
			&topic_sys_client_connect, "connect",
			false);
	emcuetiti_broker_addtopicpart(broker, &topic_sys_client,
			&topic_sys_client_disconnect, "disconnect",
			false);
}
