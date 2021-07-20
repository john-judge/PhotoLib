// edtimage/EdtImageDescr.h: interface for the EdtImageDescr class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_EDT_IMAGE_DESCR_)
#define _EDT_IMAGE_DESCR_

// Base class for different image file types
// 


#include "EdtImage.h"

#include "StringUtils.h" // Std C string utilities

#define ReadWrite (ios_base::in | ios_base::out)

#define ReadOnly  (ios_base::in)

#define WriteOnly (ios_base::out)

#define AppendOnly  (WriteOnly | ios_base::app)
    
typedef ios_base::open_mode Access;

typedef HANDLE FileHandle;

class EdtImageDescr : public EdtImageData
{
public:
    enum ImageType {
	ImageFile,
	ImageCounters
    };
	
protected:

    ImageType image_type; // file, prb, counter, specified

    string tag;

    string description; // sufficient to recreate image

    string error_string;

    bool error_state;
    bool writing;
    Access access;


public:

 
    EdtImageDescr()
    {
	error_state = false;
	image_type = ImageFile;
	access = WriteOnly;
    }

    virtual ~EdtImageDescr()
    {

    }

    virtual void SetDescription(string s)
    {
	description = s;
    }

    virtual void SetDescription(const char *s)
    {
	description = s;
    }

    const string &GetDescription() {
	return description;
    }

    virtual void SetTag(string s)
    {
	tag = s;
    }

    const string &GetTag() {
	return tag;
    }

    virtual bool IsOk() {return !error_state;}

    virtual void SetError(string error_str)
    {
	error_string = error_str;
	error_state = true;
    }

    const string &GetError()
    {
	return error_string;
    }

    void AlignImage(EdtImage *pImage)
    {
	if (pImage)
	{
	    pImage->Resize(m_nType,m_nWidth,m_nHeight,!pImage->IsAllocated());
	}

    }

    void SetImageType(ImageType t)
    {
	image_type = t;
    }

    bool HaveValues() const
    {
	return (m_nWidth > 0 && m_nHeight > 0 && m_nType > 0);
    }


    void Writing(const bool state)
    {
	writing = state;
	if (writing)
	    access = WriteOnly;
	else
	    access = ReadOnly;
    }

    const bool Writing(void)
    {
	return writing;
    }
};



#endif 
