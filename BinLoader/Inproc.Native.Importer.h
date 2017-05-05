#ifndef _INPROC_NATIVE_IMPORTER_H_
#define _INPROC_NATIVE_IMPORTER_H_

#include "Abstract.Importer.h"
#include <vector>
#include <string>

namespace ldr {
	class InprocNativeImporter : public AbstractImporter {

	private:
		struct CachedEntry {
			std::string funcName;
			unsigned int ordinal;
			std::string version;
			DWORD rva;
		};

		struct CachedModule {
			std::string moduleName;
			std::vector<CachedEntry> entries;

			void AddImport(const char *funcName, const unsigned int ordinal, const char *version, DWORD rva);

			DWORD FindCachedImport(const char *funcName);
			DWORD FindCachedImport(const unsigned int funcOrdinal);
			DWORD FindCachedImport(const char *funcName, const char *version);
		};
		std::vector<CachedModule> modules;


		CachedModule *AddModule(const char *name);
		CachedModule *FindModule(const char *moduleName);

		CachedModule *GetModule(const char *name);

	public:
		virtual DWORD FindImport(const char *moduleName, const char *funcName);
		virtual DWORD FindImport(const char *moduleName, const unsigned int funcOrdinal);

		virtual DWORD FindImport(const char *moduleName, const char *funcName, const char *version);
	};
};

#endif
