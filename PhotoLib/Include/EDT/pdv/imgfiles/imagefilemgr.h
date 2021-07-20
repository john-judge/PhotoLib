// ImageFileMgr.h: interface for the CImageFileMgr class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_EDT_IMAGE_FILE_MGR_H)
#define _EDT_IMAGE_FILE_MGR_H

#include "imagefiletype.h"

// Interface class to load and save images to named image types
// Actual data is kept in a static private class

class CImageFileMgr  
{
public:
    void AddImageFileType(CImageFileType *pFT);
    CImageFileMgr();
    virtual ~CImageFileMgr();

    CImageFileType *LookupImageType(const char *szTypeName);

    CImageFileType *CanSave(const char * szFileName, EdtImage *pImage, 
	const char *szTypeName = NULL);

    bool CanSave(const char *szTypeName);

    int SaveImage(const char *szFileName, EdtImage *pImage, const char *szTypeName = NULL);

    CImageFileType *GetFileImageType(const char *szFileName);

    EdtImage * LoadEdtImage(const char *szFileName, EdtImage *pImage);

    void SetPureRaw(BOOL bState);

};

// This is a singleton - only one of these can be defined

extern CImageFileMgr EdtImageFileMgr;

#endif 
