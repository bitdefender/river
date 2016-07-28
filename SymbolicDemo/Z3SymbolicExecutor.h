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

	void SymbolicExecuteTest(RiverInstruction *instruction);
	void SymbolicExecuteCmp(RiverInstruction *instruction);
	void SymbolicExecuteMov(RiverInstruction *instruction);
	void SymbolicExecuteMovSx(RiverInstruction *instruction);
	void SymboliExecute0x83(RiverInstruction *instruction);

	Z3_ast ExecuteAdd(Z3_ast o1, Z3_ast o2);
	Z3_ast ExecuteOr (Z3_ast o1, Z3_ast o2);
	Z3_ast ExecuteAdc(Z3_ast o1, Z3_ast o2);
	Z3_ast ExecuteSbb(Z3_ast o1, Z3_ast o2);
	Z3_ast ExecuteAnd(Z3_ast o1, Z3_ast o2);
	Z3_ast ExecuteSub(Z3_ast o1, Z3_ast o2);
	Z3_ast ExecuteXor(Z3_ast o1, Z3_ast o2);

	typedef Z3_ast (Z3SymbolicExecutor::*IntegerFunc)(Z3_ast o1, Z3_ast o2);
	template <Z3SymbolicExecutor::IntegerFunc func, unsigned int funcCode> void SymbolicExecuteInteger(RiverInstruction *instruction);

public:
	int symIndex;

	TrackingCookieFuncs *cookieFuncs;

	Z3_config config;
	Z3_context context;
	Z3_sort dwordSort, byteSort, bitSort;

	Z3_ast zero32, zeroFlag, oneFlag;

	class Z3SymbolicCpuFlag {
	private :
		Z3_ast value;
	protected:
		Z3SymbolicExecutor *parent;
		virtual Z3_ast Eval() = 0;

		static const unsigned int lazyMarker;
	public :
		Z3SymbolicCpuFlag();
		void SetParent(Z3SymbolicExecutor *p);

		virtual void SetSource(Z3_ast src, Z3_ast o1, Z3_ast o2, Z3_ast o3, unsigned int op) = 0;
		void SetValue(Z3_ast val);
		void Unset();

		Z3_ast GetValue();
	} *lazyFlags[7];

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

class Z3FlagZF : public Z3SymbolicExecutor::Z3SymbolicCpuFlag {
private :
	Z3_ast source;
protected :
	virtual Z3_ast Eval();
	virtual void SetSource(Z3_ast src, Z3_ast o1, Z3_ast o2, Z3_ast o3, unsigned int op);
};


class Z3FlagSF : public Z3SymbolicExecutor::Z3SymbolicCpuFlag {
private:
	Z3_ast source;
protected:
	virtual Z3_ast Eval();
	virtual void SetSource(Z3_ast src, Z3_ast o1, Z3_ast o2, Z3_ast o3, unsigned int op);
};

class Z3FlagPF : public Z3SymbolicExecutor::Z3SymbolicCpuFlag {
private:
	Z3_ast source;
protected:
	virtual Z3_ast Eval();
	virtual void SetSource(Z3_ast src, Z3_ast o1, Z3_ast o2, Z3_ast o3, unsigned int op);
};

#define Z3_FLAG_OP_ADD		0x80
#define Z3_FLAG_OP_SUB		0x81
#define Z3_FLAG_OP_XOR		0x82

class Z3FlagCF : public Z3SymbolicExecutor::Z3SymbolicCpuFlag {
private:
	Z3_ast source, p[2];
	unsigned int func;
protected:
	virtual Z3_ast Eval();
	virtual void SetSource(Z3_ast src, Z3_ast o1, Z3_ast o2, Z3_ast o3, unsigned int op);
};

class Z3FlagOF : public Z3SymbolicExecutor::Z3SymbolicCpuFlag {
private:
	Z3_ast source, p[2];
	unsigned int func;
protected:
	virtual Z3_ast Eval();
	virtual void SetSource(Z3_ast src, Z3_ast o1, Z3_ast o2, Z3_ast o3, unsigned int op);
};

class Z3FlagAF : public Z3SymbolicExecutor::Z3SymbolicCpuFlag {
private:
	Z3_ast source;
protected:
	virtual Z3_ast Eval();
	virtual void SetSource(Z3_ast src, Z3_ast o1, Z3_ast o2, Z3_ast o3, unsigned int op);
};

class Z3FlagDF : public Z3SymbolicExecutor::Z3SymbolicCpuFlag {
private:
	Z3_ast source;
protected:
	virtual Z3_ast Eval();
	virtual void SetSource(Z3_ast src, Z3_ast o1, Z3_ast o2, Z3_ast o3, unsigned int op);
};


#endif

