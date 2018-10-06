#ifdef __linux__

#include "../libproc/os-linux.h"

#include "MemoryLayout.h"

#include <vector>

#include <stdio.h>

namespace vmem {
	class LinMemoryLayout : public MemoryLayout {
	private:
		process_t pid;
		std::vector<MemoryRegionInfo> regions;
	public :
		LinMemoryLayout(process_t p) {
			pid = p;
		}

		virtual bool Snapshot() {
			MemoryRegionInfo mTmp;
			struct map_iterator mi;
			if (maps_init(&mi, pid) < 0) {
				//dbg_log("[DualAllocator] Cannot retrieve /proc/%d/maps\n", pid);
				return false;
			}

			struct map_prot mp;
			unsigned long low, high, offset;

			//dbg_log("[DualAllocator] /proc/%d/maps\n", pid);
			while (maps_next(&mi, &low, &high, &offset, &mp)) {
				//dbg_log("[DualAllocator] path : %s base addr : %08lx high %08lx\n", mi.path, low, high);
				ssize_t len = regions.size();
				if (len > 1) {
					// add free region before current region
					mTmp.allocationBase = (void *)((unsigned char *)regions[len - 1].allocationBase + regions[len - 1].size);
					mTmp.size = low - (unsigned long)mTmp.allocationBase;

				} else {
					mTmp.allocationBase = (void *)0x0;
					mTmp.size = low;
				}

				if (mTmp.size > 0x00) {
					//mr.state = FREE;
					//mrs.push_back(mr);

					mTmp.baseAddress = mTmp.allocationBase;
					mTmp.state = MEMORY_REGION_FREE;
					mTmp.protection = 0;
					mTmp.moduleName = nullptr; // unsuported

					regions.push_back(mTmp);
					//dbg_log("[DualAllocator] Added FREE %08x %08x\n", mr.address, mr.size);
				}

				// add current allocated region
				mTmp.baseAddress = (void *)low;
				mTmp.allocationBase = (void *)low;
				mTmp.size = high - low;
				mTmp.state = MEMORY_REGION_COMMITED;
				mTmp.protection = 0; // unsupported
				mTmp.moduleName = nullptr; //unsupported

				regions.push_back(mTmp);
				//dbg_log("[DualAllocator] Added ALLOCATED %08x %08x\n", mr.address, mr.size);


			}
			maps_close(&mi);
		}

		virtual bool Query(void *addr, MemoryRegionInfo &out) {
			for (auto it = regions.begin(); it != regions.end(); ++it) {
				if (addr < it->allocationBase)
					continue;
				if (addr >= (unsigned char *)it->allocationBase + it->size)
					continue;

				out.baseAddress = addr;
				out.allocationBase = it->allocationBase;
				out.size = it->size - ((unsigned long)addr - (unsigned long)it->allocationBase);
				out.state = it->state;
				out.protection = it->protection;
				out.moduleName = it->moduleName;
				return true;
			}

			//shouldn't reach this point
			//printf("[DualAllocator] Could not find memory region for address %08x\n", address);
			return false;
		}

		virtual bool Debug() {
			for (auto it = regions.begin(); it != regions.end(); ++it) {
				printf("%p %08x %x %x\n",
					it->allocationBase,
					it->size,
					it->state,
					it->protection
				);
			}
		}

		virtual bool Release() {
			regions.clear();
			return true;
		}
	};


	MemoryLayout *CreateMemoryLayout(process_t process) {
		return new LinMemoryLayout(process);
	}
};


#endif