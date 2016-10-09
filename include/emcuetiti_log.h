/*	This file is part of emcuetiti.
 *
 * emcuetiti is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * emcuetiti is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with emcuetiti.  If not, see <http://www.gnu.org/licenses/>.
 */

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
