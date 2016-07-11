#ifndef _COMMON_H
#define _COMMON_H

#ifdef ENABLE_DEBUG_TRANSLATIONS
#define TRANSLATE_PRINT revtracerAPI.dbgPrint
#define TRANSLATE_PRINT_INSTRUCTION RiverPrintInstruction
#else
#define TRANSLATE_PRINT
#define TRANSLATE_PRINT_INSTRUCTION
#endif

#ifdef ENABLE_DEBUG_TRACKING
#define TRACKING_PRINT revtracerAPI.dbgPrint
#else
#define TRACKING_PRINT
#endif

#endif
