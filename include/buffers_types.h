#pragma once

#include <stdint.h>

typedef struct {
	size_t head;
	size_t tail;
	unsigned refs;
} buffers_bufferhead;

typedef struct {
	buffers_bufferhead* head;
	size_t sz;
	uint8_t* buffer;
} buffers_buffer;

#define BUFFERS_STATICBUFFER(name, size) uint8_t name[sizeof(buffers_bufferhead) + size]
