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

#include "emcuetiti_config.h"
#include "emcuetiti_topic.h"
#include "emcuetiti_log.h"

#include "buffers.h"

emcuetiti_topichandle* emcuetiti_findtopic(const emcuetiti_brokerhandle* broker,
		emcuetiti_topichandle* root, const char* topicpart) {

	EMCUETITI_LOG_DEBUG("looking for %s", topicpart);
	if (root == NULL)
		root = broker->root;
	else
		root = root->child;

	for (; root != NULL; root = root->sibling) {
		if (strcmp(root->topicpart, topicpart) == 0) {
			emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG, "found %s",
					topicpart);
			return root;
		} else
			emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG, "not %s",
					root->topicpart);
	}

	return NULL;
}

int emcuetiti_topic_len(emcuetiti_topichandle* node) {
	int len = 0;
	bool first = node->parent == NULL;
	if (!first) {
		len += (emcuetiti_topic_len(node->parent) + 1);
	}
	return len + node->topicpartln;
}

static void emcuetiti_topic_attach(emcuetiti_brokerhandle* broker,
		emcuetiti_topichandle* level, emcuetiti_topichandle* sibling) {
	for (; level->sibling != NULL; level = level->sibling) {

	}
	EMCUETITI_LOG_DEBUG("attaching to %s", level->topicpart);
	level->sibling = sibling;
}

int emcuetiti_topic_topichandlewriter(libmqtt_writefunc writefunc,
		void *writefuncuserdata, void* userdata) {

	int len = 0;
	emcuetiti_topichandle* node = (emcuetiti_topichandle*) userdata;

	bool first = node->parent == NULL;
	if (!first) {
		len += emcuetiti_topic_topichandlewriter(writefunc, writefuncuserdata,
				node->parent) + 1;
		writefunc(writefuncuserdata, "/", 1);
	}

	writefunc(writefuncuserdata, node->topicpart, node->topicpartln);
	return len;
}

void emcuetiti_broker_addtopicpart(emcuetiti_brokerhandle* broker,
		emcuetiti_topichandle* root, emcuetiti_topichandle* part,
		const char* topicpart, bool targetable) {

// clear pointers
	part->sibling = NULL;
	part->child = NULL;
	part->parent = NULL;

	part->topicpart = topicpart;
	part->topicpartln = strlen(topicpart);
	part->targetable = targetable;

	emcuetiti_topichandle* attachmentlevel;

	// attaching at the root level
	if (root == NULL) {
		// root for the broker hasn't been set yet, use this topic
		if (broker->root == NULL)
			broker->root = part;
		// otherwise attach to the bottom of the broker's root
		else
			emcuetiti_topic_attach(broker, broker->root, part);
	} else {
		// if this root doesn't have a child yet become that child
		if (root->child == NULL)
			root->child = part;
		// otherwise attach to the bottom of the child
		else {
			emcuetiti_topic_attach(broker, root->child, part);
		}
		part->parent = root;
	}
}

emcuetiti_topichandle* emcuetiti_readtopicstringandfindtopic(
		emcuetiti_brokerhandle* broker, uint8_t* buffer, uint16_t* topiclen,
		emcuetiti_subscription_level* level) {

	int topicpartpos = 0;
	char topicpart[32];

	uint16_t len = (*(buffer++) << 8) | *(buffer++);
	emcuetiti_subscription_level sublevel = ONLYTHIS;

	EMCUETITI_LOG_DEBUG("part len is %d", len);

	emcuetiti_topichandle* t = NULL;
	for (uint16_t i = 0; i < len; i++) {
		uint8_t byte = *(buffer++);
		if (i + 1 == len) {
			topicpart[topicpartpos++] = byte;
			topicpart[topicpartpos] = '\0';

			if (strcmp(topicpart, "#") == 0) {
				EMCUETITI_LOG_DEBUG("have multilevel wildcard");
				sublevel = THISANDABOVE;
			} else {
				EMCUETITI_LOG_DEBUG("%s", topicpart);
				t = emcuetiti_findtopic(broker, t, topicpart);
			}
		} else if (byte == '/') {
			topicpart[topicpartpos] = '\0';
			topicpartpos = 0;
			EMCUETITI_LOG_DEBUG("%s", topicpart);
			t = emcuetiti_findtopic(broker, t, topicpart);
		} else
			topicpart[topicpartpos++] = byte;

	}

	if (topiclen != NULL)
		*topiclen = len;

	if (level != NULL)
		*level = sublevel;

	return t;
}

int emcuetiti_topic_munchtopicpart(const uint8_t* buffer, size_t len,
		buffers_buffer* topicbuffer, emcuetiti_topicpartprocessor processor,
		emcuetiti_wildcardprocessor wildcardprocessor, void* userdata) {

	size_t consumed;

	bool topicpart = false;
	bool thisandabovewildcard = false;

	for (consumed = 0; consumed < len; consumed++) {
		bool exit = false;
		switch (buffer[consumed]) {
		case '/':
			topicpart = true;
			exit = true;
			break;
		case '#':
			thisandabovewildcard = true;
			exit = true;
			break;
		}
		if (exit)
			break;
	}

	buffers_buffer_append(topicbuffer, buffer, consumed);
	if (topicpart && buffers_buffer_available(topicbuffer) > 0) {
		buffers_buffer_terminate(topicbuffer);
		if (processor != NULL)
			processor(topicbuffer, userdata);
		buffers_buffer_reset(topicbuffer);
	} else if (thisandabovewildcard)
		if (wildcardprocessor != NULL)
			wildcardprocessor(THISANDABOVE, userdata);

	return consumed;
}
