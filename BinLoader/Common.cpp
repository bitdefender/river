#include "Common.h"

#ifdef __linux__
#include <stdlib.h>
#include <wchar.h>
#include <string.h>
#include <iconv.h>
#include <dlfcn.h>

int wchar_to_utf8(const wchar_t *src, char *dst, ssize_t dst_len) {
	int len = wcslen(src);
	if (dst_len < len)
		return -1;
	memset(dst, 0, len + 1);

	char *iconv_in = (char *)src;
	char *iconv_out = (char *)dst;

	size_t iconv_in_bytes = (len + 1) * sizeof(wchar_t);
	size_t iconv_out_bytes = dst_len;

	iconv_t cd = iconv_open("UTF-8", "WCHAR_T");
	if ((iconv_t)-1 == cd)
		return -1;

	size_t ret = iconv(cd, &iconv_in, &iconv_in_bytes,
		&iconv_out, &iconv_out_bytes);
	if ((size_t)-1 == ret)
		return -1;

	return 0;
}

void *w_dlopen(const wchar_t *filename, int flags) {
	char path_utf8[wcslen(filename) + 1];
	if (wchar_to_utf8((filename), path_utf8, wcslen(filename) + 1) < 0)
		return nullptr;
	return dlopen(path_utf8, flags);
}

FILE *w_fopen(const wchar_t *path, const wchar_t *mode) {
	char path_utf8[wcslen(path) + 1];
	char  mode_utf8[wcslen(mode) + 1];

	if (wchar_to_utf8((path), path_utf8, wcslen(path) + 1) < 0)
		return nullptr;
	if (wchar_to_utf8((mode), mode_utf8, wcslen(mode) + 1) < 0)
		return nullptr;

	return fopen(path_utf8, mode_utf8);
}
#endif


#ifdef __linux__
#include <stdlib.h> //getenv
#include <dirent.h>

bool find_module(const char *moduleName, char *dirname, char *path) {

	DIR *d;
	struct dirent *dir;
	d = opendir(dirname);
	if (d)
	{
		while ((dir = readdir(d)) != NULL)
		{
			if (!strcmp(moduleName, dir->d_name)) {
				strcpy(path, dirname);
				path[strlen(path)] = '/';
				strcat(path, moduleName);

				closedir(d);
				return true;
			}
		}
		closedir(d);
	}

	return false;
}
#endif

bool find_in_env(const wchar_t *moduleName, char *path) {
	char utf8ModuleName[MAX_PATH_NAME];
	wchar_to_utf8(moduleName, utf8ModuleName, MAX_PATH_NAME);
	return find_in_env(utf8ModuleName, path);
}

bool find_in_env(const char *moduleName, char *path) {
	memset(path, 0, MAX_PATH_NAME);
#ifdef __linux__
	const char* env = getenv("LD_LIBRARY_PATH");
	if (!env)
		return false;

	// iterate thourgh it
	const char *it = env;
	const char *start = it;
	while (1) {
		if ((*it == ':') || (*it == '\0')) {
			//process from start to it - 1
			ssize_t len = it - 1 - start + 1;
			if (!len)
				continue;
			char dirname[MAX_PATH_NAME];
			memset(dirname, 0, MAX_PATH_NAME);
			strncpy(dirname, start, len);
			if (find_module(moduleName, dirname, path)) {
				return true;
			}
			if (!*it)
				break;
			start = it + 1;
		}
		it++;
	}

	return false;

#else
    strcpy_s(path, strlen(moduleName), moduleName);
    return true;
#endif
}
