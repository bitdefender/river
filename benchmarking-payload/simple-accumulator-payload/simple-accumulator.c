#include <stdio.h>

#define MAX_PAYLOAD_BUF 128

void print_payload(const unsigned char *buf) {
	for (unsigned i = 0; i < MAX_PAYLOAD_BUF; ++i) {
		printf("%02X", buf[i]);
	}
	printf("\n");
}

void test_simple(const unsigned char *buf) {
	const unsigned alpha_len = 'z' - 'A';
	unsigned char magic = 0;
	for (unsigned i = 0; i < alpha_len; ++i) {
		magic = magic | ('A' + i);
	}

	if (magic == buf[0]) {
		print_payload(buf);
	}
}
