#ifndef _REVTRACER_WRAPPER_H
#define _REVTRACER_WRAPPER_H

typedef void *ADDR_TYPE;

namespace revwrapper {
	#if defined _WIN32 || defined __CYGWIN__
		#ifdef _BUILDING_REVTRACER_WRAPPER_DLL
			#ifdef __GNUC__
				#define DLL_REVTRACER_WRAPPER_PUBLIC __attribute__ ((dllexport))
			#else
				#define DLL_REVTRACER_WRAPPER_PUBLIC __declspec(dllexport)
			#endif
		#else
			#ifdef __GNUC__
				#define DLL_REVTRACER_WRAPPER_PUBLIC __attribute__ ((dllimport))
			#else
				#define DLL_REVTRACER_WRAPPER_PUBLIC __declspec(dllimport)
			#endif
		#endif
		#define DLL_LOCAL
	#else
		#if __GNUC__ >= 4
			#define DLL_REVTRACER_WRAPPER_PUBLIC __attribute__ ((visibility ("default")))
			#define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
		#else
			#define DLL_REVTRACER_WRAPPER_PUBLIC
			#define DLL_LOCAL
		#endif
	#endif

	extern "C" {
		/** Initializes the API-wrapper */
		DLL_REVTRACER_WRAPPER_PUBLIC extern int InitRevtracerWrapper();

		/** Allocates virtual memory */
		DLL_REVTRACER_WRAPPER_PUBLIC extern void *CallAllocateMemoryHandler(unsigned long);

		/** Frees virtual memory */
		DLL_REVTRACER_WRAPPER_PUBLIC extern void CallFreeMemoryHandler(void*);

		/** Map memory at specified address */
		DLL_PUBLIC extern void *CallMapMemoryHandler(
				unsigned long mapHandler,
				unsigned long access,
				unsigned long offset,
				unsigned long size,
				void *address);

		/** Terminates the current process */
		DLL_REVTRACER_WRAPPER_PUBLIC extern void CallTerminateProcessHandler(void);

		/** Gets the address of the last executed basic block (before the process is terminated) */
		DLL_REVTRACER_WRAPPER_PUBLIC extern void *CallGetTerminationCodeHandler(void);

		/** Formatted print utility function */
		DLL_REVTRACER_WRAPPER_PUBLIC extern int CallFormattedPrintHandler(
			char *buffer,
			unsigned int sizeOfBuffer,
			const char *format,
			char *argptr);

		/** Write to file utility function (used only for basic block logging) */
		DLL_REVTRACER_WRAPPER_PUBLIC extern bool CallWriteFile(
			void *handle,
			void *buffer,
			unsigned int size,
			unsigned long *written);

		/** Yield the CPU */
		DLL_PUBLIC extern long CallYieldExecution(void);

		/** Init semaphore */
		DLL_PUBLIC extern int CallInitSemaphore(void *semaphore, int shared, int value);

		/** Wait for semaphore */
		DLL_PUBLIC extern int CallWaitSemaphore(void *semaphore);

		/** Post semaphore*/
		DLL_PUBLIC extern int CallPostSemaphore(void *semaphore);

		/** Destroy semaphore */
		DLL_PUBLIC extern int CallDestroySemaphore(void *semaphore);

		/** Get value semaphore */
		DLL_PUBLIC extern int CallGetValueSemaphore(void *semaphore, int *ret);

		/** Flush contents of instruction cache */
		DLL_PUBLIC extern void CallFlushInstructionCache(void);
	}
}; //namespace revwrapper

#endif
