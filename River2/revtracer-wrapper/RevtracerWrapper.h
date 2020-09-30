#ifndef _REVTRACER_WRAPPER_H
#define _REVTRACER_WRAPPER_H

#include "../CommonCrossPlatform/LibraryLayout.h"
#include "TokenRing.h"

namespace revwrapper {
	#if defined _WIN32 || defined __CYGWIN__
		#ifdef _BUILDING_REVTRACER_WRAPPER_DLL
			#ifdef __GNUC__
				#define DLL_WRAPPER_PUBLIC __attribute__ ((dllexport))
			#else
				#define DLL_WRAPPER_PUBLIC __declspec(dllexport)
			#endif
		#else
			#ifdef __GNUC__
				#define DLL_WRAPPER_PUBLIC __attribute__ ((dllimport))
			#else
				#define DLL_WRAPPER_PUBLIC __declspec(dllimport)
			#endif
		#endif
		#define DLL_WRAPPER_LOCAL
	#else
		#if __GNUC__ >= 4
			#define DLL_WRAPPER_PUBLIC __attribute__ ((visibility ("default")))
			#define DLL_WRAPPER_LOCAL  __attribute__ ((visibility ("hidden")))
		#else
			#define DLL_WRAPPER_PUBLIC
			#define DLL_WRAPPER_LOCAL
		#endif
	#endif

	#define WAIT_INFINITE 0x3FFFFFFF

	struct WrapperImports {
		ext::LibraryLayout *libraries;

		union {
			struct {
				struct {
					unsigned int _virtualAlloc;
					unsigned int _virtualFree;
					unsigned int _terminateProcess;
					unsigned int _writeFile;
					unsigned int _waitForSingleObject;
					unsigned int _formatPrint;
					unsigned int _print;
					unsigned int _clockGetTime;
				} libc;

				struct {
					unsigned int _shm_open;
					unsigned int _shm_unlink;
				} librt;

				struct {
					unsigned int _yieldExecution;
					unsigned int _sem_init;
					unsigned int _sem_wait;
					unsigned int _sem_timedwait;
					unsigned int _sem_post;
					unsigned int _sem_destroy;
					unsigned int _sem_getvalue;
				} libpthread;
			} linFunc;

			struct {
				struct {
					unsigned int _virtualAlloc;
					unsigned int _virtualFree;
					unsigned int _mapMemory;

					unsigned int _flushMemoryCache;

					unsigned int _terminateProcess;

					unsigned int _writeFile;
					unsigned int _waitForSingleObject;

					unsigned int _systemError;

					unsigned int _formatPrint;

					unsigned int _ntYieldExecution;
					unsigned int _flushInstructionCache;

					unsigned int _createEvent;
					unsigned int _setEvent;
				} ntdll;
			} winFunc;
		} functions;
	};

	/** Initializes the API-wrapper */
	typedef bool(*InitRevtracerWrapperFunc)(void *configPage);

	/** Allocates virtual memory */
	typedef void *(*AllocateMemoryFunc)(unsigned long);

	/** Frees virtual memory */
	typedef void (*FreeMemoryFunc)(void*);

	/** Terminates the current process */
	typedef void (*TerminateProcessFunc)(int);

	/** Gets the address of the last executed basic block (before the process is terminated) */
	typedef void *(*GetTerminationCodeFunc)(void);

	/** Formatted print utility function */
	typedef int (*FormattedPrintFunc)(
		char *buffer,
		unsigned int sizeOfBuffer,
		const char *format,
		char *argptr
	);

	/** Write to file utility function (used only for basic block logging) */
	typedef bool (*WriteFileFunc)(
		void *handle,
		void *buffer,
		unsigned int size,
		unsigned long *written
	);

	/** Init semaphore */
	typedef bool (*InitEventFunc)(
		void *handle,
		bool isSet
	);

	/** Wait for semaphore, set timeout to 0 for trywait, or WAIT_INFINITE for a permanent wait */
	typedef bool (*WaitEventFunc)(
		void *handle,
		int timeout
	);

	/** Post semaphore */
	typedef bool (*PostEventFunc)(
		void *handle
	);

	/** Destroy semaphore */
	typedef void (*DestroyEventFunc)(
		void *handle
	);

	/** Get value semaphore */
	typedef void (*GetValueEventFunc)(
		void *handle,
		int *ret
	);

	/** Open shared mem */
	typedef int (*OpenSharedMemoryFunc)(
		const char *name,
		int oflag,
		int mode
	);

	/** Unlink shared mem */
	typedef int (*UnlinkSharedMemoryFunc)(
		const char *name
	);

	/** Flush contents of instruction cache */
	typedef void (*FlushInstructionCacheFunc)(void);

	struct WrapperExports {
		InitRevtracerWrapperFunc initRevtracerWrapper;
		AllocateMemoryFunc allocateMemory;
		FreeMemoryFunc freeMemory;

		TerminateProcessFunc terminateProcess;
		GetTerminationCodeFunc getTerminationCode;

		FormattedPrintFunc formattedPrint;

		WriteFileFunc writeFile;

		InitEventFunc initEvent;
		WaitEventFunc waitEvent;
		PostEventFunc postEvent;
		DestroyEventFunc destroyEvent;
		GetValueEventFunc getValueEvent;
		OpenSharedMemoryFunc openSharedMemory;
		UnlinkSharedMemoryFunc unlinkSharedMemory;

		TokenRing *tokenRing;
	};

	extern "C" {
		DLL_WRAPPER_PUBLIC extern WrapperImports wrapperImports;
		DLL_WRAPPER_PUBLIC extern WrapperExports wrapperExports;
	};
}; //namespace revwrapper

#endif
