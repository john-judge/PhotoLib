# PhotoLib
Builds a camera/electrode management DLL to expose to Python applications. 

## Hardware Requirements
- DLL targets 64-bit machines
- RedshirtImaging's Little Dave (DaVinci 2K)
- NI-USB

## Software Requirements
- Install [NIDAQmx](https://www.ni.com/en-us/support/downloads/drivers/download.ni-daqmx.html#382067)
- Install [EDT's PDV driver](https://edt.com/updates/)

## Instructions

- Clone repository. 
- Follow the build from source guide below, or simply use the most recent stable release of `PhotoLib.dll`

## Build From Source
This project is built with Visual Studio Code 2017 or 2019 with the Dynamic Loaded Library (DLL) project template.
- Press F7 to compile and build the DLL
- Use the DLL as a library in any application needed. API guide below.
- Python: A Python wrapper module using the built-in `ctypes` library is included.

## Related Projects and Applications
- [PhotoZ](https://github.com/john-judge/PhotoZ_upgrades.git) for data acquisition and analysis with GUI entirely in C++
- [ZDA_Explorer](https://github.com/john-judge/ZDA_Explorer.git) for flexible data analysis scripting with PhotoZ raw data
- [PhotoZ_Images](https://github.com/john-judge/PhotoZ_Image.git) for camera image display
- [Cell Detection](https://github.com/ksscheuer/ROI_Identification.git) Kate's SNR clustering method for identifying ROIs
