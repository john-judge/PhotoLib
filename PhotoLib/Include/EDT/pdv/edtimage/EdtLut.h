// EdtLut.h: interface for the EdtLut class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _EDTLUT_H_
#define _EDTLUT_H_


#include "edtimage/EdtImage.h"



typedef struct RGBPixel {

    byte red, green, blue;

} RGBPixel;

class EdtLut  
{
protected:
    void *m_pLuts[EDT_MAX_COLOR_PLANES];
    bool m_bLutOwned[EDT_MAX_COLOR_PLANES];

    void *m_pSingleInputLut; // for 8-bit/16-bit -> RGB

    // Min and Max pixels for linear remap
    // If min > max, invert gray scale

    int 
        m_nPixelMin[EDT_MAX_COLOR_PLANES],
        m_nPixelMax[EDT_MAX_COLOR_PLANES];

    bool	m_bRangeDefined;
    bool	m_bNeedsUpdate;
    bool	m_bAutoAllocate;

    int m_nInputSize;

    int m_nInputType;
    int m_nOutputType;

    int m_nColors,
        m_nSrcColors,
        m_nTargetColors;


    double m_dGamma[EDT_MAX_COLOR_PLANES];

    int m_nInputMaxSize;

public:

    enum MapType {
        Copy = 0,
        Linear, 
        AutoLinear,
        Sqrt,
        AutoHEQ,
        Gamma,
        Custom
    };

    void UpdateMapGamma();
    void UpdateMapAutoLinear(EdtImage *pSrc = NULL);
    MapType m_MapType;
    void UpdateMapHeq(EdtImage *pImage);
    virtual void UpdateMap(EdtImage *pSrc = NULL); // Refresh lut based on different values

    void UpdateSingleInput();

    void UpdateMapLinear();
    void UpdateMapSqrt();

    void SetupTypes(int nInputType, int nOutputType, int nInputSize = 0,
        int nInputColors = 1, int nOutputColors = 1);

    void Destroy();
    void Allocate();
    EdtLut();
    virtual ~EdtLut();

    virtual void SetLutElement(void *pLut, const int nBand = 0);
    virtual void SetLutElements(void **pLuts);

    virtual void *GetLutElement(int nBand = 0)
    {
        return m_pLuts[nBand];
    }


    virtual int Transform(EdtImage *pSrc, EdtImage *pTarget,
        int nColor = -1);

    virtual int Transform(EdtImage *pSrc, EdtImage *pTarget,
        int fromx, int fromy, int width, int height, 
        int tox, int toy, int nColor = -1);

    virtual int Setup(EdtImage *pSrc, EdtImage *pTarget);

    virtual void SetLinear(int dLow, int dHigh, int nBand = -1);

    void SetMapType(MapType nMapType)
    {
        m_MapType = nMapType;
    }

    MapType GetMapType()
    {
        return m_MapType;
    }

    void SetGamma(double dValue, int nBand = -1)
    {
        m_dGamma[nBand] = dValue;
    }

    double GetGamma(int nBand = 0)
    {
        ASSERT(nBand >= 0 && nBand < EDT_MAX_COLOR_PLANES);
        return m_dGamma[nBand];
    }

    void SetAllGamma(double dValue)
    {
        for (int i = 0;i<EDT_MAX_COLOR_PLANES;i++)
            m_dGamma[i] = dValue;
    }



    void SetAutoAllocate(bool bState)
    {
        m_bAutoAllocate = bState;
    }
    bool IsAutoAllocate() {return m_bAutoAllocate;}


    void SetMinValue(const int nPixelMin = 0, const int nBand = 0)
    {
        if (nBand == -1)
        {
            for (int i = 0; i< m_nSrcColors;i++)
            {
                SetMinValue(nPixelMin,i);
            }
        }
        else if (nPixelMin != m_nPixelMin[nBand]) 
        {
            m_nPixelMin[nBand] = nPixelMin;
            m_bNeedsUpdate = true;
        }

    }

    int GetMinValue(const int nBand = 0) const 
    {
        return m_nPixelMin[nBand];
    }

    void SetMaxValue(const int nPixelMax = 0, const int nBand = 0)
    {
        if (nBand == -1)
        {
            for (int i = 0; i< m_nSrcColors;i++)
            {
                SetMaxValue(nPixelMax,i);
            }
        } 
        else if (nPixelMax != m_nPixelMax[nBand]) 
        {

            m_nPixelMax[nBand] = nPixelMax;
            m_bNeedsUpdate = true;

        }
    }

    int GetMaxValue(const int nBand = 0) const 
    {
        return m_nPixelMax[nBand];
    }

    int GetInputType()
    {
        return m_nInputType;
    }

    int GetOutputType()
    {
        return m_nOutputType;
    }

    bool IsDataDependent()
    {
        return (m_MapType == AutoLinear || m_MapType == AutoHEQ);	
    }

    int GetDomainLength()
    {
        return m_nInputSize;
    }

};

#endif 
