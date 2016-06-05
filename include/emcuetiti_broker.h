#pragma once

#include "emcuetiti_types.h"

void emcuetiti_broker_poll(emcuetiti_brokerhandle* broker);
void emcuetiti_broker_addtopicpart(emcuetiti_brokerhandle* broker,
		emcuetiti_topichandle* root, emcuetiti_topichandle* part,
		const char* topicpart, bool targetable);
void emcuetiti_broker_init(emcuetiti_brokerhandle* broker);
void emcuetiti_broker_dumpstate(emcuetiti_brokerhandle* broker);
