// EdtScaledLut.h: interface for the EdtScaledLut class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(EDTSCALED_LUT_H)
#define EDTSCALED_LUT_H

#include "EdtLut.h"
#include "ZoomData.h"

class EdtScaledLut : public EdtLut  
{
private:
    ZoomData *m_pZoom;
    bool m_bOwnZoom;

public:
    EdtScaledLut();
    virtual ~EdtScaledLut();



    virtual int Transform(EdtImage *pSrc, EdtImage *pTarget,
	int nColor = -1);

    virtual int Transform(EdtImage *pSrc, EdtImage *pTarget,
	int fromx, int fromy, int width, int height, 
	int tox, int toy, int nColor = -1);

    void SetZoomHandle(ZoomData *pZoom)
    {
	if (m_pZoom && m_bOwnZoom)
	{
	    delete m_pZoom;
	}

	m_pZoom = pZoom;
	m_bOwnZoom = false;

    }

    ZoomData *GetZoomHandle() 
    {
	return m_pZoom;
    }

    void SetZoom(const long nSourceX	= 0, const long nSourceY = 0,
	const double dZoomX = 1.0, const double dZoomY = 0.0)

    {
	if (m_pZoom)
	    m_pZoom->SetZoom(nSourceX, nSourceY, dZoomX, dZoomY);
    }

    void SetZoom(ZoomData *pZoom)

    {
	if (m_pZoom)
	    *m_pZoom = *pZoom;
    }

    virtual int TransformIntegerZoom(EdtImage *pSrc, EdtImage *pTarget,
	int fromx, int fromy, int width, int height, 
	int tox, int toy, int nColor = -1);

    virtual int TransformDoubleZoom(EdtImage *pSrc, EdtImage *pTarget,
	int fromx, int fromy, int width, int height, 
	int tox, int toy, int nColor = -1);

}
;

#endif // !defined(AFX_EDTSCALEDLUT_H__2DDC2ADA_2B88_435D_B378_90BE3FF94EDB__INCLUDED_)
