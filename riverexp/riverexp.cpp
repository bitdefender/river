#include "../src/concolicExecutor.h"
#include <string>

#define USE_IPC

int main()
{
	ExecutionOptions execOp;
	execOp.m_numProcessesToUse = 1;
	execOp.spawnTracersManually = false;
	execOp.testedLibrary = "libfmi.so";
	execOp.MAX_TRACER_INPUT_SIZE = 1024;
	execOp.MAX_TRACER_OUTPUT_SIZE = 1024*1024*10;

#ifdef USE_IPC
	execOp.m_execType = ExecutionOptions::EXEC_DISTRIBUTED_IPC;
	ConcolicExecutor cexec(execOp);
#else
	execOp.m_execType = ExecutionOptions::EXEC_SERIAL;
	ConcolicExecutor cexec("libfmi.so");
#endif
	ArrayOfUnsignedChars payloadInputExample = {'Y', '[', 'A', 'B'};
	cexec.searchSolutions(payloadInputExample, false);

	return 0;
}
