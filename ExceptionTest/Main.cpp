#include <Windows.h>
#include <cstdio>

#pragma section("hook", read, execute)
#pragma code_seg(push, "hook")

void __stdcall KiUEDPayload(PEXCEPTION_RECORD exception, PCONTEXT context) {
	printf("Intercepted exception at %08p\n", exception->ExceptionAddress);
}

__declspec(allocate("hook"))
unsigned char KiUEDHook[] = {
	0xFC,							// 0x00 - cld
	0x8B, 0x4C, 0x24, 0x04,			// 0x01 - mov ecx, dword ptr[esp + 4]
	0x8B, 0x1C, 0x24,				// 0x05 - mov ebx, dword ptr[esp + 0]
	0x51,							// 0x08 - push ecx
	0x53,							// 0x09 - push ebx
	0xE8, 0xFC, 0xFF, 0xFF, 0xFF,   // 0x0A - call KiUEDPayload
	0x00, 0x00, 0x00, 0x00, 0x00,   // 0x0F - original code
	0xE9, 0x00, 0x00, 0x00, 0x00    // 0x14 - jmp to the original KiUED
};

#pragma code_seg(pop)

int main() {
	HMODULE hNtdll = GetModuleHandle("ntdll.dll");
	void *kiUsr = GetProcAddress(hNtdll, "KiUserExceptionDispatcher");
	DWORD oldProtect;

	VirtualProtect(KiUEDHook, sizeof(KiUEDHook), PAGE_EXECUTE_READWRITE, &oldProtect);

	printf("KiUserExceptionDispatcher @%08p\n", kiUsr);

	memcpy(&KiUEDHook[0x0F], kiUsr, 5);
	*(DWORD *)&KiUEDHook[0x0B] = ((unsigned char *)KiUEDPayload) - (KiUEDHook + 0x0F);
	*(DWORD *)&KiUEDHook[0x15] = ((unsigned char *)kiUsr + 5) - (KiUEDHook + sizeof(KiUEDHook));

	VirtualProtect(KiUEDHook, sizeof(KiUEDHook), oldProtect, &oldProtect);


	VirtualProtect(kiUsr, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
	*(BYTE *)&((BYTE *)kiUsr)[0] = 0xe9;
	*(DWORD *)&((BYTE *)kiUsr)[1] = (BYTE *)&KiUEDHook - ((BYTE *)kiUsr + 5);
	VirtualProtect(kiUsr, 5, oldProtect, &oldProtect);


	__try {
		printf("Raising exception!\n");
		//__asm ud2;
		__asm xor eax, eax
		__asm mov dword ptr [eax], ecx
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		printf("Handled!\n");
	}

	system("pause");
	return 0;
}