// ColorBalance.cpp: implementation of the CColorBalance class.
//
//////////////////////////////////////////////////////////////////////

#include "edtinc.h"

#include "ColorBalance.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

EdtColorBalance::EdtColorBalance()
{
    m_nMinValue = 1;
    m_nMaxValue = 255;
    m_nMode = CB_RATIOS;
    m_bAutoRange = true;
    m_dAutoPercent = 10.0;
}

EdtColorBalance::~EdtColorBalance()
{

}

int EdtColorBalance::ComputeColorBalance(EdtImage *pImage, double *ratios, int mode)

{

    EdtImgStats stats[3];

    // Check image type - only makes sense if it's RGB

    if (!pImage || 
	(pImage->GetType() != TypeBGR && 
	pImage->GetType() != TypeRGB &&
	pImage->GetType() != TypeBGRA && 
	pImage->GetType() != TypeRGBA )
	) 
    {

	return -1;

    }

    if (m_bAutoRange) {

	Compute(pImage, true);

	ComputeStatistics(stats);


	// Check top 10 % of brightest green pixels 

	int checkval = (int) (m_nSum * (m_dAutoPercent / 100.0));

	int sum = 0;
	int i;

	for (i=254;i>1;i--)
	{

	    sum += m_pHist[1][i];
	    if (sum > checkval)
		break;

	}

	if (i == 1)
	{

	    return -1;
	}
	else
	{

	    m_nMinValue = i;
	    m_nMaxValue = 254;
	}

    }

    if (mode == CB_MEANS)
    {



	double gsum = 0, rsum = 0, bsum = 0;
	byte red, green, blue;

	// Compute the mean ratio between red/g. blue/g - the green value will represent the mean

	// Only use values which fall within range defined by m_nValMin & m_nValMax


	byte **rows_bp = (byte **) pImage->GetPixelRows();
	byte *bp;


	Setup(pImage);

	int row,
	    col,
	    n = 0;

	int nWidth = pImage->GetWidth();
	int nHeight = pImage->GetHeight();

	if (m_nSubSample < 1)
	    m_nSubSample = 1;

	for (row = 0;row < nHeight;row += m_nSubSample)
	{
	    bp = rows_bp[row];

	    for (col = 0 ; col < nWidth*3;col += m_nSubSample*3)
	    {

		blue = bp[col];
		green = bp[col+1];
		red = bp[col+2];

		if (green >= m_nMinValue && green <= m_nMaxValue &&
		    blue > 0 && blue <= m_nMaxValue &&
		    red > 0 && red <= m_nMaxValue)
		{
		    rsum += (double) red;
		    bsum += (double) blue;
		    gsum += (double) green;

		    n++;
		}
	    }
	}

	if (n)
	{
	    ratios[0] = (gsum / rsum);
	    ratios[2] = (gsum / bsum);
	    ratios[1] = gsum / n;
	}

    }
    else if (mode == CB_RATIOS)
    {



	double gsum = 0, rsum = 0, bsum = 0;
	byte red, green, blue;

	// Compute the mean ratio between red/g. blue/g - the green value will represent the mean

	// Only use values which fall within range defined by m_nValMin & m_nValMax


	byte **rows_bp = (byte **) pImage->GetPixelRows();
	byte *bp;


	Setup(pImage);

	int row,
	    col,
	    n = 0;

	int nWidth = pImage->GetWidth();
	int nHeight = pImage->GetHeight();

	if (m_nSubSample < 1)
	    m_nSubSample = 1;

	for (row = 0;row < nHeight;row += m_nSubSample)
	{
	    bp = rows_bp[row];

	    for (col = 0 ; col < nWidth*3;col += m_nSubSample*3)
	    {

		blue = bp[col];
		green = bp[col+1];
		red = bp[col+2];

		if (green >= m_nMinValue && green <= m_nMaxValue &&
		    blue > 0 && blue <= m_nMaxValue &&
		    red > 0 && red <= m_nMaxValue)
		{
		    rsum += (double) green / (double) red;
		    bsum += (double) green / (double) blue;
		    gsum += (double) green;

		    n++;
		}
	    }
	}

	if (n)
	{
	    ratios[0] = rsum / n;
	    ratios[1] = gsum / n;
	    ratios[2] = bsum / n;
	}
    }

    return 0;
}

