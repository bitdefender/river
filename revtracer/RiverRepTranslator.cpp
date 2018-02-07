#include "RiverRepTranslator.h"
#include "CodeGen.h"
#include "TranslatorUtil.h"

bool RiverRepTranslator::Init(RiverCodeGen *cg) {
	codegen = cg;
	return true;
}

void MakeDebugBreak(RiverInstruction *rOut, nodep::BYTE family) {
	rOut->opCode = 0xcc;
	rOut->family = family;
	rOut->modifiers = rOut->specifiers = 0;

	rOut->modFlags = rOut->testFlags = 0;
	rOut->instructionAddress = 0;
}

void MakeRepInitInstruction(RiverInstruction *rOut, nodep::BYTE family, nodep::DWORD modifiers, nodep::DWORD addr) {
	rOut->opCode = 0xF2;
	rOut->subOpCode = 0x0;
	rOut->family = family;
	rOut->modifiers = modifiers;
	rOut->specifiers = 0;

	rOut->modFlags = rOut->testFlags = 0;

	for (int i = 0; i < 4; ++i) {
		rOut->opTypes[i] = RIVER_OPTYPE_NONE;
	}

	rOut->instructionAddress = addr;
}

void MakeRepFiniInstruction(RiverInstruction *rOut, nodep::BYTE family, nodep::DWORD addr) {
	rOut->opCode = 0xF3;
	rOut->subOpCode = 0x0;
	rOut->family = family;
	rOut->modifiers = rOut->specifiers = 0;

	rOut->modFlags = rOut->testFlags = 0;

	for (int i = 0; i < 4; ++i) {
		rOut->opTypes[i] = RIVER_OPTYPE_NONE;
	}

	rOut->instructionAddress = addr;
}


void RiverRepTranslator::TranslateDefault(const RiverInstruction &rIn, RiverInstruction *rOut, nodep::DWORD &instrCount) {
	CopyInstruction(codegen, rOut[0], rIn);
	instrCount++;
}

/* translation layout
 *  repinit
 * code:
 *	//actual code//
 *	repfini
 *
 *	repinit translates to:
 *	 jmp code
 *	 loop init
 *	 jmp repfini
 *
 *	repfini translates to:
 *	 jmp loop
 */
void RiverRepTranslator::TranslateCommon(const RiverInstruction &rIn, RiverInstruction *rOut, nodep::DWORD &instrCount, nodep::DWORD riverModifier) {
	nodep::DWORD localInstrCount = 0;

	// add custom instruction to mark the beggining of a rep sequence
	MakeRepInitInstruction(&rOut[localInstrCount], RIVER_FAMILY_REP, rIn.modifiers, rIn.instructionAddress);
	localInstrCount++;

	// insert body (for the moment, the input instruction, other
	// translators will decorate it
	CopyInstruction(codegen, rOut[localInstrCount], rIn);
	// remove rep modifier
	rOut[localInstrCount].modifiers &= ~(RIVER_MODIFIER_REP | RIVER_MODIFIER_REPZ | RIVER_MODIFIER_REPNZ);
	localInstrCount++;

	//insert repfini
	MakeRepFiniInstruction(&rOut[localInstrCount], RIVER_FAMILY_REP, rIn.instructionAddress);
	localInstrCount++;

	instrCount += localInstrCount;
}

bool RiverRepTranslator::Translate(const RiverInstruction &rIn, RiverInstruction *rOut, nodep::DWORD &instrCount) {
	if (!(rIn.modifiers & RIVER_MODIFIER_REP ||
				rIn.modifiers & RIVER_MODIFIER_REPZ ||
				rIn.modifiers & RIVER_MODIFIER_REPNZ)) {
		TranslateDefault(rIn, rOut, instrCount);
		return true;
	}

	switch (rIn.opCode) {
		case 0x6C: case 0x6D: //INS 8/16/32
		case 0xA4: case 0xA5: //MOVS 8/16/32
		case 0x6E: case 0x6F: //OUTS 8/16/32
		case 0xAC: case 0xAD: //LODS 8/16/32
		case 0xAA: case 0xAB: //STOS 8/16/32
			RiverInstruction rInFixed;
			CopyInstruction(codegen, rInFixed, rIn);
			rInFixed.modifiers &= ~(RIVER_MODIFIER_REPZ);
			rInFixed.modifiers |= RIVER_MODIFIER_REP;
			// we need this workaround to fix the modifier
			TranslateCommon(rInFixed, rOut, instrCount, RIVER_MODIFIER_REP);
			return true;
		default:
			break;
	}

	if (rIn.modifiers & RIVER_MODIFIER_REPZ) {
		switch(rIn.opCode) {
			case 0xA6: case 0xA7: //CMPS 8/16/32
			case 0xAE: case 0xAF: //SCAS 8/16/32
				TranslateCommon(rIn, rOut, instrCount, RIVER_MODIFIER_REPZ);
				return true;
			default:
				TranslateDefault(rIn, rOut, instrCount);
				return true;
		}
	}

	if (rIn.modifiers & RIVER_MODIFIER_REPNZ) {
		switch(rIn.opCode) {
			case 0xA6: case 0xA7: //CMPS 8/16/32
			case 0xAE: case 0xAF: //SCAS 8/16/32
				TranslateCommon(rIn, rOut, instrCount, RIVER_MODIFIER_REPNZ);
				return true;
			default:
				TranslateDefault(rIn, rOut, instrCount);
				return true;
		}
	}
}

