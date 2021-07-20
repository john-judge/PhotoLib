/** ********************************
 * 
 * @file edtdefines.h
 *
 * Copyright (c) Engineering Design Team 2009
 * All Rights Reserved
 */


//
// edtdefines.h
//////////////////////////////////////////////////


#if !defined(EDT_DEFINES_H)
#define EDT_DEFINES_H



#include <string.h>

#include <stdlib.h>

///  OS-Specific defines


#include "edt_utils.h"

#include "edt_types.h"
//#include "edt_threads.h"


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

// Same as CRect in windows

struct EdtBox {
    int left;
    int top;
    int right;
    int bottom;
};

struct EdtLine {
    EdtPoint pt1;
    EdtPoint pt2;
};

#define EdtDC HANDLE

#define EdtWindow HANDLE

#define edt_get_dc(wnd) (0)

#define edt_release_dc(wnd,dc) (0)

#endif



#endif
