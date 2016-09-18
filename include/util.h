#pragma once

#define ARRAY_ELEMENTS(a) (sizeof(a) / sizeof(a[0]))

static size_t size_min(size_t a, size_t b) {
	if (a < b)
		return a;
	else
		return b;
}
