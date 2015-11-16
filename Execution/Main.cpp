#include <Windows.h>

#include "Execution.h"

#include <vector>

#include "Loader/PE.ldr.h"
#include "Loader/Extern.Mapper.h"

using namespace std;

class InternalExecutionController : public ExecutionController {
private :
	wstring path;
	wstring cmdLine;

	HANDLE hProcess, hMainThread;
	DWORD pid, mainTid;

	vector<VirtualMemorySection> sec;

	enum {
		NEW = EXECUTION_NEW,
		INITIALIZED = EXECUTION_INITIALIZED,
		RUNNING = EXECUTION_RUNNING,
		TERMINATED = EXECUTION_TERMINATED,
		ERR = EXECUTION_ERR
	} execState;

public :
	InternalExecutionController() {
		execState = NEW;

		path = L"";
		cmdLine = L"";
	}

	virtual int GetState() const {
		return (int)execState;
	}

	virtual bool SetPath(const wstring &p) {
		if (execState == RUNNING) {
			return false;
		}

		path = p;
		execState = INITIALIZED;
		return true;
	}

	virtual bool SetCmdLine(const wstring &c) {
		if (execState == RUNNING) {
			return false;
		}

		cmdLine = c;
		return true;
	}

	virtual bool Execute() {
		STARTUPINFOW startupInfo;
		PROCESS_INFORMATION processInfo;

		memset(&startupInfo, 0, sizeof(startupInfo));
		startupInfo.cb = sizeof(startupInfo);

		memset(&processInfo, 0, sizeof(processInfo));

		if (execState == INITIALIZED) {
			BOOL bRet = CreateProcessW(
				path.c_str(),
				NULL, //cmdLine.c_str(),
				NULL,
				NULL,
				FALSE,
				CREATE_SUSPENDED,
				NULL,
				NULL,
				&startupInfo,
				&processInfo
			);

			if (bRet) {
				execState = RUNNING;

				hProcess = processInfo.hProcess;
				hMainThread = processInfo.hThread;

				pid = processInfo.dwProcessId;
				mainTid = processInfo.dwThreadId;

				ExternMapper eMapper((DWORD)hProcess);
				FloatingPE fpe("InjectedDll.dll");
				ADDR_TYPE base = 0;

				DWORD execPayloadRva, originalEipRva, wr;
				fpe.GetExport("ExecutePayload", execPayloadRva);
				fpe.GetExport("originalEip", originalEipRva);
				fpe.MapPE(eMapper, base);
				
				CONTEXT tCtx;
				memset(&tCtx, 0, sizeof(tCtx));
				tCtx.ContextFlags = CONTEXT_ALL;
				GetThreadContext(hMainThread, &tCtx);

				WriteProcessMemory(hProcess, (LPVOID)(base + originalEipRva), &tCtx.Eip, sizeof(tCtx.Eip), &wr);

				tCtx.Eip = base + execPayloadRva;

				SetThreadContext(hMainThread, &tCtx);
				ResumeThread(hMainThread);

				return true;
			} else {
				execState = ERR;
			}
		}

		return false;
	}

	virtual bool Terminate() {
		TerminateProcess(hProcess, 0x00000000);

		CloseHandle(hProcess);
		CloseHandle(hMainThread);

		execState = TERMINATED;

		return true;
	}

	virtual bool GetProcessVirtualMemory(VirtualMemorySection *&sections, int &sectionCount) {
		sec.clear();

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

			dwAddr = mbi.BaseAddress + mbi.RegionSize;
			sec.push_back(vms);
		}

		sectionCount = sec.size();
		sections = &sec[0];

		return true;
	}
};

EXECUTION_LINKAGE ExecutionController *NewExecutionController() {
	return new InternalExecutionController();
}

EXECUTION_LINKAGE void DeleteExecutionController(ExecutionController *ptr) {
	delete ptr;
}


BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved) {
	return TRUE;
}