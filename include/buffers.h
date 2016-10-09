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

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "libmqtt.h"
#include "util.h"

#define BUFFERS_STATICBUFFER_TO_BUFFER(s, b) \
	__must_be_array(s); \
	size_t b##_size = sizeof(s); \
	buffers_buffer b; \
	buffers_staticbuffer_tobuffer((uint8_t*) s, &b##_size, &b)

#define BUFFERS_STATICBUFFER_TO_BUFFER_SIZE(s, b, sz) \
	buffers_buffer b; \
	buffers_staticbuffer_tobuffer((uint8_t*) s, &sz, &b)

void buffers_staticbuffer_tobuffer(uint8_t* staticbuffer, size_t* sz,
		buffers_buffer* buffer);

int buffers_buffer_append(buffers_buffer* target, const uint8_t* buffer,
		size_t len);
int buffers_buffer_emptyinto(buffers_buffer* src, uint8_t* dst, size_t max);

size_t buffers_buffer_free(buffers_buffer* buffer);
size_t buffers_buffer_available(buffers_buffer* buffer);
void buffers_buffer_reset(buffers_buffer* buffer);

int buffers_buffer_flush(buffers_buffer* buffer, libmqtt_writefunc writefunc,
		void* userdata);
int buffers_buffer_fill(buffers_buffer* buffer, size_t waiting,
		libmqtt_readfunc readfunc, void* userdata);
void buffers_buffer_terminate(buffers_buffer* target);

//
int buffers_buffer_writefunc(void* userdata, const uint8_t* buffer, size_t len);
int buffers_buffer_readfunc(void* userdata, uint8_t* buffer, size_t len);
void buffers_buffer_resetfunc(void* userdata);

//
bool buffers_buffer_inuse(buffers_buffer* target);
void buffers_buffer_ref(buffers_buffer* target);
void buffers_buffer_unref(buffers_buffer* target);
void buffers_buffer_createreference(buffers_buffer* buffer,
		buffers_buffer_reference* reference);

void buffers_wrap(buffers_buffer* buffer, buffers_bufferhead* head,
		void* rawbuffer, size_t* size);
