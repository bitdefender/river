#include "OverlappedRegisters.h"

// offsets from the lsb
const rev::DWORD OverlappedRegistersEnvironment::OverlappedRegister::rOff[5] = { 0, 0, 16, 0, 8 };
const rev::DWORD OverlappedRegistersEnvironment::OverlappedRegister::rSize[5] = { 32, 16, 16, 8, 8 };
const rev::DWORD OverlappedRegistersEnvironment::OverlappedRegister::rParent[5] = { 0xFF, 0, 0, 1, 1 };
const rev::DWORD OverlappedRegistersEnvironment::OverlappedRegister::rLChild[5] = { 1, 3, 0xFF, 0xFF, 0xFF };
const rev::DWORD OverlappedRegistersEnvironment::OverlappedRegister::rMChild[5] = { 2, 4, 0xFF, 0xFF, 0xFF };
const rev::DWORD OverlappedRegistersEnvironment::OverlappedRegister::rSeed[4] = { 0, 1, 3, 4 };

rev::DWORD OverlappedRegistersEnvironment::OverlappedRegister::needConcat = 0xdeadbeef;
rev::DWORD OverlappedRegistersEnvironment::OverlappedRegister::needExtract = 0xdeadbeef;

static rev::BYTE GetFundamentalRegister(rev::BYTE reg) {
	if (reg < 0x20) {
		return reg & 0x07;
	}
	return reg;
}

void OverlappedRegistersEnvironment::OverlappedRegister::MarkNeedExtract(rev::DWORD node) {
	if (rLChild[node] != 0xFF) {
		subRegs[rLChild[node]] = &needExtract;
		MarkNeedExtract(rLChild[node]);
	}

	if (rMChild[node] != 0xFF) {
		subRegs[rMChild[node]] = &needExtract;
		MarkNeedExtract(rMChild[node]);
	}
}

void OverlappedRegistersEnvironment::OverlappedRegister::MarkUnset(rev::DWORD node) {
	if (rLChild[node] != 0xFF) {
		subRegs[rLChild[node]] = nullptr;
		MarkNeedExtract(rLChild[node]);
	}

	if (rMChild[node] != 0xFF) {
		subRegs[rMChild[node]] = nullptr;
		MarkNeedExtract(rMChild[node]);
	}
}

void *OverlappedRegistersEnvironment::OverlappedRegister::Get(rev::DWORD node, rev::DWORD concreteValue) {
	if (subRegs[node] == &needExtract) {
		rev::DWORD c;
		c = node;
		while ((c != 0xFF) && (subRegs[c] != &needExtract)) {
			c = rParent[c];
		}

		if (c == 0xFF) {
			__asm int 3;
		}

		return parent->ExtractBits(
			subRegs[c],
			rOff[node] << 3,
			rSize[node]
		);
	}
	else if (subRegs[node] == &needConcat) {
		rev::DWORD msz = rSize[rMChild[node]];

		rev::DWORD msk = (1 << msz) - 1;
		rev::DWORD mVal = (concreteValue >> msz) & msk;
		rev::DWORD lVal = concreteValue & msk;

		void *ms = Get(rMChild[node], mVal);
		if (nullptr == ms) {
			ms = parent->MakeConst(
				mVal,
				msz
			);
		}

		void *ls = Get(rLChild[node], lVal);
		if (nullptr == ls) {
			ls = parent->MakeConst(
				lVal,
				msz
			);
		}

		return parent->ConcatBits(ms, ls);
	}
	else {
		return subRegs[node];
	}
}

void OverlappedRegistersEnvironment::OverlappedRegister::SetParent(sym::SymbolicExecutor *p) {
	parent = p;
}

void *OverlappedRegistersEnvironment::OverlappedRegister::Get(RiverRegister &reg, rev::DWORD &concreteValue) {
	rev::BYTE idx = (reg.name >> 3);

	if (idx > 3) {
		__asm int 3; // do not handle special registers yet
	}

	rev::DWORD seed = rSeed[idx];
	void *ret = Get(seed, concreteValue);

	concreteValue >>= rOff[idx];
	concreteValue &= (1 << rSize[idx]) - 1;

	return ret;
}

void OverlappedRegistersEnvironment::OverlappedRegister::Set(RiverRegister &reg, void *value) {
	rev::BYTE idx = (reg.name >> 3);

	if (idx > 3) {
		__asm int 3; // do not handle special registers yet
	}

	rev::DWORD seed = rSeed[idx];
	// set the current register
	subRegs[seed] = value;

	// mark parents as needing concat
	for (rev::DWORD c = rParent[seed]; c != 0xFF; c = rParent[c]) {
		subRegs[c] = &needConcat;
	}

	// mark children as needing extract
	MarkNeedExtract(seed);
}

bool OverlappedRegistersEnvironment::OverlappedRegister::Unset(RiverRegister &reg) {
	rev::BYTE idx = (reg.name >> 3);

	if (idx > 3) {
		__asm int 3; // do not handle special registers yet
	}

	rev::DWORD seed = rSeed[idx];
	// set the current register
	subRegs[seed] = nullptr;

	// unset parents if both children are unset
	for (rev::DWORD c = rParent[seed]; c != 0xFF; c = rParent[c]) {
		if ((nullptr == subRegs[rMChild[c]]) || (nullptr == subRegs[rLChild[c]])) {
			subRegs[c] = nullptr;
		}
		else {
			subRegs[c] = &needConcat;
		}
	}

	// unset children as well
	MarkUnset(seed);

	return (subRegs[0] == nullptr);
}

void OverlappedRegistersEnvironment::OverlappedRegister::SaveState(stk::LargeStack &stack) {
	for (int i = 0; i < 5; ++i) {
		stack.Push((DWORD)subRegs[i]);
	}
}

void OverlappedRegistersEnvironment::OverlappedRegister::LoadState(stk::LargeStack &stack) {
	for (int i = 4; i >= 0; ++i) {
		subRegs[i] = (void *)stack.Pop();
	}
}

bool OverlappedRegistersEnvironment::_SetCurrentInstruction(RiverInstruction *instruction, void *opBuffer) {
	current = instruction;
	return true;
}

void OverlappedRegistersEnvironment::_PushState(stk::LargeStack &stack) {
	for (int i = 0; i < 8; ++i) {
		subRegisters[i].SaveState(stack);
	}
}

void OverlappedRegistersEnvironment::_PopState(stk::LargeStack &stack) {
	for (int i = 7; i >= 0; --i) {
		subRegisters[i].LoadState(stack);
	}
}

bool OverlappedRegistersEnvironment::GetOperand(rev::BYTE opIdx, rev::BOOL &isTracked, rev::DWORD &concreteValue, void *&symbolicValue) {
	//return subEnv->GetOperand(opIdx, isTracked, concreteValue, symbolicValue);

	OverlappedRegister *reg;
	
	switch (RIVER_OPTYPE(current->opTypes[opIdx])) {
		case RIVER_OPTYPE_REG :
			if (!subEnv->GetOperand(opIdx, isTracked, concreteValue, (void *&)reg)) {
				return false;
			}

			if (isTracked) {
				symbolicValue = reg->Get(current->operands[opIdx].asRegister, concreteValue);
			}

			return true;

		case RIVER_OPTYPE_MEM :
			if (0 == current->operands[opIdx].asAddress->type) {
				if (!subEnv->GetOperand(opIdx, isTracked, concreteValue, (void *&)reg)) {
					return false;
				}

				if (isTracked) {
					symbolicValue = reg->Get(current->operands[opIdx].asAddress->base, concreteValue);
				}

				return true;
			}

			// no break/return on purpose
		default :
			return subEnv->GetOperand(opIdx, isTracked, concreteValue, symbolicValue);
	};
}

bool OverlappedRegistersEnvironment::SetOperand(rev::BYTE opIdx, void *symbolicValue) {

	OverlappedRegister *reg;

	switch (RIVER_OPTYPE(current->opTypes[opIdx])) {
		case RIVER_OPTYPE_REG :
			reg = &subRegisters[GetFundamentalRegister(current->operands[opIdx].asRegister.name)];
			reg->Set(current->operands[opIdx].asRegister, symbolicValue);
			
			return subEnv->SetOperand(opIdx, reg);

		case RIVER_OPTYPE_MEM :
			if (0 == current->operands[opIdx].asAddress->type) {
				reg = &subRegisters[GetFundamentalRegister(current->operands[opIdx].asAddress->base.name)];
				reg->Set(current->operands[opIdx].asAddress->base, symbolicValue);

				return subEnv->SetOperand(opIdx, reg);
			}

			// no break/return on purpose
		default :
			return subEnv->SetOperand(opIdx, symbolicValue);
	}
}

bool OverlappedRegistersEnvironment::UnsetOperand(rev::BYTE opIdx) {
	OverlappedRegister *reg;

	switch (RIVER_OPTYPE(current->opTypes[opIdx])) {
	case RIVER_OPTYPE_REG:
		reg = &subRegisters[GetFundamentalRegister(current->operands[opIdx].asRegister.name)];
		if (reg->Unset(current->operands[opIdx].asRegister)) {
			return subEnv->UnsetOperand(opIdx);
		}
		return true;

	case RIVER_OPTYPE_MEM:
		if (0 == current->operands[opIdx].asAddress->type) {
			reg = &subRegisters[GetFundamentalRegister(current->operands[opIdx].asAddress->base.name)];
			if (reg->Unset(current->operands[opIdx].asAddress->base)) {
				return subEnv->UnsetOperand(opIdx);
			}
			return true;
		}

		// no break on purpose
	default :
		return subEnv->UnsetOperand(opIdx);
	}
}


