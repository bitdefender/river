unsigned int serial[] = { 0x31, 0x3e, 0x3d, 0x26, 0x31 };

int Check(unsigned int *ptr) {
	int i, j = 0;
	int hash = 0xABCD;

	for (i = 0; ptr[i]; i++) {
		hash += ptr[i] ^ serial[j];

		j = (j == 5) ? 0 : j + 1;
	}

	return hash;
}

extern "C" unsigned int buffer[] = {
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 0
};

#include <Windows.h>
EXCEPTION_RECORD ex;
typedef NTSTATUS(*RtlRaiseExceptionFunc)(PEXCEPTION_RECORD);
RtlRaiseExceptionFunc rtlRaiseException = nullptr;

void InitPayload() {
	HMODULE hNtDll = GetModuleHandle(L"ntdll.dll");
	rtlRaiseException = (RtlRaiseExceptionFunc)GetProcAddress(hNtDll, "RtlRaiseException");

	ex.ExceptionAddress = 0;
	ex.ExceptionCode = 0xC000000F;
}

#include <stdio.h>

int Payload() {
	/*__try {
		//rtlRaiseException(&ex);
		__asm mov eax, 0;
		__asm mov dword ptr [eax], 0x00;
	}
	__except (1) {
		return -1;
	}
	return 0;*/

	int ret;

	__try {
		ret = Check(nullptr);
	} __except (1) {
		printf("Handled!\n");
		ret = -1;
	}

	if (ret == 0xad6d) {
		return 1;
	}

	return 0;
}