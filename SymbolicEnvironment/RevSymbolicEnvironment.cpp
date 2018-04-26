#include "RevSymbolicEnvironment.h"

#include "../revtracer/execenv.h"
#include "../revtracer/Tracking.h"
#include "../Execution/Execution.h"

static nodep::BYTE _GetFundamentalRegister(nodep::BYTE reg) {
	if (reg < 0x20) {
		return reg & 0x07;
	}
	return reg;
}

void NoAddRef(void *) {}
void NoDecRef(void *) {}

nodep::DWORD TrackAddrWrapper(void *pEnv, nodep::DWORD dwAddr, nodep::DWORD segSel) {
	nodep::DWORD ret = TrackAddr(pEnv, dwAddr, segSel);
	fprintf(stderr, "<info> TrackAddr 0x%08lx => 0x%08lx\n", dwAddr, ret);
	return ret;
}

nodep::DWORD MarkAddrWrapper(void *pEnv, nodep::DWORD dwAddr, nodep::DWORD value, nodep::DWORD segSel) {
	fprintf(stderr, "<info> MarkAddr 0x%08lx <= %lu\n", dwAddr, value);
	return MarkAddr(pEnv, dwAddr, value, segSel);
}

RevSymbolicEnvironment::RevSymbolicEnvironment(void *revEnv, ExecutionController *ctl) {
	ctrl = ctl;
	pEnv = revEnv;

	addRefFunc = NoAddRef;
	decRefFunc = NoDecRef;
}

void RevSymbolicEnvironment::GetOperandLayout(const RiverInstruction &rIn) {
	unsigned int trackIndex = 0;

	for (unsigned int i = 0; i < 4; ++i) {
		addressOffsets[i] = 0xFFFFFFFF;
		inValueOffsets[i] = 0xFFFFFFFF;
		outValueOffsets[i] = 0xFFFFFFFF;
		baseOffsets[i] = 0xFFFFFFFF;
		indexOffsets[i] = 0xFFFFFFFF;
	}

	inFlagOffset = outFlagOffset = 0xFFFFFFFF;

	for (int i = 3; i >= 0; --i) {
		// if memory operand and it is constructed from non-zero addr components
		if ((RIVER_OPTYPE(rIn.opTypes[i]) == RIVER_OPTYPE_MEM) && (0 != rIn.operands[i].asAddress->type)) {
			addressOffsets[i] = trackIndex;
			// for segmented addresses both segment and address are tracked
			trackIndex += rIn.operands[i].asAddress->HasSegment() ? 2 : 1;

			// increase trackIndex if base exists
			if (rIn.operands[i].asAddress->type & RIVER_ADDR_BASE) {
				baseOffsets[i] = trackIndex;
				trackIndex++;
			}

			// increase trackIndex if index exists
			if (rIn.operands[i].asAddress->type & RIVER_ADDR_INDEX) {
				indexOffsets[i] = trackIndex;
				trackIndex++;
			}

			/*if (0 == (rIn.specifiers & RIVER_SPEC_IGNORES_MEMORY)) {
				valueOffsets[i] = trackIndex + 1; // look at the layout one more time
				trackIndex += 2;
			}*/
		}
	}

	//if ((0 == (RIVER_SPEC_IGNORES_FLG & rIn.specifiers)) || (rIn.modFlags)) {
	if (rIn.testFlags | rIn.modFlags) {
		inFlagOffset = trackIndex;
		trackIndex++;
	}

	for (int i = 3; i >= 0; --i) {
		if (0 == (RIVER_SPEC_IGNORES_OP(i) & rIn.specifiers)) {
			switch (RIVER_OPTYPE(rIn.opTypes[i])) {
			case RIVER_OPTYPE_NONE:
			case RIVER_OPTYPE_IMM:
				break;
			case RIVER_OPTYPE_MEM:
				if (0 == (rIn.specifiers & RIVER_SPEC_IGNORES_MEMORY)) {
					if (0 == rIn.operands[i].asAddress->type) {
						inValueOffsets[i] = trackIndex;
						trackIndex++;
					} else {
						inValueOffsets[i] = trackIndex + 1;
						trackIndex += 2;
					}
				}
			case RIVER_OPTYPE_REG:
				inValueOffsets[i] = trackIndex;
				trackIndex++;
				break;
			default:
				DEBUG_BREAK;
			}
		}
	}


	if (rIn.modFlags) {
		outFlagOffset = trackIndex;
		trackIndex++;
	}

	for (int i = 3; i >= 0; --i) {
		if (RIVER_SPEC_MODIFIES_OP(i) & rIn.specifiers) {
			switch (RIVER_OPTYPE(rIn.opTypes[i])) {
			case RIVER_OPTYPE_NONE:
			case RIVER_OPTYPE_IMM:
				break;
			case RIVER_OPTYPE_MEM:
				if (0 == (rIn.specifiers & RIVER_SPEC_IGNORES_MEMORY)) {
					if (0 == rIn.operands[i].asAddress->type) {
						inValueOffsets[i] = trackIndex;
						trackIndex++;
					}
					else {
						inValueOffsets[i] = trackIndex + 1;
						trackIndex += 2;
					}
				}
			case RIVER_OPTYPE_REG:
				inValueOffsets[i] = trackIndex;
				trackIndex++;
				break;
			default:
				DEBUG_BREAK;
			}
		}
	}
}

template <nodep::BYTE offset, nodep::BYTE size> void *RevSymbolicEnvironment::GetSubexpression(nodep::DWORD address) {
	void *symExpr = (void *)TrackAddrWrapper(pEnv, address, 0);
	if (symExpr == nullptr) {
		return nullptr;
	}

	return exec->ExtractBits(symExpr, offset << 3, size << 3);
}

template <> void *RevSymbolicEnvironment::GetSubexpression<0, 4>(nodep::DWORD address) {
	void *symExpr = (void *)TrackAddrWrapper(pEnv, address, 0);

	if (nullptr != symExpr) {
		addRefFunc(symExpr);
	}

	return symExpr;
}

void *RevSymbolicEnvironment::GetSubexpressionInvalid(nodep::DWORD address) {
	DEBUG_BREAK;
}

RevSymbolicEnvironment::GetSubExpFunc RevSymbolicEnvironment::subExpsGet[4][5] = {
	{ nullptr, &RevSymbolicEnvironment::GetSubexpression<0, 1>, &RevSymbolicEnvironment::GetSubexpression<0, 2>,  &RevSymbolicEnvironment::GetSubexpression<0, 3>,  &RevSymbolicEnvironment::GetSubexpression<0, 4>  }, 
	{ nullptr, &RevSymbolicEnvironment::GetSubexpression<1, 1>, &RevSymbolicEnvironment::GetSubexpression<1, 2>,  &RevSymbolicEnvironment::GetSubexpression<1, 3>,  &RevSymbolicEnvironment::GetSubexpressionInvalid },
	{ nullptr, &RevSymbolicEnvironment::GetSubexpression<2, 1>, &RevSymbolicEnvironment::GetSubexpression<2, 2>,  &RevSymbolicEnvironment::GetSubexpressionInvalid, &RevSymbolicEnvironment::GetSubexpressionInvalid },
	{ nullptr, &RevSymbolicEnvironment::GetSubexpression<3, 1>, &RevSymbolicEnvironment::GetSubexpressionInvalid, &RevSymbolicEnvironment::GetSubexpressionInvalid, &RevSymbolicEnvironment::GetSubexpressionInvalid },
};

void *RevSymbolicEnvironment::GetExpression(nodep::DWORD address, nodep::DWORD size) {
	static const nodep::DWORD sizes[3] = { 4, 2, 1 };
	nodep::DWORD sz = sizes[RIVER_OPSIZE(size)];
	
	void *ret = nullptr;

	while (sz) {
		nodep::DWORD fa = address & (~0x03), fo = address & 0x03;
		nodep::DWORD copy = 4 - fo;

		if (sz < copy) {
			copy = sz;
		}

		void *tmp = (this->*subExpsGet[fo][copy])(address);

		if (nullptr == ret) {
			ret = tmp;
		} else {
			void *newret = exec->ConcatBits(ret, tmp);
			decRefFunc(ret);
			ret = newret;
		}

		address += copy;
		sz -= copy;
	}

	return ret;
}

//set expr from offset to offset + size
template<nodep::BYTE offset, nodep::BYTE size> void RevSymbolicEnvironment::SetSubexpression(void *expr, nodep::DWORD address, void *value) {
	/* z3: | |x| | |
	 *      3 2 1 0
	 * dw: | | |x| |
	 *	    0 1 2 3
	 */
	nodep::DWORD operandSzL = (4 - offset - size) << 3;
	nodep::DWORD operandSzR = offset << 3;
	nodep::DWORD operandSzM = size << 3;

	void *retRight = exec->ExtractBits(value, 0, operandSzR);

	if (expr == nullptr) {
		expr = exec->MakeConst(((nodep::DWORD)value >> operandSzL) & ((1 << operandSzM) - 1),
				operandSzM);
	}
	void *retMidRight = exec->ConcatBits(expr, retRight);
	decRefFunc(retRight);

	void *retLeft = exec->ExtractBits(value, operandSzM + operandSzR, operandSzL);
	void *ret = exec->ConcatBits(retLeft, retMidRight);
	decRefFunc(retMidRight);
	decRefFunc(retLeft);

	nodep::DWORD r = MarkAddrWrapper(pEnv, address, (nodep::DWORD)ret, 0);
	if (0 != r) {
		decRefFunc((void *)r);
	}
}

/*set expr on right-most bytes*/
template<nodep::BYTE size> void RevSymbolicEnvironment::SetSubexpressionOffM(void *expr, nodep::DWORD address, void *value) {
	/* z3: |x| | | |
	 *      3 2 1 0
	 * dw: | | | |x|
	 *	    0 1 2 3
	 */
	nodep::DWORD operandSzL = size << 3;
	nodep::DWORD operandSzR = (4 - size) << 3;
	void *retRight = exec->ExtractBits(value, 0, operandSzR);

	if (expr == nullptr) {
		expr = exec->MakeConst(((nodep::DWORD)value) >> operandSzR,
				operandSzL);
	}

	void *ret = exec->ConcatBits(expr, retRight);

	decRefFunc(retRight);
	//addRefFunc(ret2);
	nodep::DWORD r = MarkAddrWrapper(pEnv, address, (nodep::DWORD)ret, 0);
	if (0 != r) {
		decRefFunc((void *)r);
	}
}

/* `size` = size in bytes of `expr`
 * `expr` = SE that marks letf-most `size` bytes
 *
 * why dword and address have different layout:
 * we intend to mark left-most size bytes with SE `expr`
 */
template<nodep::BYTE size> void RevSymbolicEnvironment::SetSubexpressionOff0(void *expr, nodep::DWORD address, void *value) {
    /* z3: | | | |x|
	 *      3 2 1 0
	 * dw: |x| | | |
	 *	    0 1 2 3
	 */
	unsigned operandSzL = (4 - size) << 3;
	unsigned operandSzR = size << 3;
	// extract bytes from zero to operandSzR. consider the `dword` layout
	void *retLeft = exec->ExtractBits(value, operandSzR, operandSzL);

	if (expr == nullptr) {
		// dw processing x86
		// WARN: valid expr should not have value `0`
		expr = exec->MakeConst(((nodep::DWORD)value) & ((1 << operandSzR) - 1),
				operandSzR);
	}

	//considering `dword` layout, we concatenate SE `expr` with
	//the remaining of `value`
	void *ret = exec->ConcatBits(retLeft, expr);

	decRefFunc(retLeft);

	nodep::DWORD r = MarkAddr(pEnv, address, (nodep::DWORD)ret, 0);
	if (0 != r) {
		decRefFunc((void *)r);
	}
}

/* `expr` corresponds to memory at `address`, all 4 bytes from offset 0*/
template <> void RevSymbolicEnvironment::SetSubexpression<0, 4>(void *expr, nodep::DWORD address, void *value) {
	if (nullptr != expr) {
		addRefFunc(expr);
	}
	nodep::DWORD r = MarkAddr(pEnv, address, (nodep::DWORD)expr, 0);
	if (0 != r) {
		decRefFunc((void *)r);
	}
}

void RevSymbolicEnvironment::SetSubexpressionInvalid(void *expr, nodep::DWORD address, void *value) {
	DEBUG_BREAK;
}

RevSymbolicEnvironment::SetSubExpFunc RevSymbolicEnvironment::subExpsSet[4][5] = {
	{ nullptr, &RevSymbolicEnvironment::SetSubexpressionOff0<1>, &RevSymbolicEnvironment::SetSubexpressionOff0<2>, &RevSymbolicEnvironment::SetSubexpressionOff0<3>, &RevSymbolicEnvironment::SetSubexpression<0, 4>  },
	{ nullptr, &RevSymbolicEnvironment::SetSubexpression<1, 1>,  &RevSymbolicEnvironment::SetSubexpression<1, 2>,  &RevSymbolicEnvironment::SetSubexpressionOffM<3>,  &RevSymbolicEnvironment::SetSubexpressionInvalid },
	{ nullptr, &RevSymbolicEnvironment::SetSubexpression<2, 1>,  &RevSymbolicEnvironment::SetSubexpressionOffM<2>,  &RevSymbolicEnvironment::SetSubexpressionInvalid, &RevSymbolicEnvironment::SetSubexpressionInvalid },
	{ nullptr, &RevSymbolicEnvironment::SetSubexpressionOffM<1>, &RevSymbolicEnvironment::SetSubexpressionInvalid, &RevSymbolicEnvironment::SetSubexpressionInvalid, &RevSymbolicEnvironment::SetSubexpressionInvalid }
};

/*this will set symbolic expression `expr` at address depending
 * on the addressing type and size
 * valid approaches are:
 * * 4 bytes on 4 bytes
 * * `size` bytes written from 0 offset
 * * `size` bytes written to MAX offset
 * * `size` bytes written from `offset`
 */
void RevSymbolicEnvironment::SetExpression(void *exp, nodep::DWORD address, nodep::DWORD size, nodep::DWORD *values) {
	static const nodep::DWORD sizes[3] = { 4, 2, 1 };
	nodep::DWORD sz = sizes[RIVER_OPSIZE(size)];
	nodep::DWORD osz = sz;
	nodep::DWORD ptr = 0;

	while (sz) {
		nodep::DWORD fa = address & (~0x03), fo = address & 0x03;
		nodep::DWORD copy = 4 - fo;

		if (sz < copy) {
			copy = sz;
		}

		void *ext = (osz == copy) ? exp : exec->ExtractBits(exp, (sz - copy) << 3, copy << 3);

		void *val = (void *)TrackAddr(pEnv, fa, 0);
		if (((0 != fo) || (4 != sz)) && (nullptr == val)) {
			val = exec->MakeConst(values[ptr], 32);
		}


		(this->*subExpsSet[fo][copy])(ext, address, val);

		if (ext != exp) {
			decRefFunc(ext);
		}
		ptr++;

		address += copy;
		sz -= copy;
	}
}

void RevSymbolicEnvironment::SetReferenceCounting(AddRefFunc addRef, DecRefFunc decRef) {
	addRefFunc = addRef;
	decRefFunc = decRef;
}

bool RevSymbolicEnvironment::SetCurrentInstruction(RiverInstruction *rIn, void *context) {
	opBase = (nodep::DWORD *)context;
	current = rIn;

	GetOperandLayout(*current);
	return true;
}

void RevSymbolicEnvironment::PushState(stk::LargeStack &stack) { }
void RevSymbolicEnvironment::PopState(stk::LargeStack &stack) { }

bool RevSymbolicEnvironment::GetAddressBase(struct OperandInfo &opInfo) {
	if ((RIVER_OPTYPE(current->opTypes[opInfo.opIdx]) != RIVER_OPTYPE_MEM) ||
			(0 == current->operands[opInfo.opIdx].asAddress->type)) {
		return false;
	}

	if (0 == (current->operands[opInfo.opIdx].asAddress->type & RIVER_ADDR_BASE)) {
		return false;
	}

	opInfo.symbolic = (void *)((ExecutionEnvironment *)pEnv)->runtimeContext.taintedRegisters[_GetFundamentalRegister(
		current->operands[opInfo.opIdx].asAddress->base.name)];
	opInfo.fields = (opInfo.symbolic != NULL) ? OP_HAS_SYMBOLIC : 0;
	if (opInfo.fields) {
		fprintf(stderr, "GetAddressBase [%d] => symb [0x%08lX]\n", opInfo.opIdx, (DWORD)opInfo.symbolic);
	}
	opInfo.concreteBefore = opBase[-((int)baseOffsets[opInfo.opIdx])];
	opInfo.fields |= OP_HAS_CONCRETE_BEFORE;
	return true;
}

bool RevSymbolicEnvironment::GetAddressScaleAndIndex(struct OperandInfo &opInfo, nodep::BYTE &scale) {
	if ((RIVER_OPTYPE(current->opTypes[opInfo.opIdx]) != RIVER_OPTYPE_MEM) ||
			(0 == current->operands[opInfo.opIdx].asAddress->type)) {
		return false;
	}

	if (0 == (current->operands[opInfo.opIdx].asAddress->type & RIVER_ADDR_INDEX)) {
		return false;
	}

	scale = current->operands[opInfo.opIdx].asAddress->GetScale();
	opInfo.symbolic = (void *)((ExecutionEnvironment *)pEnv)->runtimeContext.taintedRegisters[_GetFundamentalRegister(
		current->operands[opInfo.opIdx].asAddress->index.name)];
	opInfo.fields = (opInfo.symbolic != NULL) ? OP_HAS_SYMBOLIC : 0;
	if (opInfo.fields) {
		fprintf(stderr, "GetAddressScaleAndIndex [%d] => symb [0x%08lX]\n", opInfo.opIdx, (DWORD)opInfo.symbolic);
	}
	opInfo.concreteBefore = opBase[-((int)indexOffsets[opInfo.opIdx])];
	opInfo.fields |= OP_HAS_CONCRETE_BEFORE;
	return true;
}

bool RevSymbolicEnvironment::GetAddressDisplacement(const nodep::BYTE opIdx, struct AddressDisplacement &addressDisplacement) {
	if ((RIVER_OPTYPE(current->opTypes[opIdx]) != RIVER_OPTYPE_MEM) ||
			(0 == current->operands[opIdx].asAddress->type)) {
		return false;
	}

	if (current->operands[opIdx].asAddress->type & RIVER_ADDR_DISP8) {
		addressDisplacement.type = RIVER_ADDR_DISP8;
		addressDisplacement.disp = (nodep::DWORD)current->operands[opIdx].asAddress->disp.d8;
	} else if (current->operands[opIdx].asAddress->type & RIVER_ADDR_DISP) {
		addressDisplacement.type = RIVER_ADDR_DISP;
		addressDisplacement.disp = current->operands[opIdx].asAddress->disp.d32;
	} else {
		return false;
	}

	return true;
}

bool RevSymbolicEnvironment::GetOperand(struct OperandInfo &opInfo) {
	void *symExpr;

	if (RIVER_SPEC_IGNORES_OP(opInfo.opIdx) & current->specifiers) {
		return false;
	}

	switch (RIVER_OPTYPE(current->opTypes[opInfo.opIdx])) {
	case RIVER_OPTYPE_NONE:
		return false;

	case RIVER_OPTYPE_IMM:
		opInfo.fields = OP_HAS_CONCRETE_BEFORE | OP_HAS_CONCRETE_AFTER;
		switch (RIVER_OPSIZE(current->opTypes[opInfo.opIdx])) {
		case RIVER_OPSIZE_32:
			opInfo.concreteBefore = opInfo.concreteAfter = current->operands[opInfo.opIdx].asImm32;
			break;
		case RIVER_OPSIZE_16:
			opInfo.concreteBefore = opInfo.concreteAfter = current->operands[opInfo.opIdx].asImm16;
			break;
		case RIVER_OPSIZE_8:
			opInfo.concreteBefore = opInfo.concreteAfter = current->operands[opInfo.opIdx].asImm8;
			break;
		}
		return true;

	case RIVER_OPTYPE_REG:
		symExpr = (void *)((ExecutionEnvironment *)pEnv)->runtimeContext.taintedRegisters[_GetFundamentalRegister(
				current->operands[opInfo.opIdx].asRegister.name)];

		opInfo.fields = 0;

		if (symExpr) {
			opInfo.symbolic = symExpr;
			opInfo.fields |= OP_HAS_SYMBOLIC;
		}
		
		if (0xFFFFFFFF != inValueOffsets[opInfo.opIdx]) {
			opInfo.concreteBefore = opBase[-((int)inValueOffsets[opInfo.opIdx])];
			opInfo.fields |= OP_HAS_CONCRETE_BEFORE;
		}

		if (0xFFFFFFFF != outValueOffsets[opInfo.opIdx]) {
			opInfo.concreteAfter = opBase[-((int)outValueOffsets[opInfo.opIdx])];
			opInfo.fields |= OP_HAS_CONCRETE_AFTER;
		}

		//printf("[%d] <= getOperand reg 0x%lX\n", opInfo.opIdx, (DWORD)opInfo.symbolic);
		return true;

	case RIVER_OPTYPE_MEM:
		// how do we handle dereferenced symbolic values?

		if (0 == current->operands[opInfo.opIdx].asAddress->type) {
			symExpr = (void *)((ExecutionEnvironment *)pEnv)->runtimeContext.taintedRegisters[_GetFundamentalRegister(
				current->operands[opInfo.opIdx].asAddress->base.name)];

			opInfo.fields = 0;

			if (symExpr) {
				opInfo.symbolic = symExpr;
				opInfo.fields |= OP_HAS_SYMBOLIC;
			}

			if (0xFFFFFFFF != inValueOffsets[opInfo.opIdx]) {
				opInfo.concreteBefore = opBase[-((int)inValueOffsets[opInfo.opIdx])];
				opInfo.fields |= OP_HAS_CONCRETE_BEFORE;
			}

			if (0xFFFFFFFF != outValueOffsets[opInfo.opIdx]) {
				opInfo.concreteAfter = opBase[-((int)outValueOffsets[opInfo.opIdx])];
				opInfo.fields |= OP_HAS_CONCRETE_AFTER;
			}
			//printf("[%d] <= getOperand mem reg 0x%lX\n", opInfo.opIdx, (DWORD)opInfo.symbolic);
			return true;
		}

		if (RIVER_SPEC_IGNORES_MEMORY & current->specifiers) {
			return false; // for now!
		}

		//symExpr = get from opBase[opOffset[opInfo.opIdx]]

		if (opBase[-((int)addressOffsets[opInfo.opIdx])] < 0x1000) {
			DEBUG_BREAK;
		}

		symExpr = GetExpression(opBase[-((int)addressOffsets[opInfo.opIdx])],
				RIVER_OPSIZE(current->opTypes[opInfo.opIdx])); //(void *)TrackAddrWrapper(pEnv, opBase[-(addressOffsets[opInfo.opIdx])], 0);
		opInfo.fields = 0;

		if (symExpr) {
			opInfo.symbolic = symExpr;
			opInfo.fields |= OP_HAS_SYMBOLIC;
		}

		if (0xFFFFFFFF != inValueOffsets[opInfo.opIdx]) {
			opInfo.concreteBefore = opBase[-((int)inValueOffsets[opInfo.opIdx])];
			opInfo.fields |= OP_HAS_CONCRETE_BEFORE;
		}

		if (0xFFFFFFFF != outValueOffsets[opInfo.opIdx]) {
			opInfo.concreteAfter = opBase[-((int)outValueOffsets[opInfo.opIdx])];
			opInfo.fields |= OP_HAS_CONCRETE_AFTER;
		}
		return true;
	}

	return false;
}

unsigned int BinLog2(unsigned int v) {
	register unsigned int r; // result of log2(v) will go here
	register unsigned int shift;

	r = (v > 0xFFFF) << 4; v >>= r;
	shift = (v > 0xFF) << 3; v >>= shift; r |= shift;
	shift = (v > 0x0F) << 2; v >>= shift; r |= shift;
	shift = (v > 0x03) << 1; v >>= shift; r |= shift;
	r |= (v >> 1);

	return r;
}

unsigned int flagShifts[] = {
	0, //RIVER_SPEC_FLAG_CF
	2, //RIVER_SPEC_FLAG_PF
	4, //RIVER_SPEC_FLAG_AF
	6, //RIVER_SPEC_FLAG_ZF
	7, //RIVER_SPEC_FLAG_SF
	11, //RIVER_SPEC_FLAG_OF
	10 //RIVER_SPEC_FLAG_DF
};

bool RevSymbolicEnvironment::GetFlgValue(struct FlagInfo &flagInfo) {
	if (0 == (flagInfo.opIdx & current->testFlags)) {
		return false;
	}

	unsigned int flgIdx = BinLog2(flagInfo.opIdx);
	//void *expr = pEnv->runtimeContext.taintedFlags[flgIdx];

	nodep::DWORD val = ((ExecutionEnvironment *)pEnv)->runtimeContext.taintedFlags[flgIdx];

	flagInfo.fields = 0;

	if (0 != val) {
		flagInfo.symbolic = (void *)val;
		flagInfo.fields |= OP_HAS_SYMBOLIC;
	}

	if (0xFFFFFFFF != inFlagOffset) {
		flagInfo.concreteBefore = (opBase[inFlagOffset] >> flagShifts[flgIdx]) & 1;
		flagInfo.fields |= OP_HAS_CONCRETE_BEFORE;
	}

	if (0xFFFFFFFF != outFlagOffset) {
		flagInfo.concreteAfter = (opBase[outFlagOffset] >> flagShifts[flgIdx]) & 1;
		flagInfo.fields |= OP_HAS_CONCRETE_AFTER;
	}

	return true;
}

bool RevSymbolicEnvironment::SetOperand(nodep::BYTE opIdx, void *symbolicValue, bool doRefCount) {
	nodep::DWORD tmp;
	switch (RIVER_OPTYPE(current->opTypes[opIdx])) {
	case RIVER_OPTYPE_NONE:
	case RIVER_OPTYPE_IMM:
		return false;

	case RIVER_OPTYPE_REG:
		tmp = ((ExecutionEnvironment *)pEnv)->runtimeContext.taintedRegisters[_GetFundamentalRegister(current->operands[opIdx].asRegister.name)];
		if (doRefCount && (0 != tmp)) {
			decRefFunc((void *)tmp);
		}
		((ExecutionEnvironment *)pEnv)->runtimeContext.taintedRegisters[_GetFundamentalRegister(current->operands[opIdx].asRegister.name)] = (nodep::DWORD)symbolicValue;
		if (doRefCount && (nullptr != symbolicValue)) {
			addRefFunc(symbolicValue);
		}
		//printf("[%d] SetOperand Reg <= 0x%08lX TR: %08lX Freg: %d reg: %d\n", opIdx, (DWORD)symbolicValue,
		//		&((ExecutionEnvironment *)pEnv)->runtimeContext.taintedRegisters[_GetFundamentalRegister(current->operands[opIdx].asRegister.name)],
		//		current->operands[opIdx].asRegister.name);
		return true;

	case RIVER_OPTYPE_MEM:
		if (0 == current->operands[opIdx].asAddress->type) {
			tmp = ((ExecutionEnvironment *)pEnv)->runtimeContext.taintedRegisters[_GetFundamentalRegister(current->operands[opIdx].asAddress->base.name)];
			if (doRefCount && (0 != tmp)) {
				decRefFunc((void *)tmp);
			}
			((ExecutionEnvironment *)pEnv)->runtimeContext.taintedRegisters[_GetFundamentalRegister(current->operands[opIdx].asAddress->base.name)] = (nodep::DWORD)symbolicValue;
			if (doRefCount && (nullptr != symbolicValue)) {
				addRefFunc(symbolicValue);
			}
			//printf("[%d] SetOperand Mem Reg <= 0x%08lX\n", opIdx, (DWORD)symbolicValue);
		}
		else {
			//MarkAddrWrapper(pEnv, opBase[-(addressOffsets[opIdx])], (rev::DWORD)symbolicValue, 0);
			SetExpression(symbolicValue, opBase[-((int)addressOffsets[opIdx])], RIVER_OPSIZE(current->opTypes[opIdx]), &opBase[-((int)outValueOffsets[opIdx])]);
			//printf("[%d] SetOperand Mem <= 0x%08lX\n", opIdx, (DWORD)symbolicValue);
		}
		return true;

	}

	return false;
}

bool RevSymbolicEnvironment::UnsetOperand(nodep::BYTE opIdx, bool doRefCount) {
	return SetOperand(opIdx, nullptr, doRefCount);
}

void RevSymbolicEnvironment::SetFlgValue(nodep::BYTE flg, void *symbolicValue, bool doRefCount) {
	unsigned int flgIdx = BinLog2(flg);

	nodep::DWORD tmp = ((ExecutionEnvironment *)pEnv)->runtimeContext.taintedFlags[flgIdx];
	if (doRefCount && (0 != tmp)) {
		decRefFunc((void *)tmp);
	}
	((ExecutionEnvironment *)pEnv)->runtimeContext.taintedFlags[flgIdx] = (nodep::DWORD)symbolicValue;
	if (doRefCount && (nullptr != symbolicValue)) {
		addRefFunc(symbolicValue);
	}
}

void RevSymbolicEnvironment::UnsetFlgValue(nodep::BYTE flg, bool doRefCount) {
	SetFlgValue(flg, nullptr, doRefCount);
}

void RevSymbolicEnvironment::SetSymbolicVariable(const char *name, rev::ADDR_TYPE addr, nodep::DWORD size) {
	nodep::DWORD buff[2];
	ctrl->ReadProcessMemory((unsigned int)addr & ~0x03, 8, (unsigned char *)buff);

	nodep::DWORD oSize = 0;

	switch (size) {
	case 4:
		oSize = RIVER_OPSIZE_32;
		break;
	case 2:
		oSize = RIVER_OPSIZE_16;
		break;
	case 1 : 
		oSize = RIVER_OPSIZE_8;
		break;
	default :
		DEBUG_BREAK;
	}

	SetExpression(
		exec->CreateVariable(name, size),
		(nodep::DWORD)addr,
		oSize,
		buff
	);
}
