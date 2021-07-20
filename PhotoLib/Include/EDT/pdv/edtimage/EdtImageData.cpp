/** ********************************
* 
* @file EdtImageData.cpp
*
* Copyright (c) Engineering Design Team 2010
* All Rights Reserved
*/


/// @file EdtImageData.cpp: implementation of the EdtImageData class.
///
//
//////////////////////////////////////////////////////////////////////

#include "EdtImageData.h"

//////////////////////////////////////////////////////////////////////
/// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void EdtImageData::InitValues()
{
    m_nWidth		= 0;
    m_nHeight		= 0;
    m_nX		= 0;
    m_nY		= 0;

    m_nPitch		= 0;
    m_nType		= TypeBYTE;

    m_nPixelSize	= 1;

    m_bMinMaxSet	= false;

    m_nColors		= 1;
    m_nColorTag		= COLOR_MONO;


    m_nDepth = 0;
    m_nMaxPixel = -1;
    m_nMinPixel = 0;

    m_nHBlank = 0;
    m_nVBlank = 0;

    m_timestamp = 0;

    m_header = NULL;

}

EdtImageData::EdtImageData(const char *nm)
{
    InitValues();
}

EdtImageData::EdtImageData(const EdtImageData &from, const char *nm) 
{
    InitValues();
    SetBufferValues(from);
}

EdtImageData::~EdtImageData()

{

}

//////////////////////////////
/// 

void
EdtImageData::SetType(const EdtDataType nType)

{
    m_nType = nType;
    switch (m_nType)
    {
    case TypeCHAR:
    case TypeBYTE:
        m_nPixelSize =1;
        break;

    case TypeSHORT:
    case TypeUSHORT:
        m_nPixelSize = 2;
        break;

    case TypeRGB16:
    case TypeRGB15:
        m_nPixelSize = 2;
        m_nColors = 3;
        break;

    case TypeRGB:
    case TypeBGR:
        m_nPixelSize = 3;
        m_nColors = 3;
        break;

    case TypeRGBA:
    case TypeBGRA:
        m_nPixelSize = 4;
        m_nColors = 3;
        break;

    case TypeDOUBLE:
        m_nPixelSize = 8;
        break;

    case TypeMONO:
        m_nPixelSize = 0.125;
        break;

    default:
        m_nPixelSize = 4;
    }

    m_nDepth = (int) (m_nPixelSize * 8);

}


unsigned int EdtImageData::GetMaxValue()
{

    if (m_bMinMaxSet)
    {
        return m_nMaxPixel;
    }

    switch (GetType())
    {
    case TypeCHAR:
        return 127;
    case TypeBYTE:
        return 255;
    case TypeSHORT:
        return 32767;
    case TypeUSHORT:
        return 0xffff;
    case TypeINT:
        return 0x7fffffff;
    case TypeUINT:
        return 0xffffffff;

    }

    return 0xff;

}

int EdtImageData::GetMinValue()
{
    if (m_bMinMaxSet)
    {
        return m_nMinPixel;
    }

    switch (GetType())
    {
    case TypeCHAR:
        return -128;
    case TypeBYTE:
        return 0;
    case TypeSHORT:
        return -32768;
    case TypeUSHORT:
        return 0;
    case TypeINT:
        return -((int) 0x7fffffff);
    case TypeUINT:
        return 0;

    }

    return 0;

}


int EdtImageData::GetNPixels()
{
    return GetWidth() * GetHeight();
}


void EdtImageData::GetTypeSizeAndColors(const EdtDataType nType, int &nPixelSize, int &nColors)
{
    nColors = 1;

    switch (nType)
    {
    case TypeMONO:
    case TypeCHAR:
    case TypeBYTE:
        nPixelSize =1;
        break;

    case TypeSHORT:
    case TypeUSHORT:
        nPixelSize = 2;
        break;

    case TypeRGB:
    case TypeBGR:
        nPixelSize = 3;
        nColors = 3;
        break;

    case TypeRGBA:
    case TypeBGRA:
        nPixelSize = 4;
        nColors = 3;
        break;

    case TypeRGB16:
    case TypeRGB15:
        nPixelSize = 2;
        nColors = 3;
        break;

    case TypeDOUBLE:
        nPixelSize = 8;
        break;

    default:
        nPixelSize = 4;
    }
}


int EdtImageData::GetTypeBytes(EdtDataType nType)
{
    int size, ncolors;

    GetTypeSizeAndColors(nType,size,ncolors);

    return size;
}

/// SetBufferValues - copy values from another EdtImageData structure
//

void EdtImageData::SetBufferValues(const EdtImageData &from)

{
    SetBufferValues(from.GetType(), from.GetWidth(), from.GetHeight());

    m_nX = from.m_nX;
    m_nY = from.m_nY;

    m_nDepth = from.m_nDepth;

    bsize = from.bsize;

    m_nPitch = from.m_nPitch;
    m_nMinPixel = from.m_nMinPixel;
    m_nMaxPixel = from.m_nMaxPixel;

    m_bMinMaxSet = from.m_bMinMaxSet;
    m_nColors = from.m_nColors;
    m_nColorTag = from.m_nColorTag;

    m_timestamp = from.m_timestamp;

}

int EdtImageData::GetWidth() const 
{
    return m_nWidth;
}
void EdtImageData::SetWidth(const int nWidth) 
{
    m_nWidth = nWidth;
    if (!m_nPitch)
        m_nPitch = nWidth * m_nPixelSize;
}

int EdtImageData::GetHeight() const 
{
    return m_nHeight;
}

void EdtImageData::SetHeight(const int nHeight) 
{
    m_nHeight = nHeight;
}

/// added for simulation

int EdtImageData::GetVBlank() const 
{
    return m_nVBlank;
}

void EdtImageData::SetVBlank(const int nlines) 
{
    m_nVBlank = nlines;
}

int EdtImageData::GetHBlank() const 
{
    return m_nHBlank;
}
void EdtImageData::SetHBlank(const int nclocks) 
{
    m_nHBlank = nclocks;
}

EdtDataType EdtImageData::GetType() const 
{
    return m_nType;
}

int EdtImageData::GetPitch() const 
{
    return m_nPitch;
}
void EdtImageData::SetPitch(const int nPitch) 
{
    m_nPitch = nPitch;
}

int EdtImageData::GetPitchPixels() const 
{
    return (int) (GetPitch() / GetPixelSize());
}

void EdtImageData::SetPitchPixels(const int nPitchPixels)
{
    if (m_nType == EDT_TYPE_MONO)
        SetPitch( GetPitch() >> 3);
    else 
        SetPitch((int) (nPitchPixels * GetPixelSize()));
}

int EdtImageData::GetLineWidth(int nWidth) const
{
    if (nWidth == 0)
        nWidth = m_nWidth;
    return (int) (nWidth * GetPixelSize());
}

float EdtImageData::GetPixelSize() const 
{
    return m_nPixelSize;
}

int EdtImageData::GetNColors() const
{
    return m_nColors;
}

int EdtImageData::GetDepth()
{
    return m_nDepth;
}

/// Depth in bits

void EdtImageData::SetDepth(int nDepth)
{
    m_nDepth = (byte)nDepth;
    if (nDepth < 32)
    {
        SetMinMax(0, (1 << nDepth)-1);
    }
    else
        SetMinMax(0, 0xFFFFFFFF);

    if (m_nType == TypeNull)
    {
        if (nDepth <= 8)
            m_nType = TypeBYTE;
        else if (nDepth <= 16)
            m_nType = TypeUSHORT;
        else if (nDepth <= 24)
            m_nType = TypeRGB;
        else 
            m_nType = TypeRGBA;
    }
}

void EdtImageData::SetMinMax(const int nMin, const int nMax)
{
    m_nMaxPixel = nMax;
    m_nMinPixel = nMin;
    m_bMinMaxSet = true;
}

void EdtImageData::SetBufferValues(const EdtDataType nType, 
                                   const int nWidth, const int nHeight)

{
    m_nWidth = nWidth;
    m_nHeight = nHeight;
    m_nType = nType;

    SetPixelSize();

    BufferSize((int)(nWidth * nHeight * m_nPixelSize));

    bsize.SetSizes(BufferSize());

}

void EdtImageData::GetBufferValues(EdtDataType &nType, int &nWidth, int &nHeight) const
{
    nWidth = m_nWidth;
    nHeight = m_nHeight;
    nType = m_nType;
}

void EdtImageData::SetTimeStamp(const double dTimestamp)
{
    m_timestamp = dTimestamp;
}

double EdtImageData::GetTimeStamp() const
{
    return m_timestamp;
}


int EdtImageData::BufferSize()

{
    return bsize.BufferSize();
}

void EdtImageData::BufferSize(const int size)

{
    bsize.BufferSize(size);
}


int EdtImageData::ActiveSize()

{
    return bsize.ActiveSize();
}

void EdtImageData::ActiveSize(const int size)

{
    bsize.ActiveSize(size);
}

int EdtImageData::DMASize()

{
    return bsize.DMASize();
}

void EdtImageData::DMASize(const int size)

{
    bsize.DMASize(size);
}

int EdtImageData::RawIOSize()

{
    return bsize.RawIOSize();
}

void EdtImageData::RawIOSize(const int size)

{
    bsize.RawIOSize(size);
}

int EdtImageData::FileIOSize()

{
    return bsize.FileIOSize();
}

void EdtImageData::FileIOSize(const int size)

{
    bsize.FileIOSize(size);
}

int EdtImageData::AllocatedSize()

{
    return bsize.AllocatedSize();
}

void EdtImageData::AllocatedSize(const int size)

{
    bsize.AllocatedSize(size);

}

int EdtImageData::Granularity()

{
    return bsize.Granularity();
}

void EdtImageData::Granularity(const int size)

{
    bsize.Granularity(size);

}

void EdtImageData::HeaderPosition(const HdrPosition newtype)
{
    bsize.HeaderPosition(newtype);
}

void EdtImageData::HeaderOffset(const int offset)

{
    bsize.HeaderOffset(offset);
}

void EdtImageData::HeaderSize(const int newsize)
{
    bsize.HeaderSize(newsize);
}

int EdtImageData::HeaderSize() 
{
    return bsize.HeaderSize();
}

int EdtImageData::HeaderOffset()
{
    return bsize.HeaderOffset();
}

HdrPosition EdtImageData::HeaderPosition()
{
    return bsize.HeaderPosition();
}

void EdtImageData::SetHeaderValues(HdrPosition newtype, const int newsize, const int offset)

{
    HeaderPosition(newtype);
    HeaderSize(newsize);
    HeaderOffset(offset);
}

void EdtImageData::SetHeaderValues(EdtDataHeader *phdr)

{
    HeaderPosition(phdr->HeaderPosition());
    HeaderSize(phdr->Size());
    HeaderOffset(phdr->Offset());
}

void EdtImageData::SetHeaderValues(EdtImageData *other)

{
    HeaderPosition(other->HeaderPosition());
    HeaderSize(other->HeaderSize());
    HeaderOffset(other->HeaderOffset());
}

void EdtImageData::SetSizes(const u_int buf_size,
                            const u_int act_size,
                            const u_int dma_size,
                            const u_int raw_size,
                            const u_int file_io_size,
                            const u_int alloc_size)

{
    bsize.SetSizes(buf_size,  act_size, dma_size, raw_size, file_io_size, alloc_size);
}

void EdtImageData::AlignSizes()
{
    bsize.AlignSizes();
}

void EdtImageData::SetupHeader(HdrPosition pos, const u_int header_size, int offset)

{

    SetHeaderValues(pos, header_size, offset);

    if (m_header)
    {
        m_header->Setup(pos, header_size, offset);
    }
}

void EdtImageData::Header(EdtDataHeader *phdr)
{
    m_header = phdr;
}

EdtDataHeader * EdtImageData::Header()

{
    return m_header;
}

void EdtImageData::BufferSizePrint(const char *tag, ostream &out)

{
    char buf[80];

    bsize.Print(tag, buf);

    out << buf;

}


void EdtImageData::PrintState(ostream & out)

{
    out << "EdtImageData" << endl;

    
    BufferSizePrint("Sizes", out);

    out << "nWidth             " << m_nWidth << endl;
    out << "nHeight            " << m_nHeight << endl;
    out << "nX                 " << m_nX << endl;
    out << "nY                 " << m_nY << endl;
    out << "nHBlank            " << m_nHBlank << endl;
    out << "nVBlank            " << m_nVBlank << endl;
    out << "nPitch             " << m_nPitch << endl;
    out << "nMaxPixel          " << m_nMaxPixel << endl;
    out << "nMinPixel          " << m_nMinPixel << endl;
    out << "nPixelSize         " << m_nPixelSize << endl;
    out << "nType              " << m_nType << endl;
    out << "bMinMaxSet         " << m_bMinMaxSet << endl;
    out << "nColors            " << (int) m_nColors << endl;
    out << "nColorTag          " << (int) m_nColorTag << endl;
    out << "nDepth             " << (int) m_nDepth << endl;
    out << "header             " << m_header << endl;
    out << "timestamp          " << m_timestamp << endl;

    out << endl; 
}
