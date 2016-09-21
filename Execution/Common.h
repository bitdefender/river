#ifndef _COMMON_EXECUTION_H
#define _COMMON_EXECUTION_H

#ifdef _MSC_VER
#define DEBUG_BREAK __asm \
{ __asm int 3 }
#else
#define DEBUG_BREAK asm volatile("int $0x3")
#endif

#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
typedef int FILE_T;
typedef wchar_t WCHAR;
typedef unsigned long ULONG;
typedef void* HANDLE;
typedef int BOOL;

typedef unsigned short USHORT;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef long LONG;
typedef int64_t LONGLONG;
typedef unsigned long long QWORD;
typedef unsigned int UINT_PTR;
typedef void* LPVOID;
typedef const void* LPCVOID;

typedef union _LARGE_INTEGER {
  struct {
    DWORD LowPart;
    LONG  HighPart;
  };
  struct {
    DWORD LowPart;
    LONG  HighPart;
  } u;
  LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

#define MAX_PATH 260
#define SEEK_BEGIN_FILE SEEK_SET
#define SEEK_END_FILE SEEK_END
#define TRUE 1
#define FALSE 0

#define VSNPRINTF_S(buf, size, count, format, args) vsnprintf(buf, size, format, args)
#define OPEN_FILE_W(filename) open((filename), O_WRONLY | O_CREAT, 0644)
#define OPEN_FILE_RW(filename) open((filename), O_RDWR | O_CREAT, 0644)
#define FAIL_OPEN_FILE(fd) (0 > (fd))
#define WRITE_FILE(fd, buf, size, written, ret) do { written = write((fd), (buf), (size)); ret = (written >= 0); } while (false)
#define READ_FILE(fd, buf, size, b_read, ret) do { b_read = read((fd), (buf), (size)); ret = (b_read >= 0); } while (false)
#define CLOSE_FILE(fd) close((fd))
#define LSEEK(fd, off, flag) lseek((fd), (off.QuadPart), (flag))

#include <pthread.h>
typedef pthread_t THREAD_T;

#include <unistd.h>
#define GET_CURRENT_PROC() getpid()

#else
#include <Windows.h>
typedef HANDLE FILE_T;
typedef void* THREAD_T;

#define SEEK_BEGIN_FILE FILE_BEGIN
#define SEEK_END_FILE FILE_END

#define VSNPRINTF_S(buf, size, count, format, args) vsnprintf_s(buf, size, count, format, args)
#define OPEN_FILE_W(filename) CreateFileA((filename), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr)
#define OPEN_FILE_RW(filename) CreateFileA((filename), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr)
#define FAIL_OPEN_FILE(fd) (INVALID_HANDLE_VALUE == (fd))
#define WRITE_FILE(fd, buf, size, written, ret) do { ret = WriteFile((fd), (buf), (size), &(written), nullptr); } while (false)
#define READ_FILE(fd, buf, size, read, ret) do { ret = ReadFile((fd), (buf), (size), &(read), nullptr); } while (false)
#define CLOSE_FILE(fd) CloseHandle(fd)
#define LSEEK(fd, off, flag) SetFilePointerEx((fd), (off), &(off), (flag))

#define GET_CURRENT_PROC() GetCurrentProcess()

#endif

#endif
