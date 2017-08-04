#ifndef _INPROCESS_EXECUTION_CONTROLLER_H_
#define _INPROCESS_EXECUTION_CONTROLLER_H_

#include "CommonExecutionController.h"
/*#ifdef _WIN32
#include <Windows.h>
#endif*/

#include "../revtracer-wrapper/RevtracerWrapper.h"
#include "../revtracer/revtracer.h"
#include "../CommonCrossPlatform/Common.h"
#include "../BinLoader/LoaderAPI.h"

class InprocessExecutionController : public CommonExecutionController {
private :
	THREAD_T hThread;

	ext::LibraryLayout libLayout, **expLayout;
	
	struct {
		MODULE_PTR module;
		BASE_PTR base;
		revwrapper::WrapperImports *pImports;
		revwrapper::WrapperExports *pExports;
	} wrapper;

	struct {
		MODULE_PTR module;
		BASE_PTR base;
		rev::RevtracerConfig *pConfig;
		rev::RevtracerImports *pImports;
		rev::RevtracerExports *pExports;
	} revtracer;
public :
	virtual bool SetCmdLine(const wstring &c);

	bool PatchProcess();
	bool MarkErrorHandlingBB();
	bool AnalyzeTargetModule();

	virtual THREAD_T GetProcessHandle();
	virtual rev::ADDR_TYPE GetTerminationCode();

	/*virtual bool GetProcessVirtualMemory(VirtualMemorySection *&sections, int &sectionCount);
	virtual bool GetModules(ModuleInfo *&modules, int &moduleCount);*/
	virtual bool ReadProcessMemory(unsigned int base, unsigned int size, unsigned char *buff);
	virtual bool WriteProcessMemory(unsigned int base, unsigned int size, unsigned char *buff);

	DWORD ControlThread();

	virtual bool Execute();
	virtual bool WaitForTermination();
};

#endif
