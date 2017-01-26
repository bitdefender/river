#ifdef __linux__

#include "../CommonCrossPlatform/Common.h"
#include "../revtracer-wrapper/RevtracerWrapper.h"

extern "C" bool InitFunctionOffsets(revwrapper::LibraryLayout *libs, revwrapper::ImportedApi *api) {

	LIB_T hlibc = GET_LIB_HANDLER("libc.so");
	LIB_T hlibrt = GET_LIB_HANDLER("librt.so");
	LIB_T hlibpthread = GET_LIB_HANDLER("libpthread.so");

	DWORD baselibc = GET_LIB_BASE(hlibc);
	DWORD baselibrt = GET_LIB_BASE(hlibrt);
	DWORD baselibpthread = GET_LIB_BASE(hlibpthread);

	libs->linux.libcBase = baselibc;
	api->functions.linux.libc._virtualAlloc = (unsigned int)LOAD_PROC(hlibc, "mmap") - baselibc;
	api->functions.linux.libc._virtualFree = (unsigned int)LOAD_PROC(hlibc, "munmap") - baselibc;
	api->functions.linux.libc._terminateProcess = (unsigned int)LOAD_PROC(hlibc, "exit") - baselibc;
	api->functions.linux.libc._writeFile = (unsigned int)LOAD_PROC(hlibc, "write") - baselibc;
	api->functions.linux.libc._formatPrint = (unsigned int)LOAD_PROC(hlibc, "vsnprintf") - baselibc;
	api->functions.linux.libc._print = (unsigned int)LOAD_PROC(hlibc, "printf") - baselibc;

	libs->linux.librtBase = baselibrt;
	api->functions.linux.librt._shm_open = (unsigned int)LOAD_PROC(hlibrt, "shm_open") - baselibrt;
	api->functions.linux.librt._shm_unlink = (unsigned int)LOAD_PROC(hlibrt, "shm_unlink") - baselibrt;

	libs->linux.libpthreadBase = baselibpthread;
	api->functions.linux.libpthread._yieldExecution = (unsigned int)LOAD_PROC(hlibpthread, "pthread_yield") - baselibpthread;
	api->functions.linux.libpthread._sem_init = (unsigned int)LOAD_PROC(hlibpthread, "sem_init") - baselibpthread;
	api->functions.linux.libpthread._sem_wait = (unsigned int)LOAD_PROC(hlibpthread, "sem_wait") - baselibpthread;
	api->functions.linux.libpthread._sem_post = (unsigned int)LOAD_PROC(hlibpthread, "sem_post") - baselibpthread;
	api->functions.linux.libpthread._sem_destroy = (unsigned int)LOAD_PROC(hlibpthread, "sem_destroy") - baselibpthread;
	api->functions.linux.libpthread._sem_getvalue = (unsigned int)LOAD_PROC(hlibpthread, "sem_getvalue") - baselibpthread;

	return true;
}

#endif