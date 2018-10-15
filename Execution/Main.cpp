#ifndef DISABLE_EXTERN_EXECUTION
#include "ExternExecutionController.h"
#endif

#ifndef DISABLE_INPROCESS_EXECUTION
#include "InprocessExecutionController.h"
#endif

DLL_EXECUTION_PUBLIC ExecutionController *NewExecutionController(uint32_t type) {
	switch (type) {
#ifndef DISABLE_EXTERN_EXECUTION
		case EXECUTION_EXTERNAL:
			return new ExternExecutionController();
#endif
#ifndef DISABLE_INPROCESS_EXECUTION
		case EXECUTION_INPROCESS:
			return new InprocessExecutionController();
#endif
		default:
			return nullptr;
	};
}

DLL_EXECUTION_PUBLIC void DeleteExecutionController(ExecutionController *ptr) {
	delete ptr;
}


#ifdef _WIN32
#include <Windows.h>
BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved) {
	return TRUE;
}
#endif
