#include "OverlappedRegisters.h"

// offsets from the lsb
const nodep::DWORD OverlappedRegistersEnvironment::OverlappedRegister::rOff[5] = { 0, 0, 16, 0, 8 };
const nodep::DWORD OverlappedRegistersEnvironment::OverlappedRegister::rSize[5] = { 32, 16, 16, 8, 8 };
const nodep::DWORD OverlappedRegistersEnvironment::OverlappedRegister::rParent[5] = { 0xFF, 0, 0, 1, 1 };
const nodep::DWORD OverlappedRegistersEnvironment::OverlappedRegister::rLChild[5] = { 1, 3, 0xFF, 0xFF, 0xFF };
const nodep::DWORD OverlappedRegistersEnvironment::OverlappedRegister::rMChild[5] = { 2, 4, 0xFF, 0xFF, 0xFF };
const nodep::DWORD OverlappedRegistersEnvironment::OverlappedRegister::rSeed[4] = { 0, 1, 3, 4 };


/*
 *			[eax]
 *			  |
 *        [ax]  [?]
 *          |
 *      [al] [ah]
 */

nodep::DWORD OverlappedRegistersEnvironment::OverlappedRegister::needConcat = 0xdeadbeef;
nodep::DWORD OverlappedRegistersEnvironment::OverlappedRegister::needExtract = 0xdeadbeef;

static nodep::BYTE _GetFundamentalRegister(nodep::BYTE reg) {
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

void OverlappedRegistersEnvironment::OverlappedRegister::MarkNeedExtract(nodep::DWORD node, bool doRefCount) {
	nodep::DWORD c = rLChild[node];
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

void OverlappedRegistersEnvironment::OverlappedRegister::MarkUnset(nodep::DWORD node, bool doRefCount) {
	nodep::DWORD c = rLChild[node];
	if (c != 0xFF) {
		if ((doRefCount) && (nullptr != subRegs[c]) && (&needExtract != subRegs[c]) && (&needConcat != subRegs[c])) {
			parent->decRefFunc(subRegs[c]);
		}
		
		subRegs[c] = nullptr;
		MarkUnset(c, doRefCount);
	}

	c = rMChild[node];
	if (c != 0xFF) {
		if ((doRefCount) && (nullptr != subRegs[c]) && (&needExtract != subRegs[c]) && (&needConcat != subRegs[c])) {
			parent->decRefFunc(subRegs[c]);
		}
		
		subRegs[c] = nullptr;
		MarkUnset(c, doRefCount);
	}
}

void *OverlappedRegistersEnvironment::OverlappedRegister::Get(nodep::DWORD node, nodep::DWORD concreteValue) {
	if (subRegs[node] == &needExtract) {
		nodep::DWORD c;
		c = node;
		while ((c != 0xFF) && (subRegs[c] == &needExtract)) {
			c = rParent[c];
		}

		if (c == 0xFF) {
			DEBUG_BREAK;
		}

		return parent->exec->ExtractBits(
			subRegs[c],
			rOff[node],
			rSize[node]
		);
	}
	else if (subRegs[node] == &needConcat) {
		nodep::DWORD msz = rSize[rMChild[node]];

		nodep::DWORD msk = (1 << msz) - 1;
		nodep::DWORD mVal = (concreteValue >> msz) & msk;
		nodep::DWORD lVal = concreteValue & msk;

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

void *OverlappedRegistersEnvironment::OverlappedRegister::Get(RiverRegister &reg, nodep::DWORD &concreteValue) {
	nodep::BYTE idx = (reg.name >> 3);

	if (idx > 3) {
		DEBUG_BREAK; // do not handle special registers yet
	}

	nodep::DWORD seed = rSeed[idx];
	void *ret = Get(seed, concreteValue);

	concreteValue >>= rOff[idx];
	concreteValue &= ((1 << (rSize[idx] - 1)) << 1) - 1;

	return ret;
}

// recursive function that should mark node parent for needConcat
//
void OverlappedRegistersEnvironment::OverlappedRegister::MarkNeedConcat(nodep::DWORD node, bool doRefCount) {
	// is symbolic or needExtract
	if ((nullptr != subRegs[node]) && (&needConcat != subRegs[node])) {
		// if it has child and child needs extract
		if ((0xFF != rLChild[node]) && (&needExtract == subRegs[rLChild[node]])) {
			void *symbolicValue = subRegs[node];
			// cannot be needConcat
			// xX lowest 16bit reg matches this requirement
			if (&needExtract == symbolicValue) {
				symbolicValue = parent->exec->ExtractBits(subRegs[rParent[node]],
						rOff[node],
						rSize[node]);
				subRegs[node] = symbolicValue;
			}
			subRegs[rLChild[node]] = parent->exec->ExtractBits(
				subRegs[node],
				rOff[rLChild[node]],
				rSize[rLChild[node]]
			);

			if (doRefCount) {
				parent->addRefFunc(subRegs[rLChild[node]]);
			}
		}

		if ((0xFF != rMChild[node]) && (&needExtract == subRegs[rMChild[node]])) {
			void *symbolicValue = subRegs[node];
			// cannot be needConcat
			// xX lowest 16bit reg matches this requirement
			if (&needExtract == symbolicValue) {
				symbolicValue = parent->exec->ExtractBits(subRegs[rParent[node]],
						rOff[node],
						rSize[node]);
				subRegs[node] = symbolicValue;
			}
			subRegs[rMChild[node]] = parent->exec->ExtractBits(
				subRegs[node],
				rOff[rMChild[node]],
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

	// Possible bug
	if (0xFF != rParent[node]) {
		MarkNeedConcat(rParent[node], doRefCount);
	}
	subRegs[node] = &needConcat;
}

void OverlappedRegistersEnvironment::OverlappedRegister::Set(RiverRegister &reg, void *value, bool doRefCount) {
	nodep::BYTE idx = (reg.name >> 3);

	if (idx > 3) {
		DEBUG_BREAK; // do not handle special registers yet
	}

	nodep::DWORD seed = rSeed[idx];

	// set the current register
	if (doRefCount) {
		if ((nullptr != subRegs[seed]) && (&needExtract != subRegs[seed]) && (&needConcat != subRegs[seed])) {
			parent->decRefFunc(subRegs[seed]);
		}
		if (parent->addRefFunc == nullptr)
			DEBUG_BREAK;
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
	nodep::BYTE idx = (reg.name >> 3);

	if (idx > 3) {
		DEBUG_BREAK; // do not handle special registers yet
	}

	nodep::DWORD seed = rSeed[idx];
	// set the current register
	subRegs[seed] = nullptr;

	// unset parents if both children are unset
	for (nodep::DWORD c = rParent[seed]; c != 0xFF; c = rParent[c]) {
		if ((nullptr == subRegs[rMChild[c]]) && (nullptr == subRegs[rLChild[c]])) {
			subRegs[c] = nullptr;
		}
		else {
			// if parent is marked needConcat, all needExtract children must be resolved
			MarkNeedConcat(c, doRefCount);
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

bool OverlappedRegistersEnvironment::GetOperand(struct OperandInfo &opInfo) {
	//return subEnv->GetOperand(opIdx, isTracked, concreteValue, symbolicValue);

	opInfo.symbolic = nullptr; 
	switch (RIVER_OPTYPE(current->opTypes[opInfo.opIdx])) {
		case RIVER_OPTYPE_REG :
			if (!subEnv->GetOperand(opInfo)) {
				return false;
			}

			if (opInfo.isTracked) {
				opInfo.symbolic = ((OverlappedRegister *)opInfo.symbolic)->Get(
						current->operands[opInfo.opIdx].asRegister, opInfo.concrete);
			}
			return true;

		case RIVER_OPTYPE_MEM :
			if (0 == current->operands[opInfo.opIdx].asAddress->type) {
				if (!subEnv->GetOperand(opInfo)) {
					return false;
				}

				if (opInfo.isTracked) {
					opInfo.symbolic = ((OverlappedRegister *)opInfo.symbolic)->Get(
							current->operands[opInfo.opIdx].asAddress->base, opInfo.concrete);
					opInfo.isTracked = (opInfo.symbolic != nullptr);
				}

				return true;
			}

			// no break/return on purpose
		default :
			return subEnv->GetOperand(opInfo);
	};
}

bool OverlappedRegistersEnvironment::SetOperand(nodep::BYTE opIdx, void *symbolicValue, bool doRefCount) {

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

bool OverlappedRegistersEnvironment::UnsetOperand(nodep::BYTE opIdx, bool doRefCount) {
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


