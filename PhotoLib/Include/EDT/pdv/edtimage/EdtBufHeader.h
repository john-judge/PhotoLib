#ifndef _EDT_BUF_HEADER_H_
#define _EDT_BUF_HEADER_H_


#include "EdtDataHeader.h"

#include "EdtImageData.h"

/**
* 
* \brief
* The EdtBufDataHdr is a structure which can be stored with raw data to 
* keep track of source, timestamp, etc.
*
* The m_text field can be extended
* in memory if the structure is allocated in a larger memory block.
* Generally images are allocated with at least 512 bytes for the header
* because that's the sector size for direct disk access (for speed.)
* 
*/

#define BUFDATAHDR_MAGIC 0x45445442 // "EDTB"

class EdtBufDataHdr {
public:
    u_int m_magic;
    u_int m_serial_no;
    double m_timestamp;
    double m_timestamp2;
    u_int m_buf_id;
    u_int m_data_size;
    u_int m_channel_index;
    u_short m_hdr_size;
    HdrPosition m_hdr_pos;
    int m_hdr_offset;
    uint64_t m_file_offset;
    u_int width;
    u_int height;
    u_char depth_bytes;
    u_char depth;
    u_char  m_channel;
    u_char m_header_type;
    char m_text[1];


    void Init(const int size)
    {
        m_hdr_size = size;
        m_magic = BUFDATAHDR_MAGIC;

    }
};

#include <stddef.h>

class EdtBufHeader : public EdtDataHeader
{

    EdtBufDataHdr *pdata;

public:

    EdtBufHeader(const int sz = 0, const HdrPosition pos = HeaderBefore, const int offset = 0);
    EdtBufHeader(EdtImageData *from);
    EdtBufHeader(EdtBufDataHdr *from);
    virtual ~EdtBufHeader();

    void Init(const int _size, const HdrPosition pos, const int offset);

    virtual void BufferId(const u_int id);
    virtual u_int BufferId();
    
    virtual void TimeStamp(double t);
    virtual double TimeStamp();

    virtual int Frame();
    virtual void Frame(const int frm);

    virtual int Channel();
    virtual void Channel(const int chan);

    virtual u_int Width();
    virtual void Width(const u_int w);

    virtual u_int Height();
    virtual void Height(const u_int h);

    virtual u_int Depth();
    virtual void Depth(const u_int d) ;

    virtual void SetComment(const char *s);
    virtual void AppendComment(const char *s);

    virtual const char *GetComment();

    EdtBufDataHdr *FileData();

    virtual void BaseAddr(void *addr);

    virtual void Allocate();

    virtual void TimeStampWritten(double t);
    virtual double TimeStampWritten();

    virtual void FileName(const char *s);
    virtual const char *FileName();

    virtual void DateTime(const char *s);
    virtual const char *DateTime();

    virtual void NFrames(const u_int n);
    virtual u_int NFrames();

    virtual void Clear();

    virtual const char *GetDescription();

    virtual EdtDataHeader *Clone();

    virtual u_int ChannelIndex();
    virtual void ChannelIndex(const u_int value);

    void TimeStamp2(double t);
    double TimeStamp2();

    void DepthBytes(const u_int d);
    u_int DepthBytes();

    void Debug1(u_int val);
    u_int Debug1();

    virtual void HeaderType(const int t);
    virtual int  HeaderType();

    /// Size of the full data buffer assoicated with this header
    virtual u_int DataSize();
    virtual void DataSize(const u_int size);

    /// Size in bytes of the header 
    // 
    virtual u_int Size();
    virtual void Size(const u_int size);

    /// Offset in bytes of the header in the full data buffer
    virtual int Offset();
    virtual void Offset(const int offset);

    /// One of enumerated positions relative to beginning of 
    /// actual data

    virtual HdrPosition HeaderPosition();
    virtual void HeaderPosition(HdrPosition pos);
};


void RegisterHeaderType(const int type, EdtHeaderFactory f);

EdtDataHeader *GetFrameHeader(const int type);


#endif
