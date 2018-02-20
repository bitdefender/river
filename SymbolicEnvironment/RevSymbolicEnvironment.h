#ifndef _REV_SYMBOLIC_ENVIRONMENT_H_
#define _REV_SYMBOLIC_ENVIRONMENT_H_

#include "SymbolicEnvironment.h"

class ExecutionController;

class RevSymbolicEnvironment : public sym::SymbolicEnvironment {
private :
	RiverInstruction *current;
	ExecutionController *ctrl;
	nodep::DWORD *opBase;
	
	nodep::DWORD addressOffsets[4];
	nodep::DWORD baseOffsets[4];
	nodep::DWORD indexOffsets[4];
	nodep::DWORD inValueOffsets[4];
	nodep::DWORD outValueOffsets[4];
	nodep::DWORD inFlagOffset;
	nodep::DWORD outFlagOffset;

	AddRefFunc addRefFunc;
	DecRefFunc decRefFunc;

	void *pEnv;

	void GetOperandLayout(const RiverInstruction &rIn);

	typedef void *(RevSymbolicEnvironment::*GetSubExpFunc)(nodep::DWORD address);
	template <nodep::BYTE offset, nodep::BYTE size> void *GetSubexpression(nodep::DWORD address);
	void *GetSubexpressionInvalid(nodep::DWORD address);
	static GetSubExpFunc subExpsGet[4][5];

	typedef void (RevSymbolicEnvironment::*SetSubExpFunc)(void *expr, nodep::DWORD address, void *value);
	template <nodep::BYTE offset, nodep::BYTE size> void SetSubexpression(void *expr, nodep::DWORD address, void *value);
	template <nodep::BYTE size> void SetSubexpressionOff0(void *expr, nodep::DWORD address, void *value);
	template <nodep::BYTE size> void SetSubexpressionOffM(void *expr, nodep::DWORD address, void *value);
	void SetSubexpressionInvalid(void *expr, nodep::DWORD address, void *value);
	static SetSubExpFunc subExpsSet[4][5];

	void *GetExpression(nodep::DWORD address, nodep::DWORD size);
	void SetExpression(void *exp, nodep::DWORD address, nodep::DWORD size, nodep::DWORD *values);


public :
	RevSymbolicEnvironment(void *revEnv, ExecutionController *ctl);

	virtual void SetReferenceCounting(AddRefFunc addRef, DecRefFunc decRef);
	virtual bool SetCurrentInstruction(RiverInstruction *instruction, void *opBuffer);

	virtual void PushState(stk::LargeStack &stack);
	virtual void PopState(stk::LargeStack &stack);

	virtual bool GetOperand(struct OperandInfo &opInfo);
	virtual bool GetAddressBase(struct OperandInfo &opInfo);
	virtual bool GetAddressScaleAndIndex(struct OperandInfo &opInfo, nodep::BYTE &scale);

	virtual bool GetFlgValue(struct FlagInfo &flagInfo);
	virtual bool SetOperand(nodep::BYTE opIdx, void *symbolicValue, bool doRefCount);
	virtual bool UnsetOperand(nodep::BYTE opIdx, bool doRefCount);
	virtual void SetFlgValue(nodep::BYTE flg, void *symbolicValue, bool doRefCount);
	virtual void UnsetFlgValue(nodep::BYTE flg, bool doRefCount);

	virtual void SetSymbolicVariable(const char *name, rev::ADDR_TYPE addr, nodep::DWORD size);
};

#endif


