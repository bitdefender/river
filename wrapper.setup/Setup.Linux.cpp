#ifdef __linux__

#include "../CommonCrossPlatform/Common.h"
#include "../revtracer-wrapper/RevtracerWrapper.h"

extern "C" bool InitWrapperOffsets(ext::LibraryLayout *libs, revwrapper::WrapperImports *api) {

	LIB_T hlibc = GET_LIB_HANDLER("libc.so");
	LIB_T hlibrt = GET_LIB_HANDLER("librt.so");
	LIB_T hlibpthread = GET_LIB_HANDLER("libpthread.so");

	DWORD baselibc = GET_LIB_BASE(hlibc);
	DWORD baselibrt = GET_LIB_BASE(hlibrt);
	DWORD baselibpthread = GET_LIB_BASE(hlibpthread);

	libs->linLib.libcBase = baselibc;
	api->functions.linFunc.libc._virtualAlloc = (DWORD)LOAD_PROC(hlibc, "mmap") - baselibc;
	api->functions.linFunc.libc._virtualFree = (DWORD)LOAD_PROC(hlibc, "munmap") - baselibc;
	api->functions.linFunc.libc._terminateProcess = (DWORD)LOAD_PROC(hlibc, "exit") - baselibc;
	api->functions.linFunc.libc._writeFile = (DWORD)LOAD_PROC(hlibc, "write") - baselibc;
	api->functions.linFunc.libc._formatPrint = (DWORD)LOAD_PROC(hlibc, "vsnprintf") - baselibc;
	api->functions.linFunc.libc._print = (DWORD)LOAD_PROC(hlibc, "printf") - baselibc;
	api->functions.linFunc.libc._clockGetTime = (DWORD)LOAD_PROC(hlibc, "clock_gettime") - baselibc;

	libs->linLib.librtBase = baselibrt;
	api->functions.linFunc.librt._shm_open = (DWORD)LOAD_PROC(hlibrt, "shm_open") - baselibrt;
	api->functions.linFunc.librt._shm_unlink = (DWORD)LOAD_PROC(hlibrt, "shm_unlink") - baselibrt;

	libs->linLib.libpthreadBase = baselibpthread;
	api->functions.linFunc.libpthread._yieldExecution = (DWORD)LOAD_PROC(hlibpthread, "pthread_yield") - baselibpthread;
	api->functions.linFunc.libpthread._sem_init = (DWORD)LOAD_PROC(hlibpthread, "sem_init") - baselibpthread;
	api->functions.linFunc.libpthread._sem_wait = (DWORD)LOAD_PROC(hlibpthread, "sem_wait") - baselibpthread;
	api->functions.linFunc.libpthread._sem_timedwait = (DWORD)LOAD_PROC(hlibpthread, "sem_timedwait") - baselibpthread;
	api->functions.linFunc.libpthread._sem_post = (DWORD)LOAD_PROC(hlibpthread, "sem_post") - baselibpthread;
	api->functions.linFunc.libpthread._sem_destroy = (DWORD)LOAD_PROC(hlibpthread, "sem_destroy") - baselibpthread;
	api->functions.linFunc.libpthread._sem_getvalue = (DWORD)LOAD_PROC(hlibpthread, "sem_getvalue") - baselibpthread;

	return true;
}

#endif