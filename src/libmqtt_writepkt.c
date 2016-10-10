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

#include <string.h>
#include <stdio.h>

#include "libmqtt.h"
#include "libmqtt_priv.h"
#include "libmqtt_writepkt.h"

void libmqtt_writepkt_reset(libmqtt_packetwrite* pkt) {
	memset(pkt, 0, sizeof(*pkt));
	pkt->state = LIBMQTT_PACKETREADSTATE_IDLE;
}

static void libmqtt_writepkt_simple(libmqtt_packetwrite* pkt, uint8_t type) {
	pkt->state = LIBMQTT_PACKETREADSTATE_TYPE;
	pkt->varhdr.common.type = type;
	pkt->varhdr.common.flags = 0;
	pkt->varhdr.common.length = 2;
}

static void libmqtt_writepkt_justmsgid(libmqtt_packetwrite* pkt, uint8_t type,
		uint8_t flags, uint16_t msgid) {
	pkt->state = LIBMQTT_PACKETREADSTATE_TYPE;
	pkt->varhdr.common.type = type;
	pkt->varhdr.common.flags = flags;
	pkt->varhdr.common.length = 4;
	pkt->varhdr.justmsgid.msgid = msgid;
}

void libmqtt_writepkt_puback(libmqtt_packetwrite* pkt, uint16_t messageid) {
	libmqtt_writepkt_justmsgid(pkt, LIBMQTT_PACKETTYPE_PUBACK, 0, messageid);
}

void libmqtt_writepkt_pubrec(libmqtt_packetwrite* pkt, uint16_t messageid) {
	libmqtt_writepkt_justmsgid(pkt, LIBMQTT_PACKETTYPE_PUBREC, 0, messageid);
}

void libmqtt_writepkt_pubrel(libmqtt_packetwrite* pkt, uint16_t messageid) {
	libmqtt_writepkt_justmsgid(pkt, LIBMQTT_PACKETTYPE_PUBREL, 0, messageid);
}

void libmqtt_writepkt_pubcomp(libmqtt_packetwrite* pkt, uint16_t messageid) {
	libmqtt_writepkt_justmsgid(pkt, LIBMQTT_PACKETTYPE_PUBCOMP, 0, messageid);
}

void libmqtt_writepkt_unsuback(libmqtt_packetwrite* pkt, uint16_t messageid) {
	libmqtt_writepkt_justmsgid(pkt, LIBMQTT_PACKETTYPE_UNSUBACK, 0, messageid);
}

void libmqtt_writepkt_pingreq(libmqtt_packetwrite* pkt) {
	libmqtt_writepkt_simple(pkt, LIBMQTT_PACKETTYPE_PINGREQ);
}

void libmqtt_writepkt_pingresp(libmqtt_packetwrite* pkt) {
	libmqtt_writepkt_simple(pkt, LIBMQTT_PACKETTYPE_PINGRESP);
}

void libmqtt_writepkt_disconnect(libmqtt_packetwrite* pkt) {
	libmqtt_writepkt_simple(pkt, LIBMQTT_PACKETTYPE_DISCONNECT);
}

static int libmqtt_writepkt_changestate(libmqtt_packetwrite* pkt,
		libmqtt_packetwritechange changefunc, libmqtt_packetread_state newstate,
		void* userdata) {
	int ret = 0;
	libmqtt_packetread_state previousstate = pkt->state;

	pkt->counter = 0;
	pkt->state = newstate;

// if all of the packet has been read move to finished instead of the next state
	if (pkt->state > LIBMQTT_PACKETREADSTATE_LEN
			&& pkt->state < LIBMQTT_PACKETREADSTATE_FINISHED)
		if (pkt->pos == pkt->varhdr.common.length)
			pkt->state = LIBMQTT_PACKETREADSTATE_FINISHED;

	if (changefunc != NULL)
		ret = changefunc(pkt, previousstate, userdata);

	return ret;
}

static void libmqtt_writepkt_incposandcounter(libmqtt_packetwrite* pkt, int by) {
	pkt->pos += by;
	pkt->counter += by;
}

static int libmqtt_writepkt_write(libmqtt_packetwrite* pkt,
		libmqtt_writefunc writefunc, void* writeuserdata, void* buff,
		size_t len) {
	int ret = writefunc(writeuserdata, buff, len);
	if (ret > 0)
		libmqtt_writepkt_incposandcounter(pkt, ret);
	return ret;
}

static int libmqtt_writepkt_u16(libmqtt_packetwrite* pkt,
		libmqtt_packetwritechange changefunc,
		void* changeuserdata, 	//
		libmqtt_writefunc writefunc, void* writeuserdata, uint16_t value,
		libmqtt_packetread_state nextstate) {

	uint8_t b = (value >> (8 * (1 - pkt->counter))) & 0xff;
	int ret = libmqtt_writepkt_write(pkt, writefunc, writeuserdata, &b, 1);
	if (ret == 1) {
		if (pkt->counter == 2)
			libmqtt_writepkt_changestate(pkt, changefunc, nextstate,
					changeuserdata);
	};
	return ret;
}

static int libmqtt_writepkt_u8(libmqtt_packetwrite* pkt,
		libmqtt_packetwritechange changefunc,
		void* changeuserdata, 	//
		libmqtt_writefunc writefunc, void* writeuserdata, uint8_t b,
		libmqtt_packetread_state nextstate) {
	int ret = libmqtt_writepkt_write(pkt, writefunc, writeuserdata, &b, 1);
	if (pkt->counter == 1)
		libmqtt_writepkt_changestate(pkt, changefunc, nextstate,
				changeuserdata);
	return ret;
}

static int libmqtt_writepkt_commonstates(libmqtt_packetwrite* pkt, 			//
		libmqtt_packetwritechange changefunc, void* changeuserdata, 	//
		libmqtt_writefunc writefunc, void* writeuserdata, 				//
		libmqtt_readfunc payloadreadfunc, void* payloadreaduserdata) {

	int ret = 0;
	switch (pkt->state) {
	case LIBMQTT_PACKETREADSTATE_TYPE: {
		uint8_t typeandflags = LIBMQTT_PACKETYPEANDFLAGS(
				pkt->varhdr.common.type, pkt->varhdr.common.flags);
		ret = libmqtt_writepkt_u8(pkt, changefunc, changeuserdata, writefunc,
				writeuserdata, typeandflags, LIBMQTT_PACKETREADSTATE_LEN);
	}
		break;
	case LIBMQTT_PACKETREADSTATE_LEN: {
		uint8_t len = pkt->varhdr.common.length - 2;
		ret = libmqtt_writepkt_u8(pkt, changefunc, changeuserdata, writefunc,
				writeuserdata, len, LIBMQTT_PACKETREADSTATE_MSGID);
	}
		break;
	case LIBMQTT_PACKETREADSTATE_MSGID:
		ret = libmqtt_writepkt_u16(pkt, changefunc, changeuserdata, writefunc,
				writeuserdata, pkt->varhdr.justmsgid.msgid,
				LIBMQTT_PACKETREADSTATE_FINISHED);
		break;
	}
	return ret;
}

int libmqtt_writepkt(libmqtt_packetwrite* pkt, //
		libmqtt_packetwritechange changefunc, void* changeuserdata, //
		libmqtt_writefunc writefunc, void* writeuserdata, //
		libmqtt_readfunc payloadreadfunc, void* payloadreaduserdata) {

	int ret = 0;

	if (pkt->state == LIBMQTT_PACKETREADSTATE_IDLE
			|| pkt->state >= LIBMQTT_PACKETREADSTATE_FINISHED)
		goto out;

	while (pkt->state < LIBMQTT_PACKETREADSTATE_FINISHED && ret >= 0) {
		printf("pw - s %d - %d %d %d\n", pkt->state, pkt->varhdr.common.type,
				pkt->varhdr.common.length, pkt->pos);
		if (pkt->state < LIBMQTT_PACKETREADSTATE_COMMON_END)
			ret = libmqtt_writepkt_commonstates(pkt, changefunc, changeuserdata,
					writefunc, writeuserdata, payloadreadfunc,
					payloadreaduserdata);
	}

	if (ret <= LIBMQTT_EFATAL)
		libmqtt_writepkt_changestate(pkt, changefunc,
				LIBMQTT_PACKETREADSTATE_ERROR, changeuserdata);

	out: return ret;
}
