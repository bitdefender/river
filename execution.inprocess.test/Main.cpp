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
	ModuleInfo *mInfo;
	int mCount;

	virtual void TerminationNotification(void *ctx) {
		printf("Process Terminated\n");
		SIGNAL_EVENT(hEvent);
	}

	virtual unsigned int ExecutionBegin(void *ctx, void *address) {
		printf("Process starting\n");
		ctrl->GetModules(mInfo, mCount);
		return EXECUTION_ADVANCE;
	}

	virtual unsigned int ExecutionControl(void *ctx, void *address) {
		const wchar_t unkmod[MAX_PATH] = L"???";
		unsigned int offset = (DWORD)address;
		int foundModule = -1;

		for (int i = 0; i < mCount; ++i) {
			if ((mInfo[i].ModuleBase <= (DWORD)address) && ((DWORD)address < mInfo[i].ModuleBase + mInfo[i].Size)) {
				offset -= mInfo[i].ModuleBase;
				foundModule = i;
				break;
			}
		}


		const char module[] = "";
		fprintf(fBlocks, "%-15ws + %08X\n",
			(-1 == foundModule) ? unkmod : mInfo[foundModule].Name,
			(DWORD)offset
		);
		return EXECUTION_ADVANCE;
	}

	virtual unsigned int ExecutionEnd(void *ctx) {
		fflush(fBlocks);
		return EXECUTION_TERMINATE;
	}
} observer;

#define MAX_BUFF 4096
__declspec (dllimport) char payloadBuffer[];
__declspec (dllimport) int Payload();

int main() {

	char *buff = payloadBuffer;
	unsigned int bSize = MAX_BUFF;
	do {
		fgets(buff, bSize, stdin);
		while (*buff) {
			buff++;
			bSize--;
		}
	} while (!feof(stdin));
	//Payload();


	fopen_s(&observer.fBlocks, "e.t.txt", "wt");

	ctrl = NewExecutionController(EXECUTION_INPROCESS);
	ctrl->SetEntryPoint(Payload);
	
	ctrl->SetExecutionFeatures(0);

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
