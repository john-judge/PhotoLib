#include "PdvConfigSet.h"

//#include "edtimage/StringUtils.h"

PdvConfigSet::PdvConfigSet()

{
    suffix = ".cfg";
    nfiles = 0;

    string dir;

#ifdef __linux__
    dir = "/opt/EDTpdv/camera_config/";
#elif defined(WIN32)
    dir = "c:/edt/pdv/camera_config/";
#else
    ///< . 
    dir = "/Applications/Edt/pdv/camera_config/";
#endif

    SetPath(dir);

}

PdvConfigSet::~PdvConfigSet()

{

}

int PdvConfigSet::LoadConfigFiles(const bool clink_only, const bool camclass_only, const char *dir)

{
    // check hdr path

    if (dir)
        SetPath(dir);

    DIRHANDLE d;

    size_t sufflen = 0;

    if (suffix.size())
        sufflen = suffix.size();

    d = edt_opendir(config_path.c_str());

    if (d)
    {

        char name[256];
           
        while (edt_readdir(d, name))
        {         
            if (strlen(name) > sufflen)
            {
                string sname;
                char * test = name + strlen(name)-sufflen;
                if ((sufflen && !strcmp(test,suffix.c_str())) || !sufflen)
                {
                    PdvConfig *cfg = new PdvConfig;
                    if (camclass_only)
                        cfg->ReadClassOnly(config_path,name);
                    else
                        cfg->Read(config_path,name);

                    bool useit = true;
                    if (clink_only && !cfg->IsClink())
                        useit = false;

                    if (useit)
                    {


                        PdvClassMap::iterator iter;
                        PdvConfigVec * subgroup;


                        iter = cfg_groups.find(cfg->GetCameraClass());

                        string ts = cfg->GetCameraClass();

                        if (iter == cfg_groups.end())
                        {
                            subgroup = new PdvConfigVec;
                            
                            cfg_groups[cfg->GetCameraClass()] = subgroup;
                        }
                        else
                        {
                            subgroup = iter->second;
                        }
                    
                        subgroup->push_back(cfg);
                        

                        nfiles ++;
                    }
                    else
                        delete cfg;
                }
            }
        }

        edt_closedir(d);

        return RET_SUCCESS;
    }
    else
        return ERROR_NO_FILE;

}

const string &PdvConfigSet::GetPath()
{
    return config_path;
}

void PdvConfigSet::SetPath(const string &path)

{
    config_path = path;
}

int PdvConfigSet::GetNFiles()

{
    return nfiles;
}

int PdvConfigSet::GetNGroups()

{
    return cfg_groups.size();
}

PdvCameraClass *PdvConfigSet::FirstCameraClass()

{
    curgroup = cfg_groups.begin();
    if (curgroup == cfg_groups.end())
        return NULL;

    current_class = *curgroup;

    return &current_class;
}

PdvCameraClass *PdvConfigSet::NextCameraClass()

{
    curgroup ++ ;

    if (curgroup != cfg_groups.end())
    {
        current_class = *curgroup;
        return &current_class;
    }
    else
        return NULL;
    
}
