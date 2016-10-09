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

#include "emcuetiti_broker.h"
#include "emcuetiti_port.h"

typedef struct {
	const char* clientid;
	uint16_t keepalive;
} emcuetiti_port_sys_connectdata;

typedef struct {
	const char* clientid;
} emcuetiti_port_sys_disconnectdata;

typedef void* (*emcuetiti_port_sys_encode_client_connect)(
		emcuetiti_port_sys_connectdata* connectdata, size_t* len);
typedef void* (*emcuetiti_port_sys_encode_client_disconnect)(
		emcuetiti_port_sys_disconnectdata* disconnectdata, size_t* len);
typedef void (*emcuetiti_port_sys_free)(void* buffer);

typedef struct {
	const emcuetiti_port_sys_encode_client_connect encode_client_connect;
	const emcuetiti_port_sys_encode_client_disconnect encode_client_disconnect;
	const emcuetiti_port_sys_free free;
} emcuetiti_port_sys_ops;

typedef struct {
	emcuetiti_port_sys_ops* ops;
	emcuetiti_topichandle topic_sys;
	emcuetiti_topichandle topic_sys_client;
	emcuetiti_topichandle topic_sys_client_connect;
	emcuetiti_topichandle topic_sys_client_disconnect;
} emcuetiti_port_sys_data;

void emcuetiti_port_sys_new(emcuetiti_brokerhandle* broker,
		emcuetiti_porthandle* port, emcuetiti_port_sys_data* data,
		emcuetiti_port_sys_ops* ops);
