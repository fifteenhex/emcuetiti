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

#include "emcuetiti_log.h"

void emcuetiti_log(const emcuetiti_brokerhandle* broker,
		emcuetiti_log_level level, const char* format, ...) {
	if (broker->callbacks->logx != NULL) {
		va_list args;
		va_start(args, format);
		broker->callbacks->logx(broker, format, args);
		va_end(args);
	}
}
