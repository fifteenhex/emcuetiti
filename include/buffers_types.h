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
