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

void emcuetiti_broker_poll(emcuetiti_brokerhandle* broker);
void emcuetiti_broker_addtopicpart(emcuetiti_brokerhandle* broker,
		emcuetiti_topichandle* root, emcuetiti_topichandle* part,
		const char* topicpart, bool targetable);
void emcuetiti_broker_init(emcuetiti_brokerhandle* broker);
void emcuetiti_broker_dumpstate(emcuetiti_brokerhandle* broker);

void emcuetiti_broker_publish(emcuetiti_brokerhandle* broker,
		emcuetiti_publish* publish);
bool emcuetiti_broker_canacceptmoreclients(emcuetiti_brokerhandle* broker);
uint8_t* emcuetiti_broker_getpayloadbuffer(emcuetiti_brokerhandle* broker,
		size_t* size);
