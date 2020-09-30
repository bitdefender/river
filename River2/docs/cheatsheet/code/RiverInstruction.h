struct RiverInstruction {
	nodep::WORD modifiers;
	nodep::WORD specifiers;

	nodep::BYTE family;
	nodep::BYTE unusedRegisters;
	nodep::BYTE opCode;
	nodep::BYTE subOpCode;

	nodep::BYTE disassFlags, modFlags, testFlags;

	nodep::BYTE opTypes[4];
	union RiverOperand operands[4];

	nodep::DWORD instructionAddress;
};
