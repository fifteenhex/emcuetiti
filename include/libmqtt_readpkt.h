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

#include "buffers.h"

typedef struct {
	libmqtt_packetread_state state;
	libmqtt_packetread_registers varhdr;
	// secret, don't touch
	size_t pos;
	unsigned lenmultiplier;
	uint8_t counter;
	BUFFERS_STATICBUFFER(buffer, 64);
} libmqtt_packetread;

typedef int (*libmqtt_packetreadchange)(libmqtt_packetread* pkt,
		libmqtt_packetread_state previousstate, void* userdata);

int libmqtt_readpkt(libmqtt_packetread* pkt, // pkt being read
		libmqtt_packetreadchange changefunc, void* changeuserdata, // pkt state change callback
		libmqtt_readfunc readfunc, void* readuserdata, // func and data for reading the packet in
		libmqtt_writefunc payloadwritefunc, void* payloadwriteuserdata); // func and data for writing the payload out
