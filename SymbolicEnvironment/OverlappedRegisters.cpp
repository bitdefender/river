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

static rev::BYTE _GetFundamentalRegister(rev::BYTE reg) {
	if (reg < 0x20) {
		return reg & 0x07;
	}
	return reg;
}

OverlappedRegistersEnvironment::OverlappedRegister::OverlappedRegister() {
	for (int i = 0; i < 5; ++i) {
		subRegs[i] = nullptr;
	}
}

void OverlappedRegistersEnvironment::OverlappedRegister::MarkNeedExtract(rev::DWORD node, bool doRefCount) {
	rev::DWORD c = rLChild[node];
	if (c != 0xFF) {
		if ((doRefCount) && (nullptr != subRegs[c]) && (&needExtract != subRegs[c]) && (&needConcat != subRegs[c])) {
			parent->decRefFunc(subRegs[c]);
		}
		subRegs[c] = &needExtract;
		MarkNeedExtract(c, doRefCount);
	}

	c = rMChild[node];
	if (c != 0xFF) {
		if ((doRefCount) && (nullptr != subRegs[c]) && (&needExtract != subRegs[c]) && (&needConcat != subRegs[c])) {
			parent->decRefFunc(subRegs[c]);
		}
		subRegs[c] = &needExtract;
		MarkNeedExtract(c, doRefCount);
	}
}

void OverlappedRegistersEnvironment::OverlappedRegister::MarkUnset(rev::DWORD node, bool doRefCount) {
	rev::DWORD c = rLChild[node]; 
	if (c != 0xFF) {
		if ((doRefCount) && (nullptr != subRegs[c]) && (&needExtract != subRegs[c]) && (&needConcat != subRegs[c])) {
			parent->decRefFunc(subRegs[c]);
		}
		
		subRegs[c] = nullptr;
		MarkNeedExtract(c, doRefCount);
	}

	c = rMChild[node];
	if (c != 0xFF) {
		if ((doRefCount) && (nullptr != subRegs[c]) && (&needExtract != subRegs[c]) && (&needConcat != subRegs[c])) {
			parent->decRefFunc(subRegs[c]);
		}
		
		subRegs[c] = nullptr;
		MarkNeedExtract(c, doRefCount);
	}
}

void *OverlappedRegistersEnvironment::OverlappedRegister::Get(rev::DWORD node, rev::DWORD concreteValue) {
	if (subRegs[node] == &needExtract) {
		rev::DWORD c;
		c = node;
		while ((c != 0xFF) && (subRegs[c] == &needExtract)) {
			c = rParent[c];
		}

		if (c == 0xFF) {
			DEBUG_BREAK;
		}

		return parent->exec->ExtractBits(
			subRegs[c],
			rOff[node] >> 3,
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
			ms = parent->exec->MakeConst(
				mVal,
				msz
			);
		}

		void *ls = Get(rLChild[node], lVal);
		if (nullptr == ls) {
			ls = parent->exec->MakeConst(
				lVal,
				msz
			);
		}

		return parent->exec->ConcatBits(ms, ls);
	}
	else {
		return subRegs[node];
	}
}

void OverlappedRegistersEnvironment::OverlappedRegister::SetParent(OverlappedRegistersEnvironment *p) {
	parent = p;
}

void *OverlappedRegistersEnvironment::OverlappedRegister::Get(RiverRegister &reg, rev::DWORD &concreteValue) {
	rev::BYTE idx = (reg.name >> 3);

	if (idx > 3) {
		DEBUG_BREAK; // do not handle special registers yet
	}

	rev::DWORD seed = rSeed[idx];
	void *ret = Get(seed, concreteValue);

	concreteValue >>= rOff[idx];
	concreteValue &= (1 << rSize[idx]) - 1;

	return ret;
}

void OverlappedRegistersEnvironment::OverlappedRegister::MarkNeedConcat(rev::DWORD node, bool doRefCount) {
	if (&needExtract == subRegs[node]) {
		MarkNeedConcat(rParent[node], doRefCount);
	}

	if ((nullptr != subRegs[node]) && (&needConcat != subRegs[node])) {
		if ((0xFF != rLChild[node]) && (&needExtract == subRegs[rLChild[node]])) {
			subRegs[rLChild[node]] = parent->exec->ExtractBits(
				subRegs[node],
				rOff[rLChild[node]] >> 3,
				rSize[rLChild[node]]
			);

			if (doRefCount) {
				parent->addRefFunc(subRegs[rLChild[node]]);
			}
		}

		if ((0xFF != rMChild[node]) && (&needExtract == subRegs[rMChild[node]])) {
			subRegs[rMChild[node]] = parent->exec->ExtractBits(
				subRegs[node],
				rOff[rMChild[node]] >> 3,
				rSize[rMChild[node]]
			);

			if (doRefCount) {
				parent->addRefFunc(subRegs[rMChild[node]]);
			}
		}

		if (doRefCount) {
			parent->decRefFunc(subRegs[node]);
		}
	}

	subRegs[node] = &needConcat;
}

void OverlappedRegistersEnvironment::OverlappedRegister::Set(RiverRegister &reg, void *value, bool doRefCount) {
	rev::BYTE idx = (reg.name >> 3);

	if (idx > 3) {
		DEBUG_BREAK; // do not handle special registers yet
	}

	rev::DWORD seed = rSeed[idx];

	// set the current register
	if (doRefCount) {
		if ((nullptr != subRegs[seed]) && (&needExtract != subRegs[seed]) && (&needConcat != subRegs[seed])) {
			parent->decRefFunc(subRegs[seed]);
		}
		parent->addRefFunc(value);
	}
	subRegs[seed] = value;

	// mark parents as needing concat
	/*for (rev::DWORD c = rParent[seed]; c != 0xFF; c = rParent[c]) {
		if ((doRefCount) && (nullptr != subRegs[c]) && (&needExtract != subRegs[c]) && (&needConcat != subRegs[c])) {
			parent->decRefFunc(subRegs[c]);
		}
		subRegs[c] = &needConcat;
	}*/
	if (0xFF != rParent[seed]) {
		MarkNeedConcat(rParent[seed], doRefCount);
	}

	// mark children as needing extract
	MarkNeedExtract(seed, doRefCount);
}

bool OverlappedRegistersEnvironment::OverlappedRegister::Unset(RiverRegister &reg, bool doRefCount) {
	rev::BYTE idx = (reg.name >> 3);

	if (idx > 3) {
		DEBUG_BREAK; // do not handle special registers yet
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
	MarkUnset(seed, doRefCount);

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

void OverlappedRegistersEnvironment::_SetReferenceCounting(AddRefFunc addRef, DecRefFunc decRef) {
	addRefFunc = addRef;
	decRefFunc = decRef;
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

OverlappedRegistersEnvironment::OverlappedRegistersEnvironment() {
	for (int i = 7; i >= 0; --i) {
		subRegisters[i].SetParent(this);
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

bool OverlappedRegistersEnvironment::SetOperand(rev::BYTE opIdx, void *symbolicValue, bool doRefCount) {

	OverlappedRegister *reg;

	switch (RIVER_OPTYPE(current->opTypes[opIdx])) {
		case RIVER_OPTYPE_REG :
			reg = &subRegisters[_GetFundamentalRegister(current->operands[opIdx].asRegister.name)];
			reg->Set(current->operands[opIdx].asRegister, symbolicValue, doRefCount);
			
			return subEnv->SetOperand(opIdx, reg, false);

		case RIVER_OPTYPE_MEM :
			if (0 == current->operands[opIdx].asAddress->type) {
				reg = &subRegisters[_GetFundamentalRegister(current->operands[opIdx].asAddress->base.name)];
				reg->Set(current->operands[opIdx].asAddress->base, symbolicValue, doRefCount);

				return subEnv->SetOperand(opIdx, reg, false);
			}

			// no break/return on purpose
		default :
			return subEnv->SetOperand(opIdx, symbolicValue, doRefCount);
	}
}

bool OverlappedRegistersEnvironment::UnsetOperand(rev::BYTE opIdx, bool doRefCount) {
	OverlappedRegister *reg;

	switch (RIVER_OPTYPE(current->opTypes[opIdx])) {
	case RIVER_OPTYPE_REG:
		reg = &subRegisters[_GetFundamentalRegister(current->operands[opIdx].asRegister.name)];
		if (reg->Unset(current->operands[opIdx].asRegister, doRefCount)) {
			return subEnv->UnsetOperand(opIdx, false);
		}
		return true;

	case RIVER_OPTYPE_MEM:
		if (0 == current->operands[opIdx].asAddress->type) {
			reg = &subRegisters[_GetFundamentalRegister(current->operands[opIdx].asAddress->base.name)];
			if (reg->Unset(current->operands[opIdx].asAddress->base, doRefCount)) {
				return subEnv->UnsetOperand(opIdx, false);
			}
			return true;
		}

		// no break on purpose
	default :
		return subEnv->UnsetOperand(opIdx, doRefCount);
	}
}


