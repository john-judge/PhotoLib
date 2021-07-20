//
// edtdefines.h
//////////////////////////////////////////////////

#if !defined(EDT_DEFINES_H)
#define EDT_DEFINES_H



#include <string.h>
#include <stdlib.h>


///  OS-Specific defines

#ifdef WIN32
// for using a dll

#define DLLEXPORT __declspec(dllexport)



#if defined( __AFXWIN_H__) || defined(_AFXDLL) || defined(_AFXEXT) 


#include <afxwin.h>

#define edt_get_dc(wnd) GetDC(wnd)
#define edt_release_dc(wnd, pdc) ReleaseDC(wnd, pdc)


#define EdtBox CRect

#define EdtWindow HWND
#define EdtDC HDC
#define EdtPoint CPoint


#endif

#ifndef ASSERT

#include <assert.h>

#ifdef _DEBUG

#define ASSERT(f) assert(f)

#else

#define ASSERT(f) 

#endif

#endif




#ifdef TRACE
#undef TRACE
#endif
#include "dispatch/ErrorHandler.h"
#ifdef _DEBUG



#define TRACE SysLog.Message

#else

#define TRACE

#endif


typedef __int64 EdtFileOffset;

#define THREAD_RETURN UINT

typedef unsigned char byte;

typedef unsigned int uint;

typedef unsigned long ulong;


#else


#include <stdlib.h>

#include <pthread.h>
#include <sys/user.h>

#ifdef __sun

#define mutable 

#include <sys/param.h>
#include <limits.h>

#define PAGE_SIZE PAGESIZE

#else

#include <limits.h>
#endif


#define WINAPI

#define ASSERT(f) ((void)0)

/* utility helpers */

#ifndef TRUE

#define TRUE (1)
#define FALSE (0)

#endif

#ifndef __cplusplus


typedef unsigned char bool;

#endif

// typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned int UINT;
typedef unsigned long ULONG;

typedef off_t EdtFileOffset;

#define INFINITE 0

#define SECTOR_SIZE PAGE_SIZE

#define _edtmax _POSIX_PATH_MAX


#define THREAD_RETURN void *


#ifdef _DEBUG

#define TRACE printf

#else

#define TRACE

#endif

#endif

#if !defined( __AFXWIN_H__) && !defined(_AFXDLL) && !defined(_AFXEXT) 


struct EdtPoint {
    int x;
    int y;
};

struct EdtRect {
    int x;
    int y;
    int width;
    int height;
};

struct EdtLine {
    EdtPoint pt1;
    EdtPoint pt2;
};

#endif

#ifndef edtmax

#define edtmax(a,b) ((a > b) ? (a):(b))

#endif

#include "edtinc.h"
#include "edt_types.h"

#endif
