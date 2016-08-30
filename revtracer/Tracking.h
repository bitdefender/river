#ifndef _TRACKING_H_
#define _TRACKING_H_

#include "BasicTypes.h"
#include "execenv.h"

namespace rev {
	//TrackingTable<Temp, 128> trItem;

	DWORD __stdcall TrackAddr(void *pEnv, DWORD dwAddr, DWORD segSel);
	DWORD __stdcall MarkAddr(void *pEnv, DWORD dwAddr, DWORD value, DWORD segSel);
		
};

#endif


