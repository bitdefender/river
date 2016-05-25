#ifndef _Z3_SYMBOLIC_EXECUTOR_
#define _Z3_SYMBOLIC_EXECUTOR_

#include "z3.h"
#include "../revtracer/SymbolicEnvironment.h"

#include "VariableTracker.h"
#include "TrackingCookie.h"

class Z3SymbolicExecutor : public SymbolicExecutor {
private:
	void EvalZF(Z3_ast result);

	void SymbolicExecuteUnk(RiverInstruction *instruction);

	void SymbolicExecuteJz(RiverInstruction *instruction);
	void SymbolicExecuteJnz(RiverInstruction *instruction);

	void SymbolicExecuteXor(RiverInstruction *instruction);
	void SymbolicExecuteAdd(RiverInstruction *instruction);
	void SymbolicExecuteTest(RiverInstruction *instruction);
	void SymbolicExecuteCmp(RiverInstruction *instruction);
	void SymbolicExecuteMov(RiverInstruction *instruction);
	void SymbolicExecuteMovSx(RiverInstruction *instruction);
	void SymboliExecute0x83(RiverInstruction *instruction);

public:
	int symIndex;

	TrackingCookieFuncs *cookieFuncs;

	Z3_config config;
	Z3_context context;
	Z3_sort dwordSort, byteSort, bitSort;

	Z3_ast zero32, zeroFlag, oneFlag;

	Z3_ast lastCondition;

	Z3_solver solver;

	VariableTracker<Z3_ast> variableTracker;

	typedef void(Z3SymbolicExecutor::*SymbolicExecute)(RiverInstruction *instruction);

	static SymbolicExecute executeFuncs[2][0x100];

	Z3SymbolicExecutor(SymbolicEnvironment *e, TrackingCookieFuncs *f);

	void SymbolicExecuteDispatch(RiverInstruction *instruction);

	void StepForward();
	void StepBackward();
	void Lock(Z3_ast t);
};

#endif

