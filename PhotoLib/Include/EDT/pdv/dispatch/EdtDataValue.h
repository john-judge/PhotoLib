/**
*
*  @file EdtDataValue.h 
* 
*  @copyright Copyright Engineering Design Team 2012
*  All Rights Reserved
*
**/
/** ********************************
 * 
 * @file edtimage/EdtDataValue.h
 *
 * Copyright (c) Engineering Design Team 2010
 * All Rights Reserved
 */


    /* union to hold data elements */


#ifndef _EDT_DATA_VALUE_H_
#define _EDT_DATA_VALUE_H_

#include "edt_utils.h"

#include <string>
#include <vector>
#include <iostream>

using namespace std;

extern const char *EdtEmptyString;

class EdtDataValue;
class ParmObject;

typedef int (*DV_Notifier)(void * target, EdtDataValue *value);

/**
* \brief
* Data encapsulation class, which has named and typed data objects.
*
* The data item can either be a simple value or refer to a location
* in memory.
*
* Type conversions are handled by operator casts and overrides on
* the Set command.
*/

struct EdtBitMaskTag {
    char *name;
    u_int mask;
};

// Yet another variant data type
//
// 
class EdtDataValue {
private:
    // If used to describe a value by reference
    // the value is based on a pointer instead of 
    // value in the union
    u_int size; // for reference values, size of array

    EdtDataType type; //basic types - byte, u_short, int, u_int, double, string
    bool is_set;        // whether any value has been set
    bool error;
    bool read_only;   
    bool by_ref;
    u_int archive_mask;
    u_char radix;  // for converting int <-> string
    u_char bitshift;
    u_char nbits;

    void *target; 

    // The following functions are called by set and get
    DV_Notifier update;  // set the target to dv value
    DV_Notifier refresh; // get the dv value from target
        
    string m_name;
    string m_comment;
    // if type == EDT_TYPE_STR
    string m_value;

    union vunion {
        int	i;
        u_int   u;
        u_short w; //word
        double  d;
        u_char  b;
        uint64_t uu;
        u_char *base;
        char *str_ptr;
        const char *cstr_ptr;
        string * s;
    } v;

    EdtBitMaskTag * bits;

    void SetupPointer(const char *nm, EdtDataType type, u_char *bref = NULL);

    void DoRefresh();
    void DoUpdate();
    void GetFormattedString(char *s, const char *fmt);

public:

    EdtDataValue(EdtDataType t = TypeSTLSTR);    
    EdtDataValue(const char *value);
    EdtDataValue(const int i);

    virtual ~EdtDataValue();

    // initialize a reference value
    EdtDataValue(const char *name,  
        EdtDataType t, 
        u_char *addr = NULL);

    EdtDataValue(const char *name, u_char *bref);
    EdtDataValue(const char *name, u_short *bref);
    EdtDataValue(const char *name, int *bref);
    EdtDataValue(const char *name, u_int *bref);
    EdtDataValue(const char *name, double *bref);
    EdtDataValue(const char *name, uint64_t *bref);
    EdtDataValue(const char *name, char *cref, int maxl);
    EdtDataValue(const char *name, u_int *bref, EdtBitMaskTag *masks);

    // copy costructor
    //EdtDataValue(const EdtDataValue & source);
    
    void Init(EdtDataType t);
 
    virtual void SetName(const char *nm);
    virtual void SetComment(const char *comment);
    virtual void SetType(EdtDataType t);

    const char *Name();
    const char *Value();
    const char *Comment();
    
    // Assign value to the internal value
    virtual bool Set(u_char b);
    virtual bool Set(u_short w);
    virtual bool Set(int i);
    virtual bool Set(u_int u);
    virtual bool Set(double d);
    virtual bool Set(uint64_t uu);
    virtual bool Set(EdtDataValue &other);
    virtual void Set(const string &s);
    virtual void Set(const char *s);

    // Set to ref type with char array and length (no terminator necessary)

    virtual void SetStringRange(const char *s, int length);

    virtual void GetStringValue(string &s, const char *fmt = NULL);
    virtual void GetStringValue(char *s, const char *fmt = NULL);
    virtual const char * GetFullStringValue();

    // Casting operators
    virtual operator int();
    virtual operator u_int();
    virtual operator u_short();
    virtual operator double();
    virtual operator u_char();
    virtual operator uint64_t();
    virtual operator string &();
    
    static char *StandardFormats(const EdtDataType t);
    static void SetStandardFormat(const EdtDataType t, const char *fmt);

    int  MaxChars(); /// Max length of string representation

    void BaseAddress(void *addr); // set the base address
    void *BaseAddress(); // Get the base address

    void Offset(const u_int off); // 
    const u_int Offset();

    void Address(void *addr); // set the actual address
    void *Address(); // Get the actual address

    void ByPointer(const bool state);
    bool ByPointer();

    void Radix(const int state);
    int Radix();

    // Assignment operators
    EdtDataValue & operator=(const int i);
    EdtDataValue & operator=(const u_char b);
    EdtDataValue & operator=(const u_short s);
    EdtDataValue & operator=(const double d);
    EdtDataValue & operator=(const uint64_t uu);
    EdtDataValue & operator=(const u_int u);
    EdtDataValue & operator=(const char * s);
    EdtDataValue & operator=(const string &s);

    const  char *DebugPrint(char *buf);

    void Dump(const char *tag = NULL);

    // Functions to call when getting or setting values
    void Refresh(ParmObject *target, DV_Notifier r);
    void Update(ParmObject *target, DV_Notifier r);

    DV_Notifier Refresh();
    DV_Notifier Update();

    // Initialize with a variable address
    void Setup(const char *name, u_char *bref);
    void Setup(const char *name, u_short *bref);
    void Setup(const char *name, int *bref);
    void Setup(const char *name, u_int *bref);
    void Setup(const char *name, double *bref);
    void Setup(const char *name, uint64_t *bref);
    void Setup(const char *name, char *cref, int maxl);
    void Setup(const char *name, string *sref);
    
    EdtDataType Type();

    u_int Archive();    
    void Archive(const u_int level);
    
    void SetBitTags(EdtBitMaskTag *masks);
    
    EdtBitMaskTag *GetBitTags();
    
    friend ostream &operator<<(ostream &out, EdtDataValue &dv);
    
private:
    const char *GetConvertibleString(char *buf, int maxlen);
    u_char & ByteByRef();
    u_short & UShortByRef();
    int & IntByRef();
    u_int & UIntByRef();
    double & DoubleByRef();
    uint64_t &UInt64ByRef();
    string & StringByRef();
 
};

const char *edt_type_names(EdtDataType t);

#endif
