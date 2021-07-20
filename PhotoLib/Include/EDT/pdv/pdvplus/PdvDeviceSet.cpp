#include "PdvDeviceSet.h"

//#include "edtimage/StringUtils.h"

PdvDeviceSet::PdvDeviceSet()

{
    ndevs = 0;

    string dir;

    SetInterface(EDT_INTERFACE);
}

PdvDeviceSet::~PdvDeviceSet()

{

}

int PdvDeviceSet::EnumerateDevices(const char *intfc)
{
    PdvDev *pdv_p;

    if (intfc)
        SetInterface(intfc);

    char name[256];
           
    for (int u=0; u<32; u++)
    {
        for (int c = 0; c<2; c++)
        {
            if ((pdv_p = pdv_open_device(GetInterface().c_str(), u, c, 0)) != NULL)
            {         
                // filter for camera link or simulator or ??

                PdvDevice *dev = new PdvDevice;

                dev->SetDeviceUnit(u);
                dev->SetDeviceChannel(c);

                PdvUnitMap::iterator iter;
                PdvChannelVec * subgroup;

                iter = dev_groups.find(dev->GetDeviceUnit());

                if (iter == dev_groups.end())
                {
                    subgroup = new PdvChannelVec;
                    
                    dev_groups[dev->GetDeviceUnit()] = subgroup;
                }
                else
                {
                    subgroup = iter->second;
                }
            
                subgroup->push_back(dev);

                ndevs ++;
            }
        }
    }
    return RET_SUCCESS;
}

const string &PdvDeviceSet::GetInterface()
{
    return edt_interface;
}

void PdvDeviceSet::SetInterface(const string &intfc)
{
    edt_interface = intfc;
}


int PdvDeviceSet::GetNDevices()

{
    return ndevs;
}


int PdvDeviceSet::GetNGroups()

{
    return dev_groups.size();
}


PdvDeviceUnit *PdvDeviceSet::FirstDeviceUnit()

{
    curgroup = dev_groups.begin();
    if (curgroup == dev_groups.end())
        return NULL;

    current_unit = *curgroup;

    return &current_unit;
}

PdvDeviceUnit *PdvDeviceSet::NextDeviceUnit()

{
    curgroup ++ ;

    if (curgroup != dev_groups.end())
    {
        current_unit = *curgroup;
        return &current_unit;
    }
    else
        return NULL;
    
}
