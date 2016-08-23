#include "mm.h"

//#include "lup.h" //for now
//volatile DWORD dwMMLock			= 0;

#include "common.h"
#include "revtracer.h"

extern "C" void rev_memcpy(void *dest, const void *src, unsigned int size) {
	for (unsigned int i = 0; i < size; ++i) {
		((char *)dest)[i] = ((char *)src)[i];
	}
}

extern "C" void rev_memset(void *dest, int val, unsigned int size) {
	for (unsigned int i = 0; i < size; ++i) {
		((char *)dest)[i] = val;
	}
}

struct HeapZone {
	HeapZone *Prev; // 0xFFFFFFFF if this is the first block
	HeapZone *Next; // 0xFFFFFFFF if this is the last block
	rev::DWORD Size; // size of this block
	rev::DWORD Type; // 0 - free, 1 - allocated
};

RiverHeap::RiverHeap() {
	pHeap = NULL;
	pFirstFree = NULL;
	size = 0;
}

RiverHeap::~RiverHeap() {
	if (0 != size) {
		Destroy();
	}
}

bool RiverHeap::Init(rev::DWORD heapSize) {
	HeapZone *fz;
	unsigned char *tHeap;

	tHeap = pHeap = (unsigned char *)rev::revtracerAPI.memoryAllocFunc(heapSize);

	if (!tHeap) {
		return false;
	}
	
	rev_memset(tHeap, 0, heapSize);

	fz = (HeapZone *)tHeap; 

	fz->Next = (HeapZone *) 0xFFFFFFFF;
	fz->Prev = (HeapZone *) 0xFFFFFFFF;
	fz->Type = 0;
	fz->Size = size - sizeof(HeapZone);

	pFirstFree = fz;

	size = heapSize;
	return true;
}

bool RiverHeap::Destroy() {
	if (pHeap) {
		rev::revtracerAPI.memoryFreeFunc(pHeap);
		pHeap = NULL;
		pFirstFree = NULL;
		size = 0;
	}

	return true;
}


void RiverHeap::PrintInfo(HeapZone *fz) {
	rev::revtracerAPI.dbgPrintFunc(PRINT_INSPECTION | PRINT_DEBUG, "FirstFree: %08X.\n", (rev::DWORD)pFirstFree);
	rev::revtracerAPI.dbgPrintFunc(PRINT_INSPECTION | PRINT_DEBUG, "fz  Addr : %08X.\n", (rev::DWORD)fz);
	rev::revtracerAPI.dbgPrintFunc(PRINT_INSPECTION | PRINT_DEBUG, "fz->Next : %08X.\n", (rev::DWORD)fz->Next);
	rev::revtracerAPI.dbgPrintFunc(PRINT_INSPECTION | PRINT_DEBUG, "fz->Prev : %08X.\n", (rev::DWORD)fz->Prev);
	rev::revtracerAPI.dbgPrintFunc(PRINT_INSPECTION | PRINT_DEBUG, "fz->Type : %08X.\n", (rev::DWORD)fz->Type);
	rev::revtracerAPI.dbgPrintFunc(PRINT_INSPECTION | PRINT_DEBUG, "fz->Size : %08X.\n", (rev::DWORD)fz->Size);
	rev::revtracerAPI.dbgPrintFunc(PRINT_INSPECTION | PRINT_DEBUG, "\n");
}


void *RiverHeap::Alloc(rev::DWORD sz) {
	rev::BYTE *b;
	rev::DWORD first;
	HeapZone *fz, *nfz;

	sz += 3;
	sz &= ~3L;

//	SC_Lock (&dwMMLock);

	first = 1;

	fz = pFirstFree;

	do{
	//	SC_PrintInfo (fz);

		if (fz->Type == 0) {// free block
			if (sz + sizeof (HeapZone) <= fz->Size) {
			//	printf("Found a block of %d bytes. We need only %d.\n", fz->Size, sz);

				b = (rev::BYTE *) fz + sizeof (HeapZone);

				nfz = (HeapZone *) ((rev::BYTE *) fz + sizeof (HeapZone) + sz);

				if (fz->Next != (HeapZone *) 0xFFFFFFFF)
				{
					fz->Next->Prev = nfz;
				}

				nfz->Next = fz->Next;
				nfz->Prev = fz;
				nfz->Type = 0;
				nfz->Size = fz->Size - sz - sizeof (HeapZone);

				fz->Next = nfz;
				fz->Type = 1;
				fz->Size = sz;

				if (first) {
					pFirstFree = nfz;
				}

			//	SC_Unlock (&dwMMLock);
				
				return b;
			} else {
			//	printf("Free block, but only %d in size!\n", fz->Size);
				first = 0;
			}
		}

		fz = (HeapZone *) fz->Next;

	} while (fz != (HeapZone *) 0xFFFFFFFF);

//	SC_Unlock (&dwMMLock);

	return NULL;
}

void RiverHeap::List() {
	rev::DWORD dwMaxSize;
	HeapZone *fz;

	fz = (HeapZone *)pHeap;

//	SC_Lock (&dwMMLock);

	dwMaxSize = 0;

	do{

	//	SC_PrintInfo (fz);

		if (fz->Type == 1)
		{		
		//	printf("fz->addr : %08X, fz->size : %08X.\n", fz, fz->Size);
			dwMaxSize += fz->Size;
		}

		fz = (HeapZone *) fz->Next;

	} while (fz != (HeapZone *) 0xFFFFFFFF);

//	printf("%d bytes of memory are in use.\n", dwMaxSize);

//	SC_Unlock (&dwMMLock);
}

void RiverHeap::Free(void *p) {
	HeapZone *fz, *wfz;

//	SC_Lock (&dwMMLock);

	fz = (HeapZone *) ((rev::BYTE *)p - sizeof (HeapZone));

//	SC_PrintInfo (fz);

	fz->Type = 0;

	wfz = fz->Next;

	if (wfz != (HeapZone *) 0xFFFFFFFF) {// present?
		if (wfz->Type == 0) {// free?
			fz->Next = wfz->Next;
			fz->Size = fz->Size + wfz->Size + sizeof (HeapZone);
			fz->Type = 0;
		}
	}

	if (fz < pFirstFree) {
		pFirstFree = fz;
	}

	wfz = fz->Prev;

	if (wfz != (HeapZone *) 0xFFFFFFFF) {// present?
		if (wfz->Type == 0) {// free?
			wfz->Next = fz->Next;
			wfz->Size = wfz->Size + fz->Size + sizeof (HeapZone);

			if (wfz < pFirstFree) {
				pFirstFree = wfz;
			}
		}
	}

//	SC_Unlock (&dwMMLock);
}

