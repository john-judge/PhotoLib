// ColorBalance.h: interface for the CColorBalance class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_COLORBALANCE_H__D4F689DE_BF2A_4A02_A810_02F1C7C08D41__INCLUDED_)
#define AFX_COLORBALANCE_H__D4F689DE_BF2A_4A02_A810_02F1C7C08D41__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define CB_MEANS 0
#define CB_RATIOS 1
#define CB_GEOM_RATIOS 2

#include "EdtHistogram.h"

class EdtColorBalance  : public EdtHistogram
{
public:
	int m_nMode;
	EdtColorBalance();
	virtual ~EdtColorBalance();

	byte m_nMinValue;
	byte m_nMaxValue;

	bool m_bAutoRange;

	double m_dAutoPercent;

	int ComputeColorBalance(EdtImage *pImage, double *ratios, int mode = CB_RATIOS);
	
};

#endif // !defined(AFX_COLORBALANCE_H_D4F689DE_BF2A_4A02_A810_02F1C7C08D41_INCLUDED_)
