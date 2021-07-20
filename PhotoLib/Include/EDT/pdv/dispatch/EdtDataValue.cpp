/**
*
*  @file EdtDataValue.cpp 
* 
*  @copyright Copyright Engineering Design Team 2012
*  All Rights Reserved
*
*  This file is confidential, proprietary source code. 
*  It cannot be copied or modified except with direct 
*  written permission from Engineering Design Team.
*
**/
/** ********************************
 * 
 * @file EdtDataValue.cpp
 *
 * Copyright (c) Engineering Design Team 2010
 * All Rights Reserved
 */


/* union to hold data elements */
/* */

#include <iostream>

#include "EdtDataValue.h"

#include "StringUtils.h"

#include "ErrorHandler.h"

const char *EdtEmptyString = "";

u_int GetMaskBitsFromString(const string &s, EdtBitMaskTag *bits)

{
    u_int u = 0;

    vector<string> v;

    string_split(s, "|", v);

    u_int i,j;

    for (i=0;i<v.size();i++)
    {
        string test = v[i];

        test = stringtrim(test);
        test = stringupper(test);

        // fix - no error checking

        for (j=0;bits[j].name;j++)
        {
            const char *t = test.c_str();

            if (!strcmp(t,bits[j].name))
            {
                u |= bits[j].mask;
            }
        }
    }

    return u;
}

u_int GetBitsFromString(const char *sz, EdtBitMaskTag *bits)

{
    string s = sz;

    return GetMaskBitsFromString(s,bits);

}

void SetStringFromBits(string &s, u_int u, EdtBitMaskTag *bits)

{
    int i;

    s.clear();

    for (i=0;bits[i].name;i++)
    {
        if ((u & bits[i].mask) == bits[i].mask)
        {
            if (s.size())
                s += " | " ;
            s += bits[i].name;
        }
    }
}

u_char &
EdtDataValue::ByteByRef()
{
    return *((u_char *) (v.base));
}

u_short &
EdtDataValue::UShortByRef()
{
    return *((u_short *) (v.base));
}

int &
EdtDataValue::IntByRef()
{
    return *((int *) (v.base));
}

u_int &
EdtDataValue::UIntByRef()
{
    return *((u_int *) (v.base));
}

double &
EdtDataValue::DoubleByRef()
{
    return *((double *) (v.base));
}

uint64_t &
EdtDataValue::UInt64ByRef()
{
    return *((uint64_t *) (v.base));
}

string &
EdtDataValue::StringByRef()
{
    return *((string *) (v.base));
}

EdtDataValue::~EdtDataValue()
{

}

EdtDataValue::EdtDataValue(EdtDataType t)
{
    Init(t);
}

EdtDataValue::EdtDataValue(const char *name, 
                           EdtDataType t, 
                           u_char *addr)

{
    SetupPointer(name, t, addr);
}

EdtDataValue::EdtDataValue(const char *name, u_char *bref)

{
    Setup(name, bref);
}

EdtDataValue::EdtDataValue(const char *name, u_short *bref)

{
    Setup(name, bref);
}

EdtDataValue::EdtDataValue(const char *name, int *bref)

{
    Setup(name, bref);
}

EdtDataValue::EdtDataValue(const char *name, u_int *bref)

{
    Setup(name, bref);
}

EdtDataValue::EdtDataValue(const char *name, u_int *bref, EdtBitMaskTag *masks)

{
    Setup(name, bref);
    bits = masks;
}

EdtDataValue::EdtDataValue(const char *name, double *bref)

{
    Setup(name, bref);
}

EdtDataValue::EdtDataValue(const char *name, uint64_t *bref)

{
    Setup(name, bref);
}

EdtDataValue::EdtDataValue(const char *name, char *cref, int maxl)

{
    Setup(name, cref, maxl);
}

EdtDataValue::EdtDataValue(const char *value)

{
    Init(TypeSTR);
    Set(value);
}

EdtDataValue::EdtDataValue(const int value)

{
    Init(TypeINT);
    Set(value);
}

int n_datavalues_inited = 0;

void EdtDataValue::Init(EdtDataType t)
{
    type = t;
    is_set = false;
    error = false;
    by_ref = false;
    read_only = false;
    v.uu = 0;
    radix = 0;
    size = 0;
    target = NULL;
    refresh = NULL;
    update = NULL;
    archive_mask = 0;
    bits = NULL;
    n_datavalues_inited++;

}

const char * EdtDataValue::GetConvertibleString(char *buf, int maxlen)

{
    const char *s;
    if (refresh)
        DoRefresh();

    if (type == TypeSTLSTR)
    {
        if (by_ref)
        {
            s = StringByRef().c_str();
        }
        else
        {   
            s = m_value.c_str();
        }
    }
    else if (type == EDT_TYPE_STR)
    {
        if (by_ref && v.str_ptr != NULL)
        {
            int len = size;
            if (len >= maxlen)
                len = maxlen-1;
            memcpy(buf,v.str_ptr,len);
            buf[len] = 0;
            s = buf;
        } 
        else
            s = m_value.c_str();
    }

    return s;

}

EdtDataValue::operator int()

{
    if (refresh)
        DoRefresh();

    if (by_ref)
    {
        switch (type)
        {
        case TypeBYTE:
            return (int) ByteByRef();
            break;
        case TypeUSHORT:
            return (int) UShortByRef();
            break;
        case TypeINT:
            return (int) IntByRef();
            break;
        case TypeUINT:
            return (int) UIntByRef();
            break;
        case TypeDOUBLE:
            return (int) DoubleByRef();
            break;
        case TypeUINT64:
            return (int) UInt64ByRef();
            break;

        case TypeSTLSTR:
        case TypeSTR:
            {
                const char *s;
                char temp[32];

                s = GetConvertibleString(temp,32);

                return (int) strtol(s,0,radix);
            }
            break;

        default:
            break;
        }
    }
    else
    {
        switch (type)
        {
        case TypeBYTE:
            return (int) v.b;
            break;
        case TypeUSHORT:
            return (int) v.w;
            break;
        case TypeINT:
            return (int) v.i;
            break;
        case TypeUINT:
            return (int) v.u;
            break;
        case TypeDOUBLE:
            return (int) v.d;
            break;
        case TypeUINT64:
            return (int) v.uu;
            break;

        case TypeSTLSTR:
        case TypeSTR:
            {
                const char *s;
                char temp[32];

                s = GetConvertibleString(temp,32);

                return (int) strtol(s,0,radix);
            }
            break;
        default:
            break;
        }
    }
    error = true;
    return 0;
}

EdtDataValue::operator u_int()
{
    if (refresh)
        DoRefresh();

    if (by_ref)
    {
        switch (type)
        {
        case TypeBYTE:
            return (u_int) ByteByRef();
            break;
        case TypeUSHORT:
            return (u_int) UShortByRef();
            break;
        case TypeINT:
            return (u_int) IntByRef();
            break;
        case TypeUINT:
            return (u_int) UIntByRef();
            break;
        case TypeDOUBLE:
            return (u_int) DoubleByRef();
            break;
        case TypeUINT64:
            return (u_int) UInt64ByRef();
            break;
        case TypeSTLSTR:

        case TypeSTR:
            {
                const char *s;
                char temp[32];

                s = GetConvertibleString(temp,32);

                return (u_int) strtol(s,0,radix);
            }
            break;
        default:
            break;
        }
    }
    else
    {
        switch (type)
        {
        case TypeBYTE:
            return (u_int) v.b;
            break;
        case TypeUSHORT:
            return (u_int) v.w;
            break;
        case TypeINT:
            return (u_int) v.i;
            break;
        case TypeUINT:
            return (u_int) v.u;
            break;
        case TypeDOUBLE:
            return (u_int) v.d;
            break;
        case TypeUINT64:
            return (u_int) v.uu;
            break;
        case TypeSTLSTR:
        case TypeSTR:
            {
                const char *s;
                char temp[32];

                s = GetConvertibleString(temp,32);

                return (u_int) strtoul(s,0,radix);
            }
            break;
        default:
            break;


        }
    }

    error = true;
    return 0;

}


EdtDataValue::operator u_short()
{
    if (refresh)
        DoRefresh();

    if (by_ref)
    {
        switch (type)
        {
        case TypeBYTE:
            return (u_short) ByteByRef();
            break;
        case TypeUSHORT:
            return (u_short) UShortByRef();
            break;
        case TypeINT:
            return (u_short) IntByRef();
            break;
        case TypeUINT:
            return (u_short) UIntByRef();
            break;
        case TypeDOUBLE:
            return (u_short) DoubleByRef();
            break;
        case TypeUINT64:
            return (u_short) UInt64ByRef();
            break;

        case TypeSTLSTR:
        case TypeSTR:
            {
                const char *s;
                char temp[32];

                s = GetConvertibleString(temp,32);

                return (u_short) strtol(s,0,radix);
            }
            break;
        default:
            break;

        }
    }
    else
    {
        switch (type)
        {
        case TypeBYTE:
            return (u_short) v.b;
            break;
        case TypeUSHORT:
            return (u_short) v.w;
            break;
        case TypeINT:
            return (u_short) v.i;
            break;
        case TypeUINT:
            return (u_short) v.u;
            break;
        case TypeDOUBLE:
            return (u_short) v.d;
            break;
        case TypeUINT64:
            return (u_short) v.uu;
            break;
        case TypeSTR:
        case TypeSTLSTR:
            {
                const char *s;
                char temp[32];

                s = GetConvertibleString(temp,32);


                return (u_short) strtoul(s,0,radix);
            }
            break;
        default:
            break;

        }
    }
    error = true;
    return 0;

}


EdtDataValue::operator double()
{
    if (refresh)
        DoRefresh();

    if (by_ref)
    {
        switch (type)
        {
        case TypeBYTE:
            return (double) ByteByRef();
            break;
        case TypeUSHORT:
            return (double) UShortByRef();
            break;
        case TypeINT:
            return (double) IntByRef();
            break;
        case TypeUINT:
            return (double) UIntByRef();
            break;
        case TypeDOUBLE:
            return (double) DoubleByRef();
            break;
        case TypeUINT64:
            return (double) UInt64ByRef();
            break;
        case TypeSTLSTR:
        case TypeSTR:
            {
                const char *s;
                char temp[32];

                s = GetConvertibleString(temp,32);

                // deal with hex values
                if (strncmp(s,"0x",2) == 0 ||
                    strncmp(s,"0X",2) == 0)
                {
#ifdef WIN32
                    uint64_t x = (uint64_t)_strtoui64(s,NULL,0);
#else
                    uint64_t x = (uint64_t)strtoull(s,NULL,0);
#endif
                    return (double) x;
                }

                return (double) atof(s);
            }
            break;

        default:
            error = true;
            return 0;
        }
    }
    else
    {
        switch (type)
        {
        case TypeBYTE:
            return (double) v.b;
            break;
        case TypeUSHORT:
            return (double) v.w;
            break;
        case TypeINT:
            return (double) v.i;
            break;
        case TypeUINT:
            return (double) v.u;
            break;
        case TypeDOUBLE:
            return (double) v.d;
            break;
        case TypeUINT64:
            return (double) v.uu;
            break;
        case TypeSTLSTR:
        case TypeSTR:
            {
                const char *s;
                char temp[32];

                s = GetConvertibleString(temp,32);
                return (double) atof(s);
            }
            break;

        default:
            error = true;
            return 0;
        }
    }
}


EdtDataValue::operator u_char()
{
    if (refresh)
        DoRefresh();

    if (by_ref)
    {
        switch (type)
        {
        case TypeBYTE:
            return (u_char) ByteByRef();
            break;
        case TypeUSHORT:
            return (u_char) UShortByRef();
            break;
        case TypeINT:
            return (u_char) IntByRef();
            break;
        case TypeUINT:
            return (u_char) UIntByRef();
            break;
        case TypeDOUBLE:
            return (u_char) DoubleByRef();
            break;
        case TypeUINT64:
            return (u_char) UInt64ByRef();
            break;
        case TypeSTLSTR:

        case TypeSTR:
            {
                const char *s;
                char temp[32];

                s = GetConvertibleString(temp,32);

                return (u_char) strtol(s,0,radix);
            }
            break;

        default:
            error = true;
            return 0;
        }
    }
    else
    {
        switch (type)
        {
        case TypeBYTE:
            return (u_char) v.b;
            break;
        case TypeUSHORT:
            return (u_char) v.w;
            break;
        case TypeINT:
            return (u_char) v.i;
            break;
        case TypeUINT:
            return (u_char) v.u;
            break;
        case TypeDOUBLE:
            return (u_char) v.d;
            break;
        case TypeUINT64:
            return (u_char) v.uu;
            break;
        case TypeSTLSTR:
        case TypeSTR:
            {
                const char *s;
                char temp[32];

                s = GetConvertibleString(temp,32);
                return (u_char) atof(s);
            }
            break;

        default:
            error = true;
            return 0;
        }
    }
}


EdtDataValue::operator uint64_t()
{
    if (refresh)
        DoRefresh();

    if (by_ref)
    {
        switch (type)
        {
        case TypeBYTE:
            return (uint64_t) ByteByRef();
            break;
        case TypeUSHORT:
            return (uint64_t) UShortByRef();
            break;
        case TypeINT:
            return (uint64_t) IntByRef();
            break;
        case TypeUINT:
            return (uint64_t) UIntByRef();
            break;
        case TypeDOUBLE:
            return (uint64_t) DoubleByRef();
            break;
        case TypeUINT64:
            return (uint64_t) UInt64ByRef();
            break;
        case TypeSTLSTR:

        case TypeSTR:
            {
                const char *s;
                char temp[32];

                s = GetConvertibleString(temp,32);
#ifdef WIN32
                return (uint64_t) _strtoui64(s,NULL,radix);
#else
                return  (uint64_t) strtoull(s,NULL,radix);
#endif      
            }
            break;


        default:
            error = true;
            return 0;
        }
    }
    else
    {
        switch (type)
        {
        case TypeBYTE:
            return (uint64_t) v.b;
            break;
        case TypeUSHORT:
            return (uint64_t) v.w;
            break;
        case TypeINT:
            return (uint64_t) v.i;
            break;
        case TypeUINT:
            return (uint64_t) v.u;
            break;
        case TypeDOUBLE:
            return (uint64_t) v.d;
            break;
        case TypeUINT64:
            return (uint64_t) v.uu;
            break;
         case TypeSTLSTR:
       case TypeSTR:
            {
                const char *s;
                char temp[32];

                s = GetConvertibleString(temp,32);
#ifdef WIN32
                return (uint64_t) _strtoui64(s,NULL,radix);
#else
                return  (uint64_t) strtoull(s,NULL,radix);
#endif      
            }
            break;

        default:
            error = true;
            return 0;
        }
    }
}

EdtDataValue::operator string&()

{
    if (type != TypeSTR || by_ref)
    {
        GetStringValue(m_value);
    }

    return m_value;
}

bool EdtDataValue::Set(EdtDataValue &other)

{

    if (by_ref)
    {
        switch(type)
        {
        case TypeBYTE:
            ByteByRef() = (u_char) other;
            break;
        case TypeUSHORT:
            UShortByRef() = (u_short) other;
            break;
        case TypeINT:
            IntByRef() = (int) other;
            break;
        case TypeUINT:
            UIntByRef() = (u_int) other;
            break;
        case TypeDOUBLE:
            DoubleByRef() = (double) other;
            break;
        case TypeUINT64:
            UInt64ByRef() = (uint64_t) other;
            break;

        case TypeSTLSTR:
            StringByRef() = (string &) other;
            break;

        case TypeSTR:
            strncpy(v.str_ptr,((string &) other).c_str(), size);
            break;

        default:
            return false;
        }

    }
    else
    {
        switch(type)
        {
        case TypeBYTE:
            v.b = (u_char) other;
            break;
        case TypeUSHORT:
            v.w = (u_short) other;
            break;
        case TypeINT:
            v.i = (int) other;
            break;
        case TypeUINT:
            v.u = (u_int) other;
            break;
        case TypeDOUBLE:
            v.d = (double) other;
            break;
        case TypeUINT64:
            v.uu = (uint64_t) other;
            break;
        case TypeSTLSTR:
        case TypeSTR:
            other.GetStringValue(m_value);
            break;

        default:
            return false;
        }
    }

    DoUpdate();

    return true;
}

void EdtDataValue::GetStringValue(string &s, const char *fmt) 

{
    char x[32];
    x[0] = 0;
    if (refresh)
        DoRefresh();

    if (type == TypeSTR)
    {

        if (by_ref && v.str_ptr)
        {
            m_value.clear();
            m_value = (char *) v.str_ptr;            
        }

        s = m_value;

        s.resize(size);

        // deal with possible zeroes

        size_t zero = s.find_last_not_of((char) 0);

        if (zero == string::npos)
            s.resize(0);
        else
            s.resize(zero+1);
        
    }
    else if (type == TypeSTLSTR)
    {
        if (by_ref)
            s = StringByRef();
        else
            s = m_value;
    }
    else if (bits)
    {
        u_int u;

        u = (u_int) *this;

        SetStringFromBits(s, u, bits);
    }
    else
    {
 
        if (fmt)
        {
            GetFormattedString(x,fmt);
        }
        else
        {
            GetFormattedString(x,StandardFormats(type));
        }

        s = x;
    }

}

void EdtDataValue::GetStringValue(char *s, const char *fmt)

{
    const char *f;

    if (refresh)
        DoRefresh();

    if (fmt)
        f = fmt;
    else
        f = StandardFormats(type);

    GetFormattedString(s, f);

}

void EdtDataValue::GetFormattedString(char *s, const char *f)

{
    if (by_ref)
    {
        switch (type)
        {
        case TypeBYTE:
            sprintf(s,f, ByteByRef());
            break;
        case TypeUSHORT:
            sprintf(s,f, UShortByRef());
            break;
        case TypeINT:
            sprintf(s,f, IntByRef());
            break;
        case TypeUINT:
            sprintf(s,f, UIntByRef());
            break;
        case TypeDOUBLE:
            sprintf(s,f, DoubleByRef());
            break;
        case TypeUINT64:
            sprintf(s,f, UInt64ByRef());
            break;
        case TypeSTLSTR:
            strcpy(s, StringByRef().c_str());
            break;
        case TypeSTR:
            if (v.str_ptr)
                m_value.assign(v.str_ptr, size);
            strcpy(s, m_value.c_str());
            break;
        default:
            break;
        }
    }
    else
    {
        switch(type)
        {
        case TypeBYTE:
            sprintf(s,f, v.b);
            break;
        case TypeUSHORT:
            sprintf(s,f, v.w);
            break;
        case TypeINT:
            sprintf(s,f, v.i);
            break;
        case TypeUINT:
            sprintf(s,f, v.u);
            break;
        case TypeDOUBLE:
            sprintf(s,f, v.d);
            break;
        case TypeUINT64:
            sprintf(s,f, v.uu);
            break;
        case TypeSTLSTR:
        case TypeSTR:
            strcpy(s, m_value.c_str());
            break;
        default:
            break;
        }
    }

}

void EdtDataValue::Set(const string &s)

{
    
    if (s.length() != 0)
        Set(s.c_str());

}


void EdtDataValue::Set(const char *s)

{
    if (s != NULL)
    {
        if (bits)
        {
            u_int u = GetBitsFromString(s, bits);

            Set(u);

            return;

        }
             
        if (by_ref)
        {

            switch(type)
            {
            case TypeBYTE:
                ByteByRef() = (u_char) strtoul(s,NULL,radix);
                break;
            case TypeUSHORT:
                UShortByRef() = (u_short) strtoul(s,NULL,radix);
                break;
            case TypeINT:
                IntByRef() = (int) strtol(s,NULL,radix);
                break;
            case TypeUINT:
                UIntByRef() = (u_int) strtoul(s,NULL,radix);
                break;
            case TypeDOUBLE:
                DoubleByRef() = (double) atof(s);
                break;
            case TypeUINT64:
#ifdef WIN32
                UInt64ByRef()= (uint64_t) _strtoui64(s,NULL,radix);
#else
                UInt64ByRef() = (uint64_t) strtoull(s,NULL,radix);
#endif
                break;

            case TypeSTLSTR:
                StringByRef() = s;
                break;

            case TypeSTR:
                strncpy(v.str_ptr,s, size);
                break;
            default:
                break;
            }
        }
        else
        {
            switch(type)
            {
            case TypeBYTE:
                v.b = (u_char) strtoul(s,NULL,radix);
                break;
            case TypeUSHORT:
                v.w = (u_short) strtoul(s,NULL,radix);
                break;
            case TypeINT:
                v.i = (int) strtol(s,NULL,radix);
                break;
            case TypeUINT:
                v.u = (u_int) strtoul(s,NULL,radix);
                break;
            case TypeDOUBLE:
                v.d = (double) atof(s);
                break;
            case TypeUINT64:
#ifdef WIN32
                v.uu = (uint64_t) _strtoui64(s,NULL,radix);
#else
                v.uu = (uint64_t) strtoull(s,NULL,radix);
#endif
                break;

            case TypeSTLSTR:
            case TypeSTR:
                m_value = s;
                by_ref = false;
                break;
            default:
                break;
            }
        }
    }

    DoUpdate();
}

static struct std_format {
    bool assigned;
    char *fmt;
    EdtDataType t;
} std_formats[8] = {
    {false,"%d",TypeBYTE},
    {false,"%d",TypeUSHORT},
    {false,"%d",TypeINT},
    {false,"0x%08x",TypeUINT},
    {false,"%f",TypeDOUBLE},
    {false,"0x%llx",TypeUINT64},
    {false,"%s",TypeSTR},
    {false,"XXXXXX",TypeNull}
};

char *EdtDataValue::StandardFormats(const EdtDataType t)

{
    switch(t)
    {
    case TypeBYTE:
        return std_formats[0].fmt;
    case TypeUSHORT:
        return std_formats[1].fmt;
    case TypeINT:
        return std_formats[2].fmt;
    case TypeUINT:
        return std_formats[3].fmt;
    case TypeDOUBLE:
        return std_formats[4].fmt;
    case TypeUINT64:
        return std_formats[5].fmt;
    case TypeSTR:
        return std_formats[6].fmt;

    default:
        break;
    }

    return std_formats[6].fmt;

}


bool EdtDataValue::Set(u_char b)

{
    if (type == TypeBYTE)
    {
        if (by_ref) 
            ByteByRef() = b;
        else
            v.b = b;
    }
    else
    {
        EdtDataValue x(TypeBYTE);
        x.v.b = b;
        return Set(x);
    }

    DoUpdate();

    return true;
}

bool EdtDataValue::Set(u_short w)

{
    if (type == TypeUSHORT)
    {
        if (by_ref) 
            UShortByRef() = w;
        else
            v.w = w;
    }
    else
    {
        EdtDataValue x(TypeUSHORT);
        x.v.w = w;
        return Set(x);
    }

        DoUpdate();


    return true;
}

bool EdtDataValue::Set(int i)

{
    if (type == TypeINT)
    {
        if (by_ref) 
            IntByRef() = i;
        else
            v.i = i;
    }
    else
    {
        EdtDataValue x(TypeINT);
        x.v.i = i;
        return Set(x);
    }

        DoUpdate();


    return true;
}

bool EdtDataValue::Set(u_int u)

{
    if (type == TypeUINT)
    {
        if (by_ref) 
            UIntByRef() = u;
        else
            v.u = u;
    }
    else
    {
        EdtDataValue x(TypeUINT);
        x.v.u = u;
        return Set(x);
    }


        DoUpdate();


    return true;
}

bool EdtDataValue::Set(double d)

{
    if (type == TypeDOUBLE)
    {
        if (by_ref) 
            DoubleByRef() = d;
        else
            v.d = d;
    }
    else
    {
        EdtDataValue x(TypeDOUBLE);
        x.v.d = d;
        return Set(x);
    }

        DoUpdate();


    return true;
}

bool EdtDataValue::Set(uint64_t uu)

{
    if (type == TypeUINT64)
    {
        if (by_ref) 
            UInt64ByRef() = uu;
        else
            v.uu = uu;  
    }
    else
    {
        EdtDataValue x(TypeUINT64);
        x.v.uu = uu;
        return Set(x);
    }

        DoUpdate();


    return true;
}

void EdtDataValue::SetName(const char *nm)
{
    if (nm)
        m_name = nm;
}

void EdtDataValue::SetComment(const char *comment)
{
    if (comment)
        m_comment = comment;
}

const char *EdtDataValue::Name()
{
    return m_name.c_str();
}

const char *EdtDataValue::Value()
{
    if (type != TypeSTR || by_ref)
        GetStringValue(m_value);

    return m_value.c_str();
}

const char *EdtDataValue::Comment()
{
    return m_comment.c_str();
}

void EdtDataValue::SetType(EdtDataType t)

{
    if (t == type)
        return;

    if (is_set)
    {
        // convert from previous value

        EdtDataValue old = *this;

        type = t;

        if (by_ref)
            by_ref = false;

        Set(old);
    }
    else
        type = t;
}

// take a piece of a longer string and set string value to it
//  - for tokenizing, for example
//

void EdtDataValue::SetStringRange(const char *s, int length)

{
    if (by_ref)
    {
        v.cstr_ptr = s;
        size = length;
    }
    else
        m_value.insert(0, s, length);

    is_set = true;
}

const char *edt_type_names(EdtDataType t)

{
    switch(t)
    {
    case TypeBYTE:
        return "byte";
        break;
    case TypeUSHORT:
        return "ushort";
        break;
    case TypeINT:
        return "int";
        break;
    case TypeUINT:
        return "uint";
        break;
    case TypeDOUBLE:
        return "double";
        break;
    case TypeUINT64:
        return "uint64";
        break;
    case TypeSTR:
        return "char buf";
        break;
     case TypeSTLSTR:
        return "string";
        break;
     case TypeBITFIELD:
        return "bitfield";
        break;
    default:
        break;
   }

    return "unknown";

}

void EdtDataValue::ByPointer(const bool state)
{
    by_ref = state;
}

bool EdtDataValue::ByPointer()

{
    return by_ref;
}

void EdtDataValue::Radix(const int state)
{
    radix = state;
}

int EdtDataValue::Radix()

{
    return radix;
}

EdtDataValue & EdtDataValue::operator=(const int i)

{
    Set(i);
    return *this;
}

EdtDataValue & EdtDataValue::operator=(const u_char b)

{
    Set(b);
    return *this;
}
EdtDataValue & EdtDataValue::operator=(const u_short s)

{
    Set(s);
    return *this;
}
EdtDataValue & EdtDataValue::operator=(const double d)

{
    Set(d);
    return *this;
}
EdtDataValue & EdtDataValue::operator=(const uint64_t uu)

{
    Set(uu);
    return *this;
}
EdtDataValue & EdtDataValue::operator=(const u_int u)

{
    Set(u);
    return *this;
}

EdtDataValue & EdtDataValue::operator=(const char * s)

{
    Set(s);
    return *this;
}

EdtDataValue & EdtDataValue::operator=(const string &s)

{
    Set(s);
    return *this;
}


const char *EdtDataValue::DebugPrint(char *buf)

{
    string s;
    char val[256];

    GetStringValue(val);

    sprintf(buf, "Name = %s Type = %s Value = %s",
        m_name.c_str(), 
        edt_type_names(type),
        val);

    return buf;

}

void EdtDataValue::Dump(const char *tag)

{
    char buf[256];

    if (tag)
        SysLog << tag << " ";

    SysLog << DebugPrint(buf) << endl;

}

void EdtDataValue::Refresh(ParmObject *tgt, DV_Notifier r)

{
    target = tgt;
    refresh = r;
}

DV_Notifier EdtDataValue::Refresh()

{
    return refresh;
}


void EdtDataValue::Update(ParmObject *tgt, DV_Notifier r)

{
    target = tgt;
    update = r;
}

DV_Notifier EdtDataValue::Update()

{
    return update;
}


void EdtDataValue::SetupPointer(const char *nm, EdtDataType t, u_char *bref)

{
    Init(t);
    SetName(nm);
    v.base = (u_char *) bref;
    if (v.base != NULL)
        ByPointer(true);
    else
        ByPointer(false);

}

void EdtDataValue::Setup(const char *nm, u_char *bref)

{
    SetupPointer(nm, TypeBYTE, bref);
}


void EdtDataValue::Setup(const char *nm, u_short *bref)

{
    SetupPointer(nm, TypeUSHORT, (u_char *) bref);
}

void EdtDataValue::Setup(const char *nm, int *bref)

{
    SetupPointer(nm, TypeINT, (u_char *) bref);
}

void EdtDataValue::Setup(const char *nm, u_int *bref)

{
    SetupPointer(nm, TypeUINT, (u_char *) bref);
}

void EdtDataValue::Setup(const char *nm, double *bref)

{
    SetupPointer(nm, TypeDOUBLE, (u_char *) bref);
}

void EdtDataValue::Setup(const char *nm, uint64_t *bref)

{
    SetupPointer(nm, TypeUINT64, (u_char *) bref);
}


void EdtDataValue::Setup(const char *nm, char *cref, int maxl)

{
    SetupPointer(nm, TypeSTR, (u_char *) cref);
    size = maxl;
}

void EdtDataValue::Setup(const char *nm, string *sref)

{
    SetupPointer(nm, TypeSTLSTR, (u_char *) sref);
}

void EdtDataValue::DoRefresh()

{
    if (refresh && target)
        refresh(target, this);
}

void EdtDataValue::DoUpdate()

{
    is_set = true;

    if (update && target)
        update(target, this);
}

int  EdtDataValue::MaxChars()

{
    if (size)
        return size;
    else
        return 0;
}

const char * EdtDataValue::GetFullStringValue()

{
    if (by_ref && type == TypeSTR)
        return v.cstr_ptr;
    else
        return NULL;

}

/**
 * @fn  ostream & operator<<(ostream & out, EdtDataValue &dv)
 *
 * @brief   overloaded ostream operator to print an EdtDataValue
 *
 * @date    2/24/2012
 *
 * @param [in,out]  out The ostream.
 * @param [in,out]  dv  The dv.
 *
 * @return  The shifted result.
 ***/

ostream & operator<<(ostream & out, EdtDataValue &dv)

{
    char val[256];

    dv.GetStringValue(val);

    out << val;

    return out;

}

   
EdtDataType EdtDataValue::Type()
{
    return type;
}

u_int EdtDataValue::Archive()
{
    return archive_mask;
}
void EdtDataValue::Archive(const u_int mask)
{
    archive_mask = mask;
}

void EdtDataValue::SetBitTags(EdtBitMaskTag *masks)
{
    bits = masks;
}
EdtBitMaskTag *EdtDataValue::GetBitTags()
{
    return bits;
}
