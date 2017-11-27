#ifndef _Z3_SYMBOLIC_EXECUTOR_
#define _Z3_SYMBOLIC_EXECUTOR_

#include "z3.h"

#include "../SymbolicEnvironment/SymbolicEnvironment.h"
#include "../SymbolicEnvironment/LargeStack.h"

#include "VariableTracker.h"

#define OPERAND_BITMASK(idx) (0x00010000 << (idx))

class Z3SymbolicExecutor : public sym::SymbolicExecutor {
private:
	stk::DWORD saveTop;
	stk::DWORD saveStack[0x10000];
	stk::LargeStack *ls;

	struct SymbolicOperands {
		nodep::DWORD av;
		nodep::BOOL tr[4];
		nodep::DWORD cv[4];
		void *sv[4];

		nodep::BOOL trf[7];
		nodep::BYTE cvf[7];
		void *svf[7];
	};

	//void EvalZF(Z3_ast result);

	void SymbolicExecuteUnk(RiverInstruction *instruction, SymbolicOperands *ops);

	template <unsigned int flag> 
	void SymbolicExecuteJCC(RiverInstruction *instruction, SymbolicOperands *ops);
	
	template <unsigned int f1, unsigned int f2, bool eq> 
	void SymbolicExecuteJCCCompare(RiverInstruction *instruction, SymbolicOperands *ops);

	void SymbolicExecuteMov(RiverInstruction *instruction, SymbolicOperands *ops);
	void SymbolicExecuteMovSx(RiverInstruction *instruction, SymbolicOperands *ops);
	void SymbolicExecuteMovZx(RiverInstruction *instruction, SymbolicOperands *ops);

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

	void GetSymbolicValues(SymbolicOperands *ops, nodep::DWORD mask);
public:
	int symIndex;

	Z3_config config;
	Z3_context context;
	Z3_sort dwordSort, wordSort, tbSort, byteSort, bitSort;

	Z3_ast zero32, zero16, zero8, zeroFlag, oneFlag;
	Z3_ast zeroScale, twoScale, fourScale;

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

	//VariableTracker<Z3_ast> variableTracker;

	typedef void(Z3SymbolicExecutor::*SymbolicExecute)(RiverInstruction *instruction, SymbolicOperands *ops);
	template <Z3SymbolicExecutor::SymbolicExecute fSubOps[8]> void SymbolicExecuteSubOp(RiverInstruction *instruction, SymbolicOperands *ops);
	
	static SymbolicExecute executeFuncs[2][0x100];
	static SymbolicExecute executeIntegerFuncs[8];

	Z3SymbolicExecutor(sym::SymbolicEnvironment *e);
	~Z3SymbolicExecutor();

	void StepForward();
	void StepBackward();
	//void Lock(Z3_ast t);


	virtual void *CreateVariable(const char *name, nodep::DWORD size);
	virtual void *MakeConst(nodep::DWORD value, nodep::DWORD bits);
	virtual void *ExtractBits(void *expr, nodep::DWORD lsb, nodep::DWORD size);
	virtual void *ConcatBits(void *expr1, void *expr2);
	virtual void Execute(RiverInstruction *instruction);
	virtual void *ExecuteResolveAddress(void *base, void *index, nodep::BYTE scale);
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

