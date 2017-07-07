#ifndef __SYM_DEMO_UTIL__
#define __SYM_DEMO_UTIL__


#ifdef _MSC_VER
#define DEBUG_BREAK __asm \
{ __asm int 3 }
#else
#define DEBUG_BREAK asm volatile("int $0x3")
#endif

#if defined _WIN32 || defined __CYGWIN__
	#ifdef _BUILDING_IPC_DLL
		#ifdef __GNUC__
			#define DLL_SYM_DEMO_PUBLIC __attribute__ ((dllexport))
		#else
			#define DLL_SYM_DEMO_PUBLIC __declspec(dllexport)
		#endif
	#else
		#ifdef __GNUC__
			#define DLL_SYM_DEMO_PUBLIC __attribute__ ((dllimport))
		#else
			#define DLL_SYM_DEMO_PUBLIC __declspec(dllimport)
		#endif
	#endif
	#define DLL_SYM_DEMO_LOCAL
#else
	#if __GNUC__ >= 4
		#define DLL_SYM_DEMO_PUBLIC __attribute__ ((visibility ("default")))
		#define DLL_SYM_DEMO_LOCAL  __attribute__ ((visibility ("hidden")))
	#else
		#define DLL_SYM_DEMO_PUBLIC
		#define DLL_SYM_DEMO_LOCAL
	#endif
#endif

typedef char *va_list;
#define _ADDRESSOF(v)   ( (unsigned int *)&v )
#define _INTSIZEOF(n)   ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )

#define va_start(ap,v)  ( ap = (va_list)_ADDRESSOF(v) + _INTSIZEOF(v) )
#define va_arg(ap,t)    ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define va_end(ap)      ( ap = (va_list)0 )

typedef int(*_vsnprintf_sFunc)(
		char *buffer,
		size_t count,
		const char *format,
		char *argptr
);

struct SymDemoImports {
	_vsnprintf_sFunc vsnprintf_sFunc;
};

// TODO - link this function
extern "C" {
	extern SymDemoImports symDemoImports;
}


#endif
