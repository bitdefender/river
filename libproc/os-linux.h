/* libunwind - a platform-independent unwind library
   Copyright (C) 2003-2004 Hewlett-Packard Co
   Copyright (C) 2007 David Mosberger-Tang
	Contributed by David Mosberger-Tang <dmosberger@gmail.com>

This file is part of libunwind.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

#ifndef os_linux_h
#define os_linux_h

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>

struct map_iterator
{
	off_t offset;
	int fd;
	size_t buf_size;
	char *buf;
	char *buf_end;
	char *path;
};

struct map_prot
{
	uint32_t prot;
	uint32_t flags;
};

#define FREE 0
#define ALLOCATED 1

struct map_region
{
	unsigned long address;
	unsigned long size;
	uint32_t state;
};

extern int maps_init(struct map_iterator *mi, pid_t pid);

extern int maps_next (struct map_iterator *mi,
		unsigned long *low, unsigned long *high, unsigned long *offset,
		struct map_prot *mp);

extern void maps_close (struct map_iterator *mi);

extern long get_rss();
#endif /* os_linux_h */
