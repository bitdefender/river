#include "Environment.h"

#include "RevSymbolicEnvironment.h"
#include "OverlappedRegisters.h"

DLL_PUBLIC sym::SymbolicEnvironment *NewX86RevtracerEnvironment(void *revEnv, void *ctl) {
	return new RevSymbolicEnvironment(revEnv, (ExecutionController *)ctl);
}

DLL_PUBLIC sym::SymbolicEnvironment *NewX86RegistersEnvironment(sym::SymbolicEnvironment *parent) {
	sym::ScopedSymbolicEnvironment *ret = new OverlappedRegistersEnvironment();
	ret->SetSubEnvironment(parent);

	return ret;
}