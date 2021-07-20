/** ********************************
 * 
 *
 * Copyright (c) Engineering Design Team 2010
 * All Rights Reserved
 */


#include "edt_utils.h"

#include "EdtDataHeader.h"

// Base class constructor

EdtDataHeader::EdtDataHeader()
{
    m_hdr_size = 0;
    m_data_size = 0;
    m_hdr_pos = HeaderSeparate;
    m_base_addr = 0;
    m_hdr_offset = 0;
    m_own_data = false;
    m_header_type = 0;
    m_padding = 0;
    m_imagesize = 0;
}

EdtDataHeader::~EdtDataHeader()
{
    if (m_own_data && m_base_addr)
        DeAllocate();

    m_hdr_size = 0;
    m_data_size = 0;
    m_hdr_pos = HeaderSeparate;
    m_base_addr = 0;
    m_hdr_offset = 0;
    m_own_data = false;
}


/**
 * \brief
 * Set the base address for the data this header class points to.
 * 
 * \param p
 * Start address of the data buffer
 * 
 */

void EdtDataHeader::BaseAddr(void *p)

{
    if (m_own_data && m_base_addr)
    {
        DeAllocate();
        m_base_addr = p;
    }
    else
    {
        m_base_addr = p;
        m_own_data = false;
    }
}

/**
 * \brief
 * Get the base address for header and optionally
 * the data buffer
 * 
 * \returns
 * Base address pointer
 * 
 */

void *EdtDataHeader::BaseAddr()

{

     return m_base_addr;
}


/**
 * \brief
 * Get the header size
 * 
 * \returns
 * Header size in bytes
 * 
 */
u_int EdtDataHeader::Size()

{
    return m_hdr_size;
}

/**
 * \brief
 * Set the header size
 * 
 * \param size
 * Header size in bytes
 * 
 */
void EdtDataHeader::Size(const u_int size)

{
    if (m_own_data && m_base_addr)
    {
        if (m_hdr_size != size)
            DeAllocate();
        else
            return;

        m_hdr_size = size;
        Allocate();

    }
    else
        m_hdr_size = size;
    
}

/**
 * \brief
 * Get the defined full buffer size
 * 
 * \returns
 * size in bytes
 * 
 */
u_int EdtDataHeader::DataSize()
{
    return m_data_size;
}

/**
 * \brief
 * This is the full size of the buffer including header and padding
 * 
 * \param size
 * Total size in bytes.
 * 
 */
void EdtDataHeader::DataSize(const u_int size)

{
    m_data_size = size;

}


/**
 * \brief
 * If m_data_size is set, set the header offset within the data buffer 
 * 
 * 
 */

void EdtDataHeader::ComputeOffset()

{
    if (m_data_size)
    switch(m_hdr_pos)
    {
    case HeaderBefore:
        m_hdr_offset = -1 * (int) m_hdr_size;
        break;
    case HeaderBegin:
        m_hdr_offset = 0;
        break;
    case HeaderEnd:
        if (m_imagesize)
            m_hdr_offset = m_imagesize;
        else
            m_hdr_offset = 0;
        break;

    case HeaderAfter:
        if (m_imagesize)
            m_hdr_offset = m_imagesize;
        else
            m_hdr_offset = 0;
        break;

    case HeaderSeparate:
        m_hdr_offset = 0;
        break;

    case HeaderMiddle:
        break;

    default:
        break;
    }

}

/**
 * \brief
 * For header contiguous with data, return offset from data 
 * buffer in bytes
 * 
 * \param offset
 * Offset from data beginning in bytes. Can be
 * negative for HeaderBefore position
 * 
 */
void EdtDataHeader::Offset(const int offset)
{
    m_hdr_offset = offset;
}

/**
 * \brief
 * 
 * 
 * \returns
 * Offset in bytes from data buffer start
 * 
 */
/**
 * \brief
 * FIX !!!  Write brief comment for Offset here.
 * 
 * \returns
 * FIX !!! Write description of return value here.
 * 
 */
/**
 * \brief
 * FIX !!!  Write brief comment for Offset here.
 * 
 * \returns
 * FIX !!! Write description of return value here.
 * 
 */
int EdtDataHeader::Offset()

{
    return m_hdr_offset;
}

/**
 * \brief
 * Set the header position relative to the data
 * 
 * \param pos
 * One of enumerated type HdrPosition
 * 
 */
void EdtDataHeader::HeaderPosition(const HdrPosition pos)

{
    m_hdr_pos = pos;
    
}

/**
 * \brief
 * Get the current position type
 * 
 * \returns
 * Enumerated position value
 * 
 */
HdrPosition EdtDataHeader::HeaderPosition()

{
    return m_hdr_pos;
}

/**
 * \brief
 * Checks whether data is within defined DMA buffer
 * 
 * \returns
 * true if the header is within DMA buffer, false otherwise
 * 
 */
bool EdtDataHeader::HeaderWithin()

{
    return m_hdr_pos == HeaderBegin || m_hdr_pos == HeaderMiddle || m_hdr_pos == HeaderEnd;
}

/**
 * \brief
 * Checks whether header is contiguous with data buffer
 * 
 * \returns
 * true if contiguous.
 * 
 */
bool EdtDataHeader::IsContiguous()
{
    return m_hdr_pos != HeaderSeparate;
}

/**
 * \brief
 * Initialize header values, including an offset
 * 

 * 
 */

void EdtDataHeader::Setup(HdrPosition hdr_pos, const u_int size, const int offset)

{
    Size(size);
    Offset(offset);
    HeaderPosition(hdr_pos);
    ComputeOffset();
}

/**
 * \brief
 * Initializes header values; offset is computed
 *
 * 
 */

void EdtDataHeader::Setup(HdrPosition hdr_pos, const u_int size)

{
    Size(size);
    HeaderPosition(hdr_pos);
    ComputeOffset();
}

/**
 * \brief
 * Duplicate settings for another header
 * 
 * 
 */
void EdtDataHeader::CopySettings(EdtDataHeader *other)

{
    if (other)
    {
        Size(other->Size());
        Offset(other->Offset());
        HeaderPosition(other->HeaderPosition()); 
    }
}

/**
 * \brief
 * Allocate memory for the header (non-contiguous with data)
 * 
 */
void EdtDataHeader::Allocate()

{
    if (m_hdr_size)
    {
        if (m_own_data && m_base_addr)
        {
            DeAllocate();
        }
        
        m_own_data = true;
        m_base_addr = edt_alloc_aligned(m_hdr_size);
        memset(m_base_addr, 0, m_hdr_size);

    }
    else
    {
        if (m_own_data && m_base_addr)
            DeAllocate();
    }
}

/**
 * \brief
 * Free memory if it is owned by this object
 * 
 */
void EdtDataHeader::DeAllocate()

{

    if (m_own_data && m_base_addr)
        edt_free_aligned(m_base_addr);

    m_own_data = false;
    m_base_addr = NULL;


}

/**
 * \brief
 * return the pointer to the beginning of header
 * 
 */
void *EdtDataHeader::GetHeaderPtr()

{
    return (u_char *) m_base_addr + Offset();
}

/**
 * \brief
 * Returns size outside of DMA area, depending on 
 * header position
 *
 * 
 */
int EdtDataHeader::GetExtraSize()

{
    switch(m_hdr_pos)
    {
    case HeaderBefore:
    case HeaderAfter:
    case HeaderBegin:
    case HeaderEnd:
        return m_hdr_size;
    default:
        break;
    }

    return 0;
}

int EdtDataHeader::GetExtraDMASize()

{
    switch(m_hdr_pos)
    {
    case HeaderBegin:
    case HeaderEnd:
        return m_hdr_size;
    default:
        break;
    }

    return 0;
}

/**
 * \brief
 * place holder for derived classes
 * 
 */
void EdtDataHeader::Process(const double t)

{
    TimeStamp(t);
}

/**
 * \brief
 * Sets the header type - an open ended integer tag 
 * 
 * 
 */
void EdtDataHeader::HeaderType(const int t)

{
    m_header_type = t;
}

/**
 * \brief
 * Returns the header type - an open ended integer tag 
 * 
 * 
 */
int  EdtDataHeader::HeaderType()

{
    return m_header_type;
}

/**
 * \brief
 * Returns padding value - since total size of a 
 * buffer in the file may be rounded up to next 
 * 512 byte boundary (sector size)
 * 
 * 
 */
u_int EdtDataHeader::Padding()

{
    return m_padding;
}

/**
 * \brief
 * Sets padding value - since total size of a 
 * buffer in the file may be rounded up to next 
 * 512 byte boundary (sector size)
 * 
 * 
 */
void EdtDataHeader::Padding(const u_int size)

{
    m_padding = size;
}

/**
 * \brief
 * Returns size of the actual data portion of the buffer
 * 

 * 
 */
u_int EdtDataHeader::ImageSize()

{
    return m_imagesize;
}

/**
 * \brief
 * Sets size of the actual data portion of the buffer
 * 
 */
void EdtDataHeader::ImageSize(const u_int size)

{
    m_imagesize = size;
    ComputeOffset();

}

void  EdtDataHeader::ChannelIndex(const u_int val)

{

}

u_int  EdtDataHeader::ChannelIndex()

{
    return 0;
}

char *
GetHeaderPosString(HdrPosition pos)

{
    switch (pos)
    {
    case HeaderBefore:
        return "Before";
        break;
    case HeaderBegin:
        return "Begin";
        break;
    case HeaderEnd:
        return "End";
        break;

    case HeaderAfter:
        return "After";
        break;

    case HeaderSeparate:
        return "Separate";
        break;

    case HeaderMiddle:
         return "Middle";
       break;

    default:
       break;

    }

    return "unknown";

}

void EdtDataHeader::Print()

{
    
    printf("%-20s    0x%x\n","Size",	Size());
    printf("%-20s    0x%x\n","DataSize",	DataSize());
    printf("%-20s    %u\n","Padding",	Padding());
    printf("%-20s    0x%x\n","ImageSize",	ImageSize());
    printf("%-20s    %d\n","Offset",	Offset());
    printf("%-20s    %s\n","HeaderPosition",	
        GetHeaderPosString(HeaderPosition()));
    printf("%-20s    %s\n","HeaderWithin",	HeaderWithin()?"Y":"N");
    printf("%-20s    %s\n","IsContiguous",	IsContiguous()?"Y":"N");
    printf("%-20s    %d\n","GetExtraSize",	GetExtraSize());
    printf("%-20s    %d\n","GetExtraDMASize",	GetExtraDMASize());
    printf("%-20s    %d\n","HeaderType",	HeaderType());
    printf("%-20s    %f\n","TimeStamp",	TimeStamp());
    printf("%-20s    %f\n","TimeStamp2",	TimeStamp2());
    printf("%-20s    %s\n","GetComment",	GetComment()?GetComment():"none");
    printf("%-20s    %s\n","GetDescription", GetDescription()?GetDescription():"none");
    printf("%-20s    %s\n","FileName",	FileName()?FileName():"none");
    printf("%-20s    %s\n","DateTime",	DateTime()?DateTime():"none");
    printf("%-20s    %u\n","BufferId",	BufferId());
    printf("%-20s    %d\n","Frame",	Frame());
    printf("%-20s    %d\n","Channel",	Channel());
    printf("%-20s    %u\n","Width",	Width());
    printf("%-20s    %u\n","Height",	Height());
    printf("%-20s    %u\n","DepthBytes",	DepthBytes());
    printf("%-20s    %u\n","Depth",	Depth());
    
    printf("%-20s    %u\n","NFrames",	NFrames());
    printf("%-20s    %u\n","Debug1",	Debug1());
    printf("%-20s    %u\n","ChannelIndex",	ChannelIndex());

}
