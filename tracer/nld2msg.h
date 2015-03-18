

#ifndef _NLD2MSG_H
#define _NLD2MSG_H

#include <windows.h>

#include "mm.h"
#include "tls.h"

// #define _DEBUG 1

#ifdef _DEBUG
#define msg printf
#else
#define msg
#endif


#define FLAG_NONE	0x00000000
#define FLAG_REP	0x00000001
#define FLAG_SEG	0x00000002
#define FLAG_O16	0x00000004
#define FLAG_A16	0x00000008
#define FLAG_LOCK	0x00000010
#define FLAG_EXT	0x00000020
#define FLAG_PFX	0x40000000
#define FLAG_BRANCH	0x80000000

#define FLAG_VOLATILE   0x564F4C41

/*
	8 dwords = 32 bytes
*/

#pragma pack(1)

/*struct _cb_info
{
	DWORD 			address;
	DWORD			dwSize;
	DWORD			dwCRC;
	DWORD			dwParses;
	BYTE			*pCode;
	struct _cb_info		*pNext;
};*/


#define CB_FLAG_SYSOUT	0x80000000


typedef DWORD (__cdecl *_scfn) (struct _cb_info *, DWORD *, BYTE *, BYTE *, DWORD *); 

int Translate (struct _cb_info *pCB, DWORD dwParent, DWORD dwFlags);

struct _msg_conn
{
	unsigned long free;	// 0x00
	unsigned long full;	// 0x04
	unsigned long args;	// 0x08
	unsigned long data[1];	// 0x0C
};

/*
0012FEF0  30 00 32 00 00 FC FD 7F  0.2..Åy
*/

struct _unicode_string
{
	unsigned short len;
	unsigned short maxlen;
	wchar_t *name;
};

/*
0012FED8  18 00 00 00 14 00 00 00  ......
0012FEE0  F0 FE 12 00 00 00 00 00  d_.....
0012FEE8  00 00 00 00 00 00 00 00  ........
*/

struct _object_attributes
{
	unsigned long len;		// 0x00 ; 0x18 ; sizeof(struct)
	unsigned long root;		// 0x04 ; 0x14 ; ??? ; will be set to 0
	struct _unicode_string *name; 	// 0x08 ; 
	unsigned long attr;		// 0x0C ; 0x00
	unsigned long security;		// 0x10 ; 0x00
	unsigned long qos;		// 0x14 ; 0x00
};





DWORD WINAPI ZwClose 			(HANDLE);
DWORD WINAPI ZwOpenSection 		(HANDLE *, DWORD, struct _object_attributes *);
DWORD WINAPI ZwOpenDirectoryObject	(HANDLE *, DWORD, struct _object_attributes *);
DWORD WINAPI ZwMapViewOfSection		(HANDLE, HANDLE, DWORD *, DWORD, DWORD, LARGE_INTEGER *, DWORD *, DWORD, DWORD, DWORD);
DWORD WINAPI ZwAllocateVirtualMemory	(HANDLE, DWORD *, DWORD, DWORD *, DWORD, DWORD);
DWORD WINAPI ZwFreeVirtualMemory	(HANDLE, DWORD *, DWORD *, DWORD);
DWORD WINAPI ZwProtectVirtualMemory 	(HANDLE, DWORD *, DWORD *, DWORD, DWORD *);

DWORD WINAPI ZwDelayExecution 		(DWORD, LARGE_INTEGER *);
DWORD WINAPI ZwYieldExecution 		(void);

DWORD WINAPI ZwSetEvent 		(HANDLE, DWORD *);
DWORD WINAPI ZwWaitForSingleObject	(HANDLE *, DWORD, LARGE_INTEGER *);
DWORD WINAPI ZwOpenEvent 		(HANDLE *, DWORD, struct _object_attributes *);

DWORD __inline MyInterlockedCompareExchange (volatile DWORD *p, DWORD v, DWORD c);
void *WINAPI KiUserExceptionDispatcher (DWORD, DWORD);

void *MyMemoryAlloc (DWORD);

#endif // NLD2MSG_H

