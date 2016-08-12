#ifndef _Z3_SYMBOLIC_EXECUTOR_
#define _Z3_SYMBOLIC_EXECUTOR_

#include "z3.h"
#include "../revtracer/SymbolicEnvironment.h"

#include "VariableTracker.h"
#include "TrackingCookie.h"
#include "LargeStack.h"

#define OPERAND_BITMASK(idx) (0x00010000 << (idx))

class Z3SymbolicExecutor : public rev::SymbolicExecutor {
private:
	stk::DWORD saveTop;
	stk::DWORD saveStack[0x10000];
	stk::LargeStack *ls;

	struct SymbolicOperands {
		rev::DWORD av;
		rev::BOOL tr[4];
		rev::DWORD cv[4];
		void *sv[4];

		rev::BOOL trf[7];
		rev::BYTE cvf[7];
		void *svf[7];
	};

	//void EvalZF(Z3_ast result);

	void SymbolicExecuteUnk(RiverInstruction *instruction, SymbolicOperands *ops);

	template <unsigned int flag> void SymbolicExecuteJCC(RiverInstruction *instruction, SymbolicOperands *ops);

	void SymbolicExecuteMov(RiverInstruction *instruction, SymbolicOperands *ops);
	void SymbolicExecuteMovSx(RiverInstruction *instruction, SymbolicOperands *ops);

	Z3_ast ExecuteAdd(Z3_ast o1, Z3_ast o2);
	Z3_ast ExecuteOr (Z3_ast o1, Z3_ast o2);
	Z3_ast ExecuteAdc(Z3_ast o1, Z3_ast o2);
	Z3_ast ExecuteSbb(Z3_ast o1, Z3_ast o2);
	Z3_ast ExecuteAnd(Z3_ast o1, Z3_ast o2);
	Z3_ast ExecuteSub(Z3_ast o1, Z3_ast o2);
	Z3_ast ExecuteXor(Z3_ast o1, Z3_ast o2);
	Z3_ast ExecuteCmp(Z3_ast o1, Z3_ast o2);
	Z3_ast ExecuteTest(Z3_ast o1, Z3_ast o2);

	typedef Z3_ast (Z3SymbolicExecutor::*IntegerFunc)(Z3_ast o1, Z3_ast o2);
	template <Z3SymbolicExecutor::IntegerFunc func, unsigned int funcCode> void SymbolicExecuteInteger(RiverInstruction *instruction, SymbolicOperands *ops);

	void GetSymbolicValues(SymbolicOperands *ops, rev::DWORD mask);
public:
	int symIndex;

	TrackingCookieFuncs *cookieFuncs;

	Z3_config config;
	Z3_context context;
	Z3_sort dwordSort, wordSort, byteSort, bitSort;

	Z3_ast zero32, zeroFlag, oneFlag;

	class Z3SymbolicCpuFlag {
	protected:
		Z3SymbolicExecutor *parent;
		virtual Z3_ast Eval() = 0;
		Z3_ast value;

		static const unsigned int lazyMarker;
	public :
		Z3SymbolicCpuFlag();
		void SetParent(Z3SymbolicExecutor *p);

		virtual void SetSource(Z3_ast src, Z3_ast o1, Z3_ast o2, Z3_ast o3, unsigned int op) = 0;
		virtual void SaveState(stk::LargeStack &stack) = 0;
		virtual void LoadState(stk::LargeStack &stack) = 0;

		void SetValue(Z3_ast val);
		void Unset();

		Z3_ast GetValue();
	} *lazyFlags[7];

	Z3_ast lastCondition;

	Z3_solver solver;

	VariableTracker<Z3_ast> variableTracker;

	typedef void(Z3SymbolicExecutor::*SymbolicExecute)(RiverInstruction *instruction, SymbolicOperands *ops);
	template <Z3SymbolicExecutor::SymbolicExecute fSubOps[8]> void SymbolicExecuteSubOp(RiverInstruction *instruction, SymbolicOperands *ops);
	
	static SymbolicExecute executeFuncs[2][0x100];
	static SymbolicExecute executeIntegerFuncs[8];

	Z3SymbolicExecutor(rev::SymbolicEnvironment *e, TrackingCookieFuncs *f);
	~Z3SymbolicExecutor();

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

	virtual void SaveState(stk::LargeStack &stack);
	virtual void LoadState(stk::LargeStack &stack);
};


class Z3FlagSF : public Z3SymbolicExecutor::Z3SymbolicCpuFlag {
private:
	Z3_ast source;
protected:
	virtual Z3_ast Eval();
	virtual void SetSource(Z3_ast src, Z3_ast o1, Z3_ast o2, Z3_ast o3, unsigned int op);

	virtual void SaveState(stk::LargeStack &stack);
	virtual void LoadState(stk::LargeStack &stack);
};

class Z3FlagPF : public Z3SymbolicExecutor::Z3SymbolicCpuFlag {
private:
	Z3_ast source;
protected:
	virtual Z3_ast Eval();
	virtual void SetSource(Z3_ast src, Z3_ast o1, Z3_ast o2, Z3_ast o3, unsigned int op);

	virtual void SaveState(stk::LargeStack &stack);
	virtual void LoadState(stk::LargeStack &stack);
};

#define Z3_FLAG_OP_ADD		0xA0
#define Z3_FLAG_OP_OR 		0xA1
#define Z3_FLAG_OP_ADC		0xA2
#define Z3_FLAG_OP_SBB		0xA3
#define Z3_FLAG_OP_AND		0xA4
#define Z3_FLAG_OP_SUB		0xA5
#define Z3_FLAG_OP_XOR		0xA6
#define Z3_FLAG_OP_CMP		0xA7


class Z3FlagCF : public Z3SymbolicExecutor::Z3SymbolicCpuFlag {
private:
	Z3_ast source, p[2];
	unsigned int func;
protected:
	virtual Z3_ast Eval();
	virtual void SetSource(Z3_ast src, Z3_ast o1, Z3_ast o2, Z3_ast o3, unsigned int op);

	virtual void SaveState(stk::LargeStack &stack);
	virtual void LoadState(stk::LargeStack &stack);
};

class Z3FlagOF : public Z3SymbolicExecutor::Z3SymbolicCpuFlag {
private:
	Z3_ast source, p[2];
	unsigned int func;
protected:
	virtual Z3_ast Eval();
	virtual void SetSource(Z3_ast src, Z3_ast o1, Z3_ast o2, Z3_ast o3, unsigned int op);

	virtual void SaveState(stk::LargeStack &stack);
	virtual void LoadState(stk::LargeStack &stack);
};

class Z3FlagAF : public Z3SymbolicExecutor::Z3SymbolicCpuFlag {
private:
	Z3_ast source;
protected:
	virtual Z3_ast Eval();
	virtual void SetSource(Z3_ast src, Z3_ast o1, Z3_ast o2, Z3_ast o3, unsigned int op);

	virtual void SaveState(stk::LargeStack &stack);
	virtual void LoadState(stk::LargeStack &stack);
};

class Z3FlagDF : public Z3SymbolicExecutor::Z3SymbolicCpuFlag {
private:
	Z3_ast source;
protected:
	virtual Z3_ast Eval();
	virtual void SetSource(Z3_ast src, Z3_ast o1, Z3_ast o2, Z3_ast o3, unsigned int op);

	virtual void SaveState(stk::LargeStack &stack);
	virtual void LoadState(stk::LargeStack &stack);
};


#endif

