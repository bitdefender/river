#include "Z3SymbolicExecutor.h"

#include "TrackingCookie.h"

void *CreateVariable(void *ctx, const char *name);
void Execute(void *ctx, RiverInstruction *instruction);

SymbolicExecutorFuncs sFuncs = {
	CreateVariable,
	Execute,
	nullptr,
	nullptr
};

void *CreateVariable(void *ctx, const char *name) {
	Z3SymbolicExecutor *_this = (Z3SymbolicExecutor *)ctx;

	Z3_symbol s = Z3_mk_string_symbol(_this->context, name);
	Z3_ast ret = Z3_mk_const(_this->context, s, _this->dwordSort);

	Z3_set_user_ptr(_this->context, ret, _this->cookieFuncs->allocCookie(), (void *)_this->cookieFuncs->allocCookie);

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


Z3SymbolicExecutor::Z3SymbolicExecutor(SymbolicEnvironment *e, TrackingCookieFuncs *f) :
		SymbolicExecutor(e, &sFuncs), 
		variableTracker((void *)this, ::IsLocked, ::Lock, ::Unlock) 
{
	symIndex = 1;
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

}

void Z3SymbolicExecutor::EvalZF(Z3_ast result) {
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

void Z3SymbolicExecutor::StepForward() {
	variableTracker.Forward();
	Z3_solver_push(context, solver);
}

void Z3SymbolicExecutor::StepBackward() {
	Z3_solver_pop(context, solver, 1);
	variableTracker.Backward();
}

void Z3SymbolicExecutor::Lock(Z3_ast t) {
	variableTracker.Lock(&t);
}

void Z3SymbolicExecutor::SymbolicExecuteUnk(RiverInstruction *instruction) {
	__asm int 3;
}

void Z3SymbolicExecutor::SymbolicExecuteJz(RiverInstruction *instruction) {
	rev::BOOL isTracked;
	rev::BYTE concreteValue;
	void *symValue;

	env->GetFlgValue(RIVER_SPEC_FLAG_ZF, isTracked, concreteValue, symValue);

	Z3_ast cond = Z3_mk_eq(
		context,
		(1 == concreteValue) ? oneFlag : zeroFlag,
		(Z3_ast)symValue
	);

	lastCondition = Z3_simplify(context, cond);
}

void Z3SymbolicExecutor::SymbolicExecuteJnz(RiverInstruction *instruction) {
	rev::BOOL isTracked;
	rev::BYTE concreteValue;
	void *symValue;

	env->GetFlgValue(RIVER_SPEC_FLAG_ZF, isTracked, concreteValue, symValue);

	Z3_ast cond = Z3_mk_eq(
		context,
		(1 == concreteValue) ? oneFlag : zeroFlag,
		(Z3_ast)symValue
	);

	lastCondition = Z3_simplify(context, cond);
}

void Z3SymbolicExecutor::SymbolicExecuteXor(RiverInstruction *instruction) {
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
	}
	else {
		Z3_ast xor = Z3_mk_bvxor(context, (Z3_ast)sv[0], (Z3_ast)sv[1]);
		void *symValue = (void *) xor ;

		env->SetOperand(0, symValue);
		//printf("=> %08x\n", symValue);
	}
}

void Z3SymbolicExecutor::SymbolicExecuteAdd(RiverInstruction *instruction) {
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

void Z3SymbolicExecutor::SymbolicExecuteTest(RiverInstruction *instruction) {
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
	}
	else {
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

void Z3SymbolicExecutor::SymbolicExecuteCmp(RiverInstruction *instruction) {
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
	}
	else {
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

void Z3SymbolicExecutor::SymbolicExecuteMov(RiverInstruction *instruction) {
	// mov dest, addr

	rev::BOOL isTracked;
	rev::DWORD concreteValue;
	void *symValue;

	env->GetOperand(1, isTracked, concreteValue, symValue);

	if (isTracked) {
		env->SetOperand(0, symValue);
	}
	else {
		env->UnsetOperand(0);
	}

	/*printf("%08x mov <?>, <%08x, %08x>\n",
	instruction->instructionAddress,
	symValue,
	concreteValue
	);*/
}

void Z3SymbolicExecutor::SymbolicExecuteMovSx(RiverInstruction *instruction) {
	rev::BOOL isTracked;
	rev::DWORD concreteValue;
	void *symValue;

	env->GetOperand(1, isTracked, concreteValue, symValue);

	if (isTracked) {
		Z3_ast dst = Z3_mk_sign_ext(context, 24, Z3_mk_extract(context, 7, 0, (Z3_ast)symValue));
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

void Z3SymbolicExecutor::SymboliExecute0x83(RiverInstruction *instruction) {
	if (0x07 == instruction->subOpCode) {
		SymbolicExecuteCmp(instruction);
	}
}

void Z3SymbolicExecutor::SymbolicExecuteDispatch(RiverInstruction *instruction) {
	// do some actual work
	rev::DWORD dwTable = (instruction->modifiers & RIVER_MODIFIER_EXT) ? 1 : 0;
	(this->*executeFuncs[dwTable][instruction->opCode])(instruction);
}


Z3SymbolicExecutor::SymbolicExecute Z3SymbolicExecutor::executeFuncs[2][0x100] = {
	{
		/*0x00*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteAdd,
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

		/*0x30*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteXor,
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
		/*0x74*/ &Z3SymbolicExecutor::SymbolicExecuteJz, &Z3SymbolicExecutor::SymbolicExecuteJnz, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x78*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
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

		/*0x80*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x84*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x88*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
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
