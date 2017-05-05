#include "InprocessExecutionController.h"
#include "RiverStructs.h"
#include "../CommonCrossPlatform/Common.h"
#include "../revtracer-wrapper/RevtracerWrapper.h"
#include "../BinLoader/LoaderAPI.h"
#include "../wrapper.setup/Wrapper.Setup.h"

#ifdef __linux__
#include <string.h>
#endif

bool InprocessExecutionController::SetPath(const wstring &p) {
	return false;
}

bool InprocessExecutionController::SetCmdLine(const wstring &c) {
	return false;
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
	InitWrapperOffsets(&libLayout, wrapper.pImports);

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
	glbbc = revtracer.pExports->getLastBasicBlockInfo;

	if ((nullptr == gcr) || (nullptr == gmi) || (nullptr == mmv) || (nullptr == glbbc)) {
		DEBUG_BREAK;
		return false;
	}

	//config->contextSize = sizeof(CustomExecutionContext);
	revtracer.pConfig->entryPoint = entryPoint;
	revtracer.pConfig->featureFlags = featureFlags;
	revtracer.pConfig->context = this;
	//revtracer.pConfig->sCons = symbolicConstructor;


	//revtracer.pConfig->sCons = SymExeConstructor;

	revtracerInitialize = (InitializeFunc)GET_PROC_ADDRESS(revtracer.module, revtracer.base, "Initialize");
	revtraceExecute = (ExecuteFunc)GET_PROC_ADDRESS(revtracer.module, revtracer.base, "Execute");

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

