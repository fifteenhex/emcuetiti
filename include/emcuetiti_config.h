/*	This file is part of emcuetiti.
 *
 * emcuetiti is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * emcuetiti is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with emcuetiti.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdbool.h>

// general config
#define EMCUETITI_CONFIG_DEBUG							true
#define EMCUETITI_CONFIG_TIMESTAMPTYPE					uint32_t

// config for the broker
#define EMCUETITI_CONFIG_MAXTOPICPARTLEN				32
#define EMCUETITI_CONFIG_MAXPAYLOADLEN					1024
#define EMCUETITI_CONFIG_MAXINFLIGHTPAYLOADS			2

// config for the host
#define EMCUETITI_CONFIG_HAVETLS						true

// config for ports
#define EMCUETITI_CONFIG_MAXPORTS						4

// config for clients
#define EMCUETITI_CONFIG_MAXCLIENTS						4
#define EMCUETITI_CONFIG_MAXSUBSPERCLIENT				4
#define EMCUETITI_CONFIG_DEFAULTKEEPALIVE				600
#define EMCUETITI_CONFIG_CLIENTBUFFERSZ					128
#define EMCUETITI_CONFIG_FULLDUPLEX						true

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

/*
 * These options allow you to select what callbacks a port
 * can listen for.
 */

#define EMCUETITI_CONFIG_PORT_CALLBACK_PUBLISHREADY		true
#define EMCUETITI_CONFIG_PORT_CALLBACK_CLIENTCONNECT	true
#define EMCUETITI_CONFIG_PORT_CALLBACK_CLIENTDISCONNECT	true

/* debugging output */

#define EMCUETITI_CONFIG_DEBUG_KEEPALIVE				false
