
#ifndef _PDVDEVICESET_H_
#define _PDVDEVICESET_H_

#include "edtinc.h"
#include "PdvDevice.h"


#include <string>
#include <vector>
#include <map>

using namespace std;

/**********************************************************************************************//**
 * @class   PdvDeviceSet
 *
 * @brief   A set of Pdv devices, sorted by Device Unit
 **************************************************************************************************/
typedef vector<PdvDevice *> PdvChannelVec;
typedef map<int, PdvChannelVec *> PdvUnitMap;
typedef pair<int, PdvChannelVec *> PdvDeviceUnit;

class PdvDeviceSet
{

private:
    
    PdvUnitMap dev_groups;


    PdvUnitMap::const_iterator curgroup;
    PdvDeviceUnit current_unit;

    int ndevs;

    PdvChannelVec alldevs;
    string edt_interface;

public:
    PdvDeviceSet(void);
    virtual ~PdvDeviceSet(void);

    int GetNGroups();
    int GetNDevices();

    PdvDeviceUnit *FirstDeviceUnit();
    PdvDeviceUnit *NextDeviceUnit();

    const string &GetInterface();
    void SetInterface(const string &intfc);

    int EnumerateDevices(const char *intfc = NULL);

};

#endif // _PDVDEVICESET_H_
