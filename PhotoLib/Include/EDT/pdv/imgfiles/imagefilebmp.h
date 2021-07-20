// ImageFileBMP.h: interface for the CImageFileBMP class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IMAGEFILEBMP_H__4B460FC5_2D69_11D2_97DC_00104B357776__INCLUDED_)
#define AFX_IMAGEFILEBMP_H__4B460FC5_2D69_11D2_97DC_00104B357776__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "imagefiletype.h"


class CImageFileBMP : public CImageFileType  
{
public:
	CImageFileBMP();
	virtual ~CImageFileBMP();

	virtual const char * GetExtension();

	virtual const char * GetName();

	virtual int CanLoad(const char * szFileName);

	virtual int GetImageInfo(const char *szFileName, 
		long &nWidth, long &nHeight, EdtDataType &nType);

	virtual int LoadEdtImage( const char *szFileName, 
		EdtImage *pImage);

	virtual int CanSave(const char *szFileName,
			EdtImage *pImage);

	virtual int SaveImage(const char *szFileName,
			EdtImage *pImage);

};

#endif // !defined(AFX_IMAGEFILEBMP_H__4B460FC5_2D69_11D2_97DC_00104B357776__INCLUDED_)
