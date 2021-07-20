# Quick Start: vlviewer

## Overview

The EDT VisionLink Viewer, `vlviewer`, application provides the following features:

1. Viewing and inspecting an image stream from an EDT Frame Grabber (VisionLink F1/F4).
2. Saving images to the local filesystem.

> **IMPORTANT**: vlviewer will not work if the EDT PDV driver is not installed on the target
machine.

### Supported platforms
Only Windows and Linux (Ubuntu and CentOS) are supported.

Windows:
- Supports x86/x64 architecture

Linux:
- Supports both x86/x64 architecture & arm64 architecture.

## Issues/Feature Requests

Any issues with the application or desired feature request can be submitted to the public
vlviewer gitlab repository issues page:
[https://gitlab.com/engineering-design-team/vlviewer/-/issues](https://gitlab.com/engineering-design-team/vlviewer/-/issues)

A gitlab.com account is required to submit issues.

---

## Application Status
**PLEASE READ THIS SECTION**

### Issue: Camera Bit Depth
This vlviewer application supports the following bit depths: 8, 16, 24

The product road map for this application includes features to add support for cameras with
bit depths of 10, 12, 14, etc.

For status updates on this feature see the gitlab issue:
[https://gitlab.com/engineering-design-team/vlviewer/-/issues/1](https://gitlab.com/engineering-design-team/vlviewer/-/issues/1)

#### Workaround
The PDV driver package no longer contains the legacy vlviewer code. If legacy use is 
desired an archive is available from the vlviewer public repo: 
[https://gitlab.com/engineering-design-team/vlviewer](https://gitlab.com/engineering-design-team/vlviewer).

**IMPORTANT**: Before installing the legacy `vlviewer`, please make sure to
uninstall the newer `vlviewer` application from the target system.

- [Linux Uninstall Instructions](#uninstalling)
- Linux tar ball: [https://gitlab.com/engineering-design-team/vlviewer/-/blob/master/legacy/vlviewer-linux.tgz](https://gitlab.com/engineering-design-team/vlviewer/-/blob/master/legacy/vlviewer-linux.tgz)
    - The Linux tar ball contains docs with information on how to build the application from source.

- [Windows Uninstall Instructions](#uninstalling-manually)
- Windows zip file: 
    - 64 bit: [https://gitlab.com/engineering-design-team/vlviewer/-/blob/master/legacy/vlviewer-windows_x64.zip](https://gitlab.com/engineering-design-team/vlviewer/-/blob/master/legacy/vlviewer-windows_x64.zip)
    - 32 bit: [https://gitlab.com/engineering-design-team/vlviewer/-/blob/master/legacy/vlviewer-windows_x86.zip](https://gitlab.com/engineering-design-team/vlviewer/-/blob/master/legacy/vlviewer-windows_x86.zip)
        - **IMPORTANT**: The windows zip file contains documentation on how to install the application globally. See the `README.legacy_vlviewer` for instructions on how to setup.

---

## Windows Application

### Installation

> Default installation path of PDV driver is: `C:\EDT\pdv`

By default, the Windows PDV driver will install `vlviewer` automatically. In addition, when
the PDV driver is removed, `vlviewer` will be removed as well.

> If manual control of installation and un-installation is needed, please see the following sections.

### Installing Manually

The `vlviewer-installer.exe` executable can be found in the directory where the PDV driver is installed.
In order to install `vlviewer` run `vlviewer-installer.exe` and follow the prompts of the installer.

### Uninstalling Manually

A uninstaller is registered with the system when `vlviewer-installer.exe` is run, and can be uninstalled
manually by following these steps:
1. Go to "Settings"
2. Go to "Apps" > "Apps & Features"
3. Search/select the `vlviewer` entry and then click the "Uninstall" button
4. Follow the uninstaller prompts

### Windows Application Usage

From the start menu, either search for `vlviewer` or scroll to the "V" section and select the
application to launch it.

> Pressing the `Windows Key` and searching for "vlviewer" will also launch the application.

---

## Linux Application

### Installation

> Default installation path of PDV driver is: `/opt/EDTpdv`

The Linux PDV driver bundles a tar (.tgz) archive of:
- The `vlviewer` binary for both arm64 & x86_64.
- Optional setup scripts to register/un-register the application globally.
- Optional assets needed by setup scripts.

Once the PDV driver is installed and unpacked to the desired directory, run the following commands
(which assume the PDV driver and files are in `/opt/EDTpdv`):

```
/home/user0$ cd /opt/EDTpdv

/opt/EDTpdv$ mkdir vlviewer

/opt/EDTpdv$ tar -C ./vlviewer -xvf vlv-linux-<version>.tgz

# The following command are optional:
# Note: register-globally.sh defaults referencing the directory /opt/EDTpdv/vlviewer/

/opt/EDTpdv$ pushd vlviewer && sudo ./register-globally.sh $(pwd)/bin/<cpu arch>/vlviewer && popd
Copying files to /opt/EDTpdv/vlviewer...
Generating Desktop Entry: /usr/share/applications/vlviewer.desktop
To make vlviewer available globally on login, add the following to your .bashrc or .profile
  export PATH=$PATH:/opt/EDTpdv/vlviewer/bin/<cpu arch>

/opt/EDTpdv$ echo "export PATH=$PATH:/opt/EDTpdv/vlviewer/bin/<cpu arch>" >> $HOME/.bashrc

/opt/EDTpdv$ source $HOME/.bashrc
```

> **IMPORTANT**: `register-globally.sh` can also work on non-default paths by passing a directory as
the first argument (ex. `./register-globally.sh /<path>/<to>/<xyz>/vlviewer`). If this is the case, then
the path `/opt/EDTpdv` in the prompts above should be replaced with desired installation path.

### Uninstalling

> **IMPORTANT**: If `vlviewer` was added to the $PATH environment variable, then it will need to be removed manually.

If `register-globally.sh` **WAS** used to register the program in the target system, run the following:
```
/home/user0$ cd <path>/<to>/vlviewer
<path>/<to>/vlviewer $ sudo ./unregister-globally.sh
<path>/<to>/vlviewer $ cd ..
<path>/<to>/EDTpdv $ sudo rm -rf vlviewer
```

If `register-globally.sh` **WAS NOT** used, then run the following:
```
/home/user0$ sudo rm -rf <path>/<to>/vlviewer
```

### Linux Application Usage

#### Terminal/Command Line

Running `vlviewer -h` will print the the following usage message:
```
Usage of vlviewer:
  -b int
        Number of buffers to use. Omit from command line to use defaults. (default -1)
  -c int
        VisionLink Channel. Defaults to using GUI to select unit. (default -1)
  -f string
        EDT .cfg file. Found in <EDT pdv installation path>/camera_config/ .
  -u int
        VisionLink Unit. Defaults to using GUI to select unit. (default -1) 
```
> If none of the flags are specified, then the application will walk through selecting the 
appropriate unit/channel/configuration and use a default number of buffers to setup the
application on startup.

In order to bypass the setup steps in the GUI, the `-u`, `-c`, and `-f` flags 
**MUST** all be passed like so: `vlviewer -u <unit> -c <channel> -f </path/to/config.cfg>`. If one of the
flags is missing, the setup steps will be shown in order to make sure the
application is configured correctly before being used.

#### Application Launcher

Running `vlviewer` from the system application launcher will only work if the `register-globally.sh` is used.

The application will be available via the global search by typing `vlviewer`, 
or in the applications menu under `Graphics`, `Utilities`, or `Audio & Video`.
