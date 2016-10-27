#ifndef _ABSTRACT_IMPORTER_H_
#define _ABSTRACT_IMPORTER_H_

#include "Types.h"

namespace ldr {
#define IMPORT_NOT_FOUND 0xFFFFFFFF

	class AbstractImporter {
	public:
		virtual DWORD FindImport(const char *moduleName, const char *funcName) = 0;
		virtual DWORD FindImport(const char *moduleName, const unsigned int funcOrdinal) = 0;
		virtual DWORD FindImport(const char *moduleName, const char *funcName, const char *version) = 0;
	};
};

#endif

