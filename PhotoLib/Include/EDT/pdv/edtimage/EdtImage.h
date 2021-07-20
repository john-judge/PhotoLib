/** ********************************
* 
* @file edtimage/EdtImage.h
*
* Copyright (c) Engineering Design Team 2010
* All Rights Reserved
*/


// edtimage/EdtImage.h: interface for the EdtImage class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(EDT_IMAGE_INCLUDE)
#define EDT_IMAGE_INCLUDE

// Pull in system - specific defines

//#include "edtdefines.h"
#include "EdtImageData.h"


#ifndef PDV_TIMESTAMP_SIZE
#define PDV_TIMESTAMP_SIZE sizeof(double)
#endif




enum BufferStates {
    BufIdle,
    BufToRead,
    BufReading,
    BufToWrite,
    BufWriting,
    BufPendingRead,
    BufPendingWrite
};


/**
* \brief
* 
* 
*/
struct DBufTrace {
    int m_owner_id;
    BufferStates state;
    double t;
    bool ready;
};

#include "edt_types.h"

#include "EdtImageData.h"

#include "EdtDataHeader.h"

#include <string>

using namespace std;

/**
* @brief
* Basic image class.
*
* This class defines an image buffer with member functions to access data and various image parameters.
* It can handle multi-band images of type byte,short, int, and double
* An EdtImage can reference another EdtImage as a ROI.
*
* Data is stored using an array of row pointers for easy random access.
*/

class EdtImage : public EdtImageData
{
protected:

    unsigned int	m_bAllocated:1;	// Data is allocated
    unsigned int	m_bOwnData:1;		// Object owns its data
    unsigned int	m_bRowsClean:1;	// Rows array is current
    unsigned int	m_bDataChanged:1;	// Flag for data which has been changed
    unsigned int	m_bInverted:1;	// Uses negative pitch (such as Windows DIB)
    unsigned int	m_bDisplayable:1; // If this can be displayed directly
    unsigned int	m_bContiguous:1;	// If the memory is a single contiguous block of pixels
    unsigned int	m_bRemappingDisplay:1;
    unsigned int	m_bOwnHeader:1;		// whether the header pointer belongs to this image
    unsigned int	m_bContiguousHeader:1; // if true header precedes image data 

    u_char	m_nPadBytes;	// byte multiple (0,4,8..) required

    unsigned short	m_nSectorSize;    // minimum page size for image allocation

    int	m_nSequenceNumber;

    u_char *m_data;
    u_char *m_base_data;

    bool m_own_data;

    void *m_owner;
    void *m_user;

    int m_buf_id;
    int m_serial_no;

    bool m_ready;

    int refcounts;

    DBufTrace *trace;
    int n_trace_elements;
    int max_trace_values;

    BufferStates current_state;

    string m_name;

private:

    void	**m_pRows;	// pointer to array of pointers of current 

    // Header data 

    void *m_pHeaderData;    // pointer to unformatted header data

    double *m_pTimestamp;   // first 8 bytes of header always

    int m_nLastPitch;
    int m_nFullRows; // so you can resize w/out reallocating rows

    void *m_pLibHandle; // system specific pointer i.e. XImage, XIL image...

    int m_owner_id; // to keep track of who allocated this object

protected:
    virtual void UpdatePointers(); // Make sure row pointers are 
    // current

    virtual void ResetValues(const char *nm);

    // Actually set the values
    virtual void SetValues(void *pData, 
        const EdtDataType nType,
        const int nWidth, 
        const int nHeight, 
        const int nPitch,
        const u_int nAllocated);

public:


    EdtImage(const char *nm = NULL);

    EdtImage(const EdtDataType nType, const int nWidth, const int nHeight,const char *nm = NULL);

    EdtImage(EdtImage *pParent, const int nX, const int nY,
        const int nWidth, const int nHeight, const char *nm = NULL);

    virtual ~EdtImage();

    u_char *GetDataOnly();
    u_char *GetData();

    void SetTimeStamp(const double t);
    double GetTimeStamp();

    void Create(const EdtDataType t, const int size);

    void SetOwner(void *p);
    void *GetOwner();

    void SetUser(void *user);
    void *GetUser();

    void SetBufId(const int n);
    int  GetBufId();

    EdtDataHeader *GetHeader();
    void SetHeader(EdtDataHeader *);
    void CloneHeader(EdtDataHeader *);


    void SetSerialNo(const int n);
    int  GetSerialNo();

    void SetDataPtr();

    void SetHeaderStr(const char *s);

    void SetReady(const bool state);

    bool Ready();

    void InitHeader();

    void Grab(int new_owner, BufferStates state);
    void Release(BufferStates state);

    int  Owner();

    void HeaderAppend(const char *fmt, ...);
    void HeaderClearText();

    void AllocateTrace(const int maxvalues);
    int GetNTrace();

    DBufTrace *GetTrace();

    void SetState(int m_owner_id, BufferStates state);

    BufferStates GetState();
    void ResetTrace();
    void TraceState();

    bool IsOwner(const void *o);  
    int GetDataOffset();

    int GetExtraHeaderSize();
    int GetSectorSize();
    void SetSectorSize(unsigned short nSectorSize);

    virtual void Create(const EdtDataType nType , const int nWidth, const int nHeight );
    virtual void Destroy(bool clearrows = true);

    virtual bool AttachToParent(EdtImage *pParent, const int nX, const int nY,
        const int nWidth, const int nHeight, const byte nBand = 0);

    // Create using an external data pointer, including all required values
    // if nPitch == 0, nPitch = nWIdth * GetPixelSize
    virtual void AttachToImageData(void *pData, 
        const EdtDataType nType,
        const int nWidth, 
        const int nHeight, 
        const int nPitch = 0,
        const u_int nAllocated = 0);


    void SetSubRegion(const int w, const int h);

    // Create using an external data pointer, assuming values are currently set
    virtual void AttachToData(void *pData);

    void AttachToData(u_char *p, const EdtDataType t, const int width,
        const int height, 
        const u_int nAllocated);

    void AttachToData(u_char *p, const int size,
        const u_int nAllocated);

    bool IsAllocated() const ;

    void SetAllocated(const bool bAllocated = true); 

    bool IsDisplayable() const ;

    void SetDisplayable(const bool bDisplayable = true) ;

    bool IsContiguous() const ;

    void SetContiguous(const bool bContiguous = true) ;

    bool IsInverted() const;

    void SetInverted(const bool bInverted = true) ;

    bool IsOwnData() const; 
    void SetOwnData(const bool bOwnData = true) ;
    // Data pointer at beginning of header data

    void *GetFirstPixel() const;

    void SetFirstPixel(void *pData);

    // Data pointer at beginning of allocated data 

    void *GetBaseData() ;

    void SetBaseData(void *pData);

    const bool		IsDataChanged() const ;
    void SetDataChanged();
    void SetDataClean();

    void **GetPixelRows();


    //
    // Sequence Number is used by the file read code to keep track of 
    // an image's index in the file
    //

    void SetSequenceNumber(const int n);

    int GetSequenceNumber() const;


    //////////////////////////////////////////////////
    // Copy Functions
    //////////////////////////////////////////////////

    virtual void Copy(EdtImage *pFrom);

    // Copy a region - if width/height are 0, copy whole source
    //

    virtual void CopyRegion(EdtImage *pFrom, 
        const int nFromX = 0,
        const int nFromY = 0, 
        const int nFromWidth = 0,
        const int nFromHeight = 0,
        const int nToX = 0, 
        const int nToY = 0);

    // Copy a region - if width/height are 0, copy whole source
    //

    virtual void CopyRegionSkip(EdtImage *pFrom, 
        const int nFromX = 0,
        const int nFromY = 0, 
        const int nFromWidth = 0,
        const int nFromHeight = 0,
        const int nToX = 0, 
        const int nToY = 0,
        const int nToSkip = 1,
        const int nFromSkip = 1);

    // Copy a single plane of a color image
    //

    virtual void CopyPlane(EdtImage *pFrom,
        byte nTargetPlane = 0, 
        byte nSourcePlane = 0);

    virtual void CopyRegionPlane(EdtImage *pFrom, 
        byte nTargetPlane = 0, 
        byte nSourcePlane = 0,
        const int nFromX = 0,
        const int nFromY = 0, 
        const int nFromWidth = 0,
        const int nFromHeight = 0,
        const int nToX = 0, 
        const int nToY = 0);

    // Copy parameters , allocate, and copy data
    //


    // returns true if two images have the same size and type

    virtual bool Equivalent(const EdtImage *pOther);

    // returns true if two images have equal data

    virtual bool Equal(const EdtImage *pOther);

    virtual bool FirstDifference(const EdtImage *pOther,
        int &x, int &y);

    bool operator==(const EdtImage &other);
    bool operator!=(const EdtImage &other);

    virtual void Duplicate(EdtImage *pSource);

    // specify a duplicate
    // of source image 
    // parameters, but don't copy

    virtual void DuplicateType(EdtImage *pSource);

    //////////////////////////////////////////////////
    // Pixel functions
    //////////////////////////////////////////////////

    // For color images, values are in 4 byte RGB mode
    // appropriate for image
    // If nColor != -1, then apply only to that color
    // plane

    // Fill Functions

    virtual void Fill(const int nValue, const int nColor = -1);

    virtual void FillRegion(const int nValue, const int nX, const int nY,
        const int nWidth, const int nHeight, const int nColor = -1);

    virtual void Clear(); // fill with 0

    // SetPixel Functions

    virtual void SetPixel(const int nX, const int nY, const int nValue);

    virtual void SetPixel(const int nX, const int nY, const double dValue);

    // GetPixel Functions

    virtual void GetPixel(const int nX, const int nY, int &nValue);

    virtual void GetPixel(const int nX, const int nY, double &dValue);

    virtual int GetPixelV(const int nX, const int nY, int *Values);
    // fill image with a gray scale ramp
    // defualt is full gray scale, vertical starting with row 0 == 0 and last row == max pixel value

    virtual void Ramp(int low = 0, int high = 0, bool bVertical = true, bool bAscending = true);
    virtual void ColorRamp(int low, int high, bool bVertical, bool bAscending);

    virtual char *GetDescription();

    void Resize(const EdtDataType nType, const int nWidth, const int nHeight, bool newData);

    bool SameSize(const EdtImage *pOther) const;

    virtual void BufferSizePrint(const char *tag);

    static void SetTraceSize(const int size);
    static int GetTraceSize();
};


#endif // !defined(EDT_IMAGE_INCLUDE)
