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

#pragma once

#include "emcuetiti_types.h"

void emcuetiti_port_onpublishready(emcuetiti_brokerhandle* broker,
		emcuetiti_topichandle* topic, buffers_buffer* payload);

void emcuetiti_port_onclientconnected(emcuetiti_brokerhandle* broker,
		emcuetiti_clientstate* clientstate);
void emcuetiti_port_onclientdisconnected(emcuetiti_brokerhandle* broker,
		emcuetiti_clientstate* clientstate);

void emcuetiti_port_register(emcuetiti_brokerhandle* broker,
		emcuetiti_porthandle* port);

void emcuetiti_port_poll(emcuetiti_brokerhandle* broker,
		emcuetiti_timestamp now);

