#include <stdio.h>
#include <stdarg.h>

#include <Windows.h>
#include <tlhelp32.h>

#include "../revtracer/revtracer.h"

#include "Loader/Inproc.Mapper.h"
#include "Loader/PE.ldr.h"

//using namespace rev;

DWORD baseAddr = 0x0f000000;
bool isInside = false;
DWORD a = FILE_MAP_EXECUTE;

HANDLE fDbg = INVALID_HANDLE_VALUE;
bool MapPE(DWORD &baseAddr);

struct UserContext {
	DWORD execCount;
};

void DebugPrint(DWORD printMask, const char *fmt, ...) {
	va_list va;
	char tmpBuff[512];

	static char lastChar = '\n';

	char pfxBuff[] = "[___|_____|___|_] ";
	
	const char messageTypes[][4] = {
		"___",
		"ERR",
		"INF",
		"DBG"
	};

	const char executionStages[][6] = {
		"_____",
		"BRHND",
		"DIASM",
		"TRANS",
		"REASM",
		"RUNTM",
		"INSPT",
		"CNTNR"
	};

	const char codeTypes[][4] = {
		"___",
		"NAT",
		"RIV",
		"TRK",
		"SYM"
	};

	const char codeDirections[] = {
		'_', 'F', 'B'
	};

	_snprintf_s(
		pfxBuff,
		sizeof(pfxBuff) - 1,
		"[%3s|%5s|%3s|%c] ",
		messageTypes[(printMask & PRINT_MESSAGE_MASK) >> PRINT_MESSAGE_SHIFT],
		executionStages[(printMask & PRINT_EXECUTION_MASK) >> PRINT_EXECUTION_SHIFT],
		codeTypes[(printMask & PRINT_CODE_TYPE_MASK) >> PRINT_CODE_TYPE_SHIFT],
		codeDirections[(printMask & PRINT_CODE_DIRECTION_MASK) >> PRINT_CODE_DIRECTION_SHIFT]
	);

	va_start(va, fmt);
	int sz = _vsnprintf_s(tmpBuff, sizeof(tmpBuff)-1, fmt, va);
	va_end(va);

	unsigned long wr;
	if ('\n' == lastChar) {
		WriteFile(fDbg, pfxBuff, sizeof(pfxBuff) - 1, &wr, NULL);
	}
	WriteFile(fDbg, tmpBuff, sz * sizeof(tmpBuff[0]), &wr, NULL);
	lastChar = tmpBuff[sz - 1];
}

/*__declspec(dllimport) int vsnprintf_s(
	char *buffer,
	size_t sizeOfBuffer,
	size_t count,
	const char *format,
	va_list argptr
);*/

#pragma warning(disable:4996)
HANDLE fBlocks = CreateFileA("tmp.blk", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
#pragma warning(default:4996)

void TmpPrint(const char *fmt, ...) {
	va_list va;
	char tmpBuff[512];

	va_start(va, fmt);
	int sz = _vsnprintf_s(tmpBuff, sizeof(tmpBuff)-1, fmt, va);
	va_end(va);

	unsigned long wr;
	WriteFile(fBlocks, tmpBuff, sz * sizeof(tmpBuff[0]), &wr, NULL);
	//FlushFileBuffers(fBlocks);
	//FlushFileBuffers(__.fDbg);
}

rev::ExecutionRegs rgs[2];

bool RegCheck(const rev::ExecutionRegs &r1, const rev::ExecutionRegs &r2) {
	if (r1.eax != r2.eax) {
		DebugPrint(0, "EAX inconsistent!\n");
		return false;
	}

	if (r1.ecx != r2.ecx) {
		DebugPrint(0, "ECX inconsistent!\n");
		return false;
	}

	if (r1.edx != r2.edx) {
		DebugPrint(0, "EDX inconsistent!\n");
		return false;
	}

	if (r1.ebx != r2.ebx) {
		DebugPrint(0, "EBX inconsistent!\n");
		return false;
	}

	if (r1.esp != r2.esp) {
		DebugPrint(0, "ESP inconsistent!\n");
		return false;
	}

	if (r1.ebp != r2.ebp) {
		DebugPrint(0, "EBP inconsistent!\n");
		return false;
	}

	if (r1.esi != r2.esi) {
		DebugPrint(0, "ESI inconsistent!\n");
		return false;
	}

	if (r1.edi != r2.edi) {
		DebugPrint(0, "ESI inconsistent!\n");
		return false;
	}

	if (r1.eflags != r2.eflags) {
		DebugPrint(0, "EFLAGS inconsistent!\n");
		return false;
	}
	return true;
}

DWORD CustomExecutionController(void *ctx, rev::ADDR_TYPE addr, void *cbCtx) {
	static HANDLE hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());

	static HMODULE hKernel32Dll = NULL;
	static void *IPFPFunc = NULL;
	UserContext *c = (UserContext *)ctx;
	rev::ExecutionRegs cReg;

	c->execCount++;

	char mf[MAX_PATH];
	char defMf[8] = "??";

	if (baseAddr + 0x101F == (DWORD)addr) {
		isInside = true;
	}

	if (((DWORD)addr & 0xFFFD0000) == baseAddr) {
		strcpy(defMf, "a.exe");
	}

	char *module = defMf;

	/*do {
		MODULEENTRY32 me32;

		// Take a snapshot of all modules in the specified process.
		if (hModuleSnap == INVALID_HANDLE_VALUE)
		{
			break;
		}

		// Set the size of the structure before using it.
		me32.dwSize = sizeof(MODULEENTRY32);

		// Retrieve information about the first module,
		// and exit if unsuccessful
		if (!Module32First(hModuleSnap, &me32))
		{
			CloseHandle(hModuleSnap);           // clean the snapshot object
			break;
		}

		// Now walk the module list of the process,
		// and display information about each module
		do
		{
			if ((me32.modBaseAddr <= addr) && ((me32.modBaseAddr + me32.modBaseSize) > addr)) {
				module = me32.szModule;
				break;
			}

		} while (Module32Next(hModuleSnap, &me32));
	} while (0);

	for (char *t = module; *t != '\0'; ++t) {
		*t = toupper(*t);
	}

	rev::GetCurrentRegisters(cbCtx, &cReg);

	TmpPrint("%-15s+%08X EAX:%08x ECX:%08x EDX:%08x EBX:%08x ESP:%08x EBP:%08x ESI:%08x EDI:%08x\r\n", 
		module, 
		(DWORD)addr,
		cReg.eax,
		cReg.ecx,
		cReg.edx,
		cReg.ebx,
		cReg.esp,
		cReg.ebp,
		cReg.esi,
		cReg.edi
	);*/

	if (NULL == hKernel32Dll) {
		if (NULL != (hKernel32Dll = GetModuleHandle("kernel32.dll"))) {
			IPFPFunc = GetProcAddress(hKernel32Dll, "IsProcessorFeaturePresent");
		}
	}

	/*if (NULL != IPFPFunc) {
		if (addr == IPFPFunc) {
			__asm int 3;
		}
	}*/


	/*if (baseAddr + 0x4463 == (DWORD)addr) {
		rev::MarkMemory((rev::ADDR_TYPE)(baseAddr + 0x30540), 0x0F);
	}*/

	/* some very simple control structure going three times forwards and two times back */
	DWORD ret = EXECUTION_ADVANCE;
	/*if (isInside) {
		if (c->execCount % 3 == 1) {
			ret = EXECUTION_BACKTRACK;
		}
	}*/

	static int blockCount = 1000000;

	blockCount--;
	if (0 == blockCount) {
		return EXECUTION_TERMINATE;
	}

	return ret;
}

void InitializeRevtracer(rev::ADDR_TYPE entryPoint) {
	HMODULE hNtDll = GetModuleHandle("ntdll.dll");
	rev::RevtracerAPI *api = &rev::revtracerAPI;

	api->dbgPrintFunc = DebugPrint;
	
	api->lowLevel.ntAllocateVirtualMemory = GetProcAddress(hNtDll, "NtAllocateVirtualMemory");
	api->lowLevel.ntFreeVirtualMemory = GetProcAddress(hNtDll, "NtFreeVirtualMemory");
	
	api->lowLevel.ntQueryInformationThread = GetProcAddress(hNtDll, "NtQueryInformationThread");
	api->lowLevel.ntTerminateProcess = GetProcAddress(hNtDll, "NtTerminateProcess");

	api->lowLevel.rtlNtStatusToDosError = GetProcAddress(hNtDll, "RtlNtStatusToDosError");
	api->lowLevel.vsnprintf_s = GetProcAddress(hNtDll, "_vsnprintf_s");

	api->executionControl = CustomExecutionController;


	rev::RevtracerConfig *config = &rev::revtracerConfig;
	config->contextSize = 4;
	config->entryPoint = entryPoint;
	config->featureFlags = TRACER_FEATURE_TRACKING;

	rev::Initialize();
}

int main(unsigned int argc, char *argv[]) {
	if (!MapPE(baseAddr)) {
		return false;
	}

	HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
	TmpPrint("ntdll.dll @ 0x%08x\n", (DWORD)hNtDll);

	fDbg = CreateFileA("dbg.log", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
	//fopen_s(&fDbg, "dbg.log", "wt");
	InitializeRevtracer((rev::ADDR_TYPE)(baseAddr + 0x96CE));
	//InitializeRevtracer((rev::ADDR_TYPE)(baseAddr + 0x13A6));
	//InitializeRevtracer((rev::ADDR_TYPE)tmp);

	//rev::MarkMemory((rev::ADDR_TYPE)(baseAddr + 0x2B000), 0x01);
	//rev::MarkMemory((rev::ADDR_TYPE)(baseAddr + 0x2B004), 0x02);
	
	rev::MarkMemoryValue((rev::ADDR_TYPE)(baseAddr + 0x60540), 0x01);
	rev::MarkMemoryValue((rev::ADDR_TYPE)(baseAddr + 0x60544), 0x02);
	rev::MarkMemoryValue((rev::ADDR_TYPE)(baseAddr + 0x60548), 0x04);

	//rev::MarkMemoryName((rev::ADDR_TYPE)(baseAddr + 0x60540), "a");
	//rev::MarkMemoryName((rev::ADDR_TYPE)(baseAddr + 0x60544), "b");
	//rev::MarkMemoryName((rev::ADDR_TYPE)(baseAddr + 0x60548), "c");
	rev::Execute(argc, argv);
	
	CloseHandle(fDbg);
	return 0;
}
