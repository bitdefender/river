#include "VirtualMem.h"

#include "MemoryLayout.h"

namespace vmem {
	nodep::BYTE *GetFreeRegion(process_t hProcess, nodep::DWORD size, nodep::DWORD granularity) {
		size = (size + 0xFFF) & ~0xFFF;

		nodep::DWORD dwOffset = 0x01000000;
		nodep::DWORD dwCandidate = 0, dwCandidateSize = 0xFFFFFFFF;

		MemoryLayout *layout = CreateMemoryLayout(hProcess);
		if (!layout->Snapshot()) {
			return nullptr;
		}

		while (dwOffset < 0x2FFF0000) {
			MemoryRegionInfo mri;
			nodep::DWORD regionSize = 0xFFFFFFFF;
			bool regionFree = true;

			if (!layout->Query((void *)dwOffset, mri)) {
				layout->Release();
				delete layout;
				return nullptr;
			}

			nodep::DWORD dwSize = mri.size - (dwOffset - (nodep::DWORD)mri.baseAddress); // or allocationbase
			if (regionSize > dwSize) {
				regionSize = dwSize;
			}

			regionFree &= (MEMORY_REGION_FREE == mri.state);

			if (regionFree & (regionSize >= size) & (regionSize < dwCandidateSize)) {
				dwCandidate = dwOffset;
				dwCandidateSize = regionSize;

				if (regionSize == size) {
					break;
				}
			}

			dwOffset += regionSize;
			dwOffset += granularity - 1;
			dwOffset &= ~(granularity - 1);
		}


		layout->Release();
		delete layout;

		if (0 == dwCandidate) {
			return nullptr;
		}

		return (nodep::BYTE *)dwCandidate;
	}


	nodep::BYTE *GetFreeRegion(process_t hProcess1, process_t hProcess2, nodep::DWORD size, nodep::DWORD granularity) {

		size = (size + 0xFFF) & ~0xFFF;
		// now look for a suitable address;

		nodep::DWORD dwOffset = 0x01000000; // dwGran;
		nodep::DWORD dwCandidate = 0, dwCandidateSize = 0xFFFFFFFF;

		//HANDLE hProcess[2] = { hProcess1, hProcess2 };
		MemoryLayout *layout[2];
		layout[0] = CreateMemoryLayout(hProcess1);
		layout[1] = CreateMemoryLayout(hProcess2);

		for (int i = 0; i < 2; ++i) {
			layout[i]->Snapshot();
			//layout[i]->Debug();
		}

		while (dwOffset < 0x2FFF0000) {
			MemoryRegionInfo mri;
			nodep::DWORD regionSize = 0xFFFFFFFF;
			bool regionFree = true;

			for (int i = 0; i < 2; ++i) {
				if (!layout[i]->Query((void *)dwOffset, mri)) {
					for (int i = 0; i < 2; ++i) {
						layout[i]->Release();
						delete layout[i];
					}
					return nullptr;
				}

				nodep::DWORD dwSize = mri.size - (dwOffset - (nodep::DWORD)mri.baseAddress); // or allocationbase
				if (regionSize > dwSize) {
					regionSize = dwSize;
				}

				//printf("        Proc %d offset: 0x%08x, size 0x%08x\n", i, dwOffset, dwSize);

				regionFree &= (MEMORY_REGION_FREE == mri.state);
			}

			if (regionFree & (regionSize >= size) & (regionSize < dwCandidateSize)) {
				dwCandidate = dwOffset;
				dwCandidateSize = regionSize;

				if (regionSize == size) {
					break;
				}
			}

			dwOffset += regionSize;
			dwOffset += granularity - 1;
			dwOffset &= ~(granularity - 1);
		}

		for (int i = 0; i < 2; ++i) {
			layout[i]->Release();
			delete layout[i];
		}

		if (0 == dwCandidate) {
			return nullptr;
		}

		return (nodep::BYTE *)dwCandidate;
	}
};