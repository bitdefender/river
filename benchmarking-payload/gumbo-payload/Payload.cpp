#include "CommonCrossPlatform/CommonSpecifiers.h"

#include "gumbo.h"
#include <stdint.h>

#define MAX_PAYLOAD_BUF (64 << 10)

void test_simple(const unsigned char *buf) {
	GumboOutput* output = gumbo_parse_with_options(
			&kGumboDefaultOptions, (const char *)buf, MAX_PAYLOAD_BUF);
	gumbo_destroy_output(&kGumboDefaultOptions, output);
}

extern "C" {
	DLL_PUBLIC unsigned char payloadBuffer[MAX_PAYLOAD_BUF];
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
