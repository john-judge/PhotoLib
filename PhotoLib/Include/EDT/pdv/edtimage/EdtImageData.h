/** ********************************
 * 
 * @file edtimage/EdtImageData.h
 *
 * Copyright (c) Engineering Design Team 2010
 * All Rights Reserved
 */


// edtimage/EdtImageData.h: interface for the EdtImageData class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_IMAGEDATA_H_)
#define _IMAGEDATA_H_


#include "edt_utils.h"
#include "edt_types.h"
#include "EdtBufferSize.h"
#include "EdtDataHeader.h"
#include "dispatch/ParmObject.h"
#include <iostream>

using namespace std;

 /**
 * @brief
 * Base class for all objects which descibe an image (including camera and display window).
 *
 * Incorporates the descriptive values for an image, but doesn't refer to any actual image
 * data - that's handled by the derived class EdtImage.
 *
 */

class EdtImageData : public ParmObject
{

public:

    EdtImageData(const char *nm = NULL);
    EdtImageData(const EdtImageData &from, const char *nm = NULL);
    virtual ~EdtImageData();

    virtual void InitValues();

    virtual int GetWidth() const;
    virtual void SetWidth(const int nWidth);

    virtual int GetHeight() const;
    virtual void SetHeight(const int nHeight);
    /// added for simulation

    int GetVBlank() const;
    void SetVBlank(const int nlines);

    int GetHBlank() const;
    void SetHBlank(const int nclocks);

    virtual EdtDataType GetType() const;
    virtual void SetType(const EdtDataType nType);

    virtual int GetPitch() const;
    virtual void SetPitch(const int nPitch);

    virtual int GetPitchPixels() const;
    virtual void SetPitchPixels(const int nPitchPixels);
    virtual float GetPixelSize() const;
   
    virtual int GetLineWidth(int nWidth = 0) const;

    virtual unsigned int GetMaxValue();
    virtual int GetMinValue();

    static int GetTypeBytes(EdtDataType nType);
    static void GetTypeSizeAndColors(const EdtDataType nType, int &pixelsize, int &ncolors);

    virtual int GetNPixels();
    virtual int GetNColors() const;

    // Depth in bits
    void SetDepth(int nDepth);
    int GetDepth();

    //////////////////////////////////////

    virtual int HeaderSize();
    virtual void HeaderSize(const int newsize);

    virtual HdrPosition HeaderPosition();
    virtual void HeaderPosition(const HdrPosition pos);

    virtual int HeaderOffset();
    virtual void HeaderOffset(const int offset);

    virtual void SetHeaderValues(HdrPosition pos, const int newsize, const int offset);
    virtual void SetHeaderValues(EdtDataHeader *phdr);
    virtual void SetHeaderValues(EdtImageData *other);

    void SetupHeader(const HdrPosition header_pos, const u_int header_size, const int offset = 0);

    //////////////////////////////////////

    virtual void SetMinMax(const int nMin, const int nMax);

    virtual void SetBufferValues(const EdtDataType nType, const int nWidth, const int nHeight = 1);
    virtual void SetBufferValues(const EdtImageData &from);
    virtual void GetBufferValues(EdtDataType &nType, int &nWidth, int &nHeight) const;

    virtual void SetTimeStamp(const double dTimestamp);
    double GetTimeStamp() const;
  
    //////////////////////////////////////

    virtual int BufferSize();
    virtual void BufferSize(const int buffersize);

    virtual int ActiveSize();
    virtual void ActiveSize(const int buffersize);

    virtual int DMASize();
    virtual void DMASize(const int buffersize);

    virtual int RawIOSize();
    virtual void RawIOSize(const int buffersize);

    virtual int FileIOSize();
    virtual void FileIOSize(const int buffersize);

    virtual int AllocatedSize();
    virtual void AllocatedSize(const int buffersize);

    virtual int Granularity();
    virtual void Granularity(const int gran);

    virtual void SetSizes(const u_int buf_size,
                  const u_int act_size,
                  const u_int dma_size,
                  const u_int raw_size,
                  const u_int file_io_size,
                  const u_int alloc_size);


    virtual void AlignSizes();
    virtual void Header(EdtDataHeader *phdr);
    virtual EdtDataHeader * Header();
    virtual void BufferSizePrint(const char *tag, ostream &out);
    virtual void PrintState(ostream &out);

protected:
    EdtBufferSize bsize; ///< encapsulates basic buffer size issues
                         // including data

    int	m_nWidth;		///< Width in pixels
    int	m_nHeight;		///< Height in pixels
    int	m_nX;			///< Origin in parent
    int	m_nY;			///< (0,0) if no parent

    int m_nHBlank;		///< blanking for capture/simulator
    int m_nVBlank;

    int	m_nPitch;		///< Distance from row to row in bytes;
    // negative for origin at lower left

    int	m_nMaxPixel;	///< user-defined max
    int	m_nMinPixel;	///< user-defined min

    float	m_nPixelSize;	///< pixel size in bytes
    EdtDataType	m_nType;		///< one of defined simple pixel types

    bool	m_bMinMaxSet;	///< user has defined values
    byte	m_nColors;		///< how many colors in image
    byte	m_nColorTag;	///< Storage format of planes - RGB, BGR, YUV...
    byte	m_nDepth;
    byte	m_nTaps;

    void SetPixelSize()
    {
        if (m_nType != EDT_TYPE_MONO)
        {
            m_nPixelSize = (float) (TypeSize(m_nType));
            m_nPixelSize *= GetNColors();
            m_nDepth = (byte)(m_nPixelSize * 8);
        }
        else
        {
            m_nPixelSize = 1;
            m_nDepth = 1;
        }
    }

    EdtDataHeader *m_header;

    double  m_timestamp;


};


#endif 
