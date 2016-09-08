#include <Windows.h>

#include "ExternExecutionController.h"
#include "InprocessExecutionController.h"

DLL_PUBLIC_EXECUTION ExecutionController *NewExecutionController(uint32_t type) {
	switch (type) {
		case EXECUTION_EXTERNAL:
			return new ExternExecutionController();
		case EXECUTION_INPROCESS:
			return new InprocessExecutionController();
		default:
			return nullptr;
	};
}

DLL_PUBLIC_EXECUTION void DeleteExecutionController(ExecutionController *ptr) {
	delete ptr;
}


BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved) {
	return TRUE;
}