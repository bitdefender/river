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


void SymbolicHandler(ExecutionEnvironment *env, void *offset, rev::BYTE *address, rev::BYTE index) {
	RiverInstruction instr[16];
	rev::DWORD cnt = 0, flg = 0;

	env->codeGen.Reset();
	env->codeGen.DisassembleSingle(address, instr, cnt, flg);
	sEnv.SetCurrentContext(&instr[index], offset);

	glue.sExe->Execute(&instr[index]);
}


rev::DWORD dwSymbolicHandler = (rev::DWORD)&SymbolicHandler;

