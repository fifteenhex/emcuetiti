#include <stdbool.h>
#include "emcuetiti.h"

int main(int argc, char** argv) {

	emcuetiti_brokerhandle broker;
	emcuetiti_topichandle topic1, subtopic1;

	emcuetiti_topichandle topic2, topic2subtopic1, topic2subtopic2,
			topic2subtopic2subtopic1;

	emcuetiti_init(&broker);

	emcuetiti_addtopicpart(&broker, NULL, &topic1, "topic1");
	emcuetiti_addtopicpart(&broker, &topic1, &subtopic1, "subtopic1");

	emcuetiti_addtopicpart(&broker, NULL, &topic2, "topic2");
	emcuetiti_addtopicpart(&broker, &topic2, &topic2subtopic1,
			"topic1subtopic2");
	emcuetiti_addtopicpart(&broker, &topic2, &topic2subtopic2,
			"topic2subtopic2");
	emcuetiti_addtopicpart(&broker, &topic2subtopic2, &topic2subtopic2subtopic1,
			"subtopic2subtopic1");

	emcuetiti_dumpstate(&broker);

	//while (true) {
	//	emcuetiti_poll(&broker);
	//}

	return 0;
}
