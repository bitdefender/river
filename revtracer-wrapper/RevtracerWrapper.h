#ifndef _REVTRACER_WRAPPER_H
#define _REVTRACER_WRAPPER_H

#ifdef __linux__
#include <stdarg.h>
#include <stdlib.h>
#include <dlfcn.h>
typedef void* lib_t;
#else
typedef HMODULE lib_t;
#endif
typedef void *ADDR_TYPE;

namespace revwrapper {
	#if defined _WIN32 || defined __CYGWIN__
		#ifdef _BUILDING_REVTRACER_WRAPPER_DLL
			#ifdef __GNUC__
				#define DLL_PUBLIC __attribute__ ((dllexport))
			#else
				#define DLL_PUBLIC __declspec(dllexport)
			#endif
		#else
			#ifdef __GNUC__
				#define DLL_PUBLIC __attribute__ ((dllimport))
			#else
				#define DLL_PUBLIC __declspec(dllimport)
			#endif
		#endif
		#define DLL_LOCAL
	#else
		#if __GNUC__ >= 4
			#define DLL_PUBLIC __attribute__ ((visibility ("default")))
			#define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
		#else
			#define DLL_PUBLIC
			#define DLL_LOCAL
		#endif
	#endif

	DLL_LOCAL lib_t libhandler;
	DLL_LOCAL long SetLastError(long);
	DLL_LOCAL ADDR_TYPE GetAllocateMemoryHandler(void);
	DLL_LOCAL ADDR_TYPE GetConvertToSystemErrorHandler(void);
	DLL_LOCAL ADDR_TYPE GetTerminateProcessHandler(void);
	DLL_LOCAL ADDR_TYPE GetFreeMemoryHandler(void);
	DLL_LOCAL ADDR_TYPE GetFormattedPrintHandler(void);
	DLL_LOCAL ADDR_TYPE GetWriteFileHandler(void);

#ifdef _LINUX
		DLL_LOCAL long ConvertToSystemError(long);
#endif

	extern "C" {
		DLL_PUBLIC long revtracerLastError;
		DLL_PUBLIC extern ADDR_TYPE RevtracerWrapperInit(void);
		DLL_PUBLIC extern void *CallAllocateMemoryHandler(unsigned long);
		DLL_PUBLIC extern void CallFreeMemoryHandler(void*);
		DLL_PUBLIC extern void CallTerminateProcessHandler(void);
		DLL_PUBLIC extern int CallFormattedPrintHandler(
			char *buffer,
			size_t sizeOfBuffer,
			size_t count,
			const char *format,
			va_list argptr);
		DLL_PUBLIC extern bool CallWriteFile(
			void *handle,
			int fd,
			void *buffer,
			size_t size,
			unsigned long *written);
	}
}; //namespace revwrapper

#endif
