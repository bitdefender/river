#ifndef _RELOCABLE_CODE_BUFFER_H
#define _RELOCABLE_CODE_BUFFER_H

#include "revtracer.h"
#include "mm.h"

class RelocableCodeBuffer {
private :
	nodep::BYTE *buffer;
	bool needsRAFix;
	nodep::BYTE *rvAddress;
public :
	nodep::BYTE *cursor;

	RelocableCodeBuffer();
	void Init(nodep::BYTE *buff);

	void Reset();
	void SetRelocation(nodep::BYTE *reloc);
	void CopyToFixed(nodep::BYTE *dst) const;
};

#endif
