#include <stdio.h>

unsigned int fib0 = 1, fib1 = 1;

int main() {
	for (int i = 0; i < 1000 - 2; ++i) {
		unsigned int t = fib0 + fib1;
		fib0 = fib1;
		fib1 = t;
	}

	printf("%u\n", fib1);
}

