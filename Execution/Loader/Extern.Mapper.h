#ifndef _EXTERN_MAPPER_H_
#define _EXTERN_MAPPER_H_

#include "Abstract.Mapper.h"

class ExternMapper : public AbstractPEMapper {
private :
	DWORD dwHandle;
public:
	ExternMapper(DWORD hnd);

	virtual void *CreateSection(void *lpAddress, size_t dwSize, DWORD flProtect);
	virtual bool ChangeProtect(void *lpAddress, size_t dwSize, DWORD flProtect);
	virtual bool WriteBytes(void *lpAddress, void *lpBuffer, size_t nSize);

	virtual DWORD FindImport(const char *moduleName, const char *funcName);
	virtual DWORD FindImport(const char *moduleName, const unsigned int funcOrdinal);
};

#endif