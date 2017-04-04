#ifndef _COMMON_H
#define _COMMON_H

#define ENABLE_DEBUG_TRANSLATIONS
#define ENABLE_DEBUG_TRACKING
#define ENABLE_DEBUG_BRANCHING


#ifdef ENABLE_DEBUG_TRANSLATIONS
#define TRANSLATE_PRINT revtracerImports.dbgPrintFunc
#define TRANSLATE_PRINT_INSTRUCTION RiverPrintInstruction
#else
#define TRANSLATE_PRINT
#define TRANSLATE_PRINT_INSTRUCTION
#endif

#ifdef ENABLE_DEBUG_TRACKING
#define TRACKING_PRINT revtracerImports.dbgPrintFunc
#define LIB_TRACKING_PRINT exec->DebugPrintf
#else
#define TRACKING_PRINT
#define LIB_TRACKING_PRINT
#endif

#ifdef ENABLE_DEBUG_BRANCHING
#define BRANCHING_PRINT revtracerImports.dbgPrintFunc
#else
#define BRANCHING_PRINT
#endif

#ifdef _MSC_VER
#define DEBUG_BREAK __asm \
{ __asm int 3 }
#else
#define DEBUG_BREAK asm volatile("int $0x3")
typedef unsigned int size_t;
#endif
#endif
