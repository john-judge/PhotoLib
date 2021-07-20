// ImageFile.h: interface for the CImageFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IMAGEFILE_H__4B460FC4_2D69_11D2_97DC_00104B357776__INCLUDED_)
#define AFX_IMAGEFILE_H__4B460FC4_2D69_11D2_97DC_00104B357776__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// Base class for different image file types
// Keep a list of available image file types 
// 

class CImageFile  
{
public:
	CImageFile();
	virtual ~CImageFile();

	// Returns TRUE if this module can load from a file

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


#endif // !defined(AFX_IMAGEFILE_H__4B460FC4_2D69_11D2_97DC_00104B357776__INCLUDED_)
