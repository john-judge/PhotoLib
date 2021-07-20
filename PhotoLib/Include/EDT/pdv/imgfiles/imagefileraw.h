// ImageFileRaw.h: interface for the CImageFileRaw class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IMAGEFILERAW_H__)
#define AFX_IMAGEFILERAW_H__

#include "imagefiletype.h"

class CImageFileRaw : public CImageFileType  
{
public:
	BOOL m_bPureRaw;
	CImageFileRaw();
	virtual ~CImageFileRaw();

	virtual int StartLoadIndexedImage(EdtImage *pImage, int nIndex);
	virtual int StartSaveIndexedImage(EdtImage *pImage, int nIndex);

	virtual int SeekImageFrame(const int nFrame);

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

	virtual int LoadIndexedImage(EdtImage *pImage, int nIndex);

	virtual int SaveIndexedImage(EdtImage *pImage, int nIndex);

	virtual int LoadNextImage(EdtImage *pImage);

	virtual int SaveNextImage(EdtImage *pImage);

	int LoadEdtImage( const char *szFileName, 
			EdtImage *pImage, 
			const int width, 
			const int height, 
			const EdtDataType type, 
			const int size = 0);

	BOOL m_bInverted;
	BOOL m_bInterlaced;

	virtual void Flush();

};

#endif // !defined(AFX_IMAGEFILERaw_H__4B460FC5_2D69_11D2_97DC_00104B357776__INCLUDED_)
