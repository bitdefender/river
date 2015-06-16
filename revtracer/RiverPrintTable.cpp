#include "river.h"
#include "riverinternl.h"

#include <stdio.h>

extern const char PrintMnemonicTable00[][10];
extern const char PrintMnemonicTable0F[][10];
extern const char PrintMnemonicExt[][8][10];


extern const char RegNames[][4];
extern const char MemSizes[][6];

void PrintPrefixes(struct RiverInstruction *ri) {
	if (ri->family & RIVER_FAMILY_IGNORE) {
		DbgPrint("ignore");
	}

	if (ri->family & RIVER_FAMILY_RIVEROP) {
		DbgPrint("river");
	}

	if (ri->family & RIVER_FAMILY_ORIG_xSP) {
		DbgPrint("esp");
	}
	
	if (ri->family & RIVER_FAMILY_SYMBOP) {
		DbgPrint("symbop");
	}

	if (ri->modifiers & RIVER_MODIFIER_LOCK) {
		DbgPrint("lock ");
	}

	if (ri->modifiers & RIVER_MODIFIER_REP) {
		DbgPrint("rep ");
	}

	if (ri->modifiers & RIVER_MODIFIER_REPZ) {
		DbgPrint("repz ");
	}

	if (ri->modifiers & RIVER_MODIFIER_REPNZ) {
		DbgPrint("repnz ");
	}
}

void PrintMnemonic(struct RiverInstruction *ri) {
	const char (*mTable)[10] = PrintMnemonicTable00;

	if (RIVER_MODIFIER_EXT & ri->modifiers) {
		mTable = PrintMnemonicTable0F;
	}

	if ('a' <= mTable[ri->opCode][0]) {
		DbgPrint("%s ", mTable[ri->opCode]);
	} else {
		DbgPrint("%s ", PrintMnemonicExt[mTable[ri->opCode][0]][ri->subOpCode]);
	}
}

void PrintRegister(struct RiverInstruction *ri, union RiverRegister *reg) {
	DbgPrint("%s$%d", RegNames[reg->name], reg->versioned >> 8);
}

void PrintOperand(struct RiverInstruction *ri, DWORD idx) {
	bool bWr = false;
	switch (ri->opTypes[idx] & ~0x03) {
		case RIVER_OPTYPE_IMM :
			switch (ri->opTypes[idx] & 0x03) {
				case RIVER_OPSIZE_8 :
					DbgPrint("0x%02x", ri->operands[idx].asImm8);
					break;
				case RIVER_OPSIZE_16:
					DbgPrint("0x%04x", ri->operands[idx].asImm16);
					break;
				case RIVER_OPSIZE_32:
					DbgPrint("0x%08x", ri->operands[idx].asImm32);
					break;
			};
			break;

		case RIVER_OPTYPE_REG :
			DbgPrint("%s$%d", RegNames[ri->operands[idx].asRegister.name], ri->operands[idx].asRegister.versioned >> 8);
			break;

		case RIVER_OPTYPE_MEM :
			if (0 == ri->operands[idx].asAddress->type) {
				DbgPrint("%s$%d", RegNames[ri->operands[idx].asAddress->base.name], ri->operands[idx].asAddress->base.versioned >> 8);
				break;
			}

			DbgPrint("%s ptr ", MemSizes[ri->opTypes[idx] & 0x03]);

			if (ri->operands[idx].asAddress->HasSegment()) {
				DbgPrint("%s:", RegNames[RIVER_REG_SEGMENT | (ri->operands[idx].asAddress->GetSegment() - 1)]);
			}

			DbgPrint("[");
			if (ri->operands[idx].asAddress->type & RIVER_ADDR_BASE) {
				DbgPrint("%s$%d", RegNames[ri->operands[idx].asAddress->base.name], ri->operands[idx].asAddress->base.versioned >> 8);
				bWr = true;
			}

			if (ri->operands[idx].asAddress->type & RIVER_ADDR_INDEX) {
				if (bWr) {
					DbgPrint("+");
				}

				if (ri->operands[idx].asAddress->type & RIVER_ADDR_SCALE) {
					DbgPrint("%d*", ri->operands[idx].asAddress->GetScale());
				}

				DbgPrint("%s$%d", RegNames[ri->operands[idx].asAddress->index.name], ri->operands[idx].asAddress->index.versioned >> 8);
				bWr = true;
			}

			if (ri->operands[idx].asAddress->type & (RIVER_ADDR_DISP8 | RIVER_ADDR_DISP)) {
				if (bWr) {
					DbgPrint("+");
				}

				switch (ri->operands[idx].asAddress->type & (RIVER_ADDR_DISP8 | RIVER_ADDR_DISP)) {
					case RIVER_ADDR_DISP8 :
						DbgPrint("0x%02x", ri->operands[idx].asAddress->disp.d8);
						break;
					/*case RIVER_ADDR_DISP16:
						DbgPrint("0x%04x", ri->operands[idx].asAddress->disp.d16);
						break;*/
					case RIVER_ADDR_DISP:
						DbgPrint("0x%08x", ri->operands[idx].asAddress->disp.d32);
						break;
				}
			}
				 

			DbgPrint("]");
			break;
	}
}

void PrintOperands(struct RiverInstruction *ri) {
	PrintOperand(ri, 0);

	for (int i = 1; i < 4; ++i) {
		if (ri->opTypes[i] != RIVER_OPTYPE_NONE) {
			DbgPrint(", ");
			PrintOperand(ri, i);
		}
	}
}

void RiverPrintInstruction(struct RiverInstruction *ri) {
	if (ri->family & RIVER_FAMILY_IGNORE) {
		return;
	}
	PrintPrefixes(ri);
	PrintMnemonic(ri);
	PrintOperands(ri);
	DbgPrint("\n");
	//printf("%s", )
}

const char MemSizes[][6] = {
	"dword", "word", "byte"
};

const char RegNames[][4] = {
	"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
	 "ax",  "cx",  "dx",  "bx",  "sp",  "bp",  "si",  "di",
	 "al",  "cl",  "dl",  "bl",  "--",  "--",  "--",  "--",
	 "ah",  "ch",  "dh",  "bh",  "--",  "--",  "--",  "--",

	 "es",  "cs",  "ss",  "ds",  "fs",  "gs",  "--",  "--",
	"cr0",  "--", "cr2", "cr3", "cr4",  "--",  "--",  "--",
	"dr0", "dr1", "dr2", "dr3", "dr4", "dr5", "dr6", "dr7"
};

const char PrintMnemonicTable00[][10] = {
	/*0x00*/"add", "add", "add", "add", "add", "add", "", "", "or",  "or",  "or",  "or",  "or",  "or", "", "",
	/*0x10*/"adc", "adc", "adc", "adc", "adc", "adc", "", "", "sbb", "sbb", "sbb", "sbb", "sbb", "sbb", "", "",
	/*0x20*/"and", "and", "and", "and", "and", "and", "", "", "sub", "sub", "sub", "sub", "sub", "sub", "", "",
	/*0x30*/"xor", "xor", "xor", "xor", "xor", "xor", "", "", "cmp", "cmp", "cmp", "cmp", "cmp", "cmp", "", "",
	/*0x40*/"inc", "inc", "inc", "inc", "inc", "inc", "inc", "inc", "dec", "dec", "dec", "dec", "dec", "dec", "dec", "dec",
	/*0x50*/"push", "push", "push", "push", "push", "push", "push", "push", "pop", "pop", "pop", "pop", "pop", "pop", "pop", "pop",
	/*0x60*/"", "", "", "", "", "", "", "", "push", "imul", "push", "imul", "", "", "", "",
	/*0x70*/"jo", "jno", "jc", "jnc", "jz", "jnz", "jbe", "jnbe", "js", "jns", "jp", "jnp", "jl", "jnl", "jle", "jnle",
	/*0x80*/"\1", "\1", "\1", "\1", "test", "test", "xchg", "xchg", "mov", "mov", "mov", "mov", "", "lea", "", "pop",
	/*0x90*/"nop", "xchg", "xchg", "xchg", "xchg", "xchg", "xchg", "xchg", "", "cdq", "", "", "pushf", "popf", "", "",
	/*0xA0*/"mov", "mov", "mov", "mov", "movs", "movs", "cmps", "cmps", "test", "test", "stos", "stos", "", "", "scas", "scas",
	/*0xB0*/"mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov",
	/*0xC0*/"\2", "\2", "retn", "retn", "", "", "mov", "mov", "", "leave", "", "", "", "", "", "",
	/*0xD0*/"\2", "\2", "\2", "\2", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0xE0*/"", "", "", "", "", "", "", "", "call", "jmp", "jmpf", "jmp", "", "", "", "",
	/*0xF0*/"", "", "", "", "", "", "\4", "\4", "", "", "", "", "cld", "std", "\6", "\3"
};

const char PrintMnemonicTable0F[][10] = {
	/*0x00*/"", "", "lar", "lsl", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0x10*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0x20*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0x30*/"", "rdtsc", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0x40*/"cmovo", "cmovno", "cmovc", "cmovnc", "cmovz", "cmovnz", "cmovbe", "cmovnbe", "cmovs", "cmovns", "cmovp", "cmovnp", "cmovl", "cmovnl", "cmovle", "cmovnle",
	/*0x50*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0x60*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0x70*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0x80*/"jo", "jno", "jc", "jnc", "jz", "jnz", "jbe", "jnbe", "js", "jns", "jp", "jnp", "jl", "jnl", "jle", "jnle",
	/*0x90*/"seto", "setno", "setc", "setnc", "setz", "setnz", "setbe", "setnbe", "sets", "setns", "setp", "setnp", "setl", "setnl", "setle", "setnle",
	/*0xA0*/"", "", "cpuid", "", "shld", "shld", "", "", "", "", "", "", "shrd", "shrd", "", "imul",
	/*0xB0*/"cmpxchg", "cmpxchg", "", "", "", "", "movzx", "movzx", "", "", "\5", "", "bsf", "bsr", "movsx", "movsx",
	/*0xC0*/"xadd", "xadd", "", "", "", "", "", "\7", "", "", "", "", "", "", "", "",
	/*0xD0*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0xE0*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0xF0*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""
};

const char PrintMnemonicExt[][8][10] = {
	/*for unknown */{ "___", "___", "___", "___", "___", "___", "___", "___" },
	/*for 0x83    */{ "add",  "or", "adc", "sbb", "and", "sub", "xor", "cmp" },
	/*for 0xC0/C1 */{ "rol", "ror", "rcl", "rcr", "shl", "shr", "sal", "sar"},
	/*for 0xFF    */{ "inc", "dec", "call", "call", "jmp", "jmp", "push", "___" },
	/*for 0xF6/F7 */{ "test", "test", "not", "neg", "mul", "imul", "div", "idiv" },
	/*for 0x0FBA  */{ "___", "___", "___", "___", "bt", "bts", "btr", "btc" },
	/*for 0xFE    */{ "inc", "dec", "___", "___", "___", "___", "___", "___" },
	/*for 0x0FC7  */{ "___", "cmpxchg8b", "___", "___", "___", "___", "___", "___" }
};