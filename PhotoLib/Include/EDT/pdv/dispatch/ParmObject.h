/**
*
*  @file ParmObject.h 
* 
*  @copyright Copyright Engineering Design Team 2012
*  All Rights Reserved
*
**/
/** ********************************
* 
* @file edtimage/ParmObject.h
*
* Copyright (c) Engineering Design Team 2010
* All Rights Reserved
*/




#ifndef _EDT_OBJECT_PARMS_H_
#define _EDT_OBJECT_PARMS_H_

#include "ErrorHandler.h"
#include "EdtDataValue.h"


/*** 
Base class for objects with a list of named attributes stored in a 
vector of EdtDataValue objects */

using namespace std;

#include <vector>
#include <string>
#include <iostream>

struct ParmItem {
    char *name;
    int  offset;
    int  length; // for strings 
    EdtDataType type;
};

#define NAME_SEPARATOR '.'

// 32 bits of mask for DataValue

const u_int PARM_INTERNAL_MASK=0x80000000;
const u_int PARM_EXTERNAL_MASK=0x7fffffff;

class ParmObject 

{

protected:

    string m_obj_name;

    vector <EdtDataValue *> descriptor;

    vector <ParmObject *> children;

    ParmObject *parent;

    virtual int _ObjSize();

    // easy access via name
    EdtDataValue m_typename;
    EdtDataValue m_nchildren;
    EdtDataValue m_nparms;

    string delimiter;
    bool indent;
    string m_status;

    int m_last_error;
    string m_last_error_string;

    bool throwing_errors;
    bool fail_not_found;

    bool append_ok;

    bool owns_tree;

	string m_configpath;
    string m_configname;

public:

    enum LookupType {
        LookupAll,
        LookupCurrent,
        LookupChildren
    };

    ParmObject();
    ParmObject(const char *nm);
    ParmObject(const char *nm, const string &source);
    virtual ~ParmObject();

    virtual void            SetConfigPath(const string &path);
    virtual const string &  GetConfigPath();

	virtual void            SetConfigName(const string &path);
    virtual const string &  GetConfigName();

    virtual void GetParameters(ostream &os, 
        const u_int mask, 
        const int depth, 
        string &prefix,
        bool followchildren=true);

    virtual void GetParameters(ostream &os, const u_int mask = PARM_INTERNAL_MASK, bool followchildren=true);
    virtual void GetParameters(string &s,   const u_int mask = PARM_INTERNAL_MASK, bool followchildren=true);
    virtual void DumpParameters();

    virtual int SetParameters(istream &os, const LookupType descend = LookupAll);
    virtual int SetParameters(const string &s, const LookupType descend = LookupAll);

    virtual int LoadParameters(const string &fname, const LookupType descend = LookupAll);
    virtual int SaveParameters(const string &fname, const u_int mask = PARM_INTERNAL_MASK);

    virtual int GetNChildren();
    virtual int GetNParms(const bool pluschildren = false);

    virtual const string & GetStatus();
    virtual void SetStatus(const string &status);

    virtual const string & GetLastErrorString();

    virtual int SetLastError(const int error_type, const char *format, ...);
    virtual int GetLastErrorString(string &s);
	virtual int Load(const string &fname);
    virtual const string &Name();

    virtual int Get(const string & name, string & value);
    virtual int Get(const string & name, int &value);

    virtual EdtDataValue * LookupParm(const string & name, const LookupType lookup = LookupAll);

    virtual int Set(const string & name, const string &s);
    virtual int GetStringValue(const string & name, 
        string &s, const char *fmt = NULL);

    virtual int Set(const string & name, u_char b);
    virtual int Set(const string & name, u_short w);
    virtual int Set(const string & name, int i);
    virtual int Set(const string & name, u_int u);
    virtual int Set(const string & name, double d);
    virtual int Set(const string & name, uint64_t uu);
    virtual int Set(const string & name, EdtDataValue &other);
    virtual int Set(const string & name, const char *s);

    virtual int GetStringValue(const string & name, 
        char *s, const char *fmt = NULL);

    virtual int Get(const string & name, u_int &value);
    virtual int Get(const string & name, u_short &value);
    virtual int Get(const string & name, double &value);
    virtual int Get(const string & name, u_char &value);
    virtual int Get(const string & name, uint64_t &value);

    virtual EdtDataValue * Items(const int index)
    {
        if (index >= 0 && index < (int)descriptor.size())
            return descriptor[index];

        return NULL;
    }

    virtual int ObjSize();
    virtual const char * TypeName();

    virtual void AddChild(ParmObject *child);
    virtual void RemoveChild(ParmObject *child);

    virtual EdtDataValue *AddParm(EdtDataValue *dv);
    virtual EdtDataValue *AddParm(ParmItem *parmi);

    virtual EdtDataValue * AddParm(const string &name, 
        EdtDataType t, 
        void *addr = NULL, 
        DV_Notifier refresh_cb = NULL, 
        DV_Notifier update_cb = NULL,
        u_int archive_mask = 0);

    virtual EdtDataValue * AddParm(const string &name, 
        u_char *addr,
        DV_Notifier refresh_cb = NULL, 
        DV_Notifier update_cb = NULL,
        u_int archive_mask = 0);

    virtual EdtDataValue * AddParm(const string &name, 
        u_short *addr,
        DV_Notifier refresh_cb = NULL, 
        DV_Notifier update_cb = NULL,
        u_int archive_mask = 0);

   virtual  EdtDataValue * AddParm(const string &name, 
        int *addr,
        DV_Notifier refresh_cb = NULL, 
        DV_Notifier update_cb = NULL,
        u_int archive_mask = 0);

    virtual EdtDataValue * AddParm(const string &name, 
        u_int *addr,
        DV_Notifier refresh_cb = NULL, 
        DV_Notifier update_cb = NULL,
        u_int archive_mask = 0);

    virtual EdtDataValue * AddParm(const string &name, 
        double *addr,
        DV_Notifier refresh_cb = NULL, 
        DV_Notifier update_cb = NULL,
        u_int archive_mask = 0);

    virtual EdtDataValue * AddParm(const string &name, 
        uint64_t *addr,
        DV_Notifier refresh_cb = NULL, 
        DV_Notifier update_cb = NULL,
        u_int archive_mask = 0);

    virtual EdtDataValue * AddParm(const string &name, 
        char *addr,
        int maxlen,
        DV_Notifier refresh_cb = NULL, 
        DV_Notifier update_cb = NULL,
        u_int archive_mask = 0);

    virtual EdtDataValue * AddParm(const string &name, 
        string *addr,
        DV_Notifier refresh_cb = NULL, 
        DV_Notifier update_cb = NULL,
        u_int archive_mask = 0);

    virtual EdtDataValue * AddParm(const string &name, 
        u_int *addr,
        EdtBitMaskTag *bits,
        DV_Notifier refresh_cb = NULL, 
        DV_Notifier update_cb = NULL,
        u_int archive_mask = 0);

        

    virtual ParmObject *GetChild(const size_t index);
    virtual ParmObject *LookupChild(const string &name);

    virtual void Name(const string &nm);
    virtual void Name(const char *nm);

    // set archive level for parm

    virtual void SetParmArchive(const string &nm, u_char level);


//    virtual int LogMessage(const char *format, ...);

    virtual const string & GetDelimiter();
    virtual void SetDelimiter(const string &d);

    virtual void SetFailNotFound(const bool state)
    {
        fail_not_found = state;
    }
    virtual bool FailNotFound()
    {
        return fail_not_found;
    }

    virtual EdtDataValue *  AddNewParm(const string &key, const string &value);

    virtual void SetAppendOk(const bool state)
    {
        append_ok = state;
    }
    virtual bool AppendOk()
    {
        return append_ok;
    }

    virtual void Destroy();

    virtual void SetOwnTree(const bool state)
    {
        owns_tree = state;
    }

    virtual bool OwnTree()
    {
        return owns_tree;
    }

    virtual int Set(ParmObject *source);

    virtual void SetupParms();

    virtual void PrintState(ostream &out);

    virtual int SetParameters(ParmItem *items);

    virtual int ParmNames(string &names);

};



#endif
