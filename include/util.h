#pragma once

#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:-!!(e); }))
#define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))
#define __must_be_array(a) BUILD_BUG_ON_ZERO(__same_type((a), &(a)[0]))
#define ARRAY_ELEMENTS(a) (sizeof(a) / sizeof(a[0]))

static size_t size_min(size_t a, size_t b) {
	if (a < b)
		return a;
	else
		return b;
}
