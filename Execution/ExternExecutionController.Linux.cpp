#ifdef __linux__
#include "ExternExecutionController.h"

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <string.h>
#include "../libproc/os-linux.h"

// TODO seach for this lib in LD_LIBRARY_PATH
#define LOADER_PATH "libloader.so"

ExternExecutionController::ExternExecutionController() {
	shmAlloc = -1;
}

bool ExternExecutionController::SetEntryPoint() {
	return false;
}

bool ExternExecutionController::InitializeAllocator() {
	shmAlloc = shm_open("/thug_life", O_CREAT | O_RDWR | O_TRUNC | O_EXCL, 0644);
	if (shmAlloc == -1) {
		printf("Could not allocate shared memory chunk. Exiting. %d\n", errno);
		strerror(errno);
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
				addr + i * 4, nullptr);
		memcpy(laddr, data.chars, long_size);
		++i;
		laddr += long_size;
	}
	j = len % long_size;
	if(j != 0) {
		data.val = ptrace(PTRACE_PEEKDATA, child,
				addr + i * 4, nullptr);
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

void MapSharedLibraries(unsigned long baseAddress) {
}

unsigned long GetLoaderAddress(pid_t pid) {
	struct map_iterator mi;
	struct map_prot mp;
	unsigned long hi;
	unsigned long segbase, mapoff;


	if (maps_init (&mi, pid) < 0) {
		printf("Cannot find maps for pid %d\n", pid);
		return 0;
	}

	while (maps_next (&mi, &segbase, &hi, &mapoff, &mp)) {
		if (nullptr != strstr(mi.path, LOADER_PATH)) {
			printf("Found child mapping for ld_preload %p\n", (void*)segbase);
			maps_close(&mi);
			return segbase;
		}

	}
	maps_close(&mi);
	return 0;
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

	unsigned long shmAddress = 0x0;
	child = fork();
	if(child == 0) {
		ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);

		char *const args[] = {arg, nullptr};
		char env[MAX_PATH] = "LD_PRELOAD=";
		strcat(env, LOADER_PATH);
		char *const envs[] = {env, nullptr};
		int ret = execve(arg, args, envs);

		if (ret == -1) {
			printf("Cannot start child process %s. Execve failed!\n", arg);
		}
	}

	else {
		wait(&status);

		ptrace(PTRACE_GETREGS, child, 0, &regs);
		printf("Child started. EIP = 0x%08lx\n", regs.eip);
		fflush(stdout);

		InsertBreakpoint(entryPoint, child, backup);

		ptrace(PTRACE_CONT, child, nullptr, nullptr);
		wait(nullptr);

		ptrace(PTRACE_GETREGS, child, 0, &regs);
		printf("Child should break at entry point. EIP = 0x%08lx\n", regs.eip);
		fflush(stdout);

		DeleteBreakpoint(entryPoint, child, backup);


		unsigned long symbolOffset = 0x2014;
		unsigned long symbolAddress = GetLoaderAddress(child) + symbolOffset;
		unsigned char sym_addr[4] = {0, 0, 0, 0};
		getdata(child, symbolAddress, sym_addr, 4);

		for (int i = 0; i < 4; ++i) {
			((char*)&shmAddress)[i] = sym_addr[i];
		}

		shmAddress = (unsigned long)mmap((void*)shmAddress, 1 << 30, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, shmAlloc, 0);
		printf("[Execution] Mapped shared memory at address %08lx", shmAddress);

		MapSharedLibraries(shmAddress);

		ptrace(PTRACE_CONT, child, nullptr, nullptr);
		wait(&status);

		if (WIFEXITED(status) ) {
			int es = WEXITSTATUS(status);
			printf("Exit status was %d\n", es);
		}
		fflush(stdout);

		ptrace (PTRACE_DETACH, child, nullptr, nullptr);
	}

}
#endif
