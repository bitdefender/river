#include "CodeGen.h"

#include "common.h"
#include "cb.h"
#include "mm.h"
#include "modrm32.h"
#include "crc32.h"
#include "revtracer.h"

#include "river.h"

#include "RiverX86Disassembler.h"
#include "RiverReverseTranslator.h"
#include "RiverSaveTranslator.h"

RiverCodeGen::RiverCodeGen() {
	outBufferSize = 0;
	outBuffer = NULL;
	heap = NULL;
}

RiverCodeGen::~RiverCodeGen() {
	if (0 != outBufferSize) {
		Destroy();
	}
}

bool RiverCodeGen::Init(RiverHeap *hp, RiverRuntime *rt, nodep::DWORD buffSz, nodep::DWORD dwTranslationFlags) {
	heap = hp;
	if (NULL == (outBuffer = (unsigned char *)revtracerImports.memoryAllocFunc(buffSz))) {
		return false;
	}
	outBufferSize = buffSz;
	codeBuffer.Init(outBuffer);

	disassembler.Init(this);

	metaTranslator.Init(this);

	repTranslator.Init(this);

	revTranslator.Init(this);
	saveTranslator.Init(this);

	symbopTranslator.Init(this);
	symbopSaveTranslator.Init(this);
	symbopReverseTranslator.Init(this);

	return assembler.Init(rt, dwTranslationFlags);
}

bool RiverCodeGen::Destroy() {
	revtracerImports.memoryFreeFunc(outBuffer);
	outBuffer = NULL;
	outBufferSize = 0;
	heap = NULL;
	return true;
}

void RiverCodeGen::Reset() {
	addrCount = trInstCount = fwInstCount = bkInstCount = 0;
	symbopInstCount = 0;
	rev_memset(regVersions, 0, sizeof(regVersions));
}


static int callCount = 0;
struct RiverAddress *RiverCodeGen::AllocAddr(nodep::WORD flags) {
	struct RiverAddress *ret = &trRiverAddr[addrCount];
	callCount++;
	addrCount++;
	return ret;
}

RiverAddress *RiverCodeGen::CloneAddress(const RiverAddress &mem, nodep::WORD flags) {
	struct RiverAddress *ret = AllocAddr(flags);
	rev_memcpy(ret, &mem, sizeof(*ret));
	return ret;
}


unsigned int RiverCodeGen::GetCurrentReg(unsigned char regName) const {
	if (RIVER_REG_NONE == (regName)) return regName;
	nodep::BYTE rTmp = regName & 0x07;
	return regVersions[rTmp] | regName;
}

unsigned int RiverCodeGen::GetPrevReg(unsigned char regName) const {
	if (RIVER_REG_NONE == (regName)) return regName;
	nodep::BYTE rTmp = regName & 0x07;
	return (regVersions[rTmp] - 0x100) | regName;
}

unsigned int RiverCodeGen::NextReg(unsigned char regName) {
	regVersions[regName] += 0x100;
	return GetCurrentReg(regName);
}




nodep::DWORD dwTransLock = 0;

extern "C" {
	void __stdcall BranchHandler(struct ExecutionEnvironment *pEnv, ADDR_TYPE a);
	void __stdcall SysHandler(struct ExecutionEnvironment *pEnv);
};

nodep::DWORD dwSysHandler    = (nodep::DWORD) ::SysHandler;
nodep::DWORD dwBranchHandler = (nodep::DWORD) ::BranchHandler;

unsigned char *DuplicateBuffer(RiverHeap *h, unsigned char *p, unsigned int sz) {
	unsigned int mSz = (sz + 0x0F) & ~0x0F;
	
	unsigned char *pBuf = (unsigned char *)h->Alloc(mSz);
	if (pBuf == NULL) {
		return NULL;
	}

	rev_memcpy(pBuf, p, sz);
	rev_memset(pBuf + sz, 0x90, mSz - sz);
	return pBuf;
}

unsigned char *ConsolidateBlock(RiverHeap *h, unsigned char *outBuff, unsigned int outSz, unsigned char *saveBuff, unsigned int saveSz) {
	unsigned int mSz = (outSz + saveSz + 0x0F) & (~0x0F);

	unsigned char *pBuf = (unsigned char *)h->Alloc(mSz);
	if (NULL == pBuf) {
		return NULL;
	}

	rev_memcpy(pBuf, saveBuff, saveSz);
	rev_memcpy(&pBuf[saveSz], outBuff, outSz);
	rev_memset(&pBuf[saveSz + outSz], 0x90, mSz - outSz - saveSz);
	return pBuf;
}

void MakeJMP(struct RiverInstruction *ri, nodep::DWORD jmpAddr) {
	ri->modifiers = 0;
	ri->specifiers = 0;
	ri->family = 0;
	ri->opCode = 0xE9;
	ri->opTypes[0] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_32;
	ri->operands[0].asImm32 = jmpAddr;

	ri->opTypes[1] = RIVER_OPTYPE_IMM | RIVER_OPSIZE_32;
	ri->operands[1].asImm32 = 0;
}

void RiverPrintInstruction(nodep::DWORD printMask, RiverInstruction *ri);

bool RiverCodeGen::DisassembleSingle(nodep::BYTE *&px86, RiverInstruction *rOut, nodep::DWORD &count, nodep::DWORD &dwFlags, RevtracerError *rerror) {
	bool ret = true;
	RiverInstruction dis;

	ret = disassembler.Translate(px86, dis, dwFlags);
	rerror->instructionAddress = dis.instructionAddress;
	rerror->prefix = (dis.modifiers & RIVER_MODIFIER_EXT) ? 0x0F : 0x00;
	rerror->opcode = *px86;

	if (!ret) {
		rerror->errorCode = RERROR_UNK_INSTRUCTION;
		rerror->instructionAddress = dis.instructionAddress;
		rerror->translatorId = RIVER_DISASSEMBLER_ID;
		return ret;
	}

	nodep::DWORD localRepCount = 0;
	RiverInstruction localInstrBuffer[16];
	ret = repTranslator.Translate(dis,
			(RiverInstruction *)&localInstrBuffer,
			localRepCount);

	if (!ret) {
		rerror->errorCode = RERROR_UNK_INSTRUCTION;
		rerror->instructionAddress = dis.instructionAddress;
		rerror->translatorId = RIVER_REP_TRANSLATOR_ID;
		return ret;
	}

	for (unsigned i = 0; i < localRepCount; ++i) {
		nodep::DWORD localMetaCount = 0;
		ret = metaTranslator.Translate(localInstrBuffer[i], rOut + count, localMetaCount);
		count += localMetaCount;

		if (!ret) {
			rerror->errorCode = RERROR_UNK_INSTRUCTION;
			rerror->translatorId = RIVER_META_TRANSLATOR_ID;
			rerror->instructionAddress = dis.instructionAddress;
			return ret;
		}
	}

	rerror->errorCode = RERROR_OK;
	rerror->translatorId = RIVER_NONE_ID;
	rerror->instructionAddress = dis.instructionAddress;
	return ret;
}

nodep::DWORD RiverCodeGen::TranslateBasicBlock(nodep::BYTE *px86,
		nodep::DWORD &dwInst, nodep::BYTE *&disasm,
		nodep::DWORD dwTranslationFlags, nodep::DWORD *disassFlags,
		struct rev::BranchNext *next, RevtracerError *rerror) {
	bool ret;
	nodep::BYTE *pTmp = px86;
	nodep::DWORD pFlags = 0;

	TRANSLATE_PRINT(PRINT_INFO | PRINT_DISASSEMBLY, "= x86 to river ================================================================\n");

	//RiverInstruction dis;
	//RiverInstruction symbopMain[16]; // , symbopTrack[16];

	RiverInstruction instrBuffers[3][16];
	nodep::DWORD instrCounts[3], currentBuffer;

	//nodep::DWORD svCount, metaCount; // , trackCount;

	do {
		nodep::BYTE *pAux = pTmp;
		nodep::DWORD iSize;

		currentBuffer = 0;
		instrCounts[currentBuffer] = 0;

		//disassembler.Translate(pTmp, dis, pFlags);
		//metaTranslator.Translate(dis, instrBuffers[currentBuffer], instrCounts[currentBuffer]);

		RiverInstruction *ri = instrBuffers[currentBuffer];
		DisassembleSingle(pTmp, ri, instrCounts[currentBuffer], pFlags, rerror);

		// set disassemble flags when branch instruction is found
		if (pFlags & RIVER_FLAG_BRANCH) {
			*disassFlags = pFlags;
			rev_memset(next, 0, 2 * sizeof(struct BranchNext));

			// get last instruction from translation array
			RiverInstruction *tmpRi = ri + instrCounts[currentBuffer] - 1;

			// set next instructions and branch taken info
			if (RIVER_OPTYPE(tmpRi->opTypes[0]) == RIVER_OPTYPE_IMM) {
				next[0].taken = true;
				next[0].address = tmpRi->operands[1].asImm32;

				switch(RIVER_OPSIZE(tmpRi->opTypes[0])) {
				case RIVER_OPSIZE_32:
					next[0].address = (unsigned)((int)next[0].address + (int)tmpRi->operands[0].asImm32);
					break;
				case RIVER_OPSIZE_16:
					next[0].address = (unsigned)((int)next[0].address + (short)tmpRi->operands[0].asImm16);
					break;
				case RIVER_OPSIZE_8:
					next[0].address = (unsigned)((int)next[0].address + (char)tmpRi->operands[0].asImm8);
					break;
				default:
					DEBUG_BREAK;
				}

				if (pFlags & RIVER_BRANCH_INSTR_JXX) {
					next[1].taken = false;
					next[1].address = tmpRi->operands[1].asImm32;
				}
			} else if (RIVER_OPTYPE(tmpRi->opTypes[0]) == RIVER_OPTYPE_MEM ||
					RIVER_OPTYPE(tmpRi->opTypes[0]) == RIVER_OPTYPE_REG) {
				next[0].taken = true;
				next[0].address = 0x00000000;
			}
		}

		if (rerror->errorCode != RERROR_OK) {
			return 0;
		}

		nodep::DWORD addrCount = 0;

		for (nodep::DWORD i = 0; i < instrCounts[currentBuffer]; ++i) {
			for (nodep::DWORD j = 0; j < 4; ++j) {
				if (RIVER_OPTYPE(instrBuffers[currentBuffer][i].opTypes[j]) == RIVER_OPTYPE_MEM) {
					addrCount++;
				}
			}
		}
		RiverAddress *serialAddress = nullptr;
		RiverInstruction *serialInstr = (RiverInstruction *)heap->Alloc(
			instrCounts[currentBuffer] * sizeof(serialInstr[0])
			+ addrCount * sizeof(serialAddress[0])
		); // put head of buffer here
		serialAddress = (RiverAddress *)&serialInstr[instrCounts[currentBuffer]];

		addrCount = 0;
		rev_memcpy(serialInstr, instrBuffers[currentBuffer], instrCounts[currentBuffer] * sizeof(serialInstr[0]));
		for (nodep::DWORD i = 0; i < instrCounts[currentBuffer]; ++i) {
			for (nodep::DWORD j = 0; j < 4; ++j) {
				instrBuffers[currentBuffer][i].instructionAddress = (nodep::DWORD)&serialInstr[i];

				if (RIVER_OPTYPE(serialInstr[i].opTypes[j]) == RIVER_OPTYPE_MEM) {
					rev_memcpy(&serialAddress[addrCount], serialInstr[i].operands[j].asAddress, sizeof(serialAddress[0]));
					serialInstr[i].operands[j].asAddress = &serialAddress[addrCount];
					addrCount++;
				}
			}
		}

		disasm = (unsigned char *)serialInstr;

		for (nodep::DWORD i = 0; i < instrCounts[currentBuffer]; ++i) {
			if (RIVER_FAMILY_NATIVE == RIVER_FAMILY(instrBuffers[currentBuffer][i].family)) {

				TRANSLATE_PRINT(PRINT_INFO | PRINT_DISASSEMBLY, ".%08x    ", pAux);
				iSize = pTmp - pAux;
				for (nodep::DWORD i = 0; i < iSize; ++i) {
					TRANSLATE_PRINT(PRINT_INFO | PRINT_DISASSEMBLY, "%02x ", pAux[i]);
				}

				for (nodep::DWORD i = iSize; i < 8; ++i) {
					TRANSLATE_PRINT(PRINT_INFO | PRINT_DISASSEMBLY, "   ");
				}
			}
			else {
				TRANSLATE_PRINT(PRINT_INFO | PRINT_DISASSEMBLY, ".                                    ");
			}

			TRANSLATE_PRINT_INSTRUCTION(PRINT_INFO | PRINT_DISASSEMBLY, &instrBuffers[0][i]);
		}
		currentBuffer++;

		if (TRACER_FEATURE_REVERSIBLE & dwTranslationFlags) {
			instrCounts[currentBuffer] = 0;
			for (nodep::DWORD i = 0; i < instrCounts[currentBuffer - 1]; ++i) {
				ret = saveTranslator.Translate(
						instrBuffers[currentBuffer - 1][i], 
						&instrBuffers[currentBuffer][instrCounts[currentBuffer]], 
						instrCounts[currentBuffer]
						);
				if (!ret) {
					rerror->translatorId = RIVER_SAVE_TRANSLATOR_ID;
					rerror->errorCode = RERROR_UNK_INSTRUCTION;
					return 0;
				}
			}
			currentBuffer++;
		}

		if (TRACER_FEATURE_TRACKING & dwTranslationFlags) {
			instrCounts[currentBuffer] = 0;
			for (nodep::DWORD i = 0; i < instrCounts[currentBuffer - 1]; ++i) {
				ret = symbopTranslator.Translate(
						instrBuffers[currentBuffer - 1][i], 
						&instrBuffers[currentBuffer][instrCounts[currentBuffer]], 
						instrCounts[currentBuffer], 
						&symbopInst[symbopInstCount], 
						symbopInstCount,
						dwTranslationFlags
						);
				if (!ret) {
					rerror->translatorId = RIVER_SYMBOP_TRANSLATOR_ID;
					rerror->errorCode = RERROR_UNK_INSTRUCTION;
					return 0;
				}
			}
			currentBuffer++;
		}

		rev_memcpy(&fwRiverInst[fwInstCount], instrBuffers[currentBuffer - 1], sizeof(fwRiverInst[0]) * instrCounts[currentBuffer - 1]);
		fwInstCount += instrCounts[currentBuffer - 1];

		dwInst++;
	} while (!(pFlags & RIVER_FLAG_BRANCH));

	TRANSLATE_PRINT(PRINT_INFO | PRINT_DISASSEMBLY, "===============================================================================\n");
	return pTmp - px86;
}

void GetSerializableInstruction(const RiverInstruction &rI, RiverInstruction &rO) {
	rev_memcpy(&rO, &rI, sizeof(rI));

	for (int i = 0; i < 4; ++i) {
		if (RIVER_OPTYPE(rO.opTypes[i]) == RIVER_OPTYPE_MEM) {
			rO.operands[i].asAddress = NULL;
		}
	}
}

bool SerializeInstructions(const RiverInstruction *code, int count) {
	RiverInstruction tmp;
	nodep::DWORD dwWr;
	nodep::BOOL ret;
	for (int i = 0; i < count; ++i) {
		GetSerializableInstruction(code[i], tmp);
		ret = ((rev::WriteFileCall)revtracerImports.lowLevel.ntWriteFile)(revtracerConfig.hBlocks, 0, &tmp, sizeof(tmp), &dwWr);
		if (0 == ret) {
			//debug print something
			return false;
		}

		for (int j = 0; j < 4; ++j) {
			if (RIVER_OPTYPE(code[i].opTypes[j]) == RIVER_OPTYPE_MEM) {
				ret = ((rev::WriteFileCall)revtracerImports.lowLevel.ntWriteFile)(revtracerConfig.hBlocks, 0,
					&code[i].operands[j].asAddress->type, sizeof(*code[i].operands[j].asAddress) - sizeof(void *), &dwWr);
				if (0 == ret) {
					//debug print something
					return false;
				}
			}
		}
	}
	return true;
}

bool SaveToStream(
	const RiverInstruction *forwardCode, nodep::DWORD dwFwOpCount, 
	const RiverInstruction *backwardCode, nodep::DWORD dwBkOpCount,
	const RiverInstruction *trackCode, nodep::DWORD dwTrOpCount
) {
	/*nodep::DWORD header[4] = { 'BBVR', dwFwOpCount, dwBkOpCount, dwTrOpCount };
	nodep::DWORD dwWr;
	BOOL ret;

	ret = ((rev::WriteFileCall)revtracerImports.lowLevel.ntWriteFile)(revtracerConfig.hBlocks, header, sizeof(header), &dwWr);
	if (0 == ret) {
		//debug print something
		return false;
	}

	if (
		!SerializeInstructions(forwardCode, dwFwOpCount) ||
		!SerializeInstructions(backwardCode, dwBkOpCount) ||
		!SerializeInstructions(trackCode, dwTrOpCount)
		) {
		return false;
	}*/

	return true;
}

void SetBranchInfo(RiverBasicBlock *pCB, nodep::DWORD disassFlags) {
	if (disassFlags & RIVER_BRANCH_INSTR_RET) {
		pCB->dwBranchInstruction = RIVER_INSTR_RET;
	} else if (disassFlags & RIVER_BRANCH_INSTR_JMP) {
		pCB->dwBranchInstruction = RIVER_INSTR_JMP;
	} else if (disassFlags & RIVER_BRANCH_INSTR_JXX) {
		pCB->dwBranchInstruction = RIVER_INSTR_JXX;
	} else if (disassFlags & RIVER_BRANCH_INSTR_CALL) {
		pCB->dwBranchInstruction = RIVER_INSTR_CALL;
	} else if (disassFlags & RIVER_BRANCH_INSTR_SYSCALL) {
		pCB->dwBranchInstruction = RIVER_INSTR_SYSCALL;
	}

	if (disassFlags & RIVER_BRANCH_TYPE_IMM) {
		pCB->dwBranchType = RIVER_OPTYPE_IMM;
	} else if (disassFlags & RIVER_BRANCH_TYPE_MEM) {
		pCB->dwBranchType = RIVER_OPTYPE_MEM;
	} else if (disassFlags & RIVER_BRANCH_TYPE_REG) {
		pCB->dwBranchType = RIVER_OPTYPE_REG;
	}
}

bool RiverCodeGen::Translate(RiverBasicBlock *pCB, nodep::DWORD dwTranslationFlags, RevtracerError *rerror) {
	bool ret;

	if (dwTranslationFlags & 0x80000000) {
		pCB->dwSize = 0;
		pCB->dwCRC = (nodep::DWORD)crc32(0xEDB88320, (nodep::BYTE *)pCB->address, 0);

		outBufferSize = 0;
		pCB->pCode = pCB->pFwCode = (unsigned char *)pCB->address;
		pCB->pBkCode = NULL;

		rerror->errorCode = RERROR_OK;
		return true;
	} else {

		Reset();

		nodep::DWORD disassFlags;
		pCB->dwOrigOpCount = 0;
		pCB->dwSize = TranslateBasicBlock((nodep::BYTE *)pCB->address,
				pCB->dwOrigOpCount, pCB->pDisasmCode, dwTranslationFlags,
				&disassFlags, pCB->pBranchNext, rerror);

		SetBranchInfo(pCB, disassFlags);

		if (pCB->dwSize == 0 && rerror->errorCode != RERROR_OK) {
			return false;
		}

		trInstCount += pCB->dwOrigOpCount;
		pCB->dwCRC = (nodep::DWORD)crc32(0xEDB88320, (nodep::BYTE *)pCB->address, pCB->dwSize);

		revtracerImports.dbgPrintFunc(PRINT_DEBUG, "## this: %08x\n", (nodep::DWORD)this);

		if (dwTranslationFlags & TRACER_FEATURE_REVERSIBLE) {
			// generate the reverse basic block representations
			revtracerImports.dbgPrintFunc(PRINT_DEBUG, "##Rev: %08x %d instructions\n", fwRiverInst, fwInstCount);
			for (nodep::DWORD i = 0; i < fwInstCount; ++i) {
				//TranslateReverse(this, &fwRiverInst[fwInstCount - 1 - i], &bkRiverInst[i], &tmp);
				const RiverInstruction ri = fwRiverInst[fwInstCount - 1 - i];
				ret = revTranslator.Translate(ri, bkRiverInst[i]);
				if (!ret) {
					rerror->prefix = (ri.modifiers | RIVER_MODIFIER_EXT) ? 0xFF : 0x00;
					rerror->opcode = *((nodep::BYTE*)ri.instructionAddress);
					rerror->translatorId = RIVER_REVERSE_TRANSLATOR_ID;
					rerror->instructionAddress = ri.instructionAddress;
					rerror->errorCode = RERROR_UNK_INSTRUCTION;
					return false;
				}
			}
			MakeJMP(&bkRiverInst[fwInstCount], pCB->address);
			bkInstCount = fwInstCount + 1;
		}

		if (dwTranslationFlags & TRACER_FEATURE_TRACKING) {
			TRANSLATE_PRINT(PRINT_INFO | PRINT_TRANSLATION | PRINT_TRACKING, "= SymbopTrack =================================================================\n");
			for (unsigned int i = 0; i < symbopInstCount; ++i) {
				TRANSLATE_PRINT_INSTRUCTION(PRINT_INFO | PRINT_TRANSLATION | PRINT_TRACKING, &symbopInst[i]);
			}
			TRANSLATE_PRINT(PRINT_INFO | PRINT_TRANSLATION | PRINT_TRACKING, "===============================================================================\n");

			if (dwTranslationFlags & TRACER_FEATURE_REVERSIBLE) {
				sfInstCount = 0;
				for (unsigned int i = 0; i < symbopInstCount; ++i) {
					const RiverInstruction si = symbopInst[i];
					ret = symbopSaveTranslator.Translate(si, &symbopFwRiverInst[sfInstCount], sfInstCount);
					if (!ret) {
						rerror->prefix = (si.modifiers | RIVER_MODIFIER_EXT) ? 0xFF : 0x00;
						rerror->opcode = *((nodep::BYTE*)si.instructionAddress);
						rerror->translatorId = RIVER_SYMBOP_SAVE_TRANSLATOR_ID;
						rerror->instructionAddress = si.instructionAddress;
						rerror->errorCode = RERROR_UNK_INSTRUCTION;
						return false;
					}
				}

				TRANSLATE_PRINT(PRINT_INFO | PRINT_TRANSLATION | PRINT_TRACKING | PRINT_FORWARD, "= SymbopFwRiverTrack ==========================================================\n");
				for (nodep::DWORD i = 0; i < sfInstCount; ++i) {
					TRANSLATE_PRINT_INSTRUCTION(PRINT_INFO | PRINT_TRANSLATION | PRINT_TRACKING | PRINT_FORWARD, &symbopFwRiverInst[i]);
				}
				TRANSLATE_PRINT(PRINT_INFO | PRINT_TRANSLATION | PRINT_TRACKING | PRINT_FORWARD, "===============================================================================\n");

				for (nodep::DWORD i = 0; i < sfInstCount; ++i) {
					const RiverInstruction sfri = symbopFwRiverInst[sfInstCount - 1 - i];
					ret = symbopReverseTranslator.Translate(sfri, symbopBkRiverInst[i]);
					if (!ret) {
						rerror->prefix = (sfri.modifiers | RIVER_MODIFIER_EXT) ? 0xFF : 0x00;
						rerror->opcode = *((nodep::BYTE*)sfri.instructionAddress);
						rerror->translatorId = RIVER_SYMBOP_REVERSE_TRANSLATOR_ID;
						rerror->instructionAddress = sfri.instructionAddress;
						rerror->errorCode = RERROR_UNK_INSTRUCTION;
						return false;
					}
				}
				sbInstCount = sfInstCount;

				TRANSLATE_PRINT(PRINT_INFO | PRINT_TRANSLATION | PRINT_TRACKING | PRINT_BACKWARD, "= SymbopBkRiverTrack ==========================================================\n");
				for (nodep::DWORD i = 0; i < sbInstCount; ++i) {
					TRANSLATE_PRINT_INSTRUCTION(PRINT_INFO | PRINT_TRANSLATION | PRINT_TRACKING | PRINT_BACKWARD, &symbopBkRiverInst[i]);
				}
				TRANSLATE_PRINT(PRINT_INFO | PRINT_TRANSLATION | PRINT_TRACKING | PRINT_BACKWARD, "===============================================================================\n");
			}
		}


		if (revtracerConfig.dumpBlocks) {
			SaveToStream(fwRiverInst, fwInstCount, bkRiverInst, bkInstCount, symbopInst, symbopInstCount);
		}

		//outBufferSize = rivertox86(this, rt, trRiverInst, trInstCount, outBuffer, 0x00);
		//assembler.Assemble(trRiverInst, trInstCount, outBuffer, 0x00, pCB->, outBufferSize);
		//pCB->pCode = DuplicateBuffer(heap, outBuffer, outBufferSize);

		//outBufferSize = rivertox86(this, rt, fwRiverInst, fwInstCount, outBuffer, 0x01);
		codeBuffer.Reset();
		assembler.Assemble(fwRiverInst, fwInstCount, codeBuffer, 0x10, pCB->dwFwOpCount, outBufferSize, ASSEMBLER_CODE_NATIVE | ASSEMBLER_DIR_FORWARD);
		pCB->pFwCode = DuplicateBuffer(heap, outBuffer, outBufferSize);
		//assembler.CopyFix(pCB->pFwCode, outBuffer);
		codeBuffer.CopyToFixed(pCB->pFwCode);
		
		//outBufferSize = rivertox86(this, rt, bkRiverInst, bkInstCount, outBuffer, 0x00);

		if (dwTranslationFlags & TRACER_FEATURE_REVERSIBLE) {
			codeBuffer.Reset();
			assembler.Assemble(bkRiverInst, bkInstCount, codeBuffer, 0x00, pCB->dwBkOpCount, outBufferSize, ASSEMBLER_CODE_NATIVE | ASSEMBLER_DIR_BACKWARD);
			pCB->pBkCode = DuplicateBuffer(heap, outBuffer, outBufferSize);
			//assembler.CopyFix(pCB->pBkCode, outBuffer);
			codeBuffer.CopyToFixed(pCB->pBkCode);
		}

		if (dwTranslationFlags & TRACER_FEATURE_TRACKING) {
			RiverInstruction *fwTrace = symbopInst;
			nodep::DWORD fwTraceCount = symbopInstCount;

			if (dwTranslationFlags & TRACER_FEATURE_REVERSIBLE) {
				fwTrace = symbopFwRiverInst;
				fwTraceCount = sfInstCount;
			}

			codeBuffer.Reset();
			assembler.Assemble(fwTrace, fwTraceCount, codeBuffer, 0x10, pCB->dwTrOpCount, outBufferSize, ASSEMBLER_CODE_TRACKING | ASSEMBLER_DIR_FORWARD);
			pCB->pTrackCode = DuplicateBuffer(heap, outBuffer, outBufferSize);
			codeBuffer.CopyToFixed(pCB->pTrackCode);

			if (dwTranslationFlags & TRACER_FEATURE_REVERSIBLE) {
				codeBuffer.Reset();
				assembler.Assemble(symbopBkRiverInst, sbInstCount, codeBuffer, 0x00, pCB->dwRtOpCount, outBufferSize, ASSEMBLER_CODE_TRACKING | ASSEMBLER_DIR_BACKWARD);
				pCB->pRevTrackCode = DuplicateBuffer(heap, outBuffer, outBufferSize);
				codeBuffer.CopyToFixed(pCB->pRevTrackCode);
			}
		}

		return true;
	}
}
