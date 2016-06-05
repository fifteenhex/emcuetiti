#pragma once

#include <stdbool.h>

#define EMCUETITI_CONFIG_DEBUG				true

// config for ports
#define EMCUETITI_CONFIG_MAXPORTS			4

// config for clients
#define EMCUETITI_CONFIG_MAXCLIENTS			4
#define EMCUETITI_CONFIG_MAXSUBSPERCLIENT	4

#define EMCUETITI_CONFIG_CLIENTBUFFERSZ		64

#define EMCUETITI_CONFIG_TIMESTAMPTYPE		uint32_t

/*
 * These options allow you to select what callbacks a client
 * can override from the main broker callbacks.
 * Less overridable callbacks should equal smaller per-client
 * overhead and less code.
 */

#define EMCUETITI_CONFIG_PERCLIENTCALLBACK_WRITE		false
#define EMCUETITI_CONFIG_PERCLIENTCALLBACK_DISCONNECT	false
