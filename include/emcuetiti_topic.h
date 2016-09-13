#pragma

#include "emcuetiti_types.h"

int emcuetiti_topic_topichandlewriter(libmqtt_writefunc writefunc,
		void *writefuncuserdata, void* userdata);

emcuetiti_topichandle* emcuetiti_readtopicstringandfindtopic(
		emcuetiti_brokerhandle* broker, uint8_t* buffer, uint16_t* topiclen,
		emcuetiti_subscription_level* level);

int emcuetiti_topic_len(emcuetiti_topichandle* node);

emcuetiti_topichandle* emcuetiti_findtopic(const emcuetiti_brokerhandle* broker,
		emcuetiti_topichandle* root, const char* topicpart);
