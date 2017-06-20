#include "Common.h"

#include "gumbo.h"
#include <stdint.h>

void test_simple(const char *buf) {
	GumboOutput* output = gumbo_parse_with_options(
			&kGumboDefaultOptions, buf, 4096);
	gumbo_destroy_output(&kGumboDefaultOptions, output);
}

extern "C" {
	DLL_PUBLIC char payloadBuffer[4096];
	DLL_PUBLIC int Payload() {
		test_simple(payloadBuffer);
		return 0;
	}
};

#ifdef _WIN32
#include <Windows.h>
BOOL WINAPI DllMain(
		_In_ HINSTANCE hinstDLL,
		_In_ DWORD     fdwReason,
		_In_ LPVOID    lpvReserved
		) {
	return TRUE;
}
#endif
