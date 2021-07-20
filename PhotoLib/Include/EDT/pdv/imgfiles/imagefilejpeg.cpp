#include "edtimage/EdtImage.h"
#include "imagefilejpeg.h"

#define XMD_H // yeah, but how else to avoid typedef INT32 redefinition FATAL error in jmorecfg.h?
extern "C" {
#include "jpeglib.h"
}

CImageFileJPEG::CImageFileJPEG()
{
    qValue = 0;
}

CImageFileJPEG::~CImageFileJPEG()
{

}

const char * CImageFileJPEG::GetName()
{
    return "JPEG";
}

const char * CImageFileJPEG::GetExtension()
{
    return "jpg";
}

/* gets the header from jpeg file and sets parameters */
int CImageFileJPEG::GetImageInfo(const char *szFileName,
                                 long &nWidth, long &nHeight,
                                 EdtDataType &nType)
{
    struct jpeg_decompress_struct cinfo;
    FILE *f = fopen(szFileName, "rb");

    if (!f)
        return -1;
    
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, f);
    jpeg_read_header(&cinfo, TRUE);

    nWidth = cinfo.image_width;
    nHeight = cinfo.image_height;
    
    if (cinfo.jpeg_color_space == JCS_RGB)
        nType = TypeRGB;
    else
        nType = TypeBYTE;

    fclose(f);
    jpeg_destroy_decompress(&cinfo);

    return 0;
}

/* decompress a jpeg file and create an EdtImage object
   from it. */
int CImageFileJPEG::LoadEdtImage(const char *szFilename,
                                 EdtImage *pImage)
{
    struct jpeg_decompress_struct cinfo;
    FILE *f = NULL;
    EdtDataType nType;
    JSAMPARRAY buffer;
    int row_stride;

    f = fopen(szFilename, "rb");

    if (!f)
        return -1;  

    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, f);
    jpeg_read_header(&cinfo, TRUE);

    if (cinfo.jpeg_color_space == JCS_RGB)
        nType = TypeRGB;
    else
        nType = TypeBYTE;

    pImage->Create(nType, cinfo.image_width, cinfo.image_height);

    jpeg_start_decompress(&cinfo);
    
    row_stride = cinfo.output_width * cinfo.output_components;

    buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
    byte **pRows = (byte **) pImage->GetPixelRows();

    while (cinfo.output_scanline < cinfo.output_height)
    {
        jpeg_read_scanlines(&cinfo, buffer, 1);
        parse_scanline(buffer[0], cinfo.output_scanline, cinfo.output_width,
                       cinfo.output_height, -1, *pRows++, nType);
    }
  
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    fclose(f);

    return 0;
}

void CImageFileJPEG::parse_scanline(unsigned char buf[], int line, int width,
                               int height, int alpha, unsigned char img[],
                               EdtDataType nType)
{
    int i, j, k;

    if (nType == TypeRGB)
    {
        if (alpha<0 || alpha>255)
        {
            k = (height - line) * 3 * width;
            for (i=0; i<3*width; i+=3)
            {
                img[k+i] = buf[i];
                img[k+i+1] = buf[i+1];
                img[k+i+2] = buf[i+2];
            }
        }
    
        else
        {
            k = (height - line) * 4 * width;
            j = 0;
            for (i=0; i<3*width; i+=3)
            {
                img[k+j+i] = buf[i];
                img[k+j+i+1] = buf[i+1];
                img[k+j+i+2] = buf[i+2];
                img[k+j+i+3] = alpha;
                j++;
            }
        }
    }
    else
    {
        k = (height - line) * width;
        for (i=0; i<width; i++)
            img[k+i] = buf[i];
    }
}
           
int CImageFileJPEG::CanSave(const char *szFilename, EdtImage *pImage)
{
    ASSERT(szFilename);
    ASSERT(pImage);

    if (pImage->IsAllocated() && 
        (pImage->GetType() == TypeBYTE ||
         pImage->GetType() == TypeRGB ||
         pImage->GetType() == TypeBGR ))        
        return 1;
    else
        return 0;
}

/* Set the quantization value for the image.
   Call to set before compressing and saving
   an image */
int CImageFileJPEG::SetQValue(int in_qValue)
{
    if (in_qValue >= 0 || in_qValue <= 100)
        qValue = in_qValue;
    else
        return -1;
  
    return 0;
}

int CImageFileJPEG::GetQValue()
{
    return qValue;
}

int CImageFileJPEG::SaveImage(const char *szFilename, EdtImage *pImage)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE *f;
    int row_stride;
    bool needs_transpose = false;

    if (CanSave(szFilename, pImage))
    {
        EdtImage *pWorkImage = pImage;
        SetImageValues(pImage);
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);
        
        if ((f = fopen(szFilename, "wb")) == NULL)
            return -1;
        
        jpeg_stdio_dest(&cinfo, f);        
        cinfo.image_width = pWorkImage->GetWidth();
        cinfo.image_height = pWorkImage->GetHeight();
        
        if (pImage->GetType() == TypeRGB)
        {
            cinfo.input_components = 3;
            cinfo.in_color_space = JCS_RGB;
        }
        else if (pImage->GetType() == TypeBYTE)
        {
            cinfo.input_components = 1;
            cinfo.in_color_space = JCS_GRAYSCALE;
        }
        else if (pImage->GetType() == TypeBGR)
        {
            cinfo.input_components = 3;
            cinfo.in_color_space = JCS_RGB;
            needs_transpose = true;
        }
        else
        {
            cinfo.input_components = 1;
            cinfo.in_color_space = JCS_UNKNOWN;
        }

        jpeg_set_defaults(&cinfo);
        jpeg_start_compress(&cinfo, true);

        JSAMPROW row_pointer[1];
        byte **bpRows = (byte **) pWorkImage->GetPixelRows();
        row_stride = cinfo.input_components * cinfo.image_width;
        byte *tmprow = NULL;

        while (cinfo.next_scanline < cinfo.image_height)
        {
            if (needs_transpose)
            {
                if (!tmprow) tmprow = new byte[row_stride];
                memcpy(tmprow, *bpRows++, row_stride);
                RowTranspose(tmprow, cinfo.image_width);
                row_pointer[0] = (JSAMPROW) tmprow; 
            }
            else row_pointer[0] = (JSAMPROW) *bpRows++; 

            jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }
        free(tmprow);

        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        fclose(f);
    }
    else 
        return -1;
    
    return 0;
}

void CImageFileJPEG::RowTranspose(unsigned char *row, int width)
{
    unsigned char tmp;

    for (int i=0; i<width*3; i+=3)
    {
       tmp = row[i];
       row[i] = row[i + 2];
       row[i + 2] = tmp;
    }
} 
CImageFileJPEG theJpegFileType;
CImageFileType *pJPEGFileType = &theJpegFileType;
