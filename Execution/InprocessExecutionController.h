#ifndef _INPROCESS_EXECUTION_CONTROLLER_H_
#define _INPROCESS_EXECUTION_CONTROLLER_H_

#include "CommonExecutionController.h"
#include <Windows.h>

#include "../revtracer/revtracer.h"

#ifdef _LINUX
#include <pthread.h>
typedef pthread_t THREAD_T;
#else
typedef HANDLE THREAD_T;
#endif

class InprocessExecutionController : public CommonExecutionController {
private :
	THREAD_T hThread;

	rev::RevtracerConfig *revCfg;
public :
	virtual bool SetPath();
	virtual bool SetCmdLine();

	virtual void *GetProcessHandle();

	virtual bool GetProcessVirtualMemory(VirtualMemorySection *&sections, int &sectionCount);
	virtual bool GetModules(ModuleInfo *&modules, int &moduleCount);
	virtual bool ReadProcessMemory(unsigned int base, unsigned int size, unsigned char *buff);

	DWORD ControlThread();

	virtual bool Execute();
	virtual bool WaitForTermination();
};

#endif
