

#include "edtinc.h"

#define EDT_TRACE_FILE

#include "edtregbits.h"

#include "edt_trace.h"

#include <math.h>



EDTAPI BitLabels BL_SG_NXT_CNT_WRITE[] =
{
	{"BURST_EN",	EDT_BURST_EN},
	{"DMA_MEM_RD",	EDT_DMA_MEM_RD},
	{"DMA_ABORT",	EDT_DMA_ABORT},
	{"EN_SG_DONE",	EDT_EN_SG_DONE},
	{"EN_MN_DONE",	EDT_EN_MN_DONE},
	{"DMA_START",	EDT_DMA_START},
	{"EN_RDY",	EDT_EN_RDY},
	{0,0}
};

EDTAPI BitLabels BL_SG_NXT_CNT[] =
{
	{"NXTPG_INT",	EDT_NXTPG_INT},
	{"CURPG_INT",	EDT_CURPG_INT},
	{"PG_INT",	EDT_PG_INT},
	{"READ0_0",	EDT_READ0_0},
	{"READ0_1",	EDT_READ0_1},
	{"READ0_2",	EDT_READ0_2},
	{"MN_NXT_EMP",	EDT_MN_NXT_EMP},
	{"MN_DMA_DONE",	EDT_MN_DMA_DONE},
	{"MN_BURST_EN",	EDT_MN_BURST_EN},
	{"MN_DMA_MEM_RD",	EDT_MN_DMA_MEM_RD},
	{"READ0_3",	EDT_READ0_3},
	{"DMA_RDY",	EDT_DMA_RDY},
	{"DMA_DONE",	EDT_DMA_DONE},
	{0,0}
};

EDTAPI BitLabels BL_SG_LIST_CNTL[] =
{
	{"LIST_PAGEINT",	EDT_LIST_PAGEINT},
	{0,0}
};
	
EDTAPI BitLabels BL_DMA_CFG_WRITE[] =
{
	{"RMT_ADDR",	EDT_RMT_ADDR},
	{"RMT_AUTO_INCR",	EDT_RMT_AUTO_INCR},
	{"RFIFO_ENB",	EDT_RFIFO_ENB},
	{"WRITE_STROBE",	EDT_WRITE_STROBE},
	{"RMT_EN_INTR",	EDT_RMT_EN_INTR},
	{"PCI_EN_INTR",	EDT_PCI_EN_INTR},
	{"RMT_DATA",	EDT_RMT_DATA},
	{"RMT_CCLK",	EDT_RMT_CCLK},
	{"RMT_PROG",	EDT_RMT_PROG},
	{"RMT_INIT",	EDT_RMT_INIT},
	{"RMT_DONE",	EDT_RMT_DONE},
	{"RMT_STATE",	EDT_RMT_STATE},
	{"FIFO_CNT",	EDT_FIFO_CNT},
	{"FIFO_FLG",	EDT_FIFO_FLG},
	{0,0}
};

EDTAPI BitLabels BL_DMA_CFG[] =
{
	{"FOI_XMT_EMP",	    FOI_XMT_EMP},
	{"FOI_RCV_RDY",	    FOI_RCV_RDY},		
	{"DMA_INTR",	EDT_DMA_INTR},
	{"RMT_INTR",	EDT_RMT_INTR},
	{"RMT_EN_INTR",	    EDT_RMT_EN_INTR},
	{"PCI_EN_INTR",	EDT_PCI_EN_INTR},
	{0,0}
};

EDTAPI BitLabels BL_DMA_INTCFG_ONLY[] = 
{
	{"TEMP_EN_INTR"  , EDT_ICFG_TEMP_EN_INTR},
	{"RMT_EN_INTR"  , EDT_ICFG_RMT_EN_INTR},
	{"PCI_EN_INTR"  , EDT_ICFG_PCI_EN_INTR},
	{0,0}
};


#define EDT_ICFG_WRITE_STROBE           0x02    /* give read burst fifo a dummy clock */
#define EDT_ICFG_EMPTY_CHAN_FIFO        0x08    /* SS16 Channel fifo flush bit*/
#define EDT_ICFG_TEMP_EN_INTR           0x20    /* Remote enable interrupt  */
#define EDT_ICFG_RMT_EN_INTR            0x40    /* Remote enable interrupt  */
#define EDT_ICFG_PCI_EN_INTR            0x80    /* dma enable interrupt     */

EDTAPI BitLabels BL_X_PROG[] =
{
	{"X_DATA",	X_DATA},
	{"X_CCLK",	X_CCLK},
	{"X_PROG",	X_PROG},
	{"X_INIT",	X_INIT},
	{"X_DONE",	X_DONE},
	{"X_INITSTAT",	X_INITSTAT},
	{0,0}
};
EDTAPI BitLabels BL_DMA_STATUS[] = 
{
	{"PCI_INTR", EDT_PCI_INTR},
	{"DMA_INTR", EDT_DMA_INTR},
	{"RMT_INTR", EDT_RMT_INTR},
        {0,0}
};


EDTAPI BitLabels BL_REGISTERS[] =
{
	{"DMA_CUR_ADDR",	EDT_DMA_CUR_ADDR},
	{"DMA_NXT_ADDR",	EDT_DMA_NXT_ADDR},
	{"DMA_CUR_CNT",	EDT_DMA_CUR_CNT,BL_SG_NXT_CNT},
	{"DMA_NXT_CNT",	EDT_DMA_NXT_CNT, BL_SG_NXT_CNT},
	{"SG_CUR_ADDR",	EDT_SG_CUR_ADDR},
	{"SG_NXT_ADDR",	EDT_SG_NXT_ADDR},
	{"SG_CUR_CNT",	EDT_SG_CUR_CNT, BL_SG_NXT_CNT},
	{"SG_NXT_CNT",	EDT_SG_NXT_CNT, BL_SG_NXT_CNT},
	{"DMA_CUR_ADDR",	EDT_DMA_CUR_ADDR + 0x20},
	{"DMA_NXT_ADDR",	EDT_DMA_NXT_ADDR + 0x20},
	{"DMA_CUR_CNT",	EDT_DMA_CUR_CNT + 0x20,BL_SG_NXT_CNT},
	{"DMA_NXT_CNT",	EDT_DMA_NXT_CNT + 0x20, BL_SG_NXT_CNT},
	{"SG_CUR_ADDR",	EDT_SG_CUR_ADDR + 0x20},
	{"SG_NXT_ADDR",	EDT_SG_NXT_ADDR + 0x20},
	{"SG_CUR_CNT",	EDT_SG_CUR_CNT + 0x20, BL_SG_NXT_CNT},
	{"SG_NXT_CNT",	EDT_SG_NXT_CNT + 0x20, BL_SG_NXT_CNT},
	{"DMA_CUR_ADDR",	EDT_DMA_CUR_ADDR + 0x40},
	{"DMA_NXT_ADDR",	EDT_DMA_NXT_ADDR + 0x40},
	{"DMA_CUR_CNT",	EDT_DMA_CUR_CNT + 0x40,BL_SG_NXT_CNT},
	{"DMA_NXT_CNT",	EDT_DMA_NXT_CNT + 0x40, BL_SG_NXT_CNT},
	{"SG_CUR_ADDR",	EDT_SG_CUR_ADDR + 0x40},
	{"SG_NXT_ADDR",	EDT_SG_NXT_ADDR + 0x40},
	{"SG_CUR_CNT",	EDT_SG_CUR_CNT + 0x40, BL_SG_NXT_CNT},
	{"SG_NXT_CNT",	EDT_SG_NXT_CNT + 0x40, BL_SG_NXT_CNT},
	{"DMA_CUR_ADDR",	EDT_DMA_CUR_ADDR + 0x60},
	{"DMA_NXT_ADDR",	EDT_DMA_NXT_ADDR + 0x60},
	{"DMA_CUR_CNT",	EDT_DMA_CUR_CNT + 0x60,BL_SG_NXT_CNT},
	{"DMA_NXT_CNT",	EDT_DMA_NXT_CNT + 0x60, BL_SG_NXT_CNT},
	{"SG_CUR_ADDR",	EDT_SG_CUR_ADDR + 0x60},
	{"SG_NXT_ADDR",	EDT_SG_NXT_ADDR + 0x60},
	{"SG_CUR_CNT",	EDT_SG_CUR_CNT + 0x60, BL_SG_NXT_CNT},
	{"SG_NXT_CNT",	EDT_SG_NXT_CNT + 0x60, BL_SG_NXT_CNT},
	{"GP_OUTPUT",	EDT_GP_OUTPUT},
	{"DMA_CFG",	EDT_DMA_CFG, BL_DMA_CFG},
	{"DMA_INTCFG",	EDT_DMA_INTCFG},
	{"DMA_INTCFG_ONLY",	EDT_DMA_INTCFG_ONLY},
	{"REMOTE_OFFSET",	EDT_REMOTE_OFFSET},
	{"DMA_INTCFG_ONLY",	EDT_DMA_INTCFG_ONLY},
	{"DMA_INTCFG",	EDT_DMA_INTCFG},
	{"REMOTE_OFFSET",	EDT_REMOTE_OFFSET},
	{"DMA_STATUS",	EDT_DMA_STATUS},
	{"REMOTE_DATA",	EDT_REMOTE_DATA},
	{"FLASHROM_ADDR",	EDT_FLASHROM_ADDR},
	{"FLASHROM_DATA",	EDT_FLASHROM_DATA},
	{"CHAN0",	EDT_CHAN0},
	{"CHAN1",	EDT_CHAN1},
	{"CHAN2",	EDT_CHAN2},
	{"CHAN3",	EDT_CHAN3},
	{"WRITECMD",	EDT_WRITECMD},
	{"PDV_CMD",	PDV_CMD},
	{"PDV_REV",	PDV_REV},
	{"PDV_STAT",	PDV_STAT},
	{"PDV_CFG",	PDV_CFG},
	{"SHUTTER",	PDV_SHUTTER},
	{"SHUTTER_LEFT",	PDV_SHUTTER_LEFT},
	{"PDV_UTIL3",	PDV_UTIL3},
	{"FRAME_PERIOD0",	PDV_FRAME_PERIOD0},
	{"FRAME_PERIOD1",	PDV_FRAME_PERIOD1},
	{"FRAME_PERIOD2",	PDV_FRAME_PERIOD2},
	{"DATA_PATH",	PDV_DATA_PATH},
	{"MODE_CNTL",	PDV_MODE_CNTL},
	{"DATA_MSB",	PDV_DATA_MSB},
	{"DATA_LSB",	PDV_DATA_LSB},
	{"FIXEDLEN",	PDV_FIXEDLEN},
	{"SERIAL_DATA",	PDV_SERIAL_DATA},
	{"SERIAL_DATA_STAT",	PDV_SERIAL_DATA_STAT},
	{"SERIAL_DATA_CNTL",	PDV_SERIAL_DATA_CNTL},
	{"SERIAL_CNTL2",	PDV_SERIAL_CNTL2},
	{"BYTESWAP",	PDV_BYTESWAP},
	{"UTILITY",	PDV_UTILITY},
	{"UTIL2",	PDV_UTIL2},
	{"SHIFT",	PDV_SHIFT},
	{"MASK",	PDV_MASK},
	{"MASK_LO",	PDV_MASK_LO},
	{"MASK_HI",	PDV_MASK_HI},
	{"ROICTL",	PDV_ROICTL},
	{"HSKIP",	PDV_HSKIP},
	{"HACTV",	PDV_HACTV},
	{"VSKIP",	PDV_VSKIP},
	{"VACTV",	PDV_VACTV},
	{"XOPT",	PDV_XOPT},
    {"CLSIM_CFGA", PDV_CLSIM_CFGA},
    {"CLSIM_CFGB", PDV_CLSIM_CFGB},
    {"CLSIM_CFGC", PDV_CLSIM_CFGC},
    {"CLSIM_EXSYNCDLY", PDV_CLSIM_EXSYNCDLY},
    {"CLSIM_FILLA", PDV_CLSIM_FILLA},
    {"CLSIM_FILLB", PDV_CLSIM_FILLB},
    {"CLSIM_HCNTMAX", PDV_CLSIM_HCNTMAX},
    {"CLSIM_VACTV", PDV_CLSIM_VACTV},
    {"CLSIM_VCNTMAX", PDV_CLSIM_VCNTMAX},
    {"CLSIM_HFVSTART", PDV_CLSIM_HFVSTART},
    {"CLSIM_HFVEND", PDV_CLSIM_HFVEND},
    {"CLSIM_HLVSTART", PDV_CLSIM_HLVSTART},
    {"CLSIM_HLVEND", PDV_CLSIM_HLVEND},
    {"CLSIM_HRVSTART", PDV_CLSIM_HRVSTART},
    {"CLSIM_HRVEND", PDV_CLSIM_HRVEND},
    {"CLSIM_TAP0START", PDV_CLSIM_TAP0START},
    {"CLSIM_TAP0DELTA", PDV_CLSIM_TAP0DELTA},
    {"CLSIM_TAP1START", PDV_CLSIM_TAP1START},
    {"CLSIM_TAP1DELTA", PDV_CLSIM_TAP1DELTA},
    {"CLSIM_TAP2START", PDV_CLSIM_TAP2START},
    {"CLSIM_TAP2DELTA", PDV_CLSIM_TAP2DELTA},
    {"CLSIM_TAP3START", PDV_CLSIM_TAP3START},
    {"CLSIM_TAP3DELTA", PDV_CLSIM_TAP3DELTA},
	{0,0}
};

EDTAPI BitLabels BL_PDV_ROICTL[] =
{
	{"ROICTL_DALSA_LS",	PDV_ROICTL_DALSA_LS},
	{"ROICTL_ROI_EN",	PDV_ROICTL_ROI_EN},
	{"ROICTL_SIM_SYNC",	PDV_ROICTL_SIM_SYNC},
	{"ROICTL_SIM_DAT",	PDV_ROICTL_SIM_DAT},
	{"ROICTL_DUAL_CHAN",	PDV_ROICTL_DUAL_CHAN},
	{"ROICTL_PCLKSEL_MSK",	PDV_ROICTL_PCLKSEL_MSK},
	{"RIOCTL_PCLKSEL_DBL_CAM",	PDV_RIOCTL_PCLKSEL_DBL_CAM},
	{0,0}
};

EDTAPI BitLabels BL_PDV_CMD[] =
{
	{"RESET_INTFC",	PDV_RESET_INTFC},
	{"ENABLE_GRAB",	PDV_ENABLE_GRAB},
	{"AQ_CLR",	PDV_AQ_CLR},
	{"CLR_CONT",	PDV_CLR_CONT},
	{0,0}
};

EDTAPI BitLabels BL_PDV_CFG[] =
{
	{"DIS_SHUTTER",	PDV_DIS_SHUTTER},
	{"TRIG",	PDV_TRIG},
	{"INV_SHUTTER",	PDV_INV_SHUTTER},
	{"FIFO_RESET",	PDV_FIFO_RESET},
	{"BAUD2",	PDV_BAUD2},
	{"RESERVED1",	PDV_RESERVED1},
	{"EN_DALSA",	PDV_EN_DALSA},
	{"INT_ENAQ",	PDV_INT_ENAQ},
	{0,0}
};


EDTAPI BitLabels BL_PDV_DATA_PATH[] =
{
	{"EXT_DEPTH",	PDV_EXT_DEPTH},
	{"DUAL_CHAN",	PDV_DUAL_CHAN},
	{"RES0",	PDV_RES0},
	{"RES1",	PDV_RES1},
	{"RES_MASK",	PDV_RES_MASK},
	{"INVERT",	PDV_INVERT},
	{"CONTINUOUS",	PDV_CONTINUOUS},
	{"INTERLACED",	PDV_INTERLACED},
	{"DECADE0",	PDV_DECADE0},
	{"DECADE1",	PDV_DECADE1},
	{0,0}
};


EDTAPI BitLabels BL_PDV_STAT[] =
{
	{"OVERRUN",	PDV_OVERRUN},
	{"FRAME_VALID",	PDV_FRAME_VALID},
	{"EXPOSURE",	PDV_EXPOSURE},
	{"FIELD",	PDV_FIELD},
	{"SW_ARMED",	PDV_SW_ARMED},
	{"CHAN_ID0",	PDV_CHAN_ID0},
	{"CHAN_ID1",	PDV_CHAN_ID1},
	{"GRAB_ARMED",	PDV_GRAB_ARMED},
	{"AQUIRE_IP",	PDV_AQUIRE_IP},
	{0,0}
};


EDTAPI BitLabels BL_PDV_UTILITY[] =
{
	{"BSWAP",	PDV_BSWAP},
	{"PAD0",	PDV_PAD0},
	{"PAD1",	PDV_PAD1},
	{"SSWAP",	PDV_SSWAP},
	{"DISABLEMD",	PDV_DISABLEMD},
	{"GENDATA",	PDV_GENDATA},
	{"SKIP",	PDV_SKIP},
	{"VIDDIR",	PDV_VIDDIR},
	{0,0}
};


EDTAPI BitLabels BL_PDV_UTIL2[] =
{

	{"PHOTO_TRIGGER",	PDV_PHOTO_TRIGGER},
	{"FLDID_TRIGGER",	PDV_FLDID_TRIGGER},
	{"AQUIRE_MULTIPLE",	PDV_AQUIRE_MULTIPLE},
	{"INT_ENFV",		PDV_INT_ENFV},
	{"DBL_TRIG",		PDV_DBL_TRIG},
	{"PULNIX",		PDV_PULNIX},
	{"MC4",			PDV_MC4},
	{"SEL_MC4",		PDV_SEL_MC4},
	{0,0}
};

EDTAPI BitLabels BL_PDVA_UTIL2[] =
{

	{"PHOTO_TRIGGER",	PDV_PHOTO_TRIGGER},
	{"FLDID_TRIGGER",	PDV_FLDID_TRIGGER},
	{"AQUIRE_MULTIPLE",	PDV_AQUIRE_MULTIPLE},
	{"INT_ENFV",		PDV_INT_ENFV},
	{"DBL_TRIG",		PDV_DBL_TRIG},
	{"PULNIX",		PDV_PULNIX},
	{"RX232",		PDV_RX232},
	{"SEL_MC4",		PDV_SEL_MC4},
	{0,0}
};

/* PDV_BYTESWAP treated like PCD_BYTESWAP */

EDTAPI BitLabels BL_PDV_BYTESWAP[] =
{
	{"BYTESWAP",	PCD_BYTESWAP},
	{"SHORTSWAP",	PCD_SHORTSWAP},
	{"EN_IDVDLY",	PCD_EN_IDVDLY},
	{0,0}
};


EDTAPI BitLabels BL_PDV_SHIFT[] =
{
	{"AIA_SWAP",	PDV_AIA_SWAP},
	{0,0}
};


EDTAPI BitLabels BL_PDV_MODE_CNTL[] =
{
	{"AIA_MC0",	PDV_AIA_MC0},
	{"AIA_MC1",	PDV_AIA_MC1},
	{"AIA_MC2",	PDV_AIA_MC2},
	{"AIA_MC3",	PDV_AIA_MC3},
	{"EN_SHUTTER0",	PDV_EN_SHUTTER0},
	{"EN_SHUTTER1",	PDV_EN_SHUTTER1},
	{"EN_SHUTTER2",	PDV_EN_SHUTTER2},
	{"EN_SHUTTER3",	PDV_EN_SHUTTER3},
	{0,0}
};

EDTAPI BitLabels BL_PDVCL_MODE_CNTL[] =
{
	{"CC0",	PDV_AIA_MC0},
	{"CC1",	PDV_AIA_MC1},
	{"CC2",	PDV_AIA_MC2},
	{"CC3",	PDV_AIA_MC3},
	{"EN_SHUTTER0",	PDV_EN_SHUTTER0},
	{"EN_SHUTTER1",	PDV_EN_SHUTTER1},
	{"EN_SHUTTER2",	PDV_EN_SHUTTER2},
	{"EN_SHUTTER3",	PDV_EN_SHUTTER3},
	{0,0}
};



EDTAPI BitLabels BL_PDV_MCTL_AIA[] =
{
	{"MODE_CONTROLLED",	PDV_MODE_CONTROLLED},
	{"MODE_TRG",	PDV_MODE_TRG},
	{"MODE_CONT",	PDV_MODE_CONT},

	{0,0}	
};

EDTAPI BitLabels BL_PDV_SERIAL_DATA_STAT[] =
{
	{"RECEIVE_RDY",	PDV_RECEIVE_RDY},
	{"TRANSMIT_RDY",	PDV_TRANSMIT_RDY},
	{"LHS_DONE",	LHS_DONE},
	{"LHS_INTERRUPT",	LHS_INTERRUPT},
	{"FVAL_INT",	PDV_FVAL_INT},
	{"AQUIRE_INT",	PDV_AQUIRE_INT},
	{"FVAL_INTA",	PDV_FVAL_INTA},
	{"INTFC_INT",	PDV_INTFC_INT},
	{0,0}
};

EDTAPI BitLabels BL_PDV_SERIAL_DATA_CNTL[] =
{

	{"EN_RX",	PDV_EN_RX},
	{"EN_TX",	PDV_EN_TX},
	{"EN_RX_INT",	PDV_EN_RX_INT},
	{"EN_TX_INT",	PDV_EN_TX_INT},
	{"EN_DEV_INT",	PDV_EN_DEV_INT},
	{"CLR_RX_INT",	PDV_CLR_RX_INT},
	{"BAUD0",	PDV_BAUD0},
	{"BAUD1",	PDV_BAUD1},
	{"LHS_INTEN",	LHS_INTEN},
	{0,0}

};

EDTAPI BitLabels BL_PDV_UTIL3[] =
{
	{"PDV_PTRIGINV",PDV_PTRIGINV},
	{"FV_INVERT",	PDV_FV_INVERT},
	{"FV_INT_INVERT", PDV_FV_INT_INVERT},
	{"PDV_TRIGINT",	PDV_TRIGINT},
	{"PDV_MODE16",	PDV_MODE16},
	{"PDV_FVADJ",	PDV_FVADJ},
	{"PDV_FRENA",	PDV_FRENA},
	{0,0}
};

EDTAPI BitLabels BL_PDV_LHS_CONTROL[] =
{
	{"LHS_RESET",	PDV_LHS_RESET},
	{"LHS_ENABLE",	PDV_LHS_ENABLE},
	{"DAC_LOAD",	PDV_LHS_DAC_LOAD},
	{"DAC_DATA",	PDV_LHS_DAC_DATA},
	{"DAC_CLOCK",	PDV_LHS_DAC_CLOCK},
	{0,0}
};

EDTAPI BitLabels BL_PDV_CL_CFG[] =
{
	{"RGB",		PDV_CL_CFG_RGB},
	{"IGNDVAL",	PDV_CL_CFG_IGNDVAL},
	{"LINESCAN",	PDV_CL_CFG_LINESCAN},
	{"ROIDIS",	PDV_CL_CFG_ROIDIS},
	{"GENFVAL",	PDV_CL_CFG_GENFVAL},
	{"DVINV",	PDV_CL_CFG_DVINV},
	{0,0}
};

EDTAPI BitLabels BL_PDV_CL_CFG2[] =
{
	{"SEPTRIG",	 PDV_CL_CFG2_SEPTRIG},
	{"OR_EXPOSE",	 PDV_CL_CFG2_OR_EXPOSE},
	{"OPTO_FRMTRIG", PDV_CL_CFG2_OPTO_FRMTRIG},
	{"PCLVFREE",	 PDV_CL_CFG2_PCLVFREE},
	{0,0}
};

EDTAPI BitLabels BL_PDV_CL_MGTSPEED[] =
{
	{"0001_2X", PDV_CL_MGTSPEED_CHS01_2XSPD},
	{"0203_2X", PDV_CL_MGTSPEED_CHS34_2XSPD},
	{0,0}
};

EDTAPI BitLabels BL_PDV_CLSIM_CFGA[] =
{
	{"LINESCAN",   0x80},
	{"LVCONT",     0x40},
	{"RVEN",       0x20},
	{"UARTLOOP",   0x10},
	{"SMALLOK",    0x08},
	{"INTLVEN",    0x04},
	{"FIRSTFC",    0x02},
	{"DATACNT",    0x01},
	{0,0}
};

EDTAPI BitLabels BL_PDV_CLSIM_CFGB[] =
{
	{"DVSKIP",     PDV_CLSIM_CFGB_DVSKIP},
	{"DVMODE",     PDV_CLSIM_CFGB_DVMODE},
	{0,0}
};


EDTAPI BitLabels BL_PDV_CLSIM_CFGC[] =
{
	{"LED",        PDV_CLSIM_CFGC_LED},
	{"TRIGSRC",    PDV_CLSIM_CFGC_TRIGSRC},
	{"TRIGPOL",    PDV_CLSIM_CFGC_TRIGPOL},
	{"TRIGFRAME",  PDV_CLSIM_CFGC_TRIGFRAME},
	{"TRIGLINE",   PDV_CLSIM_CFGC_TRIGLINE},
	{0,0}
};


#ifdef READY

/* For future expansion */

{"BAUD_MASK",	PDV_BAUD_MASK},
{"BRKENA",	PDV_BRKENA},
{"BRKSEL",	PDV_BRKSEL},


{"SIM_STATUS",	SIM_STATUS},
{"SIM_CFG",	SIM_CFG},
{"SIM_WIDTH",	SIM_WIDTH},
{"SIM_LDELAY",	SIM_LDELAY},
{"SIM_HEIGHT",	SIM_HEIGHT},
{"SIM_FDELAY",	SIM_FDELAY},
{"SIM_SPEED",	SIM_SPEED},
{"FOI_MSG",	FOI_MSG},
{"FOI_WR_MSG_DATA",	FOI_WR_MSG_DATA},
{"FOI_WR_MSG_STAT",	FOI_WR_MSG_STAT},
{"FOI_RD_MSG_DATA",	FOI_RD_MSG_DATA},
{"FOI_RD_MSG_STAT",	FOI_RD_MSG_STAT},
{"FOI_TX_FIFO_EMP",	FOI_TX_FIFO_EMP},
{"FOI_TX_FIFO_FULL",	FOI_TX_FIFO_FULL},
{"FOI_FIFO_FLUSH",	FOI_FIFO_FLUSH},
{"FOI_MSG_SEND",	FOI_MSG_SEND},
{"FOI_MSG_BSY",	FOI_MSG_BSY},
{"FOI_DATA_AVAIL",	FOI_DATA_AVAIL},
{"FOI_FIFO_EMPTY",	FOI_FIFO_EMPTY},
{"FOI_RXWM1",	FOI_RXWM1},
{"FOI_WAITEOF",	FOI_WAITEOF},
{"RCI_FIFO_EMPTY",	RCI_FIFO_EMPTY},
{"RCI_FIFO_AEMPTY",	RCI_FIFO_AEMPTY},
{"RCI_FIFO_AFULL",	RCI_FIFO_AFULL},
{"RCI_FIFO_FULL",	RCI_FIFO_FULL},
{"CAM_CMD",	CAM_CMD},
{"CAM_STAT",	CAM_STAT},
{"CAM_COMPARE",	CAM_COMPARE},
{"CAM_LSPIXELS",	CAM_LSPIXELS},
{"CAM_MSPIXELS",	CAM_MSPIXELS},
{"CAM_HEIGHT",	CAM_HEIGHT},
{"CAM_WIDTH",	CAM_WIDTH},
{"CAM_FV",	CAM_FV},
{"CAM_LV",	CAM_LV},
{"CAM_PCLK",	CAM_PCLK},
{"PCD_CMD",	PCD_CMD},
{"PCD_DATA_PATH_STAT",	PCD_DATA_PATH_STAT},
{"PCD_FUNCT",	PCD_FUNCT},
{"PCD_STAT",	PCD_STAT},
{"PCD_STAT_POLARITY",	PCD_STAT_POLARITY},
{"PCD_OPTION",	PCD_OPTION},
{"PCD_DIRA",	PCD_DIRA},
{"PCD_DIRB",	PCD_DIRB},
{"PCD_CONFIG",	PCD_CONFIG},
{"PCD_CYCLE_CNT",	PCD_CYCLE_CNT},
{"REF_SCALE",	EDT_REF_SCALE},
{"OUT_SCALE",	EDT_OUT_SCALE},
{"PCD_RTCWORD",	PCD_RTCWORD},
{"PCD_RTCADDR",	PCD_RTCADDR},
{"SSDIO_SHFT_RIGHT",	SSDIO_SHFT_RIGHT},
{"SSDIO_STROBE",	SSDIO_STROBE},
{"SSDIO_STOP_CLK",	SSDIO_STOP_CLK},
{"SSDIO_ENABLE_RDQ",	SSDIO_ENABLE_RDQ},
{"SSDIO_EN_EXT_CLK",	SSDIO_EN_EXT_CLK},
{"SSDIO_LAST_BIT",	SSDIO_LAST_BIT},
{"SSDIO_BYTE_CNT0",	SSDIO_BYTE_CNT0},
{"SSDIO_BYTE_CNT1",	SSDIO_BYTE_CNT1},
{"SSDIO_BYTECNT_MSK",	SSDIO_BYTECNT_MSK},
{"SSDIO_BYTECNT_SHFT",	SSDIO_BYTECNT_SHFT},
{"SSDIO_IDLE",	SSDIO_IDLE},
{"REF_XTAL",	REF_XTAL},
{"FAST_DIV",	EDT_FAST_DIV},
{"FAST_DIV1",	EDT_FAST_DIV1},
{"FAST_DIV3",	EDT_FAST_DIV3},
{"FAST_DIV5",	EDT_FAST_DIV5},
{"FAST_DIV7",	EDT_FAST_DIV7},
{"X_DIVN",	EDT_X_DIVN},
{"X_DIVN_SHFT",	EDT_X_DIVN_SHFT},
{"PCD_DIR",	PCD_DIR},
{"PCD_BNR_ENABLE",	PCD_BNR_ENABLE},
{"PCD_DATA_INV",	PCD_DATA_INV},
{"PCD_ENABLE",	PCD_ENABLE},
{"PCD_STAT_INT_EN_1",	PCD_STAT_INT_EN_1},
{"PCD_STAT_INT_EN_2",	PCD_STAT_INT_EN_2},
{"PCD_STAT_INT_EN_3",	PCD_STAT_INT_EN_3},
{"PCD_STAT_INT_EN_4",	PCD_STAT_INT_EN_4},
{"PCD_STAT_INT_EN_X",	PCD_STAT_INT_EN_X},
{"PCD_STAT_1",	PCD_STAT_1},
{"PCD_STAT_2",	PCD_STAT_2},
{"PCD_STAT_3",	PCD_STAT_3},
{"PCD_STAT_4",	PCD_STAT_4},
{"PCD_STAT_INT_1",	PCD_STAT_INT_1},
{"PCD_STAT_INT_2",	PCD_STAT_INT_2},
{"PCD_STAT_INT_3",	PCD_STAT_INT_3},
{"PCD_STAT_INT_4",	PCD_STAT_INT_4},
{"PCD_STAT_INT_X",	PCD_STAT_INT_X},
{"PCD_STAT_INT_ENA",	PCD_STAT_INT_ENA},
{"PCD_EN_BACK_IO",	PCD_EN_BACK_IO},
{"PCD_OUT_EMP",	PCD_OUT_EMP},
{"PCD_UNUSED",	PCD_UNUSED},
{"PCD_UNDERFLOW",	PCD_UNDERFLOW},
{"PCD_OVERFLOW",	PCD_OVERFLOW},
{"PCD_IN_FULL",	PCD_IN_FULL},
{"PCD_SPARE0",	PCD_SPARE0},
{"PCD_IN_ALMOST_FULL",	PCD_IN_ALMOST_FULL},
{"PCD_SPARE",	PCD_SPARE},
{"APTIX_UNDERFLOW",	APTIX_UNDERFLOW},
{"APTIX_OVERFLOW",	APTIX_OVERFLOW},
{"APTIX_IDLE",	APTIX_IDLE},
{"APTIX_WAIT_FOR_WRITE",	APTIX_WAIT_FOR_WRITE},
{"APTIX_WAIT_FOR_READ",	APTIX_WAIT_FOR_READ},
{"APTIX_READING",	APTIX_READING},
{"PCD_BYTESWAP",	PCD_BYTESWAP},
{"PCD_SHORTSWAP",	PCD_SHORTSWAP},
{"PCD_EN_IDVDLY",	PCD_EN_IDVDLY},
{"XTEST_CMD",	XTEST_CMD},
{"XTEST_CMD0",	XTEST_CMD0},
{"XTEST_CMD1",	XTEST_CMD1},
{"XTEST_STAT",	XTEST_STAT},
{"XTEST_NOTUSED",	XTEST_NOTUSED},
{"XTEST_DIRREG",	XTEST_DIRREG},
{"XTEST_DIRA",	XTEST_DIRA},
{"XTEST_DIRB",	XTEST_DIRB},
{"XTEST_COUNT1",	XTEST_COUNT1},
{"XTEST_COUNT0",	XTEST_COUNT0},
{"XTEST_RDDO",	XTEST_RDDO},
{"XTEST_RDDO0_7",	XTEST_RDDO0_7},
{"XTEST_RDDO8_15",	XTEST_RDDO8_15},
{"XTEST_RDCO",	XTEST_RDCO},
{"XTEST_RDCO0_7",	XTEST_RDCO0_7},
{"XTEST_RDCO8_15",	XTEST_RDCO8_15},
{"XTEST_RDDI",	XTEST_RDDI},
{"XTEST_RDDI0_7",	XTEST_RDDI0_7},
{"XTEST_RDDI8_15",	XTEST_RDDI8_15},
{"XTEST_RDCI",	XTEST_RDCI},
{"XTEST_RDCI0_7",	XTEST_RDCI0_7},
{"XTEST_RDCI8_15",	XTEST_RDCI8_15},
{"XTEST_EN_DATA_03_811",	XTEST_EN_DATA_03_811},
{"XTEST_EN_DATA_47_1215",	XTEST_EN_DATA_47_1215},
{"XTEST_EN_CTL_03_811",	XTEST_EN_CTL_03_811},
{"XTEST_EN_CTL_47_1215",	XTEST_EN_CTL_47_1215},
{"XTEST_ASSRT_DEV_INT",	XTEST_ASSRT_DEV_INT},
{"XTEST_EN_BACK_IO",	XTEST_EN_BACK_IO},
{"XTEST_DEV_READ",	XTEST_DEV_READ},
{"XTEST_EN_DATA",	XTEST_EN_DATA},
{"XTEST_UN_RESET_FIFO",	XTEST_UN_RESET_FIFO},
{"XTEST_BIT32",	XTEST_BIT32},
{"XTEST_SWAPWORDS",	XTEST_SWAPWORDS},
{"XTEST_INVERT",	XTEST_INVERT},
{"XTEST_SWAPBYTES",	XTEST_SWAPBYTES},
{"XTEST_NOT_OUT_FIFO_1_EMPTY",	XTEST_NOT_OUT_FIFO_1_EMPTY},
{"XTEST_NOT_OUT_FIFO_0_EMPTY",	XTEST_NOT_OUT_FIFO_0_EMPTY},
{"XTEST_NOT_IN_FIFO_1_FULL",	XTEST_NOT_IN_FIFO_1_FULL},
{"XTEST_NOT_IN_FIFO_0_FULL",	XTEST_NOT_IN_FIFO_0_FULL},
{"XTEST_NOT_IN_FIFO_A_FULL",	XTEST_NOT_IN_FIFO_A_FULL},
{"PCDTEST_ENCHK",	PCDTEST_ENCHK},
{"PCDTEST_ENCMP",	PCDTEST_ENCMP},
{"PCDTEST_ENUNDER",	PCDTEST_ENUNDER},
{"PCDTEST_ENABLE",	PCDTEST_ENABLE},
{"PCDTEST_SETDNR",	PCDTEST_SETDNR},
{"PCDTEST_IGNORE_BNR",	PCDTEST_IGNORE_BNR},
{"PCDTEST_EN_BACK_IO",	PCDTEST_EN_BACK_IO},
{"PCDTEST_STAT_MSK",	PCDTEST_STAT_MSK},
{"PCDTEST_FUNCT_MSK",	PCDTEST_FUNCT_MSK},
{"FUNCT_PLLCLK",	EDT_FUNCT_PLLCLK},
{"FUNCT_SELAV",	EDT_FUNCT_SELAV},
{"FUNCT_DATA",	EDT_FUNCT_DATA},
{"FUNCT_CLK",	EDT_FUNCT_CLK},
{"PCDTEST_OFIFO_NE",	PCDTEST_OFIFO_NE},
{"PCDTEST_IFIFO_EMP",	PCDTEST_IFIFO_EMP},
{"PCDTEST_UNDER",	PCDTEST_UNDER},
{"PCDTEST_IFIFO_NF",	PCDTEST_IFIFO_NF},
{"PCDTEST_IFIFO_NAF",	PCDTEST_IFIFO_NAF},
{"PCDTEST_SP_IN",	PCDTEST_SP_IN},
{"PCDT_CMD",	PCDT_CMD},
{"PCDT_DPSTAT",	PCDT_DPSTAT},
{"PCDT_STAT",	PCDT_STAT},
{"PCDT_FUNCT",	PCDT_FUNCT},
{"PCDT_POLARITY",	PCDT_POLARITY},
{"PCDT_DIVIDE",	PCDT_DIVIDE},
{"PCDT_START",	PCDT_START},
{"PCDT_BITE",	PCDT_BITE},
{"PCDT_IODIR",	PCDT_IODIR},
{"SSDT_FUNC",	SSDT_FUNC},
{"PCD_DIRREG",	PCD_DIRREG},
{"P16_COMMAND",	P16_COMMAND},
{"P16_CONFIG",	P16_CONFIG},
{"P16_STATUS",	P16_STATUS},
{"P16_DATA",	P16_DATA},
{"P16_EN_INT",	P16_EN_INT},
{"P16_EN_DINT",	P16_EN_DINT},
{"P16_FCLK",	P16_FCLK},
{"P16_ODDSTART",	P16_ODDSTART},
{"P16_BCLR",	P16_BCLR},
{"P16_INIT",	P16_INIT},
{"P16_FNCT3",	P16_FNCT3},
{"P16_FNCT2",	P16_FNCT2},
{"P16_FNCT1",	P16_FNCT1},
{"P16_FLUSH",	P16_FLUSH},
{"P16_SWAP",	P16_SWAP},
{"P16_DFRST",	P16_DFRST},
{"P16_CLKP",	P16_CLKP},
{"P16_DACKP",	P16_DACKP},
{"P16_INT",	P16_INT},
{"P16_DEVINT",	P16_DEVINT},
{"P16_DMA_INT",	P16_DMA_INT},
{"P16_DINT_S",	P16_DINT_S},
{"P16_STATC_S",	P16_STATC_S},
{"P16_STATB_S",	P16_STATB_S},
{"P16_STATA_S",	P16_STATA_S},
{"P16_D_INIT_S",	P16_D_INIT_S},
{"P16_FNCT3_S",	P16_FNCT3_S},
{"P16_FNCT2_S",	P16_FNCT2_S},
{"P16_FNCT1_S",	P16_FNCT1_S},
{"P16_ENDINT_S",	P16_ENDINT_S},
{"P16_DIR_S",	P16_DIR_S},
{"P11_COMMAND",	P11_COMMAND},
{"P11_CONFIG",	P11_CONFIG},
{"P11_STATUS",	P11_STATUS},
{"P11_DATA",	P11_DATA},
{"P11_COUNT",	P11_COUNT},
{"P11_COMMAND",	P11_COMMAND},
{"P11_CONFIG",	P11_CONFIG},
{"P11_STATUS",	P11_STATUS},
{"P11_DATA",	P11_DATA},
{"P11_COUNT",	P11_COUNT},
{"command",	command},
{"P11W_GO",	P11W_GO},
{"P11W_BLKM",	P11W_BLKM},
{"P11W_DIRS0",	P11W_DIRS0},
{"P11W_DIRS1",	P11W_DIRS1},
{"P11W_FNCT1",	P11W_FNCT1},
{"P11W_FNCT2",	P11W_FNCT2},
{"P11W_FNCT3",	P11W_FNCT3},
{"P11W_INIT",	P11W_INIT},
{"P11W_BCLR",	P11W_BCLR},
{"P11W_ODDSTART",	P11W_ODDSTART},
{"P11W_FCYC",	P11W_FCYC},
{"P11W_EN_ATT",	P11W_EN_ATT},
{"P11W_EN_CNT",	P11W_EN_CNT},
{"P11W_EN_INT",	P11W_EN_INT},
{"convienent",	convienent},
{"P11W_RESET",	P11W_RESET},
{"status",	status},
{"P11W_RDY_S",	P11W_RDY_S},
{"P11W_BLKM_S",	P11W_BLKM_S},
{"P11W_FNCT1_S",	P11W_FNCT1_S},
{"P11W_FNCT2_S",	P11W_FNCT2_S},
{"P11W_FNCT3_S",	P11W_FNCT3_S},
{"P11W_INIT_S",	P11W_INIT_S},
{"P11W_STATA_S",	P11W_STATA_S},
{"P11W_STATB_S",	P11W_STATB_S},
{"P11W_STATC_S",	P11W_STATC_S},
{"P11W_ATTN",	P11W_ATTN},
{"P11W_DMAINT",	P11W_DMAINT},
{"P11W_ATTNINT",	P11W_ATTNINT},
{"P11W_CNTINT",	P11W_CNTINT},
{"P11W_REOD",	P11W_REOD},
{"P11W_INT",	P11W_INT},
{"configuration",	configuration},
{"P11W_BSYP",	P11W_BSYP},
{"P11W_CYCP",	P11W_CYCP},
{"P11W_ENBB",	P11W_ENBB},
{"P11W_OUTSK0",	P11W_OUTSK0},
{"P11W_OUTSK1",	P11W_OUTSK1},
{"P11W_INSK0",	P11W_INSK0},
{"P11W_INSK1",	P11W_INSK1},
{"P11W_RDYT",	P11W_RDYT},
{"P11W_SWAP",	P11W_SWAP},
{"P11W_INV",	P11W_INV},
{"P11W_NCOA",	P11W_NCOA},
{"P11W_PFCT2",	P11W_PFCT2},
{"LOCAL_XILINX_TYPE",	LOCAL_XILINX_TYPE},
{"REMOTE_XILINX_TYPE",	REMOTE_XILINX_TYPE},
{"MAC8100_TYPE",	MAC8100_TYPE},
{"LOCAL_DMA_TYPE",	LOCAL_DMA_TYPE},
{"REG_FILE_TYPE",	REG_FILE_TYPE},
{"INTFC_BYTE",	INTFC_BYTE},
{"INTFC_WORD",	INTFC_WORD},
{"INTFC_32",	INTFC_32},
{"MAC8100_WORD",	MAC8100_WORD},
{0}
};

#endif


/* Tracebuf uses a structure to access the trace buffer 
*/

#define TRACE_BUF_SIZE ((EDT_TRACESIZE -1) >> 1)

int name_regs = 1;

typedef struct _trace_entry {
    u_int counter;
    u_int value;
} EdtTraceEntry;

typedef struct trace_buf_data {
    u_int entrys; 
    u_int start_index;
    EdtTraceEntry buffer[TRACE_BUF_SIZE];
} EdtTraceBufData;

typedef struct trace_buf {
    EdtTraceBufData data;
    u_int curcount;
    u_int count;
    u_int index;
    u_int bufsize;
} EdtTraceBuf;

static char *
lookup_reg_name(u_int reg)

{
    static char addr[10];
    int i;

    for (i=0;BL_REGISTERS[i].label;i++)
    {
	if (BL_REGISTERS[i].bit == reg)
	    return BL_REGISTERS[i].label;

    }

    sprintf(addr, "%08x", reg);

    return addr;	

}

static void
GetFlagsString(char *s, BitLabels * labels, long flags)

{

    int     i = 0;
    int     started = 0;

    s[0] = 0;

    for (i = 0; labels[i].label != NULL; i++)
    {
	if (flags & labels[i].bit)
	{
	    if (started)
		sprintf(s, " %s", labels[i].label);
	    else
		sprintf(s, "%s", labels[i].label);
	    started = 1;

	    s += strlen(s);

	}
    }
}

static void
PrintRegister(EdtDev * edt_p, char *name, int offset, BitLabels * labels)

{
    char    flags[1024];
    char    format[80];
    int     value;

    int     size = (offset >> 24) & 3;

    if (!size)
	size = 4;

    if (size == 1)
	    printf("%02x     ", offset & 0xff);
	else printf("%02x-%02x  ", offset & 0xff, (offset+size-1) & 0xff);

    size *= 2;

    sprintf(format, "%%-20s: %%0%dx", size);

    value = edt_reg_read(edt_p, offset);

    printf(format, name, value);

    if (labels)
    {
	GetFlagsString(flags, labels, value);
	printf(" %s", flags);
    }

    printf("\n");

}

static long
isascii_str(u_char * buf, int n)
{
    int     i;

    for (i = 0; i < n; i++)
	if ((buf[i] < ' ' || buf[i] > '~')
	    && (buf[i] != '\t')
	    && (buf[i] != '\n')
	    && (buf[i] != '\r'))
	    return 0;
    return 1;
}

static void
print_hex(FILE *output, u_char *buf, int count)
{
    int i;

    fprintf(output, "<");
    for (i=0; i<count; i++)
	fprintf(output, "%02x%s", buf[i], i == count-1?">":" ");
    fprintf(output, "\n");
}

static void
print_ascii(FILE *output, char *buf)
{
    unsigned int i;

    fprintf(output, "<");
    for (i=0; i<strlen(buf); i++)
    {
	if (buf[i] >= ' ' && buf[i] <= '~')
	    fprintf(output, "%c", buf[i]);
	else if (buf[i] == '\t')
	    fprintf(output, "\\t");
	else if (buf[i] == '\n')
	    fprintf(output, "\\n");
	else if (buf[i] == '\r')
	    fprintf(output, "\\r");
    }
    fprintf(output, ">\n");
}

static u_int
tracebuf_has_data(EdtTraceBuf *trb)

{
    return trb->curcount < trb->count;
}

static u_int
bump_tracebuf(EdtTraceBuf *trb, int incr)

{
    trb->curcount += incr;

    if (incr > 0)
    {
	while (incr--)
	{
	    trb->index++;
	    if (trb->index >= trb->bufsize)
		trb->index = 0;

	}
    }
    else if (incr < 0)
    {
	while (incr++ < 0)
	{
	    trb->index--;
	    if (trb->index <= 0)
		trb->index = trb->bufsize - 1;
	}
    }

    return trb->data.buffer[trb->index].value;

}

static   double  svtime = 0;

static double
tracebuf_get_timestamp(EdtTraceBuf *tb)

{
    u_int value;
    double curtime;

    value = bump_tracebuf(tb,1);
    curtime = (double) (value) * 1000000000L;
    value = bump_tracebuf(tb,1);
    curtime += (double) (value);
    curtime /= 1000000000.0;

    return curtime;
}

static double
trimmed_time(double t)

{
    double x = t / 1000.0;

    x = x - ((int64_t) x);
    x *= 1000.0;

    return x;
}

int
code_in_filter(u_int *filter, u_int code)

{
    u_int i;

    for (i=0;filter[i];i++)
	if (filter[i] == code)
	    return 1;

    return 0;
}

void
print_trace_entries(EdtTraceBuf *tb, FILE *output, int startindex, int count, int bufsize, u_int *filter, u_int filter_mode)

{
    double  curtime;
    int doit;
    u_int code;
    u_int value;
    u_int indent = 0;
    u_int todo, done, wait, waitirp, buf;
    const char *indentstr;
    u_int dmastat, found, dmaCount;

    tb->curcount = 0;
    tb->index = startindex;
    tb->bufsize = bufsize;
    tb->count = count;

    while (tracebuf_has_data(tb))
    {

	code = tb->data.buffer[tb->index].value;

	doit =  (filter)? (code_in_filter(filter,code) ^ filter_mode):1;

#define do_fprintf if (doit) fprintf

	    do_fprintf(output,"%2d: %07x ", (tb->data.buffer[tb->index].counter >> 28),
	(tb->data.buffer[tb->index].counter & 0xffffff));
	    if (indent)
	    	do_fprintf(output,"    ");
	   
	    indentstr = (indent)?"":"    "; 

	    switch (code)
	    {
	    case TR_CLEANUP:
		do_fprintf(output,"CLEANUP\n");
		break;
	    case TR_READ_GRAB:
		do_fprintf(output,"READ_GRAB\n");
		break;
	    case TR_ISR_START:
		do_fprintf(output,"ISR_START\n");
		indent = 1;
		break;
	    case TR_ISR_END:
		indent = 0;
		do_fprintf(output,"ISR_END          %s: ", indentstr);
		value = bump_tracebuf(tb,1);
		do_fprintf(output,"%08x ", value);
#ifndef WIN32
		value = bump_tracebuf(tb,1);
		curtime = tracebuf_get_timestamp(tb);
		do_fprintf(output,"%20.6f (%3.6f)\n", trimmed_time(curtime), curtime - svtime);
		svtime = curtime;
#else
	fprintf(output,"\n");
#endif
		break;
	    case TR_LOAD_SG:
		do_fprintf(output,"LOAD_SG          %s: ", indentstr);
		value = bump_tracebuf(tb,1);
		do_fprintf(output,"%02x ", value);
		value = bump_tracebuf(tb,1);
		do_fprintf(output,"%08x ", value);
		value = bump_tracebuf(tb,1);
		do_fprintf(output,"%08x\n", value);
		break;


	    case TR_RT_INTR_START:
		do_fprintf(output,"RT_INTR_START\n");
		break;
	    case TR_RT_INTR_END:
		do_fprintf(output,"RT_INTR_END\n");
		break;
	    case TR_DPC_START:
		do_fprintf(output,"DPC_START\n");
		break;
	    case TR_DPC_END:
		do_fprintf(output,"DPC_END\n");
		break;
	    case TR_LOCK:
		do_fprintf(output,"LOCK\n");
		break;
	    case TR_UNLOCK:
		do_fprintf(output,"UNLOCK\n");
		break;
	    case TR_START_BUF:
		value = bump_tracebuf(tb,1);
		do_fprintf(output,"START_BUF        %s: %-5d\n", indentstr, value);
		break;
	    case TR_WAKEUP:
		do_fprintf(output,"WAKEUP\n");
		break;
	    case TR_DOGRAB:
		value = bump_tracebuf(tb,1);
		do_fprintf(output,"DOGRAB           %s: %-5d\n", indentstr, value);

		break;
	    case TR_WAIT_START:
		do_fprintf(output,"WAIT_START       %s: ", indentstr);
		wait = bump_tracebuf(tb,1);
		value = bump_tracebuf(tb,1);
		curtime = tracebuf_get_timestamp(tb);
		done = bump_tracebuf(tb,1);
		todo = bump_tracebuf(tb,1);
		waitirp = bump_tracebuf(tb, 1);

		do_fprintf(output,"%-5d %-5d %-5d %11.6f (%3.6f)\n", 
		    wait, done, todo, 
		    trimmed_time(curtime), curtime - svtime);
		svtime = curtime;
		break;
	    case TR_NEED_WAIT:
		do_fprintf(output,"NEED_WAIT\n");
		break;
	    case TR_WAIT_END:
		do_fprintf(output,"WAIT_END         %s: ", indentstr);
		wait = bump_tracebuf(tb,1);
		value = bump_tracebuf(tb,1);
		curtime = tracebuf_get_timestamp(tb);
		done = bump_tracebuf(tb,1);
		todo = bump_tracebuf(tb,1);

		do_fprintf(output,"%-5d %-5d %-5d %11.6f (%3.6f)\n", 
		    wait, done, todo, 
		    trimmed_time(curtime), curtime - svtime);
		svtime = curtime;
		break;
	    case TR_LOADED:
		do_fprintf(output,"LOADED           %s: ", indentstr);
		value = bump_tracebuf(tb,1);
		do_fprintf(output,"%-5d ", value);
		value = bump_tracebuf(tb,1);
		curtime = tracebuf_get_timestamp(tb);
		do_fprintf(output,"%23.6f (%3.6f)\n", trimmed_time(curtime), curtime - svtime);
		svtime = curtime;
		break;
	    case 0xdead3333:
		do_fprintf(output,"TRANSFERCOUNT    %s: ", indentstr);
		buf = bump_tracebuf(tb,1);
		dmastat = bump_tracebuf(tb,1);
		found = bump_tracebuf(tb,1);
		dmaCount = bump_tracebuf(tb,1);
		do_fprintf(output,"%-5d %-5d %-8d\n", 
		    buf, found, dmaCount); 
		break;
	
	    case TR_DONE:
		do_fprintf(output,"DONE             %s: ", indentstr);
		value = bump_tracebuf(tb,1);
		do_fprintf(output,"%-5d ", value);
		value = bump_tracebuf(tb,1);
		curtime = tracebuf_get_timestamp(tb);
		do_fprintf(output,"%23.6f (%3.6f)\n", trimmed_time(curtime), curtime - svtime);
		svtime = curtime;
		break;
	    case TR_SERIAL:
		do_fprintf(output,"SERIAL\n");
		break;
	    case TR_TIMEOUT:
		value = bump_tracebuf(tb,1);
		do_fprintf(output,"TIMEOUT          %s: %-8d\n", indentstr,value);
		break;
	    case TR_SET:
		do_fprintf(output,"SET\n");
		break;

	    case TR_REG_GET:
	    case TR_REG_SET:
		{
		    u_int reg_addr;
		    u_int reg_value;
		    u_int reg_size;
		    char fmt_string[32];

		    reg_addr = bump_tracebuf(tb,1);

		    reg_value  = bump_tracebuf(tb,1);

		    reg_size = EDT_REG_SIZE(reg_addr) & 0x7f;

		    if (name_regs)
		    {
			if (code == TR_REG_SET)
	            	{
				do_fprintf(output, "%-16s %s=> ", 
				lookup_reg_name(reg_addr), indentstr) ;
		    	}
				else
		    	{
				do_fprintf(output, "%-16s %s:  ",
				lookup_reg_name(reg_addr), indentstr) ;
		    	}
		    }
		    else
		    {
			if (code == TR_REG_SET)
	            	{
				do_fprintf(output, "%08x     %s=> ",
				reg_addr, indentstr);
		    	}
		    	else
		    	{
				do_fprintf(output, "%08x     %s:  ",
				reg_addr, indentstr);
		    	}
		    }
		   
	            sprintf(fmt_string,"%%0%dx\n", 2 * reg_size);
 
		    do_fprintf(output, fmt_string, reg_value);

		}

		break;
	    case TR_SETBUF:
		do_fprintf(output,"SETBUF\n");
		break;
	    case TR_MAP_BUF:
		do_fprintf(output,"MAP_BUF\n");
		break;
	    case TR_UNMAP_BUF:
		do_fprintf(output,"UNMAP_BUF\n");
		break;
	    case TR_ENABLE_INT:
		do_fprintf(output,"ENABLE_INT\n");
		break;
	    case TR_SER_READ:
		do_fprintf(output,"SER_READ\n");
		{
		    int     i;
		    int     size;
		    int     intsize;
		    u_int   intbuf[8];
		    unsigned char *sp;

		    unsigned char tmpbuf[80];
		    unsigned char *tp;
		    char   *str_p;

		    /* bump to next here */
		    size = bump_tracebuf(tb,1);

		    do_fprintf(output,"%d bytes: ", size);

		    if (size > 15)
			size = 15;

		    tp = tmpbuf;
		    intsize = (size + 3) >> 2;

		    for (i = 0; i < intsize; i++)
		    {
			intbuf[i] = bump_tracebuf(tb,1);
		    }

		    sp = (unsigned char *) intbuf;

		    for (i = 0; i < size; i++)
		    {
			if (sp[i] == 0)
			{
			    *tp++ = '[';
			    *tp++ = '0';
			    *tp++ = ']';
			}
			else
			    *tp++ = (unsigned char) sp[i];

			*tp = 0;
		    }
		    *tp = 0;
		    str_p = (char *) tmpbuf;
		    if (isascii_str((u_char *)str_p, (int) strlen(str_p)))
			print_ascii(output, (char *)str_p);
		    else print_hex(output, (u_char *)str_p, (int) strlen(str_p));
		    /* get bumped below so back up one */

		}
		break;
	    case TR_DEVINT:
		do_fprintf(output,"DEVINT\n");
		break;
	    case TR_SER_WRITE:
		do_fprintf(output,"SER_WRITE\n");
		{
		    int     i;
		    int     size;
		    int     intsize;
		    u_int   intbuf[8];

		    unsigned char *sp;
		    unsigned char tmpbuf[80];
		    unsigned char *tp;
		    char   *str_p;


		    /* bump to next here */
		    size = bump_tracebuf(tb,1);

		    do_fprintf(output,"%d bytes: ", size);

		    if (size > 15)
			size = 15;

		    tp = tmpbuf;
		    intsize = (size + 3) >> 2;

		    for (i = 0; i < intsize; i++)
		    {
			intbuf[i] = bump_tracebuf(tb,1);
		    }

		    sp = (unsigned char *) intbuf;

		    for (i = 0; i < size; i++)
		    {
			if (sp[i] == 0)
			{
			    *tp++ = '[';
			    *tp++ = '0';
			    *tp++ = ']';
			}
			else
			    *tp++ = (unsigned char) sp[i];

			*tp = 0;
		    }


		    str_p = (char *) tmpbuf;

		    if (isascii_str((u_char *)str_p, size))
			   print_ascii(output, (char *)str_p);
		    else print_hex(output, (u_char *)str_p, size);
		}
		break;
	    case TR_SNDMSG:
		do_fprintf(output,"SNDMSG\n");
		break;
	    case TR_TMSTAMP:
		do_fprintf(output,"TMSTAMP        %s: ",indentstr);
		curtime = tracebuf_get_timestamp(tb);
		do_fprintf(output,"%29.6f (%3.6f)\n",trimmed_time(curtime) , curtime - svtime);
		svtime = curtime;
		break;
	    case TR_REFTMSTAMP:
		do_fprintf(output,"REFTMSTAMP     %s: ",indentstr);
		value = bump_tracebuf(tb,1);
		do_fprintf(output,"%08x ", value);
		bump_tracebuf(tb,1);
		curtime = tracebuf_get_timestamp(tb);
		do_fprintf(output,"%20.6f (%3.6f)\n", trimmed_time(curtime), curtime - svtime);
		svtime = curtime;
		break;
	    case TR_SER_READ_END:
		do_fprintf(output,"SER_READ_END\n");
		break;
	    case TR_SER_WRITE_END:
		do_fprintf(output,"SER_WRITE_END\n");
		break;
	    case TR_ISRGRAB:
		do_fprintf(output,"ISRGRAB\n");
		break;
	    case TR_P11W_ATTNINT:
		do_fprintf(output,"P11W_ATTNINT\n");
		break;
	    case TR_P11W_CNTINT:
		do_fprintf(output,"P11W_CNTINT\n");
		break;
	    case TR_ENDCLR:
		do_fprintf(output,"ENDCLR\n");
		break;
	    case TR_STARTCLR:
		do_fprintf(output,"STARTCLR\n");
		break;
	    case TR_FLUSH:
		do_fprintf(output,"FLUSH\n");
		break;
	    case TR_SETEVENT:
		do_fprintf(output,"SETEVENT\n");
		break;
	    case TR_EVENTSLP:
		do_fprintf(output,"EVENTSLP\n");
		break;
	    case TR_EVENTWK:
		do_fprintf(output,"EVENTWK\n");
		break;
	    case TR_EVENTWARN:
		do_fprintf(output,"EVENTWARN\n");
		break;
	    case TR_DISABLE_INT:
		do_fprintf(output,"DISABLE_INT\n");
		break;
	    case TR_SERWAIT:
		do_fprintf(output,"SERWAIT\n");
		break;
	    case TR_SEREND:
		do_fprintf(output,"SEREND\n");
		break;
	    case TR_CONTSTATE:
		do_fprintf(output,"CONTSTATE\n");
		break;
	    case TR_ABORT:
		do_fprintf(output,"ABORT\n");
		break;
	    case TR_NO_ACTION:
		do_fprintf(output,"NO_ACTION\n");
		break;
	    case TR_CHECK_SG_NXT:
		do_fprintf(output,"CHECK_SG_NXT\n");
		break;
	    case TR_ISR_START_RT:
		do_fprintf(output,"ISR_START_RT\n");
		break;
	    case TR_ISR_END_RT:
		do_fprintf(output,"ISR_END_RT\n");
		break;
	    case TR_SETUP_DMA:
		do_fprintf(output,"SETUP_DMA\n");
		break;
	    case TR_SETUP_DMA_DONE:
		do_fprintf(output,"SETUP_DMA_DONE\n");
		break;
	    case TR_SETUP_RDY:
		do_fprintf(output,"SETUP_RDY\n");
		break;
	    case TR_SETUP_RDY_DONE:
		do_fprintf(output,"SETUP_RDY_DONE\n");
		break;
	    default:
		do_fprintf(output,"%08x\n", code);
	    }
	code = bump_tracebuf(tb,1);
    }

}

EdtTraceBuf trbuf;

void edt_trace(EdtDev *edt_p, FILE *output, int doraw, int loops, u_int *filter, u_int filter_mode)

{
    u_int   entrys;
    u_int   idx;
    u_int   last_idx = 0;
    u_int   last_entrys = 0;
    int     count;
    int     firsttrace = 1;
    int     loop = 0;

    memset(&trbuf, 0, sizeof(trbuf));
    printf("sizeof trbuf = %ld\n", (long)sizeof(trbuf.data));

    for (loop = 0; loop < loops || loops == 0 ; loop ++)
    {
	edt_get_tracebuf(edt_p, (u_int *) &(trbuf.data));
	entrys = trbuf.data.entrys / 2;
	 idx = (trbuf.data.start_index-2) / 2;

	if (firsttrace)
	{
	    if (!doraw)
		fprintf(output,"TraceBuf %x entrys idx %d\n", entrys, idx);
	    if (entrys <= TRACE_BUF_SIZE)
	    {
		count = entrys;
	    }
	    else
	    {
		count = TRACE_BUF_SIZE;
	    }
	    firsttrace = 0;
	    if (entrys > TRACE_BUF_SIZE)
		 last_idx = idx;
	}
	else
	{
	    count = entrys - last_entrys;
	    /* leave tmpp alone */
	    /*
	    */

	    if (count == 0)
	    {
		loop++;
		if ((loop % 10) == 0 && !doraw)
		{
		    fprintf(output,".");
		    fflush(stdout);
		}
	    }
	    else
	    {
		loop = 0;
		if (!doraw)
		    fprintf(output,"\n");
	    }

	    if (count > TRACE_BUF_SIZE)
	    {
		count = TRACE_BUF_SIZE;
	    }

	}

	if (count)
	{
	    if (doraw)
		fwrite(&trbuf.data,sizeof(trbuf.data),1,output);
	    else
		print_trace_entries( &trbuf, output, last_idx, count, TRACE_BUF_SIZE, filter, filter_mode);
	}

	last_idx = idx - 1;
	last_entrys = entrys;

	if (loops == 0 && count == 0)
	    edt_msleep(5);
    }
}

void edt_trace_file(FILE *input, FILE *output, u_int *filter, u_int filter_mode)

{
    u_int   entrys;
    u_int   idx;
    u_int   last_idx = 0;
    u_int   last_entrys = 0;
    int     count;
    int     firsttrace = 1;
    int     loop = 0;

    memset(&trbuf, 0, sizeof(trbuf));
    printf("sizeof trbuf = %ld\n", (long)sizeof(trbuf));

    while (fread(&trbuf.data,sizeof(trbuf.data),1,input))
    {
	entrys = trbuf.data.entrys / 2;
	idx = trbuf.data.start_index / 2;

	if (firsttrace)
	{
	    fprintf(output,"TraceBuf %x entrys idx %d\n", entrys, idx);
	    if (entrys <= TRACE_BUF_SIZE)
	    {
		count = entrys;
	    }
	    else
	    {
		count = TRACE_BUF_SIZE;
	    }
	    firsttrace = 0;
	}
	else
	{
	    count = entrys - last_entrys;
	    /* leave tmpp alone */
	    /*
	    * fprintf(output,"Next start %x tmpp %x TraceBuf %d entrys idx %d %d
	    * new entrys\n", &tracebuf[2], tmpp, entrys, idx, count);
	    */

	    if (count == 0)
	    {
		loop++;
		if ((loop % 10) == 0)
		{
		    fprintf(output,".");
		    fflush(stdout);
		}
	    }
	    else
	    {
		loop = 0;
		fprintf(output,"\n");
	    }

	    if (count > TRACE_BUF_SIZE)
	    {
		count = TRACE_BUF_SIZE;
	    }

	}

	if (count)
	{
	    print_trace_entries( &trbuf, 
		output, last_idx, count, TRACE_BUF_SIZE, filter, filter_mode);
	}

	last_idx = idx;

    }
}



void edt_dump_registers(EdtDev *edt_p, int debug)

{

    u_int fifocnt;
    u_int cfg;

    if (edt_p->devid != P53B_ID)
    {
	PrintRegister(edt_p, "DMA_CUR_ADDR", EDT_DMA_CUR_ADDR, NULL);
	PrintRegister(edt_p, "DMA_NXT_ADDR", EDT_DMA_NXT_ADDR, NULL);
	PrintRegister(edt_p, "DMA_CUR_CNT", EDT_DMA_CUR_CNT, NULL);
	PrintRegister(edt_p, "DMA_NXT_CNT", EDT_DMA_NXT_CNT, NULL);
	PrintRegister(edt_p, "SG_NXT_CNT", EDT_SG_NXT_CNT, BL_SG_NXT_CNT);
	PrintRegister(edt_p, "SG_NXT_ADDR", EDT_SG_NXT_ADDR, NULL);
	PrintRegister(edt_p, "SG_CUR_CNT", EDT_SG_CUR_CNT, NULL);
	PrintRegister(edt_p, "SG_CUR_ADDR", EDT_SG_CUR_ADDR, NULL);
	PrintRegister(edt_p, "DMA_CFG", EDT_DMA_CFG, BL_DMA_CFG);
	PrintRegister(edt_p, "DMA_STATUS", EDT_DMA_STATUS, NULL);
    }
    switch (edt_p->devid)
    {
#ifdef P53B
    case P53B_ID:
	{
	    static char msgbuf[1024];
	    static u_int wordbuf[0x800];

	    edt_ioctl(edt_p, P53G_DRIVER_STATUS, msgbuf);
	    puts(msgbuf);
	    if (debug > 1)
	    {
		int i;
		edt_ioctl(edt_p, P53G_WORDBUF1, wordbuf);
		edt_ioctl(edt_p, P53G_WORDBUF2, wordbuf + 0x400);
		for (i = 0; i < 0x800; i++)
		{

		    if (wordbuf[i] & 0x80000000)
			putchar('\n');
		    else
			putchar(' ');
		    printf("%x ", wordbuf[i] & 0x7fffffff);
		}
	    }
	    break;
	}
#endif
    case PDVFOI_ID:
	PrintRegister(edt_p, "DMA_CFG", EDT_DMA_CFG, NULL);
	PrintRegister(edt_p, "DMA_STATUS", EDT_DMA_STATUS, NULL);
	PrintRegister(edt_p, "FOI_MSG", FOI_MSG, NULL);
	PrintRegister(edt_p, "PDV_REV", PDV_REV, NULL);
	PrintRegister(edt_p, "PDV_STAT", PDV_STAT, BL_PDV_STAT);
	PrintRegister(edt_p, "PDV_UTIL2", PDV_UTIL2, NULL);
	PrintRegister(edt_p, "PDV_SHIFT", PDV_SHIFT, NULL);
	PrintRegister(edt_p, "PDV_MASK", PDV_MASK, NULL);
	PrintRegister(edt_p, "PDV_DATA_PATH", PDV_DATA_PATH, NULL);
	PrintRegister(edt_p, "PDV_MODE_CNTL", PDV_MODE_CNTL, NULL);
	PrintRegister(edt_p, "PDV_ROICTL", PDV_ROICTL, NULL);
	break;

    case PDVCL_ID:
    case PDVCL2_ID:
    case PE1DVVL_ID:
    case PE4DVVL_ID:
    case PE4DVCL_ID:
    case PE8DVCL_ID:
    case PE8DVCLS_ID:
    case PE8VLCLS_ID:
    case PDV_ID:
    case PDVA_ID:
    case PDVA16_ID:
    case PDVFOX_ID:
    case PE4DVFOX_ID:
    case PE8DVFOX_ID:
    case PE4DVVLFOX_ID:
    case PDVFCI_AIAG_ID:
    case PDVFCI_USPS_ID:
    case PC104ICB_ID:
    case PGP_RGB_ID:
    case PDV44_ID:
    case PDVK_ID:
	PrintRegister(edt_p, "DMA_CFG", EDT_DMA_CFG, NULL);
	cfg = edt_reg_read(edt_p, EDT_DMA_CFG);
	if (cfg & EDT_FIFO_FLG)
	{
	    fifocnt = ((cfg & EDT_FIFO_CNT) >> EDT_FIFO_SHIFT) + 1;
	    fifocnt *= 4;
	}
	else
	    fifocnt = 0;
	printf("       fifocnt             : %d\n", fifocnt);
	PrintRegister(edt_p, "DMA_STATUS", EDT_DMA_STATUS, NULL);
	PrintRegister(edt_p, "PDV_STAT", PDV_STAT, BL_PDV_STAT);
	PrintRegister(edt_p, "PDV_CFG", PDV_CFG, BL_PDV_CFG);
	PrintRegister(edt_p, "PDV_SHUTTER_LEFT",
	    PDV_SHUTTER_LEFT, NULL);
	PrintRegister(edt_p, "PDV_UTIL3",
	    PDV_UTIL3, BL_PDV_UTIL3);
#ifdef PDV
	PrintRegister(edt_p, "PDV_FRAME_PERIOD",
	    PDV_FRAME_PERIOD, NULL);
#endif
	PrintRegister(edt_p, "PDV_DATA_PATH",
	    PDV_DATA_PATH, BL_PDV_DATA_PATH);
	if (edt_is_dvcl(edt_p) || edt_is_dvfox(edt_p))
	    PrintRegister(edt_p, "PDV_MODE_CNTL",
	    PDV_MODE_CNTL, BL_PDVCL_MODE_CNTL);
	else PrintRegister(edt_p, "PDV_MODE_CNTL",
	    PDV_MODE_CNTL, BL_PDV_MODE_CNTL);
	PrintRegister(edt_p, "PDV_DATA_MSB",
	    PDV_DATA_MSB, NULL);
	PrintRegister(edt_p, "PDV_DATA_LSB",
	    PDV_DATA_LSB, NULL);
	PrintRegister(edt_p, "PDV_SERIAL_DATA",
	    PDV_SERIAL_DATA, NULL);
	PrintRegister(edt_p, "PDV_SERIAL_DATA_STAT",
	    PDV_SERIAL_DATA_STAT, BL_PDV_SERIAL_DATA_STAT);
	PrintRegister(edt_p, "PDV_SERIAL_DATA_CNTL",
	    PDV_SERIAL_DATA_CNTL, BL_PDV_SERIAL_DATA_CNTL);

	PrintRegister(edt_p, "PDV_SERIAL_CNTL2", PDV_SERIAL_CNTL2, NULL);
	PrintRegister(edt_p, "PDV_BRATE", PDV_BRATE, NULL);


	PrintRegister(edt_p, "PDV_BYTESWAP", PDV_BYTESWAP, BL_PDV_BYTESWAP);
	edt_reg_write(edt_p, PDV_REV, 0);
	PrintRegister(edt_p, "PDV_REV", PDV_REV, NULL);
	if ((edt_p->devid == PDVA_ID) || (edt_is_dvfox(edt_p)))
	    PrintRegister(edt_p, "PDV_UTIL2", PDV_UTIL2, BL_PDVA_UTIL2);
	else PrintRegister(edt_p, "PDV_UTIL2", PDV_UTIL2, BL_PDV_UTIL2);
	PrintRegister(edt_p, "PDV_SHIFT", PDV_SHIFT, BL_PDV_SHIFT);
	PrintRegister(edt_p, "PDV_MASK", PDV_MASK, NULL);
	PrintRegister(edt_p, "PDV_ROICTL", PDV_ROICTL, BL_PDV_ROICTL);
	PrintRegister(edt_p, "PDV_REV", PDV_REV, NULL);
	PrintRegister(edt_p, "PDV_XOPT", PDV_XOPT, NULL);
	if (edt_p->devid != PGP_RGB_ID)
	{
	    PrintRegister(edt_p, "PDV_LHS_CONTROL", PDV_LHS_CONTROL, BL_PDV_LHS_CONTROL);
	    PrintRegister(edt_p, "PDV_LHS_DELAY", PDV_LHS_DELAY, NULL);
	    PrintRegister(edt_p, "PDV_LHS_PERIOD", PDV_LHS_PERIOD, NULL);
	    PrintRegister(edt_p, "PDV_LHS_COUNT_LOW", PDV_LHS_COUNT_LOW, NULL);
	    PrintRegister(edt_p, "PDV_LHS_COUNT_HI", PDV_LHS_COUNT_HI, NULL);
	}
	if (edt_is_dvcl(edt_p) || edt_is_dvfox(edt_p) | edt_is_simulator(edt_p))
	{

	    PrintRegister(edt_p, "PDV_CL_DATA_PATH", PDV_CL_DATA_PATH, NULL);
	    PrintRegister(edt_p, "PDV_CL_CFG", PDV_CL_CFG, BL_PDV_CL_CFG);
	    PrintRegister(edt_p, "PDV_CL_CFG2", PDV_CL_CFG2, BL_PDV_CL_CFG2);
	    PrintRegister(edt_p, "PDV_HSKIP", PDV_HSKIP, NULL);
	    PrintRegister(edt_p, "PDV_HACTV", PDV_HACTV, NULL);
	    PrintRegister(edt_p, "PDV_VSKIP", PDV_VSKIP, NULL);
	    PrintRegister(edt_p, "PDV_VACTV", PDV_VACTV, NULL);
	    PrintRegister(edt_p, "LINESPERFRAME", PDV_CL_LINESPERFRAME, NULL);
	    PrintRegister(edt_p, "CLOCKSPERLINE", PDV_CL_PIXELSPERLINE, NULL);
	}

	if (edt_is_simulator(edt_p))
    {
        PrintRegister(edt_p, "CLSIM_CFGA", PDV_CLSIM_CFGA, BL_PDV_CLSIM_CFGA);
        PrintRegister(edt_p, "CLSIM_CFGB", PDV_CLSIM_CFGB, BL_PDV_CLSIM_CFGB);
        PrintRegister(edt_p, "CLSIM_CFGC", PDV_CLSIM_CFGC, BL_PDV_CLSIM_CFGC);
        PrintRegister(edt_p, "CLSIM_EXSYNCDLY", PDV_CLSIM_EXSYNCDLY, NULL);
        PrintRegister(edt_p, "CLSIM_FILLA", PDV_CLSIM_FILLA, NULL);
        PrintRegister(edt_p, "CLSIM_FILLB", PDV_CLSIM_FILLB, NULL);
        PrintRegister(edt_p, "CLSIM_HCNTMAX", PDV_CLSIM_HCNTMAX, NULL);
        PrintRegister(edt_p, "CLSIM_VACTV", PDV_CLSIM_VACTV, NULL);
        PrintRegister(edt_p, "CLSIM_VCNTMAX", PDV_CLSIM_VCNTMAX, NULL);
        PrintRegister(edt_p, "CLSIM_HFVSTART", PDV_CLSIM_HFVSTART, NULL);
        PrintRegister(edt_p, "CLSIM_HFVEND", PDV_CLSIM_HFVEND, NULL);
        PrintRegister(edt_p, "CLSIM_HLVSTART", PDV_CLSIM_HLVSTART, NULL);
        PrintRegister(edt_p, "CLSIM_HLVEND", PDV_CLSIM_HLVEND, NULL);
        PrintRegister(edt_p, "CLSIM_HRVSTART", PDV_CLSIM_HRVSTART, NULL);
        PrintRegister(edt_p, "CLSIM_HRVEND", PDV_CLSIM_HRVEND, NULL);
        PrintRegister(edt_p, "CLSIM_TAP0START", PDV_CLSIM_TAP0START, NULL);
        PrintRegister(edt_p, "CLSIM_TAP0DELTA", PDV_CLSIM_TAP0DELTA, NULL);
        PrintRegister(edt_p, "CLSIM_TAP1START", PDV_CLSIM_TAP1START, NULL);
        PrintRegister(edt_p, "CLSIM_TAP1DELTA", PDV_CLSIM_TAP1DELTA, NULL);
        PrintRegister(edt_p, "CLSIM_TAP2START", PDV_CLSIM_TAP2START, NULL);
        PrintRegister(edt_p, "CLSIM_TAP2DELTA", PDV_CLSIM_TAP2DELTA, NULL);
        PrintRegister(edt_p, "CLSIM_TAP3START", PDV_CLSIM_TAP3START, NULL);
        PrintRegister(edt_p, "CLSIM_TAP3DELTA", PDV_CLSIM_TAP3DELTA, NULL);
    }

    if (edt_is_pcie_dvfox(edt_p))
    {
	    PrintRegister(edt_p, "PDV_CL_MGTSPEED", PDV_CL_MGTSPEED, BL_PDV_CL_MGTSPEED);
    }

	{
	    edt_bitpath pathbuf ;
	    edt_get_bitpath(edt_p, pathbuf, sizeof(edt_bitpath)) ;
	    printf("Interface bitfile   : %s\n", pathbuf) ;
	}

	{
	    int i = 0;
	    edt_ioctl(edt_p,EDTG_FVAL_DONE,&i);
	    if (i)
		printf("Fval Done enabled\n");
	}

	break;

    case PCD20_ID:
    case PCD40_ID:
    case PCD60_ID:
    case PCDA_ID:
    case PCDCL_ID:
    case PCDA16_ID:
    case PE4CDA16_ID:
    case PGP20_ID:
    case PGP40_ID:
    case PGP60_ID:
    case PGP_THARAS_ID:
    case PSS4_ID:
    case PSS16_ID:
    case PGS4_ID:
    case PGS16_ID:
    case PE8LX16_LS_ID:
    case PE8LX16_ID:
    case PE16G3_OCTEON3_ID:
    case WSU1_ID:
    case SNAP1_ID:
    case PE8G3S5_ID:
    case PE8G3A5_ID:
    case PE8G3KU_ID:
    case PE8LX32_ID:
    case PE8G2V7_ID:
    case PE4BL_RADIO_ID:
    case PE4BL_TXLFRADIO_ID:
    case PE4BL_RXLFRADIO_ID:
    case PE8BL_NIC_ID:
    case PE4AMC16_ID:
    case PCD_16_ID:
    case PE8BL_WBDSP_ID:
    case PE1BL_WBADC_ID:

	PrintRegister(edt_p, "DMA_CFG", EDT_DMA_CFG, NULL);
	cfg = edt_reg_read(edt_p, EDT_DMA_CFG);
	if (cfg & EDT_FIFO_FLG)
	{
	    fifocnt = ((cfg & EDT_FIFO_CNT) >> EDT_FIFO_SHIFT) + 1;
	    fifocnt *= 4;
	}
	else
	    fifocnt = 0;
	printf("       fifocnt             : %d\n", fifocnt);
	PrintRegister(edt_p, "DMA_STATUS", EDT_DMA_STATUS, NULL);
	PrintRegister(edt_p, "PCD_CMD", PCD_CMD, NULL);
	PrintRegister(edt_p, "PCD_DATA_PATH_STAT",
	    PCD_DATA_PATH_STAT, NULL);
	PrintRegister(edt_p, "PCD_FUNCT", PCD_FUNCT, NULL);
	PrintRegister(edt_p, "PCD_STAT", PCD_STAT, NULL);
	PrintRegister(edt_p, "PCD_STAT_POLARITY",
	    PCD_STAT_POLARITY, NULL);
	PrintRegister(edt_p, "PCD_OPTION", PCD_OPTION, NULL);
	PrintRegister(edt_p, "PCD_DIRA", PCD_DIRA, NULL);
	PrintRegister(edt_p, "PCD_DIRB", PCD_DIRB, NULL);
	PrintRegister(edt_p, "PCD_CONFIG", PCD_CONFIG, NULL);
	PrintRegister(edt_p, "PCD_CYCLE_CNT", PCD_CYCLE_CNT, NULL);
	PrintRegister(edt_p, "EDT_REF_SCALE", EDT_REF_SCALE, NULL);
	PrintRegister(edt_p, "EDT_OUT_SCALE", EDT_OUT_SCALE, NULL);
	PrintRegister(edt_p, "PCD_BYTESWAP", PDV_BYTESWAP, BL_PDV_BYTESWAP);
	if  (edt_p->devid == PCD_16_ID || edt_p->devid == PSS16_ID || edt_p->devid == PGS16_ID )
	{
	    PrintRegister(edt_p, "16_CHEN", SSD16_CHEN, NULL);
	    PrintRegister(edt_p, "16_CHDIR", SSD16_CHDIR, NULL);
	    PrintRegister(edt_p, "16_CHEDGE", SSD16_CHEDGE, NULL);
	    PrintRegister(edt_p, "16_LSB", SSD16_LSB, NULL);
	    PrintRegister(edt_p, "16_UNDER", SSD16_UNDER, NULL);
	    PrintRegister(edt_p, "16_OVER", SSD16_OVER, NULL);
	}
	{
	    edt_bitpath pathbuf ;
	    edt_get_bitpath(edt_p, pathbuf, sizeof(edt_bitpath)) ;
	    printf("Interface bitfile   : %s\n", pathbuf) ;
	}
	break;
    case P16D_ID:
	PrintRegister(edt_p, "P16_CONFIG", P16_CONFIG, NULL);
	PrintRegister(edt_p, "P16_COMMAND", P16_COMMAND, NULL);
	PrintRegister(edt_p, "P16_STATUS", P16_STATUS, NULL);
	PrintRegister(edt_p, "P16_DATA", P16_DATA, NULL);
	break;
    case P11W_ID:
	PrintRegister(edt_p, "P11_CONFIG", P11_CONFIG, NULL);
	PrintRegister(edt_p, "P11_COMMAND", P11_COMMAND, NULL);
	PrintRegister(edt_p, "P11_STATUS", P11_STATUS, NULL);
	PrintRegister(edt_p, "P11_DATA", P11_DATA, NULL);
	PrintRegister(edt_p, "P11_COUNT", P11_COUNT, NULL);
	break;
    }

    {
	char flags[80];
	int mask = edt_get_intr_mask(edt_p);
	printf("Interrupt mask: ");
	GetFlagsString(flags, BL_DMA_INTCFG_ONLY, mask);
	printf(" %s\n", flags);
    }

    printf("\n");

	
}
