#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "libmqtt.h"

#define BUFFERS_STATICBUFFER_TO_BUFFER(s, b) size_t b##_size = sizeof(s);\
	buffers_buffer b; \
	buffers_staticbuffer_tobuffer(s, &b##_size, &b)

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
