#include "SymbolicEnvironmentImpl.h"

void SymbolicEnvironmentImpl::GetOperandLayout(const RiverInstruction &rIn) {
	DWORD trackIndex = 0;

	for (DWORD i = 0; i < 4; ++i) {
		addressOffsets[i] = 0xFFFFFFFF;
	}

	for (DWORD i = 0; i < 4; ++i) {
		valueOffsets[i] = 0xFFFFFFFF;
	}

	flagOffset = 0xFFFFFFFF;

	if ((0 == (RIVER_SPEC_IGNORES_FLG & rIn.specifiers)) || (rIn.modFlags)) {
		flagOffset = trackIndex;
		trackIndex++;
	}

	for (int i = 3; i >= 0; --i) {
		if ((RIVER_OPTYPE(rIn.opTypes[i]) == RIVER_OPTYPE_MEM) && (0 != rIn.operands[i].asAddress->type)){
			addressOffsets[i] = trackIndex;
			trackIndex += rIn.operands[i].asAddress->HasSegment() ? 2 : 1;
		}
	}

	for (int i = 3; i >= 0; --i) {
		if (0 == (RIVER_SPEC_IGNORES_OP(i) & rIn.specifiers)) {
			switch (RIVER_OPTYPE(rIn.opTypes[i])) {
				case RIVER_OPTYPE_NONE :
				case RIVER_OPTYPE_IMM :
					break;
				case RIVER_OPTYPE_REG :
				case RIVER_OPTYPE_MEM :
					valueOffsets[i] = trackIndex;
					trackIndex++;
					break;
				default :
					_asm int 3;
			}
		}
	}
}

void SymbolicEnvironmentImpl::SetCurrentContext(RiverInstruction *rIn, void *context) {
	opBase = (rev::DWORD *)context;

	current = rIn;

	rev::DWORD cOff = 0;

	// handle flags
	GetOperandLayout(*current);

}

namespace rev {
	DWORD __stdcall TrackAddr(struct ::ExecutionEnvironment *pEnv, DWORD dwAddr, DWORD segSel);
	DWORD __stdcall MarkAddr(struct ::ExecutionEnvironment *pEnv, DWORD dwAddr, DWORD value, DWORD segSel);
};

bool GetOperand(void *ctx, rev::BYTE opIdx, rev::BOOL &isTracked, rev::DWORD &concreteValue, void *&symbolicValue) {
	SymbolicEnvironmentImpl *_this = (SymbolicEnvironmentImpl *)ctx;
	void *symExpr;

	if (RIVER_SPEC_IGNORES_OP(opIdx) & _this->current->specifiers) {
		return false;
	}

	switch (RIVER_OPTYPE(_this->current->opTypes[opIdx])) {
		case RIVER_OPTYPE_NONE :
			return false;

		case RIVER_OPTYPE_IMM :
			isTracked = false;
			switch (RIVER_OPSIZE(_this->current->opTypes[opIdx])) {
				case RIVER_OPSIZE_32 :
					concreteValue = _this->current->operands[opIdx].asImm32;
					break;
				case RIVER_OPSIZE_16 :
					concreteValue = _this->current->operands[opIdx].asImm16;
					break;
				case RIVER_OPSIZE_8 :
					concreteValue = _this->current->operands[opIdx].asImm8;
					break;
			}
			return true;

		case RIVER_OPTYPE_REG :
			symExpr = (void *)_this->pEnv->runtimeContext.taintedRegisters[GetFundamentalRegister(_this->current->operands[opIdx].asRegister.name)];

			isTracked = (symExpr != NULL);
			symbolicValue = symExpr;
			concreteValue = _this->opBase[- (_this->valueOffsets[opIdx])];
			return true;

		case RIVER_OPTYPE_MEM :
			// how do we handle dereferenced symbolic values?

			if (0 == _this->current->operands[opIdx].asAddress->type) {
				symExpr = (void *)_this->pEnv->runtimeContext.taintedRegisters[GetFundamentalRegister(_this->current->operands[opIdx].asAddress->base.name)];

				isTracked = (symExpr != NULL);
				symbolicValue = symExpr;
				concreteValue = _this->opBase[-(_this->valueOffsets[opIdx])];
				return true;
			}

			if (RIVER_SPEC_IGNORES_MEMORY & _this->current->specifiers) {
				return false; // for now!
			}

			//symExpr = get from opBase[opOffset[opIdx]]

			if (_this->opBase[-(_this->addressOffsets[opIdx])] < 0x1000) {
				__asm int 3;
			}

			symExpr = (void *)TrackAddr(_this->pEnv, _this->opBase[-(_this->addressOffsets[opIdx])], 0);
			isTracked = (symExpr != NULL);
			symbolicValue = symExpr;
			concreteValue = _this->opBase[- (_this->valueOffsets[opIdx])];
			return true;
	}

	return false;
}

DWORD BinLog2(DWORD v) {
	register DWORD r; // result of log2(v) will go here
	register DWORD shift;

	r = (v > 0xFFFF) << 4; v >>= r;
	shift = (v > 0xFF) << 3; v >>= shift; r |= shift;
	shift = (v > 0x0F) << 2; v >>= shift; r |= shift;
	shift = (v > 0x03) << 1; v >>= shift; r |= shift;
	r |= (v >> 1);

	return r;
}

BYTE flagShifts[] = {
	0, //RIVER_SPEC_FLAG_CF
	2, //RIVER_SPEC_FLAG_PF
	4, //RIVER_SPEC_FLAG_AF
	6, //RIVER_SPEC_FLAG_ZF
	7, //RIVER_SPEC_FLAG_SF
	11, //RIVER_SPEC_FLAG_OF
	10 //RIVER_SPEC_FLAG_DF
};

bool GetFlgValue(void *ctx, rev::BYTE flg, rev::BOOL &isTracked, rev::BYTE &concreteValue, void *&symbolicValue) {
	SymbolicEnvironmentImpl *_this = (SymbolicEnvironmentImpl *)ctx;
	if (0 == (flg & _this->current->testFlags)) {
		return false;
	}

	DWORD flgIdx = BinLog2(flg);
	//void *expr = pEnv->runtimeContext.taintedFlags[flgIdx];

	DWORD val = _this->pEnv->runtimeContext.taintedFlags[flgIdx];

	isTracked = (0 != val);
	concreteValue = (_this->opBase[_this->flagOffset] >> flagShifts[flgIdx]) & 1;
	if (isTracked) {
		symbolicValue = (void *)val;
	}

	return true;
}

bool SetOperand(void *ctx, rev::BYTE opIdx, void *symbolicValue) {
	SymbolicEnvironmentImpl *_this = (SymbolicEnvironmentImpl *)ctx;

	switch (RIVER_OPTYPE(_this->current->opTypes[opIdx])) {
		case RIVER_OPTYPE_NONE :
		case RIVER_OPTYPE_IMM :
			return false;

		case RIVER_OPTYPE_REG :
			_this->pEnv->runtimeContext.taintedRegisters[GetFundamentalRegister(_this->current->operands[opIdx].asRegister.name)] = (DWORD)symbolicValue;
			return true;

		case RIVER_OPTYPE_MEM :
			if (0 == _this->current->operands[opIdx].asAddress->type) {
				_this->pEnv->runtimeContext.taintedRegisters[GetFundamentalRegister(_this->current->operands[opIdx].asAddress->base.name)] = (DWORD)symbolicValue;
			} else {
				MarkAddr(_this->pEnv, _this->opBase[-(_this->addressOffsets[opIdx])], (DWORD)symbolicValue, 0);
			}
			return true;

	}

	return false;
}

bool UnsetOperand(void *ctx, rev::BYTE opIdx) {
	return SetOperand(ctx, opIdx, nullptr);
}

void SetFlgValue(void *ctx, rev::BYTE flg, void *symbolicValue) {
	SymbolicEnvironmentImpl *_this = (SymbolicEnvironmentImpl *)ctx;
	DWORD flgIdx = BinLog2(flg);

	_this->pEnv->runtimeContext.taintedFlags[flgIdx] = (DWORD)symbolicValue;
}

void UnsetFlgValue(void *ctx, rev::BYTE flg) {
	SetFlgValue(ctx, flg, nullptr);
}

void SetRegValue(void *ctx, RiverRegister reg, void *symbolicValue) {
	SymbolicEnvironmentImpl *_this = (SymbolicEnvironmentImpl *)ctx;
}

void UnsetRegValue(void *ctx, RiverRegister reg) {
	SymbolicEnvironmentImpl *_this = (SymbolicEnvironmentImpl *)ctx;

}


SymbolicEnvironmentFuncs sFuncs = {
	GetOperand,
	GetFlgValue,
	SetOperand,
	UnsetOperand,
	SetFlgValue,
	UnsetFlgValue,
	SetRegValue,
	UnsetRegValue
};


void SymbolicEnvironmentImpl::Init(ExecutionEnvironment *env) {
	pEnv = env;
	current = nullptr;
	ops = &sFuncs;
}