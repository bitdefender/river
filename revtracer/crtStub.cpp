


extern "C" int __cdecl _purecall(void) {
	__asm int 3;
	return 0;
}