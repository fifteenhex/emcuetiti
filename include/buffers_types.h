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

#include <stdint.h>

typedef struct {
	size_t head;
	size_t tail;
	unsigned refs;
} buffers_bufferhead;

typedef struct {
	size_t* size;
	size_t* head;
	size_t* tail;
	unsigned* refs;
	uint8_t* buffer;
} buffers_buffer;

typedef struct {
	size_t localtail;
	buffers_buffer buffer;
} buffers_buffer_reference;

#define BUFFERS_STATICBUFFER(name, size) uint8_t name[sizeof(buffers_bufferhead) + size]
#define BUFFERS_STATICBUFFERPOOL(name, size, items) uint8_t name[items][sizeof(buffers_bufferhead) + size]
