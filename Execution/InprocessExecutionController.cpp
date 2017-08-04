#include "InprocessExecutionController.h"
#include "RiverStructs.h"
#include "../CommonCrossPlatform/Common.h"
#include "../revtracer-wrapper/RevtracerWrapper.h"
#include "../BinLoader/LoaderAPI.h"
#include "../wrapper.setup/Wrapper.Setup.h"

#ifdef __linux__
#include <string.h>
#endif

#ifdef __linux__
#define MAX_FUNC_LIST 32
struct libc_ifunc_impl
{
	/* The name of function to be tested.  */
	const char *name;
	/* The address of function to be tested.  */
	void (*fn) (void);
	/* True if this implementation is usable on this machine.  */
	bool usable;
};

typedef size_t (*__libc_ifunc_impl_list_handle)(const char *name,
                       struct libc_ifunc_impl *array,
                       size_t max);
static __libc_ifunc_impl_list_handle my__libc_ifunc_impl_list;
#endif

#ifdef __linux__
#define CREATE_THREAD(tid, func, params, ret) do { ret = pthread_create((&tid), nullptr, (func), (params)); ret = (0 == ret); } while(false)
#define JOIN_THREAD(tid, ret) do { ret = pthread_join(tid, nullptr); ret = (0 == ret); } while (false)

#else
#define CREATE_THREAD(tid, func, params, ret) do { tid = CreateThread(nullptr, 0, (func), (params), 0, nullptr); ret = (tid != nullptr); } while (false)
#define JOIN_THREAD(tid, ret) do { ret = WaitForSingleObject(tid, INFINITE); ret = (WAIT_FAILED != ret); } while (false)
#endif

typedef ADDR_TYPE(*GetHandlerCallback)(void);

bool InprocessExecutionController::SetPath(const wstring &) {
	return false;
}

bool InprocessExecutionController::SetCmdLine(const wstring &c) {
	return false;
}

bool InprocessExecutionController::MarkErrorHandlingBB() {
#ifdef __linux__
	/* 1. get library imports. check if __errno_location is an import
	 * 2. get libc handler - find __errno_location offset
	 * 3. compute address
	 * 4. check call instructions
	 */
	LIB_T libcHandler;
	libcHandler = GET_LIB_HANDLER("libc.so");
	if (nullptr == libcHandler) {
		DEBUG_BREAK;
		return false;
	}

	DWORD local__errno_location = (DWORD) dlsym(libcHandler,
			"__errno_location");
	DWORD libcBase = ((DWORD *)libcHandler)[0];

	DWORD offset__errno_location = local__errno_location - libcBase;

	//revtracer.pConfig->address__errno_location = local__errno_location;
	//revtracer.pConfig->offset__errno_location = offset__errno_location;

#endif
	return true;
}

bool InprocessExecutionController::AnalyzeTargetModule() {
#ifdef __linux__
	/* Open tracer module with binloader and
	 * - check if call relocation is R_386_PC32 or got.plt
	 * - address of __errno_location in .plt
	 * - address of __errno_location in .got
	 */
	char symbolName[] = "__errno_location";
	size_t symbolSize = strlen(symbolName);

	ldr::AbstractBinary *targetModule = nullptr;
	CreateModule(targetModulePath.c_str(), targetModule);

	if (targetModule == nullptr) {
		DEBUG_BREAK;
	}

	if (!targetModule->IsGlobalSymbolPresent(symbolName, symbolSize)) {
		return false;
	}

#endif
	return true;
}

bool InprocessExecutionController::PatchProcess() {
#ifdef __linux__
	LIB_T libcHandler;
	struct libc_ifunc_impl func_list[MAX_FUNC_LIST];

	std::vector<std::string> func_names = {
		"bcopy", "bzero", "memchr", "memcmp", "__memmove_chk",
		"memmove", "memrchr", "__memset_chk", "memset",
		"rawmemchr", "stpncpy", "stpcpy", "strcasecmp",
		"strcasecmp_l", "strcat", "strchr", "strcmp",
		"strcpy", "strcspn", "strncasecmp", "strncasecmp_l",
		"strncat", "strncpy", "strnlen", "strpbrk", "strrchr",
		"strspn", "wcschr", "wcscmp", "wcscpy", "wcslen", "wcsrchr",
		"wmemcmp", "__memcpy_chk", "memcpy", "__mempcpy_chk",
		"mempcpy", "strlen", "strncmp"
	};

	libcHandler = GET_LIB_HANDLER("libc.so");
	if (nullptr == libcHandler) {
		DEBUG_BREAK;
		return false;
	}

	my__libc_ifunc_impl_list = (__libc_ifunc_impl_list_handle)dlsym(
			libcHandler, "__libc_ifunc_impl_list");
	if (nullptr == my__libc_ifunc_impl_list) {
		DEBUG_BREAK;
		return false;
	}

	for (auto it = func_names.begin(); it != func_names.end(); ++it) {
		ADDR_TYPE detourAddr = nullptr;
		memset(func_list, 0,
				MAX_FUNC_LIST * sizeof(struct libc_ifunc_impl));
		(my__libc_ifunc_impl_list)(it->c_str(), func_list, MAX_FUNC_LIST);
		for (int i = 0; i < MAX_FUNC_LIST; ++i) {
			if (!func_list[i].name || !func_list[i].fn)
				break;
			char *ia32 = strstr((char*)func_list[i].name, "ia32");
			if (nullptr != ia32 && (strlen(ia32) == strlen("ia32"))) {
				detourAddr = (ADDR_TYPE)func_list[i].fn;
				break;
			}
		}

		if (nullptr == detourAddr) {
			DEBUG_BREAK;
			return false;
		}
		for (int i = 0; i < MAX_FUNC_LIST; ++i) {
			if (!func_list[i].name || !func_list[i].fn)
				break;
			if ((ADDR_TYPE)func_list[i].fn != detourAddr) {
				revtracer.pConfig->hooks[revtracer.pConfig->hookCount].originalAddr = (ADDR_TYPE)func_list[i].fn;
				revtracer.pConfig->hooks[revtracer.pConfig->hookCount].detourAddr = detourAddr;
				revtracer.pConfig->hookCount++;
			}
		}

	}
#endif
	return true;
}


THREAD_T InprocessExecutionController::GetProcessHandle() {
	return GET_CURRENT_PROC();
}

rev::ADDR_TYPE InprocessExecutionController::GetTerminationCode() {
	return wrapper.pExports->getTerminationCode();
}

bool InprocessExecutionController::ReadProcessMemory(unsigned int base, unsigned int size, unsigned char * buff) {
	memcpy(buff, (LPCVOID)base, size);
	return true;
}

bool InprocessExecutionController::WriteProcessMemory(unsigned int base, unsigned int size, unsigned char * buff) {
	memcpy((LPVOID)base, buff, size);
	return true;
}

#ifdef __linux__
void *ThreadProc(void *p) {
	InprocessExecutionController *_this = (InprocessExecutionController *)p;
	_this->ControlThread();
  return nullptr;
}
#else
DWORD __stdcall ThreadProc(LPVOID p) {
	InprocessExecutionController *_this = (InprocessExecutionController *)p;
	return _this->ControlThread();
}
#endif

//unsigned int BranchHandler(void *context, void *userContext, void *nextInstruction);
void SyscallControlFunc(void *context, void *userContext);

typedef void(*InitializeFunc)();
typedef void(*ExecuteFunc)(int argc, char *argv[]);

InitializeFunc revtracerInitialize = nullptr;
ExecuteFunc revtraceExecute = nullptr;

bool InprocessExecutionController::Execute() {
#ifdef _WIN32
	wchar_t revWrapperPath[] = L"revtracer-wrapper.dll";
#else
	wchar_t revWrapperPath[] = L"librevtracerwrapper.so";
#endif

	LOAD_LIBRARYW(L"revtracer.dll", revtracer.module, revtracer.base);
	LOAD_LIBRARYW(revWrapperPath, wrapper.module, wrapper.base);

	if (((0 == revtracer.module) && (0 == revtracer.base)) ||
        ((0 == wrapper.module) && (0 == wrapper.base))) {
		DEBUG_BREAK;
		return false;
	}

	revtracer.pConfig = (rev::RevtracerConfig *)GET_PROC_ADDRESS(revtracer.module, revtracer.base, "revtracerConfig");
	revtracer.pImports = (rev::RevtracerImports *)GET_PROC_ADDRESS(revtracer.module, revtracer.base, "revtracerImports");
	revtracer.pExports = (rev::RevtracerExports *)GET_PROC_ADDRESS(revtracer.module, revtracer.base, "revtracerExports");
	
	if ((nullptr == revtracer.pConfig) || (nullptr == revtracer.pImports) || (nullptr == revtracer.pExports)) {
		DEBUG_BREAK;
		return false;
	}

	revtracer.pImports->dbgPrintFunc = ::DebugPrintf;

	revtracer.pImports->branchHandler = BranchHandlerFunc;
	revtracer.pImports->errorHandler = ErrorHandlerFunc;
	revtracer.pImports->syscallControl = SyscallControlFunc;

	if (nullptr != trackCb) {
		revtracer.pImports->trackCallback = trackCb;
	}

	if (nullptr != markCb) {
		revtracer.pImports->markCallback = markCb;
	}

	if (nullptr != symbCb) {
		revtracer.pImports->symbolicHandler = symbCb;
	}

	wrapper.pImports = (revwrapper::WrapperImports *)GET_PROC_ADDRESS(wrapper.module, wrapper.base, "wrapperImports");
	wrapper.pExports = (revwrapper::WrapperExports *)GET_PROC_ADDRESS(wrapper.module, wrapper.base, "wrapperExports");

	wrapper.pImports->libraries = &libLayout;
	if (!InitWrapperOffsets(&libLayout, wrapper.pImports)) {
		DEBUG_BREAK;
		return false;
	}

	if (!wrapper.pExports->initRevtracerWrapper(nullptr)) {
		DEBUG_BREAK;
		return false;
	}

	revtracer.pImports->memoryAllocFunc = wrapper.pExports->allocateMemory;
	revtracer.pImports->memoryFreeFunc = wrapper.pExports->freeMemory;
	revtracer.pImports->lowLevel.ntTerminateProcess = (rev::ADDR_TYPE)wrapper.pExports->terminateProcess;
	revtracer.pImports->lowLevel.vsnprintf_s = (rev::ADDR_TYPE)wrapper.pExports->formattedPrint;
	gcr = revtracer.pExports->getCurrentRegisters;
	gmi = revtracer.pExports->getMemoryInfo;
	mmv = revtracer.pExports->markMemoryValue;
	glbbi = revtracer.pExports->getLastBasicBlockInfo;

	if ((nullptr == gcr) || (nullptr == gmi) || (nullptr == mmv) || (nullptr == glbbi)) {
		DEBUG_BREAK;
		return false;
	}

	revtracer.pConfig->entryPoint = entryPoint;
	revtracer.pConfig->featureFlags = featureFlags;
	revtracer.pConfig->context = this;
	revtracer.pConfig->hookCount = 0;
	
	revtracerInitialize = (InitializeFunc)GET_PROC_ADDRESS(revtracer.module, revtracer.base, "Initialize");
	revtraceExecute = (ExecuteFunc)GET_PROC_ADDRESS(revtracer.module, revtracer.base, "Execute");

	PatchProcess();
	AnalyzeTargetModule();

	int ret;
	CREATE_THREAD(hThread, ThreadProc, this, ret);

#ifdef _WIN32
	InitSegments(hThread, revtracer.pConfig->segmentOffsets);
#endif

	return TRUE == ret;
}


DWORD InprocessExecutionController::ControlThread() {
	revtracerInitialize();
	revtraceExecute(0, nullptr);

	execState = TERMINATED;
	observer->TerminationNotification(context);
	return 0;
}

bool InprocessExecutionController::WaitForTermination() {
	int ret;
	JOIN_THREAD(hThread, ret);
	return TRUE == ret;
}

