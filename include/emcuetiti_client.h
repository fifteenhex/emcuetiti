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

int emcuetiti_client_register(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* handle);
void emcuetiti_client_unregister(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* handle);
int emcuetiti_client_readpublish(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* client, uint8_t* buffer, size_t len);
void emcuetiti_client_poll(emcuetiti_brokerhandle* broker,
		emcuetiti_timestamp now);
libmqtt_writefunc emcuetiti_client_resolvewritefunc(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs);

void emcuetiti_client_dumpstate(emcuetiti_brokerhandle* broker,
		emcuetiti_clientstate* client);
