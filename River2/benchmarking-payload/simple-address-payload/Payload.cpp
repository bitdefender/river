#include "CommonCrossPlatform/CommonSpecifiers.h"
#include "CommonCrossPlatform/Common.h"

extern "C" void test_simple(const unsigned char *buf);

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
#else
__attribute__((constructor)) void somain(void) {
}
#endif
