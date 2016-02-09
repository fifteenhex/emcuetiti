#pragma once

#include "emcuetiti_config.h"
#include "emcuetiti.h"

#define ARRAY_ELEMENTS(a) (sizeof(a) / sizeof(a[0]))

typedef enum {
	CLIENTREADSTATE_IDLE,
	CLIENTREADSTATE_TYPE,
	CLIENTREADSTATE_REMAININGLEN,
	CLIENTREADSTATE_PAYLOAD,
	CLIENTREADSTATE_COMPLETE
} emcuetiti_clientreadstate;

typedef struct {
	emcuetiti_clienthandle* client;
	char clientid[LIBMQTT_CLIENTID_MAXLENGTH];

	unsigned subscriptions;
	emcuetiti_clientreadstate readstate;
	uint8_t buffer[EMCUETITI_CONFIG_CLIENTBUFFERSZ];
	unsigned bufferpos;
	size_t remainingbytes;
} emcuetiti_clientstate;

typedef struct {
	emcuetiti_clienthandle* client;
	emcuetiti_topichandle* topic;
} emcuetiti_subscriptionhandle;
