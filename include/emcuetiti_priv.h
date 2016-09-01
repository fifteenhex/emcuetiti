#pragma once

#include "emcuetiti_config.h"
#include "emcuetiti.h"

#define ARRAY_ELEMENTS(a) (sizeof(a) / sizeof(a[0]))

void emcuetiti_port_poll(emcuetiti_brokerhandle* broker,
		emcuetiti_timestamp now);
