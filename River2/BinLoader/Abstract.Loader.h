#ifndef _ABSTRACT_LDR_H_
#define _ABSTRACT_LDR_H_

#include "Abstract.Mapper.h"
#include "Abstract.Importer.h"

#include <functional>

namespace ldr {
	class AbstractBinary {
	public:
		virtual bool IsValid() const = 0;
		virtual bool Map(AbstractMapper &mapr, AbstractImporter &impr, DWORD &baseAddr, bool callConstructors) = 0;
		virtual bool GetExport(const char *funcName, DWORD &funcRVA) const = 0;
		virtual DWORD GetRequiredSize() const = 0;

		virtual void ForAllExports(std::function<void(const char * /*funcName*/, const DWORD /*ordinal*/, const char * /*version*/, const DWORD /*rva*/, const unsigned char * /*body*/)> verb) const = 0;
	};

	AbstractBinary *LoadBinary(const char *module);
	AbstractBinary *LoadBinary(const wchar_t *module);
}; //namespace ldr

#endif

