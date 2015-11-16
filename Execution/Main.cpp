#include <Windows.h>

#include "InternalExecutionController.h"

EXECUTION_LINKAGE ExecutionController *NewExecutionController() {
	return new InternalExecutionController();
}

EXECUTION_LINKAGE void DeleteExecutionController(ExecutionController *ptr) {
	delete ptr;
}


BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved) {
	return TRUE;
}