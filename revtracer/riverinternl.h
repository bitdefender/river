#ifndef _RIVER_INTERNAL_H
#define _RIVER_INTERNAL_H

#include "river.h"
#include "execenv.h"

typedef void(*TranslateOpcodeFunc)(struct _exec_env *pEnv, struct RiverInstruction *ri, BYTE **px86, DWORD *pFlags);
typedef void(*TranslateOperandsFunc)(struct _exec_env *pEnv, struct RiverInstruction *ri, BYTE **px86);

typedef void(*ConvertInstructionFunc)(struct _exec_env *pEnv, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount);


extern const TranslateOpcodeFunc TranslateOpcodeTable00[];
extern const TranslateOpcodeFunc TranslateOpcodeTable0F[];

extern const TranslateOperandsFunc TranslateOperandsTable00[];
extern const TranslateOperandsFunc TranslateOperandsTable0F[];

#endif
