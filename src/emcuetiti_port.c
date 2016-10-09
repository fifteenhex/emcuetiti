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

#include "emcuetiti.h"
#include "emcuetiti_priv.h"
#include "emcuetiti_log.h"
#include "util.h"

void emcuetiti_port_onclientconnected(emcuetiti_brokerhandle* broker) {
#if  EMCUETITI_CONFIG_PORT_CALLBACK_CLIENTCONNECT
	for (int i = 0; i < ARRAY_ELEMENTS(broker->ports); i++) {
		emcuetiti_porthandle* port = broker->ports[i];
		if (port != NULL && port->callbacks->connected != NULL) {
			port->callbacks->connected(broker, port->portdata);
		}
	}
#endif
}

void emcuetiti_port_onclientdisconnected(emcuetiti_brokerhandle* broker) {
#if  EMCUETITI_CONFIG_PORT_CALLBACK_CLIENTDISCONNECT
	for (int i = 0; i < ARRAY_ELEMENTS(broker->ports); i++) {
		emcuetiti_porthandle* port = broker->ports[i];
		if (port != NULL && port->callbacks->disconnected != NULL) {
			port->callbacks->disconnected(broker, port->portdata);
		}
	}
#endif
}

void emcuetiti_port_onpublishready(emcuetiti_brokerhandle* broker,
		emcuetiti_topichandle* topic, buffers_buffer* payload) {
	//TODO replace with generic code
	for (int p = 0; p < ARRAY_ELEMENTS(broker->ports); p++) {
		if (broker->ports[p] != NULL) {
			emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG,
					"dispatching publish to port %d", p);
			if (broker->ports[p]->callbacks->publishreadycallback != NULL)
				broker->ports[p]->callbacks->publishreadycallback(broker, topic,
						payload, broker->ports[p]->portdata);
		}
	}
}

void emcuetiti_port_poll(emcuetiti_brokerhandle* broker,
		emcuetiti_timestamp now) {
	//TODO replace with generic code
	for (int i = 0; i < ARRAY_ELEMENTS(broker->ports); i++) {
		emcuetiti_porthandle* port = broker->ports[i];
		if (port != NULL && port->callbacks->pollfunc != NULL)
			port->callbacks->pollfunc(now, port->portdata);
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
		emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG, "port registered");
	else
		emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG,
				"failed to register port");
}
