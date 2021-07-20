// ImageFile.h: interface for the CImageFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IMAGEFILE_H__4B460FC4_2D69_11D2_97DC_00104B357776__INCLUDED_)
#define AFX_IMAGEFILE_H__4B460FC4_2D69_11D2_97DC_00104B357776__INCLUDED_

// Base class for different image file types
// Keep a list of available image file types 
// 

extern "C" {
#include "edtinc.h"
}

#include "edtimage/EdtImage.h"

typedef HANDLE FileHandle;

class CImageFileType 
{
protected:
	int m_nWidth,
		m_nHeight,
		m_nHdrOffset,
		m_nSeqNumber,
		m_nSeqOffset,
		m_nSeqGap; /* defaults to image size - can be greater if 
					      there are gaps between images in file */

    EdtDataType m_nType;

	int m_nBlockCache; /* for queueing up sequential writes */
	int m_nBlocksActive;
	void *m_pBlockStart;
	int m_nBlockSize;
	int m_nFirstBlock;

	unsigned int m_nImageSize;
	FileHandle m_pFile;

	int m_nFileMode; /* if file currently open for read */
	bool m_bMultiFileSequence; /* if sequences are file.<indx>,
							   as opposed to a single file */

	char *m_szFileName; /* the current open image */
	char *m_szFileBase; /* the base name for multi-image sequences */


	EdtFileOffset m_nCurrentPosition; /* for asynch I/O */

public:
	u_char * GetBlockStart(const int nIndex);
	bool ImageLoaded(const int nIndex, EdtImage *pImage = NULL);
	int m_nTotalBuffers;
	u_char * m_pBlockBase;
	void SetBlockBase(void *pBase, int nTotal);
	bool IsNextInBlock(void *pNewData);
	virtual int StartLoadIndexedImage(EdtImage *pImage, int nIndex);
	virtual int StartSaveIndexedImage(EdtImage *pImage, int nIndex);

	virtual void ReadImageFile(HANDLE m_pFile, void *pBuffer, uint bytestoread,  uint * bytesread);
	virtual void StartReadImageFile(HANDLE m_pFile, void *pBuffer, uint bytestoread);

	virtual void WriteImageFile(HANDLE m_pFile, void *pBuffer, uint bytestowrite,  uint * byteswritten);
	virtual void StartWriteImageFile(HANDLE m_pFile, void *pBuffer, uint bytestowrite);

	bool m_bWriteStarted;
	bool m_bReadStarted;
	bool m_bAsynch;
	bool IsOpen();
	virtual int GetNImages();
	CImageFileType();
	virtual ~CImageFileType();

	// Returns TRUE if this module can load from a file

	virtual const char * GetExtension();

	virtual const char * GetName();

	virtual int CanLoad(const char * szFileName);

	virtual int GetImageInfo(const char *szFileName, 
		int &nWidth, int &nHeight, EdtDataType &nType);

	virtual int LoadEdtImage( const char *szFileName, 
		EdtImage *pImage);

	virtual int CanSave(const char *szFileName,
			EdtImage *pImage);

	virtual int SaveImage(const char *szFileName,
			EdtImage *pImage);

	void SetImageValues(const EdtDataType nType, const int nWidth, const int nHeight, const int nSize)

	{
		m_nWidth = nWidth;
		m_nHeight = nHeight;
		m_nType = nType;
		if (nSize)
			m_nImageSize = nSize;
		else
			m_nImageSize = nWidth * nHeight * EdtImage::GetTypeBytes(nType);

		m_nSeqOffset = m_nImageSize + m_nSeqGap;
	}


	void SetImageValues(EdtImage *pImage)
	{
		ASSERT(pImage != NULL);
		SetImageValues(pImage->GetType(), pImage->GetWidth(), pImage->GetHeight(), 
			pImage->FileIOSize());
	}

	void GetImageValues(int &nWidth, int &nHeight, EdtDataType &nType) const
	{
		nWidth = m_nWidth;
		nHeight = m_nHeight;
		nType = m_nType;
	}

	void SetOffsets(const int nHdrOffset, const int nSeqGap = 0)

	{
		m_nHdrOffset = nHdrOffset;
		m_nSeqGap = nSeqGap;
	}

	void GetOffsets(int nHdrOffset, int &nSeqGap) const

	{
		nHdrOffset = m_nHdrOffset;
		nSeqGap = m_nSeqGap;
	}

	bool HaveValues() const
	{
		return (m_nWidth > 0 && m_nHeight > 0 && m_nType > 0);
	}

	virtual int LoadEdtImage(const char *szFileName, 
			EdtImage *pImage, 
			const int nWidth, const int nHeight, const int nType, const int nSize = 0);

	/* For handling image sequences */

	virtual int LoadIndexedImage(EdtImage *pImage, int nIndex);

	virtual int OpenImageFile(const char *szFileName, const int nFileMode,
		const int nWidth = 0, const int nHeight = 0, const EdtDataType nType = TypeBYTE, const int nSize = 0, const bool bFailOk = FALSE);

	virtual void CloseImageFile();

	virtual int LoadNextImage(EdtImage *pImage);

	virtual int SaveNextImage(EdtImage *pImage);

	virtual int SeekImageFrame(const int nFrame) {return -1;}

	int GetCurrentFrame() const {return m_nSeqNumber;}

	void SetAsynch(bool bState);

	bool GetAsynch()
	{
		return m_bAsynch;
	}

#ifdef WIN32
	OVERLAPPED m_Overlapped; /* for asynchronous I/O in NT */
#endif


	void SetBlockCache(const int nBlockCache)
	{
		if (IsBlockActive())
			Flush();

		m_nBlockCache = nBlockCache;
		if (m_nBlockCache < 1)
			m_nBlockCache = 1;
	}


	int GetBlockCache()
	{
		return m_nBlockCache;
	}

	virtual void Flush()
	{
	}

	// returns whether there's a currently active block

	bool IsBlockActive() 
	{ 
		return m_nBlocksActive > 0; 
	}

};



#endif // !defined(AFX_IMAGEFILE_H__4B460FC4_2D69_11D2_97DC_00104B357776__INCLUDED_)
