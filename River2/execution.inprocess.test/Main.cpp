#include "../Execution/Execution.h"
#include "../CommonCrossPlatform/Common.h"

#ifdef _WIN32
#include <Windows.h>
#elif defined(__linux__)
#include "../libproc/os-linux.h"
#endif


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
		rev::BasicBlockInfo bbInfo;
		ctrl->GetLastBasicBlockInfo(ctx, &bbInfo);

		const char unkmod[MAX_PATH] = "???";
		unsigned int offset = (DWORD)bbInfo.address;
		int foundModule = -1;

		for (int i = 0; i < mCount; ++i) {
			if ((mInfo[i].ModuleBase <= (DWORD)bbInfo.address) && ((DWORD)bbInfo.address < mInfo[i].ModuleBase + mInfo[i].Size)) {
				offset -= mInfo[i].ModuleBase;
				foundModule = i;
				break;
			}
		}


		const char module[] = "";
		fprintf(fBlocks, "%-15s + %08lX (%4ld)\n",
			(-1 == foundModule) ? unkmod : mInfo[foundModule].Name,
			(DWORD)offset,
			bbInfo.cost
		);
		return EXECUTION_ADVANCE;
	}

	virtual unsigned int ExecutionEnd(void *ctx) {
		fflush(fBlocks);
		return EXECUTION_TERMINATE;
	}

	virtual unsigned int TranslationError(void *ctx, void *address) {
		printf("Translation error @%08p\n", address);
		return EXECUTION_TERMINATE;
	}

} observer;

#define MAX_BUFF 4096
#ifdef _WIN32
#define IMPORT __declspec(dllimport)
#else
#define IMPORT extern
#endif

extern "C" {
	IMPORT char payloadBuffer[];
	IMPORT int Payload();
};
#ifdef __linux__
extern "C"  void patch__rtld_global_ro();
#endif

int main(int argc, char **argv) {
#ifdef __linux__
	patch__rtld_global_ro();
#endif
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

#ifdef _WIN32
	/* dirty hack: patch _isa_available on the loaded dll */
	HMODULE hPayload = GetModuleHandle(L"ParserPayload.dll");
	*(DWORD *)((BYTE *)hPayload + 0x00117288) = 0x00000001;
	*(DWORD *)((BYTE *)hPayload + 0x001796E0) = 0x00000000;
#endif


	FOPEN(observer.fBlocks, "e.t.txt", "wt");

	ctrl = NewExecutionController(EXECUTION_INPROCESS);
	ctrl->SetEntryPoint((void*)Payload);
	
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
