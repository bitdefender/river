#include "Common.h"

#include <turbojpeg.h>
#include <stdint.h>
#include <stdlib.h>

#include <memory>



void test_simple(const char *data) {
	tjhandle jpegDecompressor = tjInitDecompress();

	int width, height, subsamp, colorspace;
	int res = tjDecompressHeader3(
        jpegDecompressor, (unsigned char *)data, 4096,
		&width, &height, &subsamp, &colorspace);

    // Bail out if decompressing the headers failed, the width or height is 0,
    // or the image is too large (avoids slowing down too much). Cast to size_t to
    // avoid overflows on the multiplication
    if (res != 0 || width == 0 || height == 0 || ((size_t)width * height > (1024 * 1024))) {
        tjDestroy(jpegDecompressor);
        return;
    }

    std::unique_ptr<unsigned char[]> buf(new unsigned char[width * height * 3]);
    tjDecompress2(
        jpegDecompressor, (unsigned char *)data, 4096,
		buf.get(), width, 0, height, TJPF_RGB, 0);

    tjDestroy(jpegDecompressor);
}

extern "C" {
	DLL_PUBLIC char payloadBuffer[4096];
	DLL_PUBLIC int Payload() {
		test_simple(payloadBuffer);
		return 0;
	}
};

#ifdef _WIN32
#include <Windows.h>
BOOL WINAPI DllMain(
		_In_ HINSTANCE hinstDLL,
		_In_ DWORD     fdwReason,
		_In_ LPVOID    lpvReserved
		) {
	return TRUE;
}
#endif
