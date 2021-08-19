# PhotoLib
Builds a camera/electrode management DLL to expose to (Python) applications. Includes a GUI application for acquisition and analysis, written in Python.

## Building Executable
Uses `pyinstaller`. Navigate to the `PhotoLib` directory.

### One-File Mode
Not recommended because start-up is slow, but if you want a single exe for analysis only and portability between folders, shared drivers, machine, etc. For one-file mode:
```
pyinstaller -F -n pyPhoto21 --add-data nicaiu.dll;. --add-data clseredt.dll;. driver.py --distpath Photo21
```

This will take serveral minutes. 

### Building for Rig Computer (One-Folder Mode)
Or in one-folder mode (faster startup, but files are messy):
```
pyinstaller -n pyPhoto21 --add-data nicaiu.dll;. --add-data clseredt.dll;. --add-data ./x64/Release/PhotoLib.dll;./x64/Release/ driver.py --distpath .
```

Avoid using the `--clean --noconfirm` options.

This will take serveral minutes. 

Create a shortcut to the executable in the current directory. Place the shortcut anywhere you like, but do not move any files in the folder in which pyinstaller was run, and do not change the location of the folder either.

### Distributing This Application

If single-file mode, the .exe is ~400 MB, so it will have to be distributed via shared network, Google Drive, USB/external drive, or building locally on destination machine.
If single-folder mode, right-click > Send To > Compressed File to zip before distributing.

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
dumpbin /DEPENDENTS .\x64\Release\PhotoLib.dll
```
Typically:
```
  Image has the following dependencies:

    clseredt.dll
    nicaiu.dll
    KERNEL32.dll
    VCOMP140.DLL
```
Find these dependencies and make sure their directories are included in your system environment's PATH variable or add them to one of the DLL search paths.

## Architecture
### Overview
![modules (1)](https://user-images.githubusercontent.com/40705003/129975800-95b877ed-b8da-46f5-83bb-48e716169ebb.png)


## Related Projects and Applications
- [PhotoZ](https://github.com/john-judge/PhotoZ_upgrades.git) for data acquisition and analysis with GUI entirely in C++
- [ZDA_Explorer](https://github.com/john-judge/ZDA_Explorer.git) for flexible data analysis scripting with PhotoZ raw data
- [PhotoZ_Images](https://github.com/john-judge/PhotoZ_Image.git) for camera image display
- [Cell Detection](https://github.com/ksscheuer/ROI_Identification.git) Kate's SNR clustering method for identifying ROIs
