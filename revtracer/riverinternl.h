#ifndef _RIVER_INTERNAL_H
#define _RIVER_INTERNAL_H

#include "river.h"
#include "CodeGen.h"
#include "Runtime.h"

typedef void(*AssemblingOpcodeFunc)(RiverCodeGen *cg, RiverRuntime *rt, struct RiverInstruction *ri, BYTE **px86, DWORD *pFlags);
typedef void(*AssemblingOperandsFunc)(RiverCodeGen *cg, struct RiverInstruction *ri, BYTE **px86);

typedef void(*TranslateOpcodeFunc)(RiverCodeGen *cg, struct RiverInstruction *ri, BYTE **px86, DWORD *pFlags);
typedef void(*TranslateOperandsFunc)(RiverCodeGen *cg, struct RiverInstruction *ri, BYTE **px86);

typedef void(*ConvertInstructionFunc)(RiverCodeGen *cg, struct RiverInstruction *rIn, struct RiverInstruction *rOut, DWORD *outCount);


/*extern const TranslateOpcodeFunc TranslateOpcodeTable00[];
extern const TranslateOpcodeFunc TranslateOpcodeTable0F[];

extern const TranslateOperandsFunc TranslateOperandsTable00[];
extern const TranslateOperandsFunc TranslateOperandsTable0F[];*/

#endif
