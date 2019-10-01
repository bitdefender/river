#include "../src/concolicExecutor.h"
#include <string>

#define USE_IPC

int main()
{
	ExecutionOptions execOp;
	execOp.m_useIPC = true;
	execOp.m_numProcessesToUse = 1;
	execOp.spawnTracersManually = false;

#ifdef USE_IPC
	ConcolicExecutor cexec(1024, 1024*1024*10, "libfmi.so", execOp);
#else
	ConcolicExecutor cexec(1024, 1024*1024*10, "libfmi.so");
#endif
	ArrayOfUnsignedChars payloadInputExample = {'Y', '[', 'A', 'B'};
	cexec.searchSolutions(payloadInputExample, true);

	return 0;
}
