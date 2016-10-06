#ifdef _WIN32
#include <Windows.h>
#endif

#ifndef INPROCESS_EXECUTION_ONLY
#include "ExternExecutionController.h"
#endif
#include "InprocessExecutionController.h"

DLL_PUBLIC_EXECUTION ExecutionController *NewExecutionController(uint32_t type) {
	switch (type) {
#ifndef INPROCESS_EXECUTION_ONLY
		case EXECUTION_EXTERNAL:
			return new ExternExecutionController();
#endif
		case EXECUTION_INPROCESS:
			return new InprocessExecutionController();
		default:
			return nullptr;
	};
}

DLL_PUBLIC_EXECUTION void DeleteExecutionController(ExecutionController *ptr) {
	delete ptr;
}


#ifdef _WIN32
BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved) {
	return TRUE;
}
#endif
