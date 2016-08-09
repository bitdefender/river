#include "Z3SymbolicExecutor.h"

#include "TrackingCookie.h"

void *CreateVariable(void *ctx, const char *name);
void Execute(void *ctx, RiverInstruction *instruction);

rev::SymbolicExecutorFuncs sFuncs = {
	CreateVariable,
	Execute,
	nullptr,
	nullptr
};

void *CreateVariable(void *ctx, const char *name) {
	Z3SymbolicExecutor *_this = (Z3SymbolicExecutor *)ctx;

	Z3_symbol s = Z3_mk_string_symbol(_this->context, name);
	Z3_ast ret = Z3_mk_const(_this->context, s, _this->dwordSort);

	Z3_set_user_ptr(_this->context, ret, _this->cookieFuncs->allocCookie(), (void *)_this->cookieFuncs->freeCookie);

	/*VariableTracker<Z3_ast> *ret = new VariableTracker<Z3_ast>(
	Z3_mk_const(_this->context, s, _this->dwordSort)
	);*/

	Z3_ast l1[2] = {
		Z3_mk_bvule(
			_this->context,
			Z3_mk_int(_this->context, 'a', _this->dwordSort),
			ret
		),
		Z3_mk_bvuge(
			_this->context,
			Z3_mk_int(_this->context, 'z', _this->dwordSort),
			ret
		)
	};

	Z3_ast l2[2] = {
		Z3_mk_eq(
			_this->context,
			_this->zero32,
			ret
		),
		Z3_mk_and(
			_this->context,
			2,
			l1
		)
	};

	Z3_ast cond = Z3_mk_or(
		_this->context,
		2,
		l2
	);

	Z3_solver_assert(_this->context, _this->solver, cond);
	printf("(assert %s)\n\n", Z3_ast_to_string(_this->context, cond));

	_this->symIndex++;
	return ret;
}

void Execute(void *ctx, RiverInstruction *instruction) {
	Z3SymbolicExecutor *_this = (Z3SymbolicExecutor *)ctx;
	_this->SymbolicExecuteDispatch(instruction);
}

bool IsLocked(void *ctx, const Z3_ast *ast) {
	void *v = Z3_get_user_ptr((Z3_context)ctx, *ast);
	const TrackedVariableData *tvd = (const TrackedVariableData *)v;
	return tvd->isLocked;
}

void Lock(void *ctx, Z3_ast *ast) {
	void *v = Z3_get_user_ptr((Z3_context)ctx, *ast);
	TrackedVariableData *tvd = (TrackedVariableData *)v;
	tvd->isLocked = true;
}

void Unlock(void *ctx, Z3_ast *ast) {
	void *v = Z3_get_user_ptr((Z3_context)ctx, *ast);
	TrackedVariableData *tvd = (TrackedVariableData *)v;
	tvd->isLocked = false;
}

const unsigned int Z3SymbolicExecutor::Z3SymbolicCpuFlag::lazyMarker = 0xDEADBEEF;

Z3SymbolicExecutor::Z3SymbolicExecutor(rev::SymbolicEnvironment *e, TrackingCookieFuncs *f) :
		SymbolicExecutor(e, &sFuncs), 
		variableTracker((void *)this, ::IsLocked, ::Lock, ::Unlock) 
{
	symIndex = 1;

	lazyFlags[0] = new Z3FlagCF();
	lazyFlags[1] = new Z3FlagPF();
	lazyFlags[2] = new Z3FlagAF();
	lazyFlags[3] = new Z3FlagZF();
	lazyFlags[4] = new Z3FlagSF();
	lazyFlags[5] = new Z3FlagOF();
	lazyFlags[6] = new Z3FlagDF();

	for (int i = 0; i < 7; ++i) {
		lazyFlags[i]->SetParent(this);
	}
	
	lastCondition = nullptr;

	cookieFuncs = f;

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
	Z3_solver_inc_ref(context, solver);

	ls = new stk::LargeStack(saveStack, sizeof(saveStack), &saveTop, "flagStack.bin");
}

Z3SymbolicExecutor::~Z3SymbolicExecutor() {
	delete ls;

	Z3_solver_dec_ref(context, solver);

	for (int i = 0; i < 7; ++i) {
		delete lazyFlags[i];
	}
}

/*void Z3SymbolicExecutor::EvalZF(Z3_ast result) {
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
}*/

void Z3SymbolicExecutor::StepForward() {
	variableTracker.Forward();
	Z3_solver_push(context, solver);
	for (int i = 0; i < 7; ++i) {
		lazyFlags[i]->SaveState(*ls);
	}

	//printf("Solver push\n");
}

void Z3SymbolicExecutor::StepBackward() {
	for (int i = 6; i >= 0; --i) {
		lazyFlags[i]->LoadState(*ls);
	}

	Z3_solver_pop(context, solver, 1);
	variableTracker.Backward();
	//printf("Solver pop\n");
}

void Z3SymbolicExecutor::Lock(Z3_ast t) {
	variableTracker.Lock(&t);
}

void Z3SymbolicExecutor::SymbolicExecuteUnk(RiverInstruction *instruction, SymbolicOperands *ops) {
	__asm int 3;
}

template <unsigned int flag> void Z3SymbolicExecutor::SymbolicExecuteJCC(RiverInstruction *instruction, SymbolicOperands *ops) {
	Z3_ast cond = Z3_mk_eq(
		context,
		(1 == ops->cvf[flag]) ? oneFlag : zeroFlag,
		(Z3_ast)ops->svf[flag]
	);

	lastCondition = Z3_simplify(context, cond);
}

Z3_ast Z3SymbolicExecutor::ExecuteAdd(Z3_ast o1, Z3_ast o2) {
	return Z3_mk_bvadd(context, o1, o2);
}

Z3_ast Z3SymbolicExecutor::ExecuteOr(Z3_ast o1, Z3_ast o2) {
	return Z3_mk_bvor(context, o1, o2);
}

Z3_ast Z3SymbolicExecutor::ExecuteAdc(Z3_ast o1, Z3_ast o2) {
	//return Z3_mk_bvadd(context, o1, o2);
	return nullptr;
}

Z3_ast Z3SymbolicExecutor::ExecuteSbb(Z3_ast o1, Z3_ast o2) {
	//return Z3_mk_bvor(context, o1, o2);
	return nullptr;
}

Z3_ast Z3SymbolicExecutor::ExecuteAnd(Z3_ast o1, Z3_ast o2) {
	return Z3_mk_bvand(context, o1, o2);
}

Z3_ast Z3SymbolicExecutor::ExecuteSub(Z3_ast o1, Z3_ast o2) {
	return Z3_mk_bvsub(context, o1, o2);
}

Z3_ast Z3SymbolicExecutor::ExecuteXor(Z3_ast o1, Z3_ast o2) {
	if (o1 == o2) {
		return nullptr;
	}

	return Z3_mk_bvxor(context, o1, o2);
}

void Z3SymbolicExecutor::SymbolicExecuteTest(RiverInstruction *instruction, SymbolicOperands *ops) {
	if (ops->sv[0] == ops->sv[1]) {
		env->UnsetFlgValue(RIVER_SPEC_FLAG_ZF);
	}
	else {
		Z3_ast sub = Z3_mk_bvand(context, (Z3_ast)ops->sv[0], (Z3_ast)ops->sv[1]);
		//EvalZF(sub);

		lazyFlags[RIVER_SPEC_IDX_ZF]->SetSource(sub, nullptr, nullptr, nullptr, 0);
		env->SetFlgValue(RIVER_SPEC_FLAG_ZF, lazyFlags[RIVER_SPEC_IDX_ZF]);
	}
}

void Z3SymbolicExecutor::SymbolicExecuteCmp(RiverInstruction *instruction, SymbolicOperands *ops) {
	if (ops->sv[0] == ops->sv[1]) {
		env->UnsetFlgValue(RIVER_SPEC_FLAG_ZF);
	}
	else {
		Z3_ast sub = Z3_mk_bvsub(context, (Z3_ast)ops->sv[0], (Z3_ast)ops->sv[1]);

		lazyFlags[RIVER_SPEC_IDX_ZF]->SetSource(sub, nullptr, nullptr, nullptr, 0);
		env->SetFlgValue(RIVER_SPEC_FLAG_ZF, lazyFlags[RIVER_SPEC_IDX_ZF]);
	}
}

void Z3SymbolicExecutor::SymbolicExecuteMov(RiverInstruction *instruction, SymbolicOperands *ops) {
	// mov dest, addr
	if (ops->tr[1]) {
		env->SetOperand(0, ops->sv[1]);
	} else {
		env->UnsetOperand(0);
	}
}

void Z3SymbolicExecutor::SymbolicExecuteMovSx(RiverInstruction *instruction, SymbolicOperands *ops) {
	if (ops->tr[1]) {
		Z3_ast dst = Z3_mk_sign_ext(context, 24, Z3_mk_extract(context, 7, 0, (Z3_ast)ops->sv[1]));
		env->SetOperand(0, (void *)dst);
	}
	else {
		env->UnsetOperand(0);
	}
}

void Z3SymbolicExecutor::SymboliExecute0x83(RiverInstruction *instruction, SymbolicOperands *ops) {
	if (0x07 == instruction->subOpCode) {
		SymbolicExecuteCmp(instruction, ops);
	}
}

template <Z3SymbolicExecutor::IntegerFunc func, unsigned int funcCode> void Z3SymbolicExecutor::SymbolicExecuteInteger(RiverInstruction *instruction, SymbolicOperands *ops) {
	Z3_ast ret = (this->*func)((Z3_ast)ops->sv[0], (Z3_ast)ops->sv[1]); //Z3_mk_bvadd(context, (Z3_ast)sv[0], (Z3_ast)sv[1]);
	void *symValue = (void *)ret;

	env->SetOperand(0, symValue);

	for (int i = 0; i < 7; ++i) {
		if ((1 << i) & instruction->modFlags) {
			lazyFlags[i]->SetSource(ret, (Z3_ast)ops->sv[0], (Z3_ast)ops->sv[1], nullptr, funcCode);
			env->SetFlgValue(1 << i, lazyFlags[i]);
		}
	}
}

void Z3SymbolicExecutor::GetSymbolicValues(SymbolicOperands *ops, rev::DWORD mask) {
	static const unsigned char flagList[] = {
		RIVER_SPEC_FLAG_CF,
		RIVER_SPEC_FLAG_PF,
		RIVER_SPEC_FLAG_AF,
		RIVER_SPEC_FLAG_ZF,
		RIVER_SPEC_FLAG_SF,
		RIVER_SPEC_FLAG_OF
	};

	static const int flagCount = sizeof(flagList) / sizeof(flagList[0]);
	rev::DWORD m = mask & ops->av;

	for (int i = 0; i < 4; ++i) {
		if ((OPERAND_BITMASK(i) & m) && !ops->tr[i]) {
			ops->sv[i] = Z3_mk_int(context, ops->cv[i], dwordSort);
		}
	}

	for (int i = 0; i < flagCount; ++i) {
		if (flagList[i] & m) {
			if (!ops->trf[i]) {
				ops->svf[i] = Z3_mk_int(context, ops->cvf[i], bitSort);
			} else {
				ops->svf[i] = ((Z3SymbolicCpuFlag *)ops->svf[i])->GetValue();
			}
		}

		if ((flagList[i] & m) && !ops->trf[i]) {
			ops->svf[i] = Z3_mk_int(context, ops->cvf[i], bitSort);
		}
	}
}

void Z3SymbolicExecutor::SymbolicExecuteDispatch(RiverInstruction *instruction) {
	static const unsigned char flagList[] = {
		RIVER_SPEC_FLAG_CF,
		RIVER_SPEC_FLAG_PF,
		RIVER_SPEC_FLAG_AF,
		RIVER_SPEC_FLAG_ZF,
		RIVER_SPEC_FLAG_SF,
		RIVER_SPEC_FLAG_OF
	};

	static const int flagCount = sizeof(flagList) / sizeof(flagList[0]);

	SymbolicOperands ops;

	ops.av = 0;
	rev::BOOL uo[4], uof[flagCount];
	rev::BOOL isSymb = false;

	for (int i = 0; i < 4; ++i) {
		ops.tr[i] = false;
		if (true == (uo[i] = env->GetOperand(i, ops.tr[i], ops.cv[i], ops.sv[i]))) {
			ops.av |= OPERAND_BITMASK(i);
			isSymb |= ops.tr[i];
		}
	}

	for (int i = 0; i < flagCount; ++i) {
		ops.trf[i] = false;
		if (true == (uof[i] = env->GetFlgValue(flagList[i], ops.trf[i], ops.cvf[i], ops.svf[i]))) {
			ops.av |= flagList[i];
			isSymb |= ops.trf[i];
		}
	}

	if (isSymb) {
		// This functionality must be moved into individual functions if the need arises
		GetSymbolicValues(&ops, ops.av);

		rev::DWORD dwTable = (instruction->modifiers & RIVER_MODIFIER_EXT) ? 1 : 0;
		(this->*executeFuncs[dwTable][instruction->opCode])(instruction, &ops);
	} else {
		// unset all modified operands
		for (int i = 0; i < 4; ++i) {
			if (RIVER_SPEC_MODIFIES_OP(i) & instruction->specifiers) {
				env->UnsetOperand(i);
			}
		}

		// unset all modified flags
		for (int i = 0; i < flagCount; ++i) {
			if (flagList[i] & instruction->modFlags) {
				env->UnsetFlgValue(flagList[i]);
			}
		}
	}
}


Z3SymbolicExecutor::SymbolicExecute Z3SymbolicExecutor::executeFuncs[2][0x100] = {
	{
		/*0x00*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteAdd, Z3_FLAG_OP_ADD>,
		/*0x04*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x08*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x0C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x10*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x14*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x18*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x1C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x20*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x24*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x28*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x2C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x30*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteXor, Z3_FLAG_OP_XOR>,
		/*0x34*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x38*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x3C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x40*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x44*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x48*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x4C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x50*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x54*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x58*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x5C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x60*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x64*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x68*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x6C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x70*/ &Z3SymbolicExecutor::SymbolicExecuteJCC<RIVER_SPEC_IDX_OF>, &Z3SymbolicExecutor::SymbolicExecuteJCC<RIVER_SPEC_IDX_OF>, &Z3SymbolicExecutor::SymbolicExecuteJCC<RIVER_SPEC_IDX_CF>, &Z3SymbolicExecutor::SymbolicExecuteJCC<RIVER_SPEC_IDX_CF>,
		/*0x74*/ &Z3SymbolicExecutor::SymbolicExecuteJCC<RIVER_SPEC_IDX_ZF>, &Z3SymbolicExecutor::SymbolicExecuteJCC<RIVER_SPEC_IDX_ZF>, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x78*/ &Z3SymbolicExecutor::SymbolicExecuteJCC<RIVER_SPEC_IDX_SF>, &Z3SymbolicExecutor::SymbolicExecuteJCC<RIVER_SPEC_IDX_SF>, &Z3SymbolicExecutor::SymbolicExecuteJCC<RIVER_SPEC_IDX_PF>, &Z3SymbolicExecutor::SymbolicExecuteJCC<RIVER_SPEC_IDX_PF>,
		/*0x7C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x80*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteCmp, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymboliExecute0x83,
		/*0x84*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteTest, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x88*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteMov,
		/*0x8C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x90*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x94*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x98*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x9C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0xA0*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xA4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xA8*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xAC*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0xB0*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xB4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xB8*/ &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov,
		/*0xBC*/ &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov,

		/*0xC0*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xC4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteMov,
		/*0xC8*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xCC*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0xD0*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xD4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xD8*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xDC*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0xE0*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xE4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xE8*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xEC*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0xF0*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xF4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xF8*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xFC*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk
	},{
		/*0x00*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x04*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x08*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x0C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x10*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x14*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x18*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x1C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x20*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x24*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x28*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x2C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x30*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x34*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x38*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x3C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x40*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x44*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x48*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x4C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x50*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x54*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x58*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x5C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x60*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x64*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x68*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x6C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x70*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x74*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x78*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x7C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x80*/ &Z3SymbolicExecutor::SymbolicExecuteJCC<RIVER_SPEC_IDX_OF>, &Z3SymbolicExecutor::SymbolicExecuteJCC<RIVER_SPEC_IDX_OF>, &Z3SymbolicExecutor::SymbolicExecuteJCC<RIVER_SPEC_IDX_CF>, &Z3SymbolicExecutor::SymbolicExecuteJCC<RIVER_SPEC_IDX_CF>,
		/*0x84*/ &Z3SymbolicExecutor::SymbolicExecuteJCC<RIVER_SPEC_IDX_ZF>, &Z3SymbolicExecutor::SymbolicExecuteJCC<RIVER_SPEC_IDX_ZF>, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x88*/ &Z3SymbolicExecutor::SymbolicExecuteJCC<RIVER_SPEC_IDX_SF>, &Z3SymbolicExecutor::SymbolicExecuteJCC<RIVER_SPEC_IDX_SF>, &Z3SymbolicExecutor::SymbolicExecuteJCC<RIVER_SPEC_IDX_PF>, &Z3SymbolicExecutor::SymbolicExecuteJCC<RIVER_SPEC_IDX_PF>,
		/*0x8C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x90*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x94*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x98*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x9C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0xA0*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xA4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xA8*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xAC*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0xB0*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xB4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xB8*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xBC*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteMovSx, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0xC0*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xC4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xC8*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xCC*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0xD0*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xD4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xD8*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xDC*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0xE0*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xE4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xE8*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xEC*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0xF0*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xF4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xF8*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xFC*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk
	}
};
