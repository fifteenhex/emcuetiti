#include <string.h>

#include "buffers.h"
#include "buffers_types.h"
#include "util.h"

void buffers_staticbuffer_tobuffer(uint8_t* staticbuffer, size_t* sz,
		buffers_buffer* buffer) {
	buffers_bufferhead* head = (buffers_bufferhead*) staticbuffer;

	buffer->size = sz;
	buffer->head = &head->head;
	buffer->tail = &head->tail;
	buffer->refs = &head->refs;

	buffer->buffer = staticbuffer + sizeof(buffers_bufferhead);
}

static uint8_t* buffers_buffer_alloc(buffers_buffer* buffer, size_t len) {
	uint8_t* ret = buffer->buffer + *buffer->head;
	*buffer->head += len;
	return ret;
}

static uint8_t* buffers_buffer_consume(buffers_buffer* buffer, size_t len) {
	uint8_t* ret = buffer->buffer + *buffer->tail;
	*buffer->tail += len;
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

void buffers_buffer_resetfunc(void* userdata) {
	buffers_buffer* target = (buffers_buffer*) userdata;
	*target->tail = 0;
}

size_t buffers_buffer_free(buffers_buffer* buffer) {
	return *buffer->size - *buffer->head;
}

size_t buffers_buffer_available(buffers_buffer* buffer) {
	return *buffer->head - *buffer->tail;
}

void buffers_buffer_reset(buffers_buffer* buffer) {
	*buffer->head = 0;
	*buffer->tail = 0;
}

int buffers_buffer_flush(buffers_buffer* buffer, libmqtt_writefunc writefunc,
		void* userdata) {

	int ret = 0;
	size_t writesz = buffers_buffer_available(buffer);
	if (writesz != 0) {
		if (writefunc != NULL) {
			ret = writefunc(userdata, buffer->buffer + *buffer->tail, writesz);
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
		ret = readfunc(userdata, buffer->buffer + *buffer->head, readsz);
		if (ret > 0)
			buffers_buffer_alloc(buffer, ret);
	}
	return ret;
}

int buffers_buffer_emptyinto(buffers_buffer* src, uint8_t* dst, size_t max) {
	size_t cpysz = size_min(*src->head, max);
	memcpy(dst, src->buffer, cpysz);
	return cpysz;
}

void buffers_buffer_terminate(buffers_buffer* target) {
	uint8_t term = '\0';
	buffers_buffer_append(target, &term, 1);
}

bool buffers_buffer_inuse(buffers_buffer* target) {
	return *target->refs > 0;
}

void buffers_buffer_ref(buffers_buffer* target) {
	*target->refs = *target->refs++;
}

void buffers_buffer_unref(buffers_buffer* target) {
	*target->refs = *target->refs--;
	if (!buffers_buffer_inuse(target))
		buffers_buffer_reset(target);
}

void buffers_buffer_createreference(buffers_buffer* buffer,
		buffers_buffer_reference* reference) {
	memcpy(&reference->buffer, buffer, sizeof(reference->buffer));
	reference->localtail = *reference->buffer.tail;
	reference->buffer.tail = &reference->localtail;
}
