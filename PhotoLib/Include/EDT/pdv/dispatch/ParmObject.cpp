/**
*
*  @file ParmObject.cpp 
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
 * @file ParmObject.cpp
 *
 * Copyright (c) Engineering Design Team 2010
 * All Rights Reserved
 */



#include "ParmObject.h"

#include "StringUtils.h"

#include <sstream>
#include <fstream>
#include <typeinfo>

/* base class for objects with a list of named attributes stored in a 
   vector of EdtDataValue objects */

static int get_nchildren(void *p, EdtDataValue *dv)

{
    dv->Set(((ParmObject *)p)->GetNChildren());
    return 0;
}

static int get_nparms(void *p, EdtDataValue *dv)

{
    dv->Set(((ParmObject *)p)->GetNParms());
    return 0;
}


void ParmObject::SetupParms()

{

    delimiter = "=";
    indent = false;

    AddParm("NChildren", TypeINT, NULL, get_nchildren, NULL, PARM_INTERNAL_MASK);
    
    AddParm("NParms", TypeINT, NULL, get_nparms, NULL, PARM_INTERNAL_MASK);
    
}


ParmObject::ParmObject()

{
    throwing_errors = false;
    fail_not_found = false;
    parent = NULL;
    Name("");
    SetupParms();
    m_last_error = 0;
    append_ok = false;
    owns_tree = false;
}

ParmObject::ParmObject(const char *nm)
{
    throwing_errors = false;
    fail_not_found = false;
    parent = NULL;
    Name(nm);
    SetupParms();
    m_last_error = 0;
    append_ok = false;
    owns_tree = false;
}

ParmObject::ParmObject(const char *nm, const string &source) 
{
    throwing_errors = false;
    fail_not_found = false;
    parent = NULL;
    Name(nm);
    SetupParms();
    m_last_error = 0;

    append_ok = true;
    owns_tree = true;

    SetParameters(source);
}

ParmObject::~ParmObject()
{
    if (owns_tree)
        Destroy();
}

void ParmObject::Destroy()
{
    size_t i;

    for (i=0;i<children.size();i++)
    {
        delete children[i];
    }
    children.clear();

    for (i=0;i<descriptor.size();i++)
    {
        delete descriptor[i];
    }
    descriptor.clear();
 
}

int ParmObject::SetParameters(ParmItem *items)
{
    int i;
    for (i = 0; (items[i].name); i++)
    {
        AddParm(items + i);
    }

    return RET_SUCCESS;

}

void ParmObject::GetParameters(ostream &os, const u_int mask, bool followchildren)
{
    string prefix;

    GetParameters(os, mask, 0, prefix, followchildren);

}

void ParmObject::GetParameters(ostream &os, const u_int mask, const int depth, string &prefix, bool followchildren)
{
    size_t i;

    int prefixend = prefix.size();
    char *spaces = "";

    if (indent)
    {
        spaces = new char[depth * 4 + 1];
        spaces[0] = 0;

        for (i=0;i< (size_t) depth;i++)
            strcat(spaces,"    ");
    }

    if (depth > 0)
    {
        prefix.append(Name());
        prefix.append(".");
    }

    for (i=0;i<descriptor.size();i++)
    {
        string s;
        char *quotes;
        EdtDataValue *dv = descriptor[i];

        if (dv->Archive() == 0 || (mask && (dv->Archive() & mask)))
        {
            dv->GetStringValue(s);

            if (s.find(" ") != string::npos)
            {
                quotes = "\"";
            }
            else
            {
                quotes = "";
            }

            os << spaces << prefix << dv->Name() << " " << 
                delimiter << " " << quotes << s << quotes << endl;
        }
    }

    if (followchildren)
        for (i=0;i<children.size();i++)
        {
            children[i]->GetParameters(os, mask, depth + 1, prefix);
        }
    
    prefix.resize(prefixend);

    if (indent)
        delete []spaces;
}

void ParmObject::GetParameters(string &output, const u_int mask, bool followchildren)
{
    ostringstream os;
    GetParameters(os, mask, followchildren);
    output = os.str();
}

void ParmObject::DumpParameters()

{
    GetParameters(SysLog);
}

#define MAXINPUT 1024


int ParmObject::Set(ParmObject *source)
{
    // Walk the tree assigning values
    ParmObject *child;
    size_t i;

    EdtDataValue *from;
    EdtDataValue *to;

    string subkeys;

    for (i=0;i<descriptor.size();i++)
    {
        to = descriptor[i];
        from = source->LookupParm(to->Name());

        if (to && from)
            to->Set(*from);
    }

    for (i=0;i<children.size();i++)
    {
        child = source->LookupChild(children[i]->Name());

        if (child)
            children[i]->Set(child);
    }

    return 0;
}

EdtDataValue *ParmObject::AddNewParm(const string &key, const string &value)
{
    EdtDataValue *dv;
    vector<string> keys;
    
    string_split(key, ".", keys);


    ParmObject *parent = this;
    size_t i = 0;
    ParmObject *child = this;

    size_t last_key = keys.size() -1;

    dv = new EdtDataValue();
    dv->SetName(keys[last_key].c_str());

    dv->Set(value);

    for (i=0;i<last_key;i++)
    {
        child = child->LookupChild(keys[i]);

        if (!child)
        {
            child = new ParmObject(keys[i].c_str());
            child->SetAppendOk(true);
            child->SetOwnTree(true);

            parent->AddChild(child);  
        }

        parent = child;


    }

    child->AddParm(dv);

    return dv;

}

int ParmObject::SetParameters(istream &is, const LookupType lookup)
{
    string key;
    string value;
    string s;
    EdtDataValue *dv;

    char buffer[MAXINPUT];

    while (!is.eof())
    {
        is.getline(buffer, MAXINPUT-1);

        s = buffer;

        if (string_split2(s,delimiter, key, value))
        {
            key = stringtrim(key);
            value = stringtrim(value);
            value = string_dequote(value);

            if (dv = LookupParm(key, lookup))
            {

                // strip quotes
                dv->Set(value);
            }
            else
            {
                if (append_ok)
                    dv = AddNewParm(key, value);
                else
                {
                    if (lookup == LookupAll && fail_not_found)
                        SetLastError(ERROR_BADPARAM,"Unknown parameter %s\n",key.c_str());
                }
            }
        }

    }

    return RET_SUCCESS;

}

int ParmObject::SetParameters(const string &s, const LookupType lookup)

{
    istringstream is;

    is.str(s);

    return SetParameters(is, lookup);

}
/**
 * \brief
 * FIX !!!  Write brief comment for LoadParameters here.
 * 
 * \param fname
 * FIX !!! Description of parameter fname.
 * 
 * \returns
 * FIX !!! Write description of return value here.
 * 
 */
int ParmObject::LoadParameters(const string &fname, const LookupType lookup)

{
    ifstream is(fname.c_str());

    if (is.fail())
    {
        return SetLastError(ERROR_BADPATH,"Fail to open %s\n", fname.c_str());

    }
    else
        return SetParameters(is, lookup);

}

int ParmObject::SaveParameters(const string &fname, const u_int level)

{
    ofstream os(fname.c_str());
    
    if (os.fail())
    {
        return SetLastError(ERROR_BADPATH,"Fail to open %s\n", fname.c_str());

    }
    else
    {
        GetParameters(os, level);
    }

    os.close();

    return RET_SUCCESS;

}

int ParmObject::Load(const string &cfgname)

{
 
    string fullname;
    string newcfgname = cfgname;

    if (cfgname[0] == '/' || cfgname[0] == '\\' || cfgname[1] == ':')
    {
        size_t lastslash = cfgname.find_last_of("/");
        if (lastslash == string::npos)
        {
            lastslash = cfgname.find_last_of("\\");
        }

        if (lastslash != string::npos)
        {
            newcfgname = cfgname.substr(lastslash+1);
            m_configpath = cfgname.substr(0,lastslash);
        }

    }
    if (m_configpath.size() && m_configpath[m_configpath.size()-1] != '\\' && 
        m_configpath[m_configpath.size()-1] != '/')
        m_configpath += '/';

    fullname = m_configpath + newcfgname;

    m_configname = newcfgname;

	return LoadParameters(fullname);
}

int ParmObject::Set(const string & name, u_char b)

{
    EdtDataValue *dv = LookupParm(name);
    int rc = RET_SUCCESS;

    if (dv)
        dv->Set(b);
    else
    {
        return SetLastError(ERROR_BADPARAM,"Unknown parameter %s",name.c_str());
    }

    return rc;
}

int ParmObject::Set(const string & name, u_short w)

{
    EdtDataValue *dv = LookupParm(name);
    int rc = RET_SUCCESS;
    if (dv)
        dv->Set(w);
    else
    {
        return SetLastError(ERROR_BADPARAM,"Unknown parameter %s",name.c_str());
    }

    return rc;
}

int ParmObject::Set(const string & name, int i)

{
    EdtDataValue *dv = LookupParm(name);
    int rc = RET_SUCCESS;
    if (dv)

        dv->Set(i);
    else
    {
        return SetLastError(ERROR_BADPARAM,"Unknown parameter %s",name.c_str());
    }

    return rc;
}
int ParmObject::Set(const string & name, u_int u)

{
    EdtDataValue *dv = LookupParm(name);
    int rc = RET_SUCCESS;
    if (dv)

        dv->Set(u);
    else
    {
        return SetLastError(ERROR_BADPARAM,"Unknown parameter %s",name.c_str());
    }

    return rc;
}
int ParmObject::Set(const string & name, double d)

{
    EdtDataValue *dv = LookupParm(name);
    int rc = RET_SUCCESS;
    if (dv)

        dv->Set(d);
    else
    {
        return SetLastError(ERROR_BADPARAM,"Unknown parameter %s",name.c_str());
    }

    return rc;
}
int ParmObject::Set(const string & name, uint64_t uu)

{
    EdtDataValue *dv = LookupParm(name);
    int rc = RET_SUCCESS;
    if (dv)
 
        dv->Set(uu);
    else
    {
        return SetLastError(ERROR_BADPARAM,"Unknown parameter %s",name.c_str());
    }

    return rc;
}
int ParmObject::Set(const string & name, EdtDataValue &other)

{
    EdtDataValue *dv = LookupParm(name);
    int rc = RET_SUCCESS;
    if (dv)
        dv->Set(other);
    else
    {
        return SetLastError(ERROR_BADPARAM,"Unknown parameter %s",name.c_str());
    }

    return rc;
}

int ParmObject::Set(const string & name, const string &s)

{
    EdtDataValue *dv = LookupParm(name);
    int rc = RET_SUCCESS;
    if (dv)
    {
        dv->Set(s);
    }
    else
    {
        return SetLastError(ERROR_BADPARAM,"Unknown parameter %s",name.c_str());
    }

    return rc;
}

int ParmObject::Set(const string & name, const char *s)

{
    EdtDataValue *dv = LookupParm(name);
    int rc = RET_SUCCESS;
    if (dv)
        dv->Set(s);
    else
    {
        return SetLastError(ERROR_BADPARAM,"Unknown parameter %s",name.c_str());
    }

    return rc;
}

int ParmObject::GetStringValue(const string & name, string &s, const char *fmt)

{
    EdtDataValue *dv = LookupParm(name);
    int rc = RET_SUCCESS;
    if (dv)
        dv->GetStringValue(s, fmt);
    else
    {
        return SetLastError(ERROR_BADPARAM,"Unknown parameter %s",name.c_str());
    }

    return rc;
}

int ParmObject::GetStringValue(const string & name, char *s, const char *fmt)

{
    EdtDataValue *dv = LookupParm(name);
    int rc = RET_SUCCESS;
    if (dv)
         dv->GetStringValue(s, fmt);    
    else
    {
        return SetLastError(ERROR_BADPARAM,"Unknown parameter %s",name.c_str());
    }

    return rc;
}

int ParmObject::Get(const string & name, int &value)

{   
    EdtDataValue *dv = LookupParm(name);
    int rc = RET_SUCCESS;

    if (dv)
        value = (int) *dv;
    else
    {
        return SetLastError(ERROR_BADPARAM,"Unknown parameter %s",name.c_str());
    }

    return rc;
}

int ParmObject::Get(const string & name, u_int &value)

{   
    EdtDataValue *dv = LookupParm(name);
    int rc = RET_SUCCESS;

    if (dv)
        value = (u_int) *dv;
    else
    {
        return SetLastError(ERROR_BADPARAM,"Unknown parameter %s",name.c_str());
    }

    return rc;
}

int ParmObject::Get(const string & name, u_short &value)

{   
    EdtDataValue *dv = LookupParm(name);
    int rc = RET_SUCCESS;

    if (dv)
        value = (u_short) *dv;
    else
    {
        return SetLastError(ERROR_BADPARAM,"Unknown parameter %s",name.c_str());
    }

    return rc;
}

int ParmObject::Get(const string & name, double &value)

{   
    EdtDataValue *dv = LookupParm(name);
    int rc = RET_SUCCESS;

    if (dv)
        value = (double) *dv;
    else
    {
        return SetLastError(ERROR_BADPARAM,"Unknown parameter %s",name.c_str());
    }

    return rc;
}

int ParmObject::Get(const string & name, u_char &value)

{   
    EdtDataValue *dv = LookupParm(name);
    int rc = RET_SUCCESS;

    if (dv)
        value = (u_char) *dv;
    else
    {
        return SetLastError(ERROR_BADPARAM,"Unknown parameter %s",name.c_str());
    }

    return rc;
}

int ParmObject::Get(const string & name, uint64_t &value)

{   
    EdtDataValue *dv = LookupParm(name);
    int rc = RET_SUCCESS;

    if (dv)
        value = (uint64_t) *dv;
    else
    {
        return SetLastError(ERROR_BADPARAM,"Unknown parameter %s",name.c_str());
    }

    return rc;
}

int ParmObject::Get(const string & name, string &value)

{   
    EdtDataValue *dv = LookupParm(name);

    int rc = RET_SUCCESS;

    if (dv)
        value = (string) *dv;
    else
    {
        return SetLastError(ERROR_BADPARAM,"Unknown parameter %s",name.c_str());
    }

    return rc;
}

EdtDataValue * ParmObject::LookupParm(const string & name, const LookupType lookup)

{   
    size_t i;
    string test;
    string first, rest;

// split name on the first '.'    

    if (string_split2(name, ".", first, rest))
    {
        if (lookup == LookupCurrent)
            return NULL;

        for (i=0;i<children.size();i++)
        {        
            if (first == children[i]->Name())
            {
                return children[i]->LookupParm(rest,LookupAll);
            }
        }
    }
    else
    {
        if (lookup == LookupChildren)
            return NULL;

        for (i=0;i<descriptor.size();i++)
        {
            if (!strcmp(descriptor[i]->Name(), name.c_str()))
                return descriptor[i];
        }
    }

    return NULL;
}

int ParmObject::_ObjSize()
{
    return sizeof(*this);
}

 int ParmObject::ObjSize()
{
    return _ObjSize();
}

const char *ParmObject::TypeName()
{
    return typeid(*this).name();
}

void ParmObject::AddChild(ParmObject *child)

{
    if (child)
        children.push_back(child);
}

void ParmObject::RemoveChild(ParmObject *child)

{
    if (child)
    {
        // find it
        vector<ParmObject *>::iterator it;
        it = children.begin();
        while (it != children.end())
        {
            if (*it == child)
            {
                children.erase(it);
                break;
            }
            it++;
        }
    }     
}

EdtDataValue * ParmObject::AddParm(EdtDataValue *dv)

{
    if (dv)
        descriptor.push_back(dv);
    return dv;
}

EdtDataValue * ParmObject::AddParm(ParmItem *pi)

{
    EdtDataValue *dv = new EdtDataValue(pi->name, pi->type);
    if (dv)
        descriptor.push_back(dv);
    return dv;
}

EdtDataValue * ParmObject::AddParm(const string &name, 
                         EdtDataType t, 
                         void *addr, 
                         DV_Notifier refresh_cb, 
                         DV_Notifier update_cb,
                         u_int archive_mask)

{
    EdtDataValue *dv = new EdtDataValue(name.c_str(), t, (u_char *) addr);
    
    if (dv)
    {
        dv->Refresh(this, refresh_cb);
        dv->Update(this, update_cb);
        dv->Archive(archive_mask);
        descriptor.push_back(dv);
    }

    return dv;
}

EdtDataValue * ParmObject::AddParm(const string &name, 
                         u_char *addr,
                         DV_Notifier refresh_cb, 
                         DV_Notifier update_cb,
                         u_int archive_mask)

{
   return AddParm(name, TypeBYTE, addr, refresh_cb, update_cb, archive_mask);
}

EdtDataValue * ParmObject::AddParm(const string &name, 
                         u_short *addr,
                         DV_Notifier refresh_cb, 
                         DV_Notifier update_cb,
                         u_int archive_mask)

{
   return AddParm(name, TypeUSHORT, addr, refresh_cb, update_cb, archive_mask);
}

EdtDataValue * ParmObject::AddParm(const string &name, 
                         int *addr,
                         DV_Notifier refresh_cb, 
                         DV_Notifier update_cb,
                         u_int archive_mask)

{
   return AddParm(name, TypeINT, addr, refresh_cb, update_cb, archive_mask);
}

EdtDataValue * ParmObject::AddParm(const string &name, 
                         u_int *addr,
                         DV_Notifier refresh_cb, 
                         DV_Notifier update_cb,
                         u_int archive_mask)

{
   return AddParm(name, TypeUINT, addr, refresh_cb, update_cb, archive_mask);
}

EdtDataValue * ParmObject::AddParm(const string &name, 
                         u_int *addr,
                         EdtBitMaskTag *bits,
                         DV_Notifier refresh_cb, 
                         DV_Notifier update_cb,
                         u_int archive_mask)

{
   EdtDataValue *p =  AddParm(name, TypeUINT, addr, refresh_cb, update_cb, archive_mask);
   if (p)
       p->SetBitTags(bits);

   return p;
}

EdtDataValue * ParmObject::AddParm(const string &name, 
                         double *addr,
                         DV_Notifier refresh_cb, 
                         DV_Notifier update_cb,
                         u_int archive_mask)

{
   return AddParm(name, TypeDOUBLE, addr, refresh_cb, update_cb, archive_mask);
}

EdtDataValue * ParmObject::AddParm(const string &name, 
                         uint64_t *addr,
                         DV_Notifier refresh_cb, 
                         DV_Notifier update_cb,
                         u_int archive_mask)

{
   return AddParm(name, TypeUINT64, addr, refresh_cb, update_cb, archive_mask);
}

EdtDataValue * ParmObject::AddParm(const string &name, 
                         char *addr,
                         int maxlen,
                         DV_Notifier refresh_cb, 
                         DV_Notifier update_cb,
                         u_int archive_mask)

{
    EdtDataValue *dv = new EdtDataValue(name.c_str(), addr, maxlen);
    
    if (dv)
    {
        dv->Refresh(this, refresh_cb);
        dv->Update(this, update_cb);
        AddParm(dv);
    }

    return dv;
}

EdtDataValue * ParmObject::AddParm(const string &name, 
                         string *addr,
                         DV_Notifier refresh_cb, 
                         DV_Notifier update_cb,
                         u_int archive_mask)

{
    return AddParm(name, TypeSTLSTR, addr, refresh_cb, update_cb, archive_mask);

}

int ParmObject::GetNChildren()

{
    return children.size();
}

int ParmObject::GetNParms(const bool plus_children)

{
    int n = descriptor.size();

    if (plus_children)
    {
        size_t i;
        for (i=0;i<children.size();i++)
        {
            ParmObject *child = GetChild(i);
            if (child)
                n += child->GetNParms(true);
        }
    }

    return n;
}

ParmObject *ParmObject::GetChild(const size_t index)

{
    if (index < children.size())
        return children[index];
    else
        return NULL;
}

ParmObject *ParmObject::LookupChild(const string &name)

{
    size_t i;
    string test;
    string first, rest;
    bool godown = false;

// split name on the first '.'
        

    if (string_split2(name, ".", first, rest))
    {
        test = first;

        godown = true;
    }
    else
        test = name;

    for (i=0;i<children.size();i++)
    {
        
        if (test == children[i]->Name())
        {
            //
            if (godown)
                return children[i]->LookupChild(rest);
            else
                return children[i];
        }
    }

    return NULL;

}

const string & ParmObject::Name()

{
    return m_obj_name;
}


void ParmObject::Name(const string &nm)

{
    m_obj_name = nm;
}

    
void ParmObject::Name(const char *nm)

{
    if (nm != NULL)
        m_obj_name = nm;
    else
        m_obj_name = "";
}

void ParmObject::SetParmArchive(const string &nm, u_char level)

{
    EdtDataValue *dv = LookupParm(nm);

    if (dv)
    {
        dv->Archive(level);
    }

}

const string & ParmObject::GetStatus()
{
    return m_status;
}

void ParmObject::SetStatus(const string &status)
{
    m_status = status;
}


int ParmObject::SetLastError(int error_type, const char *fmt, ...)

{
    va_list stack;

    va_start(stack,fmt);

    char buffer[1024];

    buffer[0] = 0;

    if (Name().size())
    {
        strcpy(buffer,Name().c_str());
        strcat(buffer,": ");
    }

    size_t len = strlen(buffer);

    vsprintf(buffer + len, fmt, stack);

    m_last_error = error_type;
    m_last_error_string = buffer+len;

    SysError.Message(error_type, buffer);

    if (throwing_errors)
        throw m_last_error_string;

    return error_type;
}


int ParmObject::GetLastErrorString(string &s)

{
    s = m_last_error_string;
    return m_last_error;
}


const string &ParmObject::GetLastErrorString()

{
    return m_last_error_string;
}


const string & ParmObject::GetDelimiter()

{
    return delimiter;
}

void ParmObject::SetDelimiter(const string &d)

{
    delimiter = d;
}

void ParmObject::PrintState(ostream &out)

{
    out << "ParmObject" << endl << endl;
    out << "obj_name           " << m_obj_name << endl;
    out << "typename           " << m_typename << endl;
    out << "nchildren          " << m_nchildren << endl;
    out << "nparms             " << m_nparms << endl;
    out << "delimiter          " << delimiter << endl;
    out << "indent             " << indent << endl;
    out << "status             " << m_status << endl;
    out << "last_error         " << m_last_error << endl;
    out << "last_error_string  " << m_last_error_string << endl;
    out << "throwing_errors    " << throwing_errors << endl;
    out << "fail_not_found     " << fail_not_found << endl;
    out << "append_ok          " << append_ok << endl;
    out << "owns_tree          " << owns_tree << endl;

    out << endl;

}

void ParmObject::SetConfigName(const string &name)

{
    m_configname = name;
}   


const string & ParmObject::GetConfigName()

{
    return m_configname;
}


void ParmObject::SetConfigPath(const string &name)

{
    m_configpath = name;
}   


const string & ParmObject::GetConfigPath()

{
    return m_configpath;
}

int ParmObject::ParmNames(string &names)

{
    size_t i;
    string s="";
    int n = descriptor.size();

    for (i=0;i<descriptor.size();i++)
    {
        if (i>0)
            s += "\n";
        
        s += descriptor[i]->Name();

    }

    for (i=0;i<children.size();i++)
    {
        string childs;
        int childn;

        childn = children[i]->ParmNames(childs);

        if (childs.size())
        {
            s += "\n";
            s += childs;
        }

        n += childn;

    }

    names = s;

    return n;
}
