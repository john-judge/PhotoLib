#include "edtinc.h"
#include "edt_jpeg.h"

int edt_save_jpeg(const char *szFilename, int width, int height,
                  int depth, int qvalue, unsigned char* buf)

{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE *f;
    int row_index = 1;
    int needs_transpose = 0;
    int row_size = width * depth;
    unsigned char* row = (unsigned char*)malloc((row_size) *sizeof(char));
    JSAMPROW row_pointer[1];
   
    
    if ((f = fopen(szFilename, "wb")) == NULL)
    {
        edt_perror(szFilename);
        return -1;
    }

    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, f);
    cinfo.image_width = width;
    cinfo.image_height = height;

    if (depth >= 24)
    {
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;
    }
    else
    {
        cinfo.input_components = 1;
        cinfo.in_color_space = JCS_GRAYSCALE;
    }

    jpeg_set_defaults(&cinfo);
    jpeg_start_compress(&cinfo, TRUE);

    while (cinfo.next_scanline < cinfo.image_height)
    {
        memcpy(row, buf, row_size);

        row_pointer[0] = (JSAMPROW) row;
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    
        buf += row_size;
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    
    fclose(f);

    return 0;     
}


