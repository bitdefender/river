#include "mm.h"


//#include "lup.h" //for now
//volatile DWORD dwMMLock			= 0;

#include "common.h"
#include "extern.h"

void memcpy(void *dest, const void *src, unsigned int size) {
	for (unsigned int i = 0; i < size; ++i) {
		((char *)dest)[i] = ((char *)src)[i];
	}
}

void memset(void *dest, int val, unsigned int size) {
	for (unsigned int i = 0; i < size; ++i) {
		((char *)dest)[i] = val;
	}
}

struct _zone {
	struct _zone *Prev; // 0xFFFFFFFF if this is the first block
	struct _zone *Next; // 0xFFFFFFFF if this is the last block
	DWORD Size; // size of this block
	DWORD Type; // 0 - free, 1 - allocated
};

int SC_HeapInit(struct _exec_env *pEnv, unsigned int heapSize) {
	struct _zone *fz;
	unsigned char *pHeap;

	pHeap = pEnv->pHeap = EnvMemoryAlloc(heapSize);

	if (!pHeap) {
		return 0;
	}
	
	memset (pHeap, 0, heapSize);

	fz = (struct _zone *) pHeap; 

	fz->Next = (struct _zone *) 0xFFFFFFFF;
	fz->Prev = (struct _zone *) 0xFFFFFFFF;
	fz->Type = 0;
	fz->Size = pEnv->heapSize - sizeof(struct _zone);

	pEnv->pFirstFree = fz;

	pEnv->heapSize = heapSize;
	return 1;
}

int SC_HeapDestroy(struct _exec_env *pEnv) {
	if (pEnv->pHeap) {
		EnvMemoryFree(pEnv->pHeap);
		pEnv->pHeap = NULL;
		pEnv->pFirstFree = NULL;
	}

	return 1;
}


void SC_PrintInfo (struct _exec_env *pEnv, struct _zone *fz) {
	DbgPrint("FirstFree: %08X.\n", (DWORD) pEnv->pFirstFree);
	DbgPrint("fz  Addr : %08X.\n", (DWORD)fz);
	DbgPrint("fz->Next : %08X.\n", (DWORD)fz->Next);
	DbgPrint("fz->Prev : %08X.\n", (DWORD)fz->Prev);
	DbgPrint("fz->Type : %08X.\n", (DWORD)fz->Type);
	DbgPrint("fz->Size : %08X.\n", (DWORD)fz->Size);
	DbgPrint("\n");
}


BYTE *SC_HeapAlloc(struct _exec_env *pEnv, DWORD sz) {
	BYTE *b;
	DWORD first;
	struct _zone *fz, *nfz;

	sz += 3;
	sz &= ~3L;

//	SC_Lock (&dwMMLock);

	first = 1;

	fz = pEnv->pFirstFree;

	do{
	//	SC_PrintInfo (fz);

		if (fz->Type == 0) {// free block
			if (sz + sizeof (struct _zone) <= fz->Size) {
			//	printf("Found a block of %d bytes. We need only %d.\n", fz->Size, sz);

				b = (BYTE *) fz + sizeof (struct _zone);

				nfz = (struct _zone *) ((BYTE *) fz + sizeof (struct _zone) + sz);

				if (fz->Next != (struct _zone *) 0xFFFFFFFF)
				{
					fz->Next->Prev = nfz;
				}

				nfz->Next = fz->Next;
				nfz->Prev = fz;
				nfz->Type = 0;
				nfz->Size = fz->Size - sz - sizeof (struct _zone);

				fz->Next = nfz;
				fz->Type = 1;
				fz->Size = sz;

				if (first) {
					pEnv->pFirstFree = nfz;
				}

			//	SC_Unlock (&dwMMLock);
				
				return b;
			} else {
			//	printf("Free block, but only %d in size!\n", fz->Size);
				first = 0;
			}
		}

		fz = (struct _zone *) fz->Next;

	} while (fz != (struct _zone *) 0xFFFFFFFF);

//	SC_Unlock (&dwMMLock);

	return NULL;
}

int SC_HeapList(struct _exec_env *pEnv) {
	DWORD dwMaxSize;
	struct _zone *fz;

	fz = (struct _zone *)pEnv->pHeap;

//	SC_Lock (&dwMMLock);

	dwMaxSize = 0;

	do{

	//	SC_PrintInfo (fz);

		if (fz->Type == 1)
		{		
		//	printf("fz->addr : %08X, fz->size : %08X.\n", fz, fz->Size);
			dwMaxSize += fz->Size;
		}

		fz = (struct _zone *) fz->Next;

	} while (fz != (struct _zone *) 0xFFFFFFFF);

//	printf("%d bytes of memory are in use.\n", dwMaxSize);

//	SC_Unlock (&dwMMLock);

	return 0;
}

int SC_HeapFree(struct _exec_env *pEnv, BYTE *p) {
	struct _zone *fz, *wfz;

//	SC_Lock (&dwMMLock);

	fz = (struct _zone *) (p - sizeof (struct _zone));

//	SC_PrintInfo (fz);

	fz->Type = 0;

	wfz = fz->Next;

	if (wfz != (struct _zone *) 0xFFFFFFFF) {// present?
		if (wfz->Type == 0) {// free?
			fz->Next = wfz->Next;
			fz->Size = fz->Size + wfz->Size + sizeof (struct _zone);
			fz->Type = 0;
		}
	}

	if (fz < pEnv->pFirstFree) {
		pEnv->pFirstFree = fz;
	}

	wfz = fz->Prev;

	if (wfz != (struct _zone *) 0xFFFFFFFF) {// present?
		if (wfz->Type == 0) {// free?
			wfz->Next = fz->Next;
			wfz->Size = wfz->Size + fz->Size + sizeof (struct _zone);

			if (wfz < pEnv->pFirstFree) {
				pEnv->pFirstFree = wfz;
			}
		}
	}

//	SC_Unlock (&dwMMLock);

	return 1;
}

