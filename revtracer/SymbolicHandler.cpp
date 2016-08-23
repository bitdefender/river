#include "revtracer.h"
#include "Runtime.h"
#include "SymbolicEnvironment.h"
#include "SymbolicEnvironmentImpl.h"

#include "RiverX86Disassembler.h"

static SymbolicEnvironmentImpl sEnv;

typedef SymbolicEnvironment *(* SymbolicEnvironmentConstructor)();

class SymbolicGlue {
private :
	SymbolicEnvironmentConstructor sEnvFunc;
	SymbolicExecutorConstructor sExeFunc;
public :
	SymbolicEnvironment *sEnv;
	SymbolicExecutor *sExe;

	SymbolicGlue() {
		sEnvFunc = nullptr;
		sExeFunc = nullptr;

		sEnv = nullptr;
		sExe = nullptr;
	}
	
	void SetEnvConstructor(SymbolicEnvironmentConstructor func) {
		sEnvFunc = func;
	}

	void SetExeConstructor(SymbolicExecutorConstructor func) {
		sExeFunc = func;
	}

	void SetEnv(SymbolicEnvironment *env) {
		sEnv = env;
	}

	void Init() {
		if (nullptr == sEnv) {
			if (nullptr != sEnvFunc) {
				sEnv = sEnvFunc();
			}
		}

		if (nullptr == sExe) {
			if (nullptr != sExeFunc) {
				sExe = sExeFunc(sEnv);
			}
		}
	}
};

SymbolicGlue glue;

extern "C" void SetSymbolicExecutor(SymbolicExecutorConstructor func) {
	glue.SetExeConstructor(func);
}

void InitSymbolicHandler(ExecutionEnvironment *pEnv) {
	sEnv.Init(pEnv);
	glue.SetEnv(&sEnv);

	glue.Init();
}

void *CreateSymbolicVariable(const char *name) {
	return glue.sExe->CreateVariable(name);
}


/*void __stdcall SymbolicHandler(ExecutionEnvironment *env, void *offset, rev::BYTE *address) {
	RiverInstruction *instr = (RiverInstruction *)address;
	sEnv.SetCurrentContext(instr, offset);

	glue.sExe->Execute(instr);
}*/

void __stdcall SymbolicHandler(ExecutionEnvironment *env, void *offset, RiverInstruction *instr) {
	sEnv.SetCurrentContext(instr, offset);

	glue.sExe->Execute(instr);
}


rev::DWORD dwSymbolicHandler = (rev::DWORD)&SymbolicHandler;

