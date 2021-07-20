
#include "edtinc.h"

#include "EdtBufHeader.h"

#include <math.h>

#include <stddef.h>

void EdtBufHeader::Init(const int sz, const HdrPosition pos, const int offset)

{

    
    int size = sz;
    if (sz == 0)
        size = sizeof(EdtBufDataHdr);

    if (size % 512)
        size = ((size / 512) + 1) * 512;

    Size(size);
    Allocate();


    HeaderPosition(pos);
    Offset(offset);

    pdata->Init(size);

    HeaderType(HDR_TYPE_BUFHEADER);

   

}


EdtBufHeader::EdtBufHeader(const int sz, const HdrPosition pos, const int offset)

{

    pdata = NULL;
    Init(sz, pos, offset);
}

EdtBufHeader::EdtBufHeader(EdtImageData *from)

{

    pdata = NULL;

    if (from)
        Init(from->HeaderSize(), from->HeaderPosition(), from->HeaderOffset());
    else
        Init(0, HeaderSeparate, 0);

    Width(from->GetWidth());
    Height(from->GetHeight());
    DepthBytes((int) from->GetPixelSize());


}

EdtBufHeader::EdtBufHeader(EdtBufDataHdr *bhdr)

{
    pdata = NULL;

    if (bhdr)
        Init(bhdr->m_hdr_size, bhdr->m_hdr_pos, bhdr->m_hdr_offset);
    else
        Init(0, HeaderSeparate, 0);

    if (pdata && bhdr)
        memcpy(pdata,bhdr, Size());

}

EdtBufHeader::~EdtBufHeader()
{

}


/// Size in bytes of the header 
// 
u_int EdtBufHeader::Size()
{
     return m_hdr_size;
}

void EdtBufHeader::Size(const u_int size)

{
    m_hdr_size = size;
    if (pdata)
        pdata->m_hdr_size = size;
}   

/// Offset in bytes of the header in the full data buffer
int EdtBufHeader::Offset()

{
    return m_hdr_offset;
}

void EdtBufHeader::Offset(const int offset)

{
    m_hdr_offset = offset;
    if (pdata)
        pdata->m_hdr_offset = offset;
}

/// One of enumerated positions relative to beginning of 
/// actual data

HdrPosition EdtBufHeader::HeaderPosition()

{
    return m_hdr_pos;
}

void EdtBufHeader::HeaderPosition(HdrPosition pos)

{
    m_hdr_pos = pos;
    if (pdata)
        pdata->m_hdr_pos = pos;
}



void EdtBufHeader::HeaderType(const int t)

{
    m_header_type = t;
    pdata->m_header_type = 3;
}

int EdtBufHeader::HeaderType()

{
    return m_header_type;
}

void EdtBufHeader::BufferId(const u_int id)

{
      
}

u_int EdtBufHeader::BufferId()

{
    return 0;
}

void EdtBufHeader::TimeStamp(double t)

{
    pdata->m_timestamp = t;
}

double EdtBufHeader::TimeStamp()

{
    return pdata->m_timestamp;
}

int EdtBufHeader::Frame()

{
    return pdata->m_serial_no;
}
void EdtBufHeader::Frame(const int frm)

{
    pdata->m_serial_no = frm;
}

int EdtBufHeader::Channel()

{
      
    return pdata->m_channel;
 
}
void EdtBufHeader::Channel(const int chan)

{
    pdata->m_channel = chan;
}

u_int EdtBufHeader::Width()

{
    return pdata->width;
}

void EdtBufHeader::Width(const u_int w)

{
    pdata->width = w;
}

u_int EdtBufHeader::Height()

{
    return pdata->height;
}
void EdtBufHeader::Height(const u_int h)

{
    pdata->height = h;
}

void EdtBufHeader::Depth(const u_int d)

{
    pdata->depth = d;
}

u_int EdtBufHeader::Depth()

{
    return pdata->depth;
}

void EdtBufHeader::DepthBytes(const u_int d)

{
    pdata->depth_bytes = d;
}

u_int EdtBufHeader::DepthBytes()

{
    return pdata->depth_bytes;
}
void EdtBufHeader::SetComment(const char *s)

{
    strcpy(pdata->m_text, s);
}

void EdtBufHeader::AppendComment(const char *s)

{
    strcat(pdata->m_text, s);
}

const char *EdtBufHeader::GetComment()

{
    return pdata->m_text;
}

EdtBufDataHdr *EdtBufHeader::FileData()

{
    return pdata;
}

void EdtBufHeader::BaseAddr(void *addr)

{
    pdata = NULL;
    EdtDataHeader::BaseAddr(addr);
    pdata = (EdtBufDataHdr *) ((u_char *) addr + m_hdr_offset);
    pdata->Init(m_hdr_size);
    HeaderType(HDR_TYPE_BUFHEADER);
}

void EdtBufHeader::Allocate()

{
    if (m_hdr_size)
    {
        EdtDataHeader::Allocate();
        pdata = (EdtBufDataHdr *) m_base_addr;
    }

}

void EdtBufHeader::TimeStampWritten(double t)

{
    pdata->m_timestamp2 = t;
}

double EdtBufHeader::TimeStampWritten()

{
    return pdata->m_timestamp2;
}

void EdtBufHeader::FileName(const char *s)

{   
//    strncpy(pdata->name, s, sizeof(pdata->name)-1);
}

const char *EdtBufHeader::FileName()

{
    return NULL;
}


void EdtBufHeader::DateTime(const char *s)

{
    
}

const char *EdtBufHeader::DateTime()

{
    return NULL;
}

void EdtBufHeader::NFrames(const u_int n)

{
    
}

u_int EdtBufHeader::NFrames()

{
    return 0;

}
void EdtBufHeader::Clear()

{
    pdata->Init(pdata->m_hdr_size);
}

const char * EdtBufHeader::GetDescription()

{
    return GetComment();
}

EdtDataHeader * EdtBufHeader::Clone()

{
    EdtBufHeader *pfile;

    pfile = new EdtBufHeader(pdata);

    
    return pfile;


}

double EdtBufHeader::TimeStamp2()

{
    return pdata->m_timestamp2;

}

void EdtBufHeader::TimeStamp2(double t)

{
    pdata->m_timestamp2 = t;

}

u_int EdtBufHeader::Debug1()
{
    return 0;
}

void EdtBufHeader::Debug1(u_int val)

{

}

u_int EdtBufHeader::DataSize()
{
    return pdata->m_data_size;
}

void EdtBufHeader::DataSize(u_int val)

{
    pdata->m_data_size = val;

    if (Width() == 0)
    {
        Width(val);
        Height(1);
    }
    if (DepthBytes() == 0)
        DepthBytes(1);
}


u_int EdtBufHeader::ChannelIndex()
{
    return pdata->m_channel_index;
}

void EdtBufHeader::ChannelIndex(u_int val)

{
    pdata->m_channel_index = val;
}


static EdtDataHeader *CreateBufHeader()

{
    return new EdtBufHeader;
}

//#include "EdtFileHeader.h"

class Initializer
{
public:
    Initializer()
    {

    }
};

void BufHeaderSetup()
{
    RegisterHeaderType(HDR_TYPE_BUFHEADER, CreateBufHeader);
}



// Code to separate the opening of a header based on type
// from the file I/O code

#include <map>

using namespace std;

static bool setup = false;

map<const int, EdtHeaderFactory> *available_headers = NULL;

typedef pair<const int, EdtHeaderFactory> header_type_pair;

void RegisterHeaderType(const int type, EdtHeaderFactory f)

{
    if (available_headers == NULL)
        available_headers = new map<const int, EdtHeaderFactory>;

     available_headers->insert(header_type_pair(type, f));
}

extern void BufHeaderSetup();

EdtDataHeader * GetFrameHeader(const int type)

{

    if (!setup)
    {
        BufHeaderSetup();
    }

    EdtHeaderFactory f = NULL;

    map<const int,EdtHeaderFactory>::const_iterator w;

    w = available_headers->find(type);

    if (w == available_headers->end())
    {
        return NULL;
    }
    else
        return w->second();

}



