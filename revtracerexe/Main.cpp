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

void DebugPrint(const char *fmt, ...) {
	va_list va;
	char tmpBuff[512];

	va_start(va, fmt);
	int sz = _vsnprintf_s(tmpBuff, sizeof(tmpBuff)-1, fmt, va);
	va_end(va);

	unsigned long wr;
	WriteFile(fDbg, tmpBuff, sz * sizeof(tmpBuff[0]), &wr, NULL);
}

__declspec(dllimport) int vsnprintf_s(
	char *buffer,
	size_t sizeOfBuffer,
	size_t count,
	const char *format,
	va_list argptr
);

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
		DebugPrint("EAX inconsistent!\n");
		return false;
	}

	if (r1.ecx != r2.ecx) {
		DebugPrint("ECX inconsistent!\n");
		return false;
	}

	if (r1.edx != r2.edx) {
		DebugPrint("EDX inconsistent!\n");
		return false;
	}

	if (r1.ebx != r2.ebx) {
		DebugPrint("EBX inconsistent!\n");
		return false;
	}

	if (r1.esp != r2.esp) {
		DebugPrint("ESP inconsistent!\n");
		return false;
	}

	if (r1.ebp != r2.ebp) {
		DebugPrint("EBP inconsistent!\n");
		return false;
	}

	if (r1.esi != r2.esi) {
		DebugPrint("ESI inconsistent!\n");
		return false;
	}

	if (r1.edi != r2.edi) {
		DebugPrint("ESI inconsistent!\n");
		return false;
	}

	if (r1.eflags != r2.eflags) {
		DebugPrint("EFLAGS inconsistent!\n");
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

	do {
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
	);

	/*if (0xE39A == ((DWORD)addr & 0xFFFF)) {
		__asm int 3;
	}*/

	/*switch (c->execCount % 3) {
		case 0:
			RegCheck(rgs[0], cReg);
			break;
		case 1:
			memcpy(&rgs[0], &cReg, sizeof(cReg));
			break;
		case 2:
			break;
	}*/

	
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
	if (isInside) {
		if (c->execCount % 3 == 1) {
			ret = EXECUTION_BACKTRACK;
		}
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

	rev::Initialize();
}

const unsigned char tmp[] = {
	0xf0, 0x0f, 0xc7, 0x0f,
	/*0x8b, 0x45, 0xfc,
	0xf6, 0x40, 0x07, 0x80,
	0x0f, 0x84, 0x4a, 0x4b, 0x00, 0x00,*/
	0xc3
};



int main(unsigned int argc, char *argv[]) {
	if (!MapPE(baseAddr)) {
		return false;
	}

	HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
	TmpPrint("ntdll.dll @ 0x%08x\n", (DWORD)hNtDll);

	fDbg = CreateFileA("dbg.log", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
	//fopen_s(&fDbg, "dbg.log", "wt");
	//InitializeRevtracer((rev::ADDR_TYPE)(baseAddr + 0x96CE));
	InitializeRevtracer((rev::ADDR_TYPE)(baseAddr + 0x13A6));
	//InitializeRevtracer((rev::ADDR_TYPE)tmp);

	rev::MarkMemory((rev::ADDR_TYPE)(baseAddr + 0x2B004), 0x0F);
	rev::Execute(argc, argv);
	
	CloseHandle(fDbg);

	/*struct _exec_env *pEnv;
	struct UserCtx *ctx;
	DWORD dwCount = 0;
	pEnv = new _exec_env(0x1000000, 0x10000, 0x2000000, 16, 0x10000);

	pEnv->userContext = AllocUserContext(pEnv, sizeof(struct UserCtx));
	ctx = (struct UserCtx *)pEnv->userContext;
	ctx->callCount = 0;

	unsigned char *pMain = (unsigned char *)baseAddr + 0x96CE;
	DWORD ret = call_cdecl_2(pEnv, (_fn_cdecl_2)pMain, (void *)argc, (void *)argv);
	DbgPrint("Done. ret = %d\n\n", ret);

	PrintStats(pEnv);

	DbgPrint(" +++ Tainted addresses +++ \n");
	ac.PrintAddreses();
	DbgPrint(" +++++++++++++++++++++++++ \n");*/

	
	return 0;
}
