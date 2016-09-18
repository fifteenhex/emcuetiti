#pragma once

#include "emcuetiti_types.h"

int emcuetiti_client_register(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* handle);
void emcuetiti_client_unregister(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* handle);
int emcuetiti_client_readpublish(emcuetiti_brokerhandle* broker,
		emcuetiti_clienthandle* client, uint8_t* buffer, size_t len);
void emcuetiti_client_poll(emcuetiti_brokerhandle* broker,
		emcuetiti_timestamp now);
libmqtt_writefunc emcuetiti_client_resolvewritefunc(
		emcuetiti_brokerhandle* broker, emcuetiti_clientstate* cs);

void emcuetiti_client_dumpstate(emcuetiti_brokerhandle* broker,
		emcuetiti_clientstate* client);
