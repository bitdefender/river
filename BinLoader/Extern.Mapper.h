#ifndef _EXTERN_MAPPER_H_
#define _EXTERN_MAPPER_H_

#include <Windows.h>

#include "Abstract.Mapper.h"

namespace ldr {
	class ExternMapper : public AbstractMapper {
	private:
		HANDLE hProc;
		bool ownProcess;

		HMODULE RemoteFindModule(const char *module);
	public:
		ExternMapper(unsigned int pid);
		ExternMapper(HANDLE process);
		virtual ~ExternMapper();

		virtual void *CreateSection(void *lpAddress, SIZE_T dwSize, DWORD flProtect);
		virtual bool ChangeProtect(void *lpAddress, SIZE_T dwSize, DWORD flProtect);
		virtual bool WriteBytes(void *lpAddress, void *lpBuffer, SIZE_T nSize);

		virtual DWORD FindImport(const char *moduleName, const char *funcName);
		virtual DWORD FindImport(const char *moduleName, const unsigned int funcOrdinal);
	};
};

#endif
