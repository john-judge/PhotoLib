/** ********************************
* 
* @file EdtImage.cpp
*
* Copyright (c) Engineering Design Team 2010
* All Rights Reserved
*/


// EdtImage.cpp: implementation of the EdtImage class.
//
//////////////////////////////////////////////////////////////////////

//#include "edtdefines.h"
#include "EdtImage.h"
#include "edt_utils.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#ifdef __APPLE__
#define PAGE_SIZE 4096
#endif

// Clear all values;
extern "C" {
#include "edt_utils.h"
}

#include "dispatch/ErrorHandler.h"
#include "EdtRowOps.h"

// global value for 
// allocating trace buffers 
// 
static int TraceSize = 0;

void EdtImage::SetTraceSize(const int size)
{
    TraceSize = size;
}

int EdtImage::GetTraceSize()
{
    return TraceSize;
}

void EdtImage::ResetValues(const char *nm)

{
    m_bAllocated	= false;
    m_bOwnData		= false;
    m_bRowsClean	= false;
    m_bDataChanged	= false;

    m_bDisplayable	= false;
    m_bContiguous	= false;

    m_bInverted		= false;

    m_pRows			= NULL;

    m_nLastPitch    = 0;
    m_nFullRows     = 0;

    m_nPadBytes = 4;

    m_nSectorSize = SECTOR_SIZE;

    m_pHeaderData = NULL;
    m_base_data = NULL;

    m_pTimestamp = NULL;

    m_nSequenceNumber = 0;

    m_data = NULL;
    m_header = 0;

    m_own_data = false;
    m_timestamp = 0.0;
    m_header = 0;
    m_ready = true;

    refcounts = 0;

    m_owner_id = 0;

    trace = NULL;
    n_trace_elements = 0;
    max_trace_values = 0;

    AllocateTrace(TraceSize);

    if (nm)
        m_name = nm;
}

/**
*  
*  @return 
*/

EdtImage::EdtImage(const char *nm) 

{
    ResetValues(nm);
}

/**
*  Constructor, takes 
*  
*  @param  nWidth
*  @param  nHeight
*  @param  nType
*/

EdtImage::EdtImage(const EdtDataType nType,
                   const int nWidth, 
                   const int nHeight,
                   const char *nm) 
{
    ResetValues(nm);
    Create(nType, nWidth, nHeight);
}

/**

    @param pParent
    @param  nX
    @param  nY
*  @param  nWidth
*  @param  nHeight
*  @param  nBand
*/

EdtImage::EdtImage(EdtImage *pParent, const int nX, const int nY,
                   const int nWidth, const int nHeight,
                   const char *nm) 
{
    ResetValues(nm);

    if (pParent)
    {
        //Clone parent

        SetType(pParent->GetType());

        //Assign Parent
        AttachToParent(pParent,nX, nY, nWidth, nHeight);
    }
}

/**
*  
*  Destructor.
*/

EdtImage::~EdtImage()
{
    // Delete pointers
    Destroy();
}

/**
*  
*  @return void 
*  @param  nWidth
*  @param  nHeight
*  @param  nType
*/

void
EdtImage::Create(const EdtDataType nType, const int nWidth, const int nHeight )
{
    // create memory buffer


    if (nWidth == GetWidth() && nHeight == GetHeight() && nType == GetType() && AllocatedSize() && m_base_data)
    {
        return;
    }

    SetType(nType);

    int nPitch = (GetLineWidth(nWidth));

    if (m_nPadBytes && (nPitch % m_nPadBytes)) 
    {
        nPitch = ((nPitch + m_nPadBytes) & ~(m_nPadBytes-1));
    }
    else
        m_bContiguous = true;

    unsigned int buffersize = nPitch * nHeight;

    // activesize is buffer size + header size
    // dmasize is buffer size + header size if header within dma
    int activesize = bsize.AlignedActiveSize(buffersize);
    int dmasize = bsize.AlignedDMASize(buffersize);

    // rawiosize is size as written/read from disk
    // Only different if sectorsize is nonzero

    int rawiosize = bsize.AlignedRawIOSize(activesize);
    int file_io_size = bsize.AlignedFileIOSize(activesize);

    byte *pData = (byte *) m_base_data;

    if (AllocatedSize() < rawiosize)
    {
        byte *oldp = pData;

        if (AllocatedSize() > 0 && IsOwnData())
            edt_free(pData);

        pData = (byte *) edt_alloc(rawiosize);

        if (AllocatedSize() != 0)
            TRACE("Reallocating image from %p to %p old allocation = %d new = %d\n",
            oldp, pData, AllocatedSize(), rawiosize);

        SetSizes(buffersize, activesize, dmasize, rawiosize, file_io_size, rawiosize);

        SetOwnData(true);

    }
    else
    {
        SetSizes(buffersize, activesize, dmasize, rawiosize, file_io_size, AllocatedSize());
    }

    SetValues(pData, nType, nWidth, nHeight, nPitch, AllocatedSize());

    
    SetAllocated(true);
    SetDataChanged();

}

/**
*  
*  @return void 
*/

void 
EdtImage::Destroy(bool clearrows)

{
    if (m_pRows && clearrows)
    {
        delete[] m_pRows;
        m_pRows = NULL;
    }

    if (IsOwnData())
    {
        // Clear Data
        edt_free((byte *) m_base_data);
    }
    m_base_data = NULL;
    SetOwnData(false);
    AllocatedSize(0);

    SetAllocated(false);

}


/**
*  
*  @return void 
*  @param pParent
*  @param  nX
*  @param  nY
*  @param  nWidth
*  @param  nHeight
*  @param  nBand
*/

bool 
EdtImage::AttachToParent(EdtImage *pParent, const int nX, const int nY,
                         const int nWidth, const int nHeight, const byte nBand)

{

    if (pParent && pParent->IsAllocated())
    {
        if (nX >= 0 && nY >= 0 && nX < pParent->GetWidth() && nY < pParent->GetHeight())
        {

            // Get the base address
            byte **pRows = (byte **) pParent->GetPixelRows();
            byte *pData = pRows[nY] + (int) (nX * pParent->GetPixelSize());

            AttachToImageData(pData,pParent->GetType(),nWidth,nHeight,pParent->GetPitch());

            if (pParent->m_bMinMaxSet)
                SetMinMax(pParent->m_nMinPixel, pParent->m_nMaxPixel);

            return true;
        }

    }

    return false;

}

/**
*  
*  @return void 
*  @param pParent
*  @param  nWidth
*  @param  nHeight
*  @param  nType
*/

void 
EdtImage::AttachToImageData(void *pData, 
                       const EdtDataType nType, 
                       const int nWidth, 
                       const int nHeight, 
                       const int nPitch,
                       const u_int nAllocated)

{
    // Destroy any existing data
    Destroy(false);

    // Set values 
    SetType(nType);

    int nUsePitch = nPitch;

    if (nPitch == 0)
    {

        nUsePitch = (GetLineWidth(nWidth));

        if (m_nPadBytes && (nUsePitch % m_nPadBytes)) 
        {
            nUsePitch = ((nUsePitch + m_nPadBytes) & ~m_nPadBytes);
        }

    }

    // Set data
    SetValues(pData, nType, nWidth, nHeight, nUsePitch, nAllocated);

    SetAllocated(true);
    SetOwnData(false);

    BufferSize(nUsePitch * nHeight);

}

/**
*  
*  @return void 
*  @param pParent
*  @param  nWidth
*  @param  nHeight
*  @param  nType
*/

void 
EdtImage::AttachToData(void *pData)

{

    // if already set return right away

    if (pData == m_base_data)
        return;

    // Destroy any existing data	
    Destroy(false);

    if (m_nPitch == 0)
        SetPitch(GetLineWidth(GetWidth()));
    // Set data
    SetValues(pData, GetType(), GetWidth(), GetHeight(), GetPitch(), AllocatedSize());

    SetAllocated(true);
    SetOwnData(false);

}

void EdtImage::SetFirstPixel(void *pData)

{
    m_data = (u_char *) pData;
}

void EdtImage::SetBaseData(void *pData)

{
    m_base_data = (u_char *) pData;
    if (m_header && m_header->HeaderPosition() != HeaderSeparate)
        m_header->BaseAddr(pData);
}

void EdtImage::SetValues(void *pData, 
                         const EdtDataType nType, 
                         const int nWidth, 
                         const int nHeight, 
                         const int nPitch,
                         const u_int nAllocated)

{

    SetWidth(nWidth);
    SetHeight(nHeight);
    SetType(nType);

    if (nPitch)
        SetPitch(nPitch);
    else
        SetPitch(GetLineWidth(nWidth));

    SetContiguous (nPitch == nWidth * m_nPixelSize);

    SetBaseData(pData);

    SetFirstPixel((byte *)pData + GetDataOffset());

    BufferSize(nHeight * GetPitch());

    if (nAllocated)
        AllocatedSize(nAllocated);

    if (BufferSize())
        bsize.AlignSizes();

    UpdatePointers();

}

void EdtImage::SetSubRegion(const int w, const int h)

{
    SetWidth(w);
    SetHeight(h);

    SetContiguous (GetPitch() == w * m_nPixelSize);

    BufferSize(h * GetLineWidth());

    UpdatePointers();

}

void EdtImage::SetSectorSize(unsigned short nPageSize)
{
    m_nSectorSize = (unsigned short) nPageSize;
}

int EdtImage::GetSectorSize()
{
    return m_nSectorSize;
}	

void EdtImage::Resize(const EdtDataType nType, const int nWidth, const int nHeight, bool newData)
{
    if (newData)
    {
        Create(nType, nWidth, nHeight );
    }
    else
    {
        SetType(nType);
        SetWidth(nWidth);
        SetHeight(nHeight);

        UpdatePointers();
    }
}


/**
*  
*  @return void 
*  @param  nValue
*  @param nColor
*/

void
EdtImage::Fill(const int nValue, const int nColor)

{
    //Determine data type

    if (!IsAllocated())
    {
        return;
    }

    FillRegion(nValue, 0,0,GetWidth(), GetHeight(), nColor);

}

/**
*  
*  @return void 
*  @param  nValue
*  @param  int nColor
*/

void
EdtImage::FillRegion(const int nValue,  const int nX, const int nY,
                     const int nWidth, const int nHeight, const int nColor)

{
    //Determine data type

    if (!IsAllocated())
    {
        return;
    }

    switch (GetType())
    {

    case TypeBYTE:
    case TypeBGR:
    case TypeRGB:

        {
            byte **bprows;
            byte bValue = (byte) nValue;

            if (nColor == -1 || GetNColors() == 1)
            {
                if (IsContiguous() && nX == 0 && nY == 0 && 
                    nWidth == GetWidth() && nHeight == GetHeight())
                {
                    memset(GetFirstPixel(), bValue, 
                        BufferSize());
                } 
                else
                {
                    bprows = (byte **) GetPixelRows();

                    for (int y = nY;y < nY + nHeight ;y++)
                    {
                        FillRowTyped(bprows[y]+nX, bValue, nWidth);	

                    }

                }
            } 
            else
            {


                bprows = (byte **) GetPixelRows();


                for (int y = nY;y < nY + nHeight ;y++)
                {
                    FillRowTyped(bprows[y]+nX * GetNColors(), bValue, nWidth, nColor, GetNColors());	
                }

            }

        }
        break;

    case TypeUSHORT:
    case TypeRGB16:
    case TypeRGB15:
        {
            ushort_t **bprows;
            ushort_t bValue = (ushort_t) nValue;

            {
                bprows = (ushort_t **) GetPixelRows();

                for (int y = nY;y < nY + nHeight ;y++)
                {
                    FillRowTyped(bprows[y]+nX, bValue, nWidth);	

                }

            }

        }
        break;
    default:
        // Error - not implemented yet
        TRACE("Error - Fill for type not implemented\n");	
    }

    SetDataChanged();

}

void EdtImage::Ramp(int low, int high, bool bVertical, bool bAscending)

{
    int usehigh, uselow;

    if (low == 0 && high == 0) {
        usehigh = GetMaxValue();
    } else

        usehigh = high;

    if (low > usehigh)
        uselow = usehigh;
    else
        uselow = low;

    int d1;
    double d2;


    int range = abs(usehigh - uselow);
    double binsize;

    int delta_i = 1;

    if (!range)
        return;


    if (bVertical)
    {
        binsize = (double) GetHeight() / (double) range;


        d1 = uselow;
        d2 = (uselow + binsize);
        int ibinsize = (int) binsize;

        if (ibinsize < 1)
        {
            delta_i = (int) (1.0 / binsize);
            binsize = delta_i * binsize;
            ibinsize = 1;
        } 
        else
        {
            if (GetHeight() %  range)
                ibinsize += 1;
        }

        for (int i=0; i < range; i+= delta_i)
        {
            if (d1 >= GetHeight())
                break;

            FillRegion(uselow+ i,0,d1,GetWidth(),ibinsize);
            d1 = (int) d2;
            d2 += binsize; 
        }
    }
    else
    {
        binsize = (double) GetWidth() / (double) range;
        d1 = uselow;
        d2 = (uselow + binsize);
        int ibinsize = (int) binsize;

        if (ibinsize < 1)
        {
            delta_i = (int) (1.0 / binsize);
            binsize = delta_i * binsize;
            ibinsize = 1;
        } 
        else
        {
            if (GetWidth() %  range)
                ibinsize += 1;
        }

        for (int i=0; i < range; i+= delta_i)
        {
            if (d1 >= GetWidth())
                break;

            FillRegion(uselow+i,d1,0, ibinsize, GetHeight());
            d1 = (int) d2;
            d2 += binsize; 
        }
    }

    SetDataChanged();

}

void EdtImage::ColorRamp(int low, int high, bool bVertical, bool bAscending)

{
    int usehigh, uselow;

    if (low == 0 && high == 0) {

        usehigh = GetMaxValue();


    } else

        usehigh = high;

    if (low > usehigh)
        uselow = usehigh;
    else
        uselow = low;

    int d1;
    double d2;


    int range = abs(usehigh - uselow) + 1;
    double binsize;

    if (!range)
        return;

    int color;

    if (bVertical)


    {

        binsize = ((double) GetHeight() / (double) range);
        d1 = uselow;
        d2 = (uselow + binsize);

        int ibinsize = (int) binsize;
        if (GetHeight() %  range)
            ibinsize += 1;

        for (int i=0; i < range; i++)
        {

            FillRegion(uselow+ i,0,d1,GetWidth(),ibinsize,0);
            color = uselow + ((i * 2) % range);
            FillRegion(color,0,d1,GetWidth(),ibinsize,1);
            color = uselow + ((i * 4) % range);
            FillRegion(color,0,d1,GetWidth(),ibinsize,2);
            d1 = (int) d2;
            d2 += binsize; 
        }
    }
    else
    {
        binsize = (double) GetWidth() / (double) range;
        d1 = uselow;
        d2 = (uselow + binsize);

        int ibinsize = (int)binsize;

        if (GetHeight() %  range)
            ibinsize += 1;

        for (int i=0; i < range; i++)
        {

            FillRegion(uselow+i,d1,0, (int) binsize, GetHeight(),0);
            color = uselow + ((i * 2) % range);
            FillRegion(color,d1,0, (int) binsize, GetHeight(),1);
            color = uselow + ((i * 4) % range);
            FillRegion(color,d1,0, (int) binsize, GetHeight(),2);
            d1 = (int) d2;
            d2 += binsize; 
        }
    }

    SetDataChanged();

}

void EdtImage::Clear()
{
    Fill ((int) 0);
}

void EdtImage::Copy(EdtImage *pFrom)

{
    // Find bounding box

    
    int nWidth = GetWidth();
    int nHeight = GetHeight();

  
    if (pFrom)
    {
	if (pFrom->GetWidth() < nWidth)
	    nWidth = pFrom->GetWidth();

	if (pFrom->GetHeight() < nHeight)
	    nHeight = pFrom->GetHeight();

	CopyRegion(pFrom,0,0,nWidth, nHeight);
    }
 
}

void EdtImage::CopyRegion(EdtImage *pFrom, const int nFromX,
                          const int nFromY, const int nFromWidth,
                          const int nFromHeight,
                          const int nToX, const int nToY)

{
    if (!pFrom || !pFrom->IsAllocated() || !IsAllocated())
        return;

    // Clip

    // Sort out types

    int nWidth = nFromWidth;
    int nHeight = nFromHeight;

    if (nWidth + nToX > GetWidth())
    {
        nWidth = GetWidth() - nToX;
    }
    if (nHeight + nToY > GetHeight())
    {
        nHeight = GetHeight() - nToY;
    }

    if (pFrom->GetType() == GetType())
    {
        int bytescopied = (int) (GetPixelSize() * nWidth);

        byte **srcrows = (byte **) pFrom->GetPixelRows();
        byte **rows = (byte **) GetPixelRows();


        if (nFromX == 0 && nToX == 0 && nFromY == 0 && nToY == 0)
        {
            if (bytescopied == pFrom->GetPitch() && bytescopied == GetPitch())
            {
                bytescopied *= nHeight;

                memcpy(GetFirstPixel(), pFrom->GetFirstPixel(), bytescopied);

            }
            else
                for (int row = 0;row < nHeight;row++)
                    memcpy(rows[row],srcrows[row], bytescopied);
        }
        else
        {

            int foffset= (int) (nFromX * GetPixelSize());
            int offset = (int) (nToX * GetPixelSize());

            byte **fbp = (byte **)srcrows;
            byte **bp = (byte **)rows;

            for (int frow = nFromY, row = nToY; frow < nFromY + nHeight;frow++, row++)
                memcpy(bp[row] + offset,fbp[frow] + foffset , bytescopied);

        }

        SetDataChanged();

        return;

    }

    byte **bp;
    ushort_t **sp;

    switch (pFrom->GetType())
    {
    case TypeBYTE:
        switch(GetType())
        {
        case TypeUSHORT:
            bp = (byte **) pFrom->GetPixelRows();
            sp = (ushort_t **) GetPixelRows();

            for (int frow = nFromY, row = nToY; frow < nFromY + nHeight;frow++, row++)
                CopyRowTyped(bp[frow] + nFromX,sp[row] + nToX , nWidth);

            break;

        }
        break;
    case TypeUSHORT:
        switch(GetType())
        {
        case TypeBYTE:
            bp = (byte **) GetPixelRows();
            sp = (ushort_t **) pFrom->GetPixelRows();

            for (int frow = nFromY, row = nToY; frow < nFromY + nHeight;frow++, row++)
                CopyRowTyped(sp[frow] + nFromX,bp[row] + nToX , nWidth);

            break;

        }
        break;
    default:
        break;
    }

    SetDataChanged();
}

void EdtImage::CopyRegionSkip(EdtImage *pFrom, const int nFromX,
                              const int nFromY, const int nFromWidth,
                              const int nFromHeight,
                              const int nToX, const int nToY,
                              const int nToSkip, const int nFromSkip)

{
    if (!pFrom || !pFrom->IsAllocated() || !IsAllocated())
        return;

    if (nToSkip == 1 && nFromSkip == 1)
    {
        CopyRegion(pFrom, nFromX, nFromY, nFromWidth, nFromHeight, 
            nToX, nToY);
        return;
    }
    // Clip

    // Sort out types

    int nWidth = nFromWidth;
    int nHeight = nFromHeight;

    if (nWidth + nToX > GetWidth())
    {
        nWidth = GetWidth() - nToX;
    }
    if (nHeight + nToY > GetHeight())
    {
        nHeight = GetHeight() - nToY;
    }


    byte **bp, **bp2;
    ushort_t **sp, **sp2;

    switch (pFrom->GetType())
    {
    case TypeBYTE:
        switch(GetType())
        {
        case TypeBYTE:
            {
                bp = (byte **) GetPixelRows();
                bp2 = (byte **) pFrom->GetPixelRows();

                for (int frow = nFromY, row = nToY; frow < nFromY + nHeight;frow++, row++)
                    CopyRowTyped(bp2[frow],bp[row] , nWidth,
                    nFromX, nToX,
                    nFromSkip, nToSkip);
            }
            break;
        case TypeUSHORT:
            {
                bp = (byte **) pFrom->GetPixelRows();
                sp = (ushort_t **) GetPixelRows();

                for (int frow = nFromY, row = nToY; frow < nFromY + nHeight;frow++, row++)
                    CopyRowTyped(bp[frow],sp[row] , nWidth,
                    nFromX, nToX,
                    nFromSkip, nToSkip);
            }
            break;

        }
        break;
    case TypeUSHORT:
        switch(GetType())
        {
        case TypeBYTE:
            {
                bp = (byte **) GetPixelRows();
                sp = (ushort_t **) pFrom->GetPixelRows();

                for (int frow = nFromY, row = nToY; frow < nFromY + nHeight;frow++, row++)
                    CopyRowTyped(sp[frow],bp[row] , nWidth,
                    nFromX, nToX,
                    nFromSkip, nToSkip);
            }
            break;
        case TypeUSHORT:
            {
                sp = (ushort_t **) GetPixelRows();
                sp2 = (ushort_t **) pFrom->GetPixelRows();

                for (int frow = nFromY, row = nToY; frow < nFromY + nHeight;frow++, row++)
                    CopyRowTyped(sp2[frow],sp[row] , nWidth,
                    nFromX, nToX,
                    nFromSkip, nToSkip);
            }
            break;

        }
        break;
    }

    SetDataChanged();
}

// Copy a single plane of a color image
//

void EdtImage::CopyPlane(EdtImage *pFrom,
                         byte nTargetPlane, 
                         byte nSourcePlane)

{

}


void EdtImage::CopyRegionPlane(EdtImage *pFrom, 
                               byte nTargetPlane, 
                               byte nSourcePlane,
                               const int nFromX,
                               const int nFromY, 
                               const int nFromWidth,
                               const int nFromHeight,
                               const int nToX, 
                               const int nToY)

{


}

bool
EdtImage::FirstDifference(const EdtImage *pOther,
                          int &x,
                          int &row)
{
    if (Equivalent(pOther))
    {
        int w = GetLineWidth();

        for (row = 0; row < GetHeight() ; row++)
        {

            if (memcmp(m_pRows[row],pOther->m_pRows[row],w))
            {
                u_char *p1 = (u_char *) m_pRows[row];
                u_char *p2 = (u_char *) pOther->m_pRows[row];

                for (x = 0; x< w ;x++)
                    if (p1[x] != p2[x])
                        return true;

                return true;
            }
        }

        return false;
    }
    else
        return true;
}

bool
EdtImage::Equal(const EdtImage *pOther)
{
    if (Equivalent(pOther))
    {

        if (m_bContiguous)
        {

            if (memcmp(GetFirstPixel(), pOther->GetFirstPixel(),
                BufferSize()))
                return false;

        }
        else
        {

            int row;
            for (row = 0; row < GetHeight() ; row++)
            {
                if (memcmp(m_pRows[row],pOther->m_pRows[row],
                    GetLineWidth()))
                    return false;
            }

        }
        return true;
    }
    else
        return false;
}

bool
EdtImage::Equivalent(const EdtImage *pOther)
{
    return (pOther->GetWidth() == GetWidth() &&
        pOther->GetHeight() == GetHeight() &&
        pOther->GetType() == GetType() &&
        pOther->GetNColors() == GetNColors());

}

bool
EdtImage::operator==(const EdtImage &other)
{
    return Equal(&other);
}

bool
EdtImage::operator!=(const EdtImage &other)
{
    return !Equal(&other);
}
// Copy parameters , allocate, and copy data
//

void EdtImage::Duplicate(EdtImage *pSource)

{
    if (pSource)
    {
        Create(pSource->GetType(), pSource->GetWidth(), pSource->GetHeight());
        Copy(pSource);
    }
}

// specify a duplicate
// of source image 
// parameters, but don't copy

void EdtImage::DuplicateType(EdtImage *pSource)

{
    if (pSource)
    {
        Create(pSource->GetType(), pSource->GetWidth(), pSource->GetHeight());
    }
}

// SetPixel Functions

void EdtImage::SetPixel(const int nX, const int nY, const int nValue)

{
    if (nX < 0 || nX >= GetWidth() ||
        nY < 0 || nY >= GetHeight())
        return;

    if (IsAllocated())
    {
        switch(GetType())
        {
        case TypeBYTE:
            {
                byte **bpRows = (byte **) GetPixelRows();
                bpRows[nY][nX] = (byte) nValue;
            }
            break;
        case TypeUSHORT:
            {
                ushort_t **bpRows = (ushort_t **) GetPixelRows();
                bpRows[nY][nX] = (ushort_t) nValue;
            }
            break;			
        case TypeINT:
            {
                int **bpRows = (int **) GetPixelRows();
                bpRows[nY][nX] = nValue;
            }
            break;
        case TypeBGR:
            {
                byte **bpRows = (byte **) GetPixelRows();
                u_int v = ((u_int) nValue) & 0xffffff;
                bpRows[nY][nX*3+2] = v >> 16;
                bpRows[nY][nX*3+1] = (v >> 8) & 0xff;
                bpRows[nY][nX*3+0] = v & 0xff;
            }
            break;

        case TypeBGRA:
            {
                byte **bpRows = (byte **) GetPixelRows();
                u_int v = ((u_int) nValue);
                bpRows[nY][nX*4+3] = (v >> 24) & 0xff;
                bpRows[nY][nX*4+2] = (v >> 16) & 0xff;
                bpRows[nY][nX*4+1] = (v >> 8) & 0xff;
                bpRows[nY][nX*4+0] = v & 0xff;

            }
            break;
        case TypeRGB:
            {
                byte **bpRows = (byte **) GetPixelRows();
                u_int v = ((u_int) nValue) & 0xffffff;
                bpRows[nY][nX*3+0] = v >> 16;
                bpRows[nY][nX*3+1] = (v >> 8) & 0xff;
                bpRows[nY][nX*3+2] = v & 0xff;
            }
            break;

        case TypeRGBA:
            {
                byte **bpRows = (byte **) GetPixelRows();
                u_int v = ((u_int) nValue);
                bpRows[nY][nX*4+0] = (v >> 24) & 0xff;
                bpRows[nY][nX*4+1] = (v >> 16) & 0xff;
                bpRows[nY][nX*4+2] = (v >> 8) & 0xff;
                bpRows[nY][nX*4+3] = v & 0xff;
            }
            break;

        }

    }    
}

void EdtImage::SetPixel(const int nX, const int nY, const double dValue)

{

}

// GetPixel Functions

void EdtImage::GetPixel(const int nX, const int nY, int &nValue)

{
    if (nX < 0 || nX >= GetWidth() ||
        nY < 0 || nY >= GetHeight())
        return;

    if (IsAllocated())
    {
        switch(GetType())
        {
        case TypeBYTE:
            {
                byte **bpRows = (byte **) GetPixelRows();
                nValue = bpRows[nY][nX];
            }
            break;
        case TypeUSHORT:
            {
                ushort_t **bpRows = (ushort_t **) GetPixelRows();
                nValue = bpRows[nY][nX];
            }
            break;			
        case TypeINT:
            {
                int **bpRows = (int **) GetPixelRows();
                nValue = bpRows[nY][nX];
            }
            break;
        case TypeBGR:
            {
                byte **bpRows = (byte **) GetPixelRows();
                nValue = ((int) bpRows[nY][nX*3] << 16) | ((int)bpRows[nY][nX*3 + 1] << 8) | 
                    ((int) bpRows[nY][nX*3+2]);
            }
            break;

        case TypeBGRA:
            {
                byte **bpRows = (byte **) GetPixelRows();
                nValue = ((int) bpRows[nY][nX*4] << 16) | ((int)bpRows[nY][nX*4 + 1] << 8) | 
                    ((int) bpRows[nY][nX*4+2]);

                int fullvalue = ((u_int *) (bpRows[nY]))[nX];
            }
            break;
        case TypeRGB:
            {
                byte **bpRows = (byte **) GetPixelRows();
                nValue = ((int) bpRows[nY][nX*3+2] << 16) | ((int)bpRows[nY][nX*3 + 1] << 8) | 
                    ((int) bpRows[nY][nX*3]);
            }
            break;

        case TypeRGBA:
            {
                byte **bpRows = (byte **) GetPixelRows();
                nValue = ((int) bpRows[nY][nX*4+2] << 16) | ((int)bpRows[nY][nX*4 + 1] << 8) | 
                    ((int) bpRows[nY][nX*4]);

                int fullvalue = ((u_int *) (bpRows[nY]))[nX];
            }
            break;

        }

    }

}

int EdtImage::GetPixelV(const int nX, const int nY, int *values)

{
    if (nX < 0 || nX >= GetWidth() ||
        nY < 0 || nY >= GetHeight())
        return 0;

    byte **bpRows = (byte **) GetPixelRows();

    if (!bpRows)
        return 0;

    int band;

    if (IsAllocated())
    {
        switch(GetType())
        {
        case TypeBYTE:
            {
                values[0] = bpRows[nY][nX];
            }
            break;
        case TypeUSHORT:
            {
                ushort_t **bpRows = (ushort_t **) GetPixelRows();
                values[0] = bpRows[nY][nX];
            }
            break;			
        case TypeUINT:
            {
                u_int **lpRows = (u_int **) GetPixelRows();
                values[0] = lpRows[nY][nX];
            }
            break;
        case TypeBGR:
            {
                for (band = 2; band >= 0; band--)
                    values[band] = bpRows[nY][nX*3 + 2-band];
            }
            break;

        case TypeBGRA:
            {
                for (band = 3; band >= 0; band--)
                    values[band] = bpRows[nY][nX*4 + 3-band];
            }
            break;
        case TypeRGB:
            {
                for (band = 0; band < 3; band++)
                    values[band] = bpRows[nY][nX*3 + band];

            }
            break;

        case TypeRGBA:
            {
                for (band = 0; band < 4; band++)
                    values[band] = bpRows[nY][nX*4 + band];
            }
            break;

        }

    }

    return GetNColors();
}


void EdtImage::GetPixel(const int nX, const int nY, double &dValue)

{
    int v;

    GetPixel(nX, nY, v);

    dValue = v;

}

// Keep first row pointers 
// Only re-compute if we have new data pointer or larger size (so we can 
//  reduce the number of rows w/ little penalty
//

void EdtImage::UpdatePointers()

{
    byte **pRows;

    if (!GetFirstPixel())
    {
        return;
    }

    if (m_pRows)
    {
        // Only de-allocate if we need more space
        if (m_nFullRows < GetHeight())
        {
            delete[] m_pRows;
            m_pRows = NULL;
        } 
    }

    if (m_pRows)
    {
        pRows = (byte **) m_pRows;
    }
    else
    {
        pRows = new byte *[GetHeight()];
        pRows[0] = NULL;

        m_nFullRows = GetHeight();
    }

    m_pRows = (void **) pRows;

    byte *bp = (byte *) GetFirstPixel();

    int nPitch = GetPitch();

    if (IsInverted())
    {
        bp = bp + (GetHeight()-1) * nPitch;
        nPitch = -nPitch;
    }

    if (bp == pRows[0] &&  m_nLastPitch == nPitch)
        // We don't have to do anything
        return;

    for (int i=0;i< GetHeight();i++)
    {
        pRows[i] = bp;
        bp += nPitch;
    }

    m_nLastPitch = nPitch;

}

void **
EdtImage::GetPixelRows()
{
    return m_pRows;
}


bool EdtImage::IsOwner(const void *o)
{
    return (o == m_owner);
}

int EdtImage::GetDataOffset()
{
    return bsize.DataOffset();
}

int EdtImage::GetExtraHeaderSize()
{

    return bsize.ExtraSize();
}

bool EdtImage::IsAllocated() const 
{
    return m_bAllocated;
}

void EdtImage::SetAllocated(const bool bAllocated) 
{
    m_bAllocated = bAllocated;
}

bool EdtImage::IsDisplayable() const 
{
    return m_bDisplayable;
}
void EdtImage::SetDisplayable(const bool bDisplayable) 
{
    m_bDisplayable = bDisplayable;
}


bool EdtImage::IsContiguous() const 
{
    return m_bContiguous;
}
void EdtImage::SetContiguous(const bool bContiguous) 
{
    m_bContiguous = bContiguous;
}


bool EdtImage::IsInverted() const 
{
    return m_bInverted;
}
void EdtImage::SetInverted(const bool bInverted) 
{
    m_bInverted = bInverted;
}

bool EdtImage::IsOwnData() const 
{
    return m_bOwnData;
}
void EdtImage::SetOwnData(const bool bOwnData) 
{
    m_bOwnData = bOwnData;
}

// Data pointer at beginning of header data

void *EdtImage::GetFirstPixel() const
{
    return m_data;
}

// Data pointer at beginning of allocated data 

void *EdtImage::GetBaseData() 
{
    return m_base_data;
}

const bool EdtImage::IsDataChanged() const 
{
    return m_bDataChanged;
}

void EdtImage::SetDataChanged() 
{
    m_bDataChanged = true;
}

void EdtImage::SetDataClean() 
{
    m_bDataChanged = false;
}


//
// Sequence Number is used by the file read code to keep track of 
// an image's index in the file
//

void EdtImage::SetSequenceNumber(const int n)
{
    m_nSequenceNumber = n;
}

int EdtImage::GetSequenceNumber() const
{
    return m_nSequenceNumber;
}

char *EdtImage::GetDescription()
{
    return NULL;
}

bool EdtImage::SameSize(const EdtImage *pOther) const
{
    return (pOther->GetWidth() == GetWidth() && 
        pOther->GetHeight() == GetHeight());
}

u_char *EdtImage::GetDataOnly()

{
    return m_data;
}

u_char *EdtImage::GetData()

{
    return m_base_data;
}


void EdtImage::InitHeader()

{
    if (m_header && HeaderSize())
    {
        m_header->Setup(HeaderPosition(), HeaderSize(), HeaderOffset());

        m_header->BufferId(m_buf_id);

    }
}

void EdtImage::SetTimeStamp(const double t)
{
    m_timestamp = t;
    if (m_header)
        m_header->TimeStamp(t);

}

double EdtImage::GetTimeStamp()
{
    return m_timestamp;
}

void EdtImage::Create(const EdtDataType t, const int size)
{
    Create(t, size, 1);
}

void EdtImage::AttachToData(u_char *p, const EdtDataType t, const int width, const int height, const u_int allocated)
{
    SetType(t);

    SetWidth(width);
    SetHeight(height);

    u_int size = (u_int) (width * height * TypeSize(t));

    AttachToData(p,size, allocated);

}


void EdtImage::AttachToData(u_char *p, const int size, const u_int allocated)
{
    if (IsAllocated())
        Destroy();

    bsize.SetSizes(size, allocated);

    AttachToData(p);

}

void EdtImage::SetOwner(void *p)
{
    m_owner = p;
}
void *EdtImage::GetOwner()
{
    return m_owner;
}   

void EdtImage::SetUser(void *user)
{
    m_user = user;
}

void *EdtImage::GetUser()
{
    return m_user;
}

void EdtImage::SetHeaderStr(const char *s)

{
    if (s && m_header)
    {
        m_header->SetComment(s);
    }
}

void EdtImage::AllocateTrace(const int maxvalues)

{

    if (trace && (max_trace_values == maxvalues))
        return;

    if (trace)
        free(trace);

    if (maxvalues)
    {
        trace = (DBufTrace *) calloc(maxvalues, sizeof(DBufTrace));
    }
    else
        trace = NULL;

    max_trace_values = maxvalues; 

    n_trace_elements = 0;

}

void EdtImage::TraceState()

{

    if (n_trace_elements < max_trace_values)
    {
        int n = n_trace_elements++;

        trace[n].t = edt_timestamp();
        trace[n].m_owner_id = m_owner_id;
        trace[n].state = current_state;
        trace[n].ready = m_ready;
    }
}

static const char *state_name(BufferStates state)

{
    switch (state)
    {
    case BufIdle:
        return "Idle";
    case BufToRead:
        return "ToRead";
    case BufToWrite:
        return "ToWrite";
    case BufReading:
        return "Reading";
    case BufWriting:
        return "Writing";
    case BufPendingWrite:
        return "PendWrite";
    case BufPendingRead:
        return "PendRead";
    
    default:
        return "Unknown";
    }
}

void EdtImage::SetState(int new_owner, BufferStates state)

{
    BufferStates old = current_state;
    current_state = state;
    //printf("%2d: %s -> %s\n", GetBufId(), state_name(old), state_name(current_state));
    m_owner_id  = new_owner;
}

void EdtImage::SetBufId(const int n)
{
    m_buf_id = n;
    if (m_header)
        m_header->BufferId(n);
}

int  EdtImage::GetBufId()
{
    return m_buf_id;
}

EdtDataHeader *EdtImage::GetHeader()

{
    return m_header;
}

/**
 * \brief
 * FIX !!!  Write brief comment for SetHeader here.
 * 
 * \param 
 * FIX !!! Description of parameter .
 * 
 */
void EdtImage::SetHeader(EdtDataHeader *phdr)

{
    m_header = phdr;

    if (phdr)
    {
        SetHeaderValues(phdr);

        phdr->ImageSize(BufferSize());
        if (GetBaseData() && phdr->HeaderPosition() != HeaderSeparate)
            phdr->BaseAddr(GetBaseData());
    }


}

void EdtImage::CloneHeader(EdtDataHeader *phdr)

{
    m_header = phdr->Clone();
}

void EdtImage::SetSerialNo(const int n)
{
    m_serial_no = n;
    if (m_header)
        m_header->Frame(n);
}

int  EdtImage::GetSerialNo()
{
    return m_serial_no;
}
void EdtImage::SetReady(const bool state)
{
    m_ready = state;
}

bool EdtImage::Ready()
{
    return m_ready;
}
void EdtImage::Grab(int new_owner, BufferStates state)
{
    SetState(new_owner, state);
    SetReady(false);
//    TraceState();
}

void EdtImage::Release(BufferStates state)

{
    SetState(m_owner_id, state);
    SetReady(true);
//    TraceState();
}

int EdtImage::Owner()
{
    return m_owner_id;
}

int EdtImage::GetNTrace()
{
    return n_trace_elements;
}

DBufTrace *EdtImage::GetTrace()
{
    return trace;
}
BufferStates EdtImage::GetState()
{
    return current_state;
}
void EdtImage::ResetTrace()
{
    n_trace_elements = 0;
}


void EdtImage::BufferSizePrint(const char *tag)

{
    char tagbuf[128];

    sprintf(tagbuf,"%s %8p %6x\n", 
        tag, GetBaseData(), HeaderOffset());

    EdtImageData::BufferSizePrint(tag, cout);

}
