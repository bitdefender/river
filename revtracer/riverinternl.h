#ifndef _RIVER_INTERNAL_H
#define _RIVER_INTERNAL_H

#include "river.h"
#include "execenv.h"

#ifdef __cplusplus
#define RIVER_EXT extern "C"
#else
#define RIVER_EXT
#endif

typedef void(*TranslateOpcodeFunc)(struct _exec_env *pEnv, struct RiverInstruction *ri, BYTE **px86, DWORD *pFlags);
typedef void(*TranslateOperandsFunc)(struct _exec_env *pEnv, struct RiverInstruction *ri, BYTE **px86);

typedef void(*ConvertInstructionFunc)(struct _exec_env *pEnv, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount);


RIVER_EXT const TranslateOpcodeFunc TranslateOpcodeTable00[];
RIVER_EXT const TranslateOpcodeFunc TranslateOpcodeTable0F[];

RIVER_EXT const TranslateOperandsFunc TranslateOperandsTable00[];
RIVER_EXT const TranslateOperandsFunc TranslateOperandsTable0F[];

#endif
