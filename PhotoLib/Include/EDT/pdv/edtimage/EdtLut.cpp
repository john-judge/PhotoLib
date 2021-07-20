// EdtLut.cpp: implementation of the EdtLut class.
//
//////////////////////////////////////////////////////////////////////


#include "EdtLut.h"


#include "EdtHistogram.h"
#include "EdtImgMinMax.h"


#include "math.h"

#include <stdio.h>
#ifndef __APPLE__
#include <malloc.h>
#endif

#include "EdtRowOps.h"

#define thisprintf printf("%p : ", this); printf

#define RGBtoRGB16(r,g,b) ((((r)>>3) << 11) | (((g)>>2) << 5) | ((b) >> 3))
#define RGBtoRGB15(r,g,b) ((((r)>>3) << 10) | (((g)>>3) << 5) | ((b) >> 3))

/////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

EdtLut::EdtLut()
{
    for (int i = 0;i < EDT_MAX_COLOR_PLANES;i++)
    {
        m_pLuts[i] = NULL;
        m_bLutOwned[i] = false;
        m_nPixelMin[i] = 0;
        m_nPixelMax[i] = 255;
    }
    m_bRangeDefined = false;
    m_bNeedsUpdate	= true;
    m_bAutoAllocate = true;

    m_nInputType = EDT_TYPE_NULL;
    m_nOutputType = EDT_TYPE_NULL;

    m_MapType = Linear;

    SetAllGamma(1.0);

    m_pSingleInputLut = NULL;
    m_nColors =1 ;
    m_nInputSize = 0;
    m_nInputType = TypeBYTE;
    m_nSrcColors=1;
    m_nTargetColors = 1;
    m_nInputMaxSize = 0;
}

EdtLut::~EdtLut()
{

}

void
    EdtLut::SetupTypes(int nInputType, int nOutputType, int nInputSize,
    int nInputColors, int nOutputColors)

{
    if (m_nInputType != nInputType ||
        m_nOutputType != nOutputType ||
        m_nInputSize != nInputSize ||
        m_nSrcColors != nInputColors)
    {
        m_nInputType = nInputType;
        m_nOutputType = nOutputType;
        m_nInputSize = nInputSize;
        m_nInputMaxSize = nInputSize;

        m_nSrcColors = nInputColors;
        if (nOutputColors != -1)
            m_nTargetColors = nOutputColors;
        else
            m_nTargetColors = m_nSrcColors;

        m_nColors = edt_max(m_nTargetColors,m_nSrcColors);

        for (int i = 0;i < m_nColors;i++)
            m_nPixelMax[i] = nInputSize;

        if (m_nInputType == EDT_TYPE_SHORT ||
            m_nInputType == EDT_TYPE_USHORT)
            m_nInputMaxSize = 0x10000;

        if (m_bAutoAllocate)
            Allocate();

        // thisprintf("SetupTypes type = %d -> type %d srcolors = %d targetcolors = %d\n",
        // m_nInputType, m_nOutputType, nInputColors, nOutputColors);

        UpdateMap();

    }
}

void EdtLut::SetLutElement(void *pLut, const int nBand)

{
    // Check if allocated
    if (m_nInputType == EDT_TYPE_NULL)
        return;

    int band = nBand;

    if (band == -1)
        band = 0;

    if (!m_pLuts[band])
        Allocate();

    if (!m_pLuts[band])
        return;

    // copy to lut for band


    switch(m_nOutputType)
    {
    case EDT_TYPE_BYTE:
    case EDT_TYPE_BGR:
    case EDT_TYPE_RGB:
    case EDT_TYPE_BGRA:
    case EDT_TYPE_RGBA:
    case EDT_TYPE_RGB16:
    case EDT_TYPE_RGB15:

        {

            memcpy(m_pLuts[band], pLut, m_nInputMaxSize);

        }

        break;


    }

    UpdateSingleInput();

}
void EdtLut::SetLutElements(void **pLuts)

{
    // Check if allocated
    if (m_nInputType == EDT_TYPE_NULL)
        return;


    int band;


    if (!m_pLuts[0])
        Allocate();

    // copy to lut for band

    for (band = 0;band < m_nColors; band++)
    {
        switch(m_nOutputType)
        {
        case EDT_TYPE_BYTE:
        case EDT_TYPE_BGR:
        case EDT_TYPE_RGB:
        case EDT_TYPE_BGRA:
        case EDT_TYPE_RGBA:
        case EDT_TYPE_RGB16:
        case EDT_TYPE_RGB15:

            {

                memcpy(m_pLuts[band], pLuts[band], m_nInputMaxSize);

            }

            break;


        }
    }

    UpdateSingleInput();

}

int EdtLut::Transform(EdtImage *pSrc, EdtImage *pTarget, int nColor)

{
    // Not implemented yet
    if (!(pSrc && pTarget))
        return -1;

    int nWidth = edt_min(pSrc->GetWidth(), pTarget->GetWidth());
    int nHeight = edt_min(pSrc->GetHeight(), pTarget->GetHeight());

    return Transform(pSrc,pTarget,0,0,nWidth,nHeight,0,0,nColor);


}


int EdtLut::Transform(EdtImage *pSrc, EdtImage *pTarget,
    int fromx, int fromy, int width, int height,
    int tox, int toy, 
    int nColor)

{
    // Not implemented yet
    if (!(pSrc && pTarget))
        return -1;

    int nWidth = edt_min(pSrc->GetWidth(), pTarget->GetWidth());
    int nHeight = edt_min(pSrc->GetHeight(), pTarget->GetHeight());


    if (fromx < 0)
    {
        width += fromx;
        tox -= fromx;

        if (width <= 0)
            return -1;
    }

    if (tox < 0)
    {
        width += tox;
        fromx -= tox;

        if (width <= 0)
            return -1;
    }

    if (fromy < 0)
    {
        height += fromy;
        toy -= fromy;

        if (height <= 0)
            return -1;
    }

    if (toy < 0)
    {
        height += toy;
        fromy -= toy;

        if (height <= 0)
            return -1;
    }


    if (width + fromx >= pSrc->GetWidth())
    {
        nWidth = pSrc->GetWidth() - fromx;
    }
    else
        nWidth = width;

    if (height + fromy >= pSrc->GetHeight())
    {
        nHeight = pSrc->GetHeight() - fromy;
    }
    else
        nHeight = height;


    if (m_pLuts[0])
    {

        if (m_MapType == AutoLinear)
        {

            UpdateMapAutoLinear();
        }

        int nOpTypes = FUNC_MONAD(pSrc->GetType(), pTarget->GetType());

        byte **brows1, ** brows2;
        ushort_t **srows1, **srows2;
        RGBPixel  **rgbrows2;

        int startcolor = 0;
        int endcolor = m_nColors;

        int srcx, srcy;
        int destx, desty;

        // nColor == -1 means all bands

        if (nColor != -1)
        {
            startcolor = nColor;
            endcolor = nColor + 1;
            if (nColor >= m_nColors)
            {
                return -1;
            }
        }

        // Note - this doesn't do single color planes anymore - FIX!!!

        switch (nOpTypes)
        {

        case FUNC_BYTE_BYTE:
            {
                brows1 = (byte **) pSrc->GetPixelRows() + fromy;
                brows2 = (byte **) pTarget->GetPixelRows() + toy;

                CopyBlockLUTTyped(brows1, brows2, (byte *) m_pLuts[0],
                    nWidth, nHeight, fromx, tox, 1, 1);
            }
            break;

        case FUNC_USHORT_BYTE:

            {
                srows1 = (ushort_t **) pSrc->GetPixelRows() + fromy;
                brows2 = (byte **) pTarget->GetPixelRows() + toy;

                CopyBlockLUTTyped(srows1, brows2, (byte *) m_pLuts[0],
                    nWidth, nHeight, fromx, tox, 1, 1);
            }
            break;
        case FUNC_USHORT_USHORT:

            {
                srows1 = (ushort_t **) pSrc->GetPixelRows() + fromy;
                srows2 = (ushort_t **) pTarget->GetPixelRows() + toy;

                CopyBlockLUTTyped(srows1, srows2, (ushort_t *) m_pLuts[0],
                    nWidth, nHeight, fromx, tox, 1, 1);
            }
            break;



        case FUNC_BGR_BGR:
            {
                brows1 = (byte **) pSrc->GetPixelRows();
                brows2 = (byte **) pTarget->GetPixelRows();

                byte *pLut0 = (byte *) m_pLuts[0];
                byte *pLut1 = (byte *) m_pLuts[1];
                byte *pLut2 = (byte *) m_pLuts[2];

                byte *psrc, *pdest;

                nWidth *= 3;
                fromx *= 3;
                tox *= 3;

                for (srcy = fromy,desty = toy;desty<nHeight+toy;desty ++,srcy++)
                {
                    psrc = brows1[srcy];
                    pdest = brows2[srcy];

                    for (destx = tox, srcx= fromx; 
                        destx < nWidth+tox; 
                        srcx +=3)
                    {
                        byte v0 = pLut0[psrc[srcx]];
                        byte v1 = pLut1[psrc[srcx+1]];
                        byte v2 = pLut2[psrc[srcx+2]];

                        pdest[destx] = v0;
                        pdest[destx+1] = v1;
                        pdest[destx+2] = v2;
                        destx += 3;
                    }		

                }		

            }
            break;


        case FUNC_BGR_RGB:
            {
                brows1 = (byte **) pSrc->GetPixelRows();
                brows2 = (byte **) pTarget->GetPixelRows();

                byte *pLut0 = (byte *) m_pLuts[0];
                byte *pLut1 = (byte *) m_pLuts[1];
                byte *pLut2 = (byte *) m_pLuts[2];

                byte *psrc, *pdest;

                nWidth *= 3;
                fromx *= 3;
                tox *= 3;

                for (srcy = fromy,desty = toy;desty<nHeight+toy;desty ++,srcy++)
                {
                    psrc = brows1[srcy];
                    pdest = brows2[srcy];

                    for (destx = tox, srcx= fromx; 
                        destx < nWidth+tox; 
                        srcx +=3)
                    {
                        byte v0 = pLut0[psrc[srcx]];
                        byte v1 = pLut1[psrc[srcx+1]];
                        byte v2 = pLut2[psrc[srcx+2]];

                        pdest[destx] = v2;
                        pdest[destx+1] = v1;
                        pdest[destx+2] = v1;
                        destx += 3;
                    }		

                }		

            }
            break;

        case FUNC_BYTE_BGR:
            {
                brows1 = (byte **) pSrc->GetPixelRows() + fromy ;
                rgbrows2 = (RGBPixel **) pTarget->GetPixelRows() + toy;

                CopyBlockLUTTyped(brows1, rgbrows2,(RGBPixel *) m_pSingleInputLut,
                    nWidth, nHeight, fromx, tox, 1, 1);

            }
            break;

        case FUNC_USHORT_BGR:
            {
                srows1 = (ushort_t **) pSrc->GetPixelRows() + fromy  ;
                rgbrows2 = (RGBPixel **) pTarget->GetPixelRows() + toy;

                CopyBlockLUTTyped(srows1, rgbrows2,(RGBPixel *) m_pSingleInputLut,
                    nWidth, nHeight, fromx, tox, 1, 1);

            }

            break;

        case FUNC_BGR_RGB16:
            {
                brows1 = (byte **) pSrc->GetPixelRows() ;
                srows2 = (ushort_t **) pTarget->GetPixelRows();


                byte *pLut0 = (byte *) m_pLuts[0];
                byte *pLut1 = (byte *) m_pLuts[1];
                byte *pLut2 = (byte *) m_pLuts[2];

                byte *psrc;
                ushort_t *pdest;

                fromx *= 3;

                for (srcy = fromy,desty = toy;desty<nHeight+toy;desty ++,srcy++)
                {
                    psrc = brows1[srcy];
                    pdest = srows2[toy];

                    for (destx = tox, srcx= fromx; 
                        destx < nWidth+tox; 
                        srcx +=3)
                    {

                        byte v0 = pLut0[psrc[srcx]];
                        byte v1 = pLut1[psrc[srcx+1]];
                        byte v2 = pLut2[psrc[srcx+2]];

                        ushort_t v = RGBtoRGB16(v0,v1,v2);

                        pdest[destx++] = v;

                    }	
                }		

            }
            break;

        case FUNC_BYTE_RGB16:
            {
                brows1 = (byte **) pSrc->GetPixelRows()  + fromy  ;
                ushort_t **srows2 = (ushort_t **) pTarget->GetPixelRows() + toy;

                CopyBlockLUTTyped(brows1, srows2,(ushort_t *) m_pSingleInputLut,
                    nWidth, nHeight, fromx, tox, 1, 1);

            }
            break;

        case FUNC_USHORT_RGB16:
            {
                {
                    srows1 = (ushort_t **) pSrc->GetPixelRows() + fromy  ;
                    ushort_t **srows2 = (ushort_t **) pTarget->GetPixelRows() + toy;

                    CopyBlockLUTTyped(srows1, srows2,(ushort_t *) m_pSingleInputLut,
                        nWidth, nHeight, fromx, tox, 1, 1);

                }
            }

            break;

        case FUNC_BGR_BGRA:
            {
                brows1 = (byte **) pSrc->GetPixelRows();
                brows2 = (byte **) pTarget->GetPixelRows() ;


                byte *pLut0 = (byte *) m_pLuts[0];
                byte *pLut1 = (byte *) m_pLuts[1];
                byte *pLut2 = (byte *) m_pLuts[2];

                byte *psrc, *pdest;

                nWidth *= 4;
                fromx *= 3;
                tox *= 4;

                for (srcy = fromy,desty = toy;desty<nHeight+toy;desty ++,srcy++)
                {
                    psrc = brows1[srcy];
                    pdest = brows2[desty];

                    for (destx = tox, srcx= fromx; 
                        destx < nWidth+tox; 
                        srcx +=3)
                    {
                        byte v0 = pLut0[psrc[srcx]];
                        byte v1 = pLut1[psrc[srcx+1]];
                        byte v2 = pLut2[psrc[srcx+2]];

                        pdest[destx] = v0;
                        pdest[destx+1] = v1;
                        pdest[destx+2] = v2;

                        destx += 4;

                    }		
                }	
            }
            break;

        case FUNC_BYTE_BGRA:
            {
                brows1 = (byte **) pSrc->GetPixelRows() + fromy ;
                uint **irows2 = (uint **) pTarget->GetPixelRows() + toy;

                CopyBlockLUTTyped(brows1, irows2,(uint *) m_pSingleInputLut,
                    nWidth, nHeight, fromx, tox, 1, 1);


            }
            break;

        case FUNC_USHORT_BGRA:
            {
                srows1 = (ushort_t **) pSrc->GetPixelRows() + fromy;
                uint **lrows2 = (uint **) pTarget->GetPixelRows() + toy;

                CopyBlockLUTTyped(srows1, lrows2,(uint *) m_pSingleInputLut,
                    nWidth, nHeight, fromx, tox, 1, 1);

            }
            break;

        default:
            // ERROR - not implemented yet
            return -1;

        }
    }

    else
        pTarget->Copy(pSrc);

    return 0;
}


void EdtLut::UpdateSingleInput()
{
    if (m_pSingleInputLut)

    {
        int nOpTypes = FUNC_MONAD(m_nInputType, m_nOutputType);
        int i;

        switch (nOpTypes)
        {

        case FUNC_BYTE_BGR:
        case FUNC_USHORT_BGR:
            {
                RGBPixel *rgblut = (RGBPixel *) m_pSingleInputLut;
                byte *pLut0 = (byte *) m_pLuts[0];
                byte *pLut1 = (byte *) m_pLuts[1];
                byte *pLut2 = (byte *) m_pLuts[2];

                for (i = 0; i< m_nInputMaxSize;i++)
                {
                    rgblut[i].red = pLut0[i];
                    rgblut[i].green = pLut1[i];
                    rgblut[i].blue = pLut2[i];
                }
            }
            break;

        case FUNC_BYTE_RGB:
        case FUNC_USHORT_RGB:

            {
                RGBPixel *rgblut = (RGBPixel *) m_pSingleInputLut;
                byte *pLut0 = (byte *) m_pLuts[0];
                byte *pLut1 = (byte *) m_pLuts[1];
                byte *pLut2 = (byte *) m_pLuts[2];

                for (i = 0; i< m_nInputMaxSize;i++)
                {
                    rgblut[i].red = pLut0[i];
                    rgblut[i].green = pLut1[i];
                    rgblut[i].blue = pLut2[i];

                }
            }


            break;

        case FUNC_BYTE_BGRA:
        case FUNC_USHORT_BGRA:

            {
                int *rgblut = (int *) m_pSingleInputLut;
                byte *pLut0 = (byte *) m_pLuts[0];
                byte *pLut1 = (byte *) m_pLuts[1];
                byte *pLut2 = (byte *) m_pLuts[2];

                for (i = 0; i< m_nInputMaxSize;i++)
                {
                    rgblut[i] = pLut0[i] | (pLut1[i] << 8) | (pLut2[i] << 16);
                }
            }


            break;


        case FUNC_BYTE_RGBA:
        case FUNC_USHORT_RGBA:
            {
                int *rgblut = (int *) m_pSingleInputLut;
                byte *pLut0 = (byte *) m_pLuts[0];
                byte *pLut1 = (byte *) m_pLuts[1];
                byte *pLut2 = (byte *) m_pLuts[2];

                for (i = 0; i< m_nInputMaxSize;i++)
                {
                    rgblut[i] = pLut0[i] << 16 | (pLut1[i] << 8) | (pLut2[i]);
                }
            }

            break;


        case FUNC_BYTE_RGB16:
        case FUNC_USHORT_RGB16:
            {
                ushort_t *rgblut = (ushort_t *) m_pSingleInputLut;
                byte *pLut0 = (byte *) m_pLuts[0];
                byte *pLut1 = (byte *) m_pLuts[1];
                byte *pLut2 = (byte *) m_pLuts[2];

                for (i = 0; i< m_nInputMaxSize;i++)
                {
                    rgblut[i] = RGBtoRGB16(pLut0[i],pLut1[i],pLut2[i]);
                }
            }

            break;

        case FUNC_BYTE_RGB15:
        case FUNC_USHORT_RGB15:	
            {
                ushort_t *rgblut = (ushort_t *) m_pSingleInputLut;
                byte *pLut0 = (byte *) m_pLuts[0];
                byte *pLut1 = (byte *) m_pLuts[1];
                byte *pLut2 = (byte *) m_pLuts[2];

                for (i = 0; i< m_nInputMaxSize;i++)
                {
                    rgblut[i] = RGBtoRGB15(pLut0[i],pLut1[i],pLut2[i]);
                }
            }

            break;

        }
    }
}

int EdtLut::Setup(EdtImage *pSrc, EdtImage *pTarget)

{


    SetupTypes(pSrc->GetType(), pTarget->GetType(),
        pSrc->GetMaxValue()+1,pSrc->GetNColors(), pTarget->GetNColors());
    return 0;
}

void EdtLut::SetLinear(int dMin, int dMax, int nBand)

{
    if (dMax == 0)
        dMax = 255;

    // thisprintf("SetLinear %d, %d, %d\n", dMin,dMax,nBand);


    SetMinValue(dMin,nBand);
    SetMaxValue(dMax,nBand);
    if (m_bNeedsUpdate)
    {
        UpdateMapLinear();
    }

}

void EdtLut::Allocate()
{
    if (m_nInputType == EDT_TYPE_NULL)
        // ERROR - types not initialized
        return;

    Destroy();

    int nOpTypes = FUNC_MONAD(m_nInputType, m_nOutputType);

    for (int i = 0; i < m_nColors; i++)
    {
        m_bLutOwned[i] = true;

        switch (m_nOutputType)
        {

        case EDT_TYPE_BYTE:
        case EDT_TYPE_CHAR:
        case EDT_TYPE_RGB:
        case EDT_TYPE_RGBA:
        case EDT_TYPE_RGB16:
        case EDT_TYPE_RGB15:
        case EDT_TYPE_BGRA:
        case EDT_TYPE_BGR:
            {
                byte *pbTmp = (byte *) calloc(1,m_nInputMaxSize);
                for (int j =0; j< m_nInputSize;j++)
                    pbTmp[j] = (byte) j;

                m_pLuts[i] = pbTmp;
            }
            break;

        case EDT_TYPE_SHORT:
        case EDT_TYPE_USHORT:
            {
                ushort_t *psTmp = (ushort_t *) calloc(sizeof(short),m_nInputMaxSize);
                for (int j =0; j< m_nInputSize;j++)
                    psTmp[j] = (ushort_t) j;

                m_pLuts[i] = psTmp;
            }
            break;

        case EDT_TYPE_FLOAT:	
            {
                float *psTmp = (float *) calloc(sizeof(float),m_nInputMaxSize);
                for (int j =0; j< m_nInputSize;j++)
                    psTmp[j] = (float) j;

                m_pLuts[i] = psTmp;
            }
            break;

        case EDT_TYPE_DOUBLE:
            {
                double *psTmp = (double *) calloc(sizeof(double),m_nInputMaxSize);
                for (int j =0; j< m_nInputSize;j++)
                    psTmp[j] = (double) j;

                m_pLuts[i] = psTmp;
            }
            break;

        case EDT_TYPE_INT:
        case EDT_TYPE_UINT:
            {
                unsigned int *psTmp = (unsigned int *) calloc(sizeof(unsigned int),m_nInputMaxSize);
                for (int j =0; j< m_nInputSize;j++)
                    psTmp[j] = (unsigned int) j;

                m_pLuts[i] = psTmp;
            }
            break;
        }

    }

    switch (nOpTypes)
    {

    case FUNC_BYTE_BGR:
    case FUNC_USHORT_BGR:
    case FUNC_BYTE_RGB:
    case FUNC_USHORT_RGB:

        m_pSingleInputLut = (void *) calloc(sizeof(RGBPixel), m_nInputMaxSize);
        break;

    case FUNC_BYTE_BGRA:
    case FUNC_USHORT_BGRA:
    case FUNC_BYTE_RGBA:
    case FUNC_USHORT_RGBA:

        m_pSingleInputLut = (void *) calloc(sizeof(int), m_nInputMaxSize);
        break;


    case FUNC_BYTE_RGB16:
    case FUNC_USHORT_RGB16:
    case FUNC_BYTE_RGB15:
    case FUNC_USHORT_RGB15:
        m_pSingleInputLut = (void *) calloc(sizeof(short), m_nInputMaxSize);
        break;

    default:
        if (m_pSingleInputLut)
        {
            free(m_pSingleInputLut);
            m_pSingleInputLut = NULL;
        }
    }

}

void EdtLut::Destroy()
{
    for (int i = 0; i < m_nColors; i++)
    {
        if (m_bLutOwned[i])
        {
            free(m_pLuts[i]);
            m_pLuts[i] = NULL;
        }
    }
}

void EdtLut::UpdateMap(EdtImage *pImage)

{
    switch (m_MapType)
    {

    case Linear:
        UpdateMapLinear();
        break;

    case Sqrt:
        UpdateMapSqrt();
        break;

    case AutoLinear:
        UpdateMapAutoLinear(pImage);
        break;

    case AutoHEQ:
        UpdateMapHeq(pImage);
        break;

    case Gamma:
        UpdateMapGamma();
        break;

    }


}

void EdtLut::UpdateMapAutoLinear(EdtImage *pSrc)

{
    if (pSrc)
    {
        EdtImgMinMax MinMax;

        for (int i=0;i<m_nColors;i++)
        {
            MinMax.Compute(pSrc,m_nPixelMin[i], m_nPixelMax[i],i);
        }

    }

    UpdateMapLinear();
}

void EdtLut::UpdateMapSqrt()

{
    if (m_nInputType == EDT_TYPE_NULL)
        return;

    if (!m_pLuts[0])
        Allocate();

    for (int i=0;i<m_nColors;i++)
    {
        byte *blp;

        switch(m_nOutputType)
        {
        case EDT_TYPE_BYTE:
        case EDT_TYPE_BGR:
        case EDT_TYPE_RGB:
        case EDT_TYPE_BGRA:
        case EDT_TYPE_RGBA:
        case EDT_TYPE_RGB16:
        case EDT_TYPE_RGB15:

            {
                int output_max = 255;
                int output_min = 0;

                int n1 = (int) m_nPixelMin[i];
                int n2 = (int) m_nPixelMax[i];
                int dRange = abs(n1 - n2);

                double deltagv = (double) (output_max - output_min) / (double) dRange ;
                double dgv;
                double sqmax = sqrt((double)m_nInputSize);
                int gv;

                blp = (byte *) m_pLuts[i];

                if (n1 > n2)
                {

                    for (gv = m_nInputSize-1;gv > n1;gv--)
                        blp[gv] = output_min;

                    dgv = output_min;
                    for (gv = n1 ; gv > n2; gv--)
                    {
                        blp[gv] = (int) (sqrt(dgv) * 16.0 + 0.5);
                        dgv += deltagv;
                    }
                    for (gv = n2;gv >= 0;gv--)
                        blp[gv] = output_max;

                }
                else
                {
                    for (gv = 0;gv < n1;gv++)
                        blp[gv] = output_min;

                    dgv = output_min;
                    for (gv = n1 ; gv < n2; gv++)
                    {
                        blp[gv] = (int) (sqrt(dgv) * 16.0 + 0.5);
                        dgv += deltagv;
                    }
                    for (gv = n2;gv < m_nInputSize;gv++)
                        blp[gv] = output_max;
                }
            }
            break;


        }
    }

    UpdateSingleInput();

}


void EdtLut::UpdateMapLinear()

{
    if (m_nInputType == EDT_TYPE_NULL)
        return;

    if (!m_pLuts[0])
        Allocate();

    // thisprintf("UpdateMapLinear colors = %d luts = %p,%p,%p\n",
    // m_nColors, m_pLuts[0], m_pLuts[1], m_pLuts[2]);

    for (int i=0;i<m_nColors;i++)
    {
        byte *blp;

        switch(m_nOutputType)
        {
        case EDT_TYPE_BYTE:
        case EDT_TYPE_BGR:
        case EDT_TYPE_RGB:
        case EDT_TYPE_BGRA:
        case EDT_TYPE_RGBA:

        case EDT_TYPE_RGB15:
        case EDT_TYPE_RGB16:
            {
                int output_max = 255;
                int output_min = 0;


                int n1 = (int) m_nPixelMin[i];
                int n2 = (int) m_nPixelMax[i];
                int dRange = abs(n1 - n2);

                // thisprintf("UpdateMapLinear max = %d %d-%d -> %d-%d\n",
                // m_nInputSize, n1,n2,output_min,output_max);

                double deltagv = (double) (output_max - output_min) / (double) dRange ;
                double dgv;
                int gv;

                blp = (byte *) m_pLuts[i];

                if (n1 > n2)
                {

                    for (gv = m_nInputSize-1;gv > n1;gv--)
                        blp[gv] = output_min;

                    dgv = output_min;
                    for (gv = n1 ; gv > n2; gv--)
                    {
                        blp[gv] = (int) (dgv);
                        dgv += deltagv;
                    }
                    for (gv = n2;gv >= 0;gv--)
                        blp[gv] = output_max;

                }
                else
                {
                    for (gv = 0;gv < n1;gv++)
                        blp[gv] = output_min;

                    dgv = output_min;
                    for (gv = n1 ; gv < n2; gv++)
                    {
                        blp[gv] = (int) (dgv);
                        dgv += deltagv;
                    }
                    for (gv = n2;gv < m_nInputSize;gv++)
                        blp[gv] = output_max;
                }
            }
            break;


        }
    }

    UpdateSingleInput();

}

void EdtLut::UpdateMapHeq(EdtImage * pImage)
{
    //
    if (m_nInputType == EDT_TYPE_NULL)
        return;

    if (!m_pLuts[0])
        Allocate();

    // Compute Histogram

    EdtHistogram Hist;

    Hist.Compute(pImage);


    Hist.MakeCumulative();

    int band;

    for (band=0;band<m_nColors;band++)
    {
        byte *blp;
        int *hp = Hist.GetHistogram(band);

        switch(m_nOutputType)
        {
        case EDT_TYPE_BYTE:
        case EDT_TYPE_BGR:
        case EDT_TYPE_RGB:
        case EDT_TYPE_BGRA:
        case EDT_TYPE_RGBA:
        case EDT_TYPE_RGB16:
        case EDT_TYPE_RGB15:

            {
                blp = (byte *) m_pLuts[band];

                int nMax = 256;

                double gvprop = (double) nMax / (double) pImage->GetNPixels();

                int srci = 0;
                int value;

                for (int i = 0; i< m_nInputSize; i++)
                {

                    value = (int) (hp[i] * gvprop + 0.5);

                    if (value > 255)
                        value = 255;

                    blp[i] = value;

                }
            }
            break;

        }

    }

    UpdateSingleInput();

}


void EdtLut::UpdateMapGamma()
{
    if (m_nInputType == EDT_TYPE_NULL)
        return;

    if (!m_pLuts[0])
        Allocate();

    for (int i=0;i<m_nColors;i++)
    {
        byte *blp;

        switch(m_nOutputType)
        {
        case EDT_TYPE_BYTE:
        case EDT_TYPE_BGR:
        case EDT_TYPE_RGB:
        case EDT_TYPE_BGRA:
        case EDT_TYPE_RGBA:
        case EDT_TYPE_RGB16:
        case EDT_TYPE_RGB15:

            {
                double dgam;

                blp = (byte *) m_pLuts[i];

                double dmax = m_nInputSize - 1;


                for (int gv = 0; gv< m_nInputSize ; gv++)
                {
                    dgam = 255 * pow( (double) gv/dmax, 1.0 / m_dGamma[i] );

                    blp[gv] = (byte) dgam;

                }


            }
            break;


        }
    }

    UpdateSingleInput();
}
