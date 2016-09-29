#ifndef _COMMON_EXECUTION_LOADER_H
#define _COMMON_EXECUTION_LOADER_H

#ifdef _MSC_VER
#define DEBUG_BREAK __asm \
{ __asm int 3 }
#else
#define DEBUG_BREAK asm volatile("int $0x3")
#endif

#ifdef __linux__
#include <stdlib.h>
#include <iconv.h>
#include <wchar.h>
#include <string.h>
int wchar_to_utf8(const wchar_t *src, char *dst) {
	int len = wcslen(src);
	if (sizeof(dst) < (len + 1))
		return -1;
	memset(dst, 0, len + 1);

	char *iconv_in = (char *)src;
	char *iconv_out = (char *)dst;

	size_t iconv_in_bytes = (len + 1) * sizeof(wchar_t);
	size_t iconv_out_bytes = sizeof(dst);

	iconv_t cd = iconv_open("UTF-8", "WCHAR_T");
	if ((iconv_t) -1 == cd)
		return -1;

	size_t ret = iconv(cd, &iconv_in, &iconv_in_bytes,
			&iconv_out, &iconv_out_bytes);
	if ((size_t) -1 == ret)
		return -1;

	return 0;
}

FILE *w_fopen(const wchar_t *path, const wchar_t *mode) {
	char path_utf8[wcslen(path) + 1];
	char  mode_utf8[wcslen(mode) + 1];

	if (wchar_to_utf8((path), path_utf8) < 0)
		return nullptr;
	if (wchar_to_utf8((mode), mode_utf8) < 0)
		return nullptr;

	return fopen(path_utf8, mode_utf8);
}

#define FOPEN(res, path, mode) ({ res = fopen((path), (mode)); })
#define W_FOPEN(res, path, mode) ({ res = w_fopen((path), (mode)); nullptr == res; })

#else
#define FOPEN(res, path, mode) fopen_s(&(res), (path), (mode))
#define W_FOPEN(res, path, mode) _wfopen_s(&(res), (path), (mode))
#endif


#endif
