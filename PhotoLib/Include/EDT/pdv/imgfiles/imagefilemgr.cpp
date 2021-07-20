// ImageFileMgr.cpp: implementation of the CImageFileMgr class.
//
//////////////////////////////////////////////////////////////////////

#include "edtimage/EdtImage.h"

#include "imagefilemgr.h"

#include "dispatch/ErrorHandler.h"


#define MAX_IMAGE_FILE_TYPES 32

class CImageFileData {

public:

	CImageFileType *m_pFileTypes[MAX_IMAGE_FILE_TYPES];
	int m_nFileTypes;

	void AddFileType(CImageFileType *pFT)
	{
		if (m_nFileTypes < MAX_IMAGE_FILE_TYPES)
		{
			m_pFileTypes[m_nFileTypes] = pFT;
			m_nFileTypes ++;
		}
	}

	CImageFileData()
	{
		m_nFileTypes = 0;
	}
}
theFileTypeList;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern CImageFileType *pBMPFileType;
extern CImageFileType *pRawFileType;

#ifndef NO_TIFF
extern CImageFileType *pTiffFileType;
#endif

#ifndef NO_JPEG
extern CImageFileType *pJPEGFileType;
#endif

CImageFileMgr::CImageFileMgr()
{
	static BOOL First = TRUE;
	
	if (First)
	{
		theFileTypeList.AddFileType(pBMPFileType);
		theFileTypeList.AddFileType(pRawFileType);

#ifndef NO_TIFF
		theFileTypeList.AddFileType(pTiffFileType);
#endif

#ifndef NO_JPEG
		theFileTypeList.AddFileType(pJPEGFileType);
#endif

		First = FALSE;
	}
}


CImageFileMgr::~CImageFileMgr()
{

}

CImageFileType *CImageFileMgr::LookupImageType(const char *szTypeName)

{
	for (int i=0;i< theFileTypeList.m_nFileTypes;i++)
	{
		CImageFileType * pFT=theFileTypeList.m_pFileTypes[i];

		if (!strcmp(szTypeName, pFT->GetExtension()))
		{
			return pFT;
		}
	}

	return NULL;
}

CImageFileType *CImageFileMgr::CanSave(const char * szFileName, EdtImage *pImage, const char *szTypeName)

{
	CImageFileType * pFT;

	if (szTypeName && (pFT = LookupImageType(szTypeName)) && pFT->CanSave(szFileName,pImage))
	{
		return pFT;
	}
#if 0 // fallback save (but not really worthwhile)
	else
	{

		for (int i=0;i< theFileTypeList.m_nFileTypes;i++)
		{
			pFT= theFileTypeList.m_pFileTypes[i];

			if (pFT->CanSave(szFileName,pImage))
			{
				return pFT;
			}

		}
	}
#endif

	return NULL;
}

bool CImageFileMgr::CanSave(const char *szTypeName)

{
	if (LookupImageType(szTypeName))
        return true;
	return false;
}


int CImageFileMgr::SaveImage(const char *szFileName, EdtImage *pImage, const char *szTypeName)

{
	CImageFileType * pFT;
	if (!(szFileName && pImage && szTypeName))
	{
		return -1;

	}

	if (pFT = CanSave(szFileName,pImage,szTypeName))
	{

		return pFT->SaveImage(szFileName, pImage);

	}


//	SysError.SysErrorMessage("Unable to save image %s (%d x %d x %d) of file type <%s>", szFileName, 
//		pImage->GetWidth(),pImage->GetHeight(),pImage->GetPixelSize() * 8, szTypeName);


	return -1;
}

CImageFileType *CImageFileMgr::GetFileImageType(const char *szFileName)

{	
	
	for (int i=0;i< theFileTypeList.m_nFileTypes;i++)
	{
		CImageFileType * pFT=theFileTypeList.m_pFileTypes[i];

		if (pFT->CanLoad(szFileName))
		{
			return pFT;
		}
	}
	return NULL;
}


EdtImage * CImageFileMgr::LoadEdtImage(const char *szFileName, EdtImage *pImage)

{
	CImageFileType *pFT = GetFileImageType(szFileName);

	BOOL bAllocated = FALSE;

	if (pFT)
	{
		// we have a file type, now check the image
		
		if (!pImage)
		{
			int nWidth, nHeight;
			EdtDataType nType;

			pFT->GetImageInfo(szFileName, nWidth, nHeight, nType);

			pImage = new EdtImage;

			pImage->Create(nType, nWidth,nHeight);

			bAllocated = TRUE;
		
		}

		// Attempt to load with the file type

		if (pFT->LoadEdtImage(szFileName,pImage) == 0)
		{
			return pImage;			
		}
		else
		{
			if (bAllocated)
				delete pImage;		
		}
	}

	return NULL;

}


void CImageFileMgr::AddImageFileType(CImageFileType * pFT)
{
	theFileTypeList.AddFileType(pFT);
}

CImageFileMgr EdtImageFileMgr;

#include "imagefileraw.h"

extern CImageFileRaw theRawFileType;

void CImageFileMgr::SetPureRaw(BOOL state)
{
	theRawFileType.m_bPureRaw = state;	
}
