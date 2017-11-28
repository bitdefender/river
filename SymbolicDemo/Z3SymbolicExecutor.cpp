#include "Z3SymbolicExecutor.h"

#include "../CommonCrossPlatform/Common.h"

void *Z3SymbolicExecutor::CreateVariable(const char *name, nodep::DWORD size) {
	Z3_symbol s = Z3_mk_string_symbol(context, name);
	Z3_sort srt;
	Z3_ast zro;

	switch (size) {
	case 1 :
		srt = byteSort;
		zro = zero8;
		break;
	case 2 :
		srt = wordSort;
		zro = zero16;
		break;
	case 4 :
		srt = dwordSort;
		zro = zero32;
		break;
	default :
		return nullptr;
	};

	Z3_ast ret = Z3_mk_const(context, s, srt);

	Z3_ast l1[2] = {
		Z3_mk_bvule(
			context,
			Z3_mk_int(context, 'a', srt),
			ret
		),
		Z3_mk_bvuge(
			context,
			Z3_mk_int(context, 'z', srt),
			ret
		)
	};

	Z3_ast l2[2] = {
		Z3_mk_eq(
			context,
			zro,
			ret
		),
		Z3_mk_and(
			context,
			2,
			l1
		)
	};

	Z3_ast cond = Z3_mk_or(
		context,
		2,
		l2
	);

	Z3_solver_assert(context, solver, cond);
	printf("(assert %s)\n\n", Z3_ast_to_string(context, cond));

	symIndex++;

	printf("<sym> var %p <= %s\n", ret, name);
	return ret;
}

void *Z3SymbolicExecutor::MakeConst(nodep::DWORD value, nodep::DWORD bits) {
	Z3_sort type;

	switch (bits) {
		case 8:
			type = byteSort;
			break;
		case 16:
			type = wordSort;
			break;
		case 24: // only for unaligned mem acceses
			type = tbSort;
			break;
		case 32:
			type = dwordSort;
			break;
		default :
			DEBUG_BREAK;
	}

	Z3_ast ret = Z3_mk_int(
		context,
		value,
		type
	);

	printf("<sym> const %p <= %08lx\n", ret, value);

	return (void *)ret;
}

void *Z3SymbolicExecutor::ExtractBits(void *expr, nodep::DWORD lsb, nodep::DWORD size) {
	Z3_ast ret = Z3_mk_extract(
		context,
		lsb + size - 1,
		lsb,
		(Z3_ast)expr
	);
	printf("<sym> extract %p <= %p, %02lx, %02lx\n", ret, expr, lsb+size-1, lsb);
	return (void *)ret;
}

void *Z3SymbolicExecutor::ConcatBits(void *expr1, void *expr2) {
	Z3_ast ret = Z3_mk_concat(
		context,
		(Z3_ast)expr1,
		(Z3_ast)expr2
	);

	printf("<sym> concat %p <= %p, %p\n", ret, expr1, expr2);
	return (void *)ret;
}

void *Z3SymbolicExecutor::ExecuteResolveAddress(void *base, void *index,
		nodep::BYTE scale) {
	Z3_ast indexRet = 0, scaleAst = 0, Ret = 0;

	switch(scale) {
	case 0:
		indexRet = zeroScale;
		break;
	case 1:
		indexRet = (Z3_ast)index;
		break;
	case 2:
		scaleAst = twoScale;
		break;
	case 4:
		scaleAst = fourScale;
		break;
	default:
		DEBUG_BREAK;
	}

	// if index and scale are valid
	if (index != nullptr && (void *)scaleAst != nullptr) {
		indexRet = Z3_mk_bvmul(context, (Z3_ast)index, scaleAst);
	}

	if (base != nullptr) {
		if ((void *)indexRet != nullptr) {
			Ret = Z3_mk_bvadd(context, (Z3_ast)base, indexRet);
		} else {
			Ret = (Z3_ast)base;
		}
	} else {
		if (index != nullptr) {
			Ret = indexRet;
		} else {
			Ret = (Z3_ast)0;
		}
	}
	printf("base: %p, index: %p, scale: %d, indexRet: %p, Ret: %p\n",
			base, index, scale, (void *)indexRet, (void *)Ret);
	return (void *)Ret;
}

/*bool IsLocked(void *ctx, const Z3_ast *ast) {
	void *v = Z3_get_user_ptr((Z3_context)ctx, *ast);
	const TrackedVariableData *tvd = (const TrackedVariableData *)v;
	const TrackedVariableData *tmp = tvd;

	do {
		if (tmp->isLocked) {
			return true;
		}
		tmp = tmp->next;
	} while (tmp != tvd);

	return false;
}

void Lock(void *ctx, Z3_ast *ast) {
	void *v = Z3_get_user_ptr((Z3_context)ctx, *ast);
	TrackedVariableData *tvd = (TrackedVariableData *)v;
	TrackedVariableData *tmp = tvd;
	do {
		tmp->isLocked = true;
		tmp = tmp->next;
	} while (tmp != tvd);
}

void Unlock(void *ctx, Z3_ast *ast) {
	void *v = Z3_get_user_ptr((Z3_context)ctx, *ast);
	TrackedVariableData *tvd = (TrackedVariableData *)v;
	TrackedVariableData *tmp = tvd;
	do {
		tmp->isLocked = true;
		tmp = tmp->next;
	} while (tmp != tvd);
}*/

const unsigned int Z3SymbolicExecutor::Z3SymbolicCpuFlag::lazyMarker = 0xDEADBEEF;

/*====================================================================================================*/

Z3SymbolicExecutor::Z3SymbolicExecutor(sym::SymbolicEnvironment *e) :
		sym::SymbolicExecutor(e)
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

	config = Z3_mk_config();
	context = Z3_mk_context(config);
	Z3_set_ast_print_mode(context, Z3_PRINT_LOW_LEVEL);
	Z3_open_log("Z3.log");

	dwordSort = Z3_mk_bv_sort(context, 32);
	tbSort = Z3_mk_bv_sort(context, 24);
	wordSort = Z3_mk_bv_sort(context, 16);
	byteSort = Z3_mk_bv_sort(context, 8);
	bitSort = Z3_mk_bv_sort(context, 1);

	sevenBitSort = Z3_mk_bv_sort(context, 7);

	zero32 = Z3_mk_int(context, 0, dwordSort);
	zero16 = Z3_mk_int(context, 0, wordSort);
	zero7 = Z3_mk_int(context, 0, sevenBitSort);
	zero8 = Z3_mk_int(context, 0, byteSort);
	one8 = Z3_mk_int(context, 1, byteSort);

	zeroFlag = Z3_mk_int(context, 0, bitSort);
	oneFlag = Z3_mk_int(context, 1, bitSort);

	zeroScale = zero32;
	twoScale = Z3_mk_int(context, 2, dwordSort);
	fourScale = Z3_mk_int(context, 4, dwordSort);

	solver = Z3_mk_solver(context);
	Z3_solver_push(context, solver);
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

void Z3SymbolicExecutor::StepForward() {
	//variableTracker.Forward();
	Z3_solver_push(context, solver);
	for (int i = 0; i < 7; ++i) {
		lazyFlags[i]->SaveState(*ls);
	}

	/*for (int i = 0; i < 8; ++i) {
		subRegisters[i].SaveState(*ls);
	}*/

	//printf("Solver push\n");
}

void Z3SymbolicExecutor::StepBackward() {
	/*for (int i = 7; i >= 0; --i) {
		subRegisters[i].LoadState(*ls);
	}*/

	for (int i = 6; i >= 0; --i) {
		lazyFlags[i]->LoadState(*ls);
	}

	Z3_solver_pop(context, solver, 1);
	//variableTracker.Backward();
	//printf("Solver pop\n");
}

//void Z3SymbolicExecutor::Lock(Z3_ast t) {
	//variableTracker.Lock(&t);
//}

void Z3SymbolicExecutor::SymbolicExecuteUnk(RiverInstruction *instruction, SymbolicOperands *ops) {
	DEBUG_BREAK;
}

template <unsigned int flag> void Z3SymbolicExecutor::SymbolicExecuteJCC(RiverInstruction *instruction, SymbolicOperands *ops) {
	printf("<sym> jcc %p\n", ops->svf[flag]);

	Z3_ast cond = Z3_mk_eq(
		context,
		(1 == ops->cvf[flag]) ? oneFlag : zeroFlag,
		(Z3_ast)ops->svf[flag]
	);

	lastCondition = Z3_simplify(context, cond);
}

template <unsigned int flag> void Z3SymbolicExecutor::SymbolicExecuteSetCC(RiverInstruction *instruction, SymbolicOperands *ops) {
	printf("<sym> setcc %p\n", ops->svf[flag]);
	Z3_ast res = Z3_mk_concat(
			context,
			zero7,
			(Z3_ast)ops->svf[flag]
			);
	env->SetOperand(0, res);
}

template <unsigned int f1, unsigned int f2, bool eq> void Z3SymbolicExecutor::SymbolicExecuteJCCCompare(RiverInstruction *instruction, SymbolicOperands *ops) {
	printf("<sym> jcc %p %c= %p\n", ops->svf[f1], eq ? '=' : '!', ops->svf[f2]);

	Z3_ast cond = Z3_mk_eq(
		context,
		(Z3_ast)ops->svf[f1],
		(Z3_ast)ops->svf[f2]
	);

	if (!eq) {
		cond = Z3_mk_not(
			context,
			cond
		);
	}

	lastCondition = Z3_simplify(context, cond);
}

template <unsigned int f1, unsigned int f2, unsigned int f3, bool eq> void Z3SymbolicExecutor::SymbolicExecuteJCCCompareEq(RiverInstruction *instruction, SymbolicOperands *ops) {
	printf("<sym> jcc %p %c= %p\n", ops->svf[f1], eq ? '=' : '!', ops->svf[f2]);

	Z3_ast conds[] = {
		Z3_mk_eq(
			context,
			(Z3_ast)ops->svf[f1],
			(Z3_ast)ops->svf[f2]
		),
		Z3_mk_eq(
			context,
			zeroFlag,
			(Z3_ast)ops->svf[f3]
		)
	};

	Z3_ast cond = Z3_mk_and(
		context,
		2,
		conds
	);

	if (!eq) {
		cond = Z3_mk_not(
			context,
			cond
		);
	}

	lastCondition = Z3_simplify(context, cond);
}

Z3_ast Z3SymbolicExecutor::ExecuteAdd(Z3_ast o1, Z3_ast o2) {
	Z3_ast r = Z3_mk_bvadd(context, o1, o2);
	env->SetOperand(0, r);

	//printf("<sym> add %p <= %p, %p\n", r, o1, o2);
	printf("<sym> add op1 %s",
		Z3_ast_to_string(context, o1)
	);
	printf("<sym> add op2 %s",
		Z3_ast_to_string(context, o2)
	);
	return r;
}

Z3_ast Z3SymbolicExecutor::ExecuteOr(Z3_ast o1, Z3_ast o2) {
	Z3_ast r = Z3_mk_bvor(context, o1, o2);
	env->SetOperand(0, r);

	printf("<sym> or %p <= %p, %p\n", r, o1, o2);
	return r;
}

Z3_ast Z3SymbolicExecutor::ExecuteAdc(Z3_ast o1, Z3_ast o2) {
	//return Z3_mk_bvadd(context, o1, o2);
	DEBUG_BREAK;
	return nullptr;
}

Z3_ast Z3SymbolicExecutor::ExecuteSbb(Z3_ast o1, Z3_ast o2) {
	//return Z3_mk_bvor(context, o1, o2);
	DEBUG_BREAK;
	return nullptr;
}

Z3_ast Z3SymbolicExecutor::ExecuteAnd(Z3_ast o1, Z3_ast o2) {
	Z3_ast r = Z3_mk_bvand(context, o1, o2);
	env->SetOperand(0, r);

	printf("<sym> and %p <= %p, %p\n", r, o1, o2);
	return r;
}

Z3_ast Z3SymbolicExecutor::ExecuteSub(Z3_ast o1, Z3_ast o2) {
	Z3_ast r = Z3_mk_bvsub(context, o1, o2);
	env->SetOperand(0, r);

	printf("<sym> sub %p <= %p, %p\n", r, o1, o2);
	return r;
}

Z3_ast Z3SymbolicExecutor::ExecuteXor(Z3_ast o1, Z3_ast o2) {
	if (o1 == o2) {
		return nullptr;
	}

	Z3_ast r = Z3_mk_bvxor(context, o1, o2);
	env->SetOperand(0, r);

	printf("<sym> xor %p <= %p, %p\n", r, o1, o2);
	/*printf("<sym> xor op1 %s",
		Z3_ast_to_string(context, o1)
	);
	printf("<sym> xor op2 %s",
		Z3_ast_to_string(context, o2)
	);*/
	return r;
}

Z3_ast Z3SymbolicExecutor::ExecuteCmp(Z3_ast o1, Z3_ast o2) {
	Z3_ast r = Z3_mk_bvsub(context, o1, o2);
	printf("<sym> cmp %p <= %p, %p\n", r, o1, o2);
	/*printf("<sym> cmp op1 %s",
		Z3_ast_to_string(context, o1)
	);
	printf("<sym> cmp op2 %s",
		Z3_ast_to_string(context, o2)
	);*/
	return r;
}

Z3_ast Z3SymbolicExecutor::ExecuteTest(Z3_ast o1, Z3_ast o2) {
	Z3_ast r = Z3_mk_bvand(context, o1, o2);
	printf("<sym> test %p <= %p, %p\n", r, o1, o2);
	return r;
}

void Z3SymbolicExecutor::SymbolicExecuteMov(RiverInstruction *instruction, SymbolicOperands *ops) {
	// mov dest, addr
	if (ops->tr[1]) {
		env->SetOperand(0, ops->sv[1]);
		printf("<sym> mov <= %p\n", ops->sv[1]);
	} else {
		env->UnsetOperand(0);
	}
}

void Z3SymbolicExecutor::SymbolicExecuteMovSx(RiverInstruction *instruction, SymbolicOperands *ops) {
	if (ops->tr[1]) {
		Z3_ast dst = Z3_mk_sign_ext(context, 24, Z3_mk_extract(context, 7, 0, (Z3_ast)ops->sv[1]));
		env->SetOperand(0, (void *)dst);

		printf("<sym> movsx %p <= %p\n", dst, ops->sv[1]);
	}
	else {
		env->UnsetOperand(0);
	}
}

void Z3SymbolicExecutor::SymbolicExecuteMovZx(RiverInstruction *instruction, SymbolicOperands *ops) {
	if (ops->tr[1]) {
		Z3_ast dst = Z3_mk_zero_ext(context, 24, Z3_mk_extract(context, 7, 0, (Z3_ast)ops->sv[1]));
		env->SetOperand(0, (void *)dst);

		printf("<sym> movzx %p <= %p\n", dst, ops->sv[1]);
	}
	else {
		env->UnsetOperand(0);
	}
}

template <Z3SymbolicExecutor::IntegerFunc func, unsigned int funcCode> void Z3SymbolicExecutor::SymbolicExecuteInteger(RiverInstruction *instruction, SymbolicOperands *ops) {
	Z3_ast ret = (this->*func)((Z3_ast)ops->sv[0], (Z3_ast)ops->sv[1]); //Z3_mk_bvadd(context, (Z3_ast)sv[0], (Z3_ast)sv[1]);
	void *symValue = (void *)ret;
	for (int i = 0; i < 7; ++i) {
		if ((1 << i) & instruction->modFlags) {
			lazyFlags[i]->SetSource(ret, (Z3_ast)ops->sv[0], (Z3_ast)ops->sv[1], nullptr, funcCode);
			env->SetFlgValue(1 << i, lazyFlags[i]);
		}
	}
}

template <Z3SymbolicExecutor::SymbolicExecute fSubOps[8]> void Z3SymbolicExecutor::SymbolicExecuteSubOp(RiverInstruction *instruction, SymbolicOperands *ops) {
	(this->*fSubOps[instruction->subOpCode])(instruction, ops);
}

void Z3SymbolicExecutor::GetSymbolicValues(RiverInstruction *instruction, SymbolicOperands *ops, nodep::DWORD mask) {
	static const unsigned char flagList[] = {
		RIVER_SPEC_FLAG_CF,
		RIVER_SPEC_FLAG_PF,
		RIVER_SPEC_FLAG_AF,
		RIVER_SPEC_FLAG_ZF,
		RIVER_SPEC_FLAG_SF,
		RIVER_SPEC_FLAG_OF
	};

	static const int flagCount = sizeof(flagList) / sizeof(flagList[0]);
	nodep::DWORD m = mask & ops->av;

	bool foundSort = false;
	Z3_sort operandsSort = dwordSort;
	for (int i = 0; i < 4; ++i) {
		if ((OPERAND_BITMASK(i) & m) && ops->tr[i]) {
			 Z3_sort localSort = Z3_get_sort(context, (Z3_ast)ops->sv[i]);
			 if (foundSort && !Z3_is_eq_sort(context, localSort, operandsSort)) {
				 DEBUG_BREAK;
			 }
			 if (!foundSort) {
				 operandsSort = localSort;
				 foundSort = true;
			 }
		}
	}

	for (int i = 0; i < 4; ++i) {
		if ((OPERAND_BITMASK(i) & m) && !ops->tr[i]) {
			ops->sv[i] = Z3_mk_int(context, ops->cv[i], operandsSort);
			printf("<sym> mkint %lu size: %u\n", ops->cv[i], Z3_get_bv_sort_size(context, operandsSort));
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

void printoperand(struct OperandInfo oinfo) {
	printf("[%d] operand: istracked: %d concrete :0x%08lX symbolic: 0x%08lX\n",
			oinfo.opIdx, oinfo.isTracked, oinfo.concrete, (DWORD)oinfo.symbolic);
}

void Z3SymbolicExecutor::Execute(RiverInstruction *instruction) {
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
	nodep::BOOL uo[4], uof[flagCount];
	nodep::BOOL isSymb = false;

	for (int i = 0; i < 4; ++i) {
		struct OperandInfo opInfo;
		opInfo.opIdx = (nodep::BYTE)i;
		opInfo.isTracked = false;

		if (true == (uo[i] = env->GetOperand(opInfo))) {
			ops.av |= OPERAND_BITMASK(i);
			isSymb |= opInfo.isTracked;
			if (isSymb)
				printoperand(opInfo);
		}
		ops.tr[i] = opInfo.isTracked;
		ops.cv[i] = opInfo.concrete;
		ops.sv[i] = opInfo.symbolic;

		struct OperandInfo baseOpInfo, indexOpInfo;
		baseOpInfo.opIdx = i;
		if (true == env->GetAddressBase(baseOpInfo)) {
			printf("[%d] GetAddressBase: riaddr: [%08lx] isTracked: [%d] symb addr: [%p] concrete: [0x%08lX]\n",
					i, instruction->instructionAddress, (int)baseOpInfo.isTracked, baseOpInfo.symbolic, baseOpInfo.concrete);
		}

		nodep::BYTE scale;
		indexOpInfo.opIdx = i;
		if (true == env->GetAddressScaleAndIndex(indexOpInfo, scale)) {
			printf("[%d] GetAddressScaleAndIndex:riaddr: [%08lx] isTracked: [%d] symb addr: [%p] concrete: [0x%08lX]\n",
					i, instruction->instructionAddress, (int)indexOpInfo.isTracked, indexOpInfo.symbolic, indexOpInfo.concrete);
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
		GetSymbolicValues(instruction, &ops, ops.av);

		nodep::DWORD dwTable = (instruction->modifiers & RIVER_MODIFIER_EXT) ? 1 : 0;
		printf("Execute instruction: 0x%08lX\n", instruction->instructionAddress);
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
		/*0x00*/ &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteAdd, Z3_FLAG_OP_ADD>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteAdd, Z3_FLAG_OP_ADD>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteAdd, Z3_FLAG_OP_ADD>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteAdd, Z3_FLAG_OP_ADD>,
		/*0x04*/ &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteAdd, Z3_FLAG_OP_ADD>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteAdd, Z3_FLAG_OP_ADD>, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x08*/ &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteOr,  Z3_FLAG_OP_OR>,  &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteOr,  Z3_FLAG_OP_OR>,  &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteOr,  Z3_FLAG_OP_OR>,  &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteOr,  Z3_FLAG_OP_OR>,
		/*0x0C*/ &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteOr,  Z3_FLAG_OP_OR>,  &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteOr,  Z3_FLAG_OP_OR>,  &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x10*/ &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteAdc, Z3_FLAG_OP_ADC>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteAdc, Z3_FLAG_OP_ADC>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteAdc, Z3_FLAG_OP_ADC>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteAdc, Z3_FLAG_OP_ADC>,
		/*0x14*/ &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteAdc, Z3_FLAG_OP_ADC>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteAdc, Z3_FLAG_OP_ADC>, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x18*/ &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteSbb, Z3_FLAG_OP_SBB>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteSbb, Z3_FLAG_OP_SBB>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteSbb, Z3_FLAG_OP_SBB>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteSbb, Z3_FLAG_OP_SBB>,
		/*0x1C*/ &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteSbb, Z3_FLAG_OP_SBB>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteSbb, Z3_FLAG_OP_SBB>, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x20*/ &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteAnd, Z3_FLAG_OP_AND>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteAnd, Z3_FLAG_OP_AND>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteAnd, Z3_FLAG_OP_AND>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteAnd, Z3_FLAG_OP_AND>,
		/*0x24*/ &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteAnd, Z3_FLAG_OP_AND>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteAnd, Z3_FLAG_OP_AND>, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x28*/ &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteSub, Z3_FLAG_OP_SUB>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteSub, Z3_FLAG_OP_SUB>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteSub, Z3_FLAG_OP_SUB>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteSub, Z3_FLAG_OP_SUB>,
		/*0x2C*/ &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteSub, Z3_FLAG_OP_SUB>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteSub, Z3_FLAG_OP_SUB>, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0x30*/ &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteXor, Z3_FLAG_OP_XOR>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteXor, Z3_FLAG_OP_XOR>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteXor, Z3_FLAG_OP_XOR>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteXor, Z3_FLAG_OP_XOR>,
		/*0x34*/ &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteXor, Z3_FLAG_OP_XOR>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteXor, Z3_FLAG_OP_XOR>, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x38*/ &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteCmp, Z3_FLAG_OP_CMP>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteCmp, Z3_FLAG_OP_CMP>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteCmp, Z3_FLAG_OP_CMP>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteCmp, Z3_FLAG_OP_CMP>,
		/*0x3C*/ &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteCmp, Z3_FLAG_OP_CMP>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteCmp, Z3_FLAG_OP_CMP>, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

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
		/*0x7C*/ &Z3SymbolicExecutor::SymbolicExecuteJCCCompare<RIVER_SPEC_IDX_SF, RIVER_SPEC_IDX_OF, false>, &Z3SymbolicExecutor::SymbolicExecuteJCCCompare<RIVER_SPEC_IDX_SF, RIVER_SPEC_IDX_OF, true>, 
		/*0x7E*/ &Z3SymbolicExecutor::SymbolicExecuteJCCCompareEq<RIVER_SPEC_IDX_SF, RIVER_SPEC_IDX_OF, RIVER_SPEC_IDX_ZF, false>, &Z3SymbolicExecutor::SymbolicExecuteJCCCompareEq<RIVER_SPEC_IDX_SF, RIVER_SPEC_IDX_OF, RIVER_SPEC_IDX_ZF, false>,

		/*0x80*/ &Z3SymbolicExecutor::SymbolicExecuteSubOp<Z3SymbolicExecutor::executeIntegerFuncs>, &Z3SymbolicExecutor::SymbolicExecuteSubOp<Z3SymbolicExecutor::executeIntegerFuncs>, &Z3SymbolicExecutor::SymbolicExecuteSubOp<Z3SymbolicExecutor::executeIntegerFuncs>, &Z3SymbolicExecutor::SymbolicExecuteSubOp<Z3SymbolicExecutor::executeIntegerFuncs>,
		/*0x84*/ &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteTest, Z3_FLAG_OP_AND>, &Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteTest, Z3_FLAG_OP_AND>, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
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

		/*0xB0*/ &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov,
		/*0xB4*/ &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov, &Z3SymbolicExecutor::SymbolicExecuteMov,
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
		/*0x94*/ &Z3SymbolicExecutor::SymbolicExecuteSetCC<RIVER_SPEC_IDX_ZF>, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x98*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0x9C*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0xA0*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xA4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xA8*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xAC*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,

		/*0xB0*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk,
		/*0xB4*/ &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteUnk, &Z3SymbolicExecutor::SymbolicExecuteMovZx, &Z3SymbolicExecutor::SymbolicExecuteUnk,
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

Z3SymbolicExecutor::SymbolicExecute Z3SymbolicExecutor::executeIntegerFuncs[8] = {
	&Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteAdd, Z3_FLAG_OP_ADD>,
	&Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteOr,  Z3_FLAG_OP_OR>,
	&Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteAdc, Z3_FLAG_OP_ADC>,
	&Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteSbb, Z3_FLAG_OP_SBB>,
	&Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteAnd, Z3_FLAG_OP_AND>,
	&Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteSub, Z3_FLAG_OP_SUB>,
	&Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteXor, Z3_FLAG_OP_XOR>,
	&Z3SymbolicExecutor::SymbolicExecuteInteger<&Z3SymbolicExecutor::ExecuteCmp, Z3_FLAG_OP_CMP>
};
