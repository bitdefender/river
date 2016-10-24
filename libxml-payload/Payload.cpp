#include "Common.h"

#include "libxml/parser.h"
#include "libxml/tree.h"

static void nopErrorHandlerFunction(void *ctx, const char *msg, ...) {}
static xmlGenericErrorFunc nopErrorHandler = nopErrorHandlerFunction;

void test_simple(const char *buf) {
	initGenericErrorDefaultFunc(&nopErrorHandler);

	if (auto doc = xmlReadMemory(reinterpret_cast<const char *>(buf), 4096, "noname.xml", NULL,
				XML_PARSE_NOERROR | XML_PARSE_NOWARNING | XML_PARSE_NONET)) {
		xmlFreeDoc(doc);
	}
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
