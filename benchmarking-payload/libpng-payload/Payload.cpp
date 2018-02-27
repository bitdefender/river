#include "CommonCrossPlatform/Common.h"
#include "CommonCrossPlatform/CommonSpecifiers.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>


#define PNG_INTERNAL
#include "png.h"

struct BufState {
  const uint8_t* data;
  size_t bytes_left;
};

struct PngObjectHandler {
  png_infop info_ptr = nullptr;
  png_structp png_ptr = nullptr;
  png_voidp row_ptr = nullptr;
  BufState* buf_state = nullptr;

  ~PngObjectHandler() {
    if (row_ptr && png_ptr) {
      png_free(png_ptr, row_ptr);
    }
    if (png_ptr && info_ptr) {
      png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    }
    delete buf_state;
  }
};

void user_read_data(png_structp png_ptr, png_bytep data, png_size_t length) {
  BufState* buf_state = static_cast<BufState*>(png_get_io_ptr(png_ptr));
  if (length > buf_state->bytes_left) {
    png_error(png_ptr, "read error");
  }
  memcpy(data, buf_state->data, length);
  buf_state->bytes_left -= length;
  buf_state->data += length;
}

static const int kPngHeaderSize = 8;

// Entry point for LibFuzzer.
// Roughly follows the libpng book example:
// http://www.libpng.org/pub/png/book/chapter13.html

void test_simple(const unsigned char *data) {
  unsigned int size = MAX_PAYLOAD_BUF;
  if (size < kPngHeaderSize) {
    return;
  }

  if (png_sig_cmp(data, 0, kPngHeaderSize)) {
    // not a PNG.
    return;
  }

  PngObjectHandler png_handler;
  png_handler.png_ptr = png_create_read_struct
    (PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!png_handler.png_ptr) {
    return;
  }

  png_set_user_limits(png_handler.png_ptr, 2048, 2048);

  png_set_crc_action(png_handler.png_ptr, PNG_CRC_QUIET_USE, PNG_CRC_QUIET_USE);

  png_handler.info_ptr = png_create_info_struct(png_handler.png_ptr);
  if (!png_handler.info_ptr) {
    return;
  }

  // Setting up reading from buffer.
  png_handler.buf_state = new BufState();
  png_handler.buf_state->data = data + kPngHeaderSize;
  png_handler.buf_state->bytes_left = size - kPngHeaderSize;
  png_set_read_fn(png_handler.png_ptr, png_handler.buf_state, user_read_data);
  png_set_sig_bytes(png_handler.png_ptr, kPngHeaderSize);

  // libpng error handling.
  if (setjmp(png_jmpbuf(png_handler.png_ptr))) {
    return;
  }

  // Reading.
  png_read_info(png_handler.png_ptr, png_handler.info_ptr);
  png_handler.row_ptr = png_malloc(
      png_handler.png_ptr, png_get_rowbytes(png_handler.png_ptr,
                                               png_handler.info_ptr));

  // reset error handler to put png_deleter into scope.
  if (setjmp(png_jmpbuf(png_handler.png_ptr))) {
    return;
  }

  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type, compression_type;
  int filter_type;

  if (!png_get_IHDR(png_handler.png_ptr, png_handler.info_ptr, &width,
                    &height, &bit_depth, &color_type, &interlace_type,
                    &compression_type, &filter_type)) {
    return;
  }

  // This is going to be too slow.
  if (width && height > 100000000 / width)
    return;

  if (width > 2048 || height > 2048)
    return;

  int passes = png_set_interlace_handling(png_handler.png_ptr);
  png_start_read_image(png_handler.png_ptr);

  for (int pass = 0; pass < passes; ++pass) {
    for (png_uint_32 y = 0; y < height; ++y) {
      png_read_row(png_handler.png_ptr,
                   static_cast<png_bytep>(png_handler.row_ptr), NULL);
    }
  }

  return;
}

extern "C" {
	DLL_PUBLIC unsigned char payloadBuffer[MAX_PAYLOAD_BUF];
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
