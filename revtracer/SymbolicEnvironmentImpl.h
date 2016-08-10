#ifndef _SYMBOLIC_ENVIRONMENT_IMPL_H_
#define _SYMBOLIC_ENVIRONMENT_IMPL_H_

#include "SymbolicEnvironment.h"
#include "TrackedValues.h"
#include "execenv.h"

class SymbolicEnvironmentImpl : public SymbolicEnvironment {
public :
	ExecutionEnvironment *pEnv;

	
	RiverInstruction *current;
	rev::DWORD *opBase;
	
	rev::DWORD addressOffsets[4], valueOffsets[4], flagOffset;

	class SymbolicOverlappedRegister {
	private :
		void *subRegs[5];
		static const rev::DWORD rOff[5], rSize[5], rParent[5], rMChild[5], rLChild[5];
		static const rev::DWORD rSeed[4];
		static rev::DWORD needConcat, needExtract;
		SymbolicEnvironmentImpl *parent;

		// marks children as need extraction
		void MarkNeedExtract(rev::DWORD node);
		void MarkUnset(rev::DWORD node);

		void *Get(rev::DWORD node);
	public :
		void *Get(RiverRegister &reg);
		void Set(RiverRegister &reg, void *value);
		void Unset(RiverRegister &reg);
	};



	void GetOperandLayout(const RiverInstruction &rIn);
public :
	void Init(ExecutionEnvironment *env);
	void SetCurrentContext(RiverInstruction *rIn, void *context);
};

#endif
