#include <stdio.h>
extern void init(void);
int main() {
	init();
	printf("I am the traced process!\n");
}
