#ifndef _LARGE_STACK_H_
#define _LARGE_STACK_H_

#ifdef _WIN32
#include <Windows.h>
#endif

#include "../CommonCrossPlatform/Common.h"

namespace stk {
	typedef unsigned int DWORD;

	class LargeStack {
	private:
		DWORD *stackTop;
		DWORD *stackBase;
		DWORD stackSize;
		DWORD chunkSize;
		DWORD currentRegion;

		DWORD offsets[3];

		FILE_T hVirtualStack;

		DWORD CurrentRegion() const;

		bool VirtualPush(DWORD *buffer);
		bool VirtualPop(DWORD *buffer);
	public:
		LargeStack(DWORD *base, DWORD size, DWORD *top, const char *fName);
		~LargeStack();

		void Push(DWORD value);
		DWORD Pop();

		void Update();
	};
};

#endif // !_LARGE_STACK_H_
