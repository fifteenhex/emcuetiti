#include <stdio.h>

#include "testutils.h"

void testutils_printbuffer(const uint8_t* buffer, size_t len) {

	int col = 0;
	while (len > 0) {
		printf("%02x(%c)\t", *buffer, *buffer);
		buffer++;
		len--;

		col++;
		if (col == 8) {
			col = 0;
			printf("\n");
		}
	}

	if (col != 0)
		printf("\n");

}
