// EdtImgProfile.cpp: implementation of the EdtImgProfile class.
//
//////////////////////////////////////////////////////////////////////

#include "EdtImage.h"

#include "EdtImgProfile.h"

#include "math.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

EdtImgProfile::EdtImgProfile()
{
    for (int i = 0;i< EDT_MAX_COLOR_PLANES;i++)
        m_pProfiles[i] = NULL;

    m_bEndsSet = false;

    m_Pt1.x = 0;
    m_Pt1.y = 0;	
    m_Pt2.x = 0;	
    m_Pt2.y = 0;

    m_pOffsets = NULL;
    m_nPixels = 0;

}

EdtImgProfile::~EdtImgProfile()
{
    if (m_pOffsets)
    {
        delete m_pOffsets;
        m_pOffsets = NULL;
    }
}


void EdtImgProfile::SetPoints(const int x1, const int y1, const int x2, const int y2)
{

    EdtPoint Pt1, Pt2;
    Pt1.x = x1;
    Pt1.y = y1;
    Pt2.x = x2;
    Pt2.y = y2;

    SetPoints(Pt1, Pt2);

}

void
    EdtImgProfile::SetPoints(const EdtPoint Pt1, const EdtPoint Pt2)

{

    if (m_bEndsSet && 
        m_Pt1.x == Pt1.x &&
        m_Pt1.y == Pt1.y &&
        m_Pt2.x == Pt2.x &&
        m_Pt2.y == Pt2.x &&
        m_pOffsets)

        return;

    m_Pt1 = Pt1;
    m_Pt2 = Pt2;

    double dx = (m_Pt2.x - m_Pt1.x);
    double dy = (m_Pt2.y - m_Pt1.y);

    int nPixels  = (int) sqrt((dx*dx) + (dy * dy));

    dx = dx / (double) nPixels;
    dy = dy / (double) nPixels;

    double x = m_Pt1.x;
    double y = m_Pt1.y;
    int i;


    if (m_nPixels != nPixels)
    {

        Destroy();

        if (m_pOffsets)
        {
            delete[] m_pOffsets;
            m_pOffsets = NULL;			
        }
    }

    m_nPixels = nPixels;

    if (!m_pOffsets)
        m_pOffsets = new EdtPoint[m_nPixels+1];

    for (x = m_Pt1.x, y = m_Pt1.y, i = 0; i < m_nPixels; i++, x += dx, y += dy)
    {
        m_pOffsets[i].x  = (int) x;
        m_pOffsets[i].y  = (int) y;
    }

    m_bEndsSet = true;


}

void EdtImgProfile::GetPointAtIndex(const int index, int &x, int &y, int &value, int band)

{
    if (index >= 0 && index < m_nPixels)
    {
        x = m_pOffsets[index].x;
        y = m_pOffsets[index].y;

        value = m_pProfiles[band][index]; 

    }

}
int EdtImgProfile::GetPointValuesAtIndex(const int index, int &x, int &y, int *values)

{
    if (index >= 0 && index < m_nPixels)
    {
        x = m_pOffsets[index].x;
        y = m_pOffsets[index].y;

        for (int band = 0; band < m_nColors; band ++)
            values[band] = m_pProfiles[band][index]; 

    }

    return m_nColors;

}

int * EdtImgProfile::GetProfile(int band)
{
    if (band < 0 || band >= EDT_MAX_COLOR_PLANES)
    {
        // ERROR
        return NULL;
    }

    return m_pProfiles[band];
}



void EdtImgProfile::Compute(EdtImage * pSrc, const int x1, const int y1, const int x2, const int y2)
{
    SetPoints(x1,y1,x2,y2);
    Compute(pSrc);
}

// CLip point against a rectangle
// Note this uses windows definition - right and bottom are NOT in the rectangle

bool EdtImgProfile::Clipped(EdtPoint &pt, const int left, const int top, const int right, const int bottom)

{
    bool rc = false;
    if (pt.x < left)
    {
        pt.x = left;
        rc = true;
    }
    else if (pt.x >= right)
    {
        pt.x = right - 1 ;
        rc = true;
    }

    if (pt.y < top)
    {
        pt.y = top;
        rc = true;
    }
    else if (pt.y >= bottom)
    {
        pt.y = bottom - 1 ;
        rc = true;
    }

    return rc;

}	


void EdtImgProfile::Compute(EdtImage * pSrc)
{

    bool bClipped = false;

    // Clip points

    if (!pSrc || !m_bEndsSet)
    {
        // Error
        return;
    }
    // Clip points to image

    EdtPoint Pt1 = m_Pt1, Pt2 = m_Pt2;
    EdtPoint StartPt1, StartPt2;

    if (Clipped(Pt1, 0,0,pSrc->GetWidth(),pSrc->GetHeight()) ||
        Clipped(Pt2, 0,0,pSrc->GetWidth(),pSrc->GetHeight()))

    {
        bClipped = true;

        StartPt1 = m_Pt1;
        StartPt2 = m_Pt2;

        SetPoints(Pt1,Pt2);
    }

    if (m_nColors != pSrc->GetNColors())

    {
        Destroy();
    }

    m_nColors = pSrc->GetNColors();


    int i;

    for (int band = 0; band < m_nColors; band++)
    {
        if (!m_pProfiles[band])
            m_pProfiles[band] = new int[m_nPixels];

        int bandoffset = band;

        if (pSrc->GetType() == EDT_TYPE_BGR)
            bandoffset = 2 - band;
        else if (pSrc->GetType() == EDT_TYPE_BGRA)
            bandoffset = 3 - band;

        switch (pSrc->GetType())
        {
        case EDT_TYPE_BYTE:
            {
                byte **bpRows = (byte **) pSrc->GetPixelRows();

                if (!bpRows) return;

                for (i = 0; i < m_nPixels; i++)
                {
                    m_pProfiles[band][i] = bpRows[m_pOffsets[i].y][m_pOffsets[i].x];
                }
            }
            break;

        case EDT_TYPE_USHORT:
            {
                ushort_t **spRows = (ushort_t **) pSrc->GetPixelRows();

                if (!spRows) return;

                for (i = 0; i < m_nPixels; i++)
                {
                    m_pProfiles[band][i] = spRows[m_pOffsets[i].y][m_pOffsets[i].x];
                }
            }
            break;

        case EDT_TYPE_BGR:
        case EDT_TYPE_RGB:
            {
                byte **bpRows = (byte **) pSrc->GetPixelRows();

                if (!bpRows) return;

                for (i = 0; i < m_nPixels; i++)
                {
                    m_pProfiles[band][i] = bpRows[m_pOffsets[i].y][m_pOffsets[i].x*3 + (bandoffset)];
                }
            }
            break;
        case EDT_TYPE_BGRA:
        case EDT_TYPE_RGBA:
            {
                byte **bpRows = (byte **) pSrc->GetPixelRows();

                if (!bpRows) return;

                for (i = 0; i < m_nPixels; i++)
                {
                    m_pProfiles[band][i] = bpRows[m_pOffsets[i].y][m_pOffsets[i].x*4 + (bandoffset)];
                }
            }	
            break;
        }


    }

    // Reset points
    if (bClipped)
    {
        SetPoints(StartPt1, StartPt2);
    }


}

void EdtImgProfile::Destroy()
{
    for (int i = 0;i< EDT_MAX_COLOR_PLANES;i++)
    {
        if (m_pProfiles[i])
        {
            delete[] m_pProfiles[i];
            m_pProfiles[i] = NULL;
        }
    }

}


