#ifndef _EXECUTION_H

#include <string>
#include <cstdint>

#ifdef _EXECUTION_EXPORTS
#define EXECUTION_LINKAGE __declspec(dllexport)
#else
#define EXECUTION_LINKAGE __declspec(dllimport)
#endif

#define EXECUTION_NEW					0x00
#define EXECUTION_INITIALIZED			0x01
#define EXECUTION_RUNNING				0x02
#define EXECUTION_TERMINATED			0x03
#define EXECUTION_ERR					0x04

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

class ExecutionController {
public:
	virtual int GetState() const = 0;
	virtual bool SetPath(const wstring &) = 0;
	virtual bool SetCmdLine(const wstring &) = 0;
	virtual bool Execute() = 0;
	virtual bool Terminate() = 0;

	virtual bool GetProcessVirtualMemory(VirtualMemorySection *&sections, int &sectionCount) = 0;
};

EXECUTION_LINKAGE ExecutionController *NewExecutionController();
EXECUTION_LINKAGE void DeleteExecutionController(ExecutionController *);

#endif