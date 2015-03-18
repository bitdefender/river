#ifndef _VIRTUAL_REG_H
#define _VIRTUAL_REG_H

#include <basetsd.h>

#define R_AX 0x00
#define R_CX 0x01
#define R_DX 0x02
#define R_BX 0x03
#define R_SP 0x04
#define R_BP 0x05
#define R_SI 0x06
#define R_DI 0x07

#define R_LAST_REG 0x08

struct _virtual_reg {
	UINT_PTR regs[R_LAST_REG];
};

#endif
