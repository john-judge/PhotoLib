/** ********************************
 * 
 * @file edtimage/EdtImageData.h
 *
 * Copyright (c) Engineering Design Team 2010
 * All Rights Reserved
 */


#if !defined(_EDTBUFFERSIZE_H_)
#define _EDTBUFFERSIZE_H_


#include "edt_utils.h"

#include "edt_types.h"

/**=================================================================================================
 * @class   EdtBufferSize
 *
 * @brief   Edt buffer size. This class encapsulates various sizes for a single buffer, including 
 * 			any header data, as well as sizes padded out to various sizes for file access.
 * 			Access functions are overloaded - void with argument sets the value, withoout returns
 * 			the value.
 *
 * @date    2/22/2012
 *===============================================================================================**/

class EdtBufferSize {
    // sizes allocated >= rawio >= dma >= active >= buffer
    u_int buffer_size;        ///< Total size of actual data;
    u_int active_size;	      ///< Total size of data + header
    u_int dma_size;           ///< Size of DMA for EDT card - may be active_size or buffer_size depending on header type            
    u_int rawio_size;         ///< Size as rounded up to sector size
    u_int allocated_size;     ///< Size as allocated- may be rounded up to page size
    u_int file_io_size;       ///< Size of buffer as saved to disk - this may have extra room for a header that's not included 


   
    // description of any header associated with the buffer - size and
    // location only
    // 
    HdrPosition header_pos;
    u_int header_size;
    int header_offset;

    u_short sector_size;    ///< Normally 512 to ensure padding to meet sector requirements
    u_short granularity;

public:
    EdtBufferSize()
    {
        Clear();
    }

    /**=================================================================================================
     * @fn  void EdtBufferSize::Clear()
     *
     * @brief   Clears this object to its blank/initial state.
     *
     * @date    2/22/2012
     *===============================================================================================**/

    void Clear()

    {
        buffer_size = 0;
        active_size = 0;
        allocated_size = 0;
        dma_size = 0;
        rawio_size = 0;
        sector_size = 512;
        header_pos = HeaderSeparate;
        header_offset = 0;
        header_size = 0;
        granularity = 0;
        file_io_size = 0;
    }

    /**=================================================================================================
     * @fn  int EdtBufferSize::BufferSize()
     *
     * @brief   Gets the base buffer size.
     *
     * @date    2/22/2012
     *
     * @return  .
     *===============================================================================================**/

    int BufferSize()

    {
        return buffer_size;
    }

    /**=================================================================================================
     * @fn  void EdtBufferSize::BufferSize(const int size)
     *
     * @brief   Sets Buffer size.
     *
     * @date    2/22/2012
     *
     * @param   size    The size.
     *===============================================================================================**/

    void BufferSize(const int size)

    {
        buffer_size = size;
    }

    /**=================================================================================================
     * @fn  int EdtBufferSize::ActiveSize()
     *
     * @brief   Gets the active size (buffer + attached header).
     *
     * @date    2/22/2012
     *
     * @return  .
     *===============================================================================================**/

    int ActiveSize()

    {
        return active_size;
    }

    /**=================================================================================================
     * @fn  void EdtBufferSize::ActiveSize(const int size)
     *
     * @brief   Sets the Active size.
     *
     * @date    2/22/2012
     *
     * @param   size    The size.
     *===============================================================================================**/

    void ActiveSize(const int size)

    {
        active_size = size;
    }

    /**=================================================================================================
     * @fn  int EdtBufferSize::DMASize()
     *
     * @brief   Gets the dma size => buffer + header within the buffer.
     *
     * @date    2/22/2012
     *
     * @return  .
     *===============================================================================================**/

    int DMASize()

    {
        return dma_size;
    }

    /**=================================================================================================
     * @fn  void EdtBufferSize::DMASize(const int size)
     *
     * @brief   Sets the Dma size => buffer + header within the buffer
     *
     * @date    2/22/2012
     *
     * @param   size    The size.
     *===============================================================================================**/

    void DMASize(const int size)

    {
        dma_size = size;
    }

    /**=================================================================================================
     * @fn  int EdtBufferSize::RawIOSize()
     *
     * @brief   Gets the raw i/o size.
     *
     * @date    2/22/2012
     *
     * @return  .
     *===============================================================================================**/

    int RawIOSize()

    {
        if (rawio_size == 0)
            return BufferSize();
        return rawio_size;
    }

    /**=================================================================================================
     * @fn  void EdtBufferSize::RawIOSize(const int size)
     *
     * @brief   Raw i/o size.
     *
     * @date    2/22/2012
     *
     * @param   size    The size.
     *===============================================================================================**/

    void RawIOSize(const int size)

    {
        rawio_size = size;
    }

    /**=================================================================================================
     * @fn  int EdtBufferSize::FileIOSize()
     *
     * @brief   Gets the file i/o size.
     *
     * @date    2/22/2012
     *
     * @return  .
     *===============================================================================================**/

    int FileIOSize()

    {
        if (file_io_size == 0)
            return BufferSize();
        return rawio_size;
    }

    /**=================================================================================================
     * @fn  void EdtBufferSize::FileIOSize(const int size)
     *
     * @brief   File i/o size.
     *
     * @date    2/22/2012
     *
     * @param   size    The size.
     *===============================================================================================**/

    void FileIOSize(const int size)

    {
        file_io_size = size;
    }

    /**=================================================================================================
     * @fn  int EdtBufferSize::AllocatedSize()
     *
     * @brief   Gets the allocated size.
     *
     * @date    2/22/2012
     *
     * @return  .
     *===============================================================================================**/

    int AllocatedSize()

    {
        return allocated_size;
    }

    /**=================================================================================================
     * @fn  void EdtBufferSize::AllocatedSize(const int size)
     *
     * @brief   Allocated size.
     *
     * @date    2/22/2012
     *
     * @param   size    The size.
     *===============================================================================================**/

    void AllocatedSize(const int size)

    {
        allocated_size = size;
    }

    /**=================================================================================================
     * @fn  void EdtBufferSize::SetSizes(const u_int b_size, const u_int a_size, const u_int d_size,
     *      const u_int r_size, const u_int f_size, const u_int alloc_size)
     *
     * @brief   Sets the sizes.
     *
     * @date    2/22/2012
     *
     * @param   b_size      The base size.
     * @param   a_size      base size + header
     * @param   d_size      base size + header w/in dma
     * @param   r_size      raw io size => active_size rounded up to sector
     * @param   f_size      raw io size plus external header
     * @param   alloc_size  Size of the allocated buffer -> must be greater than all but FileIOSize.
     *===============================================================================================**/

    void SetSizes(const u_int b_size,
                  const u_int a_size,
                  const u_int d_size,
                  const u_int r_size,
                  const u_int f_size,
                  const u_int alloc_size)

    {
        BufferSize(b_size) ;
        ActiveSize(a_size) ;
        DMASize(d_size) ;
        RawIOSize(r_size) ;
        AllocatedSize(alloc_size);
        FileIOSize(f_size);
    }

    /**=================================================================================================
     * @fn  void EdtBufferSize::HeaderPosition(const HdrPosition newtype)
     *
     * @brief   Header position.
     *
     * @date    2/22/2012
     *
     * @param   newtype The newtype.
     *===============================================================================================**/

    void HeaderPosition(const HdrPosition newtype)
    {
        header_pos = newtype;
    }

    /**=================================================================================================
     * @fn  void EdtBufferSize::HeaderOffset(const int offset)
     *
     * @brief   Header offset.
     *
     * @date    2/22/2012
     *
     * @param   offset  The offset.
     *===============================================================================================**/

    void HeaderOffset(const int offset)

    {
        header_offset = offset;
    }

    /**=================================================================================================
     * @fn  void EdtBufferSize::HeaderSize(const int newsize)
     *
     * @brief   Header size.
     *
     * @date    2/22/2012
     *
     * @param   newsize The newsize.
     *===============================================================================================**/

    void HeaderSize(const int newsize)
    {
        header_size = newsize;
    }

    /**=================================================================================================
     * @fn  int EdtBufferSize::HeaderSize()
     *
     * @brief   Gets the header size.
     *
     * @date    2/22/2012
     *
     * @return  .
     *===============================================================================================**/

    int HeaderSize() 
    {
        return header_size;
    }

    /**=================================================================================================
     * @fn  int EdtBufferSize::HeaderOffset()
     *
     * @brief   Gets the header offset.
     *
     * @date    2/22/2012
     *
     * @return  .
     *===============================================================================================**/

    int HeaderOffset()
    {
        return  header_offset;
    }

    /**=================================================================================================
     * @fn  HdrPosition EdtBufferSize::HeaderPosition()
     *
     * @brief   Gets the header position.
     *
     * @date    2/22/2012
     *
     * @return  .
     *===============================================================================================**/

    HdrPosition HeaderPosition()
    {
        return header_pos;
    }

    /**=================================================================================================
     * @fn  void EdtBufferSize::SetHeader(HdrPosition newtype, const int newsize, const int offset)
     *
     * @brief   Sets a header.
     *
     * @date    2/22/2012
     *
     * @param   newtype The newtype.
     * @param   newsize The newsize.
     * @param   offset  The offset.
     *===============================================================================================**/

    void SetHeader(HdrPosition newtype, const int newsize, const int offset)

    {
        HeaderPosition(newtype);
        HeaderSize(newsize);
        HeaderOffset(offset);
    }

    // Size beyond the buffer size 
    // based on header position 
    // This + Buffersize is ActiveSize

    /**=================================================================================================
     * @fn  int EdtBufferSize::ExtraSize()
     *
     * @brief   Gets the extra size.
     *
     * @date    2/22/2012
     *
     * @return  .
     *===============================================================================================**/

    int ExtraSize()

    {
        switch(HeaderPosition())
        {
        case HeaderBefore:
        case HeaderAfter:
        case HeaderBegin:
        case HeaderEnd:
            return HeaderSize();
        default:

            return 0;
        }

        return 0;
    }

    /**=================================================================================================
     * @fn  int EdtBufferSize::ComputedOffset()
     *
     * @brief   Gets the computed offset.
     *
     * @date    2/22/2012
     *
     * @return  .
     *===============================================================================================**/

    int ComputedOffset()

    {
        int off = HeaderOffset();

        switch (HeaderPosition())
        {
            case HeaderBefore:
                off = - (int) HeaderSize();
            break;

            case HeaderBegin:
                off = 0;
            break;
                
            case HeaderEnd:
                off = BufferSize() - HeaderSize();
            break;
                
            case HeaderAfter:
                off = BufferSize();
            break;

        default:
            break;

        }

        return off;
    }

    /**=================================================================================================
     * @fn  int EdtBufferSize::DMAExtraSize()
     *
     * @brief   Gets the dma extra size.
     *
     * @date    2/22/2012
     *
     * @return  .
     *===============================================================================================**/

    int  DMAExtraSize()

    {
        switch(HeaderPosition())
        {
        case HeaderBegin:
        case HeaderEnd:
            return HeaderSize();
        default:
            return 0;
        }

        return 0;
    }

    /**=================================================================================================
     * @fn  int EdtBufferSize::DataOffset()
     *
     * @brief   Gets the data offset.
     *
     * @date    2/22/2012
     *
     * @return  .
     *===============================================================================================**/

    int DataOffset()
    {
        switch(HeaderPosition())
        {
        case HeaderBegin:
        case HeaderBefore:        
            return HeaderSize();
        default:
            break;
        }

        return 0;
    }

    /**=================================================================================================
     * @fn  int EdtBufferSize::SectorSize()
     *
     * @brief   Gets the sector size.
     *
     * @date    2/22/2012
     *
     * @return  .
     *===============================================================================================**/

    int SectorSize()
    {
        return sector_size;
    }

    /**=================================================================================================
     * @fn  void EdtBufferSize::SectorSize(const int size)
     *
     * @brief   Set the Sector size - default 512.
     *
     * @date    2/22/2012
     *
     * @param   size    The size.
     *===============================================================================================**/

    void SectorSize(const int size)
    {
        sector_size = size;
    }

    /**=================================================================================================
     * @fn  int EdtBufferSize::Granularity()
     *
     * @brief   Gets the granularity of allocation.
     *
     * @date    2/22/2012
     *
     * @return  .
     *===============================================================================================**/

    int Granularity()
    {
        return granularity;
    }

    /**=================================================================================================
     * @fn  void EdtBufferSize::Granularity(const int size)
     *
     * @brief   Sets the allocation granularity
     *
     * @date    2/22/2012
     *
     * @param   size    The size.
     *===============================================================================================**/

    void Granularity(const int size)
    {
        granularity = size;
    }

    // If allocatedsize, buffersize and header is set
    // determine other sizes

    /**=================================================================================================
     * @fn  u_int EdtBufferSize::AlignedActiveSize(const u_int size)
     *
     * @brief   Aligned active size.
     *
     * @date    2/22/2012
     *
     * @param   size    The size.
     *
     * @return  .
     *===============================================================================================**/

    u_int AlignedActiveSize(const u_int size)
    {
         return (size + ExtraSize());
    }

    /**=================================================================================================
     * @fn  u_int EdtBufferSize::AlignedDMASize(const u_int size)
     *
     * @brief   Aligned dma size.
     *
     * @date    2/22/2012
     *
     * @param   size    The size.
     *
     * @return  .
     *===============================================================================================**/

    u_int AlignedDMASize(const u_int size)
    {
         return (size + DMAExtraSize());
    }

    /**=================================================================================================
     * @fn  u_int EdtBufferSize::AlignedRawIOSize(const u_int size)
     *
     * @brief   Aligned raw i/o size.
     *
     * @date    2/22/2012
     *
     * @param   size    The size.
     *
     * @return  .
     *===============================================================================================**/

    u_int AlignedRawIOSize(const u_int size)

    {
        if (SectorSize())
        {
            if (size % SectorSize())
            {
                return ((size / SectorSize())+1) * SectorSize();
            }               
        }

        return size;
    }

    /**=================================================================================================
     * @fn  u_int EdtBufferSize::AlignedAllocatedSize(const u_int size)
     *
     * @brief   Aligned allocated size.
     *
     * @date    2/22/2012
     *
     * @param   size    The size.
     *
     * @return  .
     *===============================================================================================**/

    u_int AlignedAllocatedSize(const u_int size)

    {
        if (Granularity())
        {
            if (size % Granularity())
            {
                return ((size / Granularity())+1) * Granularity();
            }               
        }

        return size;
    }

    /**=================================================================================================
     * @fn  u_int EdtBufferSize::AlignedFileIOSize(const u_int size)
     *
     * @brief   Aligned file i/o size.
     *
     * @date    2/22/2012
     *
     * @param   size    The size.
     *
     * @return  .
     *===============================================================================================**/

    u_int AlignedFileIOSize(const u_int size)

    {
        if (HeaderPosition() == HeaderSeparate)
            return size + HeaderSize();

        return size;
    }

    /**=================================================================================================
     * @fn  void EdtBufferSize::AlignSizes()
     *
     * @brief   Align sizes.
     *
     * @date    2/22/2012
     *===============================================================================================**/

    void AlignSizes()

    {
        ASSERT(BufferSize());


        ActiveSize(BufferSize() + ExtraSize());
        DMASize(BufferSize() + DMAExtraSize());
        RawIOSize(AlignedRawIOSize(ActiveSize()));

        if (AllocatedSize() < RawIOSize())
            AllocatedSize(AlignedAllocatedSize(RawIOSize()));

        FileIOSize(AlignedFileIOSize(RawIOSize()));

        ASSERT(AllocatedSize() >= RawIOSize());
        ASSERT(RawIOSize() >= ActiveSize());
        ASSERT(ActiveSize() >= DMASize());
        ASSERT(DMASize() >= BufferSize());

    }

    /**=================================================================================================
     * @fn  int EdtBufferSize::MinAllocatedSize()
     *
     * @brief   Gets the minimum allocated size by rounding up.
     *
     * @date    2/22/2012
     *
     * @return  .
     *===============================================================================================**/

    int MinAllocatedSize()
    {
        ASSERT(BufferSize());

        int a = AlignedActiveSize(BufferSize());
        int io = AlignedRawIOSize(a);
        int alloc = AlignedAllocatedSize(io);

        return alloc;
        
    }

    /**=================================================================================================
     * @fn  void EdtBufferSize::SetSizes(const u_int bsize, const u_int alloc = 0)
     *
     * @brief   Sets the sizes based on a base buffer size bsize.
     *
     * @date    2/22/2012
     *
     * @param   bsize   The bsize.
     * @param   alloc   (optional) the allocated size .
     *===============================================================================================**/

    void SetSizes(const u_int bsize, const u_int alloc = 0)
    {
        BufferSize(bsize);
        ActiveSize(AlignedActiveSize(bsize));
        DMASize(BufferSize() + DMAExtraSize());
        RawIOSize(AlignedRawIOSize(ActiveSize()));

        if (alloc == 0)
            AllocatedSize(AlignedAllocatedSize(RawIOSize()));
        else
            AllocatedSize(alloc);

        if (HeaderPosition() == HeaderSeparate)
            FileIOSize(RawIOSize() + HeaderSize());
        else
            FileIOSize(RawIOSize());

        ASSERT(AllocatedSize() >= RawIOSize());
        ASSERT(RawIOSize() >= ActiveSize());
        ASSERT(ActiveSize() >= DMASize());
        ASSERT(DMASize() >= BufferSize());
    }

    /**=================================================================================================
     * @fn  void EdtBufferSize::Print(const char *tag)
     *
     * @brief   Prints.
     *
     * @date    2/22/2012
     *
     * @param   tag - the first word printed followed by all sizes
     *===============================================================================================**/

    void Print(const char *tag, char *buf)

    {
        sprintf(buf, "%s %6x %6x %6x %6x %6x %6x %6x\n",
            tag,
            BufferSize(),
            ActiveSize(),
            DMASize(),
            RawIOSize(),
            FileIOSize(),
            AllocatedSize(),
            HeaderOffset()
            );

    }
};


#endif
