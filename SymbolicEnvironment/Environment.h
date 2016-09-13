#ifndef _ENVIRONMENT_H_
#define _ENVIRONMENT_H_

#if defined _WIN32 || defined __CYGWIN__
	#ifdef _BUILDING_ENVIRONMENT_DLL
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

#include "SymbolicEnvironment.h"

/** Get a symbolic environment that can be used directly with the revtracer.
 * Features:
 *     - Lazy CPU flag evaluation
 *     - Only 32-bit CPU registers support
 *     - Constructs symbolic expressions on the fly for unaligned memory accesses
 */
DLL_PUBLIC sym::SymbolicEnvironment *NewX86RevtracerEnvironment(void *revEnv, void *ctl);

/** Get a symbolic environment that lazily constructs x86 register expressions 
 * Features:
 *     - Delegates flag evaluation to the underlying environment
 *     - Lazy evalauation of register symbolic values
 *     - Delegates memory evaluation to underlying environment
 */
DLL_PUBLIC sym::SymbolicEnvironment *NewX86RegistersEnvironment(sym::SymbolicEnvironment *parent);


#endif

