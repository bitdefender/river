#include "common.h"

extern "C" int __cdecl _purecall(void) {
	DEBUG_BREAK;
	return 0;
}