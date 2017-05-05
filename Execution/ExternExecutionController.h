#ifndef _EXTERN_EXECUTION_CONTROLLER_H
#define _EXTERN_EXECUTION_CONTROLLER_H

#include "CommonExecutionController.h"

#include "../BinLoader/Abstract.Loader.h"
#include "../BinLoader/LoaderAPI.h"
#include "../BinLoader/PE.Loader.h"
#include "DualAllocator.h"

#include "../loader/Loader.h"
#include "../revtracer-wrapper/RevtracerWrapper.h"
#include "../ipclib/ipclib.h"
#include "../revtracer/revtracer.h"


#include "../CommonCrossPlatform/Common.h"
#include "../CommonCrossPlatform/LibraryLayout.h"

#ifdef _WIN32
#include <Windows.h>
#elif defined(__linux__)
#endif

class ExternExecutionController : public CommonExecutionController {
private:
	HANDLE hProcess /* child process */, hMainThread;
	THREAD_T hControlThread;
	FILE_T hDbg;
	DWORD pid, currentPid;

	DualAllocator* shmAlloc;

	ext::LibraryLayout *libraryLayout;

	struct {
		ldr::AbstractBinary *module;
		BASE_PTR base;
		ldr::LoaderConfig vConfig;
		ldr::LoaderConfig *pConfig;
		ldr::LoaderImports *pImports;
		ldr::LoaderExports vExports; 
		ldr::LoaderExports *pExports;
	} loader;

	struct {
		ldr::AbstractBinary *module;
		BASE_PTR base;
		revwrapper::WrapperImports *pImports;
		revwrapper::WrapperExports *pExports;
	} wrapper;

	struct {
		ldr::AbstractBinary *module;
		BASE_PTR base;
		ipc::IpcImports *pImports;
		ipc::IpcExports *pExports;

		BYTE *pIPFPFunc;
	} ipc;
	
	struct {
		ldr::AbstractBinary *module;
		BASE_PTR base;
		rev::RevtracerConfig *pConfig;
		rev::RevtracerImports *pImports;
		rev::RevtracerExports *pExports;
	} revtracer;

	char debugBuffer[4096];

	FILE_T hBlocksFd;

	bool InitializeAllocator();
	bool InitLibraryLayout();
	bool MapLoader();
	bool MapWrapper();
	bool MapTracer();
	bool WriteLoaderConfig();

	bool InitializeWrapper();
	bool InitializeIpcLib();
	bool InitializeRevtracer();

	bool SwitchEntryPoint();
	bool PatchProcess();

	void ConvertWideStringPath(char *result, size_t len);
	void MapSharedLibraries(unsigned long baseAddress);
public:
	ExternExecutionController();

	virtual bool SetEntryPoint();

	virtual bool Execute();
	virtual bool WaitForTermination();

	DWORD ControlThread();

	virtual THREAD_T GetProcessHandle();
	virtual rev::ADDR_TYPE GetTerminationCode();

	virtual bool ReadProcessMemory(unsigned int base, unsigned int size, unsigned char *buff);

	virtual unsigned int ExecutionBegin(void *address, void *cbCtx);
};


#endif
