#pragma once

#include <stdbool.h>

#define EMCUETITI_CONFIG_DEBUG				true

// config for the broker
#define EMCUETITI_CONFIG_MAXTOPICPARTLEN	32
#define EMCUETITI_CONFIG_MAXPAYLOADLEN		1024

// config for the host
#define EMCUETITI_CONFIG_HAVETLS			true

// config for ports
#define EMCUETITI_CONFIG_MAXPORTS			4

// config for clients
#define EMCUETITI_CONFIG_MAXCLIENTS			4
#define EMCUETITI_CONFIG_MAXSUBSPERCLIENT	4

#define EMCUETITI_CONFIG_DEFAULTKEEPALIVE	600

#define EMCUETITI_CONFIG_CLIENTBUFFERSZ		128

#define EMCUETITI_CONFIG_TIMESTAMPTYPE		uint32_t

/*
 * These options allow you to select what callbacks a client
 * can override from the main broker callbacks.
 * Less overridable callbacks should equal smaller per-client
 * overhead and less code.
 */

#define EMCUETITI_CONFIG_PERCLIENTCALLBACKS				false
#define EMCUETITI_CONFIG_PERCLIENTCALLBACKS_ISCONNECTED	false
#define EMCUETITI_CONFIG_PERCLIENTCALLBACKS_WRITE		false
#define EMCUETITI_CONFIG_PERCLIENTCALLBACKS_READYTOREAD	false
#define EMCUETITI_CONFIG_PERCLIENTCALLBACKS_READ		false
#define EMCUETITI_CONFIG_PERCLIENTCALLBACKS_DISCONNECT	false

/* debugging output */

#define EMCUETITI_CONFIG_DEBUG_KEEPALIVE				false
