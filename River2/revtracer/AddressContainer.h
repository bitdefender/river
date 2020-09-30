#ifndef _ADDRESS_CONTAINER_H
#define _ADDRESS_CONTAINER_H

#include "revtracer.h"

#define MAX_CONTAINER_PAGES			1024

struct ContainerPage {
	nodep::DWORD mem[1024];
};

class AddressContainer {
private :
	ContainerPage pages[MAX_CONTAINER_PAGES];
	ContainerPage *freePages[MAX_CONTAINER_PAGES];
	nodep::DWORD freePageCount;
	
	void InitPageAllocator();
	ContainerPage *AllocPage();
	void FreePage(ContainerPage *page);

	ContainerPage *root;

	nodep::DWORD RecursiveSet(ContainerPage *&page, nodep::DWORD dwAddress, nodep::DWORD value, nodep::DWORD mask, nodep::DWORD shift);
	void RecursivePrintAddreses(ContainerPage *page, nodep::DWORD prefix, nodep::DWORD shift) const;
public :
	//AddressContainer();
	void Init();

	nodep::DWORD Set(nodep::DWORD dwAddress, nodep::DWORD value);
	nodep::DWORD Get(nodep::DWORD dwAddress) const;

	void PrintAddreses() const;
};


#endif