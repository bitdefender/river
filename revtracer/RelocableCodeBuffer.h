#ifndef _RELOCABLE_CODE_BUFFER_H
#define _RELOCABLE_CODE_BUFFER_H

#include "revtracer.h"
#include "mm.h"

class RelocableCodeBuffer {
private :
	rev::BYTE *buffer;
	bool needsRAFix;
	rev::BYTE *rvAddress;
public :
	rev::BYTE *cursor;

	RelocableCodeBuffer();
	void Init(rev::BYTE *buff);

	void Reset();
	void SetRelocation(rev::BYTE *reloc);
	void CopyToFixed(rev::BYTE *dst) const;
};

#endif
