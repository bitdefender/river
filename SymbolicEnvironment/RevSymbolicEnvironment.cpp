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
		valueOffsets[i] = 0xFFFFFFFF;
		baseOffsets[i] = 0xFFFFFFFF;
		indexOffsets[i] = 0xFFFFFFFF;
	}

	flagOffset = 0xFFFFFFFF;

	if ((0 == (RIVER_SPEC_IGNORES_FLG & rIn.specifiers)) || (rIn.modFlags)) {
		flagOffset = trackIndex;
		trackIndex++;
	}

	for (int i = 3; i >= 0; --i) {
		if ((RIVER_OPTYPE(rIn.opTypes[i]) == RIVER_OPTYPE_MEM) && (0 != rIn.operands[i].asAddress->type)) {
			addressOffsets[i] = trackIndex;
			trackIndex += rIn.operands[i].asAddress->HasSegment() ? 2 : 1;

			if (rIn.operands[i].asAddress->type & RIVER_ADDR_BASE) {
				baseOffsets[i] = trackIndex;
				trackIndex++;
			}

			if (rIn.operands[i].asAddress->type & RIVER_ADDR_INDEX) {
				indexOffsets[i] = trackIndex;
				trackIndex++;
			}

			if (0 == (rIn.specifiers & RIVER_SPEC_IGNORES_MEMORY)) {
				valueOffsets[i] = trackIndex + 1; // look at the layout one more time
				trackIndex += 2;
			}
		}
	}

	for (int i = 3; i >= 0; --i) {
		if (0 == (RIVER_SPEC_IGNORES_OP(i) & rIn.specifiers)) {
			switch (RIVER_OPTYPE(rIn.opTypes[i])) {
			case RIVER_OPTYPE_NONE:
			case RIVER_OPTYPE_IMM:
				break;
			case RIVER_OPTYPE_MEM: // holy shit this is an effed up switch
				if (0 == rIn.operands[i].asAddress->type) {
			case RIVER_OPTYPE_REG:
					valueOffsets[i] = trackIndex;
					trackIndex++;
				}
				break;
			/*case RIVER_OPTYPE_REG:
				valueOffsets[i] = trackIndex;
				trackIndex++;
				break;*/
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

	return exec->ExtractBits(symExpr, (4 - offset - size) << 3, size << 3);
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

template<nodep::BYTE offset, nodep::BYTE size> void RevSymbolicEnvironment::SetSubexpression(void *expr, nodep::DWORD address, void *value) {
	nodep::DWORD rSz = (4 - offset - size) << 3;
	nodep::DWORD lSz = offset << 3;
	nodep::DWORD lLsb = (4 - offset) << 3;

	void *ret1 = exec->ExtractBits(value, lLsb, lSz);
	void *ret2 = exec->ConcatBits(ret1, expr);
	decRefFunc(ret1);

	void *ret3 = exec->ExtractBits(value, 0, rSz);
	void *ret4 = exec->ConcatBits(ret2, ret3);
	decRefFunc(ret2);
	decRefFunc(ret3);
		
	nodep::DWORD r = MarkAddrWrapper(pEnv, address, (nodep::DWORD)ret4, 0);
	if (0 != r) {
		decRefFunc((void *)r);
	}
}

template<nodep::BYTE size> void RevSymbolicEnvironment::SetSubexpressionOffM(void *expr, nodep::DWORD address, void *value) {
	nodep::DWORD rSz = (4 - size) << 3;
	void *ret1 = exec->ExtractBits(value, size << 3, rSz);
	void *ret2 = exec->ConcatBits(ret1, expr);
	
	decRefFunc(ret1);
	//addRefFunc(ret2);
	nodep::DWORD r = MarkAddrWrapper(pEnv, address, (nodep::DWORD)ret2, 0);
	if (0 != r) {
		decRefFunc((void *)r);
	}
}

template<nodep::BYTE size> void RevSymbolicEnvironment::SetSubexpressionOff0(void *expr, nodep::DWORD address, void *value) {
	unsigned operandSzR = (4 - size) << 3;
	unsigned operandSzL = size << 3;
	void *ret1 = exec->ExtractBits(value, 0, operandSzR);
	/*
	 * may result in a fix ...
	if (expr == nullptr) {
		expr = exec->MakeConst(((nodep::DWORD)value) >> operandSzR,
				operandSzL);
	}*/
	void *ret2 = exec->ConcatBits(expr, ret1);

	decRefFunc(ret1);

	//addRefFunc(ret2);
	nodep::DWORD r = MarkAddr(pEnv, address, (nodep::DWORD)ret2, 0);
	if (0 != r) {
		decRefFunc((void *)r);
	}
}

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
	if ((RIVER_OPTYPE(current->opTypes[opInfo.opIdx]) != RIVER_OPTYPE_MEM) || (0 == current->operands[opInfo.opIdx].asAddress->type)) {
		return false;
	}

	if (0 == (current->operands[opInfo.opIdx].asAddress->type & RIVER_ADDR_BASE)) {
		return false;
	}

	opInfo.symbolic = (void *)((ExecutionEnvironment *)pEnv)->runtimeContext.taintedRegisters[_GetFundamentalRegister(
		current->operands[opInfo.opIdx].asAddress->base.name)];
	opInfo.isTracked = (opInfo.symbolic != NULL);
	if (opInfo.isTracked)
		fprintf(stderr, "GetAddressBase [%d] => symb [0x%08lX]\n", opInfo.opIdx, (DWORD)opInfo.symbolic);
	opInfo.concrete = opBase[-((int)baseOffsets[opInfo.opIdx])];
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
	opInfo.isTracked = (opInfo.symbolic != NULL);
	if (opInfo.isTracked) {
		fprintf(stderr, "GetAddressScaleAndIndex [%d] => symb [0x%08lX]\n", opInfo.opIdx, (DWORD)opInfo.symbolic);
	}
	opInfo.concrete = opBase[-((int)indexOffsets[opInfo.opIdx])];
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
		opInfo.isTracked = false;
		switch (RIVER_OPSIZE(current->opTypes[opInfo.opIdx])) {
		case RIVER_OPSIZE_32:
			opInfo.concrete = current->operands[opInfo.opIdx].asImm32;
			break;
		case RIVER_OPSIZE_16:
			opInfo.concrete = current->operands[opInfo.opIdx].asImm16;
			break;
		case RIVER_OPSIZE_8:
			opInfo.concrete = current->operands[opInfo.opIdx].asImm8;
			break;
		}
		return true;

	case RIVER_OPTYPE_REG:
		symExpr = (void *)((ExecutionEnvironment *)pEnv)->runtimeContext.taintedRegisters[_GetFundamentalRegister(
				current->operands[opInfo.opIdx].asRegister.name)];

		opInfo.isTracked = (symExpr != NULL);
		opInfo.symbolic = symExpr;
		opInfo.concrete = opBase[-((int)valueOffsets[opInfo.opIdx])];
		//fprintf(stderr, "[%d] <= getOperand reg 0x%lX\n", opInfo.opIdx, (DWORD)opInfo.symbolic);
		return true;

	case RIVER_OPTYPE_MEM:
		// how do we handle dereferenced symbolic values?

		if (0 == current->operands[opInfo.opIdx].asAddress->type) {
			symExpr = (void *)((ExecutionEnvironment *)pEnv)->runtimeContext.taintedRegisters[_GetFundamentalRegister(
					current->operands[opInfo.opIdx].asAddress->base.name)];

			opInfo.isTracked = (symExpr != NULL);
			opInfo.symbolic = symExpr;
			opInfo.concrete = opBase[-((int)valueOffsets[opInfo.opIdx])];
			//fprintf(stderr, "[%d] <= getOperand mem reg 0x%lX\n", opInfo.opIdx, (DWORD)opInfo.symbolic);
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
		opInfo.isTracked = (symExpr != NULL);
		opInfo.symbolic = symExpr;
		opInfo.concrete = opBase[-((int)valueOffsets[opInfo.opIdx])];
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

	flagInfo.isTracked = (0 != val);
	flagInfo.concrete = (opBase[flagOffset] >> flagShifts[flgIdx]) & 1;
	if (flagInfo.isTracked) {
		flagInfo.symbolic = (void *)val;
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
		//fprintf(stderr, "[%d] SetOperand Reg <= 0x%08lX TR: %08lX Freg: %d reg: %d\n", opIdx, (DWORD)symbolicValue,
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
			//fprintf(stderr, "[%d] SetOperand Mem Reg <= 0x%08lX\n", opIdx, (DWORD)symbolicValue);
		}
		else {
			//MarkAddrWrapper(pEnv, opBase[-(addressOffsets[opIdx])], (rev::DWORD)symbolicValue, 0);
			SetExpression(symbolicValue, opBase[-((int)addressOffsets[opIdx])], RIVER_OPSIZE(current->opTypes[opIdx]), &opBase[-((int)valueOffsets[opIdx])]);
			//fprintf(stderr, "[%d] SetOperand Mem <= 0x%08lX\n", opIdx, (DWORD)symbolicValue);
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
