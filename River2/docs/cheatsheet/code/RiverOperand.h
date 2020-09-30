union RiverOperand {
	nodep::BYTE asImm8;
	nodep::WORD asImm16;
	nodep::DWORD asImm32;
	RiverRegister asRegister;
	RiverAddress *asAddress;
};
