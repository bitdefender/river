#include "Common.h"

#include "jsmn.h"


#define TOKEN_SIZE 256

static jsmn_parser parser;
static jsmntok_t tokens[TOKEN_SIZE];

void test_simple(const char *buf) {
	jsmn_init(&parser);
	// js - pointer to JSON string
	// tokens - an array of tokens available
	jsmn_parse(&parser, buf, 4096, tokens, TOKEN_SIZE);

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
