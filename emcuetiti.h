#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "libmqtt.h"

typedef int (*emcuetiti_writefunc)(void* userdata, uint8_t* buffer,
		size_t offset, size_t len);
typedef bool (*emcuetiti_readytoreadfunc)(void* userdata);
typedef int (*emcuetiti_readfunc)(void* userdata, uint8_t* buffer,
		size_t offset, size_t len);
typedef void (*emcuetiti_freefunc)(void* userdata);
typedef int (*emcuetiti_allocfunc)(void* userdata, size_t size);

typedef struct emcuetiti_topichandle {
	const char* topicpart;
	struct emcuetiti_topichandle* parent;
} emcuetiti_topichandle;

typedef struct {
	emcuetiti_topichandle topic;
	emcuetiti_writefunc writefunc;
	emcuetiti_readfunc readfunc;
	emcuetiti_freefunc freefunc;
	void* userdata;
} emcuetiti_publish;

typedef struct {
	libmqtt_writefunc writefunc; // function pointer to the function used to write data to the client
	emcuetiti_readytoreadfunc readytoread;
	emcuetiti_readfunc readfunc; // function pointer to the function user to read data from the client
	void* userdata; // use this to stash whatever is needed to write/read the right client
					// in the write/read functions
} emcuetiti_clienthandle;

// These functions are to be driven by the code running on the broker
// to publish to clients and the receive publishes from clients
void emcuetiti_local_send(emcuetiti_publish* publish);
void emcuetiti_local_recv(emcuetiti_publish* publish);
int emcuetiti_local_waiting(void);

//
void emcuetiti_client_register(emcuetiti_clienthandle* handle);
void emcuetiti_client_unregister(emcuetiti_clienthandle* handle);

//
void emcuetiti_poll(void);
void emcuetiti_addtopicpart(emcuetiti_topichandle* root,
		emcuetiti_topichandle* part, const char* topicpart);
void emcuetiti_init(emcuetiti_topichandle* topicroot);
