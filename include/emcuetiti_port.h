#pragma once

#include "emcuetiti_types.h"

void emcuetiti_port_onpublishready(emcuetiti_brokerhandle* broker,
		emcuetiti_topichandle* topic, buffers_buffer* payload);
void emcuetiti_port_register(emcuetiti_brokerhandle* broker,
		emcuetiti_porthandle* port);

