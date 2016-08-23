#ifndef _OVERLAPPED_REGISTERS_H_
#define _OVERLAPPED_REGISTERS_H_

#include "LargeStack.h"
#include "SymbolicEnvironment.h"

#include "../revtracer/river.h"

class OverlappedRegistersEnvironment : public sym::ScopedSymbolicEnvironment {
private :
	class OverlappedRegister {
	private:
		void *subRegs[5];
		static const rev::DWORD rOff[5], rSize[5], rParent[5], rMChild[5], rLChild[5];
		static const rev::DWORD rSeed[4];
		static rev::DWORD needConcat, needExtract;
		sym::SymbolicExecutor *parent;

		// marks children as need extraction
		void MarkNeedExtract(rev::DWORD node);
		void MarkUnset(rev::DWORD node);

		void *Get(rev::DWORD node, rev::DWORD concreteValue);
	public:
		void SetParent(sym::SymbolicExecutor *p);

		void *Get(RiverRegister &reg, rev::DWORD &concreteValue);
		void Set(RiverRegister &reg, void *value);
		bool Unset(RiverRegister &reg);

		void SaveState(stk::LargeStack &stack);
		void LoadState(stk::LargeStack &stack);
	} subRegisters[8];

	RiverInstruction *current;

protected :
	virtual bool _SetCurrentInstruction(RiverInstruction *instruction, void *opBuffer);

	virtual void _PushState(stk::LargeStack &stack);
	virtual void _PopState(stk::LargeStack &stack);

public :
	virtual bool GetOperand(rev::BYTE opIdx, rev::BOOL &isTracked, rev::DWORD &concreteValue, void *&symbolicValue);
	virtual bool SetOperand(rev::BYTE opIdx, void *symbolicValue);
	virtual bool UnsetOperand(rev::BYTE opIdx);
};

#endif
