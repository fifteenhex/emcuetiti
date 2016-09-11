#pragma once

#include <stddef.h>
#include <stdint.h>

#include "libmqtt.h"

#define BUFFERS_STATICBUFFER_TO_BUFFER(s, b) buffers_buffer b; buffers_staticbuffer_tobuffer(s, sizeof(s), &b)

void buffers_staticbuffer_tobuffer(uint8_t* staticbuffer, size_t sz,
		buffers_buffer* buffer);

int buffers_buffer_writefunc(void* userdata, const uint8_t* buffer, size_t len);
size_t buffers_buffer_free(buffers_buffer* buffer);
size_t buffers_buffer_available(buffers_buffer* buffer);
void buffers_buffer_reset(buffers_buffer* buffer);

int buffers_buffer_flush(buffers_buffer* buffer, libmqtt_writefunc writefunc,
		void* userdata);
int buffers_buffer_fill(buffers_buffer* buffer, size_t waiting,
		libmqtt_readfunc readfunc, void* userdata);
