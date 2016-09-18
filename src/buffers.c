#include <string.h>

#include "buffers.h"
#include "util.h"

void buffers_staticbuffer_tobuffer(uint8_t* staticbuffer, size_t sz,
		buffers_buffer* buffer) {
	buffer->head = (buffers_bufferhead*) staticbuffer;
	buffer->sz = sz;
	buffer->buffer = staticbuffer + sizeof(buffers_bufferhead);
}

static uint8_t* buffers_buffer_alloc(buffers_buffer* buffer, size_t len) {
	uint8_t* ret = buffer->buffer + buffer->head->head;
	buffer->head->head += len;
	return ret;
}

static uint8_t* buffers_buffer_consume(buffers_buffer* buffer, size_t len) {
	uint8_t* ret = buffer->buffer + buffer->head->tail;
	buffer->head->tail += len;
	return ret;
}

int buffers_buffer_append(buffers_buffer* target, const uint8_t* buffer,
		size_t len) {
	size_t free = buffers_buffer_free(target);
	size_t writesz = size_min(free, len);
	memcpy(buffers_buffer_alloc(target, writesz), buffer, writesz);
	return writesz;
}

int buffers_buffer_writefunc(void* userdata, const uint8_t* buffer, size_t len) {
	buffers_buffer* target = (buffers_buffer*) userdata;
	return buffers_buffer_append(target, buffer, len);
}

int buffers_buffer_readfunc(void* userdata, uint8_t* buffer, size_t len) {
	buffers_buffer* target = (buffers_buffer*) userdata;
	size_t avail = buffers_buffer_available(target);
	size_t readsz = size_min(avail, len);
	memcpy(buffer, buffers_buffer_consume(target, readsz), readsz);
	return readsz;
}

size_t buffers_buffer_free(buffers_buffer* buffer) {
	return buffer->sz - buffer->head->head;
}

size_t buffers_buffer_available(buffers_buffer* buffer) {
	return buffer->head->head - buffer->head->tail;
}

void buffers_buffer_reset(buffers_buffer* buffer) {
	buffer->head->head = 0;
	buffer->head->tail = 0;
}

int buffers_buffer_flush(buffers_buffer* buffer, libmqtt_writefunc writefunc,
		void* userdata) {

	int ret = 0;
	size_t writesz = buffers_buffer_available(buffer);
	if (writesz != 0) {
		if (writefunc != NULL) {
			ret = writefunc(userdata, buffer->buffer + buffer->head->tail,
					writesz);
			if (ret > 0)
				buffers_buffer_consume(buffer, ret);
		} else {
			buffers_buffer_consume(buffer, writesz);
			ret = writesz;
		}

		if (buffers_buffer_available(buffer) == 0)
			buffers_buffer_reset(buffer);
	}
	return ret;
}

int buffers_buffer_fill(buffers_buffer* buffer, size_t waiting,
		libmqtt_readfunc readfunc, void* userdata) {
	int ret = 0;
	size_t readsz = buffers_buffer_free(buffer);
	if (readsz > 0) {
		readsz = size_min(readsz, waiting);
		ret = readfunc(userdata, buffer->buffer + buffer->head->head, readsz);
		if (ret > 0)
			buffers_buffer_alloc(buffer, ret);
	}
	return ret;
}
