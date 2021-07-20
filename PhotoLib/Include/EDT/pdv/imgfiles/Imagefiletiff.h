//--------------------------------------------
// File:
// 
// Purpose:
//
// Date:
//
// Copyright 1999 Engineering Design Team
//---------------------------------------------

#ifndef C_TIFF_STORAGE_H
#define C_TIFF_STORAGE_H


#include "imagefiletype.h"

class CImageFileTiff : public CImageFileType {

public:

	CImageFileTiff();
	~CImageFileTiff();

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


#endif
