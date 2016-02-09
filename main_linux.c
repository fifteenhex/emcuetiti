#include <stdbool.h>
#include "emcuetiti.h"

int main(int argc, char** argv) {

	emcuetiti_topichandle root, topic1, topic2, subtopic1, subtopic2;

	emcuetiti_init(&root);

	emcuetiti_addtopicpart(&root, &topic1, "topic1");
	emcuetiti_addtopicpart(&topic1, &subtopic1, "subtopic1");

	emcuetiti_addtopicpart(&root, &topic2, "topic2");
	emcuetiti_addtopicpart(&topic2, &subtopic2, "subtopic2");

	while (true) {
		emcuetiti_poll();
	}

	return 0;
}
