// EdtHistogram.h: interface for the EdtHistogram class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EDTHISTOGRAM_H__B7DF20F5_1B2B_11D2_8DE4_00A0C932D513__INCLUDED_)
#define AFX_EDTHISTOGRAM_H__B7DF20F5_1B2B_11D2_8DE4_00A0C932D513__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "edtimage/EdtImage.h"
#include "EdtImgStats.h"


class EdtHistogram 
{

protected:

    int *m_pHist[EDT_MAX_COLOR_PLANES];

    int m_BitCounts[32];
    int m_nBits;

    int m_nInputType;
    int m_nInputMin;
    int m_nInputMax;

    int m_nBins;

    int m_nBinSize;
    int m_nColors;

public:
    int GetModeValue();
    int m_nSubSample;
    unsigned int m_nSum;
    void MakeCumulative();
    void Clear();
    void Destroy();
    void ComputeStatistics(EdtImgStats *pStats);

    void CountBits();
    int *GetBitCounts() {return m_BitCounts;}
    int  GetNBits() {return m_nBits;}

    void Setup(EdtImage *pImage);
    EdtHistogram();
    virtual ~EdtHistogram();

    int *GetHistogram(int nBand = 0) 
    {
	return m_pHist[nBand];
    }

    int ** GetHistogramArray()
    {
	return m_pHist;
    }

    int GetNColors() const {return m_nColors;}

    void Compute(EdtImage *pImage, bool bClear = true);

    int GetNBins() {return m_nBins;}

    int GetInputMax() {return m_nInputMax;}

};

#endif // !defined(AFX_EDTHISTOGRAM_H__B7DF20F5_1B2B_11D2_8DE4_00A0C932D513__INCLUDED_)
