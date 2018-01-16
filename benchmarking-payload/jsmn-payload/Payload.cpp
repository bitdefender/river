#include "Common.h"

#include "jsmn.h"


#define TOKEN_SIZE 256
#define MAX_PAYLOAD_BUF (64 << 10)

static jsmn_parser parser;
static jsmntok_t tokens[TOKEN_SIZE];

void test_simple(const unsigned char *buf) {
	jsmn_init(&parser);
	// js - pointer to JSON string
	// tokens - an array of tokens available
	jsmn_parse(&parser, (const char *)buf, MAX_PAYLOAD_BUF, tokens, TOKEN_SIZE);
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
