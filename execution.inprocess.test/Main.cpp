#include "../Execution/Execution.h"
#ifdef _WIN32
#include <Windows.h>
#endif

#include "../Execution/Common.h"
#include "../libproc/os-linux.h"
#include <stdio.h>

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
		const char unkmod[MAX_PATH] = "???";
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
		fprintf(fBlocks, "%-15s + %08X\n",
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
#ifdef _WIN32
#define IMPORT __declspec(dllimport)
#else
#define IMPORT extern
#endif

IMPORT char payloadBuffer[];
IMPORT int Payload();

#ifdef __linux__
void patch__rtld_global_ro(void) {

	struct map_iterator mi;
	struct map_prot mp;
	unsigned long hi;
	unsigned long segbase, mapoff;


	if (maps_init (&mi, getpid()) < 0) {
		return;
	}


	bool found = false;
	while (maps_next (&mi, &segbase, &hi, &mapoff, &mp)) {
		if (0 != strstr(mi.path, "/lib/i386-linux-gnu/ld-2.23.so")) {
			if (!found) {
				found = true;
				continue;
			}
			ssize_t len = hi - segbase;
			mprotect((void *)segbase, len, PROT_READ | PROT_WRITE);
			void *_rtld_global_ro = (void *)segbase + 0xd00;
			off_t sse_flag = 64;

			void *_rtld_global_ro_sse = _rtld_global_ro + sse_flag;

			//*((unsigned int*)_rtld_global_ro_sse) &= 0xfffeffff;
			*((unsigned int*)_rtld_global_ro_sse) &= 0xfffffdff;
			mprotect((void *)segbase, len, PROT_READ);
			//printf("%d\n", memcmp((void*)segbase, (void*)hi, len));
			maps_close(&mi);
			return;
		}
	}

	maps_close(&mi);
}
#endif


int main() {
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
	*(DWORD *)((BYTE *)hPayload + 0x001796E8) = 0x00000000;
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
