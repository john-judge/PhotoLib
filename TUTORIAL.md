# User's Manual for pyPhoto21: Little Dave
Builds a camera/electrode management DLL to expose to (Python) applications. Includes a GUI application for acquisition and analysis, written in Python.

## Hardware Requirements
- RedshirtImaging's Little Dave (DaVinci 2K)
- NI-USB

## Software Requirements
- Install [NIDAQmx](https://www.ni.com/en-us/support/downloads/drivers/download.ni-daqmx.html#382067)
- Install [EDT's PDV driver](https://edt.com/updates/)

## Instructions
- For analysis-only mode, the executable can be launched from any location on your machine.

## Build From Source
This project is built with Visual Studio Code 2017 or 2019 with the Dynamic Loaded Library (DLL) project template.
- Target x64-Release
- Enable [/MT compiler option](https://docs.microsoft.com/en-us/cpp/build/reference/md-mt-ld-use-run-time-library?view=msvc-160)
- Copy Include Paths from [PhotoZ](https://github.com/john-judge/PhotoZ_upgrades.git) VS setup as needed
- Press F7 to compile and build the DLL
- Use the DLL as a library in any application needed. 
- Python: A Python wrapper module using the built-in `ctypes` library is included.

## Release Notes for Each Version
- Little Dave initial release (6.0.0)

## Future Features

