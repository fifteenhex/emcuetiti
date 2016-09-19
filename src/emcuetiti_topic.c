#include <string.h>
#include <stdio.h>

#include "emcuetiti_config.h"
#include "emcuetiti_topic.h"

#include "buffers.h"

emcuetiti_topichandle* emcuetiti_findtopic(const emcuetiti_brokerhandle* broker,
		emcuetiti_topichandle* root, const char* topicpart) {

	broker->callbacks->log(broker, "looking for %s", topicpart);
	if (root == NULL)
		root = broker->root;
	else
		root = root->child;

	for (; root != NULL; root = root->sibling) {
		if (strcmp(root->topicpart, topicpart) == 0) {
			broker->callbacks->log(broker, "found %s", topicpart);
			return root;
		} else
			broker->callbacks->log(broker, "not %s", root->topicpart);
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

static void emcuetiti_attach(emcuetiti_topichandle* level,
		emcuetiti_topichandle* sibling) {
	for (; level->sibling != NULL; level = level->sibling) {

	}
#ifdef EMCUETITI_CONFIG_DEBUG
	printf("attaching to %s\n", level->topicpart);
#endif
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
			emcuetiti_attach(broker->root, part);
	} else {
		// if this root doesn't have a child yet become that child
		if (root->child == NULL)
			root->child = part;
		// otherwise attach to the bottom of the child
		else {
			emcuetiti_attach(root->child, part);
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

	printf("part len is %d\n", len);

	emcuetiti_topichandle* t = NULL;
	for (uint16_t i = 0; i < len; i++) {
		uint8_t byte = *(buffer++);
		if (i + 1 == len) {
			topicpart[topicpartpos++] = byte;
			topicpart[topicpartpos] = '\0';

			if (strcmp(topicpart, "#") == 0) {
				printf("have multilevel wildcard\n");
				sublevel = THISANDABOVE;
			} else {
				printf("%s\n", topicpart);
				t = emcuetiti_findtopic(broker, t, topicpart);
			}
		} else if (byte == '/') {
			topicpart[topicpartpos] = '\0';
			topicpartpos = 0;
			printf("%s\n", topicpart);
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
