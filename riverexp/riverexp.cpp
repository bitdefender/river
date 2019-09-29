#include "../src/concolicExecutor.h"
#include <string>

int main()
{
	ConcolicExecutor cexec(1024*1024*10, "libfmi.so");
	ArrayOfUnsignedChars payloadInputExample = {'Y', '[', 'A', 'B'};
	cexec.searchSolutions(payloadInputExample, true);

	return 0;
}
