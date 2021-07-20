#!/bin/bash 
#
# validate_pdvdev.sh
#
# validate a pdv installation using whatever pdv framegrabbers are in the system, running initcam
# and various applications on channel 2 (fg simulator). 
#
# usage: validate_pdvdev.sh [options]
#  -u <unit> -- unit number (default: detects and loops through all valid pdv devices) 
#  -v, --verbose: verbose (list all missing files, not just the first 15)
#  -h, --help:    this help message
#
# prints PASS or FAIL on each test
#
# If all tests pass, exits with 0. If any test fails, exits with 1
#
DEVICE=PDV

TEMPDIR=.

POSTREBUILD=""

CENTOS_5=false

if [ "$OS" != "Windows_NT" ] ; then
    lsb_release -a |grep -q -i "centos release 5"
    if [ $? == 0 ] ; then
        CENTOS_5=true
    fi
fi


LOG=$TEMPDIR/validate_pdvdev.log

echoit()
{
    echo " $1"
    echo "$1" >> $LOG
}

echoit_n()
{
    echo -n "$1"
    echo "$1" >> $LOG
}

fn_validate_fw()
{
    OUTPUT_PCILOADV=${TEMPDIR}/validate_pdvdev_$1_pciload.txt

    echoit_n "Found pdv unit $1 ($2), verifying FW with pciload verify${POSTREBUILD}... "

    echo "./pciload -u $1 verify" >>$LOG
    ./pciload -u $1 verify >$OUTPUT_PCILOADV 2>&1
    RET=$?

    if [ $RET -ne 0 ] ; then
        echoit "WARNING"
        echoit "  $2 (unit $1) probably needs a firmware update"
    else
        echoit "PASS"
    fi

    cat $OUTPUT_PCILOADV >> $LOG
    rm -f $OUTPUT_PCILOADV

# not failing on this one
#    return $RET
    return 0
}

fn_validate_sim()
{
    SIM_UNIT=$1
    FG_UNIT=$2
}

# check a device / applications with the given unit: initcam, take, simple_take, countbits
#
fn_validate_ch2()
{

    TEMPDIR=.
    RET=0
    FGUNIT=$1
    FGDEV=$2
    CONFIG=camera_config/generic24cl.cfg

    OUTPUT_SIMPLETAKE=${TEMPDIR}/validate_pdvdev_${FGUNIT}_simpletake.txt
    OUTPUT_TAKE=${TEMPDIR}/validate_pdvdev_${FGUNIT}_take.txt
    OUTPUT_INITCAM=${TEMPDIR}/validate_pdvdev_${FGUNIT}_initcam.txt
    OUTPUT_TAKERAW=${TEMPDIR}/validate_pdvdev_${FGUNIT}_takeraw.txt
    IMAGE_TAKERAW=${TEMPDIR}/validate_pdvdev_${FGUNIT}_takeraw.raw

    rm -f $OUTPUT_SIMPLETAKE $OUTPUT_TAKE $OUTPUT_INITCAM $OUTPUT_TAKERAW $IMAGE_TAKERAW $IMAGE_VLVRAW
    if [ $? -ne 0 ] ; then
        echoit "ERROR removing temporary files ; check permissions!"
        exit 1
    fi

    echoit_n "Found pdv unit $FGUNIT ($FGDEV), validating I/O with ch. 2 simulator, 24-bits${POSTREBUILD}... "

    if [ "$OS" == "Windows_NT" ] ; then
        TIMEOUT=/usr/bin/timeout
    else
        TIMEOUT=timeout
    fi

    # initialize with intam, check exit value
    echo "$TIMEOUT 10 ./initcam -u $FGUNIT -c 2 -f $CONFIG" >> $LOG
    $TIMEOUT 10 ./initcam -u $FGUNIT -c 2 -f $CONFIG > $OUTPUT_INITCAM 2>&1
    RET_INITCAM=$?
    if [ $RET_INITCAM -ne 0 ]; then
        RET=1
    fi

    sleep 0.25

    # run take, check exit value and timeouts
    
    echo "$TIMEOUT 5 ./take -J -c 2 -u $FGUNIT -l 10" >> $LOG
    $TIMEOUT 5 ./take -J -c 2 -u $FGUNIT -l 10 > $OUTPUT_TAKE 2>&1

    RET_TAKE=$?
    if [ $RET_TAKE -ne 0 ]; then
        RET=1
    else
        grep -q " 0 timeouts" $OUTPUT_TAKE
        RET_TAKE=$?
        if [ $RET_TAKE -ne 0 ]; then
            RET=1
        fi
    fi

    # run simple take, check exit value and timeouts
    $TIMEOUT 5 ./simple_take -u $FGUNIT -c 2 -l 10 > $OUTPUT_SIMPLETAKE 2>&1
    RET_SIMPLETAKE=$?
    if [ $RET_SIMPLETAKE -ne 0 ]; then
        RET=1
    else
        grep -q " 0 timeouts" $OUTPUT_SIMPLETAKE
        RET_SIMPLETAKE=$?
        if [ $RET_SIMPLETAKE -ne 0 ]; then
            RET=1
        fi
    fi

    EXPECTED_CKSUMS=`grep "CH2_GENDATA_CKSUMS" $CONFIG | sed 's/.*CH2_GENDATA_CKSUMS *//g' |tr -d '\r\n'`

    # grab a raw image with take, check the exit value and image checksum
    TAKERAW_SUMERROR=0
    TAKERAW_MSG="none"
    $TIMEOUT 5 ./take -J -c 2 -u $FGUNIT -f $IMAGE_TAKERAW > $OUTPUT_TAKERAW 2>&1
    RET_TAKERAW=$?
    GOT_CKSUM="`cksum $IMAGE_TAKERAW | cut -d' ' -f1`"
    if [ "$RET_TAKERAW" != "0" ]; then
        TAKERAW_MSG="take -J -c 2 -u $FGUNIT -f $IMAGE_TAKERAW returned $RET_TAKERAW"
        RET=1
    else
        grep "CH2_GENDATA_CKSUMS" $CONFIG | grep -wq "$GOT_CKSUM"
        if [ $? -ne 0 ] ; then
            TAKERAW_SIMERROR=1
            TAKERAW_MSG="take -J -c 2 -u $FGUNIT -f $IMAGE_TAKERAW, got <$GOT_CKSUM>, expected <$EXPECTED_CKSUMS>"
        fi
    fi

    if [ $TAKERAW_SUMERROR -ne 0 ] ; then
        RET=1
    fi

    # output any failure mode messages
    if [ $RET -ne 0 ] ; then
        echoit "FAIL" 
        if [ $RET_INITCAM -ne 0 ] ; then
            cat $OUTPUT_INITCAM
        fi
        if [ $RET_TAKE -ne 0 ] ; then
            cat $OUTPUT_TAKE
        fi
        if [ $RET_SIMPLETAKE -ne 0 ] ; then
            cat $OUTPUT_SIMPLETAKE
        fi
        if [ $RET_TAKERAW -ne 0 ] ; then
            cat $OUTPUT_TAKERAW
        fi
        if [ $TAKERAW_SUMERROR -ne 0 ] ; then
            echoit "$TAKERAW_MSG"
        fi
        if [ $RET_VLVRAW -ne 0 ] ; then
            echoit "  error running ./$VLVIEWER -u $FGUNIT -c 2 -f $IMAGE_VLVRAW -i ./${CONFIG} -q"
        fi
        if [ "$VLVRAW_MSG" != "none" ] ; then
            echoit "$VLVRAW_MSG"
        fi
    else
        echoit "PASS"
    fi

    cat $OUTPUT_SIMPLETAKE $OUTPUT_TAKE $OUTPUT_INITCAM $OUTPUT_TAKERAW >>$LOG
    rm -f $OUTPUT_SIMPLETAKE $OUTPUT_TAKE $OUTPUT_INITCAM $OUTPUT_TAKERAW $IMAGE_TAKERAW $IMAGE_VLVRAW

    return $RET
}

# =========================================================================
# MAIN script 
# =========================================================================

rm -f $LOG

if [ "$PDVHOME" == "" ] ; then
    PDVHOME=/opt/EDTpdv
fi

EXIT_STATUS=0
NLIST=15
UNIT=-1

if [ $# -ne 0 ] ; then
    if [ "$1" == "--verbose" -o "$1" == "-v" ] ; then
        NLIST=99999
    elif  [ "$1" == "-u" ] ; then
        UNIT=$2
        NUMBERS='^[0-9]+$'
        if ! [[ $UNIT =~ $NUMBERS ]] ; then
            echoit "-u option missing required numeric argument; run $0 --help for usage"
            exit 1
        fi
    elif [ "$1" == "--post_rebuild" -o "$1" == "-p" ] ; then
            POSTREBUILD=" (post-rebuild)"
    elif [ "$1" == "--help" -o "$1" == "-h" -o "$1" == "-?" ] ; then
        echoit "Usage: $0 [OPTION]"
        echoit ""
        echoit "   -u, <unit>      unit to test (default is to loop through all valid pdv devs)"
        echoit "   -v, --verbose   verbose mode (list all missing files not just first 15)"
        echoit "   -h, --help      this help message"
        exit 0
    else
        echoit "FAIL: run $0 --help for usage"
        exit 0
    fi
fi

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PDVHOME
export PATH=$PATH:$PDVHOME

if [ "$OS" == "Windows_NT" ] ; then
    which nmake 2>&1 | grep -q -i "no nmake" >>$LOG 2>&1
    if [ $? -eq 0 ] ; then
        MAKE=""
	echoit "No nmake on this system (or not in path), will skip various builds and post-rebuild checks"
    else
        MAKE=nmake
    fi
else
    xhost local:root >/dev/null 2>&1
    MAKE=make
fi

OUTPUT_MAKE=$TMPDIR/validate_pdvdev_make.txt

if [ $UNIT -ge 0 ] ; then
    DEVNAME="`./pciload | grep -i \"pdv unit $UNIT\" | sed 's/^.*(\(.*\)):.*$/\1/g'`"
    echo $DEVNAME | grep -qwiv -E "cls|fox"
    if [ $? -eq 0 ] ; then
        echoit "pdv unit $UNIT looks like a simulator or FOX not a framegrabber${POSTREBUILD}... FAIL"
        exit 1
    elif [ "$DEVNAME" == "" ] ; then
        echoit "pdv unit unit $UNIT not found${POSTREBUILD}... FAIL"
        exit 1
    fi

    # if PCIe4/8 or VisionLink, validate using channel 2 simulator
    if ! ./pciload -u $UNIT | grep -q "pci dv" ; then
      fn_validate_ch2 $UNIT "$DEVNAME" || EXIT_STATUS=1
    fi
else
    # any pdv board, validate FW (except VisionLink CLS then possibly validate ch2 
    for UNIT in `./pciload | grep "pdv unit" | grep -viE "visionlink cls" | cut -d" " -f 3` ; do
        DEVNAME="`./pciload | grep -i \"pdv unit $UNIT\" | sed 's/^.*(\(.*\)):.*$/\1/g'`"

        fn_validate_fw $UNIT "$DEVNAME" || EXIT_STATUS=1

        sleep 0.2 # not sure if we need this...

        # if c-link or VisionLink framegrabber
        if ./pciload -u $UNIT | grep -i -E "pdv unit.*c-link|pdv unit.*visionlink" |grep -vi "visionlink CLS" 2>&1 >> $LOG; then
          fn_validate_ch2 $UNIT "$DEVNAME" || EXIT_STATUS=1
        fi
    done
fi


exit $EXIT_STATUS

