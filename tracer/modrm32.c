
#include "execenv.h"
#include "modrm32.h"


BYTE Modrm16 [0x100] = 
			{
				01, 01, 01, 01, 01, 01, 03, 01, 
				01, 01, 01, 01, 01, 01, 03, 01, 
				01, 01, 01, 01, 01, 01, 03, 01, 
				01, 01, 01, 01, 01, 01, 03, 01, 
				01, 01, 01, 01, 01, 01, 03, 01, 
				01, 01, 01, 01, 01, 01, 03, 01, 
				01, 01, 01, 01, 01, 01, 03, 01, 
				01, 01, 01, 01, 01, 01, 03, 01, 

				02, 02, 02, 02, 02, 02, 02, 02, 
				02, 02, 02, 02, 02, 02, 02, 02, 
				02, 02, 02, 02, 02, 02, 02, 02, 
				02, 02, 02, 02, 02, 02, 02, 02, 
				02, 02, 02, 02, 02, 02, 02, 02, 
				02, 02, 02, 02, 02, 02, 02, 02, 
				02, 02, 02, 02, 02, 02, 02, 02, 
				02, 02, 02, 02, 02, 02, 02, 02, 

				03, 03, 03, 03, 03, 03, 03, 03, 
				03, 03, 03, 03, 03, 03, 03, 03, 
				03, 03, 03, 03, 03, 03, 03, 03, 
				03, 03, 03, 03, 03, 03, 03, 03, 
				03, 03, 03, 03, 03, 03, 03, 03, 
				03, 03, 03, 03, 03, 03, 03, 03, 
				03, 03, 03, 03, 03, 03, 03, 03, 
				03, 03, 03, 03, 03, 03, 03, 03,
 
				01, 01, 01, 01, 01, 01, 01, 01, 
				01, 01, 01, 01, 01, 01, 01, 01, 
				01, 01, 01, 01, 01, 01, 01, 01, 
				01, 01, 01, 01, 01, 01, 01, 01, 
				01, 01, 01, 01, 01, 01, 01, 01,
				01, 01, 01, 01, 01, 01, 01, 01, 
				01, 01, 01, 01, 01, 01, 01, 01,
				01, 01, 01, 01, 01, 01, 01, 01
			};


DWORD GetModrmSize (DWORD dwFlags, BYTE *pI)
{
	DWORD dwExtra;
	BYTE bModRM, bRM, bMod, bSIB;

	if (dwFlags & FLAG_A16)
	{
		return (DWORD) (Modrm16 [pI[1]] - 1);
	}

	bModRM 		= * (pI + 1);

	bRM    		= bModRM & 0x07;
	bMod   		= bModRM >> 6;

	dwExtra = 0;

	switch (bMod)
	{
		case 0x00:
		{
			switch (bRM)
			{
				case 0x00:
				case 0x01:
				case 0x02:
				case 0x03:
				{
					break;
				}
				case 0x04:
				{
					dwExtra = 1;
					bSIB = * (pI + 2);
					if ((bSIB & 0x07) == 0x05) // EBP
					{
						dwExtra += 4;
					}
					break;
				}
				case 0x05:
				{
					// imm32
					dwExtra = 4;
					break;
				}
				case 0x06:
				case 0x07:
				default:
				{
					break;
				}
			}
			break;
		}

		case 0x01:
		{
			switch (bRM)
			{
				case 0x04:
				{
					dwExtra = 2;
					break;
				}
				case 0x00:
				case 0x01:
				case 0x02:
				case 0x03:
				case 0x05:
				case 0x06:
				case 0x07:
				default:
				{
					dwExtra = 1;
					break;
				}
			}
			break;
		}

		case 0x02:
		{
			switch (bRM)
			{
				case 0x04:
				{
					dwExtra = 5;
					break;
				}
				case 0x00:
				case 0x01:
				case 0x02:
				case 0x03:
				case 0x05:
				case 0x06:
				case 0x07:
				default:
				{
					dwExtra = 4;
					break;
				}
			}
			break;
		}
		default : break;
	}

	return dwExtra;
}
