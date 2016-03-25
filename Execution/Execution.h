#ifndef _EXECUTION_H

#include <string>
#include <cstdint>

#ifdef _EXECUTION_EXPORTS
#define EXECUTION_LINKAGE __declspec(dllexport)
#else
#define EXECUTION_LINKAGE __declspec(dllimport)
#endif

#define EXECUTION_FEATURE_REVERSIBLE		0x00000001
#define EXECUTION_FEATURE_TRACKING			0x00000004

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

struct Registers {
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;

	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint32_t eflags;
};

class ExecutionController;

typedef void(__stdcall *TerminationNotifyFunc)(void *ctx);
typedef unsigned int(__stdcall *ExecutionBeginFunc)(void *ctx, unsigned int address);
typedef unsigned int(__stdcall *ExecutionControlFunc)(void *ctx, unsigned int address);
typedef unsigned int(__stdcall *ExecutionEndFunc)(void *ctx);

class ExecutionController {
public:
	virtual int GetState() const = 0;
	virtual bool SetPath(const wstring &) = 0;
	virtual bool SetCmdLine(const wstring &) = 0;
	virtual bool SetExecutionFeatures(unsigned int feat) = 0;
	virtual bool Execute() = 0;
	virtual bool Terminate() = 0;

	virtual void *GetProcessHandle() = 0;

	virtual bool GetProcessVirtualMemory(VirtualMemorySection *&sections, int &sectionCount) = 0;
	virtual bool GetModules(ModuleInfo *&modules, int &moduleCount) = 0;
	virtual bool ReadProcessMemory(unsigned int base, unsigned int size, unsigned char *buff) = 0;

	virtual void SetNotificationContext(void *ctx) = 0;

	virtual void SetTerminationNotification(TerminationNotifyFunc func) = 0;
	virtual void SetExecutionBeginNotification(ExecutionBeginFunc func) = 0;
	virtual void SetExecutionControlNotification(ExecutionControlFunc func) = 0;
	virtual void SetExecutionEndNotification(ExecutionEndFunc func) = 0;

	virtual void GetCurrentRegisters(Registers &registers) = 0;
};

EXECUTION_LINKAGE ExecutionController *NewExecutionController();
EXECUTION_LINKAGE void DeleteExecutionController(ExecutionController *);

#endif