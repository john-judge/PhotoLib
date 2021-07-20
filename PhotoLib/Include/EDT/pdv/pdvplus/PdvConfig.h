
#ifndef _PdvConfig_H_
#define _PdvConfig_H_

//#include "edtimage\edtimagedata.h"



#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <map>

using namespace std;


/**********************************************************************************************//**
 * @class   PdvConfig
 *
 * @brief   A single Pdv configuration. 
 **************************************************************************************************/

class PdvConfig 
{

private:
    
    map<string, string> lines;

    vector<string> comments;

    string config_path;
    string config_name;

    string null_string;

    string camera_class;
    string camera_model;
    string camera_info;

    int width, height, depth;

    bool class_only;

    bool is_clink;

public:
    PdvConfig(void);
    virtual ~PdvConfig(void);

    const string &GetElement(const string &name);
    void SetElement(const string &name, const string &value);

    void Read(const string &path, const string &name);
    void Read(const bool camclass_only);
    void ReadClassOnly(const string &path, const string &name);
    void Write(const string &path, const string &name);
    void Write();

    const string &GetPath();
    void SetPath(const string &path);

    const string &GetName();
    void SetName(const string &path);

    void AddComment(const string &comment, const string &key, int position);
  
    void RemoveElement(const string &name);

    int GetNValues();
    int GetNLines();

    const string &GetCameraClass();
    void SetCameraClass(const string &value);

    const string &GetCameraInfo();
    void SetCameraInfo(const string &value);

    const string &GetCameraModel();
    void SetCameraModel(const string &value);

    int GetWidth();
    void SetWidth(const int value);

    int GetHeight();
    void SetHeight(const int value);

    int GetDepth();
    void SetDepth(const int value);

    void Clear();

    bool IsClink();

};

#endif
