#ifndef _SYMBOP_TRANSLATOR_H
#define _SYMBOP_TRANSLATOR_H

#include "revtracer.h"
#include "river.h"

/* SYMBOP ABI */
/* - if the instruction uses any flag as input or output no inline code is generated 
   - a symboppushf tracking instruction is generated
*/

/* - if the instruction uses a register as input or output no inline code is generated
   - a symboppushreg <regname> tracking instruction is generated
*/

/* - if the instruction uses a memory location as input or output the following instructions are generated:
		- push imm8/16/32	- push the offset on the stack
		- lea reg, mem		- in order to determine the effective address resolved
		- push reg			- push the reg on the stack
		- push modrm		- push a dword containting scale, index, base
   - a symboppushmem tracking instruction is generated
*/
/* - each instruction generates an inline push opcode instruction
   - each instruction generates a symboptrack instruction
*/

class RiverCodeGen;

class SymbopTranslator {
private :
	nodep::BYTE trackedValues;

	RiverCodeGen *codegen;

	nodep::DWORD GetMemRepr(const RiverAddress &mem);

	typedef void(SymbopTranslator::*TranslateOpcodeFunc)(const RiverInstruction &rIn, RiverInstruction *&rMainOut, nodep::DWORD &instrCount, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount, nodep::DWORD dwTranslationFlags);
	static TranslateOpcodeFunc translateOpcodes[2][0x100];
public :
	bool Init(RiverCodeGen *cg);

	bool Translate(const RiverInstruction &rIn, RiverInstruction *rMainOut, nodep::DWORD &instrCount, RiverInstruction *rTrackOut, nodep::DWORD &trackCount, nodep::DWORD dwTranslationFlags);

private :
	/* Translation helpers */
	void MakeInitTrack(const RiverInstruction &rIn, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount);
	void MakeCleanTrack(RiverInstruction *&rTrackOut, nodep::DWORD &trackCount);

	nodep::DWORD MakeTrackFlg(nodep::BYTE flags, RiverInstruction *&rMainOut, nodep::DWORD &instrCount, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount);

	nodep::DWORD MakePreTrackReg(const RiverRegister &reg, RiverInstruction *&rMainOut, nodep::DWORD &instrCount);
	void MakeTrackReg(const RiverRegister &reg, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount);
	
	//nodep::DWORD MakePreTrackMem(const RiverAddress &mem, nodep::WORD specifiers, nodep::DWORD addrOffset, RiverInstruction *&rMainOut, nodep::DWORD &instrCount);
	void MakeTrackMem(const RiverAddress &mem, nodep::WORD specifiers, nodep::DWORD addrOffset, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount);
	
	nodep::DWORD MakeTrackAddress(nodep::WORD specifiers, const RiverOperand &op, nodep::BYTE optype, RiverInstruction *&rMainOut, nodep::DWORD &instrCount, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount, nodep::DWORD &valuesOut);

	void MakeMarkFlg(nodep::BYTE flags, nodep::DWORD offset, RiverInstruction *&rMainOut, nodep::DWORD &instrCount, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount);
	void MakeMarkReg(const RiverRegister &reg, nodep::DWORD addrOffset, nodep::DWORD valueOffset, RiverInstruction *&rMainOut, nodep::DWORD &instrCount, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount);
	void MakeMarkMem(const RiverAddress &mem, nodep::WORD specifiers, nodep::DWORD addrOffset, nodep::DWORD valueOffset, RiverInstruction *&rMainOut, nodep::DWORD &instrCount, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount);
	//void MakeSkipMem(const RiverAddress &mem, RiverInstruction *&rMainOut, nodep::DWORD &instrCount, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount);
	
	/*  */
	void MakeTrackOp(nodep::DWORD opIdx, const nodep::BYTE type, const RiverOperand &op, nodep::WORD specifiers, nodep::DWORD addrOffset, RiverInstruction *&rMainOut, nodep::DWORD &instrCount, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount, nodep::DWORD &valueOffset);
	void MakeMarkOp(const nodep::BYTE type, nodep::WORD specifiers, nodep::DWORD addrOffset, nodep::DWORD valueOffset, const RiverOperand &op, RiverInstruction *&rMainOut, nodep::DWORD &instrCount, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount);

	void MakeCallSymbolic(const RiverInstruction &rIn, RiverInstruction *&rMainOut, nodep::DWORD &instrCount, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount);

	/* Translators */
	void TranslateUnk(const RiverInstruction &rIn, RiverInstruction *&rMainOut, nodep::DWORD &instrCount, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount, nodep::DWORD dwTranslationFlags);
	void TranslateDefault(const RiverInstruction &rIn, RiverInstruction *&rMainOut, nodep::DWORD &instrCount, RiverInstruction *&rTrackOut, nodep::DWORD &trackCount, nodep::DWORD dwTranslationFlags);
};


#endif
