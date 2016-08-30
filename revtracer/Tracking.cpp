#include "Tracking.h"
#include "common.h"

#define PRINT_RUNTIME_TRACKING	PRINT_INFO | PRINT_RUNTIME | PRINT_TRACKING

namespace rev {
#ifndef _NO_TRACK_CALLBACKS_
	DWORD __stdcall TrackAddr(void *pEnv, DWORD dwAddr, DWORD segSel) {
		DWORD ret = ((::ExecutionEnvironment *)pEnv)->ac.Get(dwAddr + revtracerConfig.segmentOffsets[segSel & 0xFFFF]);
		
		TRACKING_PRINT(PRINT_RUNTIME_TRACKING, "TrackAddr 0x%08x => %d\n", dwAddr + revtracerConfig.segmentOffsets[segSel & 0xFFFF], ret);

		if (0 != ret) {
			revtracerAPI.trackCallback(ret, dwAddr, segSel);
		}

		return ret;
	}

	DWORD __stdcall MarkAddr(void *pEnv, DWORD dwAddr, DWORD value, DWORD segSel) {
		TRACKING_PRINT(PRINT_RUNTIME_TRACKING, "MarkAddr 0x%08x <= %d\n", dwAddr + revtracerConfig.segmentOffsets[segSel & 0xFFFF], value);
		DWORD ret = ((::ExecutionEnvironment *)pEnv)->ac.Set(dwAddr + revtracerConfig.segmentOffsets[segSel & 0xFFFF], value);

		if (0 != ret) {
			revtracerAPI.markCallback(ret, value, dwAddr, segSel);
		}

		return ret;
	}
#else

	DWORD __stdcall TrackAddr(void *pEnv, DWORD dwAddr, DWORD segSel) {
		return ((::ExecutionEnvironment *)pEnv)->ac.Get(dwAddr);
	}

	DWORD __stdcall MarkAddr(void *pEnv, DWORD dwAddr, DWORD value, DWORD segSel) {
		return ((::ExecutionEnvironment *)pEnv)->ac.Set(dwAddr, value);
	}

#endif
};