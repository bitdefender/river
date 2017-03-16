#ifndef _TRACKING_H_
#define _TRACKING_H_

#include "../CommonCrossPlatform/BasicTypes.h"
#include "execenv.h"

namespace rev {
	//TrackingTable<Temp, 128> trItem;

	nodep::DWORD __stdcall TrackAddr(void *pEnv, nodep::DWORD dwAddr, nodep::DWORD segSel);
	nodep::DWORD __stdcall MarkAddr(void *pEnv, nodep::DWORD dwAddr, nodep::DWORD value, nodep::DWORD segSel);
		
};

#endif


