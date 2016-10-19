#include <sys/mman.h>

#define DEBUG_BREAK asm volatile("int $0x3")

extern unsigned int get_rtld_global_ro_addr(void);

void patch__rtld_global_ro() {

	void *_rtld_global_ro =  (void*)get_rtld_global_ro_addr();
	off_t sse_flag = 0x64;
	int ret = mprotect(_rtld_global_ro - 0xd00, 0x1000, PROT_READ | PROT_WRITE);

	void *_rtld_global_ro_sse = _rtld_global_ro + sse_flag;

	*((unsigned int*)_rtld_global_ro_sse) &= 0xfffffdff;
	*((unsigned int*)_rtld_global_ro_sse + 1) &= 0xfbffffff;
	mprotect((void *)_rtld_global_ro - 0xd00, 0x1000, PROT_READ);
}

__attribute__((constructor)) void somain(void) {
	patch__rtld_global_ro();
}
