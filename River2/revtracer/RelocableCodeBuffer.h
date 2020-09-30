#ifndef _RELOCABLE_CODE_BUFFER_H
#define _RELOCABLE_CODE_BUFFER_H

#include "revtracer.h"
#include "mm.h"

class RelocableCodeBuffer {
private :
	nodep::BYTE *buffer;
	nodep::BYTE *repInitCursor;
	bool needsRAFix, needsRepFix;
	nodep::BYTE *rvAddress;
public :
	nodep::BYTE *cursor;

	RelocableCodeBuffer();
	void Init(nodep::BYTE *buff);

	void Reset();
	void SetRelocation(nodep::BYTE *reloc);
	void CopyToFixed(nodep::BYTE *dst) const;


	/* ->>> loop init
	 * repinit <=> jmp repfini
	 * **actual code**
	 * repfini <=> jmp loop
	 */
	// start counting instructions
	void MarkRepInit();
	// fix jump offsets
	void MarkRepFini();
};

#endif
