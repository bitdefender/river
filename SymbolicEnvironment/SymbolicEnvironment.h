#ifndef _SYMBOLIC_ENVIRONMENT_H_
#define _SYMBOLIC_ENVIRONMENT_H_

#include "../revtracer/river.h"
#include "../Execution/Execution.h"
#include "LargeStack.h"

struct OperandInfo {
	nodep::BYTE opIdx;
	nodep::BOOL isTracked;
	nodep::DWORD concrete;
	void *symbolic;
};

struct FlagInfo {
	nodep::BYTE opIdx;
	nodep::BOOL isTracked;
	nodep::BYTE concrete;
	void *symbolic;
};

namespace sym {

	class SymbolicExecutor;

	class SymbolicEnvironment {
	protected :
		SymbolicExecutor *exec;
	public :
		/**
		Function typed for monitoring variable reference counting
		*/
		typedef void(*AddRefFunc)(void *);
		typedef void(*DecRefFunc)(void *);

		virtual void SetReferenceCounting(AddRefFunc addRef, DecRefFunc decRef) = 0;

		/**
		*	The set current instruction gets called by the symbolic handler.
		*	This function parses instruction operands and makes them available for the SymbolicExecutor
		*	Params:
		*		instruction - input, the instruction about to be executed
		*		obBuffer - input, buffer storing concrete operand values
		*/
		virtual bool SetCurrentInstruction(RiverInstruction *instruction, void *opBuffer) = 0;

		virtual bool SetExecutor(SymbolicExecutor *e);

		virtual void PushState(stk::LargeStack &stack) = 0;
		virtual void PopState(stk::LargeStack &stack) = 0;

		virtual void SetSymbolicVariable(const char *name, rev::ADDR_TYPE addr, nodep::DWORD size) = 0;

		/**
		*	The GetOperand function returns an operand by index.
		*	Params:
		*		opInfo - struct containing operand info
		*  Result:
		*		true - if the operand can be interogated
		*		false - if the operand can't be interogated, (if the operand type is RIVER_OPERAND_NONE, or if
		*				the RIVER_SPEC_IGNORES_OP(opIdx) flag is specified).
		*/
		virtual bool GetOperand(struct OperandInfo &opInfo) = 0;

		/**
		*	The GetAddressBase function returns the base of an address operand by index.
		*	Params:
		*		opInfo - struct containing operand info
		*  Result:
		*		true - if the operand can be interogated
		*		false - if the operand can't be interogated, (all excepting RIVER_OPTYPE_MEM)
		*/
		virtual bool GetAddressBase(struct OperandInfo &opInfo) = 0;

		/**
		*	The GetAddressScaleAndIndex function returns the scale and index of an address operand by index.
		*	Params:
		*		opInfo - struct containing operand info
		*       scale - output, returns the operands scale.
		*  Result:
		*		true - if the operand can be interogated
		*		false - if the operand can't be interogated, (all excepting RIVER_OPTYPE_MEM)
		*/
		virtual bool GetAddressScaleAndIndex(struct OperandInfo &opInfo, nodep::BYTE &scale) = 0;


		/**
		*	The GetFlgValue function returns a flag by flag value.
		*	Params:
		*		flagInfo - struct containing flag info
		*  Result:
		*		true - if the flag can be interogated
		*		false - if the flag can't be interogated, (if the flag is not in the modFlags bitmask).
		*/
		virtual bool GetFlgValue(struct FlagInfo &flagInfo) = 0;

		/**
		*  The SetOperand function binds a symbolic expression to an operand.
		*  Params:
		*		opIdx - input, specifies the operand index.
		*		symbolicValue - input, symbolic expression to bind.
		*  Result:
		*		true - if the expression was bound successfully
		*		false - otherwise
		*/
		virtual bool SetOperand(nodep::BYTE opIdx, void *symbolicValue, bool doRefCount = true) = 0;

		/**
		*  The UnsetOperand unbinds the symbolic expression from the specified operand.
		*  Params:
		*		opIdx - input, specifies the operand index.
		*/
		virtual bool UnsetOperand(nodep::BYTE opIdx, bool doRefCount = true) = 0;

		/**
		*  The SetFlgValue function binds a symbolic expression to flag.
		*  Params:
		*		flg - input, one of the RIVER_SPEC_FLAG_?F constants.
		*		symbolicValue - input, symbolic expression to bind.
		*  Result:
		*		true - if the expression was bound successfully
		*		false - otherwise
		*/
		virtual void SetFlgValue(nodep::BYTE flg, void *symbolicValue, bool doRefCount = true) = 0;

		/**
		*  The UnsetFlgValue unbinds the symbolic expression from the specified operand.
		*  Params:
		*		flg - input, one of the RIVER_SPEC_FLAG_?F constants.
		*/
		virtual void UnsetFlgValue(nodep::BYTE flg, bool doRefCount = true) = 0;
	};

	class ScopedSymbolicEnvironment : public SymbolicEnvironment {
	protected :
		SymbolicEnvironment *subEnv;

		virtual void _SetReferenceCounting(AddRefFunc addRef, DecRefFunc decRef);
		virtual bool _SetCurrentInstruction(RiverInstruction *instruction, void *opBuffer);
		
		virtual void _PushState(stk::LargeStack &stack) = 0;
		virtual void _PopState(stk::LargeStack &stack) = 0;
	public :
		ScopedSymbolicEnvironment();

		virtual bool SetExecutor(SymbolicExecutor *e);

		virtual void PushState(stk::LargeStack &stack);
		virtual void PopState(stk::LargeStack &stack);

		virtual void SetSymbolicVariable(const char *name, rev::ADDR_TYPE addr, nodep::DWORD size);

		virtual bool SetSubEnvironment(SymbolicEnvironment *env);

		virtual void SetReferenceCounting(AddRefFunc addRef, DecRefFunc decRef);
		virtual bool SetCurrentInstruction(RiverInstruction *instruction, void *opBuffer);

		virtual bool GetOperand(struct OperandInfo &opInfo);
		virtual bool GetAddressBase(struct OperandInfo &opInfo);
		virtual bool GetAddressScaleAndIndex(struct OperandInfo &opInfo, nodep::BYTE &scale);
		virtual bool GetFlgValue(struct FlagInfo &flagInfo);
		virtual bool SetOperand(nodep::BYTE opIdx, void *symbolicValue, bool doRefCount);
		virtual bool UnsetOperand(nodep::BYTE opIdx, bool doRefCount);
		virtual void SetFlgValue(nodep::BYTE flg, void *symbolicValue, bool doRefCount);
		virtual void UnsetFlgValue(nodep::BYTE flg, bool doRefCount);
	};

	/** Executor class. Inherit this class to provide bindings to a SMT solver. */
	class SymbolicExecutor {
	public:
		int mCount;
		ModuleInfo *mInfo;
		SymbolicEnvironment *env;
	public:

		SymbolicExecutor(SymbolicEnvironment *e)
			: mCount(0), mInfo(nullptr), env(e)
		{}

		// Create a new symbolic variable
		virtual void *CreateVariable(const char *name, nodep::DWORD size) = 0;

		// Make a new constant
		virtual void *MakeConst(nodep::DWORD value, nodep::DWORD bits) = 0;

		// Extract bits from an expression
		virtual void *ExtractBits(void *expr, nodep::DWORD lsb, nodep::DWORD size) = 0;

		virtual void *ConcatBits(void *expr1, void *expr2) = 0;

		// Access the symbolic environment through env->* methods
		// The environment might be subject to change as more features are added
		virtual void Execute(RiverInstruction *instruction) = 0;
		void SetModuleData(int mCount, ModuleInfo *mInfo);
	};

}; // namespace sym

#endif
