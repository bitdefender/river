#ifndef _COMMON_SPECIFIERS_H_
#define _COMMON_SPECIFIERS_H_

#if defined _WIN32 || defined __CYGWIN__
	#ifdef _IMPORT_THIS_API_
		#ifdef _MSC_VER
			#define DLL_PUBLIC __declspec(dllimport)
		#else
			#define DLL_PUBLIC __attribute__ ((dllimport))
		#endif
	#elif defined _EXPORT_THIS_API_
		#ifdef _MSC_VER
			#define DLL_PUBLIC __declspec(dllexport)
		#else
			#define DLL_PUBLIC __attribute__ ((dllexport))
		#endif
	#else
		#ifdef _MSC_VER
			#define DLL_PUBLIC
		#else
			#define DLL_PUBLIC
		#endif
	#endif

	#ifdef _MSC_VER
		#define NAKED  __declspec(naked)
	#else
		#define NAKED
	#endif

	#define DLL_LOCAL
#else
	#if __GNUC__ >= 4
		#define DLL_PUBLIC __attribute__ ((visibility ("default")))
		#define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
	    #define NAKED  __attribute__ ((naked))
	#else
		#define DLL_PUBLIC
		#define DLL_LOCAL
	#endif
#endif

#endif
