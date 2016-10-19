#ifndef _COMMON_UTIL_H
#define _COMMON_UTIL_H

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
#include <sys/mman.h>
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
#define FOPEN(res, path, mode) ({ res = fopen((path), (mode)); })
#define SPRINTF(buffer, format, ...) sprintf((buffer), (format), ##__VA_ARGS__)

#define MAP_FILE_RWX(file, size)                                   \
  ({ int fd = open((file), O_RDWR | O_CREAT | O_TRUNC, 0644);                                \
   void *ret = mmap(0, (size), PROT_READ | PROT_WRITE | PROT_EXEC, \
       MAP_SHARED, fd, 0);                                         \
   ret;                                                            \
   })

#include <pthread.h>
typedef pthread_t THREAD_T;
struct event_t {
  pthread_cond_t cond;
  pthread_mutex_t mutex;
  int exited;
};

typedef struct event_t EVENT_T;
#define CREATE_EVENT(event) \
  ({ pthread_cond_init(&((event).cond), nullptr); \
   pthread_mutex_init(&(event).mutex, nullptr); \
   (event).exited = false; \
   })

#define SIGNAL_EVENT(event) \
  ({ pthread_mutex_lock(&(event).mutex); \
   (event).exited = true; \
   pthread_cond_signal(&(event).cond); \
   pthread_mutex_unlock(&(event).mutex); \
   })

#define WAIT_FOR_SINGLE_OBJECT(event) \
  ({ pthread_mutex_lock(&(event).mutex); \
   while (!(event).exited) { \
   pthread_cond_wait(&(event).cond, &(event).mutex); \
   } \
   pthread_mutex_unlock(&(event).mutex); \
   })

#include <unistd.h>
#define GET_CURRENT_PROC() getpid()

// manual dynamic loading
#define GET_LIB_HANDLER(libname) dlopen((libname), RTLD_LAZY)
#define CLOSE_LIB(libhandler) dlclose((libhandler))
#define LOAD_PROC(libhandler, szProc) dlsym((libhandler), (szProc))


#else
#include <Windows.h>
typedef HANDLE FILE_T;
typedef void* THREAD_T;
typedef void* EVENT_T;

#define CREATE_EVENT(handle) do { handle = CreateEvent(nullptr, false, false, nullptr); } while (false)
#define SIGNAL_EVENT(handle) SetEvent((handle))
#define WAIT_FOR_SINGLE_OBJECT(handle) WaitForSingleObject((handle), INFINITE)

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
#define FOPEN(res, path, mode) fopen_s(&(res), (path), (mode))
#define SPRINTF(buffer, format, ...) sprintf_s((buffer), (format), ##__VA_ARGS__)

#define MAP_FILE_RWX(file, size) CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_EXECUTE_READWRITE, 0, (size), (file))

#define GET_CURRENT_PROC() GetCurrentProcess()

// manual dynamic loading
#define GET_LIB_HANDLER(libname) GetModuleHandleW((libname))
#define CLOSE_LIB
#define LOAD_PROC(libhandler, szProc) GetProcAddress((libhandler), (szProc))

#endif

#endif
