(function (ns) {

	ns.Disassembler = function() {
		this.state = 0;
	};

	ns.Disassembler.Modifiers = {
		MODIFIER_OPERAND_8 		: 0x0001,
		MODIFIER_OPERAND_16 	: 0x0002,

		MODIFIER_ADDRESS_16		: 0x0004,

		MODIFIER_NO_FALLTROUGH  : 0x0100,
		MODIFIER_FIXED_BRANCH   : 0x0200,

		MODIFIER_RETURN			: 0x2000,

		MODIFIER_BRANCH			: 0x4000,
		MODIFIER_EXT_PFX		: 0x8000
	};

	ns.Disassembler.GenericRegisters = {
		xAX : 0,
		xCX : 1,
		xDX : 2,
		xBX : 3,
		xSP : 4,
		xBP : 5,
		xSI : 6,
		xDI : 7
	};

	ns.Disassembler.GenericRegisterSizes = {
		RIVER_REG_SZ32  : 0x00,
		RIVER_REG_SZ16  : 0x08,
		RIVER_REG_SZ8_L : 0x10,
		RIVER_REG_SZ8_H : 0x18
	};

	ns.Disassembler.OperandType = {
		OPTYPE_IMM8 : 0x01,
		//OPTYPE_IMM16 : 0x02,
		OPTYPE_IMM : 0x03,

		OPTYPE_REGISTER : 0x04,

		OPTYPE_MEMORY : 0x05
	};

	ns.Disassembler.DisplacementSize = {
		DISP_NONE : 0,
		DISP_8BIT : 1,
		DISP_NATIVE : 2
	};

	ns.Disassembler.RegisterNames = [
		"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
		 "ax",  "cx",  "dx",  "bx",  "sp",  "bp",  "si",  "di",
		 "al",  "cl",  "dl",  "bl",  "--",  "--",  "--",  "--",
		 "ah",  "ch",  "dh",  "bh",  "--",  "--",  "--",  "--",

		 "es",  "cs",  "ss",  "ds",  "fs",  "gs",  "--",  "--",
		"cr0",  "--", "cr2", "cr3", "cr4",  "--",  "--",  "--",
		"dr0", "dr1", "dr2", "dr3", "dr4", "dr5", "dr6", "dr7"
	];

	function ConstructDefaultOpcodeDisassembler(modifiers) {
		//var _mods = modifiers;

		return function(buffer, op, disasm) {
			op.opcode = buffer[disasm.index];
			op.modifiers |= modifiers;
			disasm.index++;

			return true;
		};
	}

	function ConstructSubOpcodeDisassembler(modifiers) {
		//var _mods = modifiers;

		return function(buffer, op, disasm) {
			op.opcode = buffer[disasm.index];
			op.modifiers |= modifiers;
			disasm.index++;

			op.subOpcode = (buffer[disasm.index] >> 3) & 0x07;

			return true;
		};
	}


	function ConstructDefaultRegisterOpcodeDisassembler(position, register, modifiers) {
		return function(buffer, op, disasm) {
			op.opcode = buffer[disasm.index];
			op.modifiers |= modifiers;
			disasm.index++;

			op.operands[position] = {
				asRegister : register,
				type : Disassembler.OperandType.OPTYPE_REGISTER 
			};

			return true;
		};
	}

	function ConstructPlusRegOpcodeDisassembler(base, modifiers) {
		return function(buffer, op, disasm) {
			op.opcode = base;
			op.modifiers |= modifiers;
			disasm.index++;

			op.operands[0] = {
				asRegister : buffer[disasm.index - 1] - base,
				type : Disassembler.OperandType.OPTYPE_REGISTER 
			};

			return true;
		};
	}

	function ConstructPrefixDisassembler(modifiers) {
		return function(buffer, op, disasm) {
			op.opcode = buffer[disasm.index];
			op.modifiers |= modifiers;
			disasm.index++;

			return false;
		};
	}

	function ConstructSegmentPrefixDisassembler(segment) {
		return function(buffer, op, disasm) {
			op.opcode = buffer[disasm.index];
			op.segment = segment;
			disasm.index++;

			return false;
		};
	}

	function ConstructRelJump(instrlength, modifiers) {
		return function(buffer, op, disasm) {
			op.opcode = buffer[disasm.index];
			op.nextInstruction = disasm.offset + disasm.index + instrlength;
			op.modifiers |= Disassembler.Modifiers.MODIFIER_BRANCH | Disassembler.Modifiers.MODIFIER_FIXED_BRANCH | modifiers;

			disasm.index++;

			return true;
		};
	}

	function ConstructSubOpDisassembler(ops) {
		return function(buffer, op, disasm) {
			op.opcode = buffer[disasm.index];
			op.subOpcode = (buffer[disasm.index + 1] >> 3) & 0x07;

			return ops[op.subOpcode](buffer, op, disasm);
		};
	}


	var DisasmUnknownOpcode = function(buffer, op, disasm) {
		throw {
			"instruction" : op,
			"disasm" : disasm,
			"opcode" : buffer[disasm.index]
		};
	};

	var DisasmOpcodeDefault = ConstructDefaultOpcodeDisassembler(0);
	var DisasmOpcodeDefault8bitOp = ConstructDefaultOpcodeDisassembler(ns.Disassembler.Modifiers.MODIFIER_OPERAND_8);
	var DisasmOpcodeDefault16bitOp = ConstructPrefixDisassembler(ns.Disassembler.Modifiers.MODIFIER_OPERAND_16);
	var DisasmOpcodeDefault16bitAddr = ConstructPrefixDisassembler(ns.Disassembler.Modifiers.MODIFIER_ADDRESS_16);

	var DisasmSubOpcode = ConstructSubOpcodeDisassembler(0);
	var DisasmSubOpcode8bitOp = ConstructSubOpcodeDisassembler(ns.Disassembler.Modifiers.MODIFIER_OPERAND_8);

	var DisasmOpcodeOp1AL = ConstructDefaultRegisterOpcodeDisassembler(
		0,
		ns.Disassembler.GenericRegisters.xAX | ns.Disassembler.GenericRegisterSizes.RIVER_REG_SZ8_L,
		ns.Disassembler.Modifiers.MODIFIER_OPERAND_8
	);
	var DisasmOpcodeOp1EAX = ConstructDefaultRegisterOpcodeDisassembler(
		0,
		ns.Disassembler.GenericRegisters.xAX,
		0
	);

	var DisasmOpcodeOp2AL = ConstructDefaultRegisterOpcodeDisassembler(
		1,
		ns.Disassembler.GenericRegisters.xAX | ns.Disassembler.GenericRegisterSizes.RIVER_REG_SZ8_L,
		ns.Disassembler.Modifiers.MODIFIER_OPERAND_8
	);
	var DisasmOpcodeOp2EAX = ConstructDefaultRegisterOpcodeDisassembler(
		1,
		ns.Disassembler.GenericRegisters.xAX,
		0
	);

	var DisasmPlusReg0x40 = ConstructPlusRegOpcodeDisassembler(0x40, 0);
	var DisasmPlusReg0x48 = ConstructPlusRegOpcodeDisassembler(0x48, 0);
	var DisasmPlusReg0x50 = ConstructPlusRegOpcodeDisassembler(0x50, 0);
	var DisasmPlusReg0x58 = ConstructPlusRegOpcodeDisassembler(0x58, 0);

	var DisasmPlusReg0xB0 = ConstructPlusRegOpcodeDisassembler(0xB0, Disassembler.Modifiers.MODIFIER_OPERAND_8);
	var DisasmPlusReg0xB8 = ConstructPlusRegOpcodeDisassembler(0xB8, 0);

	var DisasmRelJmp2 = ConstructRelJump(2, 0);
	var DisasmRelJmp5 = ConstructRelJump(5, 0);

	var DisasmOpcodeRet = ConstructDefaultOpcodeDisassembler(Disassembler.Modifiers.MODIFIER_RETURN | Disassembler.Modifiers.MODIFIER_NO_FALLTROUGH);
	var DisasmOpcodeRetn = ConstructDefaultOpcodeDisassembler(
		Disassembler.Modifiers.MODIFIER_OPERAND_16 | 
		Disassembler.Modifiers.MODIFIER_RETURN | 
		Disassembler.Modifiers.MODIFIER_NO_FALLTROUGH
	);
	var DisasmOpcodeJmp2 = ConstructRelJump(2, Disassembler.Modifiers.MODIFIER_NO_FALLTROUGH);
	var DisasmOpcodeJmp5 = ConstructRelJump(5, Disassembler.Modifiers.MODIFIER_NO_FALLTROUGH);

	var DisasmExtPfx = ConstructPrefixDisassembler(ns.Disassembler.Modifiers.MODIFIER_EXT_PFX);

	function DisasmOpcodeRelJmpModRM(buffer, op, disasm) {
		op.opcode = buffer[disasm.index];
		disasm.index++;

		DisassembleModRM0(buffer, op, disasm);
		op.nextInstruction = disasm.offset + disasm.index;
		op.modifiers |= Disassembler.Modifiers.MODIFIER_BRANCH;

		return true;
	}

	var DisasmOpcode0xFF = ConstructSubOpDisassembler([
		DisasmOpcodeDefault,
		DisasmOpcodeDefault,
		DisasmOpcodeRelJmpModRM,
		DisasmUnknownOpcode,
		DisasmOpcodeRelJmpModRM,
		DisasmUnknownOpcode,
		DisasmOpcodeDefault,
		DisasmUnknownOpcode
	]);

	//var DisasmOpcodeDefault8bit = DisasmOpcodeModified.bind(undefined, ns.Disassembler.Modifiers.MODIFIER_OPERAND_8);

	var DisasmOpcode = [
		[
			/*0x00*/ DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault, DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault, 
			/*0x04*/ DisasmOpcodeOp1AL, DisasmOpcodeOp1EAX, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x08*/ DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault, DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault,
			/*0x0C*/ DisasmOpcodeOp1AL, DisasmOpcodeOp1EAX, DisasmUnknownOpcode, DisasmExtPfx,

			/*0x10*/ DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault, DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault, 
			/*0x14*/ DisasmOpcodeOp1AL, DisasmOpcodeOp1EAX, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x18*/ DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault, DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault,
			/*0x1C*/ DisasmOpcodeOp1AL, DisasmOpcodeOp1EAX, DisasmUnknownOpcode, DisasmUnknownOpcode,

			/*0x20*/ DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault, DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault, 
			/*0x24*/ DisasmOpcodeOp1AL, DisasmOpcodeOp1EAX, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x28*/ DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault, DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault,
			/*0x2C*/ DisasmOpcodeOp1AL, DisasmOpcodeOp1EAX, DisasmUnknownOpcode, DisasmUnknownOpcode,

			/*0x30*/ DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault, DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault, 
			/*0x34*/ DisasmOpcodeOp1AL, DisasmOpcodeOp1EAX, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x38*/ DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault, DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault,
			/*0x3C*/ DisasmOpcodeOp1AL, DisasmOpcodeOp1EAX, DisasmUnknownOpcode, DisasmUnknownOpcode,

			/*0x40*/ DisasmPlusReg0x40, DisasmPlusReg0x40, DisasmPlusReg0x40, DisasmPlusReg0x40, 
			/*0x44*/ DisasmPlusReg0x40, DisasmPlusReg0x40, DisasmPlusReg0x40, DisasmPlusReg0x40, 
			/*0x48*/ DisasmPlusReg0x48, DisasmPlusReg0x48, DisasmPlusReg0x48, DisasmPlusReg0x48, 
			/*0x4C*/ DisasmPlusReg0x48, DisasmPlusReg0x48, DisasmPlusReg0x48, DisasmPlusReg0x48, 

			/*0x50*/ DisasmPlusReg0x50, DisasmPlusReg0x50, DisasmPlusReg0x50, DisasmPlusReg0x50,
			/*0x54*/ DisasmPlusReg0x50, DisasmPlusReg0x50, DisasmPlusReg0x50, DisasmPlusReg0x50,
			/*0x58*/ DisasmPlusReg0x58, DisasmPlusReg0x58, DisasmPlusReg0x58, DisasmPlusReg0x58,
			/*0x5C*/ DisasmPlusReg0x58, DisasmPlusReg0x58, DisasmPlusReg0x58, DisasmPlusReg0x58,

			/*0x60*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x64*/ ConstructSegmentPrefixDisassembler("fs"), ConstructSegmentPrefixDisassembler("gs"), DisasmOpcodeDefault16bitOp, DisasmOpcodeDefault16bitAddr,
			/*0x68*/ DisasmOpcodeDefault, DisasmOpcodeDefault, DisasmOpcodeDefault, DisasmOpcodeDefault,
            /*0x6C*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,

			/*0x70*/ DisasmOpcodeJmp2, DisasmOpcodeJmp2, DisasmOpcodeJmp2, DisasmOpcodeJmp2,
			/*0x74*/ DisasmOpcodeJmp2, DisasmOpcodeJmp2, DisasmOpcodeJmp2, DisasmOpcodeJmp2,
			/*0x78*/ DisasmOpcodeJmp2, DisasmOpcodeJmp2, DisasmOpcodeJmp2, DisasmOpcodeJmp2,
			/*0x7C*/ DisasmOpcodeJmp2, DisasmOpcodeJmp2, DisasmOpcodeJmp2, DisasmOpcodeJmp2,

			/*0x80*/ DisasmSubOpcode8bitOp, DisasmSubOpcode, DisasmSubOpcode8bitOp, DisasmSubOpcode,
			/*0x84*/ DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault, DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault, 
			/*0x88*/ DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault, DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault, 
			/*0x8C*/ DisasmUnknownOpcode, DisasmOpcodeDefault, DisasmUnknownOpcode, DisasmUnknownOpcode,

			/*0x90*/ DisasmOpcodeDefault, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x94*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x98*/ DisasmUnknownOpcode, DisasmOpcodeDefault, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x9C*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,

			/*0xA0*/ DisasmOpcodeOp1AL, DisasmOpcodeOp1EAX, DisasmOpcodeOp2AL, DisasmOpcodeOp2EAX,
			/*0xA4*/ DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault, DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault,
			/*0xA8*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xAC*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault,

			/*0xB0*/ DisasmPlusReg0xB0, DisasmPlusReg0xB0, DisasmPlusReg0xB0, DisasmPlusReg0xB0,
			/*0xB4*/ DisasmPlusReg0xB0, DisasmPlusReg0xB0, DisasmPlusReg0xB0, DisasmPlusReg0xB0,
			/*0xB8*/ DisasmPlusReg0xB8, DisasmPlusReg0xB8, DisasmPlusReg0xB8, DisasmPlusReg0xB8,
			/*0xBC*/ DisasmPlusReg0xB8, DisasmPlusReg0xB8, DisasmPlusReg0xB8, DisasmPlusReg0xB8,

			/*0xC0*/ DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault, DisasmOpcodeRetn, DisasmOpcodeRet,
			/*0xC4*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault,
			/*0xC8*/ DisasmUnknownOpcode, DisasmOpcodeDefault, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xCC*/ DisasmUnknownOpcode, DisasmOpcodeDefault, DisasmUnknownOpcode, DisasmUnknownOpcode,

			/*0xD0*/ DisasmOpcodeDefault8bitOp, DisasmOpcodeDefault, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xD4*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xD8*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xDC*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,

			/*0xE0*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xE4*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xE8*/ DisasmRelJmp5, DisasmOpcodeJmp5, DisasmUnknownOpcode, DisasmOpcodeJmp2,
			/*0xEC*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,

			/*0xF0*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xF4*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xF8*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xFC*/ DisasmOpcodeDefault, DisasmOpcodeDefault, DisasmOpcodeDefault8bitOp, DisasmOpcode0xFF
		], [
			/*0x00*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x04*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x08*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x0C*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,

			/*0x10*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x14*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x18*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x1C*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,

			/*0x20*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x24*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x28*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x2C*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,

			/*0x30*/ DisasmUnknownOpcode, DisasmOpcodeDefault, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x34*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x38*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x3C*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,

			/*0x40*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x44*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x48*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x4C*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,

			/*0x50*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x54*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x58*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x5C*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,

			/*0x60*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x64*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x68*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x6C*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,

			/*0x70*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x74*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x78*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x7C*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,

			/*0x80*/ DisasmRelJmp5, DisasmRelJmp5, DisasmRelJmp5, DisasmRelJmp5,
			/*0x84*/ DisasmRelJmp5, DisasmRelJmp5, DisasmRelJmp5, DisasmRelJmp5,
			/*0x88*/ DisasmRelJmp5, DisasmRelJmp5, DisasmRelJmp5, DisasmRelJmp5,
			/*0x8C*/ DisasmRelJmp5, DisasmRelJmp5, DisasmRelJmp5, DisasmRelJmp5,

			/*0x90*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x94*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x98*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0x9C*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,

			/*0xA0*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xA4*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xA8*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xAC*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,

			/*0xB0*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xB4*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xB8*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmSubOpcode, DisasmUnknownOpcode,
			/*0xBC*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,

			/*0xC0*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xC4*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xC8*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xCC*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,

			/*0xD0*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xD4*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xD8*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xDC*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,

			/*0xE0*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xE4*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xE8*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xEC*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,

			/*0xF0*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xF4*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xF8*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode,
			/*0xFC*/ DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode, DisasmUnknownOpcode
		]
	];


	function GetOpcode(buffer, op, disasm) {
		var code = buffer[disasm.index];
		return DisasmOpcode[(op.modifiers & ns.Disassembler.Modifiers.MODIFIER_EXT_PFX) ? 1 : 0][code](buffer, op, disasm);
	}


	var DisasmUnknownOperand = function(buffer, op, disasm) {
		throw {
			"instruction" : op,
			"disasm" : disasm
		};
	};

	var DisasmNoOperand = function(buffer, op, disasm) { };


	function ConstructImmediateOperand8(position) {	
		return function (buffer, op, disasm) {
			op.operands[position] = {
				value : buffer[disasm.index],
				type : Disassembler.OperandType.OPTYPE_IMM8
			};

			disasm.index++;
		};
	}

	function ConstructImmediateOperand(position) {	
		return function (buffer, op, disasm) {
			var value = buffer[disasm.index] | (buffer[disasm.index + 1] << 8);
			disasm.index += 2;

			if (0 == (op.modifiers & Disassembler.Modifiers.MODIFIER_OPERAND_16)) {
				value |= (buffer[disasm.index] << 16) | (buffer[disasm.index + 1] << 24);
				disasm.index += 2;				
			}

			op.operands[position] = {
				value : value,
				type : Disassembler.OperandType.OPTYPE_IMM
			};
		};
	}

	var DisassembleImm8 = [
		ConstructImmediateOperand8(0),
		ConstructImmediateOperand8(1)
	];
	var DisassembleImm = [
		ConstructImmediateOperand(0),
		ConstructImmediateOperand(1)
	];

	function ConstructRegisterOperand(position) {
		return function (op, register) {
			op.operands[position] = {
				asRegister : register,
				type : Disassembler.OperandType.OPTYPE_REGISTER
			};

			if (ns.Disassembler.Modifiers.MODIFIER_OPERAND_8 & op.modifiers) {
				op.operands[position].asRegister |= ns.Disassembler.GenericRegisterSizes.RIVER_REG_SZ8_L;
			}

			if (ns.Disassembler.Modifiers.MODIFIER_OPERAND_16 & op.modifiers) {
				op.operands[position].asRegister |= ns.Disassembler.GenericRegisterSizes.RIVER_REG_SZ16;
			}
		};
	}

	var DisassembleRegisterOperand = [
		ConstructRegisterOperand(0),
		ConstructRegisterOperand(1)
	];

	function DoNothing() {}

	function ConstructMemoryOperand(position, extraFunc) {
		return function (buffer, op, disasm) {
			var modRM = buffer[disasm.index];
			disasm.index++;

			var mod = modRM >> 6;
			var rm = modRM & 0x07;
			var extra = (modRM >> 3) & 0x07;

			extraFunc(op, extra);

			op.operands[position] = {
				type : Disassembler.OperandType.OPTYPE_MEMORY,
				dispType : -1
			};

			switch (mod) {
				case 0 :
					op.operands[position].dispType = ns.Disassembler.DisplacementSize.DISP_NONE;
					break;
				case 1 :
					op.operands[position].dispType = ns.Disassembler.DisplacementSize.DISP_8BIT;
					break;
				case 2 :
					op.operands[position].dispType = ns.Disassembler.DisplacementSize.DISP_NATIVE;
					break;
				case 3 :
					DisassembleRegisterOperand[position](op, rm);
					return;
			};

			if (4 == rm) { // decode sib
				var sib = buffer[disasm.index];
				disasm.index++;

				var ss = sib >> 6;
				var idx = (sib >> 3) & 0x7;
				var bs = sib & 0x7;

				op.operands[position].scale = (1 << ss);

				if (4 != idx) {
					op.operands[position].index = idx;
				}

				if (5 == bs) {
					op.operands[position].dispType = 
						(1 & mod) ? 
							ns.Disassembler.DisplacementSize.DISP_8BIT : 
							ns.Disassembler.DisplacementSize.DISP_NATIVE;
					if (0 != mod) {
						op.operands[position].base = ns.Disassembler.GenericRegisters.xBP;
					}
				} else {
					op.operands[position].base = bs;
				}
			} else if ((0 == mod) && (5 == rm)) {
				op.operands[position].dispType = ns.Disassembler.DisplacementSize.DISP_NATIVE;
			} else {
				op.operands[position].base = rm;
			}

			if (op.operands[position].dispType == ns.Disassembler.DisplacementSize.DISP_8BIT) {
				op.operands[position].displacement = buffer[disasm.index];
				disasm.index++;
			} else if (op.operands[position].dispType == ns.Disassembler.DisplacementSize.DISP_NATIVE) {
				op.operands[position].displacement = 
					(buffer[disasm.index + 0] <<  0) |
					(buffer[disasm.index + 1] <<  8) |
					(buffer[disasm.index + 2] << 16) |
					(buffer[disasm.index + 3] << 24);
				disasm.index += 4;
			}
		};
	}

	var DisassembleRegModRM = ConstructMemoryOperand(1, DisassembleRegisterOperand[0]);
	var DisassembleModRMReg = ConstructMemoryOperand(0, DisassembleRegisterOperand[1]);

	var DisassembleSubModRM = ConstructMemoryOperand(0, function (op, extra) {
		op.subOpcode = extra;
	});

	var DisassembleModRM0 = ConstructMemoryOperand(0, DoNothing);

	function ConstructCompoundFunction(f1, f2) {
		return function(buffer, op, disasm) {
			f1(buffer, op, disasm);
			f2(buffer, op, disasm);
		};
	}

	var DisassembleModRmImm8 = ConstructCompoundFunction(DisassembleModRM0, ConstructImmediateOperand8(1));
	var DisassembleModRmImm = ConstructCompoundFunction(DisassembleModRM0, ConstructImmediateOperand(1));

	function ConstructMoffsOperand(position) {
		return function(buffer, op, disasm) {
			op.operands[position] = {
				type : Disassembler.OperandType.OPTYPE_MEMORY,
				dispType : ns.Disassembler.DisplacementSize.DISP_NATIVE,
				displacement : 
					(buffer[disasm.index + 0] <<  0) |
					(buffer[disasm.index + 1] <<  8) |
					(buffer[disasm.index + 2] << 16) |
					(buffer[disasm.index + 3] << 24)
			};

			disasm.index += 4;
		};
	};

	var DisassembleMoffs = [
		ConstructMoffsOperand(0),
		ConstructMoffsOperand(1)
	];

	function ConstructSubOperandDisassembler(ops) {
		return function(buffer, op, disasm) {
			ops[op.subOpcode](buffer, op, disasm);
		};
	}

	var DisasmOperand0xFF = ConstructSubOperandDisassembler([
		DisassembleModRM0,
		DisassembleModRM0,
		DisasmNoOperand,
		DisasmUnknownOperand,
		DisasmNoOperand,
		DisasmUnknownOperand,
		DisassembleModRM0,
		DisasmUnknownOperand
	]);

	var DisasmOperands = [
		[
			/*0x00*/ DisassembleModRMReg, DisassembleModRMReg, DisassembleRegModRM, DisassembleRegModRM,
			/*0x04*/ DisassembleImm8[1], DisassembleImm[1], DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x08*/ DisassembleModRMReg, DisassembleModRMReg, DisassembleRegModRM, DisassembleRegModRM,
			/*0x0C*/ DisassembleImm8[1], DisassembleImm[1], DisasmUnknownOperand, DisasmNoOperand,

			/*0x10*/ DisassembleModRMReg, DisassembleModRMReg, DisassembleRegModRM, DisassembleRegModRM,
			/*0x14*/ DisassembleImm8[1], DisassembleImm[1], DisasmNoOperand, DisasmNoOperand,
			/*0x18*/ DisassembleModRMReg, DisassembleModRMReg, DisassembleRegModRM, DisassembleRegModRM,
			/*0x1C*/ DisassembleImm8[1], DisassembleImm[1], DisasmUnknownOperand, DisasmUnknownOperand,

			/*0x20*/ DisassembleModRMReg, DisassembleModRMReg, DisassembleRegModRM, DisassembleRegModRM,
			/*0x24*/ DisassembleImm8[1], DisassembleImm[1], DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x28*/ DisassembleModRMReg, DisassembleModRMReg, DisassembleRegModRM, DisassembleRegModRM,
			/*0x2C*/ DisassembleImm8[1], DisassembleImm[1], DisasmUnknownOperand, DisasmUnknownOperand,

			/*0x30*/ DisassembleModRMReg, DisassembleModRMReg, DisassembleRegModRM, DisassembleRegModRM,
			/*0x34*/ DisassembleImm8[1], DisassembleImm[1], DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x38*/ DisassembleModRMReg, DisassembleModRMReg, DisassembleRegModRM, DisassembleRegModRM,
			/*0x3C*/ DisassembleImm8[1], DisassembleImm[1], DisasmUnknownOperand, DisasmUnknownOperand,

			/*0x40*/ DisasmNoOperand, DisasmNoOperand, DisasmNoOperand, DisasmNoOperand,
			/*0x44*/ DisasmNoOperand, DisasmNoOperand, DisasmNoOperand, DisasmNoOperand,
			/*0x48*/ DisasmNoOperand, DisasmNoOperand, DisasmNoOperand, DisasmNoOperand,
			/*0x4C*/ DisasmNoOperand, DisasmNoOperand, DisasmNoOperand, DisasmNoOperand,

			/*0x50*/ DisasmNoOperand, DisasmNoOperand, DisasmNoOperand, DisasmNoOperand,
			/*0x54*/ DisasmNoOperand, DisasmNoOperand, DisasmNoOperand, DisasmNoOperand,
			/*0x58*/ DisasmNoOperand, DisasmNoOperand, DisasmNoOperand, DisasmNoOperand,
			/*0x5C*/ DisasmNoOperand, DisasmNoOperand, DisasmNoOperand, DisasmNoOperand,

			/*0x60*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x64*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x68*/ DisassembleImm[0], DisasmUnknownOperand, DisassembleImm8[0], DisasmUnknownOperand,
			/*0x6C*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,

			/*0x70*/ DisassembleImm8[0], DisassembleImm8[0], DisassembleImm8[0], DisassembleImm8[0],
			/*0x74*/ DisassembleImm8[0], DisassembleImm8[0], DisassembleImm8[0], DisassembleImm8[0],
			/*0x78*/ DisassembleImm8[0], DisassembleImm8[0], DisassembleImm8[0], DisassembleImm8[0],
			/*0x7C*/ DisassembleImm8[0], DisassembleImm8[0], DisassembleImm8[0], DisassembleImm8[0],

			/*0x80*/ DisassembleModRmImm8, DisassembleModRmImm, DisassembleModRmImm8, DisassembleModRmImm8,
			/*0x84*/ DisassembleRegModRM, DisassembleRegModRM, DisassembleRegModRM, DisassembleRegModRM,
			/*0x88*/ DisassembleModRMReg, DisassembleModRMReg, DisassembleRegModRM, DisassembleRegModRM,
			/*0x8C*/ DisasmUnknownOperand, DisassembleRegModRM, DisasmUnknownOperand, DisasmUnknownOperand,

			/*0x90*/ DisasmNoOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x94*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x98*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x9C*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,

			/*0xA0*/ DisassembleMoffs[1], DisassembleMoffs[1], DisassembleMoffs[0], DisassembleMoffs[0],
			/*0xA4*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xA8*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xAC*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,

			/*0xB0*/ DisassembleImm8[1], DisassembleImm8[1], DisassembleImm8[1], DisassembleImm8[1],
			/*0xB4*/ DisassembleImm8[1], DisassembleImm8[1], DisassembleImm8[1], DisassembleImm8[1],
			/*0xB8*/ DisassembleImm[1], DisassembleImm[1], DisassembleImm[1], DisassembleImm[1],
			/*0xBC*/ DisassembleImm[1], DisassembleImm[1], DisassembleImm[1], DisassembleImm[1],

			/*0xC0*/ DisasmUnknownOperand, DisasmUnknownOperand, DisassembleImm[0], DisasmNoOperand,
			/*0xC4*/ DisasmUnknownOperand, DisasmUnknownOperand, DisassembleModRmImm8, DisassembleModRmImm,
			/*0xC8*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xCC*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,

			/*0xD0*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xD4*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xD8*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xDC*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,

			/*0xE0*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xE4*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xE8*/ DisassembleImm[0], DisassembleImm[0], DisasmUnknownOperand, DisassembleImm8[0],
			/*0xEC*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,

			/*0xF0*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xF4*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xF8*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xFC*/ DisasmNoOperand, DisasmNoOperand, DisasmUnknownOperand, DisasmOperand0xFF
		], [
			/*0x00*/ DisasmUnknownOperand, DisasmUnknownOperand, DisassembleRegModRM, DisassembleRegModRM,
			/*0x04*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x08*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x0C*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,

			/*0x10*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x14*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x18*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x1C*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,

			/*0x20*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x24*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x28*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x2C*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,

			/*0x30*/ DisasmUnknownOperand, DisasmNoOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x34*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x38*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x3C*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,

			/*0x40*/ DisassembleRegModRM, DisassembleRegModRM, DisassembleRegModRM, DisassembleRegModRM,
			/*0x44*/ DisassembleRegModRM, DisassembleRegModRM, DisassembleRegModRM, DisassembleRegModRM,
			/*0x48*/ DisassembleRegModRM, DisassembleRegModRM, DisassembleRegModRM, DisassembleRegModRM,
			/*0x4C*/ DisassembleRegModRM, DisassembleRegModRM, DisassembleRegModRM, DisassembleRegModRM,

			/*0x50*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x54*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x58*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x5C*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,

			/*0x60*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x64*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x68*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x6C*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,

			/*0x70*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x74*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x78*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x7C*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,

			/*0x80*/ DisassembleImm[0], DisassembleImm[0], DisassembleImm[0], DisassembleImm[0],
			/*0x84*/ DisassembleImm[0], DisassembleImm[0], DisassembleImm[0], DisassembleImm[0],
			/*0x88*/ DisassembleImm[0], DisassembleImm[0], DisassembleImm[0], DisassembleImm[0],
			/*0x8C*/ DisassembleImm[0], DisassembleImm[0], DisassembleImm[0], DisassembleImm[0],

			/*0x90*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x94*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x98*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0x9C*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,

			/*0xA0*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xA4*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xA8*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xAC*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisassembleRegModRM,

			/*0xB0*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xB4*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xB8*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xBC*/ DisassembleRegModRM, DisassembleRegModRM, DisassembleRegModRM, DisassembleRegModRM,

			/*0xC0*/ DisassembleModRMReg, DisassembleModRMReg, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xC4*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xC8*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xCC*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,

			/*0xD0*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xD4*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xD8*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xDC*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,

			/*0xE0*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xE4*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xE8*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xEC*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,

			/*0xF0*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xF4*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xF8*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand,
			/*0xFC*/ DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand, DisasmUnknownOperand
		]
	];

	function GetOperands(buffer, op, disasm) {
		DisasmOperands[(op.modifiers & ns.Disassembler.Modifiers.MODIFIER_EXT_PFX) ? 1 : 0][op.opcode](buffer, op, disasm);
	}


	ns.Disassembler.prototype.GetOpcode = function(buffer, op) {
		while (!GetOpcode(buffer, op, this)) { };

		GetOperands(buffer, op, this);
	}

	ns.Disassembler.prototype.Disassemble = function(buffer, offset, index, flags) {
		this.offset = offset;
		var start = index;
		this.index = index;
		var instr = [];

		while (this.index < buffer.length) {
			try {
				var newInstr = {
					opcode : 0,
					subOpcode : 0,
					operands : [],

					modifiers : 0,
					segment: "",
					prefixes : [],
					mnemonic : "",

					offset : this.index
				};

				//console.log(this.index);
				this.GetOpcode(buffer, newInstr);
				newInstr.size = this.index - newInstr.offset;
				instr.push(newInstr);

				//console.log(PrintInstruction(newInstr));
			} catch (ex) {
				//throw ex;
				console.log(ex);

				debugger;
				break;
			}

			if ((1 == flags) && (newInstr.modifiers & (Disassembler.Modifiers.MODIFIER_BRANCH | Disassembler.Modifiers.MODIFIER_RETURN))) {
				break;
			}
		}

		var branches = [];
		var lastInstr = instr[instr.length - 1];

		if (0 == (lastInstr.modifiers & Disassembler.Modifiers.MODIFIER_NO_FALLTROUGH)) {
			branches.push(this.offset + lastInstr.offset + lastInstr.size);
		}

		if (lastInstr.modifiers & Disassembler.Modifiers.MODIFIER_FIXED_BRANCH) {
			branches.push(lastInstr.nextInstruction + lastInstr.operands[0].value);
		}

		return {
			address : offset + start,
			instructions : instr,
			size : this.index - start,
			branches : branches
		};
	};

	var subMnemonics = [
		/*for unknown */[ "___", "___", "___", "___", "___", "___", "___", "___" ],
		/*for 0x83    */[ "add",  "or", "adc", "sbb", "and", "sub", "xor", "cmp" ],
		/*for 0xC0/C1 */[ "rol", "ror", "rcl", "rcr", "shl", "shr", "sal", "sar"],
		/*for 0xFF    */[ "inc", "dec", "call", "call", "jmp", "jmp", "push", "___" ],
		/*for 0xF6/F7 */[ "test", "test", "not", "neg", "mul", "imul", "div", "idiv" ],
		/*for 0x0FBA  */[ "___", "___", "___", "___", "bt", "bts", "btr", "btc" ],
		/*for 0xFE    */[ "inc", "dec", "___", "___", "___", "___", "___", "___" ],
		/*for 0x0FC7  */[ "___", "cmpxchg8b", "___", "___", "___", "___", "___", "___" ]
	];

	var mnemonics = [
		[
			/*0x00*/"add", "add", "add", "add", "add", "add", "", "", "or",  "or",  "or",  "or",  "or",  "or", "", "",
			/*0x10*/"adc", "adc", "adc", "adc", "adc", "adc", "", "", "sbb", "sbb", "sbb", "sbb", "sbb", "sbb", "", "",
			/*0x20*/"and", "and", "and", "and", "and", "and", "", "", "sub", "sub", "sub", "sub", "sub", "sub", "", "",
			/*0x30*/"xor", "xor", "xor", "xor", "xor", "xor", "", "", "cmp", "cmp", "cmp", "cmp", "cmp", "cmp", "", "",
			/*0x40*/"inc", "inc", "inc", "inc", "inc", "inc", "inc", "inc", "dec", "dec", "dec", "dec", "dec", "dec", "dec", "dec",
			/*0x50*/"push", "push", "push", "push", "push", "push", "push", "push", "pop", "pop", "pop", "pop", "pop", "pop", "pop", "pop",
			/*0x60*/"", "", "", "", "", "", "", "", "push", "imul", "push", "imul", "", "", "", "",
			/*0x70*/"jo", "jno", "jc", "jnc", "jz", "jnz", "jbe", "jnbe", "js", "jns", "jp", "jnp", "jl", "jnl", "jle", "jnle",
			/*0x80*/subMnemonics[1], subMnemonics[1], subMnemonics[1], subMnemonics[1], "test", "test", "xchg", "xchg", "mov", "mov", "mov", "mov", "", "lea", "", "pop",
			/*0x90*/"nop", "xchg", "xchg", "xchg", "xchg", "xchg", "xchg", "xchg", "", "cdq", "", "", "pushf", "popf", "", "",
			/*0xA0*/"mov", "mov", "mov", "mov", "movs", "movs", "cmps", "cmps", "test", "test", "stos", "stos", "", "", "scas", "scas",
			/*0xB0*/"mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov", "mov",
			/*0xC0*/subMnemonics[2], subMnemonics[2], "retn", "retn", "", "", "mov", "mov", "", "leave", "", "", "", "", "", "",
			/*0xD0*/subMnemonics[2], subMnemonics[2], subMnemonics[2], subMnemonics[2], "", "", "", "", "", "", "", "", "", "", "", "",
			/*0xE0*/"", "", "", "", "", "", "", "", "call", "jmp", "jmpf", "jmp", "", "", "", "",
			/*0xF0*/"", "", "", "", "", "", subMnemonics[4], subMnemonics[4], "", "", "", "", "cld", "std", subMnemonics[6], subMnemonics[3]
		],[
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
			/*0xB0*/"cmpxchg", "cmpxchg", "", "", "", "", "movzx", "movzx", "", "", subMnemonics[5], "", "bsf", "bsr", "movsx", "movsx",
			/*0xC0*/"xadd", "xadd", "", "", "", "", "", subMnemonics[7], "", "", "", "", "", "", "", "",
			/*0xD0*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
			/*0xE0*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
			/*0xF0*/"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""
		]
	];

	function PrintOperand(mod, op) {
		switch (op.type) {
			case Disassembler.OperandType.OPTYPE_IMM8 :
				return "0x" + ("00" + op.value.toString(16)).substr(-2);
			case Disassembler.OperandType.OPTYPE_IMM :
				return "0x" + ("00000000" + (op.value >>> 0).toString(16)).substr(-8);
			case Disassembler.OperandType.OPTYPE_REGISTER :
				return Disassembler.RegisterNames[op.asRegister];
			case Disassembler.OperandType.OPTYPE_MEMORY :
				var r = "";

				if (mod & Disassembler.Modifiers.MODIFIER_OPERAND_8) {
					r = "BYTE PTR [";
				} else if (mod & Disassembler.Modifiers.MODIFIER_OPERAND_16) {
					r = "WORD PTR [";
				} else {
					r = "DWORD PTR [";
				}

				var t = "";
				if ("undefined" !== typeof op.base) {
					t = Disassembler.RegisterNames[op.base];
				}

				if ("undefined" !== typeof op.index) {
					if (t != "") {
						t += " + ";
					}
					t += op.scale + " * " + Disassembler.RegisterNames[op.index];
				}

				if ("undefined" !== typeof op.displacement) {
					if (t != "") {
						t += " + ";
					}

					t += "0x";

					if (op.dispType == Disassembler.DisplacementSize.DISP_8BIT) {
						t += ("00" + op.displacement.toString(16)).substr(-2);
					} else if (op.dispType == Disassembler.DisplacementSize.DISP_NATIVE) {
						t += ("00000000" + (op.displacement >>> 0).toString(16)).substr(-8);
					} else {
						t += "??";
					}
				}



				return r + t + "]";
			default :
				return "<wat?>"
		} 
	}

	ns.PrintInstruction = function (instruction) {
		var str = "";
		var t = mnemonics[(instruction.modifiers & Disassembler.Modifiers.MODIFIER_EXT_PFX) ? 1 : 0][instruction.opcode];

		if ("object" === typeof t) {
			t = t[instruction.subOpcode];
		}

		var str = t;
		for (var i = 0; i < instruction.operands.length; ++i) {
			str += " " + PrintOperand(instruction.modifiers, instruction.operands[i]);
			if (i < instruction.operands.length - 1) {
				str += ","
			}
		}

		//console.log(t);
		return str;
	}

	function NextPow2(x) {
		x |= x >>  1;
		x |= x >>  2;
		x |= x >>  4;
		x |= x >>  8;
		x |= x >> 16;

		return x + 1;
	}


	ns.CodeSection = function (buffer, offset) {
		this.buffer = buffer;
		this.offset = offset;

		this.objects = [];	
		this.objects[0] = {
			offset : 0,
			size : this.buffer.length,
			bytes : this.buffer.slice()
		};
	};

	ns.CodeSection.prototype.FindObject = function(object) {
		var s = NextPow2(this.objects.length + 1);
		var p = -1;

		for (; s > 0; s >>= 1) {
			if (s + p < this.objects.length) {
				if (this.objects[s + p].offset == object.offset) {
					if (this.objects[s + p].size == object.size) {
						return s + p;
					} else {
						return -1;
					}
				} else if (this.objects[s + p].offset < object.offset) {
					p += s;
				}
			}
		}

		return -1;
	};

	ns.CodeSection.prototype.FindObjectByOffset = function(offset) {
		var s = NextPow2(this.objects.length + 1);
		var p = -1;

		for (; s > 0; s >>= 1) {
			if (s + p < this.objects.length) {
				if (this.objects[s + p].offset <= offset) {
					if (this.objects[s + p].offset + this.objects[s + p].size > offset) {
						return this.objects[s + p];
					} else {
						p += s;
					}
				} else if (this.objects[s + p].offset < offset) {
					p += s;
				}
			}
		}

		return -1;
	};

	ns.CodeSection.prototype.SplitObject = function(oldObject, instr) {
		var idx = this.FindObject(oldObject);
		var ins = [idx, 1];



		if (instr[0].offset > 0) {
			ins.push({
				offset : oldObject.offset,
				size : instr[0].offset,
				bytes : oldObject.bytes.slice(0, instr[0].offset)
			});
		}

		for (var i = 0; i < instr.length; ++i) {
			ins.push({
				offset : instr[i].offset + oldObject.offset,
				size : instr[i].size,
				bytes : oldObject.bytes.slice(instr[i].offset - oldObject.offset, instr[i].offset - oldObject.offset + instr[i].size),
				code : instr[i]
			});
		}

		var nextByte =  instr[instr.length - 1].offset + instr[instr.length - 1].size;
		var nextOffset = nextByte + oldObject.offset;
		if (nextByte < oldObject.size) {
			ins.push({
				offset : nextOffset,
				size : oldObject.size - nextByte,
				bytes : oldObject.bytes.slice(nextByte, oldObject.size)
			});
		}

		this.objects.splice.apply(this.objects, ins);

	};

	ns.CodeSection.prototype.Disassemble = function(disasm) {
		var queue = [this.offset];

		while (queue.length) {
			var addr = queue.pop();

			console.log("Analisys @ " + addr);
			var obj = this.FindObjectByOffset(addr);

			if ("undefined" === typeof obj.code) {
				var ret = disasm.Disassemble(obj.bytes, obj.offset, addr - obj.offset, 1);

				this.SplitObject(obj, ret.instructions);

				for (var i = 0; i < ret.branches.length; ++i) {
					if ((ret.branches[i] >= this.offset) && (ret.branches[i] < this.offset + this.buffer.length)) {
						console.log("Push bb " + ret.branches[i] + " while analyzing " + addr);
						queue.push(ret.branches[i]);
					} 
				}
			}
		}
	}
})(window);