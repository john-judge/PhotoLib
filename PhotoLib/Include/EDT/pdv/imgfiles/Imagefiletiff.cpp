//--------------------------------------------
// File:
// 
// Purpose:
//
// Date:
//

#include "stdio.h"
#include "fcntl.h"
#include "edtimage/EdtImage.h"
#include "Imagefiletiff.h"

#include "tiffio.h"

#include "dispatch/ErrorHandler.h"

static void
IpTiffErrorHandler(const char *s1, const char *s2,va_list ap)

{
	// do nothing
}

static int TiffErrorLoaded = TRUE;

CImageFileTiff::CImageFileTiff()

{
	// Disable alert
	if (!TiffErrorLoaded) {
		TIFFSetErrorHandler(IpTiffErrorHandler);
		TiffErrorLoaded = TRUE;
	}

}	

CImageFileTiff::~CImageFileTiff()

{
}

const char * CImageFileTiff::GetName() 
{
	return "Tiff Image File";
}

const char * CImageFileTiff::GetExtension() 
{
	return "tif";
}

// Determine if this is a tiff image

int CImageFileTiff::CanLoad(const char * szFileName)

{
	// don't mem map for right now (jsc 1-13-97) - fails for some reason

	bool rc = FALSE;

	TIFF* tif = TIFFOpen(szFileName, "ru");
	
	if (tif) {
 
		uint16 imagedepth, planes;
       
        TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &imagedepth);
		TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &planes);

		// if OK, return true

		if (planes == 1  && (imagedepth == 8 || imagedepth == 16)) {
		
			rc = TRUE;
		}

        TIFFClose(tif);
	}

	return rc;
}


bool TiffIpLoad(EdtImage *pImage,TIFF *tif)

{
	void **pTmpRow;

	pTmpRow = pImage->GetPixelRows();

	int height = pImage->GetHeight();

	for (int row = 0;row<height;row++,pTmpRow++)
		TIFFReadScanline(tif,*pTmpRow,row,0);
	
	return TRUE;
}


bool TiffIpSave(EdtImage *pImage,TIFF *tif)

{
	void **pTmpRow;

	pTmpRow = pImage->GetPixelRows();
    u_char *invRow = NULL;
	int height = pImage->GetHeight();
	int width = pImage->GetWidth();
    int nType = pImage->GetType();

	if (nType == TypeBGR)
        invRow = (u_char *)edt_alloc(width * 3 * sizeof(u_char));

	for (int row = 0;row<height;row++,pTmpRow++)
    {
		if (nType == TypeBGR)
        {
            int i;
            u_char tmp;
            u_char *dp, *sp = (u_char *)*pTmpRow;

            // invert BGR -> RGB 
            for (i = 0, dp = invRow; i < width; i++, sp +=3)
            {
                *dp++ = *(sp+2);
                *dp++ = *(sp+1);
                *dp++ = *sp;
            }
    		TIFFWriteScanline(tif,invRow,row,0);
        }
        else 
        {
    		TIFFWriteScanline(tif,*pTmpRow,row,0);
        }
    }

    edt_free(invRow);
	
	return TRUE;
}

int CImageFileTiff::GetImageInfo(const char *szFileName, 
	long &nWidth, long &nHeight, EdtDataType &nType)

{
	// Make sure we can read it

	FILE *f;
	bool bIsPM = FALSE;
    int ret = 0;

	f = fopen(szFileName,"r");

	if (!f)
	{
		return -1;
	}

	bool rc = FALSE;

	// don't mem map for right now (jsc 1-13-97) - fails for some reason

	TIFF* tif = TIFFOpen(szFileName, "ru");

	if (tif) {

		uint32 imagewidth, imageheight;
		uint16 imagedepth, planes;
       
 		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &imagewidth);
	    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imageheight);
        TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &imagedepth);
		TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &planes);

 
		nWidth = imagewidth;
		nHeight = imageheight;
		
		switch (planes)
		{
    		case 1:
            {
    			if (imagedepth == 8)
    				nType = TypeBYTE;
    			else if (imagedepth == 16)
    				nType = TypeUSHORT;
    			else if (imagedepth == 1)
    				nType = TypeMONO;
    			else ret = -1;
            }
            break;

            case 3:
            {
    			if (imagedepth == 8)
    				nType = TypeBGR;
                else ret = -1;
            }
            break;
        }
 
		TIFFClose(tif);
	}

 	fclose(f);

	return ret;

}


//---------------------------------------------------------------------------------------------------------------------
//	FUNCTION			CImageFileTiff::Load
//---------------------------------------------------------------------------------------------------------------------
//
//  Purpose          :	Loads an image from a file.
//
//  Input            :  const CString&		Image filename
//
//  Output           :  bool			TRUE if successful, FALSE if not
//
//  Restriction      :	The image format will be autodetected during load.
//---------------------------------------------------------------------------------------------------------------------
int CImageFileTiff::LoadEdtImage(const char *szFileName,
		EdtImage *pImage)
{

	int rc = -1;

	// don't mem map for right now (jsc 1-13-97) - fails for some reason

	TIFF* tif = TIFFOpen(szFileName, "ru");
	

	if (tif) {
  		uint32 imagewidth, imageheight;
		uint16 imagedepth, planes;
       
   		uint32 config;
		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &imagewidth);
	    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imageheight);
        TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &imagedepth);
		TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &planes);

        TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &config);
 
		// if OK, read
		m_nWidth = imagewidth;
		m_nHeight = imageheight;
	
		switch (planes)
		{
			case 1:
				if (imagedepth == 8)
					m_nType = TypeBYTE;
				else if (imagedepth == 16)
					m_nType = TypeUSHORT;
				else if (imagedepth == 1)
					m_nType = TypeMONO;

				pImage->Create(m_nType, m_nWidth, m_nHeight);
				rc = !TiffIpLoad(pImage,tif);

            case 3:
				if (imagedepth == 8)
					m_nType = TypeBGR;

				pImage->Create(m_nType, m_nWidth, m_nHeight);
				rc = !TiffIpLoad(pImage,tif);
				
			break;
			
		}

         TIFFClose(tif);

		 // Mark for update
		 pImage->SetDataChanged();

    }

	return rc;


}

//---------------------------------------------------------------------------------------------------------------------
//	FUNCTION			CImageFileTiff::Save
//---------------------------------------------------------------------------------------------------------------------
//
//  Purpose          :	Saves an image to file using the given format and compression parameter
//
//  Input            :  const CString&		Image filename
//						EImageFileType	Format to save image in (default = CT_BMP, Windows Bitmap)
//						unsigned int	JPEG quality setting when JPEG compression is used (default = 100)
//
//  Output           :  bool			TRUE if successful, FALSE if not
//
//  Restriction      :	None
//---------------------------------------------------------------------------------------------------------------------

int CImageFileTiff::SaveImage(const char *szFileName,
		EdtImage *pImage)
{

	int rc = -1;
    uint16 planes, depth;
	TIFF* tif;

	if (CanSave(szFileName, pImage))
	{
		SetImageValues(pImage);

		switch (m_nType) {

		case TypeBYTE:
		case TypeUSHORT:
    		tif = TIFFOpen(szFileName, "w");
    		TIFFSetField( tif,TIFFTAG_PHOTOMETRIC,PHOTOMETRIC_MINISBLACK );
    		TIFFSetField( tif,TIFFTAG_BITSPERSAMPLE, pImage->GetDepth() );
    		TIFFSetField( tif,TIFFTAG_SAMPLESPERPIXEL, 1 );

            break;

		case TypeBGR:
		case TypeRGB:
    		tif = TIFFOpen(szFileName, "w");
    		TIFFSetField( tif,TIFFTAG_PHOTOMETRIC,PHOTOMETRIC_RGB );
    		TIFFSetField( tif,TIFFTAG_BITSPERSAMPLE, 8 );
    		TIFFSetField( tif,TIFFTAG_SAMPLESPERPIXEL, 3 );
			break;

		default:
			return -1;

		}

		TIFFSetField( tif,TIFFTAG_IMAGEWIDTH,	pImage->GetWidth());
		TIFFSetField( tif,TIFFTAG_IMAGELENGTH,	pImage->GetHeight() );
		TIFFSetField( tif,TIFFTAG_ORIENTATION,ORIENTATION_TOPLEFT );

#ifdef USE_COMPRESSION

			switch (Format) {
			case AC_TIFF_HUFFMAN:
				TIFFSetField( tif,TIFFTAG_COMPRESSION,COMPRESSION_CCITTRLE ); 
				break;
			case AC_TIFF_LZW:
				TIFFSetField( tif,TIFFTAG_COMPRESSION,COMPRESSION_LZW ); 
				break;
			}
#endif
			
		TIFFSetField( tif,TIFFTAG_DOCUMENTNAME,szFileName );
		TIFFSetField( tif,TIFFTAG_ROWSPERSTRIP, pImage->GetHeight() );
	//	TIFFSetField( tif,TIFFTAG_STRIPBYTECOUNTS,1 );
		TIFFSetField( tif,TIFFTAG_PLANARCONFIG,PLANARCONFIG_CONTIG );
	
		rc = !TiffIpSave(pImage,tif);

		TIFFClose(tif);

	}
	
	return rc;

}


int CImageFileTiff::CanSave(const char *szFileName,
		EdtImage *pImage)

{
	ASSERT(szFileName);
	ASSERT(pImage);

	if ( pImage->IsAllocated() &&
        ((pImage->GetType() == TypeUSHORT) ||
         (pImage->GetType() == TypeBYTE) ||
         (pImage->GetType() == TypeRGB) ||
         (pImage->GetType() == TypeBGR))) 

		return 1;
	else

		return 0;

}

/* Global Single Module */

CImageFileTiff theTiffFileType;
CImageFileType *pTiffFileType = &theTiffFileType;

