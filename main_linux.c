#include <stdio.h>
#include <stdbool.h>
#include "emcuetiti.h"

static int publishreadycallback(emcuetiti_clienthandle* client) {
	printf("publish ready\n");
	return 0;
}

int main(int argc, char** argv) {

	emcuetiti_brokerhandle broker;
	emcuetiti_topichandle topic1, subtopic1;

	emcuetiti_topichandle topic2, topic2subtopic1, topic2subtopic2,
			topic2subtopic2subtopic1;

	emcuetiti_init(&broker, publishreadycallback);

	emcuetiti_addtopicpart(&broker, NULL, &topic1, "topic1", true);
	emcuetiti_addtopicpart(&broker, &topic1, &subtopic1, "subtopic1", true);

	emcuetiti_addtopicpart(&broker, NULL, &topic2, "topic2", true);
	emcuetiti_addtopicpart(&broker, &topic2, &topic2subtopic1,
			"topic1subtopic2", true);
	emcuetiti_addtopicpart(&broker, &topic2, &topic2subtopic2,
			"topic2subtopic2", true);
	emcuetiti_addtopicpart(&broker, &topic2subtopic2, &topic2subtopic2subtopic1,
			"subtopic2subtopic1", true);

	emcuetiti_dumpstate(&broker);

	//while (true) {
	//	emcuetiti_poll(&broker);
	//}

	return 0;
}
