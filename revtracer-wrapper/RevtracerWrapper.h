#ifndef _REVTRACER_WRAPPER_H
#define _REVTRACER_WRAPPER_H

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

	union LibraryLayout {
		struct LinuxLayout {
			unsigned int libcBase;
			unsigned int librtBase;
			unsigned int libpthreadBase;
		} linLib;
		struct WindowsLayout {
			unsigned int ntdllBase;
		} winLib;
	};

	struct ImportedApi {
		LibraryLayout *libraries;

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
				} libc;

				struct {
					unsigned int _shm_open;
					unsigned int _shm_unlink;
				} librt;

				struct {
					unsigned int _yieldExecution;
					unsigned int _sem_init;
					unsigned int _sem_wait;
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

	extern "C" {
		//DLL_WRAPPER_PUBLIC extern LibraryLayout temporaryLayout;
		DLL_WRAPPER_PUBLIC extern ImportedApi wrapperImports;

		/** Initializes the API-wrapper */
		DLL_WRAPPER_PUBLIC extern int InitRevtracerWrapper(void *configPage);

		/** Allocates virtual memory */
		DLL_WRAPPER_PUBLIC extern void *CallAllocateMemoryHandler(unsigned long);

		/** Frees virtual memory */
		DLL_WRAPPER_PUBLIC extern void CallFreeMemoryHandler(void*);

		/** Map memory at specified address */
		DLL_WRAPPER_PUBLIC extern void *CallMapMemoryHandler(
				unsigned long mapHandler,
				unsigned long access,
				unsigned long offset,
				unsigned long size,
				void *address);

		/** Terminates the current process */
		DLL_WRAPPER_PUBLIC extern void CallTerminateProcessHandler(void);

		/** Gets the address of the last executed basic block (before the process is terminated) */
		DLL_WRAPPER_PUBLIC extern void *CallGetTerminationCodeHandler(void);

		/** Formatted print utility function */
		DLL_WRAPPER_PUBLIC extern int CallFormattedPrintHandler(
			char *buffer,
			unsigned int sizeOfBuffer,
			const char *format,
			char *argptr);

		/** Write to file utility function (used only for basic block logging) */
		DLL_WRAPPER_PUBLIC extern bool CallWriteFile(
			void *handle,
			void *buffer,
			unsigned int size,
			unsigned long *written);

		/** Yield the CPU */
		DLL_WRAPPER_PUBLIC extern long CallYieldExecution(void);

		/** Init semaphore */
		DLL_WRAPPER_PUBLIC extern int CallInitSemaphore(void *semaphore, int shared, int value);

		/** Wait for semaphore */
		DLL_PUBLIC extern int CallWaitSemaphore(void *semaphore, bool blocking);

		/** Post semaphore*/
		DLL_WRAPPER_PUBLIC extern int CallPostSemaphore(void *semaphore);

		/** Destroy semaphore */
		DLL_WRAPPER_PUBLIC extern int CallDestroySemaphore(void *semaphore);

		/** Get value semaphore */
		DLL_WRAPPER_PUBLIC extern int CallGetValueSemaphore(void *semaphore, int *ret);

		/** Open shared mem */
		DLL_WRAPPER_PUBLIC extern int CallOpenSharedMemory(const char *name, int oflag, int mode);

		/** Unlink shared mem */
		DLL_WRAPPER_PUBLIC extern int CallUnlinkSharedMemory(const char *name);

		/** Flush contents of instruction cache */
		DLL_WRAPPER_PUBLIC extern void CallFlushInstructionCache(void);
	}
}; //namespace revwrapper

#endif
