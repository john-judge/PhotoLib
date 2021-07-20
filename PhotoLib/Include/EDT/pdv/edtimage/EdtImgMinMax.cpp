// EdtImgMinMax.cpp: implementation of the EdtImgMinMax class.
//
//////////////////////////////////////////////////////////////////////


#include "EdtImgMinMax.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

EdtImgMinMax::EdtImgMinMax()
{

}

EdtImgMinMax::~EdtImgMinMax()
{

}


int 
    EdtImgMinMax::Compute(EdtImage *pImage, int &nMin, int &nMax, int nBand)

{

    int row, col;

    if (pImage && pImage->IsAllocated())
    {
        int nWidth = pImage->GetWidth();
        int nHeight = pImage->GetHeight();

        switch (pImage->GetType())
        {

        case EDT_TYPE_BYTE:
            {

                byte **pRows = (byte **) pImage->GetPixelRows();
                byte bMin = pRows[0][0];
                byte bMax = pRows[0][0];
                for (row = 0; row < nHeight; row++)
                {
                    for (col = 0; col < nWidth; col ++ )
                    {
                        byte b = pRows[row][col];

                        if (b > bMax)
                            bMax = b;
                        else if (b < bMin)
                            bMin = b;
                    }
                }
                nMin = bMin;
                nMax = bMax;
            }
            break;

        case EDT_TYPE_USHORT:

            {

                ushort_t **pRows = (ushort_t **) pImage->GetPixelRows();
                ushort_t bMin = pRows[0][0];
                ushort_t bMax = pRows[0][0];
                for (row = 0; row < nHeight; row++)
                {
                    for (col = 0; col < nWidth; col ++ )
                    {
                        ushort_t b = pRows[row][col];

                        if (b > bMax)
                            bMax = b;
                        else if (b < bMin)
                            bMin = b;
                    }
                }
                nMin = bMin;
                nMax = bMax;

            }
            break;
                    
        case EDT_TYPE_BGR:
        case EDT_TYPE_RGB:
            {

                byte **pRows = (byte **) pImage->GetPixelRows();
                byte bMin = pRows[0][nBand];
                byte bMax = pRows[0][nBand];
                for (row = 0; row < nHeight; row++)
                {
                    for (col = 0; col < nWidth * 3; col +=3 )
                    {
                        byte b = pRows[row][col];

                        if (b > bMax)
                            bMax = b;
                        else if (b < bMin)
                            bMin = b;
                    }
                }
                nMin = bMin;
                nMax = bMax;
            }
            break;
        }
    }

    return 0;
}
