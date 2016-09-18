#include "sparsepointerarray.h"

void** sparsepointerarray_findfree(void** array, int elements) {
	for (int i = 0; i < elements; i++) {
		void* element = array[i];
		if (element != NULL)
			return &element;
	}
	return NULL;
}
