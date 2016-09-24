#pragma once

#include "emcuetiti_types.h"

typedef enum {
	EMCUETITI_LOG_LEVEL_ERROR,
	EMCUETITI_LOG_LEVEL_WARNING,
	EMCUETITI_LOG_LEVEL_INFO,
	EMCUETITI_LOG_LEVEL_DEBUG
} emcuetiti_log_level;

void emcuetiti_log(const emcuetiti_brokerhandle* broker,
		emcuetiti_log_level level, const char* format, ...) __attribute__ ((format (printf, 3, 4)));
