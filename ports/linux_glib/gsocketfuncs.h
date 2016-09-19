#pragma once

#include <stdint.h>
#include <stdbool.h>

bool gsocket_readytoread(void* userdata);
int gsocket_read(void* connectiondata, uint8_t* buffer, size_t len);
