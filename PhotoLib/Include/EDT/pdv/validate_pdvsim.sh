#!/bin/bash
#
# validate_pdvsim.sh
#
# validate the pdv framegrabber(s) and simulator(s) on the system. First evaluate which simulator
# units / channels are connected to which framegrabber units / channels (if none then exit with FAIL)
# Then test each one found using various tests.
#
# usage: validate_pdvsim.sh [options]
#  -f <fg unit> -s <sim unit> -- unit numbers (default: detects and loops through all valid pdv devices) 
#  -v, --verbose: verbose (list all missing files, not just the first 15)
#  -h, --help:    this help message
#
# prints PASS or FAIL on each test
#
# If all tests pass, exits with 0. If any test fails, exits with 1
#
DEVICE=PDV

TEMPDIR=.

LOG=$TEMPDIR/validate_pdvsim.log

if [ "$OS" == "Windows_NT" ] ; then
    TIMEOUT=/usr/bin/timeout
else
    TIMEOUT=timeout
fi

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

#
# echo PASS, and log and cleanup files
#
# @param $1 output file to cat to log then delete
# @param $2 file (usually image file) to delete 
#
# all params are optional
#
PASS()
{
    echoit "PASS"
    
    if  [ $? -gt 0 ] ; then
        cat $1 >> $LOG
        rm -f $1 >>$LOG 2>&1
    fi

    if  [ $? -gt 1 ] ; then
        rm -f $2 >>$LOG 2>&1
    fi
}

#
# echo FAIL followed by (optional) message, and log and cleanup files
#
# @param $1 message to print after FAIL
# @param $2 output file to cat to log then delete
# @param $3 file (usually image file) to delete 
#
# all params are optional
#
FAIL()
{
    echoit "FAIL"
    EXIT_STATUS=1

    # output optional message 
    if  [ $# -gt 0 ] ; then
        echoit_n "  "
        echoit "$1"
    fi

    # 2nd argument: file to log, then delete
    if  [ $# -gt 1 ] ; then
        cat $2 >> $LOG 2>&1
        rm -f $2 >/dev/null 2>&1
    fi

    # 3rd argument: file to delete (likely something.raw)
    if  [ $# -gt 2 ] ; then
        rm -f $3 >/dev/null 2>&1
    fi
}

stop_processes()
{
    iter=0

    echoit_n "stop any existing $1 processes... "
    while [ "`ps | grep -w $1 | grep -v grep | head -1 | wc -l`" != "0" ] ; do
        PROCESS_ID="`ps | grep -w $1 | grep -v grep | head -1 | awk -F' ' '{printf "%d",$1}'`"
        if [ "$PROCESS_ID" == "0" ] ; then
            break
        fi
        stop_process $PROCESS_ID

        # if more than 10 iterations, then fail since it's stuck or something
        ((++iter))
        if [ $iter -gt 9 ] ; then
            FAIL "too many $1 processes, or they refuse to die"
            exit 1
        fi 
    done

    echoit "PASS"

}

# get a numeric configuraton directive value
#
# @param $1 (DIRECTIVE) the config file directive, e.g. "height"
# @param $2 (CFG) the config file path
# @return the value of the directive
#
cfg_value()
{
    DIRECTIVE=$1
    CFG=$2
    VAL=(`grep "^ *${DIRECTIVE}:" ${CFG} | awk -F":" '{printf "%d",$2}'`)
    if  [ "$VAL" == "" ] ; then
        val=0
    fi

    echo $VAL
    return $VAL
}

#
# Check a config (width/vactv, height/hactv for off boundary typ. 16) condition
# return 
#  
# @param $1 (CONFIG) config file
# @param $2 (BOUNDARY) byte boundary to check against
# @return 0 for on-boundary, 1 for off-boundary
#
check_boundary()
{
    CONFIG=$1
    BOUNDARY=$2

    declare -i HACTV=$(cfg_value "hactv" $CONFIG)
    declare -i WIDTH=$(cfg_value "width" $CONFIG)
    declare -i VACTV=$(cfg_value "vactv" $CONFIG)
    declare -i HEIGHT=$(cfg_value "height" $CONFIG)
    declare -i DEPTH=$(cfg_value "depth" $CONFIG)

    if [ "$HACTV" != "" ] ; then HACTV=$WIDTH; fi
    if [ "$VACTV" != "" ] ; then VACTV=$HEIGHT; fi

    declare -i DEPTHBYTES=$((DEPTH/8))
    declare -i TOTALBYTES=$((WIDTH * HEIGHT * DEPTHBYTES))

    if [ `expr $TOTALBYTES % $BOUNDARY` -eq 0 ] ; then
        echo "0"
        return 0
    else
        echo "1"
        return 1
    fi
}

#
# check for at least one FG and one CLS device with pciload
#
check_devices()
{
    echoit_n "check for required simulator + framergrabber devices... "

    ./pciload 2>&1 | grep -vi cls |grep -vi fox |grep "pdv unit .*" >> $LOG 2>&1
    if [ $? -ne 0 ]; then
        FAIL "no framegrabbers found; this test requires one or more EDT PCI or PCIe framegrabbers"
        return $EXIT_STATUS
    else
        ./pciload 2>&1 | grep -i cls |grep "pdv unit .*" >> $LOG 2>&1
        if [ $? -ne 0 ]; then
            FAIL "no simulators found; this test requires one or more EDT PCI or PCIe CLS boards"
            return $EXIT_STATUS
        fi
    fi

    echoit "PASS"
    return 0
}



#
# initialize devices, use serial_cmd to  see which CLS channels are connected to which
# FG channels; least 2 are required for this test (typically sim_ch0->fg_ch0, sim_ch1->fg_ch1)
#
init_and_enumerate()
{

    # initialize all the framegrabbers first
    declare -i CONN

    CONN=0

    echoit_n "init / detect connected sim -> fg channels..."

    for SIMUNIT in `./pciload | grep -i cls |grep -vi fox |grep -i "pdv unit ." | head | cut -d" " -f 3` ; do

        if [ $? -eq 0 ] ; then

            SIMDESC=$(dev_description $SIMUNIT)

            for SIMCHAN in 0 1  ; do

                $TIMEOUT 10 clsiminit -u $SIMUNIT -c $SIMCHAN -f camera_config/generic24cl.cfg  >> $LOG 2>&1
                if [ $? -eq 0 ] ; then
                    ((SIMS_INITD++))
                    sleep 0.5
                else
                    FAIL "$TIMEOUT 10 clsiminit -u $SIMUNIT -c $SIMCHAN -f camera_config/generic24cl.cfg returned nonzero"
                    return $EXIT_STATUS
                fi

                for FGUNIT in `./pciload | grep -vi cls |grep -vi fox |grep -i "pdv unit ." | head | cut -d" " -f 3` ; do

                    FGDESC=$(dev_description $FGUNIT)

                    for FGCHAN in 0 1  ; do
                        $TIMEOUT 10 ./initcam -u $FGUNIT -c $FGCHAN -f camera_config/generic24cl.cfg >> $LOG 2>&1
                        if [ $? -eq 0 ] ; then
                            sleep 0.5
                        else
                            FAIL "$TIMEOUT 10 initcam -u $FGUNIT -c $FGCHAN -f camera_config/generic24cl.cfg returned nonzero"
                            return $EXIT_STATUS
                        fi

                        SENDTEXT="HELLO_FROM_SIM_${SIMUNIT}_${SIMCHAN}"
                        $TIMEOUT 10 ./serial_cmd -u $SIMUNIT -c $SIMCHAN -t 50 \"$SENDTEXT\" >>$LOG 2>&1

                        # echoit_n "framegrabber unit $FGUNIT channel $FGCHAN trying read... "
                        $TIMEOUT 10 ./serial_cmd -u $FGUNIT -c $FGCHAN -t 50 -p |grep -q $SENDTEXT >>$LOG 2>&1
                        if [ $? -eq 0 ] ; then
                            ((MAXCHAN++))
                            SIMUNITS[$MAXCHAN]="$SIMUNIT"
                            SIMCHANS[$MAXCHAN]="$SIMCHAN"
                            FGUNITS[$MAXCHAN]="$FGUNIT"
                            FGCHANS[$MAXCHAN]="$FGCHAN"
                            ((++CONN))
                        fi

                    done

                done

            done
        fi

    done

    if [ $SIMS_INITD -lt 2 ] ; then
        FAIL "$SIM_INITD simulator channels initialized, expected at least 2"
        return $EXIT_STATUS
    else
        echoit "got $CONN, PASS"
    fi

    return 0
}

dev_description()
{
    echo "`./pciload | grep -i \"pdv unit ${1}\" | sed 's/^.*(\(.*\)):.*$/\1/g'`"
}


getpid()
{
    if [[ $# < 1 || "$1" == "nothing" ]] ; then
        PROCESS_ID=0
    else
        PROCESS_ID="`ps | grep -w $1 | grep -v grep | head -1 | awk -F" " '{printf "%d",$1}'`"
    fi
    echo $PROCESS_ID
}

stop_process()
{
    if [ $1 -le 0 ] ; then
        return
    fi

    re='^[0-9]+$' # (before killing anything, make sure it's a pid)
    if  [[ $1 =~ $re ]] ; then
        kill -9 $1 >> $LOG 2>&1
        wait  $1 >> $LOG 2>&1
        sleep 1
    fi
}

#
# run  clink_tester on all detected simulator unit/channel
# -> framegrabber unit/channel combinations
#
# @return exit status (global)
#
#
do_clink_tester()
{
    # run clink_tester on all detected channels
    for idx in `seq 0 $MAXCHAN` ; do 

        SIMDESC=$(dev_description ${SIMUNITS[$idx]})
        FGDESC=$(dev_description ${FGUNITS[$idx]})

        echoit_n "test $SIMDESC ${SIMUNITS[$idx]}_${SIMCHANS[$idx]} -> $FGDESC ${FGUNITS[$idx]}_${FGCHANS[$idx]} with clink_tester... "

        clink_tester -u ${SIMUNITS[$idx]} -c ${SIMCHANS[$idx]} >> $LOG 2>&1 & 
        sleep 1
        BACKGROUND_PID=$(getpid "clink_tester")

        OUTPUT_CLINK_TESTER="${TEMPDIR}/validate_pdvsim_clink_tester${SIMUNITS[$idx]}${SIMCHANS[$idx]}${FGUNITS[$idx]}${FGCHANS[$idx]}.txt"

        $TIMEOUT 20 clink_tester -u ${FGUNITS[$idx]} -c ${FGCHANS[$idx]} -l 10 >> $OUTPUT_CLINK_TESTER 2>&1

        grep -qi "good card" $OUTPUT_CLINK_TESTER
        if  [ $? -ne 0 ] ; then
            FAIL "$TIMEOUT 60 clink_tester -u ${FGUNITS[$idx]} -c ${FGCHANS[$idx]} -l 10" $OUTPUT_CLINK_TESTER
            return
        fi

        PASS $OUTPUT_CLINK_TESTER

        # kill the clink_tester that's running on the sim
        stop_process $BACKGROUND_PID
        rm -f $OUTPUT_CLINK_TESTER
    done
}

#
# run clsiminit/initcam and simple_clsend/take on all detected
# simulator unit/channel -> framegrabber unit/channel combinations
#
# @param $1 = config_file
# @param $2 = imagelist or "gendata"
# @return exit status (global)
#
do_simtest()
{
    CONFIG="$1"
    IMAGELIST="$2"
    NUMARGS=$#
    BASE_CFG=`basename $CONFIG`
    BASEONLY=`basename $CONFIG | sed 's/\.cfg$//'`
    OFFBOUNDARY=$(check_boundary $CONFIG 16)

    if [ "$IMAGELIST" == "gendata" ] ; then
        CKSUM_LABEL="CLS_GENDATA_CKSUMS"
    else
        CKSUM_LABEL="CLS_IMGDATA_CKSUMS"
    fi

    for idx in `seq 0 $MAXCHAN` ; do 

        declare -i RET=0

        SIMDESC=$(dev_description ${SIMUNITS[$idx]})
        FGDESC=$(dev_description ${FGUNITS[$idx]})
        BACKGROUND_PID=$(getpid "nothing")
        OUTPUT_SIMTEST="${TEMPDIR}/validate_pdvsim_simtest${SIMUNITS[$idx]}${SIMCHANS[$idx]}${FGUNITS[$idx]}${FGCHANS[$idx]}.txt"
        IMAGEFILE="${TEMPDIR}/validate_pdvsim_simtest${SIMUNITS[$idx]}${SIMCHANS[$idx]}${FGUNITS[$idx]}${FGCHANS[$idx]}_${BASEONLY}.raw"
        rm -f $OUTPUT_SIMTEST $IMAGEFILE

        # simulator side: initialize with clsiminit, specify -s if "gendata"
        if [ "$IMAGELIST" == "gendata" ] ; then
            CLSIMINIT="clsiminit -s"
            echoit_n "clsiminit -s / take, $SIMDESC ${SIMUNITS[$idx]}_${SIMCHANS[$idx]} -> $FGDESC ${FGUNITS[$idx]}_${FGCHANS[$idx]}, ${BASE_CFG} $IMAGELIST... "
        else
            CLSIMINIT="clsiminit"
            echoit_n "simple_clsend / take, $SIMDESC ${SIMUNITS[$idx]}_${SIMCHANS[$idx]} -> $FGDESC ${FGUNITS[$idx]}_${FGCHANS[$idx]}, ${BASE_CFG} $IMAGELIST... "
        fi

        $CLSIMINIT -u ${SIMUNITS[$idx]} -c ${SIMCHANS[$idx]} -f $CONFIG >> $LOG 2>&1 & 
        if [ $? -ne 0 ] ; then
            FAIL "error on $CLSIMINIT -u ${SIMUNITS[$idx]} -c ${SIMCHANS[$idx]} -f $CONFIG"
            return
        fi

        # framegrabber side: initialize with initcam
        initcam -u ${FGUNITS[$idx]} -c ${FGCHANS[$idx]} -f $CONFIG >> $LOG 2>&1 & 
        if [ $? -ne 0 ] ; then
            FAIL "error on initcam -u ${FGUNITS[$idx]} -c ${FGCHANS[$idx]} -f $CONFIG"
            return
        fi

        # make sure inits have settled down
        sleep 0.5

        # simulator side: if clsimint wasn't set to generate data, then start simple_clsend, capture the PID so we can kill it later
        if [ "$IMAGELIST" != "gendata" ] ; then
            simple_clsend -u ${SIMUNITS[$idx]} -c ${SIMCHANS[$idx]} -l 0 -i $IMAGELIST >> $LOG 2>&1 & 
            sleep 0.5
            BACKGROUND_PID=$(getpid "simple_clsend")
            if [ "$BACKGROUND_PID" == "0" ] ; then
                FAIL "simple_clsend -u ${SIMUNITS[$idx]} -c ${SIMCHANS[$idx]} -l 0 -i $IMAGELIST bg process returned $BACKGROUND_PID" $OUTPUT_SIMTEST $IMAGEFILE
                return
            fi
        fi

        # if not off boundary, do 1 grab to a file and check the checksum
        if [[ "$OFFBOUNDARY" == "0" ]] ; then
            $TIMEOUT 5 take -u ${FGUNITS[$idx]} -c ${FGCHANS[$idx]} -f $IMAGEFILE >> $OUTPUT_SIMTEST 2>&1
            if [ $? -ne 0 ] ; then
                FAIL "error on take -u ${FGUNITS[$idx]} -c ${FGCHANS[$idx]} -f $IMAGEFILE" $OUTPUT_SIMTEST $IMAGEFILE
                stop_process $BACKGROUND_PID
                return
            fi

            GOT_CKSUM="`cksum $IMAGEFILE | awk -F" " '{printf "%d",$1}'`"

            # match checksum configs can have one or multiple checksums, e.g. clstest256.cfg
            grep "$CKSUM_LABEL" $CONFIG | grep -wq "$GOT_CKSUM"
            if [ $? -ne 0 ] ; then
                FAIL "error on take -u ${FGUNITS[$idx]} -c ${FGCHANS[$idx]} -f $IMAGEFILE: checksum <$GOT_CKSUM>" $OUTPUT_SIMTEST $IMAGEFILE
                stop_process $BACKGROUND_PID
                return
            fi

            grep -q " 0 timeouts" $OUTPUT_SIMTEST
            if [ $? -ne 0 ] ; then
                FAIL "take -l 1 -f $IMAGEFILE got timeouts or other error" $OUTPUT_SIMTEST $IMAGEFILE
                stop_process $BACKGROUND_PID
                return
            fi
        fi

        # framegrabber side: try 10 takes
        $TIMEOUT 10 take -u ${FGUNITS[$idx]} -c ${FGCHANS[$idx]} -l 10 >> $OUTPUT_SIMTEST 2>&1
        let RET=$?

        stop_process $BACKGROUND_PID

        # framegrabber side checking response:
        # * off-boundary test is only to make sure it doesn't freeze the thing so we
        #     only check for "10 images" and ignore any timeouts / exitval
        # * normal test, should see 0 timeouts& good (0) exitval 
        #
        if [[ $OFFBOUNDARY ]] ; then
            grep -q "10 images" $OUTPUT_SIMTEST
            if [ $? -ne 0 ] ; then
                FAIL "with $BASE_CFG, didn't see expected '10 images' from take -u ${FGUNITS[$idx]} -c ${FGCHANS[$idx]} -l 10"  $OUTPUT_SIMTEST $IMAGEFILE
                return
            fi
        else
            if [ $RET -ne 0 ] ; then
                FAIL "with $BASE_CFG, take -l 10 got timeouts or other error" $OUTPUT_SIMTEST $IMAGEFILE
                return
            fi

            grep -q " 0 timeouts" $OUTPUT_SIMTEST
            if [ $? -ne 0 ] ; then
                FAIL "with $BASE_CFG, take -l 10 got timeouts" $OUTPUT_SIMTEST $IMAGEFILE
                return
            fi
        fi

        PASS $OUTPUT_SIMTEST $IMAGEFILE

        rm -f $OUTPUT_SIMTEST $IMAGEFILE

    done

    return  
}

# =========================================================================
# MAIN script 
# =========================================================================

    # globals
    declare -i EXIT_STATUS=0
    declare -a SIMUNITS
    declare -a SIMCHANS
    declare -a FGUNITS
    declare -a FGCHANS
    declare -i MAXCHAN=-1
    declare -i SIMS_INITD=0

    echo logfile=$LOG
    rm -f $LOG

    if [ "$OS" == "Windows_NT" ] ; then
        if [ "$PDVHOME" == "" ] ; then
            PDVHOME=/cygdrive/c/EDT/pdv
        fi
    else
        if [ "$PDVHOME" == "" ] ; then
            PDVHOME=/opt/EDTpdv
        fi
    fi


    if [ $# -ne 0 ] ; then
        if [ "$1" == "--verbose" -o "$1" == "-v" ] ; then
            NLIST=99999
        elif [ "$1" == "--help" -o "$1" == "-h" -o "$1" == "-?" ] ; then
            echoit "Usage: $0 [OPTION]"
            echoit ""
            echoit "   -u, <unit>      unit to test (default is to loop through all valid pdv devs)"
            echoit "   -v, --verbose   verbose mode (list all missing files not just first 15)"
            echoit "   -h, --help      this help message"
            exit 0
        else
            FAIL "run $0 --help for usage"
            exit 1
        fi
    fi

    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PDVHOME
    export PATH=$PATH:$PDVHOME

    stop_processes "simple_clsend"
    check_devices
    init_and_enumerate

    if [ $MAXCHAN -lt 0 ] ; then
         FAIL "didn't find any SIM->FG channels connected"
    else
        do_clink_tester 
        do_simtest camera_config/clstest256.cfg imagelist.txt
        do_simtest camera_config/generic8cl.cfg gendata
        do_simtest camera_config/generic8x2cl.cfg gendata
        do_simtest camera_config/generic10cl.cfg gendata
        do_simtest camera_config/generic10x2cl.cfg gendata
        do_simtest camera_config/generic12cl.cfg gendata
        do_simtest camera_config/generic12x2cl.cfg gendata
        do_simtest camera_config/generic14cl.cfg gendata
        do_simtest camera_config/generic16cl.cfg gendata
        do_simtest camera_config/generic24cl.cfg gendata
# inconsistent result (rolling data for one thing, won't work on f1 for another);
# skip it for now but sometime look at 8x8 or 8x10 testing conditional on F4
#        do_simtest camera_config/baumerhxc13_1024_10x8f.cfg gendata
        do_simtest camera_config/edt_validate_658x501x10x1.cfg gendata
        do_simtest camera_config/edt_validate_658x501x10x2.cfg gendata
    fi

    exit $EXIT_STATUS
#end main


