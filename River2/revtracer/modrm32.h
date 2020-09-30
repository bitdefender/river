#ifndef _MODRM32_H
#define _MODRM32_H

#define FLAG_NONE	0x00000000
#define FLAG_REP	0x00000001
#define FLAG_SEG	0x00000002
#define FLAG_O16	0x00000004
#define FLAG_A16	0x00000008
#define FLAG_LOCK	0x00000010
#define FLAG_EXT	0x00000020

/* river flag to mark a disabled instruction */
#define FLAG_RNONE  0x10000000
/* river flag to mark a custom opcode */
#define FLAG_RIVER  0x20000000
#define FLAG_PFX	0x40000000
#define FLAG_BRANCH	0x80000000

nodep::DWORD GetModrmSize (nodep::DWORD, nodep::BYTE *pI);

#endif //  _MODRM32_H

