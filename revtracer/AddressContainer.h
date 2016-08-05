#ifndef _ADDRESS_CONTAINER_H
#define _ADDRESS_CONTAINER_H

#include "revtracer.h"

#define MAX_CONTAINER_PAGES			1024

struct ContainerPage {
	rev::DWORD mem[1024];
};

class AddressContainer {
private :
	ContainerPage pages[MAX_CONTAINER_PAGES];
	ContainerPage *freePages[MAX_CONTAINER_PAGES];
	rev::DWORD freePageCount;
	
	void InitPageAllocator();
	ContainerPage *AllocPage();
	void FreePage(ContainerPage *page);

	ContainerPage *root;

	rev::DWORD RecursiveSet(ContainerPage *&page, rev::DWORD dwAddress, rev::DWORD value, rev::DWORD mask, rev::DWORD shift);
	void RecursivePrintAddreses(ContainerPage *page, rev::DWORD prefix, rev::DWORD shift) const;
public :
	//AddressContainer();
	void Init();

	rev::DWORD Set(rev::DWORD dwAddress, rev::DWORD value);
	rev::DWORD Get(rev::DWORD dwAddress) const;

	void PrintAddreses() const;
};


#endif