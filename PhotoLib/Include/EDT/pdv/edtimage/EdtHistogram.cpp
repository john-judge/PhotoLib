// EdtHistogram.cpp: implementation of the EdtHistogram class.
//
//////////////////////////////////////////////////////////////////////


#include "EdtHistogram.h"

#include <math.h>
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

EdtHistogram::EdtHistogram()
{
    m_nBins = 0;
    m_nBinSize = 1;
    m_nInputMax = 0;
    m_nInputMin = 0;
    m_nColors = 1;
    m_nSubSample= 1;
    m_nSum = 0;

    int i;
    for (i = 0;i< EDT_MAX_COLOR_PLANES;i++)
        m_pHist[i] = NULL;

    for (i = 0;i< 32;i++)
        m_BitCounts[i] = 0;
}

EdtHistogram::~EdtHistogram()
{
    Destroy();
}

void EdtHistogram::Setup(EdtImage * pImage)
{
    int newbins;

    if (!pImage || !pImage->IsAllocated())
        return; // Error

    newbins = (pImage->GetMaxValue() - pImage->GetMinValue() + 1);


    newbins /= m_nBinSize;

    if (newbins != m_nBins || pImage->GetNColors() != m_nColors)
    {
        Destroy();

        m_nBins = newbins;
        if (newbins > 256 && newbins < 0x10000)
        {
            newbins = 0x10000;
        }
        for (int i = 0; i < pImage->GetNColors();i++)
            m_pHist[i] = new int[newbins];
        m_nColors = pImage->GetNColors();

    }

    m_nInputMax = pImage->GetMaxValue();
    m_nInputMin = pImage->GetMinValue();

}

void EdtHistogram::Compute(EdtImage *pImage, bool bClear)

{
    if (!pImage || !pImage->IsAllocated())
        return; // Error

    Setup(pImage);
    if (bClear)
        Clear();

    int row,
        col;

    int nWidth = pImage->GetWidth();
    int nHeight = pImage->GetHeight();

    if (m_nSubSample < 1)
        m_nSubSample = 1;

    for (int band = 0;band < pImage->GetNColors();band++)
    {

        switch(pImage->GetType())
        {
        case TypeBYTE:
            {
                byte **bp = (byte **) pImage->GetPixelRows();

                for (row = 0;row < nHeight;row += m_nSubSample)
                    for (col = 0; col < nWidth;col += m_nSubSample)
                        m_pHist[band][bp[row][col]] += 1;
            }
            break;

        case TypeBGR:
            {
                byte **bp = (byte **) pImage->GetPixelRows();

                for (row = 0;row < nHeight;row += m_nSubSample)
                    for (col = (2-band) ; col < nWidth*3;col += m_nSubSample*3)
                        m_pHist[band][bp[row][col]] += 1;
            }
            break;

        case TypeBGRA:
            {
                byte **bp = (byte **) pImage->GetPixelRows();

                for (row = 0;row < nHeight;row += m_nSubSample)
                    for (col = (2-band) ; col < nWidth*4;col += m_nSubSample*4)
                        m_pHist[band][bp[row][col]] += 1;
            }
            break;
        case TypeRGB:
            {
                byte **bp = (byte **) pImage->GetPixelRows();

                for (row = 0;row < nHeight;row += m_nSubSample)
                    for (col = (band) ; col < nWidth*3;col += m_nSubSample*3)
                        m_pHist[band][bp[row][col]] += 1;
            }
            break;

        case TypeRGBA:
            {
                byte **bp = (byte **) pImage->GetPixelRows();

                for (row = 0;row < nHeight;row += m_nSubSample)
                    for (col = (band) ; col < nWidth*4;col += m_nSubSample*4)
                        m_pHist[band][bp[row][col]] += 1;
            }
            break;
        case TypeUSHORT:
            {
                ushort_t **sp = (ushort_t **) pImage->GetPixelRows();

                for (row = 0;row < nHeight;row += m_nSubSample)
                    for (col = 0; col < nWidth;col += m_nSubSample)
                        m_pHist[band][sp[row][col]] += 1;
            }

            break;
        }
    }

    m_nSum = (nWidth / m_nSubSample) * (nHeight / m_nSubSample);
}


void EdtHistogram::ComputeStatistics(EdtImgStats * pStats)

{
    for (int band = 0;band < m_nColors;band++)
    {

        pStats[band].Clear();
        int gvn;
        int gv;
        bool bMinSet = false;
        bool bMaxSet = false;

        int mode_max = 0;
        pStats[band].m_nMode = 0;

        // compute n;

        for (gv = 0; gv < m_nBins;gv++)
        {
            pStats[band].m_nN += m_pHist[band][gv];
        }

        int halfn = pStats[band].m_nN / 2;
        int nRunning = 0;

        for (gv = 0; gv < m_nBins;gv++)
        {
            if (gvn = m_pHist[band][gv])
            {
                if (!bMinSet)
                {
                    pStats[band].m_nMin = gv;
                    bMinSet = true;
                }

                pStats[band].m_nMax = gv;

                nRunning += gvn;

                if (nRunning >= halfn)
                {
                    pStats[band].m_nMedian = gv;
                    halfn = pStats[band].m_nN + 1;
                }

                if (gvn > mode_max)
                {
                    pStats[band].m_nMode = gv;
                    pStats[band].m_nModeFreq = gvn;
                    mode_max = gvn;
                }

                float fgvn = (float) gvn * (float) gv;

                pStats[band].m_dSum += fgvn;
                pStats[band].m_dSSQ += (fgvn * gv);

            }
        }

        if (pStats[band].m_nN > 1)
        {
            pStats[band].m_dMean = pStats[band].m_dSum / pStats[band].m_nN;

            double d = (pStats[band].m_dSSQ / pStats[band].m_nN) -
                (pStats[band].m_dMean * pStats[band].m_dMean);

            pStats[band].m_dSD = sqrt(fabs(d));
        }

        // find median

    }
}

void
    EdtHistogram::CountBits()

{
    m_nBits = 0;
    int nBins = m_nBins;
    int i;

    while (nBins)
    {
        m_nBits++;
        nBins >>= 1;
    }

    for (i=0;i<GetNBits();i++)
    {
        m_BitCounts[i] = 0;
    }

    int band;

    for (band = 0; band < m_nColors; band ++)
    {


        for (i=0; i< m_nBins;i++)
        {
            int val = i;
            int index = (band * 8);

            while (val)
            {
                if (val & 1)
                    m_BitCounts[index] += m_pHist[band][i];

                val >>= 1;
                index ++;

            }

        }

    }

}

// Clear histogram vectors
void EdtHistogram::Destroy()
{
    for (int i = 0;i< EDT_MAX_COLOR_PLANES;i++)
        if (m_pHist[i])
        {
            delete[] m_pHist[i];
            m_pHist[i]= NULL;
        }

        m_nBins = 0;
        m_nColors = 1;
}

// reset hist to all zeros
void EdtHistogram::Clear()
{
    for (int i=0;i< EDT_MAX_COLOR_PLANES; i++)
        if (m_pHist[i])
        {
            memset(m_pHist[i],0, m_nBins * sizeof(int));
        }

}

void EdtHistogram::MakeCumulative()
{
    for (int band = 0;band < m_nColors;band++)
    {
        for (int i = 1;i<m_nBins;i++)
            m_pHist[band][i] += m_pHist[band][i-1];
    }

}

// Returns value of largest bin
// (useful for plotting)

int EdtHistogram::GetModeValue()
{
    int nModeValue = 0;
    for (int band = 0;band < m_nColors;band++)
    {
        for (int i = 0;i<m_nBins;i++)
            if (m_pHist[band][i] > nModeValue)
                nModeValue = m_pHist[band][i];
    }

    return nModeValue;
}
