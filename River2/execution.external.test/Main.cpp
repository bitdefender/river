#include "../Execution/Execution.h"

#include <Windows.h>

ExecutionController *ctrl = NULL;
HANDLE hEvent;

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

		/*if (0xE39A == (address & 0xFFFF)) {
		__asm int 3;
		}*/

		/*if (b) {*/
		ctrl->GetCurrentRegisters(ctx, &rgs);

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

	virtual unsigned int TranslationError(void *ctx, void *address) {
		printf("Translation error @%08p\n", address);
		return EXECUTION_TERMINATE;
	}

} observer;

int main() {

	fopen_s(&observer.fBlocks, "e.t.txt", "wt");

	ctrl = NewExecutionController(EXECUTION_EXTERNAL);
	ctrl->SetPath(L"D:\\wrk\\evaluators\\lzo\\a.exe");
	
	ctrl->SetExecutionFeatures(0 /*EXECUTION_FEATURE_REVERSIBLE | EXECUTION_FEATURE_TRACKING*/);

	ctrl->SetExecutionObserver(&observer);
	//ctrl->SetExecutionControlNotification(ExecControl);
	//ctrl->SetTerminationNotification(Term);

	hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	ctrl->Execute();

	WaitForSingleObject(hEvent, INFINITE);

	DeleteExecutionController(ctrl);
	ctrl = NULL;

	fclose(observer.fBlocks);
	system("pause");

	return 0;
}