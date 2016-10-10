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
#include <assert.h>

#include "buffers.h"
#include "libmqtt_priv.h"

#include "libmqtt.h"
#include "libmqtt_readpkt.h"

static int libmqtt_readpkt_changestate(libmqtt_packetread* pkt,
		libmqtt_packetreadchange changefunc, libmqtt_packetread_state newstate,
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

static void libmqtt_readpkt_incposandcounter(libmqtt_packetread* pkt, int by) {
	pkt->pos += by;
	pkt->counter += by;
}

static int libmqtt_readpkt_read(libmqtt_packetread* pkt,
		libmqtt_readfunc readfunc, void* readuserdata, void* buff, size_t len) {
	int ret = readfunc(readuserdata, buff, len);
	if (ret > 0)
		libmqtt_readpkt_incposandcounter(pkt, ret);
	return ret;
}

static int libmqtt_readpkt_u8(libmqtt_packetread* pkt,
		libmqtt_packetreadchange changefunc,
		void* changeuserdata, 	//
		libmqtt_readfunc readfunc, void* readuserdata, uint8_t* target,
		libmqtt_packetread_state nextstate) {
	uint8_t b;
	int ret = libmqtt_readpkt_read(pkt, readfunc, readuserdata, &b, 1);
	if (ret == 1) {
		*target = b;
		if (pkt->counter == 1)
			libmqtt_readpkt_changestate(pkt, changefunc, nextstate,
					changeuserdata);
	};
	return ret;
}

static int libmqtt_readpkt_u16(libmqtt_packetread* pkt,
		libmqtt_packetreadchange changefunc,
		void* changeuserdata, 	//
		libmqtt_readfunc readfunc, void* readuserdata, uint16_t* target,
		libmqtt_packetread_state nextstate) {
	uint8_t b;
	int ret = libmqtt_readpkt_read(pkt, readfunc, readuserdata, &b, 1);
	if (ret == 1) {
		*target = (*target << 8) | b;
		if (pkt->counter == 2)
			libmqtt_readpkt_changestate(pkt, changefunc, nextstate,
					changeuserdata);
	};
	return ret;
}

static int libmqtt_readpkt_string(libmqtt_packetread* pkt,
		libmqtt_packetreadchange changefunc, void* changeuserdata, 	//
		libmqtt_readfunc readfunc, void* readuserdata, //
		libmqtt_writefunc payloadwritefunc, void* payloadwriteuserdata, //
		uint16_t len, libmqtt_packetread_state nextstate) {

	uint8_t b;
	int ret = libmqtt_readpkt_read(pkt, readfunc, readuserdata, &b, 1);
	if (ret == 1) {
		if (payloadwritefunc != NULL)
			payloadwritefunc(payloadwriteuserdata, &b, 1);
		if (pkt->counter == len) {
			libmqtt_readpkt_changestate(pkt, changefunc, nextstate,
					changeuserdata);
		}
	};
	return ret;
}

static int libmqtt_readpkt_connectstates(libmqtt_packetread* pkt, 			//
		libmqtt_packetreadchange changefunc, void* changeuserdata, 	//
		libmqtt_readfunc readfunc, void* readuserdata, 				//
		libmqtt_writefunc payloadwritefunc, void* payloadwriteuserdata) {

	int ret = 0;

	switch (pkt->state) {
	// connect var header

	case LIBMQTT_PACKETREADSTATE_CONNECT_PROTOLEN: {
		ret = libmqtt_readpkt_u16(pkt, changefunc, changeuserdata, readfunc,
				readuserdata, &pkt->varhdr.publish.topiclen,
				LIBMQTT_PACKETREADSTATE_CONNECT_PROTO);
	}
		break;
	case LIBMQTT_PACKETREADSTATE_CONNECT_PROTO: {
		uint8_t topicbyte;
		ret = libmqtt_readpkt_read(pkt, readfunc, readuserdata, &topicbyte, 1);
		if (ret == 1) {
			//payloadwritefunc(payloadwriteuserdata, &topicbyte, 1);
			if (pkt->counter == pkt->varhdr.publish.topiclen) {
				libmqtt_readpkt_changestate(pkt, changefunc,
						LIBMQTT_PACKETREADSTATE_CONNECT_PROTOLEVEL,
						changeuserdata);
			}
		};
	}
		break;

	case LIBMQTT_PACKETREADSTATE_CONNECT_PROTOLEVEL:
		ret = libmqtt_readpkt_u8(pkt, changefunc, changeuserdata, readfunc,
				readuserdata, &pkt->varhdr.connect.level,
				LIBMQTT_PACKETREADSTATE_CONNECT_FLAGS);
		break;

	case LIBMQTT_PACKETREADSTATE_CONNECT_FLAGS:
		ret = libmqtt_readpkt_u8(pkt, changefunc, changeuserdata, readfunc,
				readuserdata, &pkt->varhdr.connect.flags,
				LIBMQTT_PACKETREADSTATE_CONNECT_KEEPALIVE);
		break;

	case LIBMQTT_PACKETREADSTATE_CONNECT_KEEPALIVE:
		ret = libmqtt_readpkt_u16(pkt, changefunc, changeuserdata, readfunc,
				readuserdata, &pkt->varhdr.connect.keepalive,
				LIBMQTT_PACKETREADSTATE_CONNECT_CLIENTIDLEN);
		break;

		// connect payload
	case LIBMQTT_PACKETREADSTATE_CONNECT_CLIENTIDLEN:
		ret = libmqtt_readpkt_u16(pkt, changefunc, changeuserdata, readfunc,
				readuserdata, &pkt->varhdr.connect.clientidlen,
				LIBMQTT_PACKETREADSTATE_CONNECT_CLIENTID);
		break;

	case LIBMQTT_PACKETREADSTATE_CONNECT_CLIENTID:
		ret = libmqtt_readpkt_string(pkt, changefunc, changeuserdata, readfunc,
				readuserdata, payloadwritefunc, payloadwriteuserdata,
				pkt->varhdr.connect.clientidlen,
				LIBMQTT_PACKETREADSTATE_PUBLISH_PAYLOAD);
		break;
		//
	}
	return ret;
}

static int libmqtt_readpkt_publishstates(libmqtt_packetread* pkt, 			//
		libmqtt_packetreadchange changefunc, void* changeuserdata, 	//
		libmqtt_readfunc readfunc, void* readuserdata, 				//
		libmqtt_writefunc payloadwritefunc, void* payloadwriteuserdata) {

	assert(pkt->varhdr.common.type == LIBMQTT_PACKETTYPE_PUBLISH);

	int ret = 0;

	switch (pkt->state) {
	case LIBMQTT_PACKETREADSTATE_PUBLISH_TOPICLEN: {
		libmqtt_readpkt_u16(pkt, changefunc, changeuserdata, readfunc,
				readuserdata, &pkt->varhdr.publish.topiclen,
				LIBMQTT_PACKETREADSTATE_PUBLISH_TOPIC);
	}
		break;
	case LIBMQTT_PACKETREADSTATE_PUBLISH_TOPIC: {
		bool publishhasmsgid = ((pkt->varhdr.common.flags >> 1) & 0x3);
		libmqtt_packetread_state nextstate =
				publishhasmsgid ?
						LIBMQTT_PACKETREADSTATE_PUBLISH_MSGID :
						LIBMQTT_PACKETREADSTATE_PUBLISH_PAYLOAD;
		ret = libmqtt_readpkt_string(pkt, changefunc, changeuserdata, readfunc,
				readuserdata, payloadwritefunc, payloadwriteuserdata,
				pkt->varhdr.publish.topiclen, nextstate);
	}
		break;
	case LIBMQTT_PACKETREADSTATE_PUBLISH_MSGID:
		ret = libmqtt_readpkt_u16(pkt, changefunc, changeuserdata, readfunc,
				readuserdata, &pkt->varhdr.publish.msgid,
				LIBMQTT_PACKETREADSTATE_PUBLISH_PAYLOAD);
		break;
	case LIBMQTT_PACKETREADSTATE_PUBLISH_PAYLOAD: {
		BUFFERS_STATICBUFFER_TO_BUFFER(pkt->buffer, pktbuffer);
		// try to read
		size_t remaining = (pkt->varhdr.common.length - pkt->pos)
				- buffers_buffer_available(&pktbuffer);

		if (remaining > 0) {
			ret = buffers_buffer_fill(&pktbuffer, remaining, readfunc,
					readuserdata);
			if (ret < 0)
				break;
		}

		// try to flush
		ret = buffers_buffer_flush(&pktbuffer, payloadwritefunc,
				payloadwriteuserdata);
		if (ret > 0)
			libmqtt_readpkt_incposandcounter(pkt, ret);

		libmqtt_readpkt_changestate(pkt, changefunc,
				LIBMQTT_PACKETREADSTATE_PUBLISH_PAYLOAD, changeuserdata);
	}
		break;
	}
	return ret;
}

static int libmqtt_readpkt_subscribestates(libmqtt_packetread* pkt, 		//
		libmqtt_packetreadchange changefunc, void* changeuserdata, 	//
		libmqtt_readfunc readfunc, void* readuserdata, 				//
		libmqtt_writefunc payloadwritefunc, void* payloadwriteuserdata) {

	int ret = 0;

	switch (pkt->state) {
	case LIBMQTT_PACKETREADSTATE_SUBSCRIBE_MSGID:
		ret = libmqtt_readpkt_u16(pkt, changefunc, changeuserdata, readfunc,
				readuserdata, &pkt->varhdr.subscribe.msgid,
				LIBMQTT_PACKETREADSTATE_SUBSCRIBE_TOPICLEN);
		break;
	case LIBMQTT_PACKETREADSTATE_SUBSCRIBE_TOPICLEN:
		ret = libmqtt_readpkt_u16(pkt, changefunc, changeuserdata, readfunc,
				readuserdata, &pkt->varhdr.subscribe.topicfilterlen,
				LIBMQTT_PACKETREADSTATE_SUBSCRIBE_TOPICFILTER);
		break;
	case LIBMQTT_PACKETREADSTATE_SUBSCRIBE_TOPICFILTER:
		ret = libmqtt_readpkt_string(pkt, changefunc, changeuserdata, readfunc,
				readuserdata, payloadwritefunc, payloadwriteuserdata,
				pkt->varhdr.subscribe.topicfilterlen,
				LIBMQTT_PACKETREADSTATE_SUBSCRIBE_QOS);
		break;
	case LIBMQTT_PACKETREADSTATE_SUBSCRIBE_QOS:
		ret = libmqtt_readpkt_u8(pkt, changefunc, changeuserdata, readfunc,
				readuserdata, &pkt->varhdr.subscribe.topicfilterqos,
				LIBMQTT_PACKETREADSTATE_SUBSCRIBE_TOPICLEN);
		break;
	}
	return ret;
}

static int libmqtt_readpkt_subackstates(libmqtt_packetread* pkt, 		//
		libmqtt_packetreadchange changefunc, void* changeuserdata, 	//
		libmqtt_readfunc readfunc, void* readuserdata, 				//
		libmqtt_writefunc payloadwritefunc, void* payloadwriteuserdata) {

	int ret = 0;

	switch (pkt->state) {
	case LIBMQTT_PACKETREADSTATE_SUBACK_MSGID:
		ret = libmqtt_readpkt_u16(pkt, changefunc, changeuserdata, readfunc,
				readuserdata, &pkt->varhdr.suback.msgid,
				LIBMQTT_PACKETREADSTATE_SUBACK_RESULT);
		break;
	case LIBMQTT_PACKETREADSTATE_SUBACK_RESULT:
		ret = libmqtt_readpkt_u8(pkt, changefunc, changeuserdata, readfunc,
				readuserdata, &pkt->varhdr.suback.result,
				LIBMQTT_PACKETREADSTATE_SUBACK_RESULT);
		break;
	}
	return ret;
}

static int libmqtt_readpkt_unsubscribestates(libmqtt_packetread* pkt, 		//
		libmqtt_packetreadchange changefunc, void* changeuserdata, 	//
		libmqtt_readfunc readfunc, void* readuserdata, 				//
		libmqtt_writefunc payloadwritefunc, void* payloadwriteuserdata) {

	int ret = 0;

	switch (pkt->state) {
	case LIBMQTT_PACKETREADSTATE_UNSUBSCRIBE_MSGID:
		ret = libmqtt_readpkt_u16(pkt, changefunc, changeuserdata, readfunc,
				readuserdata, &pkt->varhdr.unsubscribe.msgid,
				LIBMQTT_PACKETREADSTATE_UNSUBSCRIBE_TOPICLEN);
		break;
	case LIBMQTT_PACKETREADSTATE_UNSUBSCRIBE_TOPICLEN:
		ret = libmqtt_readpkt_u16(pkt, changefunc, changeuserdata, readfunc,
				readuserdata, &pkt->varhdr.unsubscribe.topicfilterlen,
				LIBMQTT_PACKETREADSTATE_UNSUBSCRIBE_TOPICFILTER);
		break;
	case LIBMQTT_PACKETREADSTATE_UNSUBSCRIBE_TOPICFILTER:
		ret = libmqtt_readpkt_string(pkt, changefunc, changeuserdata, readfunc,
				readuserdata, payloadwritefunc, payloadwriteuserdata,
				pkt->varhdr.unsubscribe.topicfilterlen,
				LIBMQTT_PACKETREADSTATE_UNSUBSCRIBE_TOPICLEN);
		break;
	}
	return ret;
}

static int libmqtt_readpkt_connackstates(libmqtt_packetread* pkt, 			//
		libmqtt_packetreadchange changefunc, void* changeuserdata, 	//
		libmqtt_readfunc readfunc, void* readuserdata, 				//
		libmqtt_writefunc payloadwritefunc, void* payloadwriteuserdata) {

	int ret = 0;

	switch (pkt->state) {
	case LIBMQTT_PACKETREADSTATE_CONNACK_FLAGS:
		ret = libmqtt_readpkt_u8(pkt, changefunc, changeuserdata, readfunc,
				readuserdata, &pkt->varhdr.connack.ackflags,
				LIBMQTT_PACKETREADSTATE_CONNACK_RETCODE);
		break;
	case LIBMQTT_PACKETREADSTATE_CONNACK_RETCODE:
		ret = libmqtt_readpkt_u8(pkt, changefunc, changeuserdata, readfunc,
				readuserdata, &pkt->varhdr.connack.returncode,
				LIBMQTT_PACKETREADSTATE_FINISHED);
		break;
	}
	return ret;
}

static int libmqtt_readpkt_commonstates(libmqtt_packetread* pkt, 			//
		libmqtt_packetreadchange changefunc, void* changeuserdata, 	//
		libmqtt_readfunc readfunc, void* readuserdata, 				//
		libmqtt_writefunc payloadwritefunc, void* payloadwriteuserdata) {

	int ret = 0;

	switch (pkt->state) {
	case LIBMQTT_PACKETREADSTATE_IDLE: {
		libmqtt_readpkt_changestate(pkt, changefunc,
				LIBMQTT_PACKETREADSTATE_TYPE, changeuserdata);
	}
		break;
	case LIBMQTT_PACKETREADSTATE_TYPE: {
		uint8_t typeandflags;
		ret = libmqtt_readpkt_read(pkt, readfunc, readuserdata, &typeandflags,
				1);
		if (ret == 1) {
			pkt->varhdr.common.type = LIBMQTT_PACKETTYPEFROMPACKETTYPEANDFLAGS(
					typeandflags);

			// check for valid packet type
			if (pkt->varhdr.common.type >= LIBMQTT_PACKETTYPE_CONNECT
					&& pkt->varhdr.common.type <= LIBMQTT_PACKETTYPE_DISCONNECT) {
				pkt->varhdr.common.flags =
						LIBMQTT_PACKETFLAGSFROMPACKETTYPEANDFLAGS(typeandflags);
				pkt->varhdr.common.length = 1;
				pkt->lenmultiplier = 1;
				libmqtt_readpkt_changestate(pkt, changefunc,
						LIBMQTT_PACKETREADSTATE_LEN, changeuserdata);
			} else
				libmqtt_readpkt_changestate(pkt, changefunc,
						LIBMQTT_PACKETREADSTATE_ERROR, changeuserdata);
		}
	}
		break;
	case LIBMQTT_PACKETREADSTATE_LEN: {
		uint8_t lenbyte;
		ret = libmqtt_readpkt_read(pkt, readfunc, readuserdata, &lenbyte, 1);
		if (ret == 1) {
			pkt->varhdr.common.length += 1
					+ (LIBMQTT_LEN(lenbyte) * pkt->lenmultiplier);
			pkt->lenmultiplier *= 128;
			// last byte of the length has been read
			if (LIBMQTT_ISLASTLENBYTE(lenbyte)) {
				libmqtt_packetread_state nextstate = libmqtt_nextstateafterlen(
						pkt->varhdr.common.type, pkt->varhdr.common.length,
						pkt->pos);
				libmqtt_readpkt_changestate(pkt, changefunc, nextstate,
						changeuserdata);
			}
		}
	}
		break;

	case LIBMQTT_PACKETREADSTATE_MSGID: {
		ret = libmqtt_readpkt_u16(pkt, changefunc, changeuserdata, readfunc,
				readuserdata, &pkt->varhdr.justmsgid.msgid,
				LIBMQTT_PACKETREADSTATE_FINISHED);
	}
		break;
	}
	return ret;
}

int libmqtt_readpkt(libmqtt_packetread* pkt, 						//
		libmqtt_packetreadchange changefunc, void* changeuserdata, 	//
		libmqtt_readfunc readfunc, void* readuserdata, 				//
		libmqtt_writefunc payloadwritefunc, void* payloadwriteuserdata) {

	int ret = 0;

// reset a finished or error'd packet
	if (pkt->state >= LIBMQTT_PACKETREADSTATE_FINISHED)
		memset(pkt, 0, sizeof(*pkt));

	while (pkt->state < LIBMQTT_PACKETREADSTATE_FINISHED && ret >= 0) {
		printf("pr - s %d - %d %d %d\n", pkt->state, pkt->varhdr.common.type,
				pkt->varhdr.common.length, pkt->pos);

		if (pkt->state < LIBMQTT_PACKETREADSTATE_COMMON_END)
			ret = libmqtt_readpkt_commonstates(pkt, changefunc, changeuserdata,
					readfunc, readuserdata, payloadwritefunc,
					payloadwriteuserdata);
		else if (pkt->state < LIBMQTT_PACKETREADSTATE_CONNECT_END)
			ret = libmqtt_readpkt_connectstates(pkt, changefunc, changeuserdata,
					readfunc, readuserdata, payloadwritefunc,
					payloadwriteuserdata);
		else if (pkt->state < LIBMQTT_PACKETREADSTATE_CONNACK_END)
			ret = libmqtt_readpkt_connackstates(pkt, changefunc, changeuserdata,
					readfunc, readuserdata, payloadwritefunc,
					payloadwriteuserdata);
		else if (pkt->state < LIBMQTT_PACKETREADSTATE_SUBSCRIBE_END)
			ret = libmqtt_readpkt_subscribestates(pkt, changefunc,
					changeuserdata, readfunc, readuserdata, payloadwritefunc,
					payloadwriteuserdata);
		else if (pkt->state < LIBMQTT_PACKETREADSTATE_SUBACK_END)
			ret = libmqtt_readpkt_subackstates(pkt, changefunc, changeuserdata,
					readfunc, readuserdata, payloadwritefunc,
					payloadwriteuserdata);
		else if (pkt->state < LIBMQTT_PACKETREADSTATE_UNSUBSCRIBE_END)
			ret = libmqtt_readpkt_unsubscribestates(pkt, changefunc,
					changeuserdata, readfunc, readuserdata, payloadwritefunc,
					payloadwriteuserdata);
		else if (pkt->state < LIBMQTT_PACKETREADSTATE_PUBLISH_END)
			ret = libmqtt_readpkt_publishstates(pkt, changefunc, changeuserdata,
					readfunc, readuserdata, payloadwritefunc,
					payloadwriteuserdata);
	}

	if (ret <= LIBMQTT_EFATAL)
		libmqtt_readpkt_changestate(pkt, changefunc,
				LIBMQTT_PACKETREADSTATE_ERROR, changeuserdata);

	return ret;
}
