/*
 * pciediag: Diagnostic utility for EDT, Inc. PCI Express devices
 *
 * Displays configuration space information regarding negotiated
 * express #lanes, max payload size, maximum link speed, etc. for
 * installed EDT, Inc. PCI Express devices.
 *
 * Copyright (c) 2015, Engineering Design Team, Inc.
 */
#include "edtinc.h"
#include "pciecfg.h"
#include <stdio.h>
#include <iostream>
#include <bitset>
#include <sstream>

using namespace std;

static void usage(char *progname, const char *errmsg)
{
    printf("%s", errmsg);
    printf("Usage: %s [options]\n", progname);
    printf("  -u unit           - device number (default 0)\n");
    printf("                      A full device pathname can also be used e.g. pcd0\n");
    exit(1);
}


int main(int argc, char **argv)
{
    edt_buf buf;
    u_int capId = 0;
    int unit = 0; //default dev unit
    char *unitstr = NULL;
    char dev[EDT_STRBUF_SIZE];
    char *progname = argv[0];
    int ret;

    strcpy(dev, EDT_INTERFACE);
   
    --argc;
    ++argv;
    while (argc && ((argv[0][0] == '-') || (argv[0][0] == '/')))
    {
        switch (argv[0][1])
        {
            case 'u':		/* device unit number */
                ++argv;
                --argc;
                if (argc < 1) 
                    usage(progname, "Error: option 'u' requires a numeric argument\n");

                unitstr = argv[0];
                break;

            case 'h':
            default:
                usage(progname, "");
        }

    }

    if (unitstr && *unitstr)
        unit = edt_parse_unit(unitstr, dev, NULL);

    EdtDev *edt_p;

    if((edt_p = edt_open_channel(dev, unit, 0)) == NULL)
    {
    	printf("Failed to open channel");
    	return 0;
    }

    //start with address 0 and check the cap ID
    //if cap ID == 0x10 then we are at the 
    //capability register (where we want to be)
    u_int addr = 0x0;

    buf.desc = addr;
    ret = edt_ioctl(edt_p, EDTG_CONFIG, &buf);

    u_int devId = (buf.value >> 16);
    u_int venId = (buf.value & 0x0000ffff);

    //create configuration object
    pcieCfg *cfgObj = new pcieCfg();
    cfgObj->set_ids(devId, venId);

    addr = 0x4;
    buf.desc = addr;
    ret = edt_ioctl(edt_p, EDTG_CONFIG, &buf);
    u_int mbit = (buf.value & 0x0002) >> 1;
    cfgObj->set_masterbit(mbit);

    //get the pointer to the capabilities register
    addr = 0x34;
    buf.desc = addr;
    edt_ioctl(edt_p, EDTG_CONFIG, &buf);
    u_int cap_ptr = buf.value;
    u_int prev_addr;

    do
    {
	buf.desc = cap_ptr;
        prev_addr = cap_ptr;
        edt_ioctl(edt_p, EDTG_CONFIG, &buf);
        capId = (buf.value & 0x000000ff);
    	cap_ptr = ((buf.value & 0x0000ffff) >> 8);


    } while(capId != cfgObj->get_capId() && cap_ptr != 0);

    if(capId != 0x10)
    {
    // filter out known devids that aren't PCIe
        if ((edt_p->devid < PE8DVFOX_ID)
         && (edt_p->devid !=  PE4CDA_ID)
         && (edt_p->devid != PE4CDA16_ID)
         && (edt_p->devid != PE4CDALL_ID))
            cout << "Not a PCIe device (devid 0x" << std::hex << edt_p->devid << ")" << endl;
        else cout << "No PCI Express configuration register found for "  << dev <<  unit << " -- is this a PCIe device?" << endl;

    	return 0;
    }

    cout << endl << "PCI Express device configuration space status for EDT " << dev << " unit " << unit << endl << endl;

    cfgObj->display_ids();
    cfgObj->display_masterbit();

    //get device capabilities
    buf.desc = prev_addr + 0x4;
    edt_ioctl(edt_p, EDTG_CONFIG, &buf);
    cfgObj->set_dcr(buf.value);
    cfgObj->display_dcr();

    //get [device status][device control]
    buf.desc = prev_addr + 0x8;
    edt_ioctl(edt_p, EDTG_CONFIG, &buf);
    cfgObj->set_devstat(buf.value);
    cfgObj->display_devstat();

    buf.desc = prev_addr + 0x8;
    edt_ioctl(edt_p, EDTG_CONFIG, &buf);
    cfgObj->set_devctrl(buf.value);
    cfgObj->display_devctrl();

    //get [link capabilities]
    buf.desc = prev_addr + 0xC;
    edt_ioctl(edt_p, EDTG_CONFIG, &buf);
    cfgObj->set_lcr(buf.value);
    cfgObj->display_lcr();

    //get [link status][link control]
    buf.desc = prev_addr + 0x10;
    edt_ioctl(edt_p, EDTG_CONFIG, &buf);
    cfgObj->set_linkstat(buf.value);
    cfgObj->display_linkstat();
    cout << endl;

	return 1;
    
}



		
    



