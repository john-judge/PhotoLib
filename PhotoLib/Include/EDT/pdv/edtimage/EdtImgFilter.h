// EdtImgFilter.h: interface for the CEdtImgFilter class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EDTIMGFILTER_H__BB6710D4_1D92_11D2_8DE7_00A0C932D513__INCLUDED_)
#define AFX_EDTIMGFILTER_H__BB6710D4_1D92_11D2_8DE7_00A0C932D513__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "edtimage/EdtImage.h"

#define EDT_KERNEL_USERDEFINED 0

#define EDT_KERNEL_LOWPASS 1
#define EDT_KERNEL_HIGHPASS 2
#define EDT_KERNEL_SOBEL	3
#define EDT_KERNEL_GAUSSIAN 4
#define EDT_KERNEL_ROBERTS	5
#define EDT_KERNEL_LAPLACIAN 6


class CEdtImgFilter  
{
	EdtImage m_Kernel;
	EdtImage m_Sum;
	
	double m_dGain;
	
	int m_nOffset;
	bool m_bAbsolute;

	int	m_nFilterType;

public:
	CEdtImgFilter();
	virtual ~CEdtImgFilter();

	int Compute(EdtImage *pSrc, EdtImage *pDest);

	void CreateKernel(int nWidth, int nHeight, EdtDataType nType = TypeINT);

	void SetKernel(int *pKernel, int nWidth, int nHeight);

	void SetKernel(double *pKernel, int nWidth, int nHeight);

	void SetKernelElement(int nX, int nY, int nValue);

	void SetKernelElement(int nX, int nY, double nValue);

	void GetKernelElement(int nX, int nY, int &nValue);

	void GetKernelElement(int nX, int nY, double &nValue);

	void SetGain(const double dGain) 
	{
		m_dGain = dGain;
	}

	double GetGain()
	{
		return m_dGain;
	}
	void SetOffset(const int nOffset) 
	{
		m_nOffset = nOffset;
	}

	double GetOffset()
	{
		return m_nOffset;
	}

	void SetStandardKernel(int nStandardType);

};

#endif // !defined(AFX_EDTIMGFILTER_H__BB6710D4_1D92_11D2_8DE7_00A0C932D513__INCLUDED_)
