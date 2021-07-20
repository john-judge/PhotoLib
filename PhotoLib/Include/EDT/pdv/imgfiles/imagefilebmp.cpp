// ImageFileBMP.cpp: implementation of the CImageFileBMP class.
//
//////////////////////////////////////////////////////////////////////

#include "stdio.h"
#include "fcntl.h"
#include "edtimage/EdtImage.h"
#include "imagefilebmp.h"

#include "edtimage/EdtLut.h"


#define WIDTHBYTES(bits) (((bits) + 31) / 32 * 4)

#ifndef WIN32

#define DWORD unsigned long
#define LONG long
#define WORD unsigned short
#define BYTE unsigned char


typedef struct tagBITMAPFILEHEADER {
        WORD    bfType;
        DWORD   bfSize;
        WORD    bfReserved1;
        WORD    bfReserved2;
        DWORD   bfOffBits;
} BITMAPFILEHEADER;

/* structures for defining DIBs */
typedef struct tagBITMAPCOREHEADER {
        DWORD   bcSize;                 /* used to get to color table */
        WORD    bcWidth;
        WORD    bcHeight;
        WORD    bcPlanes;
        WORD    bcBitCount;
} BITMAPCOREHEADER ;


typedef struct tagBITMAPINFOHEADER{
        DWORD      biSize;
        LONG       biWidth;
        LONG       biHeight;
        WORD       biPlanes;
        WORD       biBitCount;
        DWORD      biCompression;
        DWORD      biSizeImage;
        LONG       biXPelsPerMeter;
        LONG       biYPelsPerMeter;
        DWORD      biClrUsed;
        DWORD      biClrImportant;
} BITMAPINFOHEADER;

/* constants for the biCompression field */
#define BI_RGB        0L
#define BI_RLE8       1L
#define BI_RLE4       2L
#define BI_BITFIELDS  3L

typedef struct tagRGBQUAD {
        BYTE    rgbBlue;
        BYTE    rgbGreen;
        BYTE    rgbRed;
        BYTE    rgbReserved;
} RGBQUAD, *LPRGBQUAD;

typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER    bmiHeader;
    RGBQUAD             bmiColors[1];
} BITMAPINFO;

#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CImageFileBMP::CImageFileBMP()
{

}

CImageFileBMP::~CImageFileBMP()
{

}

const char * CImageFileBMP::GetName() 
{
	return "Windows Bitmap";
}

const char * CImageFileBMP::GetExtension() 
{
	return "bmp";
}

int CImageFileBMP::CanLoad(const char * szFileName)

{
	// check if file exists

	long nWidth,
		nHeight;

	EdtDataType nType;
	
	if (GetImageInfo(szFileName,nWidth,nHeight,nType) != -1)
	{
		if (nType == TypeBYTE ||
			nType == TypeBGR)
			return 1;
	}

	return 0;
}

int CImageFileBMP::GetImageInfo(const char *szFileName, 
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

	BITMAPFILEHEADER BmpFileHdr;

	int nbytes = fread(&BmpFileHdr,1,sizeof(BmpFileHdr),f);

	if (nbytes < sizeof(BmpFileHdr))
	{
		goto abort;
	}

	if (BmpFileHdr.bfType != 0x4D42)
	{
		goto abort;
	}
	
	// read header
    BITMAPINFOHEADER BmpInfoHdr;
    nbytes = fread(&BmpInfoHdr, 1, sizeof(BmpInfoHdr),f); 
    if (nbytes != sizeof(BmpInfoHdr))
	{	
		goto abort;
    }

    // Check that we got a real Windows DIB file.

	if (BmpInfoHdr.biSize != sizeof(BITMAPINFOHEADER))
	{
        if (BmpInfoHdr.biSize != sizeof(BITMAPCOREHEADER))
		{
			goto abort;
        }

        // Set a flag to convert PM file to Win format later.
        bIsPM = TRUE;

        // Back up the file pointer and read the BITMAPCOREHEADER
        // and create the BITMAPINFOHEADER from it.
        fseek(f, 0, SEEK_SET);
        BITMAPCOREHEADER BmpCoreHdr;
        nbytes = fread(&BmpCoreHdr, 1,sizeof(BmpCoreHdr),f); 
        if (nbytes != sizeof(BmpCoreHdr))
		{		
			goto abort;
        }

        BmpInfoHdr.biSize = sizeof(BITMAPINFOHEADER);
        BmpInfoHdr.biWidth = (int) BmpCoreHdr.bcWidth;
        BmpInfoHdr.biHeight = (int) BmpCoreHdr.bcHeight;
        BmpInfoHdr.biPlanes = BmpCoreHdr.bcPlanes;
        BmpInfoHdr.biBitCount = BmpCoreHdr.bcBitCount;
        BmpInfoHdr.biCompression = BI_RGB;
        BmpInfoHdr.biSizeImage = 0;
        BmpInfoHdr.biXPelsPerMeter = 0;
        BmpInfoHdr.biYPelsPerMeter = 0;
        BmpInfoHdr.biClrUsed = 0;
        BmpInfoHdr.biClrImportant = 0;
    }
	
	if ((BmpInfoHdr.biPlanes == 1 && 
		BmpInfoHdr.biBitCount == 8) ||
		(BmpInfoHdr.biPlanes == 3 && 
		BmpInfoHdr.biBitCount == 8) || 
		(BmpInfoHdr.biPlanes == 1 && 
		BmpInfoHdr.biBitCount == 24))
	
	{
		nWidth = BmpInfoHdr.biWidth;
		nHeight = BmpInfoHdr.biHeight;

		if (BmpInfoHdr.biPlanes == 1 && BmpInfoHdr.biBitCount == 8)
			nType = TypeBYTE;
		else
			nType = TypeBGR;

		fclose(f);
		return 0;
	}
	

abort:
	fclose(f);

	return -1;
}

int CImageFileBMP::LoadEdtImage( const char *szFileName, 
	EdtImage *pImage)

{

	// Make sure we can read it

	FILE *f = NULL;



	long nWidth,
		nHeight;

	EdtDataType nType;

	if (GetImageInfo(szFileName,nWidth,nHeight,nType) != -1)
	{
		
#ifdef WIN32

		_fmode = _O_BINARY;

#endif

		f = fopen(szFileName,"r");

		if (!f)
		{
			return -1;
		}

		BITMAPFILEHEADER BmpFileHdr;

		int nbytes = fread(&BmpFileHdr,1,sizeof(BmpFileHdr),f);

		if (nbytes < sizeof(BmpFileHdr))
		{
			goto abort;
		}

		if (BmpFileHdr.bfType != 0x4D42)
		{
			goto abort;
		}
		
		// read header
		BITMAPINFOHEADER BmpInfoHdr;
		nbytes = fread(&BmpInfoHdr, 1, sizeof(BmpInfoHdr),f); 
		if (nbytes != sizeof(BmpInfoHdr))
		{	
			goto abort;
		}
		// Open File 
		
		pImage->Create(nType, nWidth, nHeight);

		// Seek to Data
		fseek(f,BmpFileHdr.bfOffBits,SEEK_SET);

		// Read Data

		byte **pRows = (byte **) pImage->GetPixelRows();

		int nTargetSize = pImage->GetPitch();

		for (int row = nHeight - 1;row >= 0;row --)
		{
			
			nbytes = fread(pRows[row],1,nTargetSize,f);

			if (nbytes != nTargetSize)
				goto abort;

		}

		// Close File

		fclose(f);

		return 0;
	}

abort:
	if (f)
		fclose(f);

	return -1;
}


int CImageFileBMP::CanSave(const char *szFileName,
		EdtImage *pImage)

{
	ASSERT(szFileName);
	ASSERT(pImage);

	if (pImage->IsAllocated() &&
		(pImage->GetType() == TypeBYTE ||
		pImage->GetType() == TypeBGR || 
		pImage->GetType() == TypeUSHORT))

		return 1;
	else

		return 0;
}

int CImageFileBMP::SaveImage(const char *szFileName,
		EdtImage *pImage)

{
	// Make sure we can save it

	if (CanSave(szFileName, pImage))
	{
		EdtImage *pWorkImage = pImage;


		// Allocate memory for the header (if necessary)
		BITMAPINFO *pBMI = (BITMAPINFO *)
				malloc(sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));

		if (pImage->GetType() == TypeUSHORT)
		{

			// make a temporary copy

			pWorkImage = new EdtImage(TypeBYTE, pImage->GetWidth(), pImage->GetHeight());

			EdtLut lut;

			lut.Setup(pImage, pWorkImage);

			lut.SetLinear(0,pImage->GetMaxValue());

			lut.Transform(pImage, pWorkImage);

		}

	  // Fill in the header info.
		BITMAPINFOHEADER* pBI = (BITMAPINFOHEADER*) pBMI;
		pBI->biSize = sizeof(BITMAPINFOHEADER);
		pBI->biWidth = pWorkImage->GetWidth();
		pBI->biHeight = pWorkImage->GetHeight();
		pBI->biPlanes = 1;
		pBI->biBitCount = (pWorkImage->GetType()==TypeBYTE)?8:24;
		pBI->biCompression = BI_RGB;
		pBI->biSizeImage = 0;
		pBI->biXPelsPerMeter = 0;
		pBI->biYPelsPerMeter = 0;
		pBI->biClrUsed = 0;
		pBI->biClrImportant = 0;

		RGBQUAD* prgb = (LPRGBQUAD)(((BYTE *)(pBMI)) + sizeof(BITMAPINFOHEADER));

		for (int i = 0; i < 256; i++)
		{	
			prgb->rgbBlue = prgb->rgbGreen = prgb->rgbRed = (BYTE) i;
		    prgb->rgbReserved = 0;
		    prgb++;
		}

		// Fill Header
		// Fill in the fields of the file header
		BITMAPFILEHEADER bmfHdr; // Header for Bitmap file

		// Fill in file type (first 2 bytes must be "BM" for a bitmap)
		bmfHdr.bfType = 0x4D42;  // "BM"

		// Calculate the size of the DIB.
		// First, find size of header plus size of color table.  Since the
		// first DWORD in both BITMAPINFOHEADER and BITMAPCOREHEADER contains
		// the size of the structure, let's use this. Partial Calculation.
		int palettesize = (pBMI->bmiHeader.biBitCount == 8)? (256 * sizeof(RGBQUAD)):0;

		//pBMI->bmiHeader.biSize += palettesize;

		DWORD dwDIBSize = *(unsigned long *)pBMI + palettesize;
		
		DWORD dwBmBitsSize = WIDTHBYTES(pWorkImage->GetWidth() * ((DWORD)pBMI->bmiHeader.biBitCount)) * 
			pWorkImage->GetHeight();

		int nPadding = WIDTHBYTES(pWorkImage->GetWidth() * ((DWORD)pBMI->bmiHeader.biBitCount)) -
				(int)(pWorkImage->GetLineWidth());

		dwDIBSize += dwBmBitsSize;

		// Now, since we have calculated the correct size, why don't we
		// fill in the biSizeImage field (this will fix any .BMP files
		// which have this field incorrect).
		pBMI->bmiHeader.biSizeImage = dwBmBitsSize;
			
		// Calculate the file size by adding the DIB size to sizeof(BITMAPFILEHEADER)
		bmfHdr.bfSize = dwDIBSize + sizeof(BITMAPFILEHEADER);
		bmfHdr.bfReserved1 = 0;
		bmfHdr.bfReserved2 = 0;

		// Now, calculate the offset the actual bitmap bits will be in
		// the file -- It's the Bitmap file header plus the DIB header,
		// plus the size of the color table.
		bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) +
			pBMI->bmiHeader.biSize + palettesize;

		// Open File
		FILE *f = fopen(szFileName, "wb");


		// Write Header 
		fwrite(&bmfHdr,1, sizeof(bmfHdr),f);
		fwrite(pBMI, 1, pBMI->bmiHeader.biSize + palettesize, f);
		// Write Data

		byte **bpRows = (byte **) pWorkImage->GetPixelRows();

		int w = pWorkImage->GetLineWidth();
		
		for (int row = pWorkImage->GetHeight()-1;row >= 0;row--)
		{
			fwrite(bpRows[row],1,
				w,f);
			if (nPadding)
				fwrite(bpRows[row],1,nPadding,f);
		}
		// Close File
		fclose(f);

		if (pWorkImage != pImage)
			delete pWorkImage;

		return 0;
	}

	return -1;
}

CImageFileBMP theBMPFileType;
CImageFileType *pBMPFileType = &theBMPFileType;
