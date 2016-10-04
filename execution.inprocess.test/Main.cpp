#include "../Execution/Execution.h"
#include "../SyscallNumbers/SyscallNumber.h"
#include "Hook.h"

#include <Windows.h>

ExecutionController *ctrl = NULL;
HANDLE hEvent;

void *ntRaiseException;

class CustomObserver : public ExecutionObserver {
public :
	FILE *fBlocks;

	virtual void TerminationNotification(void *ctx) {
		printf("Process Terminated\n");
		SetEvent(hEvent);
	}

	virtual unsigned int ExecutionBegin(void *ctx, void *address) {
		printf("Process starting\n");
		return EXECUTION_ADVANCE;
	}

	virtual unsigned int ExecutionControl(void *ctx, void *address) {
		//static bool b = true;
		rev::ExecutionRegs rgs;

		ctrl->GetCurrentRegisters(ctx, &rgs);
		if (address == ntRaiseException) {
			DWORD *stack = (DWORD *)rgs.esp;
			EXCEPTION_RECORD *ex = (EXCEPTION_RECORD *)stack[1];
			CONTEXT *ctx = (CONTEXT *)stack[2];
			BOOLEAN handle = (BOOLEAN)stack[3];

			printf("NtRaiseException; retAddr %08p\n", stack[0]);
		}

		/*if (0xE39A == (address & 0xFFFF)) {
		__asm int 3;
		}*/

		/*if (b) {*/
		
		const char module[] = "";
		fprintf(fBlocks, "%-15s+%08X EAX:%08x ECX:%08x EDX:%08x EBX:%08x ESP:%08x EBP:%08x ESI:%08x EDI:%08x\n",
			module,
			(DWORD)address,
			rgs.eax,
			rgs.ecx,
			rgs.edx,
			rgs.ebx,
			rgs.esp,
			rgs.ebp,
			rgs.esi,
			rgs.edi
		);
		/*}*/

		return EXECUTION_ADVANCE;
	}

	virtual unsigned int ExecutionEnd(void *ctx) {
		return EXECUTION_TERMINATE;
	}

	virtual unsigned int ExecutionSyscall(void *ctx) {

		rev::ExecutionRegs rgs;

		ctrl->GetCurrentRegisters(ctx, &rgs);

		const char *syscallName = syscall::GetName(rgs.eax);

		if ((nullptr != syscallName) && (0 == strcmp("NtContinue", syscallName))) {
			DWORD *stack = (DWORD *)rgs.esp;
			CONTEXT *context = (CONTEXT *)stack[2];

			context->Eip = (DWORD)ctrl->ControlTransfer(ctx, (rev::ADDR_TYPE)context->Eip);
		}

		return 0;
	}
} observer;

extern int Payload();
extern void InitPayload();

int main() {

	//InitPayload();
	//Payload();

	syscall::Init();

	HMODULE hNtDll = GetModuleHandle(L"ntdll.dll");
	ntRaiseException = GetProcAddress(hNtDll, "NtRaiseException");

	fopen_s(&observer.fBlocks, "e.t.txt", "wt");

	ctrl = NewExecutionController(EXECUTION_INPROCESS);
	ctrl->SetEntryPoint(Payload);
	
	ctrl->SetExecutionFeatures(0 /*EXECUTION_FEATURE_REVERSIBLE | EXECUTION_FEATURE_TRACKING*/);

	ctrl->SetExecutionObserver(&observer);
	
	hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	ctrl->Execute();

	WaitForSingleObject(hEvent, INFINITE);

	DeleteExecutionController(ctrl);
	ctrl = NULL;

	fclose(observer.fBlocks);
	
	return 0;
}