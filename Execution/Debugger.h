#ifndef __DEBUGGER__
#define __DEBUGGER__

#include <map>
#include <unistd.h>
#include <sys/ptrace.h>

namespace dbg {
class Debugger {
	public:
		Debugger();
		void Attach(pid_t pid);
		void Detach();
		unsigned long InsertBreakpoint(unsigned long address);
		void DeleteBreakpoint(unsigned long address);
		int Run(enum __ptrace_request request);
		void SetEip(unsigned long address);
		void PrintEip();
		void GetData(long addr, unsigned char *str, int len);
		void PutData(long addr, unsigned char *str, int len);

	private:
		pid_t Tracee;
		std::map<unsigned long, long> BreakpointCode;

};
} //namespace dbg
#endif
