#ifndef _INPROC_NATIVE_IMPORTER_H_
#define _INPROC_NATIVE_IMPORTER_H_

#include "Abstract.Importer.h"

namespace ldr {
	class InprocNativeImporter : public AbstractImporter {
	public:
		virtual DWORD FindImport(const char *moduleName, const char *funcName);
		virtual DWORD FindImport(const char *moduleName, const unsigned int funcOrdinal);

		virtual DWORD FindImport(const char *moduleName, const char *funcName, const char *version);
	};
};

#endif
