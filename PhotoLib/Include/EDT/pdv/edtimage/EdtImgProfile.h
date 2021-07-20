// EdtImgProfile.h: interface for the EdtImgProfile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(EDTIMGPROFILE_H)
#define EDTIMGPROFILE_H

// Class to compute a line profile from an image
//

#include "edtinc.h"

#include "EdtRect.h"
#include "EdtImage.h"

class EdtImgProfile  
{
    int *m_pProfiles[EDT_MAX_COLOR_PLANES];
    EdtPoint m_Pt1, m_Pt2;

    int m_nPixels;
    bool m_bEndsSet;

    int m_nColors;

    EdtPoint *m_pOffsets;

public:
    void Destroy();

    void Compute(EdtImage *pSrc, const int x1, const int y1, const int x2, const int y2);
    void Compute(EdtImage *pSrc, const EdtPoint pt1, const EdtPoint pt2);
    void Compute(EdtImage *pSrc); // if no end points, use current ones

    void SetPoints(const int x1, const int y1, const int x2, const int y2);
    void SetPoints(const EdtPoint x1, const EdtPoint y1);

    bool GetPoints(EdtPoint &pt1, EdtPoint &pt2)
    {
	if (m_bEndsSet)
	{
	    pt1 = m_Pt1;
	    pt2 = m_Pt2;
	    return true;
	}
	else
	{
	    return false;
	}
    }

    void GetPointAtIndex(const int index, int &x, int &y, int &value, int band = 0);
    int GetPointValuesAtIndex(const int index, int &x, int &y, int *values);
    int GetNColors() const {return m_nColors;}

    int GetNPixels() const
    {
	return m_nPixels;
    }

    int * GetProfile(int band = 0);

    int **GetProfileArray()
    {
	return m_pProfiles;
    }

    EdtImgProfile();
    virtual ~EdtImgProfile();

    EdtPoint *GetOffsets()
    {
	return m_pOffsets;

    }
    bool Clipped(EdtPoint &pt, const int left, const int top, const int right, const int bottom);

};

#endif // !defined(AFX_EDTIMGPROFILE_H__918A6C23_1D04_11D2_8DE7_00A0C932D513__INCLUDED_)
