#include <string.h>
#include <stdio.h>

#include "emcuetiti_config.h"
#include "emcuetiti_topic.h"

static emcuetiti_topichandle* emcuetiti_findtopic(
		emcuetiti_brokerhandle* broker, emcuetiti_topichandle* root,
		const char* topicpart) {

	printf("looking for %s\n", topicpart);
	if (root == NULL)
		root = broker->root;
	else
		root = root->child;

	for (; root != NULL; root = root->sibling) {
		if (strcmp(root->topicpart, topicpart) == 0) {
			printf("found %s\n", topicpart);
			return root;
		} else
			printf("not %s\n", root->topicpart);
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

static emcuetiti_topichandle* emcuetiti_findparent(
		emcuetiti_topichandle* sibling) {
	for (; sibling->sibling != NULL; sibling = sibling->sibling) {

	}
#ifdef EMCUETITI_CONFIG_DEBUG
	printf("attaching to %s\n", sibling->topicpart);
#endif
	return sibling;
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

	if (root == NULL) {
		if (broker->root == NULL)
			broker->root = part;
		else {
			emcuetiti_topichandle* parent = emcuetiti_findparent(broker->root);
			parent->sibling = part;
		}
	} else {
		// if this root doesn't have a child yet become that child
		if (root->child == NULL)
			root->child = part;
		else {
			emcuetiti_topichandle* parent = emcuetiti_findparent(root->child);
			parent->sibling = part;
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
