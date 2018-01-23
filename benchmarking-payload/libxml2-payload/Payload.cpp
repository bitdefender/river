#include "CommonCrossPlatform/CommonSpecifiers.h"

#include "libxml/parser.h"
#include "libxml/tree.h"

#define MAX_PAYLOAD_BUF (64 << 10)

static void nopErrorHandlerFunction(void *ctx, const char *msg, ...) {}
static xmlGenericErrorFunc nopErrorHandler = nopErrorHandlerFunction;

void test_simple(const unsigned char *buf) {
	initGenericErrorDefaultFunc(&nopErrorHandler);

	if (auto doc = xmlReadMemory(reinterpret_cast<const char *>(buf), MAX_PAYLOAD_BUF, "noname.xml", NULL,
				XML_PARSE_NOERROR | XML_PARSE_NOWARNING | XML_PARSE_NONET)) {
		xmlFreeDoc(doc);
	}
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
