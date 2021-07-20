#include "edtinc.h"
#include <stdio.h>
#include <bitset>
#include <iostream>
#include <sstream>

#ifndef PCIECFG_H
#define PCIECFG_H

using namespace std;


class pcieCfg
{
public:
    pcieCfg();
    ~pcieCfg();
    u_char get_capId();
    int compare_ids(u_char id);
    void set_ids(u_int dev, u_int ven);
    void set_dcr(u_int reg_values);
    void set_devstat(u_int reg_values);
    void set_devctrl(u_int reg_values);
    void set_linkstat(u_int reg_values);
    void set_masterbit(u_int mbit);
    void set_lcr(u_int reg_values);
    void display_masterbit();
    void display_dcr();
    void display_devstat();
    void display_devctrl();
    void display_linkstat();
    void display_ids();
    void display_lcr();


private:
    u_int devId;
    u_int venId;
    u_int capId;    //equals 0x10 for the capabilities reg
    u_int dcr;      //device capabilities reg
    u_int devStat;  //device status register
    u_int devCtrl;  //device control register
    u_int linkStat; //link status register
    u_int masterbit;
    u_int lcr;      //link capabilities reg

};

#endif //PCIECFG_H
