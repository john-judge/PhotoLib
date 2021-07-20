// ZoomData.h: interface for the ZoomData class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ZOOMDATA_H)
#define _ZOOMDATA_H

#ifndef M_PI
#include <math.h>
#endif

class ZoomData  
{

private:
    double m_dZoomX,
	m_dZoomY;

    // If zoomed, source pixel for upper left corner

    int	m_nSourceX,
	m_nSourceY;
public:

    bool m_bActive;
    bool m_bIsInteger;
    bool m_bIsIdentity;

    ZoomData()
    {
	m_dZoomX = 1.0;
	m_dZoomY  = 1.0;
	m_bIsIdentity = true;
	m_bIsInteger = true;

	m_nSourceX = 0;
	m_nSourceY = 0;

	m_bActive = false;

    }

    virtual ~ZoomData()
    {

    }

    /// Zoom Value set/get functions

    bool IsActive() const
    {
	return m_bActive;
    }

    void SetActive(bool bState = true)
    {
	m_bActive = bState;
    }

    void CheckActive()
    {

	m_bActive = true;

	if (m_dZoomX == 1.0 && m_dZoomY == 1.0)
	{
	    m_bIsIdentity = true;
	    m_bIsInteger = true;

	    if (m_nSourceX == 0 &&
		m_nSourceY == 0)
		m_bActive = false;

	    return;
	} 
	else if (m_dZoomX >= 1.0 && m_dZoomY >= 1.0)
	{
	    if (fmod(m_dZoomX,1.0) < 0.0001 && 
		fmod(m_dZoomY,1.0) < 0.0001)
	    {
		m_bIsIdentity = false;
		m_bIsInteger = true;
		return;
	    }

	}

	m_bIsIdentity = false;
	m_bIsInteger = false;

    }


    void SetZoomX(const double dZoomX = 1.0)
    {
	m_dZoomX = dZoomX;
	CheckActive();
    }

    void SetZoomY(const double dZoomY = 1.0)
    {
	m_dZoomY = dZoomY;

	CheckActive();
    }

    void SetZoomXY(const double dZoomX = 1.0, const double dZoomY = 0.0)
    {
	SetZoomX(dZoomX);
	if (dZoomY == 0.0)
	    SetZoomY(dZoomX);
	else
	    SetZoomY(dZoomY);
    }

    double GetZoomX() const 
    {
	return m_dZoomX;
    }

    double GetZoomY() const 
    {
	return m_dZoomY;
    }

    void SetSourceX(const long nSourceX    = 0)
    {
	m_nSourceX = nSourceX;
	CheckActive();

    }

    void SetSourceY(const long nSourceY    = 0)
    {
	m_nSourceY = nSourceY;
	CheckActive();
    }

    void SetSourceXY(const long nSourceX    = 0, const long nSourceY = 0)
    {
	SetSourceX(nSourceX);
	SetSourceY(nSourceY);
    }

    int GetSourceX() const
    {
	return m_nSourceX;
    }

    int GetSourceY() const
    {
	return m_nSourceY;
    }

    void SetZoom(const int nSourceX	= 0, const int nSourceY = 0,
	const double dZoomX = 1.0, const double dZoomY = 0.0)

    {
	SetZoomXY(dZoomX, dZoomY);
	SetSourceXY(nSourceX, nSourceY);
    }


    void GetZoomValues(int &nSourceX, int &nSourceY, double &dZoomX, double &dZoomY)
    {
	nSourceX = GetSourceX();
	nSourceY = GetSourceY();
	dZoomX = GetZoomX();
	dZoomY = GetZoomY();
    }

    bool IsIdentity()
    {

	return m_bIsIdentity;
    }

    bool IsInteger()
    {
	return m_bIsInteger;
    }

    void ScalePoint(int x, int y, double &resultx, double& resulty)
    {
	resultx = x;
	resulty = y;

	resultx -= GetSourceX();
	resulty -= GetSourceY();

	resultx *= GetZoomX();
	resulty *= GetZoomY();

    }
};

#endif 
