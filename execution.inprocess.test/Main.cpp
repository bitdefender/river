#include "../Execution/Execution.h"
#ifdef _WIN32
#include <Windows.h>
#endif

#include "../Execution/Common.h"

ExecutionController *ctrl = NULL;
EVENT_T hEvent;

class CustomObserver : public ExecutionObserver {
public :
	FILE *fBlocks;

	virtual void TerminationNotification(void *ctx) {
		printf("Process Terminated\n");
		SIGNAL_EVENT(hEvent);
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
		fprintf(fBlocks, "%-15s+%08lX EAX:%08lx ECX:%08lx EDX:%08lx EBX:%08lx ESP:%08lx EBP:%08lx ESI:%08lx EDI:%08lx\n",
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
} observer;

extern int Payload();

int main() {

	FOPEN(observer.fBlocks, "e.t.txt", "wt");

	ctrl = NewExecutionController(EXECUTION_INPROCESS);
	ctrl->SetEntryPoint((void*)Payload);

	ctrl->SetExecutionFeatures(EXECUTION_FEATURE_REVERSIBLE | EXECUTION_FEATURE_TRACKING);

	ctrl->SetExecutionObserver(&observer);

	CREATE_EVENT(hEvent);

	ctrl->Execute();

	WAIT_FOR_SINGLE_OBJECT(hEvent);

	DeleteExecutionController(ctrl);
	ctrl = NULL;

	fclose(observer.fBlocks);
	system("pause");

	return 0;
}
