#include "river.h"
#include "riverinternl.h"

#include <stdio.h>

extern const char PrintMnemonicTable00[][10];
extern const char PrintMnemonicTable0F[][10];
extern const char PrintMnemonicExt[][8][10];


extern const char RegNames[][4];
extern const char MemSizes[][6];

void PrintPrefixes(struct RiverInstruction *ri) {
	if (ri->modifiers & RIVER_MODIFIER_IGNORE) {
		printf("ignore");
	}

	if (ri->modifiers & RIVER_MODIFIER_RIVEROP) {
		printf("river");
	}

	if (ri->modifiers & RIVER_MODIFIER_ORIG_xSP) {
		printf("esp");
	}

	if (ri->modifiers & RIVER_MODIFIER_LOCK) {
		printf("lock ");
	}

	if (ri->modifiers & RIVER_MODIFIER_REP) {
		printf("rep ");
	}

	if (ri->modifiers & RIVER_MODIFIER_REPZ) {
		printf("repz ");
	}

	if (ri->modifiers & RIVER_MODIFIER_REPNZ) {
		printf("repnz ");
	}
}

void PrintMnemonic(struct RiverInstruction *ri) {
	const char (*mTable)[10] = PrintMnemonicTable00;

	if (RIVER_MODIFIER_EXT & ri->modifiers) {
		mTable = PrintMnemonicTable0F;
	}

	if ('a' <= mTable[ri->opCode][0]) {
		printf("%s ", mTable[ri->opCode]);
	} else {
		printf("%s ", PrintMnemonicExt[mTable[ri->opCode][0]][ri->subOpCode]);
	}
}

void PrintRegister(struct RiverInstruction *ri, union RiverRegister *reg) {
	printf("%s$%d", RegNames[reg->name], reg->versioned >> 8);
}

void PrintOperand(struct RiverInstruction *ri, DWORD idx) {
	bool bWr = false;
	switch (ri->opTypes[idx] & ~0x03) {
		case RIVER_OPTYPE_IMM :
			switch (ri->opTypes[idx] & 0x03) {
				case RIVER_OPSIZE_8 :
					printf("0x%02x", ri->operands[idx].asImm8);
					break;
				case RIVER_OPSIZE_16:
					printf("0x%04x", ri->operands[idx].asImm16);
					break;
				case RIVER_OPSIZE_32:
					printf("0x%08x", ri->operands[idx].asImm32);
					break;
			};
			break;

		case RIVER_OPTYPE_REG :
			printf("%s$%d", RegNames[ri->operands[idx].asRegister.name], ri->operands[idx].asRegister.versioned >> 8);
			break;

		case RIVER_OPTYPE_MEM :
			if (0 == ri->operands[idx].asAddress->type) {
				printf("%s$%d", RegNames[ri->operands[idx].asAddress->base.name], ri->operands[idx].asAddress->base.versioned >> 8);
				break;
			}

			printf("%s ptr [", MemSizes[ri->opTypes[idx] & 0x03]);
			if (ri->operands[idx].asAddress->type & RIVER_ADDR_BASE) {
				printf("%s$%d", RegNames[ri->operands[idx].asAddress->base.name], ri->operands[idx].asAddress->base.versioned >> 8);
				bWr = true;
			}

			if (ri->operands[idx].asAddress->type & RIVER_ADDR_INDEX) {
				if (bWr) {
					printf("+");
				}

				if (ri->operands[idx].asAddress->type & RIVER_ADDR_SCALE) {
					printf("%d*", ri->operands[idx].asAddress->scale);
				}

				printf("%s$%d", RegNames[ri->operands[idx].asAddress->index.name], ri->operands[idx].asAddress->index.versioned >> 8);
				bWr = true;
			}

			if (ri->operands[idx].asAddress->type & (RIVER_ADDR_DISP8 | RIVER_ADDR_DISP)) {
				if (bWr) {
					printf("+");
				}

				switch (ri->operands[idx].asAddress->type & (RIVER_ADDR_DISP8 | RIVER_ADDR_DISP)) {
					case RIVER_ADDR_DISP8 :
						printf("0x%02x", ri->operands[idx].asAddress->disp.d8);
						break;
					/*case RIVER_ADDR_DISP16:
						printf("0x%04x", ri->operands[idx].asAddress->disp.d16);
						break;*/
					case RIVER_ADDR_DISP:
						printf("0x%08x", ri->operands[idx].asAddress->disp.d32);
						break;
				}
			}
				 

			printf("]");
			break;
	}
}

void PrintOperands(struct RiverInstruction *ri) {
	PrintOperand(ri, 0);
	if (ri->opTypes[1] != RIVER_OPTYPE_NONE) {
		printf(", ");
		PrintOperand(ri, 1);
	}
}

void RiverPrintInstruction(struct RiverInstruction *ri) {
	if (ri->modifiers & RIVER_MODIFIER_IGNORE) {
		return;
	}
	PrintPrefixes(ri);
	PrintMnemonic(ri);
	PrintOperands(ri);
	printf("\n");
	//printf("%s", )
}

const char MemSizes[][6] = {
	"dword", "word", "byte"
};

const char RegNames[][4] = {
	"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
	"ax", "cx", "dx", "bx", "sp", "bp", "si", "di",
	"al", "cl", "dl", "bl", "--", "--", "--", "--",
	"ah", "ch", "dh", "bh", "--", "--", "--", "--"
};

const char PrintMnemonicTable00[][10] = {
	/*0x00*/"", "add", "", "add", "", "", "", "", "", "or", "", "or", "", "", "", "",
	/*0x10*/"", "adc", "", "adc", "", "", "", "", "", "sbb", "", "sbb", "", "", "", "",
	/*0x20*/"", "and", "", "and", "", "", "", "", "", "sub", "", "sub", "", "", "", "",
	/*0x30*/"", "xor", "", "xor", "", "", "", "", "", "cmp", "", "cmp", "", "", "", "",
	/*0x40*/"inc", "inc", "inc", "inc", "inc", "inc", "inc", "inc", "dec", "dec", "dec", "dec", "dec", "dec", "dec", "dec",
	/*0x50*/"push", "push", "push", "push", "push", "push", "push", "push", "pop", "pop", "pop", "pop", "pop", "pop", "pop", "pop",
	/*0x60*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0x70*/"jo", "jno", "jc", "jnc", "jz", "jnz", "jbe", "jnbe", "js", "jns", "jp", "jnp", "jl", "jnl", "jle", "jnle",
	/*0x80*/"\1", "\1", "\1", "\1", "", "", "", "", "", "mov", "", "mov", "", "lea", "", "pop",
	/*0x90*/"nop", "xchg", "xchg", "xchg", "xchg", "xchg", "xchg", "xchg", "", "", "", "", "pushf", "popf", "", "",
	/*0xA0*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0xB0*/"mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov",
	/*0xC0*/"", "", "", "retn", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0xD0*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0xE0*/"", "", "", "", "", "", "", "", "", "jmp", "", "", "", "", "", "",
	/*0xF0*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "\2"
};

const char PrintMnemonicTable0F[][10] = {
	/*0x00*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0x10*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0x20*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0x30*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0x40*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0x50*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0x60*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0x70*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0x80*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0x90*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0xA0*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0xB0*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0xC0*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0xD0*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0xE0*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	/*0xF0*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""
};

const char PrintMnemonicExt[][8][10] = {
	/*for unknown */{ "___", "___", "___", "___", "___", "___", "___", "___" },
	/*for 0x83    */{ "add", "or", "adc", "sbb", "and", "sub", "xor", "cmp" },
	/*for 0xFF    */{ "inc", "dec", "call", "call", "jmp", "jmp", "push", "___" }
};