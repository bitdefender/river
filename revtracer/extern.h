#ifndef _EXTERN_H
#define _EXTERN_H

/* Define NULL pointer value */
#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else  /* __cplusplus */
#define NULL    ((void *)0)
#endif  /* __cplusplus */
#endif  /* NULL */

typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;

void DbgPrint(const char *fmt, ...);

void *EnvMemoryAlloc(unsigned long dwSize);
void EnvMemoryFree(void *b);

#endif