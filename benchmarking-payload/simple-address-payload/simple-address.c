#include <stdio.h>
#include <stdlib.h>
#include "CommonCrossPlatform/Common.h"

// data layout
//		size | ... | payload | payload ... | payload
// payload layout
//		data | size

void my_warn(void) {
	exit(0);
}

void test_simple(const unsigned char *buf) {
	unsigned size = 0;
	const unsigned char *payload_cursor = NULL;

	size = ((unsigned *)buf)[0];
	payload_cursor = buf + size;

	payload_cursor -= 4;

	unsigned payload_size = ((unsigned *)payload_cursor)[0];
	//payload_cursor -= payload_size;

	__asm("mov 4(%%ebp), %%ebx;"
			"mov %0, %%eax;"
			"test %%eax, %%eax;"
			"jz my_warn;"
			: : "r" (payload_cursor) : "%eax", "%ebx"
		 );
}
