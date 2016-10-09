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

#define LIBMQTT_MINIMUMPACKETBYTES		2 // the minimum packet size is the type/flags byte and a single byte length

#define LIBMQTT_MINIMUMFIXEDHEADERBYTES 2 //
#define LIBMQTT_MAXIMUMFIXEDHEADERBYTES	5 //

#define LIBMQTT_PACKETYPEANDFLAGS(t, f) ((t << LIBMQTT_PACKETTYPE_SHIFT) | f)

#define LIBMQTT_MQTTSTRLEN(l) (l + 2)

#define LIBMQTT_FLAGS_CONNECT_CLEANSESSION	(1 << 1)
#define LIBMQTT_FLAGS_CONNECT_WILLFLAG		(1 << 2)
#define LIBMQTT_FLAGS_CONNECT_WILLRETAIN	(1 << 5)
#define LIBMQTT_FLAGS_CONNECT_PASSWORDFLAG	(1 << 6)
#define LIBMQTT_FLAGS_CONNECT_USERNAMEFLAG	(1 << 7)

#define LIBMQTT_MSB(a) ((a >> 8) & 0xff)
#define LIBMQTT_LSB(a) (a & 0xff)
