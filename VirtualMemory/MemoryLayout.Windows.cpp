#if defined _WIN32 || defined __CYGWIN__


#include <Windows.h>
#include <Psapi.h>

#include "MemoryLayout.h"

namespace vmem {

	class WinMemoryLayout : public MemoryLayout {
	private :
		process_t process;
		char moduleName[MAX_PATH];

		static unsigned int MemoryState(DWORD state) {
			switch (state) {
				case MEM_COMMIT :
					return MEMORY_REGION_COMMITED;
				case MEM_RESERVE :
					return MEMORY_REGION_RESERVED;
				case MEM_FREE :
					return MEMORY_REGION_FREE;
				default :
					return MEMORY_REGION_UNKNOWN;
			}
		}

		static unsigned int MemoryProtect(DWORD protect) {
			switch (protect) {
				case PAGE_EXECUTE :
					return MEMORY_REGION_EXECUTE;
				case PAGE_EXECUTE_READ :
					return MEMORY_REGION_EXECUTE | MEMORY_REGION_READ;
				case PAGE_EXECUTE_READWRITE :
					return MEMORY_REGION_EXECUTE | MEMORY_REGION_READ | MEMORY_REGION_WRITE;
				case PAGE_NOACCESS :
					return 0;
				case PAGE_READONLY :
					return MEMORY_REGION_READ;
				case PAGE_READWRITE :
					return MEMORY_REGION_READ | MEMORY_REGION_WRITE;
				default :
					return 0;
			}
		}
	public :
		WinMemoryLayout(process_t p) {
			process = p;
		}

		virtual bool Snapshot() {
			return true;
		}

		virtual bool Query(void *addr, MemoryRegionInfo &out) {
			MEMORY_BASIC_INFORMATION32 mbi;

			if (0 == VirtualQueryEx(process, addr, (PMEMORY_BASIC_INFORMATION)&mbi, sizeof(mbi))) {
				return false;
			}

			out.baseAddress = (void *)mbi.BaseAddress;
			out.allocationBase = (void *)mbi.AllocationBase;
			out.size = mbi.RegionSize;
			out.state = MemoryState(mbi.State);
			out.protection = MemoryProtect(mbi.Protect);
			out.moduleName = moduleName;

			moduleName[0] = '\0';
			if (MEM_IMAGE == mbi.Type) {
				GetModuleFileNameExA(
					process,
					(HMODULE)addr,
					moduleName,
					sizeof(moduleName)
				);
			}
			return true;
		}

		virtual bool Debug() {
			return true;
		}

		virtual bool Release() {
			return true;
		}
	};


	MemoryLayout *CreateMemoryLayout(process_t process) {
		return new WinMemoryLayout(process);
	}
};


#endif