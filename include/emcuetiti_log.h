#pragma once

#include "emcuetiti_types.h"

#ifdef EMCUETITI_CONFIG_DEBUG
#define EMCUETITI_LOG_DEBUG(msg, ...) emcuetiti_log(broker, EMCUETITI_LOG_LEVEL_DEBUG, msg, ##__VA_ARGS__)
#else
EMCUETITI_LOG_DEBUG(msg)
#endif

typedef enum {
	EMCUETITI_LOG_LEVEL_ERROR,
	EMCUETITI_LOG_LEVEL_WARNING,
	EMCUETITI_LOG_LEVEL_INFO,
	EMCUETITI_LOG_LEVEL_DEBUG
} emcuetiti_log_level;

void emcuetiti_log(const emcuetiti_brokerhandle* broker,
		emcuetiti_log_level level, const char* format, ...) __attribute__ ((format (printf, 3, 4)));
