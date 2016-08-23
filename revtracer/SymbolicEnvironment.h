#ifndef _SYMBOLIC_ENVIRONMENT_H_
#define _SYMBOLIC_ENVIRONMENT_H_

#include "river.h"

/** This header describes available interactions between River and an SMT solver.
 * In order to integrate a custom SMT solver with River one must follow this checklist:
 *		1) Implement and populate a SymbolicExecutorFuncs structure.
 *		2) Inherit the SymbolicExecutor funcs.
 *		3) Implement a SymExeConstructorFunc that returns a new SymbolicExecutor instance.
 *		4) Profit!
 *
 * This method appears to be a lot more complicated than necessary. Indeed, function polimorphism
 * is a central aspect of C++. Unfortunately, polimorphism is not available in in River, because
 * River can't rely on any CRT or STL feature. The next best thing is to simulate polymorphism 
 * ourselves unsing a class member as _vptr substitute.
 */

namespace rev {

	/** Following function types are part of the symbolic environment implementation */
	typedef bool(*GetOperandFunc)(void *_this, rev::BYTE opIdx, rev::BOOL &isTracked, rev::DWORD &concreteValue, void *&symbolicValue);
	typedef bool(*GetFlgValueFunc)(void *_this, rev::BYTE flg, rev::BOOL &isTracked, rev::BYTE &concreteValue, void *&symbolicValue);

	typedef bool(*SetOperandFunc)(void *_this, rev::BYTE opIdx, void *symbolicValue);
	typedef bool(*UnsetOperandFunc)(void *_this, rev::BYTE opIdx);

	typedef void(*SetFlgValueFunc)(void *_this, rev::BYTE flg, void *symbolicValue);
	typedef void(*UnsetFlgValueFunc)(void *_this, rev::BYTE flg);

	typedef void(*SetRegValueFunc)(void *_this, RiverRegister reg, void *symbolicValue);
	typedef void(*UnsetRegValueFunc)(void *_this, RiverRegister reg);

	class SymbolicEnvironmentFuncs {
	public :
		GetOperandFunc GetOperand;
		GetFlgValueFunc GetFlgValue;

		SetOperandFunc SetOperand;
		UnsetOperandFunc UnsetOperand;

		SetFlgValueFunc SetFlgValue;
		UnsetFlgValueFunc UnsetFlgValue;

		SetRegValueFunc SetRegValue;
		UnsetRegValueFunc UnsetRegValue;
	};

	class SymbolicExecutorFuncs;

	/** 
		The symbolic environment
	 */
	class SymbolicEnvironment {
	protected :
		SymbolicEnvironmentFuncs *ops;
		SymbolicExecutorFuncs *execOps;
	public :

		void SetExecutorFuncs(SymbolicExecutorFuncs *_execOps) {
			execOps = _execOps;
		}

		/**
		 *	The GetOperand function returns an operand by index.
		 *	Params:
		 *		opIdx - input, specifies the operand index.
		 *		isTracked - output, returns true if the operand is tracked.
		 *		concreteValue - output, returns the operands concrete value.
		 *		symbolicValue - output, if isSymblic is true, this variable holds the symbolic expression.
		 *  Result:
		 *		true - if the operand can be interogated
		 *		false - if the operand can't be interogated, (if the operand type is RIVER_OPERAND_NONE, or if
		 *				the RIVER_SPEC_IGNORES_OP(opIdx) flag is specified).
		 */
		bool GetOperand(rev::BYTE opIdx, rev::BOOL &isTracked, rev::DWORD &concreteValue, void *&symbolicValue) {
			return ops->GetOperand(this, opIdx, isTracked, concreteValue, symbolicValue);
		}

		/**
		 *	The GetFlgValue function returns a flag by flag value.
		 *	Params:
		 *		flg - input, one of the RIVER_SPEC_FLAG_?F constants.
		 *		isTracked - output, returns true if the flag is tracked.
		 *		concreteValue - output, returns the flags concrete value.
		 *		symbolicValue - output, if isSymblic is true, this variable holds the symbolic expression.
		 *  Result:
		 *		true - if the flag can be interogated
		 *		false - if the flag can't be interogated, (if the flag is not in the modFlags bitmask).
		 */
		bool GetFlgValue(rev::BYTE flg, rev::BOOL &isTracked, rev::BYTE &concreteValue, void *&symbolicValue) {
			return ops->GetFlgValue(this, flg, isTracked, concreteValue, symbolicValue);
		}

		/**
		 *  The SetOperand function binds a symbolic expression to an operand.
		 *  Params:
		 *		opIdx - input, specifies the operand index.
		 *		symbolicValue - input, symbolic expression to bind.
		 *  Result:
		 *		true - if the expression was bound successfully
		 *		false - otherwise
		 */
		bool SetOperand(rev::BYTE opIdx, void *symbolicValue) {
			return ops->SetOperand(this, opIdx, symbolicValue);
		}

		/**
		 *  The UnsetOperand unbinds the symbolic expression from the specified operand.
		 *  Params:
		 *		opIdx - input, specifies the operand index.
		 */
		bool UnsetOperand(rev::BYTE opIdx) {
			return ops->UnsetOperand(this, opIdx);
		}

		/**
		 *  The SetFlgValue function binds a symbolic expression to flag.
		 *  Params:
		 *		flg - input, one of the RIVER_SPEC_FLAG_?F constants.
		 *		symbolicValue - input, symbolic expression to bind.
		 *  Result:
		 *		true - if the expression was bound successfully
		 *		false - otherwise
		 */
		void SetFlgValue(rev::BYTE flg, void *symbolicValue) {
			ops->SetFlgValue(this, flg, symbolicValue);
		}

		/**
		 *  The UnsetFlgValue unbinds the symbolic expression from the specified operand.
		 *  Params:
		 *		flg - input, one of the RIVER_SPEC_FLAG_?F constants.
		 */
		void UnsetFlgValue(rev::BYTE flg) {
			ops->UnsetFlgValue(this, flg);
		}

		/* Avoid the functions below as much as possible! */
		/**
		 *  The SetRegValue function binds a symbolic value to a specific register.
		 *  Params:
		 *		reg - input, one of the RIVER_REG_* constants.
		 *		symbolicValue - input, symbolic expression to bind.
		 */
		void SetRegValue(RiverRegister reg, void *&symbolicValue) {
			ops->SetRegValue(this, reg, symbolicValue);
		}

		/**
		*  The UnsetRegValue unbinds the symbolic expression from the specified register.
		*  Params:
		*		reg - input, one of the RIVER_REG_* constants.
		*/
		void UnsetRegValue(RiverRegister reg) {
			ops->UnsetRegValue(this, reg);
		}
	};


	/** Following function types are part of the symbolic executor interface */
	typedef void *(*CreateVariableFunc)(void *_this, const char *name);
	typedef void *(*MakeConstFunc)(void *_this, rev::DWORD value, rev::DWORD bits); 
	
	typedef void *(*ExtractBitsFunc)(void *_this, void *expr, rev::DWORD lsb, rev::DWORD size);
	typedef void *(*ConcatBitsFunc)(void *_this, void *expr1, void *expr2);

	typedef void(*ExecuteFunc)(void *_this, RiverInstruction *instruction);
	

	class SymbolicExecutorFuncs {
	public:
		CreateVariableFunc CreateVariable;
		MakeConstFunc MakeConst;
		
		ExtractBitsFunc ExtractBits;
		ConcatBitsFunc ConcatBits;

		ExecuteFunc Execute;
	};

	/** Executor class. Inherit this class to provide bindings to a SMT solver. */
	class SymbolicExecutor {
	public:
		SymbolicEnvironment *env;
		SymbolicExecutorFuncs *ops;
	public:

		SymbolicExecutor(SymbolicEnvironment *e, SymbolicExecutorFuncs *f) {
			env = e;
			ops = f;
			env->SetExecutorFuncs(ops);
		}

		// Create a new symbolic variable
		void *CreateVariable(const char *name) {
			return ops->CreateVariable(this, name);
		}

		// Make a new constant
		void *MakeConst(rev::DWORD value, rev::DWORD bits) {
			return ops->MakeConst(this, value, bits);
		}

		// Extract bits from an expression
		void *ExtractBits(void *expr, rev::DWORD lsb, rev::DWORD size) {
			return ops->ExtractBits(this, expr, lsb, size);
		}

		void *ConcatBits(void *expr1, void *expr2) {
			return ops->ConcatBits(this, expr1, expr2);
		}

		// Access the symbolic environment through env->* methods
		// The environment might be subject to change as more features are added
		void Execute(RiverInstruction *instruction) {
			ops->Execute(this, instruction);
		}
	};

	/* See the documentation header about why this function definition exists */
	typedef SymbolicExecutor *(*SymExeConstructorFunc)(SymbolicEnvironment *env);
};

#endif
