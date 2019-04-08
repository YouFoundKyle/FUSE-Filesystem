#include "bitmap.h"
#include <stdio.h>
#include <stdint.h>

int bitmap_get(void* bm, int ii){	
 	return (( ((uint8_t*) bm)[ii / 8] >> (7 - (ii % 8))) & 1);
	
}

void bitmap_put(void* bm, int ii, int vv){
	if (vv == 0) {
		((uint8_t*)bm) [ii / 8] &= ~(1 << (7 - (ii % 8)));
	} else {
		((uint8_t*)bm) [ii / 8] |= 1 << (7 - (ii % 8));
	}
}

void bitmap_print(void* bm, int size){

	uint8_t* b = bm;
	puts("---BITMAP---");
	for (int i = 0; i < size; i++){
		printf("%u ", b[i]);
	}
	puts("\n");
	
}
