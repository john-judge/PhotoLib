// EdtImgFilter.cpp: implementation of the CEdtImgFilter class.
//
//////////////////////////////////////////////////////////////////////

#include "EdtImage.h"
#include "EdtImgFilter.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEdtImgFilter::CEdtImgFilter()
{
    m_nFilterType = EDT_KERNEL_USERDEFINED;
    m_dGain = 1.0;
    m_nOffset = 0;
    m_bAbsolute = false;

}

CEdtImgFilter::~CEdtImgFilter()
{

}

int CEdtImgFilter::Compute(EdtImage *pSrc, EdtImage *pDest)

{
    bool bGain;
    int row,
	col;

    if (! ( pSrc && pDest))
	return -1;

    bGain = (m_dGain != 1.0);

    // radius is half the kernel size
    int radius = edt_max(m_Kernel.GetWidth(), m_Kernel.GetHeight());

    radius /= 2;


    int nWidth = edt_min(pSrc->GetWidth(), pDest->GetWidth());
    int nHeight = edt_min(pSrc->GetHeight(), pDest->GetHeight());

    int kXMin = - (m_Kernel.GetWidth() / 2);
    int kXMax = kXMin + m_Kernel.GetWidth();
    int kYMin = - (m_Kernel.GetHeight() / 2);
    int kYMax = kYMin + m_Kernel.GetHeight();

    // Very inefficient general convolution routine

    switch(m_Kernel.GetType())
    {
    case TypeINT:
	{				

	    m_Sum.Create(TypeINT, nWidth, nHeight );

	    m_Sum.Clear();

	    switch (pSrc->GetType())
	    {

	    case TypeBYTE:
		{
		    byte **bpRows = (byte **) pSrc->GetPixelRows();
		    int **lpRows = (int **) m_Sum.GetPixelRows();
		    int *kData = (int *) m_Kernel.GetFirstPixel();


		    for (row = radius; row < nHeight - radius; row ++)
		    {

			for (col = radius; col < nWidth - radius; col++)
			{
			    int sum = 0;
			    int x,y, k =0;

			    for (y = kYMin; y < kYMax; y++)
				for (x=kXMin; x < kXMax; x++, k++)
				{
				    sum += bpRows[row + y][col + x] * kData[k];
				}

				lpRows[row][col] = sum;

			}
		    }
		}

		break;

	    case TypeUSHORT:
		{
		    ushort_t **bpRows = (ushort_t **) pSrc->GetPixelRows();
		    int **lpRows = (int **) m_Sum.GetPixelRows();
		    int *kData = (int *) m_Kernel.GetFirstPixel();


		    for (row = radius; row < nHeight - radius; row ++)
		    {

			for (col = radius; col < nWidth - radius; col++)
			{
			    int sum = 0;
			    int x,y, k =0;

			    for (y = kYMin; y < kYMax; y++)
				for (x=kXMin; x < kXMax; x++, k++)
				{
				    sum += bpRows[row + y][col + x] * kData[k];
				}

				lpRows[row][col] = sum;

			}
		    }
		}

		break;
	    }

	    switch (pDest->GetType())
	    {
	    case TypeBYTE:
		{
		    byte **bpRows = (byte **) pDest->GetPixelRows();
		    int **lpRows = (int **) m_Sum.GetPixelRows();
		    if (m_bAbsolute)
		    {
			for (row = radius; row < nHeight - radius; row ++)
			{

			    for (col = radius; col < nWidth - radius; col++)
			    {
				int v = abs((int) ((lpRows[row][col] * m_dGain) + m_nOffset));

				if (v > 0xff) v = 0xff;

				bpRows[row][col] = (byte) v;

			    }
			}
		    } 
		    else
		    {
			for (row = radius; row < nHeight - radius; row ++)
			{

			    for (col = radius; col < nWidth - radius; col++)
			    {
				int v = (int) (lpRows[row][col] * m_dGain) + m_nOffset;

				if (v > 0xff) v = 0xff;
				else if (v < 0) v = 0;

				bpRows[row][col] = (byte) v;

			    }
			}
		    }

		}
		break;

	    case TypeUSHORT:
		{
		    ushort_t **bpRows = (ushort_t **) pDest->GetPixelRows();
		    int **lpRows = (int **) m_Sum.GetPixelRows();
		    if (m_bAbsolute)
		    {
			for (row = radius; row < nHeight - radius; row ++)
			{

			    for (col = radius; col < nWidth - radius; col++)
			    {
				int v = abs((int) (lpRows[row][col] * m_dGain) + m_nOffset);

				if (v > 0xffff) v = 0xffff;

				bpRows[row][col] = (ushort_t) v;
			    } 
			}
		    }
		    else
		    {
			for (row = radius; row < nHeight - radius; row ++)
			{
			    for (col = radius; col < nWidth - radius; col++)
			    {
				int v = (int) (lpRows[row][col] * m_dGain) + m_nOffset;

				if (v > 0xffff) v = 0xffff;
				else if (v < 0) v = 0;

				bpRows[row][col] = (ushort_t) v;
			    }
			}
		    }
		}

		break;

	    }
	}
	break;

    }

    return 0;
}


void CEdtImgFilter::CreateKernel(int nWidth, int nHeight, EdtDataType nType)

{
    m_Kernel.Create(nType, nWidth, nHeight);

}

void CEdtImgFilter::SetKernel(int *pKernel, int nWidth, int nHeight)

{
    m_Kernel.AttachToData((u_char *) pKernel, TypeINT, nWidth, nHeight, 0);
}

void CEdtImgFilter::SetKernel(double *pKernel, int nWidth , int nHeight)

{
    m_Kernel.AttachToData((u_char *) pKernel, TypeDOUBLE, nWidth, nHeight, 0);

}

void CEdtImgFilter::SetKernelElement(int nX, int nY, int nValue)

{
    m_Kernel.SetPixel(nX, nY, nValue);
}

void CEdtImgFilter::SetKernelElement(int nX, int nY, double nValue)

{
    m_Kernel.SetPixel(nX, nY, nValue);
}

void CEdtImgFilter::GetKernelElement(int nX, int nY, int &nValue)

{	
    m_Kernel.GetPixel(nX, nY, nValue);
}

void CEdtImgFilter::GetKernelElement(int nX, int nY, double &nValue)

{
    m_Kernel.GetPixel(nX, nY, nValue);
}

static int nLowPassValues[9] = {1,1,1,
1,1,1,
1,1,1};
static int nHighPassValues[9] = {0,-1,0,
-1,9,-1,
0,-1, 0};

static int nSobelValues[9] = {0,-1,0,
-1,4,-1,
0,-1, 0};

void CEdtImgFilter::SetStandardKernel(int nStandardType)

{


    switch (nStandardType)
    {
    case EDT_KERNEL_LOWPASS:
	SetKernel(nLowPassValues, 3, 3);
	m_nFilterType = nStandardType;
	SetGain(1.0 / 9.0);
	m_bAbsolute = false;
	break;

    case EDT_KERNEL_HIGHPASS:
	SetKernel(nHighPassValues, 3, 3);
	m_nFilterType = nStandardType;
	SetGain(0.2);
	m_bAbsolute = false;
	break;

    case EDT_KERNEL_SOBEL:
	SetKernel(nSobelValues, 3, 3);
	m_nFilterType = nStandardType;
	SetGain(1);
	m_bAbsolute = true;
	break;

    }

}

