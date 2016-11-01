#ifdef __linux__
#include "ExternExecutionController.h"

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <string.h>

ExternExecutionController::ExternExecutionController() {
	shmAlloc = -1;
}

bool ExternExecutionController::SetEntryPoint() {
	return false;
}

bool ExternExecutionController::InitializeAllocator() {
	shmAlloc = shm_open("/thug_life", O_CREAT | O_RDWR | O_EXCL, 0744);
	if (shmAlloc) {
		printf("Could not allocate shared memory chunk. Exiting.\n");
		return false;
	}
	return true;
}

void ExternExecutionController::ConvertWideStringPath(char *result, size_t len) {
	memset(result, 0, len);
	const wchar_t *pathStream = path.c_str();
	std::wcstombs(result, pathStream, len);
}

const int long_size = sizeof(long);

void getdata(pid_t child, long addr,
		unsigned char *str, int len)
{   unsigned char *laddr;
	int i, j;
	union u {
		long val;
		char chars[long_size];
	}data;
	i = 0;
	j = len / long_size;
	laddr = str;
	while(i < j) {
		data.val = ptrace(PTRACE_PEEKDATA, child,
				addr + i * 4, NULL);
		memcpy(laddr, data.chars, long_size);
		++i;
		laddr += long_size;
	}
	j = len % long_size;
	if(j != 0) {
		data.val = ptrace(PTRACE_PEEKDATA, child,
				addr + i * 4, NULL);
		memcpy(laddr, data.chars, j);
	}
	str[len] = '\0';
}
void putdata(pid_t child, long addr,
		unsigned char *str, int len)
{   unsigned char *laddr;
	int i, j;
	union u {
		long val;
		char chars[long_size];
	}data;
	i = 0;
	j = len / long_size;
	laddr = str;
	while(i < j) {
		memcpy(data.chars, laddr, long_size);
		ptrace(PTRACE_POKEDATA, child,
				addr + i * 4, data.val);
		++i;
		laddr += long_size;
	}
	j = len % long_size;
	if(j != 0) {
		memcpy(data.chars, laddr, j);
		ptrace(PTRACE_POKEDATA, child,
				addr + i * 4, data.val);
	}
}

void InsertBreakpoint(DWORD address, pid_t child, unsigned char *bkp) {
	unsigned char code[] = {0xcd,0x80,0xcc,0};

	printf("Inserting breakpoint at address %lx\n", address);
	getdata(child, address, bkp, 3);
	putdata(child, address, code, 3);

}

void DeleteBreakpoint(DWORD address, pid_t child, unsigned char *bkp) {
	printf("Deleting breakpoint at address %lx\n", address);
	putdata(child, address, bkp, 3);
}

bool ExternExecutionController::Execute() {

	if (!InitializeAllocator()) {
		execState = ERR;
		DEBUG_BREAK;
		return false;
	}

	pid_t child;
	int status;
	struct user_regs_struct regs;

	/* int 0x80, int3 */
	unsigned char backup[4];

	char arg[MAX_PATH];
	DWORD entryPoint;

	ConvertWideStringPath(arg, MAX_PATH);
	entryPoint = GetEntryPoint(arg);

	child = fork();
	if(child == 0) {
		ptrace(PTRACE_TRACEME, 0, NULL, NULL);
		execve(arg, (char**)&arg, NULL);
	}

	else {
		wait(&status);

		ptrace(PTRACE_GETREGS, child, 0, &regs);
		printf("Child started. EIP = 0x%08lx\n", regs.eip);
		fflush(stdout);

		InsertBreakpoint(entryPoint, child, backup);

		//continue
		ptrace(PTRACE_CONT, child, NULL, NULL);
		wait(NULL);

		ptrace(PTRACE_GETREGS, child, 0, &regs);
		printf("Child should break at entry point. EIP = 0x%08lx\n", regs.eip);
		fflush(stdout);

		DeleteBreakpoint(entryPoint, child, backup);

		//try to load .so in here
		ptrace(PTRACE_CONT, child, NULL, NULL);
		wait(&status);

		if (WIFEXITED(status) ) {
			int es = WEXITSTATUS(status);
			printf("Exit status was %d\n", es);
		}
		fflush(stdout);

		ptrace (PTRACE_DETACH, child, NULL, NULL);
	}


	// create child process and trace it
	// create shared mem and map it in the child process
	// map revtracer.dll and libipc.so in the child process
	// continue in child process
}
#endif
