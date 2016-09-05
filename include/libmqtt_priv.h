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
