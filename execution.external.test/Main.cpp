#include "../Execution/Execution.h"

#include <Windows.h>

ExecutionController *ctrl = NULL;
HANDLE hEvent;

void __stdcall Term(void *ctx) {
	printf("Process Terminated\n");
	SetEvent(hEvent);
}

unsigned int __stdcall ExecBegin(void *ctx, unsigned int address) {
	printf("Process starting\n");
	return EXECUTION_ADVANCE;
}

FILE *fBlocks = NULL;
unsigned int __stdcall ExecControl(void *ctx, void *address) {
	//static bool b = true;
	Registers rgs;

	/*if (0xE39A == (address & 0xFFFF)) {
		__asm int 3;
	}*/

	/*if (b) {*/
		ctrl->GetCurrentRegisters(rgs);

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

unsigned int __stdcall ExecEnd(void *ctx) {
	return EXECUTION_TERMINATE;
}

int main() {

	fopen_s(&fBlocks, "e.t.txt", "wt");

	ctrl = NewExecutionController(EXECUTION_EXTERNAL);
	ctrl->SetPath(L"D:\\wrk\\evaluators\\lzo\\a.exe");
	
	ctrl->SetExecutionFeatures(EXECUTION_FEATURE_REVERSIBLE | EXECUTION_FEATURE_TRACKING);

	ctrl->SetExecutionControlNotification(ExecControl);
	ctrl->SetTerminationNotification(Term);

	hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	ctrl->Execute();

	WaitForSingleObject(hEvent, INFINITE);

	DeleteExecutionController(ctrl);
	ctrl = NULL;

	fclose(fBlocks);
	system("pause");

	return 0;
}