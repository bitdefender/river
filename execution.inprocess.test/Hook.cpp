#include <Windows.h>
#include <intrin.h>

#include "Hook.h"

#pragma section("hook", read, execute)
#pragma code_seg(push, "hook")

extern unsigned char *KiUEDOriginal;

static RiverExceptionHandler handler;

void *__stdcall KiUEDPayload(PEXCEPTION_RECORD exception, PCONTEXT context) {
	//printf("Intercepted exception at %08p\n", exception->ExceptionAddress);
	unsigned int originalEip;
	void *jumpBuff = handler((unsigned int)exception->ExceptionAddress, originalEip);

	if (nullptr == jumpBuff) {
		return KiUEDOriginal;
	}

	exception->ExceptionAddress = (PVOID)originalEip;
	context->Eip = originalEip;

	return jumpBuff;
}

__declspec(allocate("hook"))
unsigned char KiUEDHook[] = {
	0xFC,							// 0x00 - cld
	0x8B, 0x4C, 0x24, 0x04,			// 0x01 - mov ecx, dword ptr[esp + 4]
	0x8B, 0x1C, 0x24,				// 0x05 - mov ebx, dword ptr[esp + 0]
	0x51,							// 0x08 - push ecx
	0x53,							// 0x09 - push ebx
	0xE8, 0x00, 0x00, 0x00, 0x00,   // 0x0A - call KiUEDPayload
	0x50,							// 0x0F - push eax
	0xC3,							// 0x10 - ret
	0x00, 0x00, 0x00, 0x00, 0x00,   // 0x11 - original code
	0xE9, 0x00, 0x00, 0x00, 0x00    // 0x16 - jmp to the original KiUED
};

unsigned char *KiUEDOriginal = &KiUEDHook[0x11];

#pragma code_seg(pop)

bool ApplyExceptionHook(RiverExceptionHandler hnd) {
	handler = hnd;
	HMODULE hNtdll = GetModuleHandle(L"ntdll.dll");
	void *kiUsr = GetProcAddress(hNtdll, "KiUserExceptionDispatcher");
	DWORD oldProtect;

	VirtualProtect(KiUEDHook, sizeof(KiUEDHook), PAGE_EXECUTE_READWRITE, &oldProtect);
	*(DWORD *)&KiUEDHook[0x0B] = ((unsigned char *)KiUEDPayload) - (KiUEDHook + 0x0F);
	memcpy(&KiUEDHook[0x11], kiUsr, 5);
	*(DWORD *)&KiUEDHook[0x17] = ((unsigned char *)kiUsr + 5) - (KiUEDHook + sizeof(KiUEDHook));
	VirtualProtect(KiUEDHook, sizeof(KiUEDHook), oldProtect, &oldProtect);

	union _TMP {
		LONG64 asLongLong;
		BYTE asBytes[8];
	} llPatch, llOrig, *llKiUsr = (_TMP *)kiUsr;

	llPatch.asLongLong = llOrig.asLongLong = llKiUsr->asLongLong;
	llPatch.asBytes[0] = 0xe9;
	*(DWORD *)&llPatch.asBytes[1] = (BYTE *)&KiUEDHook - ((BYTE *)kiUsr + 5);

	VirtualProtect(kiUsr, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
	InterlockedCompareExchange64((volatile LONG64 *)kiUsr, llPatch.asLongLong, llOrig.asLongLong);
	VirtualProtect(kiUsr, 5, oldProtect, &oldProtect);

	return true;
}