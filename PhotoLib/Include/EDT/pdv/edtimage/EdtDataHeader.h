/** ********************************
 * 
 *
 * Copyright (c) Engineering Design Team 2010
 * All Rights Reserved
 */

/** @file EdtImage/EdtDataHeader.h
*  Defines an abstract base class for data header descriptions
*  so the description + data can be generalized.
*/

#ifndef _EDT_DATA_HEADER_H_
#define _EDT_DATA_HEADER_H_

/**

 The EdtDataHeader class is an abstract base class for 
 Header descriptors. The header desriptor is an interface to
 some underlying structure, with the functions to get pertinent 
 data virtual so derived classes can desribe a variety of header
 types.
 
 The actual header data is 
 referenced from a pointer (m_own_data).
 Derived classes will typically have a defined structure representing
 the binary structure. References to values in the
 header are all through the virtual functions defined here.
 

 The header is described by a position and a size. In some cases the header is not even 
 a contguous piece of memory with the data buffer.

 */
 
#include "edt_utils.h"

class EdtDataHeader {

protected:

    u_int m_hdr_size;
    u_int m_data_size;
    u_int m_padding;
    u_int m_imagesize;

    int m_hdr_offset;
    HdrPosition m_hdr_pos;

    int m_header_type;

    void *m_base_addr;

    bool m_own_data;

public:

    EdtDataHeader();
    virtual ~EdtDataHeader();

    /// Set/Get the pointer to header data
    //
    virtual void BaseAddr(void *p); 
    virtual void *BaseAddr();

    /// Size in bytes of the header 
    // 
    virtual u_int Size();
    virtual void Size(const u_int size);

    /// Size of the full data buffer assoicated with this header
    virtual u_int DataSize();
    virtual void DataSize(const u_int size);

    /// Size of the room at the end of the buffer (to expand to 512)
    ///

    virtual u_int Padding();
    virtual void Padding(const u_int size);

    /// Size of the underlying image or data buffer

    virtual u_int ImageSize();
    virtual void  ImageSize(const u_int size);

    /// Offset in bytes of the header in the full data buffer

    virtual int Offset();
    virtual void Offset(const int offset);

    /// One of enumerated positions relative to beginning of 
    /// actual data

    virtual HdrPosition HeaderPosition();
    virtual void HeaderPosition(HdrPosition pos);

    /// Is header part of the actual data
    /// 
    virtual bool HeaderWithin();

    /// Is header contiguous with data buffer
    /// 

    virtual bool IsContiguous();

    /// 

    virtual void Setup(HdrPosition hdr_pos, 
        const u_int size);

    virtual void Setup(HdrPosition hdr_pos, 
        const u_int size, 
        const int offset);

    virtual void ComputeOffset();

    virtual void CopySettings(EdtDataHeader *other);

    virtual void Allocate();
    virtual void DeAllocate();

    virtual void *GetHeaderPtr();

    virtual int  GetExtraSize();
    virtual int  GetExtraDMASize();

    /// Processing to do after capture
    virtual void Process(const double t);

    /// Open ended tags for header type
    virtual void HeaderType(const int t);
    virtual int  HeaderType();
    
    virtual void TimeStamp(double t) = 0;
    virtual double TimeStamp() = 0;

    virtual void TimeStamp2(double t) = 0;
    virtual double TimeStamp2() = 0;

    virtual void SetComment(const char *s) = 0;
    virtual void AppendComment(const char *s) = 0;

    virtual const char *GetComment() = 0;

    virtual const char *GetDescription() = 0;

    virtual void FileName(const char *s) = 0;
    virtual const char *FileName() = 0;

    virtual void DateTime(const char *s) = 0;
    virtual const char *DateTime() = 0;

    virtual void Clear() = 0;
    
    virtual EdtDataHeader *Clone() = 0;

    virtual void BufferId(const u_int id) = 0;
    virtual u_int BufferId() = 0;

    virtual int Frame() = 0;
    virtual void Frame(const int frm) = 0;

    virtual int Channel() = 0;
    virtual void Channel(const int chan) = 0;

    virtual u_int Width() = 0;
    virtual void Width(const u_int w) = 0;

    virtual u_int Height() = 0;
    virtual void Height(const u_int h) = 0;

    virtual u_int DepthBytes() = 0;
    virtual void DepthBytes(const u_int d)  = 0;

    virtual u_int Depth() = 0;
    virtual void Depth(const u_int d)  = 0;

    virtual void NFrames(const u_int n) = 0;
    virtual u_int NFrames() = 0;

    virtual void Debug1(u_int val) = 0;
    virtual u_int Debug1() = 0;

    virtual void ChannelIndex(const u_int val);
    virtual u_int ChannelIndex();

    virtual void Print();
};


typedef EdtDataHeader * (*EdtHeaderFactory)();



#endif
