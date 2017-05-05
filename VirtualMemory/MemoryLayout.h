#ifndef _MEMORY_LAYOUT_H_

#include "VirtualMem.h"

namespace vmem {

#define MEMORY_REGION_FREE			0x00
#define MEMORY_REGION_RESERVED		0x01
#define MEMORY_REGION_COMMITED		0x02
#define MEMORY_REGION_UNKNOWN		0xFF

#define MEMORY_REGION_READ			0x4
#define MEMORY_REGION_WRITE			0x2
#define MEMORY_REGION_EXECUTE		0x1

	struct MemoryRegionInfo {
		void *baseAddress;
		void *allocationBase;

		unsigned int size;

		// free, reserved, protected
		unsigned int state;

		// read, write, execute
		unsigned int protection;

		char *moduleName;
	};

	class MemoryLayout {
	public:
		virtual bool Snapshot() = 0;
		virtual bool Query(void *addr, MemoryRegionInfo &out) = 0;
		virtual bool Release() = 0;

		virtual bool Debug() = 0;
	};

	MemoryLayout *CreateMemoryLayout(process_t process);

};

#endif // !_MEMORY_LAYOUT_H_