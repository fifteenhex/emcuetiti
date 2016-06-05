#pragma once

#include "emcuetiti_types.h"

void emcuetiti_poll(emcuetiti_brokerhandle* broker);
void emcuetiti_addtopicpart(emcuetiti_brokerhandle* broker,
		emcuetiti_topichandle* root, emcuetiti_topichandle* part,
		const char* topicpart, bool targetable);
void emcuetiti_init(emcuetiti_brokerhandle* broker);
void emcuetiti_dumpstate(emcuetiti_brokerhandle* broker);
