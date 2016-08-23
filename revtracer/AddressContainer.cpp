#include "revtracer.h"
#include "common.h"
#include "AddressContainer.h"

static const rev::DWORD printMask = PRINT_INFO | PRINT_INSPECTION;

using namespace rev;

void AddressContainer::InitPageAllocator() {
	freePageCount = MAX_CONTAINER_PAGES;

	for (DWORD i = 0; i < freePageCount; ++i) {
		freePages[i] = &pages[i];
	}
}

ContainerPage *AddressContainer::AllocPage() {
	if (0 == freePageCount) {
		return NULL;
	}

	freePageCount--;

	for (int i = 0; i < 1024; ++i) {
		freePages[freePageCount]->mem[i] = 0;
	}

	return freePages[freePageCount];
}

void AddressContainer::FreePage(ContainerPage *page) {
	freePages[freePageCount] = page;
	freePageCount++;
}

void AddressContainer::Init() {
	root = NULL;

	InitPageAllocator();
}

DWORD AddressContainer::RecursiveSet(ContainerPage *&page, DWORD dwAddress, DWORD value, DWORD mask, DWORD shift) {
	if (NULL == page) {
		if (0 == value) {
			return 0;
		}

		page = AllocPage();

		if (NULL == page) {
			return 0xFFFFFFFF;
		}
	}

	if (0 == shift) {
		DWORD dwRet = page->mem[(dwAddress & mask) >> shift];
		page->mem[(dwAddress & mask) >> shift] = value;
		return dwRet;
	}

	return RecursiveSet((ContainerPage *&)page->mem[(dwAddress & mask) >> shift], dwAddress, value, mask >> 10, shift - 10);
}

DWORD AddressContainer::Set(DWORD dwAddress, DWORD value) {
	ContainerPage *cPage = root;

	DWORD ret = 0;
	DWORD aMask = 0x3FF00000;
	BYTE aShift = 20;

	dwAddress >>= 2;

	return RecursiveSet(root, dwAddress, value, aMask, aShift);
}

DWORD AddressContainer::Get(DWORD dwAddress) const {
	ContainerPage *cPage = root;
	DWORD ret = 0;
	DWORD aMask = 0x3FF00000;
	BYTE aShift = 20;

	dwAddress >>= 2;

	while (NULL != cPage) {
		cPage = (ContainerPage *)cPage->mem[(dwAddress & aMask) >> aShift];

		aMask >>= 10;
		aShift -= 10;

		if (0 == aMask) {
			ret = (DWORD)cPage;
			break;
		}
	}

	return ret;
}

void AddressContainer::RecursivePrintAddreses(ContainerPage *page, DWORD prefix, DWORD shift) const {
	if (NULL == page) {
		return;
	}

	if (0 == shift) {
		for (DWORD i = 0; i < 1024; ++i) {
			if (0 != page->mem[i]) {
				TRACKING_PRINT(printMask, " @ 0x%08x - %d\n", (prefix | i) << 2, page->mem[i]);
			}
		}
	} else {
		for (DWORD i = 0; i < 1024; ++i) {
			RecursivePrintAddreses((ContainerPage *)page->mem[i], prefix | (i << shift), shift - 10);
		}
	}
}

void AddressContainer::PrintAddreses() const {
	RecursivePrintAddreses(root, 0, 20);
}
