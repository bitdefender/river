#ifndef _COMMON_IPC_H
#define _COMMON_IPC_H

#ifdef _MSC_VER
#define DEBUG_BREAK __asm \
{ __asm int 3 }
#else
#define DEBUG_BREAK asm volatile("int $0x3")
typedef unsigned int size_t;
#endif

#endif
