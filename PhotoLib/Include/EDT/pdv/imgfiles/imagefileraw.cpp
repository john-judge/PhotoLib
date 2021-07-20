// ImageFileRaw.cpp: implementation of the CImageFileRaw class.
//
//////////////////////////////////////////////////////////////////////

#include "stdio.h"
#include "fcntl.h"
#include "edtimage/EdtImage.h"
#include "imagefileraw.h"
// #include "sys/types.h"

#include "dispatch/ErrorHandler.h"

#define WIDTHBYTES(bits) (((bits) + 31) / 32 * 4)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CImageFileRaw::CImageFileRaw()
{
    m_bInverted = m_bInterlaced = m_bPureRaw = FALSE;
}

CImageFileRaw::~CImageFileRaw()
{

}

const char * CImageFileRaw::GetName() 
{
    return "Raw Image File";
}

const char * CImageFileRaw::GetExtension() 
{
    return "raw";
}

int CImageFileRaw::CanLoad(const char * szFileName)

{
    // check if file exists

    long nWidth,
	nHeight;

    EdtDataType nType;


    if (HaveValues() && 
	GetImageInfo(szFileName,nWidth,nHeight,nType) != -1)
    {
	// check that there's enough data

	if (GetNImages() > 0)
	    return 1;

    }

    return 0;

}

int CImageFileRaw::GetImageInfo(const char *szFileName, 
				long &nWidth, long &nHeight, EdtDataType &nType)

{
    // Make sure we can read it

    FILE *f;
    BOOL bIsPM = FALSE;

    f = fopen(szFileName,"r");

    if (!f)
    {
	return -1;
    }

    fclose(f);

    return 0;

}


int CImageFileRaw::LoadEdtImage( const char *szFileName, 
			     EdtImage *pImage)

{

    int rc = -1;
    // Make sure we can read it
    ASSERT(m_nWidth > 0 && m_nHeight > 0 && m_nType > 0);

    if (m_nWidth > 0 && m_nHeight > 0 && m_nType > 0) 
    {

	OpenImageFile(szFileName, 'r');

	if (m_pFile)
	{

	    rc = LoadIndexedImage(pImage, 0);

	    CloseImageFile();
	}

    }

    return rc;
}


int CImageFileRaw::LoadEdtImage( const char *szFileName, 
			     EdtImage *pImage, const int width, const int height, const EdtDataType type, const int size)

{

    SetImageValues(type, width, height, size);

    return LoadEdtImage(szFileName, pImage);


}


int CImageFileRaw::CanSave(const char *szFileName,
			   EdtImage *pImage)

{
    ASSERT(szFileName);
    ASSERT(pImage);

    if (pImage->IsAllocated())

	return 1;
    else

	return 0;
}

int CImageFileRaw::SaveImage(const char *szFileName,
			     EdtImage *pImage)

{
    // Make sure we can save it

    if (CanSave(szFileName, pImage))
    {

	if (m_bPureRaw) // Ignore timestamp, etc.
	    SetImageValues(pImage->GetType(), pImage->GetWidth(), pImage->GetHeight(), 0);
	else
	    SetImageValues(pImage);

	OpenImageFile(szFileName, 'w');

	if (m_pFile)
	{
	    uint rc;
#ifdef WIN32

	    WriteFile(m_pFile,
		pImage->GetBaseData(), 
		m_nImageSize, 
		(LPDWORD) &rc,
		NULL);
#else

	    rc = write(m_pFile, pImage->GetBaseData(),
		m_nImageSize);
#endif

	    CloseImageFile();

	    if (rc == m_nImageSize)

	    {
		return 0;

	    }
	    else
	    {
		SysError.SysErrorMessage("Error Writing to file %s",
		    szFileName);
	    }


	}

    }

    return -1;


}

// Start reading a raw image 
// Can start an asynchronous read
//

int CImageFileRaw::StartLoadIndexedImage(EdtImage *pImage, int nIndex)
{
    if (!m_bReadStarted)
    {

	{
	    pImage->Create( m_nType, m_nWidth, m_nHeight);

	    int rc = SeekImageFrame(nIndex);

	    // Only start the read if it's a full buffer

	    if (rc == 0 && m_bAsynch)
	    {
		if (pImage->IsContiguous() && !(m_bInverted || m_bInterlaced))
		    StartReadImageFile(m_pFile,pImage->GetBaseData(), m_nImageSize);
	    }

	    return rc;

	}
    }

    return 0;
}

// Start reading a raw image 
// Can start an asynchronous read
//

int CImageFileRaw::StartSaveIndexedImage(EdtImage *pImage, int nIndex)
{
    if (!m_bWriteStarted)
    {

	int rc = SeekImageFrame(nIndex);

	// Only start the read if it's a full buffer

	if (rc == 0 && m_bAsynch)
	{
	    if (pImage->IsContiguous() && !(m_bInverted || m_bInterlaced))
		StartWriteImageFile(m_pFile,pImage->GetBaseData(), m_nImageSize);
	}

	return rc;
    }

    return 0;
}



int CImageFileRaw::SaveIndexedImage(EdtImage *pImage, int nIndex)

{

    ASSERT (pImage);

    uint nbytes;

    if (m_nBlockCache > 1)
    {

	// First check if there's an active block
	if (IsBlockActive())
	{
	    if (IsNextInBlock(pImage->GetBaseData()))
	    {
		m_nBlockSize += m_nImageSize;
		m_nBlocksActive ++;

		if (m_nBlocksActive >= m_nBlockCache)
		    Flush();

		return 0;
	    }
	    else
		Flush();

	}

	if (pImage->IsContiguous() && !(m_bInverted || m_bInterlaced))
	{
	    m_nBlockSize = m_nImageSize;
	    m_nBlocksActive = 1;
	    m_pBlockStart = pImage->GetBaseData();
	    m_nFirstBlock = nIndex;

	    // Don't do any writing yet

	    return 0;
	}

    }

    if (StartSaveIndexedImage(pImage, nIndex) == 0)
    {

	// Check if contiguous data 

	if (pImage->IsContiguous() && !(m_bInverted || m_bInterlaced))
	{
	    WriteImageFile(m_pFile,pImage->GetBaseData(), m_nImageSize,
		&nbytes);

#if 0			
	    if (pImage->GetHeaderSize())
	    {
		u_short *ps = (u_short *) pImage->GetHeaderData();
		if (ps)
		{
		    int k;
		    for (k=0;k<8;k++)
		    {
			TRACE("%04x ", ps[k]);
		    }
		    TRACE("\n");

		}
	    }

	    TRACE("Writing %d bytes\n", m_nImageSize);
#endif

	    if (nbytes != m_nImageSize)
		return -1;
	}
	else
	{
	    byte **pRows = (byte **) pImage->GetPixelRows();
	    int row;

	    unsigned int nTargetSize = 
		(int) (pImage->GetPitch() *
		pImage->GetPixelSize());


	    if (m_bInverted)
	    {
		if (m_bInterlaced)
		{

		    for (row = m_nHeight - 1;row >= 0;row -= 2)
		    {

			WriteImageFile(m_pFile,pImage->GetBaseData(), m_nImageSize,
			    &nbytes);

			if (nbytes != nTargetSize)
			    return -1;

		    }
		    for (row = m_nHeight-2;row >= 0;row -= 2)
		    {

			WriteImageFile(m_pFile,pImage->GetBaseData(), m_nImageSize,
			    &nbytes);

			if (nbytes != nTargetSize)
			    return -1;

		    }


		}
		else
		{

		    for (row = m_nHeight - 1;row >= 0;row --)
		    {

			WriteImageFile(m_pFile,pImage->GetBaseData(), m_nImageSize,
			    &nbytes);

			if (nbytes != nTargetSize)
			    return -1;

		    }
		}
	    } 
	    else 
	    {
		if (m_bInterlaced)
		{

		    for (row = 0;row < m_nHeight;row += 2)
		    {

			WriteImageFile(m_pFile,pImage->GetBaseData(), m_nImageSize,
			    &nbytes);

			if (nbytes != nTargetSize)
			    return -1;

		    }
		    for (row = 1;row < m_nHeight;row += 2)
		    {

			WriteImageFile(m_pFile,pImage->GetBaseData(), m_nImageSize,
			    &nbytes);

			if (nbytes != nTargetSize)
			    return -1;

		    }
		}
		else
		{

		    for (row = 0;row < m_nHeight;row ++)
		    {

			WriteImageFile(m_pFile,pImage->GetBaseData(), m_nImageSize,
			    &nbytes);

			if (nbytes != nTargetSize)
			    return -1;

		    }
		}

	    }


	}

	return 0;
    }

    return -1;

}

void CImageFileRaw::Flush()

{
    uint byteswritten = 0;

    if (m_pFile)
    {

#ifdef WIN32
	if (m_bAsynch && m_bWriteStarted)
	{

	    GetOverlappedResult(m_pFile, &m_Overlapped, (LPDWORD) &byteswritten, TRUE);

	}
#endif
	if (m_pBlockStart)
	{


	    //rc = SeekImageFrame(m_nFirstBlock);
	    uint nbytes = 0;


	    if (m_bAsynch)
	    {
		StartWriteImageFile(m_pFile,m_pBlockStart, m_nBlockSize);
	    }
	    else
	    {
		WriteImageFile(m_pFile,m_pBlockStart, m_nBlockSize,
		    &nbytes);

	    }

#if 0
	    TRACE("Writing %d bytes at %d from %p returns %d\n",
		m_nBlockSize, m_nFirstBlock, m_pBlockStart, nbytes);
#endif

	    m_nSeqNumber += m_nBlocksActive;

	    m_nFirstBlock = 0;

	    m_pBlockStart = NULL;
	    m_nBlockSize = 0;
	    m_nBlocksActive = 0;


	}
    }
}

int CImageFileRaw::LoadIndexedImage(EdtImage *pImage, int nIndex)

{

    ASSERT (pImage);

    uint nbytes;

    // Check for cached block
    if (m_nBlockCache > 1 && m_pBlockBase)
    {
	if (ImageLoaded(nIndex, pImage))
	{
	    //TRACE("Image %d already loaded\n", nIndex);

	    m_nSeqNumber = nIndex;

	    return 0;
	}

	if (pImage->IsContiguous() && !(m_bInverted || m_bInterlaced))
	{

	    int rc = SeekImageFrame(nIndex);

	    if (rc == 0)
	    {

		m_nFirstBlock = m_nSeqNumber;

		m_pBlockStart = GetBlockStart(nIndex);

		if (m_pBlockStart)
		{

		    m_nBlockSize = m_nBlockCache * m_nImageSize;

		    ReadImageFile(m_pFile,m_pBlockStart, m_nBlockSize,
			&nbytes);

		    //TRACE("Reading %d bytes \n", nbytes);

		    if (nbytes != (u_int) m_nBlockSize)
		    {
			SysError.SysErrorMessage("Error reading cache: image %d",nIndex);
			return -1;
		    }

		    return 0;

		}
		else
		{
		    SysError.SysErrorMessage("Error getting cache pointer: image %d",nIndex);
		    return -1;
		}
	    }

	}	
    }

    if (StartLoadIndexedImage(pImage, nIndex) == 0)
    {

	// Check if contiguous data 

	if (pImage->IsContiguous() && !(m_bInverted || m_bInterlaced))
	{


	    ReadImageFile(m_pFile,pImage->GetBaseData(), m_nImageSize,
		&nbytes);

	    if (nbytes != m_nImageSize)
	    {
		SysError.SysErrorMessage("Error reading image %d",nIndex);
		return -1;
	    }
	}
	else
	{
	    byte **pRows = (byte **) pImage->GetPixelRows();
	    int row;

	    unsigned int nTargetSize = (int) (pImage->GetPitch() *
		pImage->GetPixelSize());


	    if (m_bInverted)
	    {
		if (m_bInterlaced)
		{

		    for (row = m_nHeight - 1;row >= 0;row -= 2)
		    {

			ReadImageFile(m_pFile,pImage->GetBaseData(), m_nImageSize,
			    &nbytes);

			if (nbytes != nTargetSize)
			    return -1;

		    }
		    for (row = m_nHeight-2;row >= 0;row -= 2)
		    {

			ReadImageFile(m_pFile,pImage->GetBaseData(), m_nImageSize,
			    &nbytes);

			if (nbytes != nTargetSize)
			    return -1;

		    }


		}
		else
		{

		    for (row = m_nHeight - 1;row >= 0;row --)
		    {

			ReadImageFile(m_pFile,pImage->GetBaseData(), m_nImageSize,
			    &nbytes);

			if (nbytes != nTargetSize)
			    return -1;

		    }
		}
	    } 
	    else 
	    {
		if (m_bInterlaced)
		{

		    for (row = 0;row < m_nHeight;row += 2)
		    {

			ReadImageFile(m_pFile,pImage->GetBaseData(), m_nImageSize,
			    &nbytes);

			if (nbytes != nTargetSize)
			    return -1;

		    }
		    for (row = 1;row < m_nHeight;row += 2)
		    {

			ReadImageFile(m_pFile,pImage->GetBaseData(), m_nImageSize,
			    &nbytes);

			if (nbytes != nTargetSize)
			    return -1;

		    }
		}
		else
		{

		    for (row = 0;row < m_nHeight;row ++)
		    {

			ReadImageFile(m_pFile,pImage->GetBaseData(), m_nImageSize,
			    &nbytes);

			if (nbytes != nTargetSize)
			    return -1;

		    }
		}

	    }


	}

	pImage->SetDataChanged();
	return 0;
    }

    return -1;
}

int CImageFileRaw::LoadNextImage(EdtImage *pImage)

{
    ASSERT(pImage);
    ASSERT(HaveValues());

    if (HaveValues() && IsOpen() && m_nFileMode == 'r')
    {
	return LoadIndexedImage(pImage, m_nSeqNumber+1);
    }

    return -1;
}

int CImageFileRaw::SaveNextImage(EdtImage *pImage)

{
    ASSERT(pImage);
    ASSERT(HaveValues());

    if (HaveValues() && IsOpen() && (m_nFileMode == 'a' ||m_nFileMode == 'w') )
    {
	return SaveIndexedImage(pImage, m_nSeqNumber+1);
    }

    return -1;
}

#ifndef WIN32

#define LONGLONG long long

#endif

int CImageFileRaw::SeekImageFrame(const int nFrame)
{

    int nActualFrame = nFrame;

    if (!IsOpen())
    {
	return -1;
    }

    ASSERT(IsOpen());

    if (m_pBlockBase && m_nBlockCache > 1 && m_nFirstBlock >= 0)
    {
	int offset = nFrame - m_nFirstBlock;

	if (offset < m_nBlockCache)
	{
	    m_nSeqNumber = nFrame;
	    //TRACE("Setting frame to %d\n", m_nSeqNumber);

	    return 0;
	}

    }

    if (m_nBlockCache > 1)
    {
	nActualFrame = (nActualFrame / m_nBlockCache) * m_nBlockCache;

	//TRACE("Seek: setting actual frame from %d to %d\n",
	//		nActualFrame, nFrame);

    }

    // return right away if we're already there

    //	if (nFrame == m_nSeqNumber)
    //		return 0;

    //	TRACE ("Seek %d\n",nFrame);


#ifdef WIN32

    LARGE_INTEGER fullsize;


    fullsize.QuadPart = (LONGLONG) nActualFrame * ((LONGLONG) m_nSeqOffset) + (LONGLONG) m_nHdrOffset;

    //TRACE("Seek : %d * (%d + %d) = %I64d\n", nActualFrame, m_nSeqOffset, m_nHdrOffset,
    //		fullsize.QuadPart);

    if (m_bAsynch)
    {
	m_nCurrentPosition = fullsize.QuadPart;
	m_nSeqNumber = nActualFrame;

	m_Overlapped.Offset = fullsize.LowPart;
	m_Overlapped.OffsetHigh = fullsize.HighPart;

	return 0;
    }
    else
    {


	//	TRACE("Seeking %I64d\n", fullsize.QuadPart);

	uint rc = SetFilePointer(m_pFile, 
	    fullsize.LowPart, &fullsize.HighPart, SEEK_SET);


	if (rc == fullsize.LowPart)
	{
	    m_nSeqNumber = nActualFrame;
	    return 0;
	}
	else
	    return -1;
    }

#else

    long long fullsize;

    fullsize = (LONGLONG) nActualFrame * 
	((LONGLONG) m_nSeqOffset) + (LONGLONG) m_nHdrOffset;

    return lseek64(m_pFile,fullsize,SEEK_SET);

#endif


}

/* Global Single Module */

CImageFileRaw theRawFileType;

EDTAPI
CImageFileType *pRawFileType = &theRawFileType;


