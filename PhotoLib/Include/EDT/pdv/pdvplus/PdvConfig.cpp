#include "PdvConfig.h"

//#include "edtimage/StringUtils.h"

PdvConfig::PdvConfig(void)
{
    is_clink = false;
}

PdvConfig::~PdvConfig(void)
{
}

const string &PdvConfig::GetElement(const string &name)

{
    map<string, string>::const_iterator iter;  
    
    iter = lines.find(name);

    if (iter==lines.end())
    {
        return null_string;
    }
    else
    {
        return iter->second;
    }
}

void PdvConfig::Clear()

{
    lines.clear();
    comments.clear();
}

char *strim(char *cp)

{
    int start=0;
    int len=strlen(cp);

    if (len == 0)
        return cp;

    while (cp[start] && isspace(cp[start]))
        start++;

    len--;
    while (len>start && isspace(cp[len]))
        len--;
    len++;

    cp[len] = 0;

    return cp+start;
    
        
}

char *dequote(char *cp)

{
    int start=0;
    int len=strlen(cp);

    if (len == 0)
        return cp;

    while (cp[start] && cp[start] == '"')
    {
        start++;
    }

    while (len>start && cp[len-1] == '"')
        len--;
    
    cp[len] = 0;

    return cp+start;
    
        
}
void PdvConfig::Read(const string &path, const string &fname)

{

    SetPath(path);
    SetName(fname);

    Read(false);
}

void PdvConfig::Read(const bool camclass_only)

{
    int maxcount = 255;
    char buf[256];

    Clear();

    string nm = config_path + config_name;

    FILE *f = fopen(nm.c_str(), "r");

    if (!f)
        return;

    while (fgets(buf, maxcount, f) != NULL)
    {

        int i = 0;
        char *kp;

        while (buf[i] && (buf[i] == '\t' || buf[i] == ' ' || buf[i] == '\n') && i <= maxcount)
            i++;

        kp=buf+i;

        if (!camclass_only)
            comments.push_back(buf);

        if (*kp == '#')
        {
            
        }
        else
        {
            char *colon = strchr(kp,':');

            if (colon)
            {
                *colon = 0;
                colon++;

                kp = strim(kp);
                colon = strim(colon);
            
                string key = kp;

                if (camclass_only)
                {
                    if (key == "camera_class")
                    {
                        camera_class = dequote(colon);
                        
                    }
                    else if (key == "CL_DATA_PATH_NORM")
                        is_clink = true;
                    else if (key == "CL_CFG_NORM")
                        is_clink = true;

                }
                else
                {
                    string value = dequote(colon);

     
                    if (key == "width")
                        SetWidth(atoi(value.c_str()));
                    else if (key == "height")
                        SetHeight(atoi(value.c_str()));
                    else if (key == "depth")
                        SetDepth(atoi(value.c_str()));
                    else if (key == "camera_class")
                        camera_class = value;
                    else if (key == "camera_info")
                        camera_info = value;
                    else if (key == "camera_model")
                        camera_model = value;
                    else if (key == "CL_DATA_PATH_NORM")
                        is_clink = true;
                    else if (key == "CL_CFG_NORM")
                        is_clink = true;

                    lines[key] = value;

                }

            }
        }        
    }

    fclose(f);

    class_only = camclass_only;

}

void PdvConfig::ReadClassOnly(const string &path, const string &fname)

{

    SetPath(path);
    SetName(fname);

    Read(true);
}

void PdvConfig::Write(const string &path, const string &fname)

{
    SetPath(path);
    SetName(fname);
    Write();
}

void PdvConfig::Write()

{

}

const string &PdvConfig::GetPath()
{
    return config_path;
}

void PdvConfig::SetPath(const string &path)

{
    config_path = path;
}

const string &PdvConfig::GetName()
{
    return config_name;
}

void PdvConfig::SetName(const string &path)

{
    config_name = path;
}

void PdvConfig::SetElement(const string &name, const string &value)

{
    lines[name] = value;


}
void PdvConfig::AddComment(const string &comment, const string &key, int position)

{
    
}

void PdvConfig::RemoveElement(const string &name)

{

}

int PdvConfig::GetNValues()

{
    if (class_only)
        Read(false);

    return lines.size();
}

int PdvConfig::GetNLines()

{
    return lines.size() + comments.size();
}

const string &PdvConfig::GetCameraClass()
{
    return camera_class;
}

void PdvConfig::SetCameraClass(const string &value)

{
    camera_class = value;
}

const string &PdvConfig::GetCameraInfo()
{
    if (class_only)
        Read(false);
    return camera_info;
}

void PdvConfig::SetCameraInfo(const string &value)

{
    camera_info = value;
}

const string &PdvConfig::GetCameraModel()
{
    if (class_only)
        Read(false);
    return camera_model;
}

void PdvConfig::SetCameraModel(const string &value)

{
    camera_model = value;
}

int PdvConfig::GetWidth()
{
    if (class_only)
        Read(false);
    return width;
}

void PdvConfig::SetWidth(const int value)

{
    width = value;
}

int PdvConfig::GetHeight()
{
    if (class_only)
        Read(false);
    return height;
}

void PdvConfig::SetHeight(const int value)

{
    height = value;
}

int PdvConfig::GetDepth()
{
    if (class_only)
        Read(false);
    return depth;
}

void PdvConfig::SetDepth(const int value)

{
    depth = value;
}

bool PdvConfig::IsClink()

{
    return is_clink;
}

