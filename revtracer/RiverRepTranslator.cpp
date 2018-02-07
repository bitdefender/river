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

void MakeRepInitInstruction(RiverInstruction *rOut, nodep::BYTE family, nodep::DWORD addr) {
	rOut->opCode = 0xF2;
	rOut->subOpCode = 0x0;
	rOut->family = family;
	rOut->modifiers = rOut->specifiers = 0;

	rOut->modFlags = rOut->testFlags = 0;
	rOut->instructionAddress = addr;
}

void MakeRepFiniInstruction(RiverInstruction *rOut, nodep::BYTE family, nodep::DWORD addr) {
	rOut->opCode = 0xF3;
	rOut->subOpCode = 0x0;
	rOut->family = family;
	rOut->modifiers = rOut->specifiers = 0;

	rOut->modFlags = rOut->testFlags = 0;
	rOut->instructionAddress = addr;
}

void MakeJmpInstruction(RiverInstruction *rOut, nodep::BYTE family, nodep::DWORD offset, nodep::DWORD addr) {
	rOut->opCode = 0xE9; //jmp rel32
	rOut->subOpCode = 0x0;
	rOut->family = family;
	rOut->modifiers = rOut->specifiers = 0;

	rOut->modFlags = rOut->testFlags = 0;

	rOut->opTypes[0] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_32;
	rOut->operands[0].asImm32 = offset;

	rOut->instructionAddress = addr;
}

void MakeLoopInstruction(RiverInstruction *rOut, nodep::BYTE family, nodep::DWORD riverModifier, nodep::BYTE offset, nodep::DWORD addr) {
	switch(riverModifier) {
		case RIVER_MODIFIER_REP:
			rOut->opCode = 0xE2;
			rOut->testFlags = 0x0;
			break;
		case RIVER_MODIFIER_REPZ:
			rOut->opCode = 0xE1;
			rOut->testFlags = RIVER_SPEC_FLAG_ZF;
			break;
		case RIVER_MODIFIER_REPNZ:
			rOut->opCode = 0xE0;
			rOut->testFlags = RIVER_SPEC_FLAG_ZF;
			break;
		default:
			DEBUG_BREAK;
	}
	rOut->subOpCode = 0x0;
	rOut->family = family;
	rOut->modifiers = rOut->specifiers = 0;

	rOut->modFlags = 0;

	rOut->opTypes[0] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_8;
	rOut->operands[0].asImm8 = offset;

	rOut->instructionAddress = addr;
}


void RiverRepTranslator::TranslateDefault(const RiverInstruction &rIn, RiverInstruction *rOut, nodep::DWORD &instrCount) {
	CopyInstruction(codegen, rOut[0], rIn);
	instrCount++;
}

/* translation layout
 *    init:
 *      jmp code
 *    loop:
 *      loop init
 *     |repinit <=> jmp repfini
 *     | code:
 *	   |   //actual code//
 *	   |-->repfini <=> jmp loop
 */
void RiverRepTranslator::TranslateCommon(const RiverInstruction &rIn, RiverInstruction *rOut, nodep::DWORD &instrCount, nodep::DWORD riverModifier) {
	nodep::DWORD localInstrCount = 0;
	//translate rep

	// add jump instruction to jump to actual code
	nodep::DWORD jmpCodeOffset =  2 /*loopcc imm8*/ + 5 /*jmp repfini*/;
	MakeJmpInstruction(&rOut[localInstrCount], RIVER_FAMILY_REP, jmpCodeOffset, rIn.instructionAddress);
	localInstrCount++;

	// add loop instruction to repinit
	nodep::DWORD loopInitOffset = -1 * (5 /*jmp imm32*/ + 2 /*loop imm8*/);
	MakeLoopInstruction(&rOut[localInstrCount], RIVER_FAMILY_REP, riverModifier, loopInitOffset, rIn.instructionAddress);
	localInstrCount++;

	// add custom instruction to mark the beggining of a rep sequence
	MakeRepInitInstruction(&rOut[localInstrCount], RIVER_FAMILY_REP, rIn.instructionAddress);
	localInstrCount++;

	// insert body (for the moment, the input instruction, other
	// translators will decorate it
	CopyInstruction(codegen, rOut[localInstrCount], rIn);
	// remove rep modifier
	rOut[localInstrCount].modifiers &= ~(RIVER_MODIFIER_REP | RIVER_MODIFIER_REPZ | RIVER_MODIFIER_REPNZ);
	localInstrCount++;

	//insert repfini marker
	MakeRepFiniInstruction(&rOut[localInstrCount], RIVER_FAMILY_REP, rIn.instructionAddress);
	localInstrCount++;

	//MakeDebugBreak(&rOut[localInstrCount], RIVER_FAMILY_REP);
	//localInstrCount++;

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

