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



	void GetOperandLayout(const RiverInstruction &rIn);
public :
	void Init(ExecutionEnvironment *env);
	void SetCurrentContext(RiverInstruction *rIn, void *context);
};

#endif
