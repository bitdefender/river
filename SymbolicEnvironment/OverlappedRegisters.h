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
		static const nodep::DWORD rOff[5], rSize[5], rParent[5], rMChild[5], rLChild[5];
		static const nodep::DWORD rSeed[4];
		static nodep::DWORD needConcat, needExtract;
		OverlappedRegistersEnvironment *parent;

		// marks children as need extraction
		void MarkNeedExtract(nodep::DWORD node, bool doRefCount);
		void MarkNeedConcat(nodep::DWORD node, bool doRefCount);
		void MarkUnset(nodep::DWORD node, bool doRefCount);

		void *Get(nodep::DWORD node, nodep::DWORD concreteValue);
	public:
		OverlappedRegister();

		void SetParent(OverlappedRegistersEnvironment *p);

		void *Get(RiverRegister &reg, nodep::DWORD &concreteValue);
		void Set(RiverRegister &reg, void *value, bool doRefCount);
		bool Unset(RiverRegister &reg, bool doRefCount);

		void SaveState(stk::LargeStack &stack);
		void LoadState(stk::LargeStack &stack);
	} subRegisters[8];

	RiverInstruction *current;
	AddRefFunc addRefFunc;
	DecRefFunc decRefFunc;

protected :
	virtual bool _SetCurrentInstruction(RiverInstruction *instruction, void *opBuffer);

	virtual void _SetReferenceCounting(AddRefFunc addRef, DecRefFunc decRef);

	virtual void _PushState(stk::LargeStack &stack);
	virtual void _PopState(stk::LargeStack &stack);

public :
	OverlappedRegistersEnvironment();

	virtual bool GetOperand(nodep::BYTE opIdx, nodep::BOOL &isTracked, nodep::DWORD &concreteValue, void *&symbolicValue);
	virtual bool SetOperand(nodep::BYTE opIdx, void *symbolicValue, bool doRefCount);
	virtual bool UnsetOperand(nodep::BYTE opIdx, bool doRefCount);
};

#endif
