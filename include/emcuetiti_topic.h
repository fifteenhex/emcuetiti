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

#pragma

#include "emcuetiti_types.h"
#include "emcuetiti_broker.h"

int emcuetiti_topic_topichandlewriter(libmqtt_writefunc writefunc,
		void *writefuncuserdata, void* userdata);

emcuetiti_topichandle* emcuetiti_readtopicstringandfindtopic(
		emcuetiti_brokerhandle* broker, uint8_t* buffer, uint16_t* topiclen,
		emcuetiti_subscription_level* level);

int emcuetiti_topic_len(emcuetiti_topichandle* node);

typedef void (*emcuetiti_topicpartprocessor)(buffers_buffer* topicbuffer,
		void* userdata);
typedef void (*emcuetiti_wildcardprocessor)(emcuetiti_subscription_level level,
		void* userdata);

emcuetiti_topichandle* emcuetiti_findtopic(const emcuetiti_brokerhandle* broker,
		emcuetiti_topichandle* root, const char* topicpart);

int emcuetiti_topic_munchtopicpart(const uint8_t* buffer, size_t len,
		buffers_buffer* topicbuffer, emcuetiti_topicpartprocessor processor,
		emcuetiti_wildcardprocessor, void* userdata);
