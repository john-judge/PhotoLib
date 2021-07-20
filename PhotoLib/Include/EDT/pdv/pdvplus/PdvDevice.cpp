#include "PdvDevice.h"

//#include "edtimage/StringUtils.h"

PdvDevice::PdvDevice(void)
{
}

PdvDevice::~PdvDevice(void)
{

}


const int &PdvDevice::GetDeviceUnit()
{
    return unit;
}

const int &PdvDevice::GetDeviceChannel()
{
    return channel;
}

void PdvDevice::SetDeviceUnit(const int &value)

{
    unit = value;
}

void PdvDevice::SetDeviceChannel(const int &value)

{
    channel = value;
}


const string &PdvDevice::GetDeviceInterface()
{
    return edt_device_interface;
}

void PdvDevice::SetDeviceInterface(const string &intfc)
{
    edt_device_interface = intfc;
}
