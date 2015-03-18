#ifndef _EXTERN_H
#define _EXTERN_H

void DbgPrint(const char *fmt, ...);

void *EnvMemoryAlloc(unsigned long dwSize);
void EnvMemoryFree(unsigned char *b);

#endif