
//layout

// [Y][C][B][A][X]
// A, B, C large regions
// X, Y, small overflow detection regions
// Layout: Divide the available buffer into 8 chunks
//         Y|C|C|B|B|A|A|X



// in order to keep things consistent
// When Top in Y:
// 		Move contents of Y in A
//		Move top in A
// When Top in X:
//		Move top in C

// When Top in A:
// 		B must be clear
//      C must be prev -> clone C to X
// When Top in B:
// 		A must be prev
// 		C must be clear
// When Top in C:
//      A must be clear
//      B must be prev

#include "LargeStack.h"

#define LOG_MIN_CHUNK_SIZE 12
#define MIN_CHUNK_SIZE (1 << LOG_MIN_CHUNK_SIZE)
#define CHUNK_COUNT    0x8
#define MIN_STACK_SIZE (CHUNK_COUNT << LOG_MIN_CHUNK_SIZE)

namespace stk {

	LargeStack::LargeStack(DWORD *base, DWORD size, DWORD *top, char *fName) {
		if (size & (MIN_STACK_SIZE - 1)) {
			__asm int 3;
		}

		stackBase = base;
		stackSize = size;
		stackTop = top;

		chunkSize = stackSize / CHUNK_COUNT;

		*stackTop = (DWORD)stackBase + 7 * chunkSize;
		currentRegion = CurrentRegion();

		offsets[0] = 2;
		offsets[1] = 1;
		offsets[2] = 0;

		hVirtualStack = CreateFileA(
			fName,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		if (INVALID_HANDLE_VALUE == hVirtualStack) {
			__asm int 3;
		}
	}

	LargeStack::~LargeStack() {
		CloseHandle(hVirtualStack);
	}

	DWORD LargeStack::CurrentRegion() const {
		DWORD ttop = *stackTop - (DWORD)stackBase - 4;

		ttop /= chunkSize;

		return (ttop + 1) >> 1;
	}

	bool LargeStack::VirtualPush(DWORD *buffer) {
		::DWORD dwWr;
		
		return TRUE == WriteFile(hVirtualStack, buffer, chunkSize << 1, &dwWr, NULL);
	}

	bool LargeStack::VirtualPop(DWORD *buffer) {
		LARGE_INTEGER liPos;
		::DWORD dwRd;

		liPos.QuadPart = ~(LONGLONG)(chunkSize << 1) + 1;

		SetFilePointerEx(hVirtualStack, liPos, &liPos, FILE_END);

		ReadFile(hVirtualStack, buffer, chunkSize << 1, &dwRd, NULL);

		SetFilePointerEx(hVirtualStack, liPos, &liPos, FILE_BEGIN);

		SetEndOfFile(hVirtualStack);

		return true;
	}

	void LargeStack::Update() {
		DWORD newRegion = CurrentRegion();
		DWORD copySize;

		if (newRegion != currentRegion) {
			DWORD *clearBuffer = NULL, *loadBuffer = NULL;

			switch (newRegion) {
				case 0: // move to 3
					copySize = (DWORD)stackBase + (chunkSize << 1) - *stackTop;
					memcpy((void *)(*stackTop + 6 * chunkSize), (void *)*stackTop, copySize);
					*stackTop += 6 * chunkSize;
					newRegion = 3;
					break;
				case 4: // move to 1
					*stackTop -= 6 * chunkSize;
					newRegion = 1;
					break;
			}

			switch (newRegion) {
				case 1:
					if (2 == currentRegion) {
						clearBuffer = (DWORD *)((DWORD)stackBase + 5 * chunkSize);
					} else { // 3
						loadBuffer = (DWORD *)((DWORD)stackBase + 3 * chunkSize);
					}
					break;
				case 2:
					if (3 == currentRegion) {
						clearBuffer = (DWORD *)((DWORD)stackBase + 1 * chunkSize);
					} else { // 1
						loadBuffer = (DWORD *)((DWORD)stackBase + 5 * chunkSize);
					}
					break;
				case 3:
					if (1 == currentRegion) {
						clearBuffer = (DWORD *)((DWORD)stackBase + 3 * chunkSize);
					} else { // 2
						loadBuffer = (DWORD *)((DWORD)stackBase + 1 * chunkSize);
					}
					break;
			}

			if (NULL != clearBuffer) {
				VirtualPush(clearBuffer);
				//optionally
				memset(clearBuffer, 0, chunkSize << 1);
			}

			if (NULL != loadBuffer) {
				VirtualPop(loadBuffer);
			}

			currentRegion = newRegion;
		}
	}

	void LargeStack::Push(DWORD value) {
		*stackTop -= 4;
		*stackTop = value;
	}

	DWORD LargeStack::Pop()	{
		DWORD ret = *stackTop;
		*stackTop += 4;
		return ret;
	}

};