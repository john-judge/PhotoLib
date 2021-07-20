#ifndef CIMAGEFILEJPEG_H
#define CIMAGEFILEJPEG_H

#include "imagefiletype.h"

class CImageFileJPEG : public CImageFileType
{
public:
    CImageFileJPEG();
    virtual ~CImageFileJPEG();
    
    virtual const char *GetExtension();
    virtual const char *GetName();
    
    virtual int GetImageInfo(const char *szFilename, long &width, 
                             long &height, EdtDataType &nType);
    virtual int LoadEdtImage(const char *szFilename, EdtImage *pImage);
    virtual int CanSave(const char *szFilename, EdtImage *pImage);
    virtual int SaveImage(const char *szFilename, EdtImage *pImage);
    
    void parse_scanline(unsigned char buf[], int line, int width,
                        int height, int alpha, unsigned char img[],
                        EdtDataType nType);
    void RowTranspose(unsigned char *row, int width);

    int SetQValue(int in_qValue);
    int GetQValue();

private:
    int qValue;

};

#endif
