#ifndef _ABSTRACT_MAPPER_H_
#define _ABSTRACT_MAPPER_H_

#include "Types.h"

class AbstractPEMapper {
public:
	virtual void *CreateSection(void *lpAddress, size_t dwSize, DWORD flProtect) = 0;
	virtual bool ChangeProtect(void *lpAddress, size_t dwSize, DWORD flProtect) = 0;
	virtual bool WriteBytes(void *lpAddress, void *lpBuffer, size_t nSize) = 0;

	virtual DWORD FindImport(const char *moduleName, const char *funcName) = 0;
	virtual DWORD FindImport(const char *moduleName, const unsigned int funcOrdinal) = 0;
};

#endif

