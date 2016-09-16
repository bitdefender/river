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
	rev::BYTE trackedValues;

	RiverCodeGen *codegen;

	void CopyInstruction(RiverInstruction &rOut, const RiverInstruction &rIn);

	rev::DWORD GetMemRepr(const RiverAddress &mem);

	typedef void(SymbopTranslator::*TranslateOpcodeFunc)(const RiverInstruction &rIn, RiverInstruction *&rMainOut, rev::DWORD &instrCount, RiverInstruction *&rTrackOut, rev::DWORD &trackCount, rev::DWORD dwTranslationFlags);
	static TranslateOpcodeFunc translateOpcodes[2][0x100];
public :
	bool Init(RiverCodeGen *cg);

	bool Translate(const RiverInstruction &rIn, RiverInstruction *rMainOut, rev::DWORD &instrCount, RiverInstruction *rTrackOut, rev::DWORD &trackCount, rev::DWORD dwTranslationFlags);

private :
	/* Translation helpers */
	void MakeInitTrack(RiverInstruction *&rTrackOut, rev::DWORD &trackCount, rev::DWORD index);
	void MakeCleanTrack(RiverInstruction *&rTrackOut, rev::DWORD &trackCount, rev::DWORD index);

	rev::DWORD MakeTrackFlg(rev::BYTE flags, RiverInstruction *&rMainOut, rev::DWORD &instrCount, RiverInstruction *&rTrackOut, rev::DWORD &trackCount, rev::DWORD index);

	rev::DWORD MakePreTrackReg(const RiverRegister &reg, RiverInstruction *&rMainOut, rev::DWORD &instrCount, rev::DWORD index);
	void MakeTrackReg(const RiverRegister &reg, RiverInstruction *&rTrackOut, rev::DWORD &trackCount, rev::DWORD index);
	
	void MakeTrackMem(const RiverAddress &mem, rev::WORD specifiers, rev::DWORD addrOffset, RiverInstruction *&rTrackOut, rev::DWORD &trackCount, rev::DWORD index);
	
	rev::DWORD MakeTrackAddress(rev::WORD specifiers, const RiverOperand &op, rev::BYTE optype, RiverInstruction *&rMainOut, rev::DWORD &instrCount, RiverInstruction *&rTrackOut, rev::DWORD &trackCount, rev::DWORD &valuesOut, rev::DWORD index);

	void MakeMarkFlg(rev::BYTE flags, rev::DWORD offset, RiverInstruction *&rMainOut, rev::DWORD &instrCount, RiverInstruction *&rTrackOut, rev::DWORD &trackCount, rev::DWORD index);
	void MakeMarkReg(const RiverRegister &reg, rev::DWORD addrOffset, rev::DWORD valueOffset, RiverInstruction *&rMainOut, rev::DWORD &instrCount, RiverInstruction *&rTrackOut, rev::DWORD &trackCount, rev::DWORD index);
	void MakeMarkMem(const RiverAddress &mem, rev::WORD specifiers, rev::DWORD addrOffset, rev::DWORD valueOffset, RiverInstruction *&rMainOut, rev::DWORD &instrCount, RiverInstruction *&rTrackOut, rev::DWORD &trackCount, rev::DWORD index);
	//void MakeSkipMem(const RiverAddress &mem, RiverInstruction *&rMainOut, rev::DWORD &instrCount, RiverInstruction *&rTrackOut, rev::DWORD &trackCount);
	
	/*  */
	void MakeTrackOp(rev::DWORD opIdx, const rev::BYTE type, const RiverOperand &op, rev::WORD specifiers, rev::DWORD addrOffset, RiverInstruction *&rMainOut, rev::DWORD &instrCount, RiverInstruction *&rTrackOut, rev::DWORD &trackCount, rev::DWORD &valueOffset, rev::DWORD index);
	void MakeMarkOp(const rev::BYTE type, rev::WORD specifiers, rev::DWORD addrOffset, rev::DWORD valueOffset, const RiverOperand &op, RiverInstruction *&rMainOut, rev::DWORD &instrCount, RiverInstruction *&rTrackOut, rev::DWORD &trackCount, rev::DWORD index);

	void MakeCallSymbolic(const RiverInstruction &rIn, RiverInstruction *&rMainOut, rev::DWORD &instrCount, RiverInstruction *&rTrackOut, rev::DWORD &trackCount, rev::DWORD index);

	/* Translators */
	void TranslateUnk(const RiverInstruction &rIn, RiverInstruction *&rMainOut, rev::DWORD &instrCount, RiverInstruction *&rTrackOut, rev::DWORD &trackCount, rev::DWORD dwTranslationFlags);
	void TranslateDefault(const RiverInstruction &rIn, RiverInstruction *&rMainOut, rev::DWORD &instrCount, RiverInstruction *&rTrackOut, rev::DWORD &trackCount, rev::DWORD dwTranslationFlags);
};


#endif
