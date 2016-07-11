#include <stdio.h>

#include "LargeStack.h"

using namespace rev;

rev::DWORD stack[0x10000];
rev::DWORD top;

int main() {
	LargeStack ls(stack, sizeof(stack), &top, "stack.bin");

	unsigned int val = 0;
	for (int i = 0; i < 0x1000000; ++i) {
		for (int j = 0; j < 13; ++j) {

			//push equivalent
			top -= 4; 
			*((rev::DWORD *)top) = val;
			
			val++;
		}

		ls.Update();

		for (int j = 0; j < 12; ++j) {
			val--;

			rev::DWORD v = *((rev::DWORD *)top);
			top += 4;

			if (v != val) __asm int 3;
		}

		ls.Update();
	}

	return 0;
}