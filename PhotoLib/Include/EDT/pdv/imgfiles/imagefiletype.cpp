// ImageFile.cpp: implementation of the CImageFile class.
//
//////////////////////////////////////////////////////////////////////

#include "edtimage/EdtImage.h"
#include "imagefiletype.h"

#include <sys/stat.h>

#ifdef WIN32
#include <io.h>
#endif

#include <fcntl.h>

#include "dispatch/ErrorHandler.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CImageFileType::CImageFileType()
{
	m_nHdrOffset = m_nWidth = m_nHeight = m_nSeqNumber = m_nSeqOffset = 
		m_nSeqGap = m_nImageSize = m_nFileMode = 0 ;
	m_nType = TypeBYTE;
	m_pFile = (FileHandle) 0;

	m_szFileName = NULL;
	m_szFileBase = NULL;

	m_bAsynch = m_bReadStarted = m_bWriteStarted = FALSE;

#ifdef WIN32

	memset(&m_Overlapped, 0, sizeof(m_Overlapped));

#endif

	// Caching for writing multiple images at one
	m_nBlockCache = 1;
	m_nBlocksActive  = 0;
	m_pBlockStart = NULL;
	m_nBlockSize = 0;
	m_nFirstBlock = 0;
	
	// Caching to read multiple images at once

	m_nTotalBuffers = 1;
	m_pBlockBase = NULL;
//	m_nCurrentBlock = 0;

}

CImageFileType::~CImageFileType()
{
	if (m_szFileName)
		free(m_szFileName);
}
	// Returns TRUE if this module can load from a file

const char * CImageFileType::GetName() 
{
	return "Dummy";
}

const char * CImageFileType::GetExtension() 
{
	return "Dummy";
}

int CImageFileType::CanLoad(const char * szFileName)

{
	return 0;
}

int CImageFileType::GetImageInfo(const char *szFileName, 
	int &nWidth, int &nHeight, EdtDataType &nType)

{
	return -1;
}

int CImageFileType::LoadEdtImage( const char *szFileName, 
	EdtImage *pImage)

{
	return -1;
}

int CImageFileType::LoadEdtImage( const char *szFileName, 
	EdtImage *pImage, const int nWidth, const int nHeight, const int nType, const int nSize)

{
	return -1;
}

int CImageFileType::CanSave(const char *szFileName,
		EdtImage *pImage)

{
	return -1;
}

int CImageFileType::SaveImage(const char *szFileName,
		EdtImage *pImage)

{
	return -1;
}



int CImageFileType::LoadIndexedImage(EdtImage *pImage, int nIndex)

{
	return -1;
}

//
// Open a file for either read or write
// return -1 on failure
//
int CImageFileType::OpenImageFile(const char *szFileName, 
								const int nFileMode,
								const int nWidth, 
								const int nHeight, 
								const EdtDataType nType,
								const int nSize,
								const bool bFailOk)

{

	int ok = 0;

	ASSERT(szFileName != NULL);

	if (m_pFile)
	{
		CloseImageFile();
	}

	char modestr[3];

	ASSERT(nFileMode == 'r' || nFileMode == 'w' || nFileMode == 'a');

	modestr[0] = nFileMode;
	modestr[1] = 'b';
	modestr[2] = '\0';

		if (nWidth != 0 && nHeight != 0 && nType != 0)
		{
			SetImageValues(nType, nWidth, nHeight, nSize);
		}
#ifdef WIN32

	int rwmode;
	if (nFileMode == 'r')
		rwmode = GENERIC_READ;
	else
		rwmode = GENERIC_WRITE;

	int createmode;
	if (nFileMode == 'a')
		createmode = OPEN_ALWAYS;
	else if (nFileMode == 'w')
		createmode = CREATE_ALWAYS;
	else
		createmode = OPEN_EXISTING;


	int flags = FILE_FLAG_NO_BUFFERING;

	if (m_nImageSize && 
	    (m_nImageSize % SECTOR_SIZE))
	    flags = 0;

	if (m_bAsynch)
		flags |= FILE_FLAG_OVERLAPPED; 

	m_pFile = CreateFile(szFileName,
							rwmode,
							FILE_SHARE_READ,
							NULL,
							createmode,
							flags,
							NULL
							);

	ok = (m_pFile != (void *) 0xffffffff);

	if (!ok)
	{
		if (!bFailOk)
			SysError.SysErrorMessage("Unable to open [%s] for access type [%c]",
				szFileName, nFileMode);
		m_pFile = NULL;

	}
#else
	int rwmode;

	if (nFileMode == 'r')
		rwmode = O_RDONLY;
	else
		rwmode = O_WRONLY;

	if (nFileMode == 'a')
		rwmode |= O_APPEND;
	else if (nFileMode == 'w')
		rwmode |= O_CREAT;
 
	m_pFile = open(szFileName,rwmode, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);

	ok = (m_pFile > 0);

#endif
	if (ok) {

	
		m_nFileMode = nFileMode;

		m_szFileName = strdup(szFileName);

		if (nFileMode == 'a')
			m_nSeqNumber = GetNImages()-1;
		else
			m_nSeqNumber = -1;

		m_nFirstBlock = -1;

		return 0;
	}

	return -1;

}

void CImageFileType::CloseImageFile()

{
	TRACE("Closing: %x", this);
	if (m_szFileName)
		TRACE(" %s", m_szFileName);
	else if (!m_pFile)
		TRACE(" Already Closed");
	
	TRACE("\n");

	if (m_pFile)
	{

		Flush();

		if (m_bWriteStarted)
			Flush();

#ifdef WIN32
		CloseHandle(m_pFile);
#else
		close(m_pFile);
#endif
		m_pFile = (FileHandle) 0;
	}

	if (m_szFileName)
	{
		free(m_szFileName);
		m_szFileName = NULL;
	}

	m_nSeqNumber = -1;

	m_nFirstBlock = -1;

}

int CImageFileType::LoadNextImage(EdtImage *pImage)

{
	return -1;
}

int CImageFileType::SaveNextImage(EdtImage *pImage)

{
	return -1;
}

int CImageFileType::GetNImages()
{
	
	if (m_nSeqOffset && m_pFile)
	{

#ifdef WIN32

		LARGE_INTEGER fullsize;

		fullsize.LowPart = GetFileSize(m_pFile, (LPDWORD) &fullsize.HighPart);

		fullsize.QuadPart -= m_nHdrOffset;
		
		return (int) (fullsize.QuadPart / ((LONGLONG) m_nSeqOffset));
#else
		uint fpos = lseek(m_pFile,0,SEEK_CUR);
		uint filesize = lseek(m_pFile,0,SEEK_END);
		lseek(m_pFile,fpos,SEEK_SET);

		return (int) (filesize/m_nSeqOffset);

#endif
		  
	}
	
	return 0;

}

bool CImageFileType::IsOpen()
{
	return (m_pFile != (FileHandle) 0);
}

void CImageFileType::SetAsynch(bool bState)

{
	if (m_bAsynch != bState)

	{
		if (m_bAsynch)
		{

		}
		else
		{
			// turn on asynch
#ifdef   WIN32
			if (!m_Overlapped.hEvent)
				m_Overlapped.hEvent = CreateEvent(NULL,TRUE,TRUE,NULL);
#endif

		}

		m_bAsynch = bState;

	}
	else
	{

	}
}

void CImageFileType::ReadImageFile(HANDLE pFile, void *pBuffer, uint bytestoread, uint * bytesread)
{
#ifdef WIN32
	if (m_bAsynch)
	{
		if (!m_bReadStarted)
		{
			TRACE("Starting read %g\n",
				edt_timestamp());

			StartReadImageFile(pFile, pBuffer, bytestoread);
		}

		// wait for completion

		TRACE("Starting Get Overlapped %g\n",
				edt_timestamp());
		
		GetOverlappedResult(pFile, &m_Overlapped, (LPDWORD)bytesread, TRUE);

		TRACE("Back From Get Overlapped %g\n",
				edt_timestamp());
	
	}
	else
		ReadFile(pFile, pBuffer, bytestoread, (LPDWORD)bytesread, NULL);

#else

	int rc = read(pFile,pBuffer,bytestoread);
	if (bytesread)
	  *bytesread = rc;

#endif

	m_bReadStarted = FALSE;
}

void CImageFileType::StartReadImageFile(HANDLE pFile, void *pBuffer, uint    bytestoread)
{

#ifdef WIN32
	if (m_bAsynch)
	{

		// Set the file position in overlapped

		ReadFile(pFile,pBuffer, bytestoread, NULL, &m_Overlapped);

		m_bReadStarted = TRUE;
	}
#endif

}

int CImageFileType::StartLoadIndexedImage(EdtImage *pImage, int nIndex)
{

	return -1;

}

void CImageFileType::WriteImageFile(HANDLE pFile, void *pBuffer, uint bytestoWrite, uint * byteswritten)
{
#ifdef WIN32
	if (m_bAsynch)
	{
		if (!m_bWriteStarted)
		{
			StartWriteImageFile(pFile, pBuffer, bytestoWrite);
		}

		// wait for completion

		GetOverlappedResult(pFile, &m_Overlapped, (LPDWORD)byteswritten, TRUE);
	}
	else
		WriteFile(pFile, pBuffer, bytestoWrite, (LPDWORD)byteswritten, NULL);

#else

	int rc = write(pFile,pBuffer,bytestoWrite);
	if (byteswritten)
	  *byteswritten = rc;

#endif

	m_bWriteStarted = FALSE;
}

void CImageFileType::StartWriteImageFile(HANDLE pFile, void *pBuffer, uint    bytestowrite)
{

#ifdef WIN32
	if (m_bAsynch)
	{

		// Set the file position in overlapped

		WriteFile(pFile,pBuffer, bytestowrite, NULL, &m_Overlapped);

		m_bWriteStarted = TRUE;
	}
#endif

}

int CImageFileType::StartSaveIndexedImage(EdtImage *pImage, int nIndex)
{

	return -1;

}

bool CImageFileType::IsNextInBlock(void *pNewData)
{
	if (m_pBlockStart)
	{
		if ((byte *)pNewData - (byte *)m_pBlockStart == m_nBlockSize)
			return TRUE;
	}

	return FALSE;

}

void CImageFileType::SetBlockBase(void *pBase, int nTotal)
{
	m_pBlockBase = (u_char *) pBase;
	m_nTotalBuffers = nTotal;
}

bool CImageFileType::ImageLoaded(const int nIndex, EdtImage *pImage)

{
	if (m_nFirstBlock < 0)
	{
		return FALSE;
	}
	
	int delta = nIndex - m_nFirstBlock;

	if (delta >= m_nBlockCache || delta < 0)
	{
		return FALSE;
	}

	if (pImage)
	{

		u_char *pData = (u_char *) pImage->GetBaseData();
		u_char *pBufferData = (u_char *) m_pBlockStart + (delta * m_nImageSize);

		if (pData != pBufferData)
		{
			return FALSE;
		}
	}

	return TRUE;

}

u_char * CImageFileType::GetBlockStart(const int nIndex)
{
	if (m_nTotalBuffers && m_pBlockBase)
		return (u_char *) m_pBlockBase + (nIndex % m_nTotalBuffers) * m_nImageSize;
	else
		return NULL;
}
