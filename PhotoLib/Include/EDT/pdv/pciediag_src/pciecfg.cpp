#include "pciecfg.h"


pcieCfg::pcieCfg()
{
    capId = 0x10;
}

u_char pcieCfg::get_capId()
{
    return this->capId;
}

//used to check if currently at
//Capability register set
//return 1 on true
int pcieCfg::compare_ids(u_char id)
{
    if(id == this->capId)
	return 1;

    return 0;
}

void pcieCfg::set_ids(u_int dev, u_int ven)
{
	this->devId = dev;
	this->venId = ven;
}

void pcieCfg::set_masterbit(u_int mbit)
{
    this->masterbit = mbit;
}

void pcieCfg::display_masterbit()
{
    cout << "\nBus master enable: " << hex << this->masterbit << endl;
}

void pcieCfg::display_ids()
{
	cout << "Device ID: " << hex << this->devId << endl;
	cout << "Vendor ID: " << hex << this->venId << endl;
}

void pcieCfg::set_dcr(u_int reg_values)
{
	this->dcr = reg_values;
}

void pcieCfg::display_dcr()
{
	cout << "\n********************************\n" ;
	cout << "Device Capabilities Register:\n";
    bitset<32> dcr_set(this->dcr);
	bitset<3> mps;
	mps[0] = dcr_set[0];
	mps[1] = dcr_set[1];
	mps[2] = dcr_set[2];
	
	u_int max_pyld_sz = mps.to_ulong();

	cout << endl << "Max Payload Size Supported: " << mps;

	switch(max_pyld_sz)
	{
	case 0:
		cout << " = 128 bytes" << endl;
		break;
	case 1:
		cout << " = 256 bytes" << endl;
		break;
	case 2:
		cout << " = 512 bytes" << endl;
		break;
	case 3:
		cout << " = 1KB" << endl;
		break;
	case 4:
		cout << " = 2KB" << endl;
		break;
	case 5:
		cout << " = 4KB" << endl;
		break;
	case 6:
		cout << " = Reserved" << endl;
		break;
	case 7:
		cout << " = Reserved" << endl;
		break;
	}

	bitset<2> pfs;
	pfs[0] = dcr_set[3];
	pfs[1] = dcr_set[4];
	cout << "Phantom Functions Supported: " << pfs << endl;

	cout << "End Tag Field Supported: " << dcr_set[5] << endl;

	bitset<3> e0al;
	e0al[0] = dcr_set[6];
	e0al[1] = dcr_set[7];
	e0al[2] = dcr_set[8];
	cout << "Endpoint L0 Acceptable Latency: " << e0al << endl;

	bitset<3> e1al;
	e1al[0] = dcr_set[10];
	e1al[1] = dcr_set[11];
	e1al[2] = dcr_set[12];
	cout << "Endpoint L1 Acceptable Latency: " << e1al << endl;

    cout << "Attention Button Present: " << dcr_set[12] << endl;

    cout << "Attention Indicator Present: " << dcr_set[13] << endl;

    cout  << "Power Indicator Present: " << dcr_set[14] << endl;

    u_int csplv = ((this->dcr & 0x00ff0000) >> 16);
    bitset<8> csplv_set(csplv);
    cout <<  "Captured Slot Power Limit Value: " << csplv_set << endl;

    bitset<2> cspls;
    cspls[0] = dcr_set[26];
    cspls[1] = dcr_set[27];
    cout << "Captured Slot Power Limit Scale: " << cspls << endl;


}

void pcieCfg::set_devstat(u_int reg_values)
{
	this->devStat = ((reg_values & 0xffff0000) >> 16);
}

void pcieCfg::display_devstat()
{
	cout << "\n********************************\n" ;
	cout << "Device Status Register:\n\n";
	bitset<16> devstat_set(this->devStat);

	cout << "Correctable Error Detected: " << devstat_set[0] << endl;
	cout << "Non-Fatal Error Detected: " << devstat_set[1] << endl;
	cout << "Fatal Error Detected: " << devstat_set[2] << endl;
	cout << "Unsupported Request Detected: " << devstat_set[3] << endl;
	cout << "Aux Power Detected: " << devstat_set[4] << endl;
	cout << "Transactions Pending: " << devstat_set[5] << endl;

}

void pcieCfg::set_linkstat(u_int reg_values)
{
	this->linkStat = ((reg_values & 0xffff0000) >> 16);
}


void pcieCfg::display_linkstat()
{
	cout << "\n********************************\n" ;
	cout << "Link Status Register:\n\n";
	bitset<16> linkstat_set(this->linkStat);
	u_int ls = ((this->linkStat & 0x000f));
	bitset<4>lnk_spd(ls);
	
	cout << "Negotiated Link Speed: " << lnk_spd;
#if 0 // WAS
	if(ls == 1)
		cout << " = 2.5 Gb/s" << endl;
	else
		cout << " = 5.0 Gb/s" << endl;
#else

	switch(ls)
	{
	case 0:
		cout << " = undefined" << endl;
		break;
	case 1:
		cout << " = 2.5 Gbps" << endl;
		break;
	case 2:
		cout << " = 5.0 Gbps" << endl;
		break;
	case 3:
		cout << "= 8.0 Gbps" << endl;
		break;
	case 4:
		cout << " = RSVD" << endl;
		break;
	case 5:
		cout << " = RSVD" << endl;
		break;
	case 6:
		cout << " = RSVD" << endl;
		break;
	case 7:
		cout << " = RSVD" << endl;
		break;
	}
#endif

    bitset<6> lnk_width;
    lnk_width[0] = linkstat_set[4];
    lnk_width[1] = linkstat_set[5];
    lnk_width[2] = linkstat_set[6];
    lnk_width[3] = linkstat_set[7];
    lnk_width[4] = linkstat_set[8];
    lnk_width[5] = linkstat_set[9];

    cout << "Negotiated Link Width: " << lnk_width;
    u_int lw = lnk_width.to_ulong();

    switch(lw)
	{
    case 1:
    	cout << " = x1" << endl;
    	break;
    case 2:
    	cout << " = x2" << endl;
    	break;
    case 4:
    	cout << " = x4" << endl;
    	break;
    case 8:
    	cout << " = x8" << endl;
    	break;
    case 12:
    	cout << " = x12" << endl;
    	break;
    case 16:
    	cout << " = x16" << endl;
    	break;
    case 32:
    	cout << " = x32" << endl;
    	break;
    default:
    	cout << " Reserved Encoding" << endl;
    	break;
    }

    cout << "Training Error: " << linkstat_set[10] << endl;
    cout << "Link Training: " << linkstat_set[11] << endl;
    cout << "Slot Clock Config: " << linkstat_set[12] << endl;

}

void pcieCfg::set_devctrl(u_int reg_values)
{
	this->devCtrl = (reg_values & 0x0000ffff);
}

void pcieCfg::display_devctrl()
{
    cout << "\n********************************\n" ;
	cout << "Device Control Register:\n\n";
    u_int max_read_req_size = ((this->devCtrl & 0x7000) >> 12);
    bitset<3> mrrs(max_read_req_size);
	cout << "Max Read Request Size: " << mrrs;

    switch(max_read_req_size)
    {
    case 0:
    	cout << " = 128 byte max" << endl;
    	break;
    case 1:
    	cout << " = 256 byte max" << endl;
    	break;
    case 2:
    	cout << " = 512 byte max" << endl;
    	break;
    case 3:
    	cout << " = 1KB max" << endl;
    	break;
    case 4:
    	cout << " = 2KB max" << endl;
    	break;
    case 5:
    	cout << " = 4KB max" << endl;
    	break;
    case 6:
    	cout << " = Reserved" << endl;
        break;
    case 7:
    	cout << " = Reserved" << endl;
    	break;
    }

    u_int max_payload = ((this->devCtrl & 0xe0));
    bitset<3> mxp(max_payload);
    cout << "Max Payload Size: " << mxp;

    switch(max_payload)
    {
    case 0:
    	cout << " = 128 byte max payload" << endl;
    	break;
    case 1:
    	cout << " = 256 byte max payload" << endl;
    	break;
    case 2:
    	cout << " = 512 byte max payload" << endl;
    	break;
    case 3:
    	cout << " = 1024 byte max payload" << endl;
    	break;
    case 4:
    	cout << " = 2048 byte max payload" << endl;
    	break;
    case 5:
    	cout << " = 4096 byte max payload" << endl;
    	break;
    case 6:
    	cout << " = Reserved" << endl;
        break;
    case 7:
    	cout << " = Reserved" << endl;
    	break;
    }

}

void pcieCfg::set_lcr(u_int reg_values)
{
	this->lcr = reg_values;
}

void pcieCfg::display_lcr()
{
	cout << "\n********************************\n" ;
	cout << "LINK Capabilities Register:\n";
    bitset<32> lcr_set(this->lcr);
	bitset<4> mls;
	mls[0] = lcr_set[0];
	mls[1] = lcr_set[1];
	mls[2] = lcr_set[2];
	mls[3] = lcr_set[3];
	
	u_int max_lnk_speed = mls.to_ulong();

	cout << endl << "Max Link Speed Supported: " << mls;

	switch(max_lnk_speed)
	{
	case 0:
		cout << " = undefined" << endl;
		break;
	case 1:
		cout << " = 2.5 Gbps" << endl;
		break;
	case 2:
		cout << " = 5.0 Gbps" << endl;
		break;
	case 3:
		cout << "= 8.0 Gbps" << endl;
		break;
	case 4:
		cout << " = RSVD" << endl;
		break;
	case 5:
		cout << " = RSVD" << endl;
		break;
	case 6:
		cout << " = RSVD" << endl;
		break;
	case 7:
		cout << " = RSVD" << endl;
		break;
	}

}





