#!/bin/bash
#
# validate_pdvinst.sh
#
# validation test for pdv package insallation. Checks for the presence of, and runs
# typical files and applications to cover the normal operaitons expected after inst-
# allation of an EDT pdv package.
# Upon installation, gets renamed to validate_install.sh to provide a consistent
# validation method with pcd which has a similar script that also gets renamed.
#
# usage: validate_install.sh [options] (on windows with cygwin: sh validate_install.sh)
#  -v, --verbose: verbose (list all missing files, not just the first 15)
#  -h, --help:    this help message
#
# prints PASS or FAIL on each test
#
# If all tests pass, exits with 0. If any test fails, exits with 1
#

DEVICE=PDV


TEMPDIR=.
LOG=$TEMPDIR/validate_pdvinst.log

COMMON_FILES='
    bitload.c
    cl_speed.c
    clink_tester.c
    clsim_lib.c
    clsim_lib.h
    clsiminit.c
    clsimvar.c
    countbits.c
    dvinfo.c
    edt_bitload.c
    edt_bitload.h
    edt_devid.h
    edt_error.c
    edt_error.h
    edt_flash.c
    edt_jpeg.c
    edt_jpeg.h
    edt_sysinfo.c
    edt_threads.h
    edt_types.h
    edt_utils.h
    edtdef.h
    edtinc.h
    edtreg.h
    edtregbits.h
    gen_tiffs.c
    imagelist.txt
    includes.mk
    initcam.c
    initcam.h
    irigdump.c
    libdvu.c
    libdvu.h
    libedt.c
    libedt.h
    libpdv.c
    libpdv.h
    makefile
    makefile.def
    pciload.h
    pciload_fn.c
    pciload_main.c
    pdb.c
    pdv_bayer_filter.c
    pdv_dependent.h
    pdv_initcam.c
    pdv_interlace.c
    pdv_interlace.h
    pdv_interlace_methods.h
    pdvterm.c
    rcxload.c
    readcfg.c
    send_tiffs.c
    serial_cmd.c
    setdebug.c
    simple_clsend.c
    simple_take.c
    simplest_take.c
    take.c
    dispatch/ErrorHandler.cpp
    dispatch/ErrorHandler.h
    dispatch/ParmObject.cpp
    dispatch/ParmObject.h
    dispatch/makefile
    edt_camlink/edt_camlink.cpp
    edt_camlink/edt_camlink.h
    edt_camlink/makefile
    edtimage/ColorBalance.cpp
    edtimage/ColorBalance.h
    edtimage/makefile
    imgfiles/imagefile.cpp
    imgfiles/imagefile.h
    imgfiles/makefile
    pciediag_src/makefile
    pciediag_src/pciecfg.cpp
    pciediag_src/pciecfg.h
    pciediag_src/pciediag.cpp
    pdvplus/ImageEventFunc.h
    pdvplus/PdvInput.cpp
    pdvplus/PdvInput.h
    pdvplus/makefile
    tiffs256/num0.tif
    tiffs256/num1.tif
    tiffs256/num2.tif
    camera_config/Lil-Joe-1024CL.cfg
    camera_config/adimec1000m-10cl_dual.cfg
    camera_config/basler104k_8d.cfg
    camera_config/cameratest.cfg
    camera_config/clstest256.cfg
    camera_config/cohu7700_10f.cfg
    camera_config/dalsa4m30_8et.cfg
    camera_config/e2v_aviivam4_2k_8cs.cfg
    camera_config/edt_validate_658x501x10x1.cfg
    camera_config/f12drum.cfg
    camera_config/generic8.cfg
    camera_config/hama_orcaflash4.0_5x16f.cfg
    camera_config/hc7942-12_2x2bin.cfg
    camera_config/hkpf100-10-8.cfg
    camera_config/icc_mbc100-12.cfg
    camera_config/illunis2000-10-cd.cfg
    camera_config/imb2040_12f.cfg
    camera_config/jai_ptm6700cl.cfg
    camera_config/km14-8.cfg
    camera_config/mikrotron1362.cfg
    camera_config/nar12cl.cfg
    camera_config/ptm1010-10.cfg
    camera_config/redlake_es40_12c.cfg
    camera_config/sbf.cfg
    camera_config/su320m17r5-12c.cfg
    camera_config/tosh_cs3960dcl_10f.cfg
    camera_config/up610-10.cfg
    camera_config/va8m16-mf10_1t12f.cfg
    '


BITFILES='
    camera_config/bitfiles/dv/4005/aiaext.bit
    camera_config/bitfiles/dv/4010/aiag.bit
    camera_config/bitfiles/dv/4013/aiaext.bit
    camera_config/bitfiles/dv/4013/aiag.bit
    camera_config/bitfiles/dva/xc2s100e/aiag.bit
    camera_config/bitfiles/dva/xc2s100e/aiag24.bit
    camera_config/bitfiles/dva/xc2s600e/aiag.bit
    camera_config/bitfiles/dvcl2/xc2s400e/clsim.bit
    camera_config/bitfiles/dvfox/xc2s400e/aiag4.bit
    camera_config/bitfiles/dvk/4005/aiaext.bit
    camera_config/bitfiles/dvk/4005/aiag.bit
    camera_config/bitfiles/dvk/4010/aiag.bit
    camera_config/bitfiles/dvk/4013/aiag.bit'

FLASH='
    flash/4013e/pcidv.bit
    flash/4013e/pdvk.bit
    flash/4013xla/pcidv_3v.bit
    flash/4013xla/pcidv_5v.bit
    flash/4013xla/pdvk_3v.bit
    flash/4013xla/pdvkkla_3v.bit
    flash/4028xla/pcidv_3v.bit
    flash/5cgtfd5c5/visionlinkf1.bit
    flash/5cgtfd5c5/visionlinkf4.bit
    flash/rcx/fx12.rcx
    flash/rcx/fyz12.rcx
    flash/rcx/my12.rcx
    flash/rcx/rgb08.rcx
    flash/rcx/rgb11d.rcx
    flash/xc2s100/fciusps01_3v.bit
    flash/xc2s150/pdvcamlk_3v.bit
    flash/xc2s200/camlkla_5v.bit
    flash/xc2s200/dva1_3v.bit
    flash/xc2s200/dva4_3v.bit
    flash/xc2s200/dvcl4_3v.bit
    flash/xc2s200/dvtlk1_5v.bit
    flash/xc2s200/dvtlk4_pmcdvf_3v.bit
    flash/xc2s200/pdvcamlk2_3v.bit
    flash/xc2s200/pdvcamlk_5v.bit
    flash/xc3s1200e/pedvcamlk.bit
    flash/xc3s1200e/pedvclsim.bit
    flash/xc5vlx30t/pe4dvacamlk.bit
    flash/xc5vlx30t/pe4dvacamlk_fm.bit
    flash/xc5vlx30t/pe8dvacamlk.bit
    flash/xc5vlx30t/pe8dvacamlk_fm.bit
    flash/xc5vlx30t/pe8dvaclsim.bit
    flash/xc5vlx30t/pe8dvcamlk.bit
    flash/xc5vlx30t/pe8dvcamlk2.bit
    flash/xc5vlx30t/pe8dvclsim.bit
    flash/xc5vlx30t/pe8dvclsim_fm.bit'

# linux look in homedir/fname
# windows look in homedir/fname.exe
WINDOWS_APPS='
    bitload.exe
    clink_tester.exe
    clsiminit.exe
    clsimvar.exe
    countbits.exe
    dumpit.exe
    dvinfo.exe
    edt_sysinfo.exe
    gen_tiffs.exe
    initcam.exe
    irigdump.exe
    msp430_load.exe
    new_strobe.exe
    pciediag.exe
    pciload.exe
    pdb.exe
    pdvterm.exe
    rcxload.exe
    send_tiffs.exe
    serial_cmd.exe
    setdebug.exe
    simple_clsend.exe
    simple_event.exe
    simple_take.exe
    simplest_take.exe
    speedtest.exe
    take.exe
    trace.exe
    watchstat.exe'

WINDOWS_FILES='
    CHANGELOG_PDV.txt
    EDTtermsOfUse.txt
    README.pdv
    README.pdv.utils
    README.send_tiffs
    README64.utils
    version
    bin32.bat
    bin64.bat
    pdvcom.exe
    load_arch.bat
    win_sysinfo.c
    edt_os_nt.c
    edt_os_nt.h
    pdvcom.cpp
    win_sysinfo.exe
    clseredt.dll
    pdvlib.lib
    clseredt.dll
    pdvlib.dll
    Uninstall_pdv.exe
    wdf/install/EDTpdv_install_silent.bat
    wdf/install/EDTpdv_uninstall_silent.bat
    wdf/reload32.bat
    wdf/reload64.bat
    dispatch/dispatch.vcproj
    projects.vs2008/PdvPlus.vcproj
    projects.vs2008/bitload.vcproj
    projects.vs2008/bitload.vcproj.user
    projects.vs2008/simple_take.vcproj
    projects.vs2008/simple_take.vcproj.user
    projects.vs2008/take.vcproj
    projects.vs2008/take.vcproj.user
    images/edticon64.ico
    images/edtinstall.ico
    images/installsplash.bmp
    images/welcomesplash.bmp
    jpeg-6b/README
    jpeg-6b/ansi2knr.c
    jpeg-6b/cjpeg.c
    jpeg-6b/djpeg.c
    jpeg-6b/makefile
    jpeg-6b/jpeg-6b.vcproj
    redist/vcredist_x64.exe
    redist/vcredist_x86.exe
    wdf/wdf_reload.exe
    wdf/wdf_reload64.exe
    wdf/install/amd64/WdfCoInstaller01009.dll
    wdf/install/amd64/pdvwdf.cat
    wdf/install/amd64/pdvwdf.inf
    wdf/install/amd64/pdvwdf.sys
    wdf/install/win10_amd64/WdfCoInstaller01009.dll
    wdf/install/win10_amd64/pdvwdf.cat
    wdf/install/win10_amd64/pdvwdf.inf
    wdf/install/win10_amd64/pdvwdf.sys
    wdf/install/x86/WdfCoInstaller01009.dll
    wdf/install/x86/pdvwdf.cat
    wdf/install/x86/pdvwdf.inf
    wdf/install/x86/pdvwdf.sys
    wdf/install/win10_x86/WdfCoInstaller01009.dll
    wdf/install/win10_x86/pdvwdf.cat
    wdf/install/win10_x86/pdvwdf.inf
    wdf/install/win10_x86/pdvwdf.sys
    lib/amd64/clseredt.lib
    lib/amd64/imgfiles.lib
    lib/amd64/pdvlib.lib
    lib/amd64/pdvplus.lib
    lib/x86/clseredt.lib
    lib/x86/edtimage.lib
    lib/x86/libtiff.lib
    lib/x86/pdvplus.lib'

TIFF_FILES='
    tiff/libtiff/tif_close.c
    tiff/libtiff/tif_codec.c
    tiff/libtiff/tif_color.c
    tiff/libtiff/tif_compress.c
    tiff/libtiff/tif_config.h
    tiff/libtiff/tif_dir.c
    tiff/libtiff/zlib.h
    '

TIFF403_FILES='
    tiff-4.0.3/libtool
    tiff-4.0.3/config/install-sh
    tiff-4.0.3/build/Makefile
    tiff-4.0.3/port/getopt.c
    tiff-4.0.3/configure
    tiff-4.0.3/tools/ras2tiff
    tiff-4.0.3/tools/tiff2rgba.c
    tiff-4.0.3/README
    tiff-4.0.3/config.log
    tiff-4.0.3/html/intro.html
    tiff-4.0.3/Makefile
    tiff-4.0.3/contrib/ras/Makefile
    tiff-4.0.3/contrib/ras/tif2ras.c
    tiff-4.0.3/libtiff/.libs/libtiff.a
    tiff-4.0.3/libtiff/tif_jpeg.c
    tiff-4.0.3/Makefile.in
    tiff-4.0.3/config.status
    '

LINUX_FILES='
    libpdv.a
    module/driverlib.mk
    module/driver_rules.mk
    module/edtdrv_3.o_shipped
    module/edtdrv_4.o_shipped
    module/edtdrvx86_64_3.o_shipped
    module/edtdrvx86_64_4.o_shipped
    module/edt.ko
    module/edt_lnx_kernel.c
    module/edt_lnx_kernel.h
    module/edt_lnx_mm.c
    module/edt_mem_mapping.h
    module/edt_mem_tag.c
    module/edt_mem_tag.h
    module/edt.mod.c
    module/lib_edtdrv_3.a
    module/lib_edtdrv_4.a
    module/lib_edtdrvx86_64_3.a
    module/lib_edtdrvx86_64_4.a
    module/Makefile
    module/makefile.drv
    module/make.output
    module/README.linux64
    module/rules.mk
    /etc/ld.so.conf.d/EDTpdv-x86_64.conf'

FILES="$FILES $COMMON_FILES $BITFILES $FLASH"

LINUX_APPS="`echo $WINDOWS_APPS | sed 's/\.exe//g'`"

CENTOS_5=false

if [ "$OS" == "Windows_NT" ] ; then
    FILES="$FILES $WINDOWS_FILES $WINDOWS_APPS $TIFF_FILES"
else
    FILES="$FILES $LINUX_FILES $LINUX_APPS $TIFF403_FILES"

    lsb_release -a |grep -q -i "centos release 5"
    if [ $? == 0 ] ; then
        CENTOS_5=true
    fi

fi

# check these?
#CopyFiles C:\EDT\pdv\bin\amd64\clseredt.dll C:\Program Files\CameraLink\Serial
#CopyFiles C:\EDT\pdv\bin\x86\clseredt.dll C:\Windows\system32
#Create shortcut: C:\Users\Public\Desktop\Pdv Utilities.lnk

# this is some goofy stuff that may be needed to create the logfile on cygwin
fn_initemptyfile()
{
    rm -f $1 || return 1

    if [ "$OS" == "Windows_NT" ] ; then
        TEMPFILE=EdtCygwinTempFile.tmp
        touch $TEMPFILE
        install --mode=644 -T $TEMPFILE $1 || return 1
        rm -f $TEMPFILE
    else
        touch $1 || return 1
    fi

    return 0
}

echoit()
{
    echo "$1"
    echo "$1" >> $LOG
}

echoit_n()
{
    echo -n "$1"
    echo "$1" >> $LOG
}

fn_find_lspci()
{
  lspci=`which lspci 2>/dev/null`
  if [ "$lspci" == "" ] ; then
    if [ -x /usr/bin/lspci ]; then
      lspci=/usr/bin/lspci
    elif [ -x /usr/sbin/lspci ]; then 
      lspci=/usr/sbin/lspci
    elif [ -x /bin/lspci ]; then
      lspci=/bin/lspci
    elif [ -x /sbin/lspci ]; then
      lspci=/sbin/lspci
    else
      return 1

    fi
  fi

  return 0
}


fn_validate_files()
{
    RET=0
    num_errors=0

    for filename in $FILES ; do
      DIR=`echo $filename | cut -d"/" -f1`
      DIR2=`echo $filename | cut -d"/" -f1-2`
      gzfilename=$filename;
      if [ "$DIR" == "bitfiles" -o "$DIR" == "flash" -o "$DIR2" == "camera_config/bitfiles" ] ; then
          gzfilename=`echo $filename | sed 's/\.bit$/.bit.gz/'`
      fi

      if [ ! -f $filename -a ! -f $gzfilename ] ; then
        if [ $RET -eq 0 ] ; then
            echoit "FAIL"
            RET=1
        fi
        if [ $num_errors -lt $NLIST ]; then
          if [ "$DIR" == "bitfiles" -o "$DIR" == "flash" ] ; then
              echoit "  missing file $filename or $gzfilename"
          else 
              echoit "  missing file $filename"
          fi
        fi

        num_errors=$((num_errors + 1))
      fi
    done

    if [ $num_errors -ge $NLIST ]; then
        echoit "  (skipping remaining $((num_errors - $NLIST)) missing files, run with -v or --verbose"
        echoit "  to see all the missing files)"
    fi

    return $RET
}

fn_validate_version()
{
    EXIT_STATUS=0

    echoit_n "Checking version numbers... "

    CHANGELOG=CHANGELOG_${DEVICE}.txt
    if [ ! -f version ] ; then
        echoit "FAIL"
        echoit "  version file not found"
        EXIT_STATUS=1
    elif [ ! -f $CHANGELOG ] ; then
        echoit "FAIL"
        echoit "  $CHANGELOG not found"
        EXIT_STATUS=1
    else
        VERSION=$(head -1 version)
        LIB_VERSION=$(./setdebug -v 2>&1 |grep -i "Library version" | sed 's/.*ibrary version \([0-9,\.]*\).*/\1/' )
        LOG_VERSION=$(grep "V\.[\t ][0-9]\.[0-9]\.[0-9]" CHANGELOG_PDV.txt |head -1 |sed 's/\t/ /' | cut -d" " -f2)
        DRV_VERSION=$(./setdebug -v 2>&1 |grep -i "Driver version"  | sed 's/.*river version \([0-9,\.]*\).*/\1/' )
        DEVICE_NOT_PRESENT=$(./setdebug -v 2>&1 | grep -qi "pdv open failed")
        XV=$(grep EDTAPI_VERSION libedt.h | awk {'print $3'} | sed 's/^0x//')
        API_VERSION=$((16#$(echo $XV|cut -c1-2))).$((16#$(echo $XV|cut -c3-4))).$((16#$(echo $XV|cut -c5-6))).$((16#$(echo $XV|cut -c7-8)))

        # if no PDV in the system, skip the driver version check by just setting it same as version file
        if [[ $DEVICE_NOT_PRESENT ]] ; then
            DRV_VERSION=$VERSION
        fi

        if [[ $LOG_VERSION != $VERSION || $LIB_VERSION != $VERSION || $DRV_VERSION != $VERSION || $API_VERSION != $VERSION ]] ; then
            echoit "FAIL"
            if [[ $DEVICE_NOT_PRESENT ]] ; then
                DRV_VERSION="n/a, no pdv found"
            fi
            echoit "  Driver ($DRV_VERSION), lib ($LIB_VERSION), libedt.h ($API_VERSION [0x$XV]) or 1st ver. in changelog ($LOG_VERSION) != version file ($VERSION)"
            EXIT_STATUS=1
        fi

    fi
    return $EXIT_STATUS
}

fn_validate_nodes()   # NOTE: Linux only
{
    echoit_n "Checking device nodes, lspci and pciload... "

    local numnodes=`ls -1 /dev/edt[0-9] 2>&1 | grep "edt[0-9]" | wc -l`

    fn_find_lspci
    if (( $? == 0 )) ; then
      local numlspcis=`$lspci 2>&1 |grep "Engineering Design" |wc -l`
      if (( $numnodes != $numlspcis )) ; then
        echoit "FAIL"
        echoit "  Number of EDT devs from by lspci ($num) doesn\'t match the number of /dev/edtN nodes ($numnodes)"
        EXIT_STATUS=1
      fi
    else
      echoit "FAIL"
      echoit "  lspci not found"
      EXIT_STATUS=1
      return $EXIT_STATUS
    fi

    declare -a A1
    declare -a A2

    A1=($(lspci 2>&1 |grep "Engineering Design Team" | sed 's/^.*Device 0*\([0-9,a-z]*\).*/\1/' |sort -n))
    A2=($(./pciload 2>&1 |grep "^ *ID 0x" | sed 's/.*ID 0x0*\([0-9,a-z]*\),.*/\1/' | sort -n))

    A1_SIZE=${#A1[@]}
    A2_SIZE=${#A2[@]}

    if (( A1_SIZE != A2_SIZE )) ; then
        echo "FAIL"
        echo "  Number of boards from lspci (${A1_SIZE}) not the same as from pciload (${A2_SIZE}),"
        echo "  check for errors in edtinit and edt_find_boards (edt_lnx_kernel.c)"
        EXIT_STATUS=1
    fi

    for (( i=0; i<${A1_SIZE}; i++ )); do

        if [ ${A1[$i]} != ${A2[$i]} ] ; then
            echo "FAIL"
            echo "  Device ID mismatch between lspci and pciload, check for errors in"
            echo "  edtinit and edt_find_boards (edt_lnx_kernel.c)"
            EXIT_STATUS=1
        fi
    done

    return $EXIT_STATUS
}

# =============================================================================
# MAIN script 
# =============================================================================

# create the log file, work around cygwin permission problems
fn_initemptyfile $LOG
if [ $? -ne 0 ] ; then
  if [ "$OS" == "Windows_NT" ] ; then
    echo "Permission problem on cygwin; typical workaround is to edit /etc/fstab and"
    echo "add 'noacl,' after 'binary,' then open a new terminal window"
  else
    echo "Need to run $0 as root, or fix permissions e.g. sudo chmod -R <user> $PDVHOME"
  fi
  exit 1;
fi

uname -a >> $LOG

touch ./validate_pdvinst_test_file_write.tmp
if [ $? -ne 0 ] ; then
   echo "Can't run script due to permissions (current dir not writeable by you)."
   exit 1
fi
rm -f ./validate_pdvinst_test_file_write.tmp

if [ "$OS" == "Windows_NT" ] ; then
    which nmake 2>&1 | grep -q -i "no nmake" >>$LOG 2>&1
    if [ $? -eq 0 ] ; then
        MAKE=""
    echoit "No nmake on this system (or not in path), will skip various builds and post-rebuild checks"
    else
        MAKE=nmake
    fi
else
    MAKE=make
fi

if [ "$OS" != "Windows_NT" ] ; then
  fn_validate_nodes
  if [ $? -ne 0 ] ; then
      EXIT_STATUS=1
  else
      echoit "PASS"
  fi
fi

if [ "$PDVHOME" == "" ] ; then
    PDVHOME=/opt/EDTpdv
fi

EXIT_STATUS=0

NLIST=15
if [ $# -ne 0 ] ; then
    if [ "$1" == "--verbose" -o "$1" == "-v" ] ; then
        NLIST=99999
    elif [ "$1" == "--help" -o "$1" == "-h" -o "$1" == "-?" ] ; then
        echoit "Usage: $0 [OPTION]"
        echo ""
        echo "   -v, --verbose   verbose mode (list all missing files not just first 15)"
        echo "   -h, --help      this help message"
        exit 0
    else
        echoit "FAIL: run $0 --help for usage"
        exit 0
    fi
fi

export PATH=$PATH:$PDVHOME

echoit_n "Checking for errors in installation log... "
INSTALL_LOG=install_pdv.log
if [ ! -f $INSTALL_LOG ] ; then
    echoit "FAIL"
    echoit "  $INSTALL_LOG not found"
    EXIT_STATUS=1
else
    grep "but with errors" $INSTALL_LOG >>$LOG 2>&1
    if [ $? -eq 0 ]; then
        echoit "FAIL"
        echoit "  errors found in $INSTALL_LOG"
        EXIT_STATUS=1
    else
        echoit "PASS"
    fi
fi

echoit_n "Running ./pciload  with no arguments & checking return value... "
./pciload >>$LOG 2>&1
RET=$?
if [ $RET -ne 0 ]; then
    echoit "FAIL"
    echoit "  pciload returned $RET"
    EXIT_STATUS=1
else
    echoit "PASS"
fi

fn_validate_version
if [ $? -ne 0 ] ; then
    EXIT_STATUS=1
else
    echoit "PASS"
fi


echoit_n "Checking for expected files... "
fn_validate_files
if [ $? -ne 0 ] ; then
    EXIT_STATUS=1
else
    echoit "PASS"
fi

# device validate has been pulled out to a separate script so it can be
# run independently if desired
./validate_pdvdev.sh || EXIT_STATUS=1

cat validate_pdvdev.log >> $LOG

if [ "$MAKE" != "" ] ; then
    echoit_n "Rebuilding all... "
    $MAKE clean >> $LOG 2>&1 

    $MAKE all >> $LOG 2>&1 
    MAKEALL_STATUS=$?

    # for some reason libdispatch doesn\'t get built as part of all;
    # should fix but in any case this works around that
    if [ "$OS" != "Windows_NT" ] ; then
      $MAKE libdispatch.so >> $LOG 2>&1 
      MAKEDISP_STATUS=$?
    fi

    if [ "$OS" == "Windows_NT" ] ; then
      $MAKE clseredt >> $LOG 2>&1
      MAKECLSER_STATUS=$?
    fi

    if [[ $MAKEALL_STATUS -ne 0 || $MAKEDISP_STATUS -ne 0 ]] ; then
        echoit "FAIL"
        echoit "  see $LOG for details"
        EXIT_STATUS=1
    else
        echoit "PASS"
    fi

    ./validate_pdvdev.sh --post_rebuild || EXIT_STATUS=1
    cat validate_pdvdev.log >> $LOG

    echoit_n "Checking for expected files (post-rebuild)... "
    fn_validate_files
    if [ $? -ne 0 ] ; then
        EXIT_STATUS=1
    else
        echoit "PASS"
    fi
fi

# On Windows, we couldn't do lspci / node check above, so here at the end we just report & ask verification
if [ "$OS" == "Windows_NT" ] ; then
  numpcilds=`./pciload 2>&1 |grep "^p[cd][dv] unit" |wc -l`
  if (( $numpcilds > 0 )) ; then 
    echo "Checking for installed boards... "
    ./pciload 2>&1 | grep "p[cd][dv] unit" | sed -e 's/^p/  p/' -e 's/: *//'
    echo "Verify that this is what's in your system -- if not, diagnose and report."
  else
    echo "Checking for installed boards... no EDT boards were found."
    echo "Verify that no EDT boards are present -- if there are, diagnose and report."
  fi
fi

(( $EXIT_STATUS == 0 )) || echo "Include the logfile $LOG when reporting errors"

exit $EXIT_STATUS
