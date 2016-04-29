#include "../Execution/Execution.h"
#include <vector>
#include <memory>
#include <cstring>

#include <time.h>
#include <Windows.h>

ExecutionController *ctrl = NULL;
HANDLE hEvent;
FILE *fVars;

struct ProcessAddress {
	wchar_t moduleName[MAX_PATH];
	DWORD offset;

	bool resolved;
	DWORD address;

	ProcessAddress(wchar_t *mn, DWORD off) {
		if (L'\0' == mn[0]) {
			address = offset;
			resolved = true;
		} else {
			resolved = false;
		}
		wcscpy_s(moduleName, mn);
		offset = off;
		
	}

	bool Resolve(const ModuleInfo *modules, int moduleCount) {
		if (resolved) {
			return true;
		}

		for (int i = 0; i < moduleCount; ++i) {
			if (0 == wcscmp(modules[i].Name, moduleName)) {
				address = modules[i].ModuleBase + offset;
				resolved = true;
				return true;
			}
		}

		return false;
	}
};

struct FuzzTestItem {
	char varName[16];
	ProcessAddress trigger;
	ProcessAddress bufferStart;
	DWORD bufferSize;
	bool used;

	FuzzTestItem(char *var, wchar_t *triggerModule, DWORD triggerOffset, wchar_t *bufferModule, DWORD bufferOffset, DWORD bufferSz) :
		trigger(triggerModule, triggerOffset),
		bufferStart(bufferModule, bufferOffset)
	{
		strncpy(varName, var, sizeof(varName) - 1);
		bufferSize = bufferSz;
		used = false;
	}

	bool Resolve(const ModuleInfo *modules, int moduleCount) {
		return
			trigger.Resolve(modules, moduleCount) &&
			bufferStart.Resolve(modules, moduleCount);
	}
};

typedef void(*TriggerCallback)(const FuzzTestItem &itm);

struct FuzzConfig {
	std::vector<FuzzTestItem> testItems;
	bool allResolved;

	FuzzConfig() {
		allResolved = true;
	}

	void AddItem(char *name, wchar_t *triggerModule, DWORD triggerOffset, wchar_t *bufferModule, DWORD bufferOffset, DWORD bufferSize) {
		testItems.emplace_back(
			name,
			triggerModule,
			triggerOffset,
			bufferModule,
			bufferOffset,
			bufferSize
		);

		allResolved = false;
	}

	void Resolve(const ModuleInfo *modules, int moduleCount) {
		if (allResolved) {
			return;
		}

		bool d = true;
		for (auto itr = testItems.begin(); itr != testItems.end(); ++itr) {
			d &= itr->Resolve(modules, moduleCount);
		}

		if (d) {
			allResolved = true;
		}
	}

	void Trigger(DWORD eip, TriggerCallback tcb) {
		for (auto itr = testItems.begin(); itr != testItems.end(); ++itr) {
			if (
				(itr->trigger.resolved) && 
				(itr->bufferStart.resolved) &&
				(eip == itr->trigger.address)
			) {
				tcb(*itr);
			}
		}
	}
};


FuzzConfig fConfig;

void FuzzTest(const FuzzTestItem &itm) {
	std::unique_ptr<unsigned char []> buffer(new unsigned char[itm.bufferSize]);
	SIZE_T wr;
	char alphabet[] = "0123456789+-*/% ";

	int rSize = rand() % (itm.bufferSize - 1);

	for (DWORD i = 0; i < rSize; ++i) {
		buffer[i] = alphabet[rand() % sizeof(alphabet)];
	}

	buffer[rSize] = '\0';

	fprintf(fVars, "%s = %s\n", itm.varName, buffer);

	WriteProcessMemory(
		ctrl->GetProcessHandle(),
		(LPVOID)itm.bufferStart.address,
		(LPVOID)buffer.get(),
		itm.bufferSize,
		&wr
	);
}

void __stdcall Term(void *ctx) {
	printf("Process Terminated\n");
	SetEvent(hEvent);
}

unsigned int seed = 0;
unsigned int __stdcall ExecBegin(void *ctx, unsigned int address) {
	printf("Process starting\n");
	srand(seed);
	return EXECUTION_ADVANCE;
}

FILE *fBlocks = NULL;
unsigned int __stdcall ExecControl(void *ctx, unsigned int address) {
	ModuleInfo *mds;
	int mCount;

	ctrl->GetModules(mds, mCount);

	
	fConfig.Resolve(mds, mCount);
	fConfig.Trigger(address, FuzzTest);
	
	const wchar_t unkmod[MAX_PATH] = L"???";
	unsigned int offset = address;
	int foundModule = -1;

	for (int i = 0; i < mCount; ++i) {
		if ((mds[i].ModuleBase <= address) && (address < mds[i].ModuleBase + mds[i].Size)) {
			offset -= mds[i].ModuleBase;
			foundModule = i;
			break;
		}
	}

	fprintf(fBlocks, "%-15ws+%08X\n",
		(-1 == foundModule) ? unkmod : mds[foundModule].Name,
		(DWORD)offset
	);

	static int blockCount = 100000;

	/*blockCount--;
	if (0 == blockCount) {
		return EXECUTION_TERMINATE;
	}*/


	return EXECUTION_ADVANCE;
}

unsigned int __stdcall ExecEnd(void *ctx) {
	return EXECUTION_TERMINATE;
}

int main() {

	/*fConfig.AddItem(
		"a",
		L"a.exe", 0x4463,
		L"a.exe", 0x30540,
		0x20000
	);*/

	fConfig.AddItem(
		"a",
		L"expr.exe",  0x1070,
		L"expr.exe", 0x2B000,
		0x100
	);

	seed = (unsigned int)time(NULL);
	srand(seed);

	char fbname[] = "fuzzout\\e.________.blocks.txt";
	sprintf(fbname, "fuzzout\\e.%08x.blocks.txt", seed);
	fBlocks = fopen(fbname, "wt");

	char fvname[] = "fuzzout\\e.________.vars.txt";
	sprintf(fvname, "fuzzout\\e.%08x.vars.txt", seed);
	fVars = fopen(fvname, "wt");

	ctrl = NewExecutionController();
	ctrl->SetPath(L"D:\\wrk\\evaluators\\expression\\expr.exe");
	
	ctrl->SetExecutionFeatures(0);

	ctrl->SetExecutionBeginNotification(ExecBegin);
	ctrl->SetExecutionControlNotification(ExecControl);
	ctrl->SetExecutionEndNotification(ExecEnd);

	ctrl->SetTerminationNotification(Term);

	hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	ctrl->Execute();

	WaitForSingleObject(hEvent, INFINITE);

	DeleteExecutionController(ctrl);
	ctrl = NULL;

	fclose(fBlocks);

	return 0;
}
