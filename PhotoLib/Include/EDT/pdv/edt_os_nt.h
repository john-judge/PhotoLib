/*
 * Header file for NT specific includes
 * for libedt
 *
 * jsc 2/4/99
 *
 */

#ifndef _EDT_OS_NT_H
#define _EDT_OS_NT_H

#include <stdio.h>
#include <io.h>

#ifndef _KERNEL
/* not for driver */

#include <winsock2.h>
#include <winioctl.h>

#include <process.h>
#include <time.h>

// ifndef _KERNEL
#endif

// for gcc

#ifndef _MSC_VER

#include <winnt.h>

#endif

#ifdef _MSC_VER
typedef unsigned __int64 uint64_t;
typedef __int64 int64_t;
typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned short uint16_t;
typedef short int16_t;

#endif

typedef unsigned char *caddr_t;
typedef unsigned char uchar_t;
typedef unsigned short ushort_t;
typedef unsigned int uint_t;

typedef unsigned long u_long;

#define NULL_HANDLE INVALID_HANDLE_VALUE
#define thread_t HANDLE

#define DIRHANDLE HANDLE

#define EDTAPI __declspec(dllexport)

#ifdef LIBDLL

#define EDTIMPORT __declspec(dllimport)

#else

#define EDTIMPORT

#endif

#define strncasecmp strnicmp
#define strcasecmp stricmp

#define strtoull _strtoui64

#define PAGESIZE 4096

#define SECTOR_SIZE 512
/* WIN32 */

/* turn off warnings about styrcpy, etc.  in newer versions of MSVC */

#if _MSC_VER >= 1300

#pragma warning(disable : 4996)

// _MSC_VER >= 1300
#endif

// for using a dll
#ifndef _KERNEL

#define DLLEXPORT __declspec(dllexport)

// for MFC

#ifdef __cplusplus

#if defined(__AFXWIN_H__) || defined(_AFXDLL) || defined(_AFXEXT)

#include <afxwin.h>

#define edt_get_dc(wnd) GetDC(wnd)
#define edt_release_dc(wnd, pdc) ReleaseDC(wnd, pdc)

#define EdtBox CRect

#define EdtWindow HWND
#define EdtDC HDC
#define EdtPoint CPoint

// if AFXWIN
#endif
/* c++ */
#endif
#ifndef ASSERT

#include <assert.h>

#ifdef _DEBUG

#define ASSERT(f) assert(f)

#else

#define ASSERT(f)

// ifdef DEBUG
#endif

// ifndef ASSERT
#endif

#ifdef TRACE
#undef TRACE
// ifdef TRACE
#endif

#ifdef _DEBUG

#define TRACE SysLog.Message

#else

#define TRACE

// ifdef _DEBUG
#endif

typedef __int64 EdtFileOffset;

typedef unsigned char byte;

typedef unsigned int uint;

typedef unsigned long ulong;
typedef HANDLE thread_t;

#ifndef LaunchThread

#define LaunchThread(pThread, func, pointer)                                                 \
  {                                                                                          \
    unsigned int thrdaddr;                                                                   \
    pThread = (thread_t)_beginthreadex(NULL, 0, func, pointer, CREATE_SUSPENDED, &thrdaddr); \
    ResumeThread(pThread);                                                                   \
  }

#endif

#define WaitForThread(pThread, timeout) WaitForSingleObject(pThread, timeout)

typedef unsigned int(__stdcall *edt_thread_function)(void *);

typedef thread_t thread_type;

#define THREAD_RETURN UINT
#define THREAD_FUNC_DECLARE UINT __stdcall

#ifdef _MSC_VER

typedef CRITICAL_SECTION critical_section_t;

#define init_critical(cp) InitializeCriticalSection(&cp);
#define enter_critical(cp) EnterCriticalSection(&cp);
#define leave_critical(cp) LeaveCriticalSection(&cp);
#define delete_critical(cp) DeleteCriticalSection(&cp);

#endif

typedef HANDLE mutex_t;

#define create_mutex_named(m, nm) m = CreateMutex(NULL, FALSE, nm)
#define create_mutex(m) m = CreateMutex(NULL, FALSE, NULL)

#define wait_mutex_timeout(m, timeout) WaitForSingleObject(m, timeout)
#define wait_mutex(m) WaitForSingleObject(m, INFINITE)
#define release_mutex(m) ReleaseMutex(m)
#define delete_mutex(m) CloseHandle(m)

typedef HANDLE event_t;

#define create_event_named(m, manual, init_state, nm) m = CreateEvent(NULL, manual, init_state, nm)

#define create_event(m, manual, init_state) m = CreateEvent(NULL, manual, init_state, NULL)

#define wait_event_timeout(m, timeout) WaitForSingleObject(m, timeout)
#define wait_event(m) WaitForSingleObject(m, INFINITE)

#define signal_event(e) SetEvent(e)
#define clear_event(e) ResetEvent(e)

#endif

// ifndef _EDT_OS_NT_H_
#endif
