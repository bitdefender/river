#include "CommonExecutionController.h"
#include "../revtracer/DebugPrintFlags.h"
#include "../CommonCrossPlatform/Common.h"
#include <iostream>

#ifdef __linux__
#include <stdarg.h>
#include <stdio.h>
#include "../libproc/os-linux.h"

#include <sys/time.h>
#include <sys/resource.h>

#include <libgen.h> //basename
#include <asm/ldt.h>
#else
#include <Windows.h>
#include <Psapi.h>
#include <tlhelp32.h>
#include <Winternl.h>
#endif


#ifdef _WIN32
void InitSegment(HANDLE hThread, DWORD dwSeg, DWORD &offset) {
	LDT_ENTRY entry;
	DWORD base, limit;

	if (FALSE == GetThreadSelectorEntry(hThread, dwSeg, &entry)) {
		base = 0xFFFFFFFF;
		limit = 0;
	}
	else {
		base = entry.BaseLow | (entry.HighWord.Bytes.BaseMid << 16) | (entry.HighWord.Bytes.BaseHi << 24);
		limit = entry.LimitLow | (entry.HighWord.Bits.LimitHi << 16);
	}

	if (entry.HighWord.Bits.Granularity) {
		limit = (limit << 12) | 0x0FFF;
	}

	offset = base;
}


void InitSegments(void *hThread, nodep::DWORD *segments) {
	for (DWORD i = 0; i < 0x100; ++i) {
		InitSegment((HANDLE)hThread, i, segments[i]);
	}
}
#elif defined(__linux__)
#define __NR_get_thread_area 244
void InitSegments(void *hThread, nodep::DWORD *segments) {
	struct user_desc* table_entry_ptr = nullptr;

	table_entry_ptr = (struct user_desc*)malloc(sizeof(struct user_desc));

	// segments max size if 0xff, so index max is 0x1f
	for (int index = 0; index < 0x1f; ++index) {
		table_entry_ptr->entry_number = index;
		int ret = syscall( __NR_get_thread_area,
				table_entry_ptr);
		// if entry number is not available
		unsigned value = 0x0;
		if (ret == -1 && errno != EINVAL) {
			printf("Error found when get_thread_area. errno %d\n", errno);
			DEBUG_BREAK;
		} else if (ret == 0) {
			value = table_entry_ptr->base_addr;
		}

		for (int j = 0; j < 8; ++j) {
			nodep::WORD segmentvalue = (index << 3) | j;
			// bit #2 specifies LDT(set) or GDT(unset)
			if (j & 0x4) {
				segments[segmentvalue] = 0xffffffff;
			} else {
				segments[segmentvalue] = value;
			}
		}
	}
	free(table_entry_ptr);
}
#endif


class DefaultObserver : public ExecutionObserver {
public :
	unsigned int ExecutionBegin(void *ctx, void *address) {
		return EXECUTION_ADVANCE;
	}

	unsigned int ExecutionControl(void *ctx, void *address) {
		return EXECUTION_ADVANCE;
	}

	unsigned int ExecutionEnd(void *ctx) {
		return EXECUTION_TERMINATE;
	}

	unsigned int TranslationError(void *ctx, void *address) {
		return EXECUTION_TERMINATE;
	}

	void TerminationNotification(void *ctx) { }
} defaultObserver;


const rev::RevtracerVersion CommonExecutionController::supportedVersion = {
	0,
	1,
	1
};

CommonExecutionController::CommonExecutionController() {
	execState = NEW;

	path = L"";
	cmdLine = L"";
	entryPoint = nullptr;
	featureFlags = EXECUTION_FEATURE_REVERSIBLE | EXECUTION_FEATURE_TRACKING;

	context = NULL;

	observer = &defaultObserver;

	virtualSize = commitedSize = 0;
	updated = false;

	trackCb = nullptr;
	markCb = nullptr;
	symbCb = nullptr;
}

int CommonExecutionController::GetState() const {
	return (int)execState;
}

bool CommonExecutionController::SetPath(const wstring &p) {
	if (execState == RUNNING) {
		return false;
	}

	std::wcout << "[CommonExecutionController] Setting tracee path to [" << p.c_str() << "]\n";
	path = p;
	execState = INITIALIZED;
	return true;
}

bool CommonExecutionController::SetCmdLine(const wstring &c) {
	if (execState == RUNNING) {
		return false;
	}

	cmdLine = c;
	return true;
}

bool CommonExecutionController::SetEntryPoint(void *ep) {
	if (execState == RUNNING) {
		return false;
	}

	entryPoint = ep;
	return true;
}

bool CommonExecutionController::SetExecutionFeatures(unsigned int feat) {
	featureFlags = feat;
	return true;
}

void CommonExecutionController::SetExecutionObserver(ExecutionObserver * obs) {
	observer = obs;
}

void CommonExecutionController::SetTrackingObserver(rev::TrackCallbackFunc track, rev::MarkCallbackFunc mark) {
	trackCb = track;
	markCb = mark;
}

void CommonExecutionController::SetSymbolicHandler(rev::SymbolicHandlerFunc symb) {
	symbCb = symb;
}

unsigned int CommonExecutionController::ExecutionBegin(void *address, void *cbCtx) {
	execState = SUSPENDED_AT_START;
	return observer->ExecutionBegin(cbCtx, address);
}

unsigned int CommonExecutionController::ExecutionControl(void *address, void *cbCtx) {
	return observer->ExecutionControl(cbCtx, address);
}

unsigned int CommonExecutionController::ExecutionEnd(void *cbCtx) {
	execState = SUSPENDED_AT_TERMINATION;
	return observer->ExecutionEnd(cbCtx);
}

unsigned int CommonExecutionController::TranslationError(void *address, void *cbCtx) {
	return observer->TranslationError(cbCtx, address);
}

struct _VM_COUNTERS_ {
	ULONG PeakVirtualSize;
	ULONG VirtualSize; // << VM use 
	ULONG PageFaultCount;
	ULONG PeakWorkingSetSize;
	ULONG WorkingSetSize;
	ULONG QuotaPeakPagedPoolUsage;
	ULONG QuotaPagedPoolUsage;
	ULONG QuotaPeakNonPagedPoolUsage;
	ULONG QuotaNonPagedPoolUsage;
	ULONG PagefileUsage; // << Commited use
	ULONG PeakPagefileUsage;
};

#ifdef __linux__

long get_prss() {
	struct rusage rusage;
	getrusage( RUSAGE_SELF, &rusage );

	return (size_t)(rusage.ru_maxrss * 1024L);
}

char *TrimModulePath(char *modulePath) {
	return basename(modulePath);
}

void CommonExecutionController::PrintModules() {
	for (auto it = mod.begin(); it != mod.end(); ++it) {
		printf("## %s base: 0x%08X size: 0x%08X\n", it->Name, it->ModuleBase,
				it->Size);
	}
}

bool CommonExecutionController::UpdateLayout() {
	if (updated) {
		return true;
	}

	if ((virtualSize == get_prss()) && (commitedSize == get_rss())) {
		return true;
	}

	virtualSize = get_prss();
	commitedSize = get_rss();

	sec.clear();
	mod.clear();

	struct map_iterator mi;
	struct map_prot mp;
	unsigned long hi;
	unsigned long segbase, mapoff;


	if (maps_init (&mi, getpid()) < 0) {
		updated = false;
		return false;
	}


	while (maps_next (&mi, &segbase, &hi, &mapoff, &mp)) {
		VirtualMemorySection vms;
		vms.BaseAddress = segbase;
		vms.Protection = mp.prot;
		vms.RegionSize = hi - segbase;
		sec.push_back(vms);

		ssize_t pathLen = strlen(mi.path);
		char *moduleName = TrimModulePath(mi.path);

		if ((0 == mod.size()) ||
				(0 != strcmp(moduleName, mod[mod.size() - 1].Name))) {
			ModuleInfo minfo;

			if (0 == pathLen) {
				//handle anonymous mapping
				minfo.anonymous = true;
			} else {
				minfo.anonymous = false;
				memset(minfo.Name, 0, MAX_PATH);
				strncpy(minfo.Name, moduleName, pathLen);
			}

			minfo.ModuleBase = segbase;
			minfo.Size = hi - segbase;
			mod.push_back(minfo);
		} else {
			ModuleInfo &mi = mod[mod.size() - 1];
			mi.Size += hi - segbase;
		}

	}

	updated = true;
	return true;
}
#else
bool CommonExecutionController::UpdateLayout() {
	if (updated) {
		return true;
	}

	ULONG retLen;
	_VM_COUNTERS_ ctrs;

	THREAD_T hProcess = GetProcessHandle();

	NtQueryInformationProcess(
		hProcess,
		(PROCESSINFOCLASS)3,
		&ctrs,
		sizeof(ctrs),
		&retLen
	);

	if ((ctrs.VirtualSize == virtualSize) && (ctrs.PagefileUsage == commitedSize)) {
		return true;
	}

	virtualSize = ctrs.VirtualSize;
	commitedSize = ctrs.PagefileUsage;

	sec.clear();
	mod.clear();

	char mf[MAX_PATH];

	for (DWORD dwAddr = 0; dwAddr < 0x7FFF0000;) {
		MEMORY_BASIC_INFORMATION32 mbi;

		if (0 == VirtualQueryEx(hProcess, (LPCVOID)dwAddr, (PMEMORY_BASIC_INFORMATION)&mbi, sizeof(mbi))) {
			return false;
		}

		VirtualMemorySection vms;
		vms.BaseAddress = dwAddr;
		vms.Protection = mbi.Protect;
		vms.RegionSize = mbi.RegionSize;
		vms.State = mbi.State;
		vms.Type = mbi.Type;
		sec.push_back(vms);

		DWORD dwSz = GetMappedFileName(hProcess, (LPVOID)dwAddr, mf, (sizeof(mf) / sizeof(mf[0])) - 1);
		if (0 != dwSz) {
			char *module = mf;
			for (DWORD i = 1; i < dwSz; ++i) {
				if (mf[i - 1] == '\\') {
					module = &mf[i];
				}
			}

			for (char *t = module; *t != '\0'; ++t) {
				*t = tolower(*t);
			}

			if ((0 == mod.size()) || (0 != strcmp(module, mod[mod.size() - 1].Name))) {
				ModuleInfo mi;
				strcpy_s(mi.Name, module);
				mi.ModuleBase = dwAddr;
				mi.Size = mbi.RegionSize;

				mod.push_back(mi);
			}
			else {
				ModuleInfo &mi = mod[mod.size() - 1];

				mi.Size = dwAddr - mi.ModuleBase + mbi.RegionSize;
			}
		}


		dwAddr = mbi.BaseAddress + mbi.RegionSize;
	}

	updated = true;
	return true;
}
#endif

bool CommonExecutionController::GetProcessVirtualMemory(VirtualMemorySection *&sections, int &sectionCount) {
	if (!UpdateLayout()) {
		return false;
	}

	sectionCount = sec.size();
	sections = &sec[0];

	return true;
}

bool CommonExecutionController::GetModules(ModuleInfo *&modules, int &moduleCount) {
	if (!UpdateLayout()) {
		return false;
	}

	moduleCount = mod.size();
	modules = &mod[0];
	return true;
}

int GeneratePrefix(char *buff, int size, ...) {
	va_list va;

	va_start(va, size);
	int sz = VSNPRINTF_S(
		buff,
		size,
		size - 1,
		"[%3s|%5s|%3s|%c] ",
		va
	);
	va_end(va);

	return sz;
}

#ifdef ENABLE_RIVER_SIDE_DEBUGGING
FILE_T hDbg = OPEN_FILE_W("execution.log");

void vDebugPrintf(const DWORD printMask, const char *fmt, va_list args) {
	DWORD dwWr;
	char tmpBuff[512];

	char pfxBuff[] = "[___|_____|___|_]      ";
	static char lastChar = '\n';

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

	if ('\n' == lastChar) {

		int sz = GeneratePrefix(
			pfxBuff,
			sizeof(pfxBuff),
			messageTypes[(printMask & PRINT_MESSAGE_MASK) >> PRINT_MESSAGE_SHIFT],
			executionStages[(printMask & PRINT_EXECUTION_MASK) >> PRINT_EXECUTION_SHIFT],
			codeTypes[(printMask & PRINT_CODE_TYPE_MASK) >> PRINT_CODE_TYPE_SHIFT],
			codeDirections[(printMask & PRINT_CODE_DIRECTION_MASK) >> PRINT_CODE_DIRECTION_SHIFT]
		);
		//debugLog.Write(pfxBuff, sz);

		BOOL ret;
		WRITE_FILE(hDbg, pfxBuff, sz, dwWr, ret);
	}

	int sz = VSNPRINTF_S(tmpBuff, sizeof(tmpBuff) - 1, sizeof(tmpBuff) - 1, fmt, args);
	
	if (sz) {
		//debugLog.Write(tmpBuff, sz);

		BOOL ret;
		WRITE_FILE(hDbg, tmpBuff, sz, dwWr, ret);
		lastChar = tmpBuff[sz - 1];
	}
}

void DebugPrintf(const unsigned int printMask, const char *fmt, ...) {
#ifdef IS_DEBUG_BUILD // don't worry about function call overhead, it will be optimized by compiler
	va_list va;
	va_start(va, fmt);
	vDebugPrintf(printMask, fmt, va);
	va_end(va);
#endif
}

void CommonExecutionController::DebugPrintf(const unsigned long printMask, const char * fmt, ...) {
#ifdef IS_DEBUG_BUILD // don't worry about function call overhead, it will be optimized by compiler
	va_list va;
	va_start(va, fmt);
	vDebugPrintf(printMask, fmt, va);
	va_end(va);
#endif
}

#else
void DebugPrintf(const unsigned int printMask, const char *fmt, ...) { }
#endif


void CommonExecutionController::GetFirstEsp(void *ctx, nodep::DWORD &esp) {
	gfe(ctx, esp);
}

void CommonExecutionController::GetCurrentRegisters(void *ctx, rev::ExecutionRegs *regs) {
	gcr(ctx, regs);
}

void *CommonExecutionController::GetMemoryInfo(void *ctx, void *ptr) {
	return gmi(ctx, ptr);
}

bool CommonExecutionController::GetLastBasicBlockInfo(void *ctx, rev::BasicBlockInfo *bbInfo) {
	return glbbi(ctx, bbInfo);
}

void CommonExecutionController::MarkMemoryValue(void *ctx, rev::ADDR_TYPE addr, nodep::DWORD value) {
	mmv(ctx, addr, value);
}

void CommonExecutionController::onBeforeTrackingInstructionCheck(void *address, void *cbCtx)
{
	rev::BasicBlockInfo bbInfo;
	GetLastBasicBlockInfo(cbCtx, &bbInfo);
	observer->setCurrentExecutedBasicBlockDesc(&bbInfo);
}