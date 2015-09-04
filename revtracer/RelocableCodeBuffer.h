#ifndef _RELOCABLE_CODE_BUFFER_H
#define _RELOCABLE_CODE_BUFFER_H

#include "extern.h"
#include "mm.h"

class RelocableCodeBuffer {
private :
	BYTE *buffer;
	bool needsRAFix;
	BYTE *rvAddress;
public :
	BYTE *cursor;

	RelocableCodeBuffer();
	void Init(BYTE *buff);

	void Reset();
	void SetRelocation(BYTE *reloc);
	void CopyToFixed(BYTE *dst) const;
};

#endif
