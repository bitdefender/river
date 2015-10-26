#ifndef _ADDRESS_CONTAINER_H
#define _ADDRESS_CONTAINER_H

#include "revtracer.h"

using namespace rev;

#define MAX_CONTAINER_PAGES			1024

struct ContainerPage {
	DWORD mem[1024];
};

class AddressContainer {
private :
	ContainerPage pages[MAX_CONTAINER_PAGES];
	ContainerPage *freePages[MAX_CONTAINER_PAGES];
	DWORD freePageCount;
	
	void InitPageAllocator();
	ContainerPage *AllocPage();
	void FreePage(ContainerPage *page);

	ContainerPage *root;

	DWORD RecursiveSet(ContainerPage *&page, DWORD dwAddress, DWORD value, DWORD mask, DWORD shift);
	void RecursivePrintAddreses(ContainerPage *page, DWORD prefix, DWORD shift) const;
public :
	//AddressContainer();
	void Init();

	DWORD Set(DWORD dwAddress, DWORD value);
	DWORD Get(DWORD dwAddress) const;

	void PrintAddreses() const;
};


#endif