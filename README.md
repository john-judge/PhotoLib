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
- Follow the build from source guide below. Or, when/if I've released a stable, standalone version of `PhotoLib.dll`, use that directly (dependencies should come with this repository).

## Build From Source
This project is built with Visual Studio Code 2017 or 2019 with the Dynamic Loaded Library (DLL) project template.
- Target x64-Release
- Enable [/MT compiler option](https://docs.microsoft.com/en-us/cpp/build/reference/md-mt-ld-use-run-time-library?view=msvc-160)
- Copy Include Paths from [PhotoZ](https://github.com/john-judge/PhotoZ_upgrades.git) VS setup as needed
- Press F7 to compile and build the DLL
- Use the DLL as a library in any application needed. 
- Python: A Python wrapper module using the built-in `ctypes` library is included.

## Python GUI
Install the conda environment from `environment.yml`:
```
conda env create -f environment.yml
```
Then run `python driver.py`

Development: if additional conda dependencies are added to the environment, update the  `environment.yml` with:
```
conda env export > environment.yml
```


## Troubleshooting
- [Microsoft DUMPBIN tool](https://docs.microsoft.com/en-us/cpp/build/reference/dependents?view=msvc-160) – A tool to find DLL dependents.
From VS Terminal (Developer Powershell) run:
```
dumpbin /DEPENDENTS .\x64\Debug\PhotoLib.dll
```

## Related Projects and Applications
- [PhotoZ](https://github.com/john-judge/PhotoZ_upgrades.git) for data acquisition and analysis with GUI entirely in C++
- [ZDA_Explorer](https://github.com/john-judge/ZDA_Explorer.git) for flexible data analysis scripting with PhotoZ raw data
- [PhotoZ_Images](https://github.com/john-judge/PhotoZ_Image.git) for camera image display
- [Cell Detection](https://github.com/ksscheuer/ROI_Identification.git) Kate's SNR clustering method for identifying ROIs
