#ifdef  __linux__

#include "Debugger.h"

#include <sys/wait.h>
#include <sys/user.h>
#include <string.h>

namespace dbg {

#if defined(__i386)
#define REGISTER_IP EIP
#define TRAP_LEN    1
#define TRAP_INST   0xCC
#define TRAP_MASK   0xFFFFFF00
#endif

	const int long_size = sizeof(long);
	void Debugger::GetData(long addr, unsigned char *str, int len)
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
			data.val = ptrace(PTRACE_PEEKDATA, Tracee,
					addr + i * 4, nullptr);
			memcpy(laddr, data.chars, long_size);
			++i;
			laddr += long_size;
		}
		j = len % long_size;
		if(j != 0) {
			data.val = ptrace(PTRACE_PEEKDATA, Tracee,
					addr + i * 4, nullptr);
			memcpy(laddr, data.chars, j);
		}
		str[len] = '\0';
	}

	void Debugger::PutData(long addr, unsigned char *str, int len)
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
			ptrace(PTRACE_POKEDATA, Tracee,
					addr + i * 4, data.val);
			++i;
			laddr += long_size;
		}
		j = len % long_size;
		if(j != 0) {
			memcpy(data.chars, laddr, j);
			ptrace(PTRACE_POKEDATA, Tracee,
					addr + i * 4, data.val);
		}
	}

	void Debugger::SetEip(unsigned long address) {
		struct user_regs_struct regs;
		ptrace(PTRACE_GETREGS, Tracee, 0, &regs);
		regs.eip = address;
		ptrace (PTRACE_SETREGS, Tracee, 0, &regs);
	}

	void Debugger::PrintEip() {
		struct user_regs_struct regs;
		ptrace(PTRACE_GETREGS, Tracee, 0, &regs);
		printf("[Debugger] Tracee process %d stopped at eip %lx\n", Tracee, regs.eip);
	}

	void Debugger::PrintRegs() {
		struct user_regs_struct regs;
		ptrace(PTRACE_GETREGS, Tracee, 0, &regs);
		printf("[Debugger] EAX: %08lx  EDX: %08lx  ECX: %08lx  EBX: %08lx\n", regs.eax, regs.edx, regs.ecx, regs.ebx);
		printf("[Debugger] ESP: %08lx  EBP: %08lx  ESI: %08lx  EDI: %08lx\n", regs.esp, regs.ebp, regs.esi, regs.edi);
	}

	int Debugger::CheckEip(unsigned eip) {
		struct user_regs_struct regs;
		ptrace(PTRACE_GETREGS, Tracee, 0, &regs);
		printf("[Debugger] Tracee process %d testing eip %08lx with limit %08x\n", Tracee, regs.eip, eip);
		return eip == regs.eip;
	}

	Debugger::Debugger() {
		Tracee = -1;
	}

	void Debugger::Attach(pid_t pid) {
		if (Tracee != -1) {
			printf("[Debugger] Already tracing %d\n", Tracee);
			return;
		}

		Tracee = pid;
		int status;

		waitpid(pid, &status, 0);

		struct user_regs_struct regs;
		ptrace(PTRACE_GETREGS, pid, 0, &regs);
		printf("[Debugger] Attached to pid %d, ip %08lx\n", pid, regs.eip);
	}

	unsigned long Debugger::InsertBreakpoint(unsigned long address) {
		printf("[Debugger] Inserting breakpoint at address %lx\n", address);

		long orig = ptrace(PTRACE_PEEKTEXT, Tracee, address);
		ptrace(PTRACE_POKETEXT, Tracee, address, (orig & TRAP_MASK) | TRAP_INST);
		BreakpointCode.insert(std::make_pair(address, orig));
		return address;
	}

	void Debugger::DeleteBreakpoint(unsigned long address) {
		printf("[Debugger] Deleting breakpoint at address %lx\n", address);
		if (BreakpointCode.find(address) == BreakpointCode.end()) {
			printf("[Debugger] Breakpoint not found at address %lx\n", address);
			return;
		}
		ptrace(PTRACE_POKETEXT, Tracee, address, BreakpointCode[address]);
		SetEip(address);
	}

	int Debugger::Run(enum __ptrace_request request) {
		int status, last_sig = 0, event;

		while (1) {
			ptrace(request, Tracee, 0, last_sig);
			waitpid(Tracee, &status, WCONTINUED);

			if (WIFEXITED(status)) {
				printf("[Debugger] Pid %d exited\n", Tracee);
				return 0;
			}

			if (WIFSTOPPED(status)) {
				last_sig = WSTOPSIG(status);
				if (last_sig == SIGTRAP) {
					event = (status >> 16) & 0xffff;
					printf("[Debugger] Tracee received SIGTRAP\n");
					PrintEip();
					return (event == PTRACE_EVENT_EXIT) ? 0 : 1;
				} else if (last_sig == SIGUSR1) {
					printf("[Debugger] Child received SIGUSR1, continuing ...\n");
					fflush(stdout);
				} else {
					printf("[Debugger] Tracee received signal %d\n", last_sig);
					PrintEip();
					PrintRegs();
					if (last_sig == 11) {
						return -1;
					}
				}
			}
		}
		return 0;
	}


	unsigned long Debugger::GetAndResolveModuleAddress(unsigned long symbolAddress) {
		unsigned char sym_addr[5] = {0, 0, 0, 0, 0};
		//printf("[Parent] Trying to read data from child %08lx\n", symbolAddress);
		GetData(symbolAddress, sym_addr, 4);

		unsigned long result;
		for (int i = 0; i < 4; ++i) {
			((char*)&result)[i] = sym_addr[i];
		}
		return result;
	}

} //namespace dbg

#endif
