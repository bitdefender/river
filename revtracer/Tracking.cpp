#include "Tracking.h"
#include "common.h"

#define PRINT_RUNTIME_TRACKING	PRINT_INFO | PRINT_RUNTIME | PRINT_TRACKING

namespace rev {
#ifndef _NO_TRACK_CALLBACKS_
	nodep::DWORD __stdcall TrackAddr(void *pEnv, nodep::DWORD dwAddr, nodep::DWORD segSel) {
		nodep::DWORD ret = ((::ExecutionEnvironment *)pEnv)->ac.Get(dwAddr + revtracerConfig.segmentOffsets[segSel & 0xFFFF]);
		
		TRACKING_PRINT(PRINT_RUNTIME_TRACKING, "TrackAddr 0x%08x => %d\n", dwAddr + revtracerConfig.segmentOffsets[segSel & 0xFFFF], ret);

		if (0 != ret) {
			revtracerImports.trackCallback(ret, dwAddr, segSel);
		}

		return ret;
	}

	nodep::DWORD __stdcall MarkAddr(void *pEnv, nodep::DWORD dwAddr, nodep::DWORD value, nodep::DWORD segSel) {
		TRACKING_PRINT(PRINT_RUNTIME_TRACKING, "MarkAddr 0x%08x <= %d\n", dwAddr + revtracerConfig.segmentOffsets[segSel & 0xFFFF], value);
		nodep::DWORD ret = ((::ExecutionEnvironment *)pEnv)->ac.Set(dwAddr + revtracerConfig.segmentOffsets[segSel & 0xFFFF], value);

		if (0 != ret) {
			revtracerImports.markCallback(ret, value, dwAddr, segSel);
		}

		return ret;
	}
#else

	nodep::DWORD __stdcall TrackAddr(void *pEnv, nodep::DWORD dwAddr, nodep::DWORD segSel) {
		return ((::ExecutionEnvironment *)pEnv)->ac.Get(dwAddr);
	}

	nodep::DWORD __stdcall MarkAddr(void *pEnv, nodep::DWORD dwAddr, nodep::DWORD value, nodep::DWORD segSel) {
		return ((::ExecutionEnvironment *)pEnv)->ac.Set(dwAddr, value);
	}

#endif
};