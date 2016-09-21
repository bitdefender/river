#ifndef _EXECUTION_H

#include <string>
#include <cstdint>

#include "../revtracer/revtracer.h"

#if defined _WIN32 || defined __CYGWIN__
	#ifdef _EXECUTION_EXPORTS
		#ifdef __GNUC__
			#define DLL_PUBLIC_EXECUTION __attribute__ ((dllexport))
		#else
			#define DLL_PUBLIC_EXECUTION __declspec(dllexport)
		#endif
	#else
		#ifdef __GNUC__
			#define DLL_PUBLIC_EXECUTION __attribute__ ((dllimport))
		#else
			#define DLL_PUBLIC_EXECUTION __declspec(dllimport)
		#endif
	#endif
	#define DLL_LOCAL
#else
	#if __GNUC__ >= 4
		#define DLL_PUBLIC_EXECUTION __attribute__ ((visibility ("default")))
		#define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
	#else
		#define DLL_PUBLIC_EXECUTION
		#define DLL_LOCAL
	#endif
#endif

#ifdef __linux__
typedef pthread_t THREAD_T;
#else
typedef void* THREAD_T;
#endif

#define EXECUTION_INPROCESS						0x00000000
#define EXECUTION_EXTERNAL						0x00000001

#define EXECUTION_FEATURE_REVERSIBLE			0x00000001
#define EXECUTION_FEATURE_TRACKING				0x00000002
#define EXECUTION_FEATURE_ADVANCED_TRACKING		0x00000004 // never use this flag --- use _SYMBOLIC instead
#define EXECUTION_FEATURE_SYMBOLIC				EXECUTION_FEATURE_TRACKING | EXECUTION_FEATURE_ADVANCED_TRACKING

#define EXECUTION_ADVANCE					0x00000000
#define EXECUTION_BACKTRACK					0x00000001
#define EXECUTION_TERMINATE					0x00000002

#define EXECUTION_NEW							0x00
#define EXECUTION_INITIALIZED					0x01
#define EXECUTION_SUSPENDED_AT_START			0x02
#define EXECUTION_RUNNING						0x03
#define EXECUTION_SUSPENDED						0x04
#define EXECUTION_SUSPENDED_AT_TERMINATION		0x05
#define EXECUTION_TERMINATED					0x06
#define EXECUTION_ERR							0xFF

using namespace std;

#define EXEC_MEM_COMMIT					0x01000
#define EXEC_MEM_FREE					0x10000
#define EXEC_MEM_RESERVE				0x02000

#define EXEC_MEM_IMAGE					0x1000000
#define EXEC_MEM_MAPPED					0x0040000
#define EXEC_MEM_PRIVATE				0x0020000

#define EXEC_PAGE_NOACCESS				0x01
#define EXEC_PAGE_EXECUTE				0x10
#define EXEC_PAGE_READONLY				0x02
#define EXEC_PAGE_READWRITE				0x04
#define EXEC_PAGE_GUARD					0x100
#define EXEC_PAGE_NOCACHE				0x200
#define EXEC_PAGE_WRITECOMBINE			0x400

struct VirtualMemorySection {
	uint32_t BaseAddress;
	uint32_t RegionSize;
	uint32_t State; // free, reserved, commited
	uint32_t Protection; // read, write, execute
	uint32_t Type; // image, mapped, private
};

struct ModuleInfo {
	wchar_t Name[260];
	uint32_t ModuleBase;
	uint32_t Size;
};

/*struct Registers {
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;

	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint32_t eflags;
};*/

class ExecutionObserver {
public :
	virtual unsigned int ExecutionBegin(void *ctx, void *address) = 0;
	virtual unsigned int ExecutionControl(void *ctx, void *address) = 0;
	virtual unsigned int ExecutionEnd(void *ctx) = 0;

	virtual void TerminationNotification(void *ctx) = 0;
};

class ExecutionController {
public:
	virtual int GetState() const = 0;
	virtual bool SetPath(const wstring &) = 0;
	virtual bool SetCmdLine(const wstring &) = 0;
	virtual bool SetEntryPoint(void *ep) = 0;
	virtual bool SetExecutionFeatures(unsigned int feat) = 0;
	virtual bool Execute() = 0;
	virtual bool WaitForTermination() = 0;

	virtual THREAD_T GetProcessHandle() = 0;

	virtual bool GetProcessVirtualMemory(VirtualMemorySection *&sections, int &sectionCount) = 0;
	virtual bool GetModules(ModuleInfo *&modules, int &moduleCount) = 0;
	virtual bool ReadProcessMemory(unsigned int base, unsigned int size, unsigned char *buff) = 0;

	virtual void SetExecutionObserver(ExecutionObserver *obs) = 0;
	virtual void SetTrackingObserver(rev::TrackCallbackFunc track, rev::MarkCallbackFunc mark) = 0;
	virtual void SetSymbolicHandler(rev::SymbolicHandlerFunc symb) = 0;

	virtual unsigned int ExecutionBegin(void *address, void *cbCtx) = 0;
	virtual unsigned int ExecutionControl(void *address, void *cbCtx) = 0;
	virtual unsigned int ExecutionEnd(void *cbCtx) = 0;

	virtual void DebugPrintf(const unsigned long printMask, const char *fmt, ...) = 0;

	// in-execution api
	virtual void GetCurrentRegisters(void *ctx, rev::ExecutionRegs *registers) = 0;
	virtual void *GetMemoryInfo(void *ctx, void *ptr) = 0;
	virtual void MarkMemoryValue(void *ctx, rev::ADDR_TYPE addr, rev::DWORD value) = 0;

};

DLL_PUBLIC_EXECUTION extern ExecutionController *NewExecutionController(uint32_t type);
DLL_PUBLIC_EXECUTION extern void DeleteExecutionController(ExecutionController *);

#endif
