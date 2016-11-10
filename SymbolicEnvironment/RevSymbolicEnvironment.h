#ifndef _REV_SYMBOLIC_ENVIRONMENT_H_
#define _REV_SYMBOLIC_ENVIRONMENT_H_

#include "SymbolicEnvironment.h"

class ExecutionController;

class RevSymbolicEnvironment : public sym::SymbolicEnvironment {
private :
	RiverInstruction *current;
	ExecutionController *ctrl;
	rev::DWORD *opBase;
	rev::DWORD addressOffsets[4], valueOffsets[4], flagOffset;

	AddRefFunc addRefFunc;
	DecRefFunc decRefFunc;

	void *pEnv;

	void GetOperandLayout(const RiverInstruction &rIn);

	typedef void *(RevSymbolicEnvironment::*GetSubExpFunc)(rev::DWORD address);
	template <rev::BYTE offset, rev::BYTE size> void *GetSubexpression(rev::DWORD address);
	void *GetSubexpressionInvalid(rev::DWORD address);
	static GetSubExpFunc subExpsGet[4][5];

	typedef void (RevSymbolicEnvironment::*SetSubExpFunc)(void *expr, rev::DWORD address, void *value);
	template <rev::BYTE offset, rev::BYTE size> void SetSubexpression(void *expr, rev::DWORD address, void *value);
	template <rev::BYTE size> void SetSubexpressionOff0(void *expr, rev::DWORD address, void *value);
	template <rev::BYTE size> void SetSubexpressionOffM(void *expr, rev::DWORD address, void *value);
	void SetSubexpressionInvalid(void *expr, rev::DWORD address, void *value);
	static SetSubExpFunc subExpsSet[4][5];

	void *GetExpression(rev::DWORD address, rev::DWORD size);
	void SetExpression(void *exp, rev::DWORD address, rev::DWORD size, rev::DWORD *values);


public :
	RevSymbolicEnvironment(void *revEnv, ExecutionController *ctl);

	virtual void SetReferenceCounting(AddRefFunc addRef, DecRefFunc decRef);
	virtual bool SetCurrentInstruction(RiverInstruction *instruction, void *opBuffer);

	virtual void PushState(stk::LargeStack &stack);
	virtual void PopState(stk::LargeStack &stack);

	virtual bool GetOperand(rev::BYTE opIdx, rev::BOOL &isTracked, rev::DWORD &concreteValue, void *&symbolicValue);
	virtual bool GetFlgValue(rev::BYTE flg, rev::BOOL &isTracked, rev::BYTE &concreteValue, void *&symbolicValue);
	virtual bool SetOperand(rev::BYTE opIdx, void *symbolicValue);
	virtual bool UnsetOperand(rev::BYTE opIdx);
	virtual void SetFlgValue(rev::BYTE flg, void *symbolicValue);
	virtual void UnsetFlgValue(rev::BYTE flg);

	virtual void SetSymbolicVariable(const char *name, rev::ADDR_TYPE addr, rev::DWORD size);
};

#endif


