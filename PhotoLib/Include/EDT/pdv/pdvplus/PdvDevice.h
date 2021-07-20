
#ifndef _PDVDEVICE_H_
#define _PDVDEVICE_H_

//#include "edtimage\edtimagedata.h"


#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <map>

using namespace std;


/**********************************************************************************************//**
 * @class   PdvDevice
 *
 * @brief   A single Pdv device/channel. 
 **************************************************************************************************/

class PdvDevice 
{

private:
    
  //  vector<string> devinfo;

    int unit;
    int channel;
    string edt_device_interface;

public:
    PdvDevice(void);
    virtual ~PdvDevice(void);

    void RemoveElement(const string &name);

    const int &GetDeviceUnit();
    void SetDeviceUnit(const int &value);
    void SetDeviceInterface(const string &intfc);
    const string &GetDeviceInterface();

    const int &GetDeviceChannel();
    void SetDeviceChannel(const int &value);

};

#endif //_PDVDEVICE_H_
