#ifndef _REV_SYMBOLIC_ENVIRONMENT_H_
#define _REV_SYMBOLIC_ENVIRONMENT_H_

#include "SymbolicEnvironment.h"

class RevSymbolicEnvironment : public sym::SymbolicEnvironment {
private :
	RiverInstruction *current;
	rev::DWORD *opBase;
	rev::DWORD addressOffsets[4], valueOffsets[4], flagOffset;

	void *pEnv;

	void GetOperandLayout(const RiverInstruction &rIn);
public :
	RevSymbolicEnvironment(void *revEnv);

	virtual bool SetCurrentInstruction(RiverInstruction *instruction, void *opBuffer);

	virtual void PushState(stk::LargeStack &stack);
	virtual void PopState(stk::LargeStack &stack);

	virtual bool GetOperand(rev::BYTE opIdx, rev::BOOL &isTracked, rev::DWORD &concreteValue, void *&symbolicValue);
	virtual bool GetFlgValue(rev::BYTE flg, rev::BOOL &isTracked, rev::BYTE &concreteValue, void *&symbolicValue);
	virtual bool SetOperand(rev::BYTE opIdx, void *symbolicValue);
	virtual bool UnsetOperand(rev::BYTE opIdx);
	virtual void SetFlgValue(rev::BYTE flg, void *symbolicValue);
	virtual void UnsetFlgValue(rev::BYTE flg);
};

#endif


