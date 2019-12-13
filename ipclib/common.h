#ifndef _COMMON_IPC_H
#define _COMMON_IPC_H

#include "../revtracer/common.h"


#if defined _WIN32 || defined __CYGWIN__
	#ifdef _BUILDING_IPC_DLL
		#ifdef __GNUC__
			#define DLL_IPC_PUBLIC __attribute__ ((dllexport))
		#else
			#define DLL_IPC_PUBLIC __declspec(dllexport)
		#endif
	#else
		#ifdef __GNUC__
			#define DLL_IPC_PUBLIC __attribute__ ((dllimport))
		#else
			#define DLL_IPC_PUBLIC __declspec(dllimport)
		#endif
	#endif
	#define DLL_IPC_LOCAL
#else
	#if __GNUC__ >= 4
		#define DLL_IPC_PUBLIC __attribute__ ((visibility ("default")))
		#define DLL_IPC_LOCAL  __attribute__ ((visibility ("hidden")))
	#else
		#define DLL_IPC_PUBLIC
		#define DLL_IPC_LOCAL
	#endif
#endif


typedef void *ADDR_TYPE;

namespace ipc {
	typedef int (*InitSemaphoreHandler)(void *, int, int);
	typedef int (*WaitSemaphoreFunc)(void *, bool);
	typedef int (*PostSemaphoreFunc)(void *);
	typedef int (*DestroySemaphoreHandler)(void *);
	typedef int (*GetvalueSemaphoreHandler)(void *, int*);

	/*struct IpcAPI {
		ADDR_TYPE ntYieldExecution;
		ADDR_TYPE initSemaphore;
		ADDR_TYPE waitSemaphore;
		ADDR_TYPE postSemaphore;
		ADDR_TYPE destroySemaphore;
		ADDR_TYPE getvalueSemaphore;
		ADDR_TYPE vsnprintf_s;

		ADDR_TYPE ldrMapMemory;
	};*/
} //namespace ipc

#endif
