
#ifndef _PdvConfigSet_H_
#define _PdvConfigSet_H_

#include "edtinc.h"
#include "PdvConfig.h"



#include <string>
#include <vector>
#include <map>

using namespace std;

/**********************************************************************************************//**
 * @class   PdvConfig
 *
 * @brief   A set of Pdv configurations, sorted by Camera Class (vendor) 
 **************************************************************************************************/
typedef vector<PdvConfig *> PdvConfigVec;
typedef map<string, PdvConfigVec *> PdvClassMap;
typedef pair<string, PdvConfigVec *> PdvCameraClass;

class PdvConfigSet
{

private:
    
    PdvClassMap cfg_groups;

    string config_path;
    string suffix;

    PdvClassMap::const_iterator curgroup;
    PdvCameraClass current_class;

    int nfiles;

    PdvConfigVec allfiles;

public:
    PdvConfigSet(void);
    virtual ~PdvConfigSet(void);

    const string &GetPath();
    void SetPath(const string &path);

    int GetNGroups();
    int GetNFiles();

    PdvCameraClass *FirstCameraClass();
    PdvCameraClass *NextCameraClass();

    int LoadConfigFiles(const bool clink_only, const bool camclass_only = true, const char *dir = NULL);


};

#endif
