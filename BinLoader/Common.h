#ifndef _COMMON_EXECUTION_LOADER_H
#define _COMMON_EXECUTION_LOADER_H

#ifdef _MSC_VER
#define DEBUG_BREAK __asm \
{ __asm int 3 }
#else
#define DEBUG_BREAK asm volatile("int $0x3")
#endif

#ifdef __linux__
#include <stdio.h>
#define MAX_PATH_NAME 4096
int wchar_to_utf8(const wchar_t *src, char *dst, ssize_t dst_len);
void *w_dlopen(const wchar_t *filename, int flags);
FILE *w_fopen(const wchar_t *path, const wchar_t *mode);

#define FOPEN(res, path, mode) ( res = fopen((path), (mode)), res == nullptr)
#define W_FOPEN(res, path, mode) ({ res = w_fopen((path), (mode)); nullptr == res; })

#else
#define MAX_PATH_NAME 260
#define FOPEN(res, path, mode) fopen_s(&(res), (path), (mode))
#define W_FOPEN(res, path, mode) _wfopen_s(&(res), (path), (mode))
#endif

void solve_path(const char *moduleName, char *path);
void solve_path(const wchar_t *moduleName, wchar_t *path);
#endif
