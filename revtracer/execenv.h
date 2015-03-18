#ifndef _EXEC_ENV_H
#define _EXEC_ENV_H

//#include <Ntifs.h>
#include <Basetsd.h>
#include "environment.h"
#include "sync.h"
#include "virtualreg.h"
#include "river.h"
#include "mm.h"

struct _zone;
struct _cb_info;

#define RIVER_TRANSLATE_INSTRUCTIONS				128
#define RIVER_FORWARD_INSTRUCTIONS					512
#define RIVER_BACKWARD_INSTRUCTIONS					512

struct _exec_env {
	UINT_PTR virtualStack;				// + 0x00 - mandatory first member (used in vm-rm transitions)
	UINT_PTR returnRegister;			// + 0x04 - ax/eax/rax value
	UINT_PTR jumpBuff;					// + 0x08
	UINT_PTR execBuff;					// + 0x0C

	UINT_PTR saveLog;
	
	unsigned int /*heapSize,*/ historySize, logHashSize, outBufferSize;

	unsigned char *pStack; // = NULL;

	//unsigned char *pHeap; //NULL
	//struct _zone *pFirstFree; //NULL
	class RiverHeap heap;

	_tbm_mutex cbLock; //  = 0;
	struct _cb_info **hashTable; // = 0

	UINT_PTR *history;
	unsigned long posHist, totHist; // = 0;

	UINT_PTR *executionBuffer;

	unsigned char *outBuffer; // = NULL;
	unsigned char *saveBuffer;

	unsigned int bForward;

	struct RiverInstruction trRiverInst[RIVER_TRANSLATE_INSTRUCTIONS]; // to be removed in the near future
	struct RiverInstruction fwRiverInst[RIVER_FORWARD_INSTRUCTIONS];
	struct RiverInstruction bkRiverInst[RIVER_BACKWARD_INSTRUCTIONS];
	struct RiverAddress trRiverAddr[512];
	DWORD trInstCount, fwInstCount, bkInstCount, addrCount;

	unsigned int regVersions[8];


	void *userContext;
};

struct _exec_env *NewEnv(unsigned int heapSize, unsigned int historySize, unsigned int executionSize, unsigned int logHashSize, unsigned int outBufferSize);
void DeleteEnv(struct _exec_env *pEnv);

void *AllocUserContext(struct _exec_env *pEnv, unsigned int size);
void DeleteUserContext(struct _exec_env *pEnv);

struct RiverAddress *AllocRiverAddr(struct _exec_env *pEnv);
void RiverMemReset(struct _exec_env *pEnv);

unsigned int GetCurrentReg(struct _exec_env *pEnv, unsigned char regName);
unsigned int GetPrevReg(struct _exec_env *pEnv, unsigned char regName);
unsigned int NextReg(struct _exec_env *pEnv, unsigned char regName);
void ResetRegs(struct _exec_env *pEnv);

#endif
