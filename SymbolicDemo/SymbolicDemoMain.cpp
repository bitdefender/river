#include <Windows.h>
#include <stdio.h>

#include "../revtracer/revtracer.h"
#include "../revtracer/SymbolicEnvironment.h"

#include "z3.h"

::HANDLE fDbg = ((::HANDLE)(::LONG_PTR)-1);

void DebugPrint(rev::DWORD printMask, const char *fmt, ...) {
	va_list va;
	char tmpBuff[512];

	static char lastChar = '\n';

	char pfxBuff[] = "[___|_____|___|_] ";

	const char messageTypes[][4] = {
		"___",
		"ERR",
		"INF",
		"DBG"
	};

	const char executionStages[][6] = {
		"_____",
		"BRHND",
		"DIASM",
		"TRANS",
		"REASM",
		"RUNTM",
		"INSPT",
		"CNTNR"
	};

	const char codeTypes[][4] = {
		"___",
		"NAT",
		"RIV",
		"TRK",
		"SYM"
	};

	const char codeDirections[] = {
		'_', 'F', 'B'
	};

	_snprintf_s(
		pfxBuff,
		sizeof(pfxBuff)-1,
		"[%3s|%5s|%3s|%c] ",
		messageTypes[(printMask & PRINT_MESSAGE_MASK) >> PRINT_MESSAGE_SHIFT],
		executionStages[(printMask & PRINT_EXECUTION_MASK) >> PRINT_EXECUTION_SHIFT],
		codeTypes[(printMask & PRINT_CODE_TYPE_MASK) >> PRINT_CODE_TYPE_SHIFT],
		codeDirections[(printMask & PRINT_CODE_DIRECTION_MASK) >> PRINT_CODE_DIRECTION_SHIFT]
		);

	va_start(va, fmt);
	int sz = _vsnprintf_s(tmpBuff, sizeof(tmpBuff)-1, fmt, va);
	va_end(va);

	unsigned long wr;
	if ('\n' == lastChar) {
		WriteFile(fDbg, pfxBuff, sizeof(pfxBuff) - 1, &wr, NULL);
	}
	WriteFile(fDbg, tmpBuff, sz * sizeof(tmpBuff[0]), &wr, NULL);
	lastChar = tmpBuff[sz - 1];
}

void *CreateVariable(void *ctx, const char *name);
void Execute(void *ctx, RiverInstruction *instruction);

SymbolicExecutorFuncs sFuncs = {
	CreateVariable,
	Execute,
	nullptr,
	nullptr
};

class MySymbolicExecutor : public SymbolicExecutor {
public:
	int symIndex;

	Z3_config config;
	Z3_context context;
	Z3_sort dwordSort, byteSort, bitSort;

	Z3_ast zero32, zeroFlag, oneFlag;

	Z3_ast lastCondition;

	Z3_solver solver;

	typedef void(MySymbolicExecutor::*SymbolicExecute)(RiverInstruction *instruction);

	static SymbolicExecute executeFuncs[2][0x100];

	MySymbolicExecutor(SymbolicEnvironment *e) : SymbolicExecutor(e, &sFuncs) {
		symIndex = 1;
		lastCondition = nullptr;

		config = Z3_mk_config();
		context = Z3_mk_context(config);
		Z3_open_log("Z3.log");

		dwordSort = Z3_mk_bv_sort(context, 32);
		byteSort = Z3_mk_bv_sort(context, 8);
		bitSort = Z3_mk_bv_sort(context, 1);

		zero32 = Z3_mk_int(context, 0, dwordSort);
		zeroFlag = Z3_mk_int(context, 0, bitSort);
		oneFlag = Z3_mk_int(context, 1, bitSort);

		solver = Z3_mk_solver(context);
	}

	void EvalZF(Z3_ast result) {
		env->SetFlgValue(
			RIVER_SPEC_FLAG_ZF,
			Z3_mk_ite(context,
				Z3_mk_eq(
					context,
					result,
					zero32
				),
				oneFlag,
				zeroFlag
			)
		);
	}

	void SymbolicExecuteUnk(RiverInstruction *instruction) {
		__asm int 3;
	}

	void SymbolicExecuteJz(RiverInstruction *instruction) {
		rev::BOOL isTracked;
		rev::BYTE concreteValue;
		void *symValue;

		env->GetFlgValue(RIVER_SPEC_FLAG_ZF, isTracked, concreteValue, symValue);

		Z3_ast cond = Z3_mk_eq(
			context,
			(1 == concreteValue) ? oneFlag : zeroFlag,
			(Z3_ast)symValue
		);

		//printf("; (assert %s)\n\n", Z3_ast_to_string(context, cond));
		
		//Z3_ast sCond = Z3_simplify(context, cond);
		lastCondition = Z3_simplify(context, cond);

		//printf("(assert %s)\n\n", Z3_ast_to_string(context, sCond));

		//Z3_solver_assert(context, solver, sCond);
	}

	void SymbolicExecuteJnz(RiverInstruction *instruction) {
		rev::BOOL isTracked;
		rev::BYTE concreteValue;
		void *symValue;

		env->GetFlgValue(RIVER_SPEC_FLAG_ZF, isTracked, concreteValue, symValue);

		Z3_ast cond = Z3_mk_eq(
			context,
			(1 == concreteValue) ? oneFlag : zeroFlag,
			(Z3_ast)symValue
		);

		//printf("; (assert %s)\n\n", Z3_ast_to_string(context, cond));

		//Z3_ast sCond = Z3_simplify(context, cond);
		lastCondition = Z3_simplify(context, cond);

		//printf("(assert %s)\n\n", Z3_ast_to_string(context, sCond));

		//Z3_solver_assert(context, solver, sCond);
	}

	void SymbolicExecuteXor(RiverInstruction *instruction) {
		rev::BOOL tr[2];
		rev::DWORD cv[2];
		void *sv[2];

		for (int i = 0; i < 2; ++i) {
			env->GetOperand(i, tr[i], cv[i], sv[i]);

			if (!tr[i]) {
				sv[i] = Z3_mk_int(context, cv[i], dwordSort);
			}
		}

		/*printf("%08x xor <%08x, %08x>, <%08x, %08x> ", 
			instruction->instructionAddress, 
			sv[0],
			cv[0],
			sv[1],
			cv[1]
		);*/

		if (sv[0] == sv[1]) {
			env->UnsetOperand(0);
			//printf("=> 00000000\n");
		} else {
			Z3_ast xor = Z3_mk_bvxor(context, (Z3_ast)sv[0], (Z3_ast)sv[1]);
			void *symValue = (void *)xor;

			env->SetOperand(0, symValue);
			//printf("=> %08x\n", symValue);
		}
	}

	void SymbolicExecuteAdd(RiverInstruction *instruction) {
		rev::BOOL tr[2];
		rev::DWORD cv[2];
		void *sv[2];

		for (int i = 0; i < 2; ++i) {
			env->GetOperand(i, tr[i], cv[i], sv[i]);

			if (!tr[i]) {
				sv[i] = Z3_mk_int(context, cv[i], dwordSort);
			}
		}

		Z3_ast add = Z3_mk_bvadd(context, (Z3_ast)sv[0], (Z3_ast)sv[1]);
		void *symValue = (void *)add;

		env->SetOperand(0, symValue);

		/*printf("%08x add <%08x, %08x>, <%08x, %08x> => %08x\n",
			instruction->instructionAddress,
			sv[0],
			cv[0],
			sv[1],
			cv[1],
			symValue
		);*/
	}

	void SymbolicExecuteTest(RiverInstruction *instruction) {
		rev::BOOL tr[2];
		rev::DWORD cv[2];
		void *sv[2];

		for (int i = 0; i < 2; ++i) {
			env->GetOperand(i, tr[i], cv[i], sv[i]);

			if (!tr[i]) {
				sv[i] = Z3_mk_int(context, cv[i], dwordSort);
			}
		}

		if (sv[0] == sv[1]) {
			env->UnsetFlgValue(RIVER_SPEC_FLAG_ZF);
		} else {
			Z3_ast sub = Z3_mk_bvand(context, (Z3_ast)sv[0], (Z3_ast)sv[1]);
			EvalZF(sub);
		}

		/*printf("%08x test <%08x, %08x>, <%08x, %08x>\n",
			instruction->instructionAddress,
			sv[0],
			cv[0],
			sv[1],
			cv[1]
		);*/
	}

	void SymbolicExecuteCmp(RiverInstruction *instruction) {
		rev::BOOL tr[2];
		rev::DWORD cv[2];
		void *sv[2];

		for (int i = 0; i < 2; ++i) {
			env->GetOperand(i, tr[i], cv[i], sv[i]);

			if (!tr[i]) {
				sv[i] = Z3_mk_int(context, cv[i], dwordSort);
			}
		}

		if (sv[0] == sv[1]) {
			env->UnsetFlgValue(RIVER_SPEC_FLAG_ZF);
		} else {
			Z3_ast sub = Z3_mk_bvsub(context, (Z3_ast)sv[0], (Z3_ast)sv[1]);
			EvalZF(sub);
		}

		/*printf("%08x cmp <%08x, %08x>, <%08x, %08x>\n",
			instruction->instructionAddress,
			sv[0],
			cv[0],
			sv[1],
			cv[1]
		);*/
	}

	void SymbolicExecuteMov(RiverInstruction *instruction) {
		// mov dest, addr

		rev::BOOL isTracked;
		rev::DWORD concreteValue;
		void *symValue;

		env->GetOperand(1, isTracked, concreteValue, symValue);

		if (isTracked) {
			//Z3_ast src; // we don't need it tho
			env->SetOperand(0, symValue);
		} else {
			env->UnsetOperand(0);
		}

		/*printf("%08x mov <?>, <%08x, %08x>\n",
			instruction->instructionAddress,
			symValue,
			concreteValue
		);*/
	}

	void SymbolicExecuteMovSx(RiverInstruction *instruction) {
		rev::BOOL isTracked;
		rev::DWORD concreteValue;
		void *symValue;

		env->GetOperand(1, isTracked, concreteValue, symValue);

		if (isTracked) {
			Z3_ast dst = Z3_mk_sign_ext(context, 24, Z3_mk_extract(context, 7, 0, (Z3_ast)symValue));
			//symValue = (void *)symValue;
			env->SetOperand(0, symValue);
		}
		else {
			env->UnsetOperand(0);
		}

		/*printf("%08x movsx <?>, <%08x, %08x>\n",
			instruction->instructionAddress,
			symValue,
			concreteValue
		);*/
	}

	void SymboliExecute0x83(RiverInstruction *instruction) {
		if (0x07 == instruction->subOpCode) {
			SymbolicExecuteCmp(instruction);
		}
	}

	void SymbolicExecuteDispatch(RiverInstruction *instruction) {
		// do some actual work
		rev::DWORD dwTable = (instruction->modifiers & RIVER_MODIFIER_EXT) ? 1 : 0;
		(this->*executeFuncs[dwTable][instruction->opCode])(instruction);
	}
};

MySymbolicExecutor::SymbolicExecute MySymbolicExecutor::executeFuncs[2][0x100] = {
	{
		/*0x00*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteAdd,
		/*0x04*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x08*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x0C*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0x10*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x14*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x18*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x1C*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0x20*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x24*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x28*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x2C*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0x30*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteXor,
		/*0x34*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x38*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x3C*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0x40*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x44*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x48*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x4C*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0x50*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x54*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x58*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x5C*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0x60*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x64*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x68*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x6C*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0x70*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x74*/ &MySymbolicExecutor::SymbolicExecuteJz, &MySymbolicExecutor::SymbolicExecuteJnz, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x78*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x7C*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0x80*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteCmp, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymboliExecute0x83,
		/*0x84*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteTest, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x88*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteMov, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteMov,
		/*0x8C*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0x90*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x94*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x98*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x9C*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0xA0*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteMov, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xA4*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xA8*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xAC*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0xB0*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xB4*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xB8*/ &MySymbolicExecutor::SymbolicExecuteMov, &MySymbolicExecutor::SymbolicExecuteMov, &MySymbolicExecutor::SymbolicExecuteMov, &MySymbolicExecutor::SymbolicExecuteMov,
		/*0xBC*/ &MySymbolicExecutor::SymbolicExecuteMov, &MySymbolicExecutor::SymbolicExecuteMov, &MySymbolicExecutor::SymbolicExecuteMov, &MySymbolicExecutor::SymbolicExecuteMov,

		/*0xC0*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xC4*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteMov,
		/*0xC8*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xCC*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0xD0*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xD4*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xD8*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xDC*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0xE0*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xE4*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xE8*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xEC*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0xF0*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xF4*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xF8*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xFC*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk
	}, {
		/*0x00*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x04*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x08*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x0C*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0x10*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x14*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x18*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x1C*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0x20*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x24*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x28*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x2C*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0x30*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x34*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x38*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x3C*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0x40*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x44*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x48*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x4C*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0x50*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x54*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x58*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x5C*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0x60*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x64*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x68*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x6C*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0x70*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x74*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x78*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x7C*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0x80*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x84*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x88*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x8C*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0x90*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x94*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x98*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0x9C*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0xA0*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xA4*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xA8*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xAC*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0xB0*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xB4*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xB8*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xBC*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteMovSx, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0xC0*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xC4*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xC8*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xCC*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0xD0*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xD4*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xD8*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xDC*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0xE0*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xE4*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xE8*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xEC*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,

		/*0xF0*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xF4*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xF8*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk,
		/*0xFC*/ &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk, &MySymbolicExecutor::SymbolicExecuteUnk
	}
};


void *CreateVariable(void *ctx, const char *name) {
	MySymbolicExecutor *_this = (MySymbolicExecutor *)ctx;

	Z3_symbol s = Z3_mk_string_symbol(_this->context, name);
	Z3_ast ret = Z3_mk_const(_this->context, s, _this->dwordSort);

	Z3_ast cond1 = Z3_mk_bvule(
		_this->context,
		Z3_mk_int(_this->context, 0x61, _this->dwordSort),
		ret
	);

	Z3_solver_assert(_this->context, _this->solver, cond1);
	printf("(assert %s)\n\n", Z3_ast_to_string(_this->context, cond1));

	Z3_ast cond2 = Z3_mk_bvuge(
		_this->context,
		Z3_mk_int(_this->context, 0x7a, _this->dwordSort),
		ret
	);

	Z3_solver_assert(_this->context, _this->solver, cond2);
	printf("(assert %s)\n\n", Z3_ast_to_string(_this->context, cond2));

	_this->symIndex++;
	return ret; //(void *)(_this->symIndex - 1);
}

void Execute(void *ctx, RiverInstruction *instruction) {
	MySymbolicExecutor *_this = (MySymbolicExecutor *)ctx;
	/*rev::BOOL isTrackedOp[4], isTrackedFlg[7];
	rev::DWORD concreteValuesOp[4];
	rev::BYTE concreteValuesFlg[7];
	void *symbolicValuesOp[4], *symbolicValuesFlg[7];*/

	/*for (rev::DWORD i = 0; i < 7; i++) {
		if ((1 << i) & instruction->testFlags) {
			_this->env->GetFlgValue((1 << i), isTrackedFlg[i], concreteValuesFlg[i], symbolicValuesFlg[i]);
		}
	}


	for (int i = 0; i < 4; ++i) {
		_this->env->GetOperand(i, isTrackedOp[i], concreteValuesOp[i], symbolicValuesOp[i]);
	}*/

	_this->SymbolicExecuteDispatch(instruction);

	/*for (int i = 0; i < 4; ++i) {
		if (RIVER_SPEC_MODIFIES_OP(i) & instruction->specifiers) {
			void *tmp = (void *)_this->symIndex;
			_this->env->SetOperand(i, tmp);
			_this->symIndex++;
		}
	}

	for (rev::BYTE i = 0; i < 7; i++) {
		if ((1 << i) & instruction->modFlags) {
			void *tmp = (void *)_this->symIndex;
			_this->env->SetFlgValue(i, tmp);
			_this->symIndex++;
		}
	}*/
}

MySymbolicExecutor *executor = NULL;

rev::DWORD CustomExecutionController(void *ctx, rev::ADDR_TYPE addr, void *cbCtx) {
	rev::DWORD ret = EXECUTION_ADVANCE;

	if (nullptr != executor->lastCondition) {

		// revert last condition
		if (0x1769 == ((rev::DWORD)addr & 0xFFFF)) {
			executor->lastCondition = Z3_mk_not(
				executor->context,
				executor->lastCondition
			);
		}
		
		Z3_solver_assert(executor->context, executor->solver, executor->lastCondition);

		printf("(assert %s)\n\n", Z3_ast_to_string(executor->context, executor->lastCondition));

		executor->lastCondition = nullptr;
	}

	return ret;
}

SymbolicExecutor *SymExeConstructor(SymbolicEnvironment *env) {
	executor = new MySymbolicExecutor(env);
	return executor;
}

void InitializeRevtracer(rev::ADDR_TYPE entryPoint) {
	HMODULE hNtDll = GetModuleHandle(L"ntdll.dll");
	rev::RevtracerAPI *api = &rev::revtracerAPI;

	api->dbgPrintFunc = DebugPrint;

	api->lowLevel.ntAllocateVirtualMemory = GetProcAddress(hNtDll, "NtAllocateVirtualMemory");
	api->lowLevel.ntFreeVirtualMemory = GetProcAddress(hNtDll, "NtFreeVirtualMemory");

	api->lowLevel.ntQueryInformationThread = GetProcAddress(hNtDll, "NtQueryInformationThread");
	api->lowLevel.ntTerminateProcess = GetProcAddress(hNtDll, "NtTerminateProcess");

	api->lowLevel.rtlNtStatusToDosError = GetProcAddress(hNtDll, "RtlNtStatusToDosError");
	api->lowLevel.vsnprintf_s = GetProcAddress(hNtDll, "_vsnprintf_s");

	api->executionControl = CustomExecutionController;


	rev::RevtracerConfig *config = &rev::revtracerConfig;
	config->contextSize = 4;
	config->entryPoint = entryPoint;
	config->featureFlags = TRACER_FEATURE_SYMBOLIC | TRACER_FEATURE_REVERSIBLE;
	config->sCons = SymExeConstructor;

	rev::Initialize();
}



// here are the bindings to the payload
extern "C" unsigned int buffer[];
int Payload();
int Check(unsigned int *ptr);

int main(int argc, char *argv[]) {
	
	unsigned int actualPass[10] = { 0 };
	HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
	//TmpPrint("ntdll.dll @ 0x%08x\n", (DWORD)hNtDll);

	Payload();

	fDbg = CreateFileA("symb.log", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);

	InitializeRevtracer((rev::ADDR_TYPE)(&Payload));
	rev::MarkMemoryName((rev::ADDR_TYPE)(&buffer[0]), "a[0]");
	rev::MarkMemoryName((rev::ADDR_TYPE)(&buffer[1]), "a[1]");
	rev::MarkMemoryName((rev::ADDR_TYPE)(&buffer[2]), "a[2]");
	rev::MarkMemoryName((rev::ADDR_TYPE)(&buffer[3]), "a[3]");
	rev::MarkMemoryName((rev::ADDR_TYPE)(&buffer[4]), "a[4]");
	//rev::MarkMemoryName((rev::ADDR_TYPE)(&buffer[5]), "a[5]");
	//rev::MarkMemoryName((rev::ADDR_TYPE)(&buffer[6]), "a[6]");
	//rev::MarkMemoryName((rev::ADDR_TYPE)(&buffer[7]), "a[7]");

	rev::Execute(argc, argv);

	Z3_lbool ret = Z3_solver_check(executor->context, executor->solver);

	if (Z3_L_TRUE == ret) {
		Z3_model model = Z3_solver_get_model(executor->context, executor->solver);
		
		//printf("%s\n", Z3_model_to_string(executor->context, model));

		unsigned int cnt = Z3_model_get_num_consts(executor->context, model);

		for (unsigned int i = 0; i < cnt; ++i) {
			Z3_func_decl c = Z3_model_get_const_decl(executor->context, model, i);

			printf("%s\n", Z3_func_decl_to_string(executor->context, c));

			Z3_ast ast = Z3_func_decl_to_ast(executor->context, c);
			
			Z3_symbol sName = Z3_get_decl_name(executor->context, c);

			Z3_string r = Z3_get_symbol_string(executor->context, sName);

			Z3_ast val;
			Z3_bool pEval = Z3_eval_func_decl(
				executor->context,
				model,
				c,
				&val
			);

			rev::DWORD ret;
			Z3_bool p = Z3_get_numeral_uint(executor->context, val, (unsigned int *)&ret);

			actualPass[r[2] - '0'] = ret;
		}

		printf("Check(\"");
		for (int i = 0; actualPass[i]; ++i) {
			printf("%c", actualPass[i]);
		}
		printf("\") = %04x\n", Check(actualPass));
	}
	else if (Z3_L_FALSE == ret) {
		printf("unsat\n");
	}
	else { // undef
		printf("undef\n");
	}

	Z3_close_log();

	CloseHandle(fDbg);
	system("pause");
	return 0;
}
