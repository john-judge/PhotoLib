// EdtImgMinMax.h: interface for the CEdtImgMinMax class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EDTIMGMINMAX_H__B7DF20F7_1B2B_11D2_8DE4_00A0C932D513__INCLUDED_)
#define AFX_EDTIMGMINMAX_H__B7DF20F7_1B2B_11D2_8DE4_00A0C932D513__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "edtimage/EdtImage.h"

class EdtImgMinMax  
{
public:
    EdtImgMinMax();
    virtual ~EdtImgMinMax();

    int Compute(EdtImage *pImage,int &nMin, int &nMax, int nBand = 0);

};

#endif // !defined(AFX_EDTIMGMINMAX_H__B7DF20F7_1B2B_11D2_8DE4_00A0C932D513__INCLUDED_)
