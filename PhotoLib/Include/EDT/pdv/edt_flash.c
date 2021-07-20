/**
 * Copyright (c) 1997-2018 Engineering Design Team (EDT), Inc.
 * All rights reserved. This file is subject to the terms
 * and conditions of the EULA defined at edt.com/terms-of-use.
 *
 * Technical Contact: <tech@edt.com>
 */

/*
 * @file edt_flash.c
 *
 * FPGA header read / write utilities
 *
 * New version of edt_x_ (xilinx) subroutines, and a replacement thereof, since we've
 * long since expanded beyond Xilinx to Altera, etc. And also it's more about the
 * flash PROMs anyway. edt_xilinx.c still exists but is being replaced in EDT libraries
 * in favor of these subroutines. Since edt_x_anything() probably shouldn't be getting
 * called by customer propgrams, this should be okay, but if not they can go back and
 * add edt_xilinx.c to their projects.
 */

#include "edtdef.h"

#ifdef _KERNEL

#include "edtdrv.h"

#define EdtDev Edt_Dev

#ifdef _NT_DRIVER_

#define edt_strtol strtol

#endif

#else

#include "edtinc.h"
#include "pciload.h"

/* for driver builds */
#define edt_set edt_reg_write
#define edt_get edt_reg_read
#define edt_delay edt_msleep

#define EDTPRINTF(a, b, c)     \
  if (edt_get_verbosity() > b) \
  {                            \
    printf c;                  \
  }

#define edt_strtol strtol

#endif

#ifdef DOXYGEN_SHOW_UNDOC
/**
 * @weakgroup edt_undoc
 * @{
 */
#endif

/* always use ida_() for new stuff. All the rest should be obsoleted by that
 * one but... to change them would mean a regression testing nightmare
 */
static u_int ida_4013e(void *dmy, int sector);
static u_int ida_4013xla(void *dmy, int sector);
static u_int ida_4028xla(void *dmy, int sector); /* and xc2s150 */
static u_int ida_xc2s100(void *dmy, int sector); /* and 200 and 300e */
static u_int ida_xc3s1200e(void *dmy, int sector);
static u_int ida_bt8(void *edt_p, int dmy);
static u_int ida_f16(void *edt_p, int dmy);
static u_int ida_micron(void *edt_p, int dmy);
static u_int get_fpga_info_index_bt8(EdtDev *edt_p);

void mic_reset(EdtDev *edt_p);
static u_int mic_write(EdtDev *edt_p, u_int val);
static u_int mic_read(EdtDev *edt_p);
static u_int mic_read_flash(EdtDev *edt_p, u_int command, u_int address, u_char *read_bytes,
                            u_int read_num_bytes);
static u_int mic_write_flash(EdtDev *edt_p, u_int command, u_int address, u_char *write_bytes,
                             u_int write_num_bytes, u_int time_min, u_int timeout_loops,
                             u_int timeout_sleep);
static u_short mic_get_status(EdtDev *edt_p);
static u_int mic_write_enable(EdtDev *edt_p);
static u_int mic_write_disable(EdtDev *edt_p);
static u_int mic_subsector_erase(EdtDev *edt_p, u_int address);
static u_int mic_sector_erase(EdtDev *edt_p, u_int address);
static u_int mic_page_program(EdtDev *edt_p, u_int address, u_char *write_bytes,
                              u_int write_num_bytes);
static u_int mic_write_status_register(EdtDev *edt_p, u_char status_val);
static u_int mic_enter_4_byte_mode(EdtDev *edt_p);
static u_int mic_exit_4_byte_mode(EdtDev *edt_p);
static u_char mic_read_ext_addr_register(EdtDev *edt_p);
static u_int mic_write_ext_addr_register(EdtDev *edt_p, u_char config_val);
static uint_t mic_rd_prog_reg(EdtDev *edt_p, u_short reg);
static void mic_wr_prog_reg(EdtDev *edt_p, uint_t reg, uint_t val);

static void mic_set_device(EdtDev *edt_p, uint_t sector);
static uint_t mic_get_pal_id(EdtDev *edt_p);
static uint_t mic_get_jumper(EdtDev *edt_p);
int mic_get_manufacturer_info(EdtDev *edt_p);

int edt_mic_is_protected(EdtDev *edt_p);
void edt_get_sns(EdtDev *edt_p, char *esn, char *osn);
const char *edt_get_fpga_mfg(EdtDev *edt_p);
int edt_flash_prom_detect(EdtDev *edt_p, u_short *stat);
void edt_read_prom_data(EdtDev *edt_p, int promcode, int segment, EdtPromData *pdata);
void edt_flash_byte_program(EdtDev *edt_p, u_int addr, u_char data, int ftype);
void edt_flash_block_program(EdtDev *edt_p, u_int addr, u_char *data, u_int nbytes, int ftype);
u_char *edt_flash_block_read(EdtDev *edt_p, u_int addr, u_char *buf, u_int size, int ftype);
int edt_get_max_promcode();

/* get from edt_get_prominfo */
#define PROMCODE_TBL_SIZE 64
/* clang-format off */
static Edt_prominfo EPinfo[PROMCODE_TBL_SIZE] =
{ /*                                                                                                                  (Note: block = segment; for 16-bit devs, sect. size is in words)*/
    /* Comment              idx     FPGA                Flash prom                       status bits          prog        magic         sector    sects/ num    dflt  id_addr()     boot block */
    /*                              name                name                             id    xid     bus    type        index         size      block  blocks block (ptr)         #0  #1     */
    /* DUMMY                 0 */  {"unknown",          "unknown",                       0xff, 0xff,  "na",   0,          0,            0,         0,    0,     0,    NULL,         -1, -1},
    /* AMD_4013E             1 */  {"4013e",            "AMD 29F010",                    0xff, 0xff,  "PCI",  FTYPE_X,    XilinxMagic,  0x004000,  8,    1,     0,    ida_4013e,     1, -1},
    /* AMD_4013XLA           2 */  {"4013xla",          "AMD 29F010",                    0xff, 0xff,  "PCI",  FTYPE_X,    XilinxMagic,  0x010000,  1,    3,     1,    ida_4013xla,   1,  3},
    /* AMD_4028XLA           3 */  {"4028xla",          "AMD 29F010",                    0x00, 0xff,  "PCI",  FTYPE_BT,   XilinxMagic,  0x010000,  2,    4,     2,    ida_4028xla,   2,  3},
    /* AMD_XC2S150           4 */  {"xc2s150",          "AMD 29LV040B",                  0x01, 0xff,  "PCI",  FTYPE_BT,   XilinxMagic,  0x010000,  2,    4,     2,    ida_4028xla,   2,  3},
    /* AMD_XC2S200_4M        5 */  {"xc2s200",          "AMD 29LV040B 4MB",              0x02, 0xff,  "PCI",  FTYPE_BT,   XilinxMagic,  0x010000,  4,    2,     2,    ida_xc2s100,   0,  1},
    /* AMD_XC2S200_8M        6 */  {"xc2s200",          "AMD 29LV081B 8MB",              0x03, 0xff,  "PCI",  FTYPE_BT,   XilinxMagic,  0x010000,  4,    4,     2,    ida_xc2s100,   2,  3},
    /* AMD_XC2S100_8M        7 */  {"xc2s100",          "AMD 29LV081B 8MB",              0x04, 0xff,  "PCI",  FTYPE_BT,   XilinxMagic,  0x010000,  4,    4,     2,    ida_xc2s100,   2,  3},
    /* AMD_XC2S300E          8 */  {"xc2s300e",         "AMD 29LV081B 8MB",              0xff, 0xff,  "PCI",  FTYPE_LTX,  XilinxMagic,  0x010000,  4,    4,     2,    ida_xc2s100,  -1, -1},
    /* SPI_XC3S1200E         9 */  {"xc3s1200e",        "SPI W25P16",                    0x05, 0xff,  "PCIe", FTYPE_SPI,  XilinxMagic,  0x100000,  32,   1,     0,    ida_xc3s1200e, 0, -1},
    /* AMD_XC5VLX30T        10 */  {"xc5vlx30t",        "AMD S29GL064N",                 0x06, 0xff,  "PCIe", FTYPE_BT2,  XilinxMagic,  0x010000,  32,   4,     3,    ida_bt8,       3, -1},
    /* AMD_XC5VLX50T        11 */  {"xc5vlx50t",        "AMD S29GL064N",                 0x07, 0xff,  "PCIe", FTYPE_BT2,  XilinxMagic,  0x010000,  32,   4,     3,    ida_bt8,       3, -1},
    /* AMD_EP2SGX30D        12 */  {"ep2sgx30d",        "AMD S29GL128N",                 0x15, 0xff,  "PCIe", FTYPE_BT2,  AlteraMagic,  0x010000,  32,   4,     3,    ida_bt8,       3, -1},
    /* AMD_XC5VLX70T        13 */  {"xc5vlx70t",        "AMD S29GL064N",                 0x08, 0xff,  "PCIe", FTYPE_BT2,  XilinxMagic,  0x010000,  64,   1,     0,    ida_bt8,       0, -1},
    /* AMD_XC5VLX30T_A      14 */  {"xc5vlx30t",        "AMD S29GL064N (A)",             0x09, 0xff,  "PCIe", FTYPE_BT2,  XilinxMagic,  0x010000,  32,   4,     3,    ida_bt8,       3, -1},
    /* AMD_XC6SLX45         15 */  {"xc6slx45",         "AMD S29GL064N",                 0x0a, 0xff,  "PC104",FTYPE_BT2,  XilinxMagic,  0x010000,  32,   4,     3,    ida_bt8,       3, -1},
    /* AMD_EP2SGX30D_A      16 */  {"ep2sgx30d",        "AMD S29GL256P",                 0x0b, 0xff,  "PCIe", FTYPE_BT2,  AlteraMagic,  0x020000,  32,   4,     3,    ida_bt8,       3, -1},
    /* AMD_EP2AGX45D        17 */  {"ep2agx45d",        "AMD S29GL256S",                 0x11, 0xff,  "PCIe", FTYPE_F16,  AlteraMagic,  0x010000,  64,   4,     3,    ida_f16,       3, -1},
    /* AMD_5SGXMA3K2F40C3   18 */  {"5sgxma3k2f40c3",   "AMD S29GL01GS",                 0x0c, 0x03,  "PCIe", FTYPE_F16,  AlteraMagic,  0x010000,  512,  2,     1,    ida_f16,       1, -1},
    /* AMD_5SGXMA5K2F40C3   19 */  {"5sgxma5k2f40c3",   "AMD S29GL01GS",                 0x0c, 0x04,  "PCIe", FTYPE_F16,  AlteraMagic,  0x010000,  512,  2,     1,    ida_f16,       1, -1},
    /* AMD_5SGXMA7K2F40C3   20 */  {"5sgxma7k2f40c3",   "AMD S29GL01GS",                 0x0c, 0x02,  "PCIe", FTYPE_F16,  AlteraMagic,  0x010000,  512,  2,     1,    ida_f16,       1, -1},
    /* AMD_5SGXMA8K2H40C3N  21 */  {"5sgxma8k2h40c3n",  "AMD S29GL01GS",                 0x0c, 0x05,  "PCIe", FTYPE_F16,  AlteraMagic,  0x010000,  512,  2,     1,    ida_f16,       1, -1},
    /* AMD_5SGXMA7K1F40C2   22 */  {"5sgxma7k1f40c2",   "AMD S29GL01GS",                 0x0c, 0x01,  "PCIe", FTYPE_F16,  AlteraMagic,  0x010000,  512,  2,     1,    ida_f16,       1, -1},
    /* AMD_5SGXEA2K1F40C2ES 23 */  {"5sgxea2k1f40c2es", "AMD S29GL01GS",                 0x0c, 0x00,  "PCIe", FTYPE_F16,  AlteraMagic,  0x010000,  512,  2,     1,    ida_f16,       1, -1},
    /* AMD_5SGXMA9K2H40C2   24 */  {"5sgxma9k2h40c2",   "AMD S29GL01GS",                 0x0c, 0x06,  "PCIe", FTYPE_F16,  AlteraMagic,  0x010000,  512,  2,     1,    ida_f16,       1, -1},
    /* MIC_N25Q064A13ESE40G 25 */  {"5cgtfd5c5",        "Micron N25Q064A13ESE40G",       0x0d, 0x01,  "PCIe", FTYPE_MIC,  Altera2Magic, 0x010000,  128,  1,     0,    ida_micron,    0, -1},
    /* AMD_5AGZME1          26 */  {"5agzme1e2h29c3",   "AMD S29GL01GS",                 0x0c, 0x07,  "PCIe", FTYPE_F16A, AlteraMagic,  0x010000,  256,  2,     1,    ida_f16,       1,  0},
    /* MIC_N25Q256A13EF840  27 */  {"5agzme3h2f35c3n",  "Micron N25Q256A13EF840",        0x0e, 0x00,  "PCIe", FTYPE_MIC,  Altera2Magic, 0x010000,  512,  1,     0,    ida_micron,    0, -1},
    /* MIC_N25Q256A13EF840  28 */  {"5agzme5h2f35c3n",  "Micron N25Q256A13EF840",        0x0e, 0x01,  "PCIe", FTYPE_MIC,  Altera2Magic, 0x010000,  512,  1,     0,    ida_micron,    0, -1},
    /* MIC_N25Q256A13EF840  29 */  {"5agzme7h2f35c3n",  "Micron N25Q256A13EF840",        0x0e, 0x02,  "PCIe", FTYPE_MIC,  Altera2Magic, 0x010000,  512,  1,     0,    ida_micron,    0, -1},
    /* MIC_N25Q256A13EF840  30 */  {"5agzme3h2f35c3n",  "Micron N25Q256A13EF840",        0x0e, 0x04,  "PCIe", FTYPE_MIC,  Altera2Magic, 0x010000,  512,  1,     0,    ida_micron,    0, -1},
    /* MIC_N25Q256A13EF840  31 */  {"5agzme5h2f35c3n",  "Micron N25Q256A13EF840",        0x0e, 0x05,  "PCIe", FTYPE_MIC,  Altera2Magic, 0x010000,  512,  1,     0,    ida_micron,    0, -1},
    /* MIC_N25Q256A13EF840  32 */  {"5agzme7h2f35c3n",  "Micron N25Q256A13EF840",        0x0e, 0x06,  "PCIe", FTYPE_MIC,  Altera2Magic, 0x010000,  512,  1,     0,    ida_micron,    0, -1},
    /* AMD_5AGZME1          33 */  {"5agzme1e2h29c3",   "AMD S29GL01GS",                 0x0c, 0x08,  "PCIe", FTYPE_F16A, AlteraMagic,  0x010000,  256,  2,     1,    ida_f16,       1,  0},
    /* AMD_S29GL512S        34 */  {"xc7k160t",         "AMD S29GL512S",                 0x0d, 0x02,  "PCIe", FTYPE_F16A, AlteraMagic,  0x010000,  256,  2,     1,    ida_f16,       1,  0},
    /* AMD_S29GL512S        35 */  {"xc7k325t",         "AMD S29GL512S",                 0x0d, 0x03,  "PCIe", FTYPE_F16A, AlteraMagic,  0x010000,  256,  2,     1,    ida_f16,       1,  0},
    /* AMD_S29GL512S        36 */  {"xc7k410t",         "AMD S29GL512S",                 0x0d, 0x04,  "PCIe", FTYPE_F16A, AlteraMagic,  0x010000,  256,  2,     1,    ida_f16,       1,  0},
    /* MIC_MT25QL256ABA1EW9 37 */  {"5agzme1e3h29c4n",  "Micron MT25QL256ABA1EW9-0SIT",  0x0e, 0x07,  "PCIe", FTYPE_MIC,  Altera2Magic, 0x010000,  512,  1,     0,    ida_micron,    0,  -1},

    /* END OF LIST             */  {"",                 "",                        0x00, 0x00,  "",     0,          0,                   0,    0,  0,     0,    NULL,         -1, -1},
};
/* clang-format on */

static struct
{
  u_int mid;
  u_int devid;
  u_int prom;
} older_devices[] = {
    {0x1, 0x20, AMD_4013E}, {0x1, 0x4f, AMD_4013XLA}, {0x20, 0xe3, AMD_4013XLA}, {0x00, 0x00, 0}};

/* shorthand debug level */
#define DEBUG1 EDTLIB_MSG_INFO_1
#define DEBUG2 EDTLIB_MSG_INFO_2

/*
 * command bits for the 4028xla boot controller
 */
#define BT_EN_READ 0x8000

/*
 * bit to set in all prom addresses to enable the upper 2 bits of prom
 * address after we have read the status (jumper and 5/3.3v)
 */
#define EN_HIGH_ADD 0x00800000

static int needswap = 0;
static char micron_chip_specific_info[256];
static int micron_4byte_addressing = 0;
static u_int micron_flash_size = 0;
static char *micron_jedec_flash_type = "";

static void f16a_print16(EdtDev *edt_p, u_int addr);
static u_int f16a_wr_cmd(EdtDev *edt_p, u_int cmd, u_int val, u_int wait_limit);
static u_int f16a_rd_cmd(EdtDev *edt_p, u_int cmd);
static u_int f16a_sector_erase(EdtDev *edt_p, u_int sector_addr);
static void f16a_rd_flash_block(EdtDev *edt_p, u_int addr, u_char *buffer, u_int byte_count);
static int f16a_wr_flash(EdtDev *edt_p, int addr, u_char *buffer, int length);
static void f16a_dump_flash(EdtDev *edt_p, u_int waddr, u_int size);
static int f16a_reset(EdtDev *edt_p);

static void f16_print16(EdtDev *edt_p, u_int addr);
static u_int f16_wr_cmd(EdtDev *edt_p, u_int cmd, u_int val, u_int wait_limit);
static u_int f16_rd_cmd(EdtDev *edt_p, u_int cmd);
static u_int f16_sector_erase(EdtDev *edt_p, u_int sector_addr);
static int f16_wr_flash(EdtDev *edt_p, u_int addr, u_char *buffer, int nbytes);
static void f16_rd_flash_block(EdtDev *edt_p, u_int addr, u_char *buffer, u_int byte_count);
static int f16_reset(EdtDev *edt_p);

static void spi_read(EdtDev *edt_p, u_int addr, u_char *rbuf, int rcnt);
static int spi_reset(EdtDev *edt_p);
static void spiprim(EdtDev *edt_p, int opcode, u_int wbytes, u_int rbytes, u_char *wbuf,
                    u_char *rbuf);
static void spi_xwr(EdtDev *edt_p, int val);
static int spi_xrd(EdtDev *edt_p);
static void spi_xw(EdtDev *edt_p, u_int addr, u_int data);
static u_int spi_xr(EdtDev *edt_p, u_int addr);
static int spi_block_program(EdtDev *edt_p, int addr0, u_int size, u_char *spibuf, int verify_only);
static void spi_print16(EdtDev *edt_p, u_int addr);

static void bt_write(EdtDev *edt_p, u_int addr, u_char val);
static u_char bt_read(EdtDev *edt_p, u_int addr);
static void bt_reset(EdtDev *edt_p);
static u_char bt_readstat(EdtDev *edt_p);

static void bt2_reset(EdtDev *edt_p);

static void x_write(EdtDev *edt_p, u_int addr, u_char val);
static u_char x_read(EdtDev *edt_p, u_int addr);
static void x_reset(EdtDev *edt_p);
static u_char x_readstat(EdtDev *edt_p);
static void x_print16(EdtDev *edt_p, u_int addr);

static void tst_write(EdtDev *edt_p, uint_t desc, uint_t val);
static uint_t tst_read(EdtDev *edt_p, uint_t desc);
static void tst_pflash(EdtDev *edt_p, uint_t addr, uint_t val);

static void program_info_str(EdtDev *edt_p, u_int addr, char *str, char *tag, u_int size,
                             int ftype);

/**
 * Returns the pciload device information structure for the specific flash PROM code given.
 *
 * @param promcode the flash PROM code index for the device's #EPinfo table entry -- see AMD_ etc.
 * defines in libedt.h, and #edt_flash_prom_detect.
 * @return pointer to #Edt_prominfo struct.
 */
Edt_prominfo *edt_get_prominfo(int promcode)
{
  return (promcode <= edt_get_max_promcode() ? &EPinfo[promcode] : &EPinfo[0]);
}

/**
 * Returns in string form (character array) the FPGA manufacturer.
 *
 * @param edt_p pointer to edt device structure returned by #edt_open
 * @return character array containing the name of the FPGA type; one of Xilinx, Altera, or empty
 * string if unknown.
 */
const char *edt_get_fpga_mfg(EdtDev *edt_p)
{
  int promcode = edt_flash_prom_detect(edt_p, NULL);

  switch (EPinfo[promcode].magic)
  {
    case AlteraMagic:
    case Altera2Magic:
      return "Altera";
      break;
    case XilinxMagic:
      return "Xilinx";
      break;

    default:
      return "";
      break;
  }
}

/*
 * get highest numbered known promcode
 */
int edt_get_max_promcode()
{
  int i;
  for (i = 0; i < PROMCODE_TBL_SIZE; i++)
    if (!EPinfo[i].fpga[0])
      return i - 1;
  return PROMCODE_TBL_SIZE - 1; /* should never happen */
}

  /* return true if machine is little_endian */

#ifdef _KERNEL

static int edt_little_endian()
{
  u_short test;
  u_char *byte_p;

  byte_p = (u_char *)&test;
  *byte_p++ = 0x11;
  *byte_p = 0x22;
  if (test == 0x1122)
  {
    return (0);
  }
  else
  {
    return (1);
  }
}

#endif

#define DQ7 0x80
#define DQ5 0x20
#define DQ3 0x08

static int maxloops = 0;
static int debug_fast = 0;
static int do_fast = 0;
static int force_slow = 0;

int edt_flash_get_debug_fast()
{
  return (debug_fast);
}

int edt_flash_set_debug_fast(int val)
{
  (debug_fast = val);
  return val;
}

int edt_flash_get_do_fast()
{
  return (do_fast);
}

void edt_flash_set_do_fast(int val)
{
  do_fast = val;
}

int edt_flash_get_force_slow()
{
  return (force_slow);
}

void edt_flash_set_force_slow(int val)
{
  force_slow = val;
}

/* what are these? */

volatile caddr_t dmyaddr = 0;
volatile u_int *cfgaddr;
volatile u_int *tstaddr;

u_int swap32(u_int val)
{
  /* already set hardware swap if LTX hub */
  return (((val & 0x000000ff) << 24) | ((val & 0x0000ff00) << 8) | ((val & 0x00ff0000) >> 8) |
          ((val & 0xff000000) >> 24));
}

/*
 * print 16 bytes at the address passed read from 4028xla boot controller
 */
static void bt_print16(EdtDev *edt_p, u_int addr)
{
  int i;

  EDTPRINTF(edt_p, 2, ("%08x ", addr));
  for (i = 1; i < 16; i++)
  {
    EDTPRINTF(edt_p, 2, (" %02x", bt_read(edt_p, addr)));
    addr++;
  }
  EDTPRINTF(edt_p, 2, ("\n"));
}

void edt_flash_print16(EdtDev *edt_p, u_int addr, int ftype)
{
  switch (ftype)
  {
    case FTYPE_F16A:
      f16a_print16(edt_p, (u_int)addr);
      break;

    case FTYPE_F16:
      f16_print16(edt_p, (u_int)addr);
      break;

    case FTYPE_MIC:
      /* mic_print16(edt_p, addr); no such beast */
      break;

    case FTYPE_SPI:
      spi_print16(edt_p, (u_int)addr);
      break;

    case FTYPE_BT:
    case FTYPE_BT2:
      bt_print16(edt_p, addr);
      break;

    case FTYPE_X:
      x_print16(edt_p, addr);
      break;

    default:
      break;
  }
}

int edt_flash_get_ftype(EdtDev *edt_p)
{
  int promcode = edt_flash_prom_detect(edt_p, NULL);
  Edt_prominfo *ep = edt_get_prominfo(promcode);
  return ep->ftype;
}

u_char edt_flash_read8(EdtDev *edt_p, u_int addr, int ftype)
{
  u_char val;

  switch (ftype)
  {
    case FTYPE_F16:  /* NA 16-bit devices */
    case FTYPE_F16A: /* NA 16-bit devices */
      return 0;

    case FTYPE_MIC:
      return 0; /* STUB (not needed?) */

    case FTYPE_SPI:
      spi_read(edt_p, addr, &val, 1);
      return val;

    case FTYPE_BT:
    case FTYPE_BT2:
      return bt_read(edt_p, addr);

    case FTYPE_X:
      return x_read(edt_p, addr);
  }

  return 0;
}

/*
 * sector erase for AMD 29LV0XXX
 */
static int bt_sector_erase(EdtDev *edt_p, u_int sector, u_int sec_size)
{
  u_int addr;
  int done = 0;
  u_int loops = 0;
  const u_int lots = 32768, too_many = 65535;
  u_char val;

  addr = sector * sec_size;
  bt_write(edt_p, 0x5555, 0xaa);
  bt_write(edt_p, 0x2aaa, 0x55);
  bt_write(edt_p, 0x5555, 0x80);
  bt_write(edt_p, 0x5555, 0xaa);
  bt_write(edt_p, 0x2aaa, 0x55);
  bt_write(edt_p, addr, 0x30);
  done = 0;
  while (!done)
  {
    val = bt_read(edt_p, addr);
    loops++;
    if (val & DQ7)
    {
      done = 1;
    }
    else if (loops > lots)
      edt_delay(1);

    else if (loops > too_many)
    {
      return -1;
    }
  }
  return 0;
}

/*
 * sector erase for AMD S29GL64N, S29GL128N
 */

static int bt2_sector_erase(EdtDev *edt_p, u_int sector, u_int sec_size)
{
  int result = 0;
#ifndef _KERNEL
  u_int addr;
  int done = 0;
  u_char val;
  double t = edt_timestamp();
  double elapsed;

  addr = sector * sec_size;
  bt_write(edt_p, 0xaaa, 0xaa);
  bt_write(edt_p, 0x555, 0x55);
  bt_write(edt_p, 0xaaa, 0x80);
  bt_write(edt_p, 0xaaa, 0xaa);
  bt_write(edt_p, 0x555, 0x55);
  bt_write(edt_p, addr, 0x30);
  done = 0;
  while (!done)
  {
    edt_delay(1);
    val = bt_read(edt_p, addr);
    elapsed = edt_timestamp() - t;

    if (val & DQ7)
    {
      done = 1;
    }
    if (elapsed > 3.0)
    {
      done = 1;
      result = 1;
    }
  }
#endif

  return (result);
}

static int xsector_erase(EdtDev *edt_p, u_int sector, u_int sec_size)
{
  u_int addr;
  int done = 0;
  int loops = 0;
  u_char val;

  addr = sector * sec_size;
  x_write(edt_p, 0x5555, 0xaa);
  x_write(edt_p, 0x2aaa, 0x55);
  x_write(edt_p, 0x5555, 0x80);
  x_write(edt_p, 0x5555, 0xaa);
  x_write(edt_p, 0x2aaa, 0x55);
  x_write(edt_p, addr, 0x30);
  done = 0;
  while (!done)
  {
    val = x_read(edt_p, addr);
    loops++;
    if (val & DQ7)
    {
      done = 1;
      break;
    }
  }
  return 0;
}

/*
 * for 4028xla, xc2s100, xc2s200
 */
static int bt_byte_program(EdtDev *edt_p, int ftype, u_int addr, u_char data)
{
  u_char val;
  int done = 0;
  int loops = 0;

  bt_write(edt_p, 0x5555, 0xaa);
  bt_write(edt_p, 0x2aaa, 0x55);
  bt_write(edt_p, 0x5555, 0xa0);
  bt_write(edt_p, addr, data);
  tst_write(edt_p, EDT_FLASHROM_DATA, BT_RD_ROM);

  while (!done)
  {
    val = (u_char)tst_read(edt_p, EDT_FLASHROM_DATA);

    if (val == data)
      done = 1;
    if (loops > 1000)
    {
      EDTPRINTF(edt_p, 1, ("bt_byte_program: failed on %x data %x val %x\n", addr, data, val));
      bt_reset(edt_p);
      bt_write(edt_p, 0x555, 0xaa);
      bt_write(edt_p, 0x2aa, 0x55);
      bt_write(edt_p, 0x555, 0xa0);
      bt_write(edt_p, addr, data);
      tst_write(edt_p, EDT_FLASHROM_DATA, BT_RD_ROM);
      loops = 0;
    }
    loops++;
  }
  if (loops > maxloops)
  {
    maxloops = loops;
  }
  if (done)
    return (0);
  else
    return (addr);
}

/*
 * for xc5vlx30t, xc5vlx50t, ep2sgx30d
 */
static int bt2_byte_program(EdtDev *edt_p, int ftype, u_int addr, u_char data)
{
  u_char val;
  int done = 0;
  int loops = 0;
  int failures = 0;

  bt_write(edt_p, 0xaaa, 0xaa);
  bt_write(edt_p, 0x555, 0x55);
  bt_write(edt_p, 0xaaa, 0xa0);
  bt_write(edt_p, addr, data);
  tst_write(edt_p, EDT_FLASHROM_DATA, BT_RD_ROM);

  while (!done)
  {
    val = (u_char)tst_read(edt_p, EDT_FLASHROM_DATA);

    if (val == data)
      done = 1;
    if (loops > 1000)
    {
      bt2_reset(edt_p);
      bt_write(edt_p, 0xaaa, 0xaa);
      bt_write(edt_p, 0x555, 0x55);
      bt_write(edt_p, 0xaaa, 0xa0);
      bt_write(edt_p, addr, data);
      tst_write(edt_p, EDT_FLASHROM_DATA, BT_RD_ROM);
      loops = 0;
      failures++;
    }
    loops++;

    if (failures > 5)
    {
      EDTPRINTF(edt_p, 0, ("bt2_byte_program: failed on %x data %x val %x\n", addr, data, val));
      return (1);
    }
  }
  if (loops > maxloops)
  {
    maxloops = loops;
  }
  if (done)
    return (0);
  else
    return (addr);
}

/*
 * for 4013e and others
 */

/* return 0 for success */
static int xbyte_program(EdtDev *edt_p, u_int addr, u_char data)
{
  u_char val;
  int done = 0;
  int loops = 0;

  tst_pflash(edt_p, addr, data);
  while (!done)
  {
    loops++;
    val = x_read(edt_p, addr);
    if (val == data)
      done = 1;
    if (loops > 100)
    {
      EDTPRINTF(edt_p, 1, ("xbyte_program: failed on %x data %x val %x\n", addr, data, val));
      loops = 0;
    }
  }
  if (loops > maxloops)
  {
    if (loops > 10)
    {
      EDTPRINTF(edt_p, 1, ("maxloops %x %d\n", addr, loops));
    }
    maxloops = loops;
  }
  if (done)
    return (0);
  else
    return (addr);
}

/*
 * Program xilinx: 1 byte
 */
void edt_flash_byte_program(EdtDev *edt_p, u_int addr, u_char data, int ftype)
{
  u_char buf[2];

  switch (ftype)
  {
    case FTYPE_F16A:
    case FTYPE_F16:
      /* byte writes to 16-bit na */
      break;

    case FTYPE_MIC:
      /*  byte writes to MIC part na? or stub */
      break;

    case FTYPE_SPI:
      buf[0] = data;
      edt_flash_block_program(edt_p, addr, buf, 1, ftype);

      break;
    case FTYPE_BT:
      bt_byte_program(edt_p, ftype, addr, data);
      break;

    case FTYPE_BT2:
      bt2_byte_program(edt_p, ftype, addr, data);
      break;

    case FTYPE_X:
      xbyte_program(edt_p, addr, data);
      break;
  }
}

u_int edt_flash_writesize(int ftype)
{
  switch (ftype)
  {
    case FTYPE_F16:
    case FTYPE_F16A:
      return 2;
    default:
      return 1;
  }
}

/*
 * Program Xilinx: N bytes starting at addr
 */
void edt_flash_block_program(EdtDev *edt_p, u_int addr, u_char *data, u_int nbytes, int ftype)
{
  int i;
  u_char *dp = data;

  switch (ftype)
  {
    case FTYPE_MIC:
      mic_page_program(edt_p, addr, data, nbytes);
      break;

    case FTYPE_F16A:
      f16a_wr_flash(edt_p, addr, data, nbytes);
      break;

    case FTYPE_F16:
      f16_wr_flash(edt_p, addr, data, nbytes);
      break;

    case FTYPE_SPI:
      spi_block_program(edt_p, addr, nbytes, data, 0);
      break;

    case FTYPE_BT:
      for (i = 0; (u_int)i < nbytes; i++)
        bt_byte_program(edt_p, ftype, addr++, *(dp++));
      break;
    case FTYPE_BT2:
      for (i = 0; (u_int)i < nbytes; i++)
        bt2_byte_program(edt_p, ftype, addr++, *(dp++));
      break;
    case FTYPE_X:
      for (i = 0; (u_int)i < nbytes; i++)
        xbyte_program(edt_p, addr++, *(dp++));
      break;
  }
}

/*
 * Verify Xilinx: N bytes starting at addr
 */
void edt_flash_verify(EdtDev *edt_p, u_int addr, u_char *data, int nbytes, int ftype)
{
  switch (ftype)
  {
    case FTYPE_SPI:
      spi_block_program(edt_p, addr, nbytes, data, 1);
      break;
    default:
      EDTPRINTF(
          edt_p, 1,
          ("Oops! Can't verify type %d FPGA with edt_flash_verify (pciload internal)\n", ftype));
      break;
  }
}

/** Retrieve the board's embedded information string from the PCI xilinx information header.
 *
 * The EDT manufacturer's part numbers is embedded in an unused area of the Xilinx
 * FPGA PROM, and is preserved across reloads (via pciload, hubload, etc.) unless
 * options to overwrite are invoked in one of those utilities. This subroutine
 * retrieves the EDT serial number portion of that information.
 *
 * The data is an ASCII string, with the following colon-separated fields:
 *
 * serial number:part number:clock speed:options:revision:interface xilinx:macaddrs:
 *
 * (To see the information string, run \e pciload with no arguments.)
 *
 * @note Information embedding was implemented in Sept. 2004; boards shipped before
 * that time will yield a string with all NULLS unless the board's FPGA
 * has since been updated with the embedded information.
 *
 * @param edt_p pointer to edt device structure returned by #edt_open
 * @param esn the EDT part number without dashes.
 * @see edt_get_sns, edt_get_osn, edt_parse_devinfo, edt_fmt_pn
 */
void edt_get_esn(EdtDev *edt_p, char *esn)
{
  char dmy[256];
  edt_get_sns(edt_p, esn, dmy);
}

/** Retrieve the board OEM's embedded information string from the PCI xilinx information header.
 *
 * Some OEMs embed part number or other information about the board in an unused area of the
 * Xilinx FPGA PROM. This information is preserved across reloads (via pciload, hubload, etc.)
 * unless options to overwrite are invoked in one of those utilities. This subroutine
 * retrieves the OEM serial number portion of that information.
 *
 * @note Information embedding was implemented in Sept. 2004; boards shipped before
 * that time will yield a string with all NULLS unless the board's FPGA
 * has since been updated with the embedded information.
 *
 * @param edt_p pointer to edt device structure returned by #edt_open
 * @param osn the OEM's part number, if present.
 * @see edt_get_sns, edt_get_esn, edt_fmt_pn
 */
void edt_get_osn(EdtDev *edt_p, char *osn)
{
  char dmy[256];
  edt_get_sns(edt_p, dmy, osn);
}

/**
 * Extract the name of the on-board firmware in the device's FPGA PROM, minus
 * the extension.
 *
 * Fills in the name string field with the name of the device's onboard PROM,
 * minus the voltage descriptor if any, and extension (.ncd).
 * For devices with two voltage sectors to the firmware, two files with
 * _3v.ncd and _5v.ncd extensions will be present; in such cases the _3v and _5v
 * will be stripped off as well.  For example if the device is loaded with
 * "pcidev-10_3v.ncd" and "pcidev-10_5v.ncd", the subroutine will return "pcidev-10".
 *
 * @param edt_p pointer to edt device structure returned by #edt_open
 * @param name FPGA filename
 * @see edt_flash_get_fname_auto
 */
int edt_flash_get_fname(EdtDev *edt_p, char *name)
{
  int promcode;
  char *p;
  char *idstr;
  EdtPromData pdata;
  Edt_prominfo *ep;
  u_short stat;

  promcode = edt_flash_prom_detect(edt_p, &stat);

  ep = edt_get_prominfo(promcode);

  edt_read_prom_data(edt_p, promcode, ep->defaultseg, &pdata);

  idstr = pdata.id;

#if defined(__APPLE__) && defined(_KERNEL)
  if ((p = edt_strrchr(idstr, '.')) != NULL)
    *p = '\0';

  if ((p = edt_strrchr(idstr, '_')) != NULL)
  {
    if ((*(p + 1) == '3' || *(p + 1) == '5') && (*(p + 2) == 'v'))
      *p = '\0';
  }

  if ((p = edt_strrchr(idstr, '-')) != NULL)
  {
    *p = '\0';
  }
#else
  if ((p = strrchr(idstr, '.')) != NULL)
    *p = '\0';

  if ((p = strrchr(idstr, '_')) != NULL)
  {
    if ((*(p + 1) == '3' || *(p + 1) == '5') && (*(p + 2) == 'v'))
      *p = '\0';
  }

  if ((p = strrchr(idstr, '-')) != NULL)
  {
    *p = '\0';
  }

#endif
  strcpy(name, idstr);

  return 0;
}

/**
 * Extract the name of the on-board firmware in the device's FPGA PROM, minus
 * the version and extension.
 *
 * Fills in the name string field with the name of the device's onboard PROM,
 * minus the version, voltage descriptor if any, and extension (.ncd).
 * For devices with two voltage sectors to the firmware, two files with
 * _3v.ncd and _5v.ncd extensions will be present; in such cases _3v and _5v
 * will be stripped off as well.  For example if the device is loaded with
 * "pcidev-10_3v.ncd" and "pcidev-10_5v.ncd", the subroutine will return "pcidev".
 *
 *
 * @param edt_p pointer to edt device structure returned by #edt_open
 * @param name FPGA filename
 * @see edt_flash_get_fname
 */
int edt_flash_get_fname_auto(EdtDev *edt_p, char *name)
{
  Edt_prominfo *ep;
  char *p;
  char *idstr;
  EdtPromData pdata;
  int promcode;
  u_short stat;

  promcode = edt_flash_prom_detect(edt_p, &stat);

  ep = edt_get_prominfo(promcode);
  edt_read_prom_data(edt_p, promcode, ep->defaultseg, &pdata);

  idstr = pdata.id;

  /* strip off trailing .ncd (etc) */
#if defined(__APPLE__) && defined(_KERNEL)
  if ((p = edt_strrchr(idstr, '.')) != NULL)
#else
  if ((p = strrchr(idstr, '.')) != NULL)
#endif
  {
    *p = '\0';

    /* strip off trailing _[35]v */
    if ((strlen(idstr) > 3) && ((strcmp(p - 3, "_3v") == 0) || (strcmp(p - 3, "_5v") == 0)))
      *(p - 3) = '\0';
  }

    /* if present, strip off trailing -xxx (version number) */
#if defined(__APPLE__) && defined(_KERNEL)
  if ((p = edt_strrchr(idstr, '-')) != NULL)
#else
  if ((p = strrchr(idstr, '-')) != NULL)
#endif
    *p = '\0';

  strcpy(name, idstr);

  return 0;
}

/** Retrieve the board's manufacturer and OEM embedded information strings strings from the PCI
 * xilinx information header.
 *
 * Certain information about the board, including manufacturer's part number, serial number, clock
 * speed, Xilinx FPGA, and options, is embedded in an unused area of the Xilinx FPGA PROM at the
 * time of manufacture. This information is preserved across reloads (via pciload, hubload, etc.)
 * unless options overwrite are invoked in the utility. This subroutine retrieves EDT and OEM (if
 * present) information. The data is an ASCII string, with the following colon-separated fields:
 *
 * serial number:part number:clock speed:options:revision:interface xilinx:
 *
 * (To see the information string, run \e pciload with no arguments.)
 *
 * @note Information embedding was implemented in Sept. 2004; boards shipped before
 * that time will yield a string with all NULLS unless the board's FPGA
 * has since been updated with the embedded information.
 *
 * @param edt_p pointer to edt device structure returned by #edt_open
 * @param esn the EDT part number without dashes.
 * @param osn the OEM's part number, if present.
 * @see edt_get_esn, edt_get_osn, edt_parse_devinfo, edt_fmt_pn
 */

void edt_get_sns_sector(EdtDev *edt_p, char *esn, char *osn, int sector)
{
  int promcode;
  EdtPromData pdata;

  promcode = edt_flash_prom_detect(edt_p, NULL);

  if ((promcode == PROM_UNKN) || (promcode > edt_get_max_promcode()))
  {
    esn[0] = 0;
    osn[0] = 0;
  }
  else
  {
    edt_read_prom_data(edt_p, promcode, sector, &pdata);
    strcpy(esn, pdata.esn);
    strcpy(osn, pdata.osn);
  }
}

void edt_get_sns(EdtDev *edt_p, char *esn, char *osn)
{
  int promcode = edt_flash_prom_detect(edt_p, NULL);
  Edt_prominfo *ep = edt_get_prominfo(promcode);

  edt_get_sns_sector(edt_p, esn, osn, ep->defaultseg);
}

int edt_flash_is_protected(EdtDev *edt_p)
{
  u_short stat;
  int promcode = edt_flash_prom_detect(edt_p, &stat);

  if (edt_is_micron_prom(edt_p))
    return (stat & 0x1) ? 1 : 0;

  else
    return (stat & EDT_ROM_JUMPER) ? 0 : 1;
}

/***************************************************************************
 * Instruction set for MICRON N25Q FLASH used on VisionLink, A5, RCI
 **************************************************************************
 */
#define MIC_JEDEC_ID 0x20 /* Manufacturer ID, Assigned by JEDEC to MICRON*/
#define MIC_RDID 0x9E     /* Read ID,       0 params,      1-20 rbytes */
#define SPI_READ 0x03     /* Read data,     3 addr,           n rbytes */
#define SPI_FREAD 0x0B    /* Fast Read,     3 addr + 1 dummy, n rbytes */
#define MIC_RDNVCR 0xB5   /* Read Nonvolatile configuration register 2 bytes */
#define MIC_WRNVCR 0xB1   /* Write Nonvolatile configuration register 2 bytes */
#define MIC_EN4B 0xB7     /* Enter 4B addr mode, 0 data                */
#define MIC_EX4B 0xE9     /* Exit 4B addr mode, 0 data                 */

#define SPI_WREN 0x06 /* Write Enable,  0 params                   */
#define SPI_WRDI 0x04 /* Write Disable, 0 params                   */
#define SPI_RDSR 0x05 /* Read Status,   0 wbytes, 1 rbyte          */
#define SPI_WRSR 0x01 /* Write Status,  1 wbyte                    */
#define SPI_RDFS 0x70 /* Read Flag Status, 0 params 1-infinite     */
#define SPI_CLFS 0x50 /* Clear Flag Status, 0 params               */
#define SPI_RDEA 0xC8 /* Read Ext. Addr, 0 params                  */
#define SPI_WREA 0xC5 /* Write Ext. Addr, 0 params                 */
#define SPI_PP 0x02   /* Page Program,  3 addr + 1-256 wbytes      */
#define SPI_SSE 0x20  /* Sub Sector Erase, 3 addr                  */
#define SPI_SE 0xD8   /* Sector Erase,  3 addr                     */
#define SPI_BE 0xC7   /* Bulk Erase,    0 params                   */
#define SPI_DP 0xB9   /* Power Down,    0 params                   */
#define SPI_RES 0xAB  /* Read Sig,      3 wdummy, 1 rbyte  (DP off)*/

#define SPI_REG 0x02000084 /* PCI Register for flash writes */
#define SPI_EN 0x8000      /* Enable access to SPI reads and writes */
#define SPI_S 0x2000       /* SEL, low true */
#define SPI_D 0x1000       /* DATA out to serial flash */
#define SPI_Q 0x0200       /* Data from flash spi_q is in bit 9 */

#define MIC_EN 0x00008000  /* Enables access to SPI reads and writes */
#define MIC_CS 0x00004000  /* Chip select note:active low */
#define MIC_STB 0x00002000 /* Falling or Rising edge latches data */
#define MIC_LSB 0x00001000 /* Write and Read lsb first */

static const uint32_t MICRON_JEDEC_SIZE_64MB = 0x17;   //  64MB / 0x4000000
static const uint32_t MICRON_JEDEC_SIZE_128MB = 0x18;  // 128MB / 0x8000000
static const uint32_t MICRON_JEDEC_SIZE_256MB = 0x19;  // 256MB / 0x10000000

/* SPI WORKING VARIABLES */
u_char Spi_wbufa[260];            /* Need 3 addr + 256 data for PP */
u_char *Spi_wbuf = Spi_wbufa + 3; /* Just the 256 bytes of data */
u_char Spi_rbuf[260];             /* Read buffer, could use 256 Kbytes? */
volatile caddr_t Spi_mapaddr = 0;

/* for addr=0x040100C0     04:byte_count 01:interface_Xilinx, C0:PCI_address */
static void spi_xw(EdtDev *edt_p, u_int addr, u_int data)
{
  u_int dmy;
  edt_set(edt_p, addr, data);
  dmy = edt_get(edt_p, 0x04000080); /*flush chipset write buffers*/
}

static u_int spi_xr(EdtDev *edt_p, u_int addr)
{
  return (edt_get(edt_p, addr));
}

static void spi_xwr(EdtDev *edt_p, int val)
{
  int d;
  d = SPI_EN | val; /* Set SEREN bit */
  spi_xw(edt_p, SPI_REG, d);
}

static int spi_xrd(EdtDev *edt_p)
{
  int v;
  v = spi_xr(edt_p, 0x02000084);
  if (v & SPI_Q)
    return (1);
  else
    return (0);
}

static void spiprim(EdtDev *edt_p, int opcode, u_int wbytes, u_int rbytes, u_char *wbuf,
                    u_char *rbuf)
{
  int b, cdat;
  u_int c;

  /* spi_xwr(edt_p, SPI_S); */ /* SEL hi, goes low with first bit*/

  cdat = opcode;
  for (b = 0; b < 8; b++)
  { /* for each bit in the char */
    if (cdat & 0x80)
    {
      spi_xwr(edt_p, SPI_D);
    }
    else
    {
      spi_xwr(edt_p, 0);
    }
    cdat <<= 1;
  }

  for (c = 0; c < wbytes; c++)
  { /* for each char in wbuf */
    cdat = wbuf[c];
    for (b = 0; b < 8; b++)
    { /* for each bit in the char */
      if (cdat & 0x80)
      {
        spi_xwr(edt_p, SPI_D);
      }
      else
      {
        spi_xwr(edt_p, 0);
      }
      cdat <<= 1;
    }
  }

  for (c = 0; c < rbytes; c++)
  { /* for each char in rbuf */
    cdat = 0;
    for (b = 0; b < 8; b++)
    { /* for each bit in the char */
      cdat <<= 1;
      if (spi_xrd(edt_p) & 1)
        cdat |= 1;
      if ((b != 7) || (c != (rbytes - 1)))
      {
        spi_xwr(edt_p, 0);
      }
    }
    rbuf[c] = (char)cdat;
  }

  spi_xwr(edt_p, SPI_S); /* Raise the Select line (to deselect) */
}

/*
 * Read electronic signature
 */
static int spi_reset(EdtDev *edt_p)
{
  u_char rval;
  char *magic = "555"; /* For M25P20 should be 0x11/0x13 */

  spiprim(edt_p, SPI_RES, 3, 1, (u_char *)magic, &rval);

  return (rval & 0xff); /* Also out of Power Down mode  */
}

/* Read rcnt bytes starting from address addr, store in rbuf[] */
static void spi_read(EdtDev *edt_p, u_int addr, u_char *rbuf, int rcnt)
{
  if ((rcnt < 1) || (rcnt > 256))
  {
    EDTPRINTF(edt_p, 1, ("Error, bad spi_read() rcnt of %d\n", rcnt));
    return;
  }
  Spi_wbufa[0] = addr >> 16;
  Spi_wbufa[1] = addr >> 8;
  Spi_wbufa[2] = (char)addr; /* 24 bit address */
  Spi_wbufa[3] = 0;          /* dummy byte */

  spiprim(edt_p, SPI_FREAD, 4, rcnt, Spi_wbufa, rbuf);
}

static int spi_write(EdtDev *edt_p, u_int addr, int wcnt)
{
  int statcnt;

  if ((wcnt < 1) || (wcnt > 256))
  {
    EDTPRINTF(edt_p, 1, ("Error, bad spi_write() wcnt of %d\n", wcnt));
    return -1;
  }
  spiprim(edt_p, SPI_WREN, 0, 0, NULL, NULL); /* write-enable */

  Spi_wbufa[0] = addr >> 16;
  Spi_wbufa[1] = addr >> 8;
  Spi_wbufa[2] = (char)addr; /* 24 bit address */

  spiprim(edt_p, SPI_PP, 3 + wcnt, 0, Spi_wbufa, NULL); /* page program */

  statcnt = 0;
  do
  {
    spiprim(edt_p, SPI_RDSR, 0, 1, NULL, Spi_rbuf); /* Read Status reg */
    statcnt++;
  } while ((Spi_rbuf[0] & 1) == 1); /* Could hang here forever? */

  return 0;
}

#define SPI_TOO_MANY 100000

static int spi_sector_erase(EdtDev *edt_p, u_int addr)
{
  int statcnt;

  spiprim(edt_p, SPI_WREN, 0, 0, NULL, NULL); /* write-enable */
  Spi_wbufa[0] = addr >> 16;
  Spi_wbufa[1] = addr >> 8;
  Spi_wbufa[2] = (u_char)addr; /* 24 bit address */
  spiprim(edt_p, SPI_SE, 3, 0, Spi_wbufa, NULL);

  statcnt = 0;
  do
  {
    spiprim(edt_p, SPI_RDSR, 0, 1, NULL, Spi_rbuf); /* Read Status reg */
    if (statcnt++ > SPI_TOO_MANY)
    {
      EDTPRINTF(edt_p, 1, ("erase spun for too long, bailing out\n"));
      return (-1);
    }
  } while ((Spi_rbuf[0] & 1) == 1); /* Could hang here forever? */
  /* ALERT also should check all 8 status bits says JG */

  return (0);
}

/*
 * print 16 bytes at the address passed
 */
static void spi_print16(EdtDev *edt_p, u_int addr)
{
  int i;
  u_char val;

  EDTPRINTF(edt_p, 2, ("%08x ", addr));
  for (i = 1; i < 16; i++)
  {
    spi_read(edt_p, (u_int)addr, &val, 1);
    EDTPRINTF(edt_p, 2, (" %02x", val));
    addr++;
  }
  EDTPRINTF(edt_p, 2, ("\n"));
}

  /* ************************************************************************ */
  /*  16-bit flash interface */
  /* ************************************************************************ */

#define F16_FLASH_BANK 3

#define F16_REG0 0x30000000
#define F16_P4_CMDDATA_REG 0x04000084
#define F16_P4_DATA_REG 0x04000088

#define F16_CMD_NOP 0x0
#define F16_CMD_LD_ADDR_HI 0x1
#define F16_CMD_LD_CTRL_REG 0x2
#define F16_CMD_LD_ADDR_LO 0x3
#define F16_CMD_FW_ADDR_2AA 0x4
#define F16_CMD_FW_ADDR_555 0x5
#define F16_CMD_FW 0x6
#define F16_CMD_FW_INC_ADDR 0x7
#define F16_CMD_RD_PAL_ID 0x8
#define F16_CMD_RD_PAL_VER 0x9
#define F16_CMD_RD_ADDR_LO 0xA
#define F16_CMD_RD_ADDR_HI 0xB
#define F16_CMD_RD_CTRL_REG 0xC
#define F16_CMD_RD_FLASH 0xD
/* #define F16_CMD_RD_TEST       0xE */

/* NOTE: F16_CMD_IDLE is 0xF in all devs up to KU, careful about this one */
#define F16_CMD_IDLE 0xF

#define F16A_CMD_RD_STATUS 0xF
#define F16A_CMD_IDLE 0x1F

#define F16_FL_BUSY 0x80
#define F16_FL_RESET 0x80
#define F16_FL_WBUF_SIZE 0x10000 /* bytes */
#define F16_FL_RBUF_SIZE 4096    /* bytes */

#define F16A_KU_STAT_FL_BUSY 0x0004 /* 1 = FLASH OPERATIONS IN PROGRESS */

#define F16KU_CTRL_KU_CFG_PG_MASK 0x0007 /* KU CONFIGURATIONPAGE = (CTRL_REG & MASK) */
#define F16KU_CTRL_WP_OVERRIDE 0x0010    /* 1 = OVERRIDE HW FLASH WP */
#define F16KU_CTRL_FLASH_RESET 0x0080    /* 1 = DRIVE FLASK RESET PIN LOW */

/* ************************************************************************ */

static u_int f16_wr_cmd(EdtDev *edt_p, u_int cmd, u_int val, u_int wait_limit)
{
  u_int waitcount = 0, loopwait = 0;
  u_int id_reg, cmd_data;

  cmd_data = (val & 0xffff) | ((cmd << 16) & 0xff0000);

  edt_set(edt_p, F16_P4_CMDDATA_REG, cmd_data);
  /*   rb  = edt_get(edt_p,F16_P4_CMDDATA_REG) & 0xffff ; */
  cmd_data |= 0xff0000;
  edt_set(edt_p, F16_P4_CMDDATA_REG, cmd_data);
  cmd_data = (cmd_data & 0xffff) | ((F16_CMD_RD_PAL_ID << 16) & 0xff0000);

  if (cmd == F16_CMD_FW)
  {
    edt_set(edt_p, F16_P4_CMDDATA_REG, cmd_data);
    if (wait_limit != 0)
    {
      while (1)
      {
        edt_set(edt_p, F16_P4_CMDDATA_REG, cmd_data);
        id_reg = edt_get(edt_p, F16_P4_CMDDATA_REG);
        if ((id_reg & F16_FL_BUSY) != 0)
          break;

        /* loop instead of wait for waitcount loops
         * before adding a delay (speeds it up)
         */
        if (loopwait++ > wait_limit)
        {
          edt_delay(1);
          ++waitcount;
        }

        if (waitcount > wait_limit)
        {
          edt_set(edt_p, F16_P4_CMDDATA_REG, F16_CMD_IDLE << 16);
          EDTPRINTF(edt_p, 1,
                    ("Timeout: ID_REG = %4.4X; CMD = %X; DATA = %4.4X; count = %d\n", id_reg, cmd,
                     val, waitcount));
          return waitcount;
        }
      }
    }
  }
  edt_set(edt_p, F16_P4_CMDDATA_REG, F16_CMD_IDLE << 16);
  return 0;
}

/* ************************************************************************ */
static int f16a_wr_flash(EdtDev *edt_p, int addr, unsigned char *buffer, int length)
{
  int wr_data, sect_addr, pgm_count, max_pgm_count, i;

  if (length == 0)
  {
    length = (int)strlen(buffer);
  }

  while (length > 0)
  {
    sect_addr = addr;
    max_pgm_count = ((addr + 0x100) & 0xffffff00) - addr;
    if (max_pgm_count > ((length + 1) / 2))
      pgm_count = ((length + 1) / 2) - 1;
    else
      pgm_count = max_pgm_count - 1;
    f16a_wr_cmd(edt_p, F16_CMD_LD_ADDR_HI, ((sect_addr >> 16) & 0xffff), 0);
    f16a_wr_cmd(edt_p, F16_CMD_LD_ADDR_LO, (sect_addr & 0xffff), 0);

    // Now the two Unlock cycles
    f16a_wr_cmd(edt_p, F16_CMD_FW_ADDR_555, 0x00AA, 0);
    f16a_wr_cmd(edt_p, F16_CMD_FW_ADDR_2AA, 0x0055, 0);
    f16a_wr_cmd(edt_p, F16_CMD_FW, 0x0025, 0);
    f16a_wr_cmd(edt_p, F16_CMD_FW, pgm_count, 0);

    do
    {
      wr_data = (unsigned int)*(buffer++);
      wr_data |= (unsigned int)*(buffer++) << 8;
      f16a_wr_cmd(edt_p, F16_CMD_FW_INC_ADDR, wr_data, 0);
      sect_addr++;
      length -= 2;
      if (length <= 0)
        break;

    } while ((sect_addr & 0x000000ff) != 0);

    f16a_wr_cmd(edt_p, F16_CMD_LD_ADDR_LO, (addr & 0xffff), 0);

    // transfer FLASH write buffer to ROM
    i = f16a_wr_cmd(edt_p, F16_CMD_FW, 0x0029, 600);
    if (i > 600)
    {
      EDTPRINTF(edt_p, 0,
                ("TIMEOUT: Programming failure at address %8.8X; code = %d\n", sect_addr, i));
      return (-1);
    }
    addr = sect_addr;
  }
}

/* ************************************************************************ */
unsigned int f16a_wr_cmd(EdtDev *edt_p, unsigned int cmd, unsigned int val, unsigned int wait_limit)
{
  unsigned int waitcount = 0;
  unsigned int stat_reg, cmd_data;

  cmd_data = (val & 0xffff) | ((cmd << 16) & 0xff0000);

  edt_set(edt_p, F16_P4_CMDDATA_REG, cmd_data);
  //    rb    = edt_get(edt_p,F16_P4_CMDDATA_REG) & 0xffff ;
  cmd_data |= 0xff0000;
  edt_set(edt_p, F16_P4_CMDDATA_REG, cmd_data);
  cmd_data = (cmd_data & 0xffff) | ((F16A_CMD_RD_STATUS << 16) & 0xff0000);

  if (cmd == F16_CMD_FW)
  {
    edt_set(edt_p, F16_P4_CMDDATA_REG, cmd_data);
    if (wait_limit != 0)
    {
      while (1)
      {
        edt_set(edt_p, F16_P4_CMDDATA_REG, cmd_data);
        stat_reg = edt_get(edt_p, F16_P4_CMDDATA_REG);
        if ((stat_reg & F16A_KU_STAT_FL_BUSY) == 0)
          break;
        edt_delay(1);
        if (++waitcount > wait_limit)
        {
          edt_set(edt_p, F16_P4_CMDDATA_REG, F16A_CMD_IDLE << 16);
          EDTPRINTF(edt_p, 0,
                    ("Timeout: ID_REG = %4.4X; CMD = %X; DATA = %4.4X; count = %d\n", stat_reg, cmd,
                     val, waitcount));
          return waitcount;
        }
      }
    }
  }
  edt_set(edt_p, F16_P4_CMDDATA_REG, F16A_CMD_IDLE << 16);
  return waitcount;
}

/* ************************************************************************ */
static u_int f16_rd_cmd(EdtDev *edt_p, u_int cmd)
{
  int rdata;

  edt_set(edt_p, F16_P4_CMDDATA_REG, cmd << 16);

  rdata = edt_get(edt_p, F16_P4_CMDDATA_REG) & 0xffff;
  edt_set(edt_p, F16_P4_CMDDATA_REG, F16_CMD_IDLE << 16);
  /* NOTE F16_CMD_IDLE = 0xF for F16, F16A CMD_RD_STATUS is overloaded; but
   * OK for the purposes of this subroutine to use for both since it's basically just to delay a bit
   */
  return rdata;
}

/* ************************************************************************ */
static u_int f16a_rd_cmd(EdtDev *edt_p, u_int cmd)
{
  return f16_rd_cmd(edt_p, cmd);
}

/* ************************************************************************ */
static u_int f16_sector_erase(EdtDev *edt_p, u_int sector_addr)
{
  int rdata;

  /*  First set the sector address */
  f16_wr_cmd(edt_p, F16_CMD_LD_ADDR_LO, (sector_addr & 0xffff), 0);
  f16_wr_cmd(edt_p, F16_CMD_LD_ADDR_HI, ((sector_addr >> 16) & 0xffff), 0);
  /*  the "fake addresses" 0x2AA and 0x555 are generated by the cpld */
  /*  Now the two Unlock cycles */
  f16_wr_cmd(edt_p, F16_CMD_FW_ADDR_555, 0x00AA, 0);
  f16_wr_cmd(edt_p, F16_CMD_FW_ADDR_2AA, 0x0055, 0);
  /*  Command Cycle 1 */
  f16_wr_cmd(edt_p, F16_CMD_FW_ADDR_555, 0x0080, 0);
  /*  Command Cycle 2 */
  f16_wr_cmd(edt_p, F16_CMD_FW_ADDR_555, 0x00AA, 0);
  /*  Command Cycle 3 */
  f16_wr_cmd(edt_p, F16_CMD_FW_ADDR_2AA, 0x0055, 0);
  /*  Command Cycle 4: addr = sector_address, data = 0x0030 */
  rdata = f16_wr_cmd(edt_p, F16_CMD_FW, 0x0030, 1200);
  return rdata;
}

/* ************************************************************************ */
static u_int f16a_sector_erase(EdtDev *edt_p, u_int sector_addr)
{
  return f16_sector_erase(edt_p, sector_addr);
}

/* ************************************************************************ */
static int f16_wr_flash(EdtDev *edt_p, u_int addr, u_char *buffer, int nbytes)

{
  int wr_data, sector_addr, pgm_count, max_pgm_count, i;

  if (nbytes == 0)
  {
    nbytes = (int)strlen((char *)buffer);
  }

  while (nbytes > 0)
  {
    sector_addr = addr;
    max_pgm_count = ((addr + 0x100) & 0xffffff00) - addr;
    if (max_pgm_count > ((nbytes + 1) / 2))
      pgm_count = ((nbytes + 1) / 2) - 1;
    else
      pgm_count = max_pgm_count - 1;
    f16_wr_cmd(edt_p, F16_CMD_LD_ADDR_LO, (sector_addr & 0xffff), 0);
    f16_wr_cmd(edt_p, F16_CMD_LD_ADDR_HI, ((sector_addr >> 16) & 0xffff), 0);

    /*  Now the two Unlock cycles */
    f16_wr_cmd(edt_p, F16_CMD_FW_ADDR_555, 0x00AA, 0);
    f16_wr_cmd(edt_p, F16_CMD_FW_ADDR_2AA, 0x0055, 0);
    f16_wr_cmd(edt_p, F16_CMD_FW, 0x0025, 0);
    f16_wr_cmd(edt_p, F16_CMD_FW, pgm_count, 0);

    do
    {
      wr_data = (u_int) * (buffer++);
      wr_data |= (u_int) * (buffer++) << 8;
      f16_wr_cmd(edt_p, F16_CMD_FW_INC_ADDR, wr_data, 0);
      sector_addr++;
      nbytes -= 2;
      if (nbytes <= 0)
        break;

    } while ((sector_addr & 0x000000ff) != 0);

    f16_wr_cmd(edt_p, F16_CMD_LD_ADDR_LO, (addr & 0xffff), 0);
    f16_wr_cmd(edt_p, F16_CMD_LD_ADDR_HI, ((addr >> 16) & 0xffff), 0);

    /*  transfer FLASH write buffer to ROM */
    i = f16_wr_cmd(edt_p, F16_CMD_FW, 0x0029, 200);
    if (i > 200)
    {
      EDTPRINTF(edt_p, 0,
                ("TIMEOUT: Programming failure at address %8.8X; code = %d\n", sector_addr, i));
      return -1;
    }

    addr = sector_addr;
  }
  return 0;
}

/* ************************************************************************ */
static void f16_rd_flash_block(EdtDev *edt_p, u_int addr, u_char *buffer, u_int byte_count)

#define USE_F16A 1 /* just for this */
#ifdef USE_F16A    /* should work for both, maybe better? */
{
  f16a_rd_flash_block(edt_p, addr, buffer, byte_count);
}
#else
{
  u_int i, rd_data;

  f16_wr_cmd(edt_p, F16_CMD_LD_ADDR_HI, ((addr >> 16) & 0xffff), 0);

  for (i = 0; i < byte_count; i += 2)
  {
    f16_wr_cmd(edt_p, F16_CMD_LD_ADDR_LO, (addr & 0xffff), 0);
    f16_wr_cmd(edt_p, F16_CMD_RD_FLASH, 0, 0);

    rd_data = edt_get(edt_p, F16_P4_CMDDATA_REG);
    *(buffer + i) = (u_char)(rd_data & 0xff);
    *(buffer + i + 1) = (u_char)((rd_data >> 8) & 0xff);
    addr++;
    if (addr & 0x0000ffff == 0)
      f16_wr_cmd(edt_p, F16_CMD_LD_ADDR_HI, ((addr >> 16) & 0xffff), 0);
  }
}
#endif

static void f16a_dump_flash(EdtDev *edt_p, u_int waddr, u_int size)
{
  int i, j, index;
  unsigned char c = 0;
  unsigned char read_buffer[1024];  // FLASH read buffer

  /* First, RESET the Flash part and read the ID and version registers */
  if (edt_p->devid == PE8G3KU_ID)
  {
    unsigned int ctrl_register = f16a_rd_cmd(edt_p, F16_CMD_RD_CTRL_REG);
    unsigned int ku_cfg_bank = ctrl_register & 0x07;

    // f16a_wr_cmd(edt_p, F16_CMD_LD_CTRL_REG, ku_cfg_bank, 0) ;

    f16a_wr_cmd(edt_p, F16_CMD_LD_CTRL_REG, ku_cfg_bank | F16KU_CTRL_FLASH_RESET, 0);
    edt_delay(100);
    f16a_wr_cmd(edt_p, F16_CMD_LD_CTRL_REG, ku_cfg_bank, 0);
    edt_delay(100);
  }

  waddr &= 0xffffff00;
  f16a_wr_cmd(edt_p, F16_CMD_LD_ADDR_LO, (waddr & 0xffff), 0);
  f16a_wr_cmd(edt_p, F16_CMD_LD_ADDR_HI, ((waddr >> 16) & 0xffff), 0);

  while ((int)size > 0)
  {
    // load 512 bytes into the read buffer
    f16a_rd_flash_block(edt_p, waddr, read_buffer, 512);

    index = 0;
    for (j = 0; j < 32; j++)
    {
      EDTPRINTF(edt_p, 0, ("\n%8.8X=", waddr));
      for (i = 0; i < 16; i++)
      {
        EDTPRINTF(edt_p, 0, ("%2.2X ", read_buffer[index++]));
        if ((i % 4) == 3)
          EDTPRINTF(edt_p, 0, (" "));
      }

      index -= 16;

      for (i = 0; i < 16; i++)
      {
        if (((c = read_buffer[index++]) >= 0x20) && (c < 0x77))
        {
          EDTPRINTF(edt_p, 0, ("%c", c));
        }
        else
          EDTPRINTF(edt_p, 0, ("."));
      }

      waddr += 8;
    }
    size -= 1024;
  }
  EDTPRINTF(edt_p, 0, ("\n"));
}

/*************************************************************************/
static void f16a_rd_flash_block(EdtDev *edt_p, unsigned int addr, unsigned char *buffer,
                                unsigned int byte_count)
{
  unsigned int i, rd_data;

  f16a_wr_cmd(edt_p, F16_CMD_LD_ADDR_HI, ((addr >> 16) & 0xffff), 0);

  for (i = 0; i < byte_count; i += 2)
  {
    f16a_wr_cmd(edt_p, F16_CMD_LD_ADDR_LO, (addr & 0xffff), 0);
    f16a_wr_cmd(edt_p, F16_CMD_RD_FLASH, 0, 0);
    rd_data = edt_get(edt_p, F16_P4_CMDDATA_REG);
    *(buffer + i) = (unsigned char)(rd_data & 0xff);
    *(buffer + i + 1) = (unsigned char)((rd_data >> 8) & 0xff);
    addr++;
    if ((addr & 0x0000ffff) == 0)
      f16a_wr_cmd(edt_p, F16_CMD_LD_ADDR_HI, ((addr >> 16) & 0xffff), 0);
  }
}

/* ************************************************************************ */
static int f16a_reset(EdtDev *edt_p)
{
  return f16_reset(edt_p);
}

/*  RESET the Flash part and read the ID and version registers */
static int f16_reset(EdtDev *edt_p)
{
  f16_wr_cmd(edt_p, F16_CMD_LD_CTRL_REG, F16_FL_RESET, 0);
  edt_delay(100);
  f16_wr_cmd(edt_p, F16_CMD_LD_CTRL_REG, 0, 0);
  edt_delay(100);
  return 0;
}

/*
 * print 16 bytes at the address passed read from 4028xla boot controller
 */
static void f16_print16(EdtDev *edt_p, u_int addr)
{
  int i;
  u_char buf[16];

  EDTPRINTF(edt_p, 2, ("%08x ", addr));
  f16_rd_flash_block(edt_p, addr, buf, 16);
  for (i = 1; i < 16; i++)
  {
    EDTPRINTF(edt_p, 2, (" %02x", buf[i]));
    addr++;
  }
  EDTPRINTF(edt_p, 2, ("\n"));
}

/*
 * print 16 bytes at the address passed read from 4028xla boot controller
 */
static void f16a_print16(EdtDev *edt_p, u_int addr)
{
  int i;
  u_char buf[16];

  EDTPRINTF(edt_p, 2, ("%08x ", addr));
  f16a_rd_flash_block(edt_p, addr, buf, 16);
  for (i = 1; i < 16; i++)
  {
    EDTPRINTF(edt_p, 2, (" %02x", buf[i]));
    addr++;
  }
  EDTPRINTF(edt_p, 2, ("\n"));
}

/*
 * from spiw in pdbx.c
 */
static int spi_block_program(EdtDev *edt_p, int addr0, u_int size, u_char *spibuf, int verify_only)
{
  int i, n, errorcnt, val, first, last;
  u_int addr, bcnt;

  /* reset HACK -- sometimes it doesn't work the first time (?) */
  for (i = 0; i < 5; i++)
  {
    if ((val = spi_reset(edt_p)) != 0)
      break;
    (void)edt_delay(100);
  }

  if ((val != 0x11) && (val != 0x13) && (val != 0x14))
  {
    EDTPRINTF(edt_p, 0, ("    Error, did not detect SPI flash device, \n"));
    EDTPRINTF(edt_p, 0, ("      spi_reset() returned 0x%02x, not 0x11/0x13/0x14\n", val));
    return -1;
  }

  if (!verify_only)
  {
    first = (addr0 & 0xff0000) >> 16;
    last = ((addr0 + size) & 0xff0000) >> 16;
    EDTPRINTF(edt_p, 0, ("  First sector is %d, last is %d, erasing", first, last));

    for (n = first; n <= last; n++)
    {
      spi_sector_erase(edt_p, n << 16);
      EDTPRINTF(edt_p, 0, ("."));
    }
    EDTPRINTF(edt_p, 0, ("\n"));

    EDTPRINTF(edt_p, 0, ("  Writing 0x%x bytes to spiflash at 0x%x.", size, addr0));
    bcnt = 0;
    addr = addr0;
    while (bcnt < size)
    {
      n = 0;
      while (bcnt < size)
      {
        if ((n != 0) && (((addr + n) & 0xff) == 0))
          break; /* page end */
#ifndef _KERNEL
        if (!(bcnt % 10000))
        {
          printf(".");
          fflush(stdout);
        }
#endif
        Spi_wbuf[n] = spibuf[bcnt];
        n = n + 1;
        bcnt = bcnt + 1;
      }
      if (n > 0)
        spi_write(edt_p, addr, n); /* Write 1-256 bytes from Spi_wbuf*/
      addr = addr + n;
    }
  }

  EDTPRINTF(edt_p, 0, ("\n"));
  EDTPRINTF(edt_p, 0, ("  Verifying."));
  bcnt = 0;
  addr = addr0;
  errorcnt = 0;
  while (bcnt < size)
  {
    spi_read(edt_p, addr, Spi_rbuf, 256);
    n = 0;
    while (bcnt < size)
    {
      if ((n != 0) && (((addr + n) & 0xff) == 0))
        break; /* page end */
#ifndef _KERNEL
      if (!(bcnt % 10000))
      {
        printf(".");
        fflush(stdout);
      }
#endif
      if ((Spi_rbuf[n] & 0xff) != (spibuf[bcnt] & 0xff))
      {
        if (errorcnt++ <= 10)
        {
          EDTPRINTF(
              edt_p, 0,
              ("Error at 0x%x: wrote:%02x, read:%02x\n", addr0 + bcnt, spibuf[bcnt], Spi_rbuf[n]));
        }
      }
      n = n + 1;
      bcnt = bcnt + 1;
    }
    addr = addr + n;
  }
  if (errorcnt)
    EDTPRINTF(edt_p, 0, ("    Saw %d errors\n", errorcnt));

  if (!errorcnt)
    EDTPRINTF(edt_p, 0, ("\nno errors\n"));

  return 0;
}

/* write a byte to the PCI interface prom (only 4013E and 4013XLA) */
static void x_write(EdtDev *edt_p, u_int addr, u_char val)
{
  (void)tst_read(edt_p, EDT_FLASHROM_ADDR);
  tst_write(edt_p, EDT_FLASHROM_DATA, val);
  (void)tst_read(edt_p, EDT_FLASHROM_ADDR);
  tst_write(edt_p, EDT_FLASHROM_ADDR, EN_HIGH_ADD | addr | EDT_WRITECMD);
}

/* read a byte from the PCI interface prom (only 4013E and 4013XLA) */
static u_char x_read(EdtDev *edt_p, u_int addr)
{
  (void)tst_read(edt_p, EDT_FLASHROM_ADDR);
  tst_write(edt_p, EDT_FLASHROM_ADDR, EN_HIGH_ADD | addr);
  (void)tst_read(edt_p, EDT_FLASHROM_ADDR);
  return (u_char)(tst_read(edt_p, EDT_FLASHROM_DATA));
}

/* ?reset? 4013e or 4013xla */
static void x_reset(EdtDev *edt_p)
{
  x_write(edt_p, 0x5555, 0xaa);
  x_write(edt_p, 0x2aaa, 0x55);
  x_write(edt_p, 0x5555, 0xf0);
}

/*
 * write a byte to the PROM thru the boot controller on the 4028xla
 */
static void bt_write(EdtDev *edt_p, u_int addr, u_char val)
{
  /* write the address out in three chunks to the boot controller */
  /* first all commands are off */
  tst_write(edt_p, EDT_FLASHROM_DATA, 0);
  /* first byte of address */
  tst_write(edt_p, EDT_FLASHROM_DATA, BT_LD_LO | (addr & 0xff));
  tst_write(edt_p, EDT_FLASHROM_DATA, 0);
  /* second byte of address */
  tst_write(edt_p, EDT_FLASHROM_DATA, BT_LD_MID | ((addr >> 8) & 0xff));
  tst_write(edt_p, EDT_FLASHROM_DATA, 0);

  /* third byte of address */
  tst_write(edt_p, EDT_FLASHROM_DATA, BT_LD_HI | ((addr >> 16) & 0xff));
  tst_write(edt_p, EDT_FLASHROM_DATA, 0);

  tst_write(edt_p, EDT_FLASHROM_DATA, BT_LD_ROM | (val & 0xff));
  tst_write(edt_p, EDT_FLASHROM_DATA, 0);
}

/* ?reset? the 4028xla */
static void bt_reset(EdtDev *edt_p)
{
  bt_write(edt_p, 0x555, 0xaa);
  bt_write(edt_p, 0x2aa, 0x55);
  bt_write(edt_p, 0x555, 0xf0);
}

/* ?reset? for xc5vlx30t, xc5vlx50t, ep2sgx30d */
static void bt2_reset(EdtDev *edt_p)
{
  bt_write(edt_p, 0x0, 0xf0);
}

static u_char x_readstat(EdtDev *edt_p)
{
  uint_t stat;
  uint_t rdval;

  /*
   * disable the high address drive by writing 0 to bit24 of FPROM address
   * register.
   */
  tst_write(edt_p, EDT_FLASHROM_ADDR, 0);
  rdval = tst_read(edt_p, EDT_FLASHROM_DATA);
  stat = (u_short)((rdval >> 8) & 0xff);
  return (u_char)stat;
}

/*
 * print 16 bytes at the address passed
 */
static void x_print16(EdtDev *edt_p, u_int addr)
{
  int i;

  EDTPRINTF(edt_p, 2, ("%08x ", addr));
  for (i = 1; i < 16; i++)
  {
    EDTPRINTF(edt_p, 2, (" %02x", x_read(edt_p, addr)));
    addr++;
  }
  EDTPRINTF(edt_p, 2, ("\n"));
}

/*
 * read a byte from the 4028xla prom through the boot controller.
 */
static u_char bt_read(EdtDev *edt_p, u_int addr)
{
  u_char stat;

  /* write the address out in three chunks to the boot controller */
  /* first all commands are off */
  tst_write(edt_p, EDT_FLASHROM_DATA, 0);
  /* first byte of address */
  tst_write(edt_p, EDT_FLASHROM_DATA, BT_LD_LO | (addr & 0xff));
  /* clear BT_LD_LO */
  tst_write(edt_p, EDT_FLASHROM_DATA, 0);
  /* second byte of address */
  tst_write(edt_p, EDT_FLASHROM_DATA, BT_LD_MID | ((addr >> 8) & 0xff));
  /* clear BT_LD_MID */
  tst_write(edt_p, EDT_FLASHROM_DATA, 0);
  /* third byte of address */
  tst_write(edt_p, EDT_FLASHROM_DATA, BT_LD_HI | ((addr >> 16) & 0xff));
  /* clear BT_LD_HI , set rd command and disable output buffer */
  tst_write(edt_p, EDT_FLASHROM_DATA, BT_RD_ROM);

  /* read the byte */
  stat = (u_short)tst_read(edt_p, EDT_FLASHROM_DATA) | 0xff00;
  /* all commands are off */
  tst_write(edt_p, EDT_FLASHROM_DATA, 0);
  return (stat);
}

/*
 * read the jumper position and bus voltage from the boot controller
 */
static u_char bt_readstat(EdtDev *edt_p)
{
  u_char stat;
  uint_t rdval;

  /*
   * read the boot controller status A0 and A1 low with read asserted -
   * enables boot controller on bus
   */
  tst_write(edt_p, EDT_FLASHROM_DATA, 0);
  tst_write(edt_p, EDT_FLASHROM_DATA, BT_EN_READ | BT_READ);
  rdval = tst_read(edt_p, EDT_FLASHROM_DATA);
  stat = rdval;
  /* reset all boot command */
  tst_write(edt_p, EDT_FLASHROM_DATA, 0);
  return (stat);
}

void edt_flash_reset(EdtDev *edt_p, int ftype)
{
  switch (ftype)
  {
    case FTYPE_F16A:
      f16a_reset(edt_p);
      break;
    case FTYPE_F16:
      f16_reset(edt_p);
      break;
    case FTYPE_MIC:
      mic_reset(edt_p);
      break;
    case FTYPE_SPI:
      spi_reset(edt_p);
      break;
    case FTYPE_BT:
      bt_reset(edt_p);
      break;
    case FTYPE_BT2:
      bt2_reset(edt_p);
      break;
    case FTYPE_X:
      x_reset(edt_p);
      break;
    default:
      break;
  }
}

/*
 * get the sector size -- from array for older (pre PCIe), 1st 4 bytes of PROM
 * for newer (PCIe8, etc.)
 */
u_int edt_get_fpga_sectorsize(EdtDev *edt_p, Edt_prominfo *ep)
{
  u_int tmpval;

  if (ep->ftype == FTYPE_BT)
  {
    tmpval = bt_read(edt_p, 0x0) | (bt_read(edt_p, 0x1) << 8) | (bt_read(edt_p, 0x2) << 16) |
             (bt_read(edt_p, 0x3) << 24);
    return tmpval;
  }
  else
    return ep->sectorsize;
}

/*
 * new fpgas (pcie8, etc) info addr is indexed from 2nd 4 bytes of prom
 */
u_int get_fpga_info_index_bt8(EdtDev *edt_p)
{
  u_int tmpval;

  tmpval = bt_read(edt_p, 0x4) | (bt_read(edt_p, 0x5) << 8) | (bt_read(edt_p, 0x6) << 16) |
           (bt_read(edt_p, 0x7) << 24);
  return tmpval;
}

/*
 * get the onboard PROM device info from the string at the end of the segment
 *  NOTE: don't call directly -- instead use edt_flash_get_promaddrs
 */
u_int edt_get_id_addr(int promcode, int segment)
{
  u_int id_addr;
  Edt_prominfo *ep = edt_get_prominfo(promcode);

  if (ep->sectorsize == 0)
    return 1;

  switch (promcode)
  {
    case AMD_4013E:
      id_addr = (ep->sectsperseg * ep->sectorsize) - PCI_ID_SIZE;
      break;

    case SPI_XC3S1200E:
      id_addr = segment * ep->sectorsize;
      break;

    case MIC_N25Q064A13ESE40G:
      id_addr = MIC_N25Q064A13ESE40G_PROMINFO_ADDR;
      break;

    case MIC_N25Q256A13EF840:
      id_addr = (segment + 1) * ep->sectorsize * (ep->sectsperseg - 1);
      break;

    default: /* is at the end of the segment */
      id_addr = ((segment + 1) * ep->sectsperseg * ep->sectorsize) - PCI_ID_SIZE;
      break;
  }
  return id_addr;
}

/* Get the flash PROM bank size.
 * Note that this may be the full size of the PROM or the size of one program
 * bank, if the device is split up into multiple banks (program sectors).
 * @param promcode the prom code, from #edt_flash_prom_detect
 * @return the size in bytes of the flash PROM programming bank
 */
u_int edt_get_flashsize(int promcode)
{
  Edt_prominfo *ep = edt_get_prominfo(promcode);

  return ep->sectsperseg * ep->sectorsize;
}

/*
 * Read the board's FPGA configuration file id and serial numbers.
 *
 * @param edt_p pointer to edt device structure returned by #edt_open
 * @param promcode device prom code, from #edt_flash_prom_detect
 * @param sect sector from which to read the prom (-1 for the default)
 * @param idstr FPGA configuration file ID string for the given segment
 * @param devinfo device information string including serial_number part number, and other
 * information
 * @param osn the OEM's part number string, if present.
 * @see  edt_parse_devinfo, edt_flash_prom_detect
 */
void edt_readinfo(EdtDev *edt_p, int promcode, int sect, char *idstr, char *devinfo, char *oemsn)
{
  Edt_prominfo *ep = edt_get_prominfo(promcode);
  EdtPromData pdata;
  int sector = (sect < 0) ? ep->defaultseg : sect;

  edt_read_prom_data(edt_p, promcode, sector, &pdata);
  strncpy(idstr, pdata.id, PCIE_PID_SIZE);
  strncpy(devinfo, pdata.esn, ESN_SIZE);
  strncpy(oemsn, pdata.osn, OSN_SIZE);
}

/*
 * get id addresses, ==>including serial numbers<== (since they're not all the same
 * any more -- specifically with newer PCIE (XC3S1200E, XCVLX30/50T) we put the ID
 * at the beginning not the end, and have up to 1024 bytes (though the sizes,
 * particularly OSN, are still the same so far) (OSN will stay the same I think too)
 */
u_int edt_flash_get_promaddrs(EdtDev *edt_p, int promcode, int segment, EdtPromIdAddresses *paddr)

{
  Edt_prominfo *ep = edt_get_prominfo(promcode);
  char tst[4];
  /* u_int info_addr = ep->id_addr((u_long)edt_p, segment); */
  /* STORE MORE STUFF HERE ?? */

  paddr->id_addr = edt_get_id_addr(promcode, segment);

  /*
   * SPI devices:
   *   PROM ID 128088
   *   OSN 32
   *   ESN 64 (or more up to total of 1024)
   *   data
   */
  if (ep->ftype == FTYPE_SPI)
  {
    paddr->osn_addr = paddr->id_addr + PCI_ID_SIZE;
    paddr->esn_addr = paddr->osn_addr + OSN_SIZE;
    paddr->extra_tag_addr = paddr->esn_addr + ESN_SIZE;
    paddr->extra_size_addr = paddr->extra_tag_addr + 4;
    paddr->extra_data_addr = paddr->extra_size_addr + 4;
  }
  /*
   * All other devices:
   *   data (0 up to segment size minus 128 [PCI_ID_SIZE])
   *   .
   *   .
   *   .
   *   OSN 32
   *   ESN 64
   *   PROM ID 128
   */
  else if (ep->ftype == FTYPE_MIC)
  {
    paddr->osn_addr = paddr->id_addr + PCI_ID_SIZE;
    paddr->esn_addr = paddr->osn_addr + OSN_SIZE;
    paddr->optsn_addr = paddr->esn_addr + ESN_SIZE;
    paddr->extra_tag_addr = paddr->optsn_addr + OPTSN_SIZE;
    paddr->extra_size_addr = paddr->extra_tag_addr + EXTRAADDR_SIZE;
    paddr->extra_data_addr = paddr->extra_size_addr + EXTRAADDR_SIZE;
    paddr->maclist_addr = paddr->extra_data_addr + EXTRAADDR_SIZE;
  }
  else
  {
    paddr->osn_addr = paddr->id_addr - OSN_SIZE;
    paddr->esn_addr = paddr->osn_addr - ESN_SIZE;
    paddr->extra_tag_addr = paddr->esn_addr - EXTRAADDR_SIZE;
    paddr->extra_size_addr = paddr->extra_tag_addr - EXTRAADDR_SIZE;
    paddr->optsn_addr = paddr->extra_tag_addr - OPTSN_SIZE;
    paddr->maclist_addr = paddr->optsn_addr - MACLIST_SIZE;
  }

  edt_flash_block_read(edt_p, paddr->extra_tag_addr, (u_char *)tst, 4, ep->ftype);

  if (strncmp(tst, "XTR:", 4) == 0)
  {
    edt_flash_block_read(edt_p, paddr->extra_size_addr, (u_char *)&paddr->extra_size, 4, ep->ftype);

    if (needswap)
    {
      paddr->extra_size = swap32(paddr->extra_size);
    }
    if ((ep->ftype != FTYPE_SPI) && (ep->ftype != FTYPE_F16) && (ep->ftype != FTYPE_F16A) &&
        (paddr->extra_size != 0) && (paddr->extra_size > PROM_EXTRA_SIZE))
      paddr->extra_data_addr = paddr->extra_size_addr - (4 + paddr->extra_size);
    else
    {
      paddr->extra_data_addr = 0;
      paddr->extra_size = 0;
    }
  }
  else
  {
    paddr->extra_size = 0;
    paddr->extra_data_addr = 0;
  }

  return paddr->id_addr;
}

/*
 * addresses for id information, per xilinx type
 */

u_int ida_4013e(void *dmy, int segment)
{
  return (8 * E_SECTOR_SIZE) - PCI_ID_SIZE;
}

u_int ida_4013xla(void *dmy, int segment)
{
  return (segment * AMD_SECTOR_SIZE) + AMD_SECTOR_SIZE - PCI_ID_SIZE;
}

u_int ida_4028xla(void *dmy, int segment)
{
  return ((segment + 1) * 2 * AMD_SECTOR_SIZE) - PCI_ID_SIZE;
}

u_int ida_xc2s100(void *dmy, int segment)
{
  return ((segment + 1) * 4 * AMD_SECTOR_SIZE) - PCI_ID_SIZE;
}

u_int ida_xc3s1200e(void *dmy, int segment)
{
  return segment * SPI_SECTOR_SIZE;
}

static u_int ida_bt8(void *edt_p, int dmy)
{
  return get_fpga_info_index_bt8((EdtDev *)edt_p);
}

static u_int ida_f16(void *edt_p, int segment)
{
  u_short stat;
  int promcode = edt_flash_prom_detect(edt_p, &stat);
  Edt_prominfo *ep = edt_get_prominfo(promcode);
  u_int id_addr = ((segment + 1) * ep->sectsperseg * ep->sectorsize) - PCI_ID_SIZE;

  return id_addr;
}

static u_int ida_micron(void *edt_p, int segment)
{
  return edt_get_id_addr(edt_flash_prom_detect(edt_p, NULL), segment);
}

static void tst_init(EdtDev *edt_p)
{
#ifndef _KERNEL

  if (!edt_p->mapaddr)
    edt_p->mapaddr = (caddr_t)edt_mapmem(edt_p, 0, 256);

#endif

  needswap = !edt_little_endian();
}

static void tst_write(EdtDev *edt_p, uint_t desc, uint_t val)
{
#ifndef _KERNEL
  u_int dmy;

  if (edt_p->mapaddr)
  {
    tstaddr = (volatile u_int *)(edt_p->mapaddr + (desc & 0xff));

    if (needswap)
    {
      *tstaddr = swap32(val);
    }
    else
    {
      *tstaddr = val;
    }

    dmy = *tstaddr;
  }
  else
#endif
    edt_set(edt_p, desc, val);
}

static uint_t tst_read(EdtDev *edt_p, uint_t desc)
{
#ifndef _KERNEL

  if (edt_p->mapaddr)
  {
    tstaddr = (volatile u_int *)(edt_p->mapaddr + (desc & 0xff));

    if (needswap)
    {
      return (swap32(*tstaddr));
    }
    else
      return (*tstaddr);
  }
  else
#endif
    return (edt_get(edt_p, desc));
}

void tst_pflash(EdtDev *edt_p, uint_t addr, uint_t val)
{
  tst_write(edt_p, EDT_FLASHROM_DATA, (u_char)0xaa);
  (void)tst_read(edt_p, EDT_FLASHROM_ADDR);
  tst_write(edt_p, EDT_FLASHROM_ADDR, (u_int)(0x5555 | EDT_WRITECMD));
  (void)tst_read(edt_p, EDT_FLASHROM_ADDR);
  tst_write(edt_p, EDT_FLASHROM_DATA, (u_char)0x55);
  (void)tst_read(edt_p, EDT_FLASHROM_ADDR);
  tst_write(edt_p, EDT_FLASHROM_ADDR, (u_int)(0x2aaa | EDT_WRITECMD));
  (void)tst_read(edt_p, EDT_FLASHROM_ADDR);
  tst_write(edt_p, EDT_FLASHROM_DATA, (u_char)0xa0);
  (void)tst_read(edt_p, EDT_FLASHROM_ADDR);
  tst_write(edt_p, EDT_FLASHROM_ADDR, (u_int)(0x5555 | EDT_WRITECMD));
  (void)tst_read(edt_p, EDT_FLASHROM_ADDR);
  tst_write(edt_p, EDT_FLASHROM_DATA, (u_char)val);
  (void)tst_read(edt_p, EDT_FLASHROM_ADDR);
  tst_write(edt_p, EDT_FLASHROM_ADDR, (u_int)(EN_HIGH_ADD | addr | EDT_WRITECMD));
  (void)tst_read(edt_p, EDT_FLASHROM_ADDR);
}

/*
 * read a string out of the flash PROM, of a given size, from a given address.
 * used for FPGA ID id string and serial number strings
 *
 * ARGUMENTS
 *   edt_p     pointer to already opened edt device
 *   addr      address to start reading the string
 *   buf       buffer to read the string into
 *   size      number of bytes to read
 *
 * RETURNS
 *   pointer to magic first valid ASCII character in string (should be
 *   0th but may not be, esp. if blank)
 *
 */
static char *edt_flash_string_read(EdtDev *edt_p, u_int addr, char *buf, u_int size, int ftype)
{
  u_int i;
  u_char *ret = 0;

  /* EDTPRINTF(edt_p,1,("%x ", val)); */

  edt_flash_block_read(edt_p, addr, (u_char *)buf, size, ftype);

  /* record start of string */
  for (i = 0; i < size; i++)
  {
    if (!ret && buf[i] >= 0x20 && buf[i] <= 0x7e)
      ret = &buf[i];
  }

  buf[size] = 0;
  return ret;
}

/*
 * read a string out of the FPGA, of a given size, from a given address.
 * used for Xilinx ID id string and serial number strings
 *
 * ARGUMENTS
 *   edt_p     pointer to already opened edt device
 *   addr      address to start reading the string
 *   buf       buffer to read the string into
 *   size      number of bytes to read
 *
 * RETURNS   buffer passed in
 *
 */
u_char *edt_flash_block_read(EdtDev *edt_p, u_int addr, u_char *buf, u_int size, int ftype)
{
  u_int i;

  /* EDTPRINTF(edt_p,1,("full string = ")); */
  switch (ftype)
  {
    case FTYPE_F16A:

      f16a_rd_flash_block(edt_p, addr, buf, size);
      break;

    case FTYPE_F16:
      f16_rd_flash_block(edt_p, addr, buf, size);
      break;

    case FTYPE_MIC:
      mic_read_flash(edt_p, SPI_READ, addr, buf, size);
      break;

    case FTYPE_SPI:
      spi_reset(edt_p);
      spi_read(edt_p, (u_int)addr, buf, size);
      break;

    case FTYPE_BT:
    case FTYPE_BT2:
      for (i = 0; i < size; i++)
        buf[i] = bt_read(edt_p, addr++);
      break;

    case FTYPE_X:
      for (i = 0; i < size; i++)
        buf[i] = x_read(edt_p, addr++);
      break;

    default:
      break;
  }

  return buf;
}

int edt_sector_erase(EdtDev *edt_p, u_int sector, u_int sec_size, int ftype)
{
  int result = 0;

  switch (ftype)
  {
    case FTYPE_F16A:
      result = f16a_sector_erase(edt_p, sector << 16);
      break;

    case FTYPE_F16:
      result = f16_sector_erase(edt_p, sector << 16);
      break;

    case FTYPE_MIC:
      result = mic_sector_erase(edt_p, sector << 16);
      break;

    case FTYPE_SPI:
      result = spi_sector_erase(edt_p, sector << 16);
      break;

    case FTYPE_BT:
      result = bt_sector_erase(edt_p, sector, sec_size);
      break;

    case FTYPE_BT2:
      result = bt2_sector_erase(edt_p, sector, sec_size);
      break;

    case FTYPE_X:
      xsector_erase(edt_p, sector, sec_size);
      break;

    default:
      result = -1;
      break;
  }
  return (result);
}

/**
 * Find the flash prom and flash status (jumper positions) for the device.
 * Finds out which flash PROM is used and queries the device PROM status --
 * see EPinfo stat field for status bits values
 *
 * @param edt_p pointer to edt device structure returned by #edt_open
 * @param stat device status (register value) (if passed in value of STAT is NULL, it will be
 * ignored)
 * @return prom device index -- see "Legal device proms" in libedt.h & EPinfo table at the top of
 * #file
 */
int edt_flash_prom_detect(EdtDev *edt_p, u_short *stat)
{
  u_int desc = EDT_FLASHROM_DATA;
  u_short idbits = 0xff;  /* invalid */
  u_short xidbits = 0xff; /* invalid */
  int prom = PROM_UNKN;
  u_int flashrom_bits;
  u_int straps; 
  u_int pal_ver = 0;
  u_short status = 0; /* non-existant EPinfo stat field value */
  char *typestr = "unknown";

  EDTPRINTF(edt_p, 2, ("edt_flash_prom_detect\n"));

  if (edt_is_16bit_prom(edt_p) || edt_is_micron_prom(edt_p))
  {
    if (edt_is_micron_prom(edt_p))
    {
      status = mic_get_status(edt_p);
      typestr = "MIC";
      if (edt_p->devid != PE8VLCLS_ID)
        mic_get_manufacturer_info(edt_p);
    }
    else /* F16 */
    {
      edt_set(edt_p, 0, 8);
      status = (u_short)(f16_rd_cmd(edt_p, F16_CMD_RD_PAL_ID) &
                         0xffff); /* f16_rd_cmd is the same for F16 and F16A */
      typestr = "F16";
    }

    straps = status & 0x03;
    idbits = (status >> 2) & 0x1f;
    xidbits = (status >> 8) & 0xff;
    pal_ver = f16_rd_cmd(edt_p, F16_CMD_RD_PAL_VER); /* f16_rd_cmd is the same for F16 and F16A */
  }
  else
  {
    /*
     * set up structures for direct peek/poke;
     * if the BT_A0 bit in the PROM_DATA can be written high
     * this must be a newer board with a 4028xla or XC2S___
     * (but not newest, with 16-bit flash)
     */
    tst_init(edt_p);
    tst_write(edt_p, desc, BT_A0);
    flashrom_bits = tst_read(edt_p, desc) & BT_A0;

    if (flashrom_bits == BT_A0)
    {
      EDTPRINTF(edt_p, 2, ("  FLASHROM command BT_A0 can be set board has boot controller\n"));
      status = (u_short)bt_readstat(edt_p) & 0xff;
      xidbits = 0xff;
      idbits = (u_char)((status & STAT_IDMASK) >> STAT_IDSHFT);
      typestr = "BT";
    }
    else
      typestr = "OLD";
  }

  EDTPRINTF(
      edt_p, 2,
      ("\t\tedt_flash_prom_detect (%s): stat %04x id bits %02x %02x, pal_ver %x, finding prom...\n",
       typestr, status, xidbits, idbits, pal_ver));

  if (!(idbits == 0xff && xidbits == 0xff))
  {
    int i;
    int max = edt_get_max_promcode() + 1;

    for (i = 0; i < max; i++)
    {
      if ((idbits == (u_int)EPinfo[i].stat) &&
          (((xidbits == (u_int)EPinfo[i].statx)) || (EPinfo[i].statx == 0xff)))
      {
        if (stat)
          *stat = status;
        return (i);
      }
    }
  }

  /*
   * hmm still not set, probably an older board...
   * Check with x_write/read
   */
  if (prom == PROM_UNKN)
  {
    int i;
    u_char mid, devid;

    edt_flash_reset(edt_p, 0);

    /* this code borrowed from xautoselect() */
    x_write(edt_p, 0x5555, 0xaa);
    x_write(edt_p, 0x2aaa, 0x55);
    x_write(edt_p, 0x5555, 0x90);
    mid = x_read(edt_p, 0x0);
    devid = x_read(edt_p, 0x1);

    for (i = 0; older_devices[i].prom != 0; i++)
    {
      if (mid == older_devices[i].mid && devid == older_devices[i].devid)
      {
        prom = older_devices[i].prom;
        break;
      }
    }

    edt_flash_reset(edt_p, 0);
    status =
        (u_short)x_readstat(edt_p) | 0xff00; /* returning stat as short so populate statx with ff */

    EDTPRINTF(edt_p, 2, ("\t\tedt_flash_prom_detect (X_): stat %04x prom %d\n", status, prom));
  }
  if (stat)
    *stat = status;
  return (prom);
}

EdtPromParmBlock *edt_get_parms_block(EdtPromData *pdata, char *id)

{
  EdtPromParmBlock *p;

  p = (EdtPromParmBlock *)pdata->extra_buf;

  if (p->size > (u_int)pdata->extra_size)
    return BAD_PARMS_BLOCK;

  while (p->size && ((u_char *)p - pdata->extra_buf) < pdata->extra_size)
  {
    if (strncmp(id, p->type, 4) == 0)
      return p;
    p = (EdtPromParmBlock *)((u_char *)p + p->size);
  }

  return NULL;
}

EdtPromParmBlock *edt_add_parmblock(EdtPromData *pdata, char *type, int datasize)
{
  EdtPromParmBlock *p;

  p = edt_get_parms_block(pdata, type);

  if (p)
    return NULL;

  if (pdata->extra_size + datasize + 8 > PROM_EXTRA_SIZE)
    return NULL;

  p = (EdtPromParmBlock *)(pdata->extra_buf + pdata->extra_size);
  strncpy(p->type, type, 4);
  p->size = datasize;

  pdata->extra_size += 8 + datasize;

  return p;
}

/* for newset (F16) boards, program the whole thing. Expect idsize is a multiple of 2
 * for others (BT, BT2), skip the header (irritating, but necessary for backwards compat)
 */
void edt_program_flashid(EdtDev *edt_p, u_int addr, u_char *idbuf, u_int idsize, int ftype,
                         int verbose)
{
  u_char *inptr = idbuf;
  u_int count, bytes_programmed = 0;
  const u_int blocksize = 16;

  if (verbose)
  {
    edt_flash_print16(edt_p, (u_int)idbuf, ftype);
    EDTPRINTF(edt_p, 0, ("  id string %s size %d\n", (char *)idbuf, idsize));
  }

  while (bytes_programmed < idsize)
  {
    count = (idsize - bytes_programmed >= blocksize) ? blocksize : idsize - bytes_programmed;

    edt_flash_block_program(edt_p, addr, inptr, count, ftype);

#ifndef _KERNEL
    EDTPRINTF(edt_p, 0, ("."));
    fflush(stdout);
#endif

    bytes_programmed += count;
    inptr += count;
    addr += (count / edt_flash_writesize(ftype));
  }
}

void edt_program_extra(EdtDev *edt_p, int promcode, EdtPromData *pdata, int sect)
{
  u_int id_addr;
  Edt_prominfo *ep = edt_get_prominfo(promcode);
  int sector;
  EdtPromIdAddresses addr;
  char *key_str = "XTR:";
  u_int size;

  sector = (sect == IS_DEFAULT_SECTOR) ? ep->defaultseg : sect;

  if ((id_addr = edt_flash_get_promaddrs(edt_p, promcode, sector, &addr)) == 1)
  {
    EDTPRINTF(edt_p, 0, ("invalid device; no ID info\n"));
    return;
  }

  /* Need it?  don't think so and it slows things down...
     edt_flash_reset(edt_p, ep->ftype);
   */

  if ((size = pdata->extra_size) > PROM_EXTRA_SIZE)
    return;

  size = pdata->extra_size;
  if (!edt_little_endian())
    size = swap32(size);

  addr.extra_data_addr = addr.extra_size_addr - (4 + pdata->extra_size);
  addr.extra_data_addr = addr.extra_size_addr - (4 + pdata->extra_size);

  edt_flash_block_program(edt_p, addr.extra_tag_addr, (u_char *)key_str, 4, ep->ftype);
  edt_flash_block_program(edt_p, addr.extra_size_addr, (u_char *)&size, 4, ep->ftype);
  if ((ep->ftype != FTYPE_SPI) && size)
    edt_flash_block_program(edt_p, addr.extra_data_addr, pdata->extra_buf, pdata->extra_size,
                            ep->ftype);
}

/*
 * on some sun platforms memset isn't in the kernel
 */
void edt_zero(void *s, size_t n)
{
#ifdef __sun
  bzero(s, n);
#else
  memset(s, 0, n);
#endif
}

static void edt_program_info_str(EdtDev *edt_p, u_int addr, char *str, char *tag, u_int size,
                                 int ftype)
{
  char
      info_str[1024]; /* arbitrarily large; just needs to be > MACLIST_SIZE unless/until something
                         bigger comes a long */

  if (size < 1 || size > 1024) /* reality check */
    return;

  if (!info_str)
    return;

  if (str[0])
  {
    /* Need it?  don't think so and it slows things down...
       edt_flash_reset(edt_p, ftype);
     */
    edt_zero(info_str, size);
    if (strncmp(str, "erase", 5) != 0)
    {
      strcpy(info_str, tag);
      strcat(info_str, str);
    }

    edt_flash_block_program(edt_p, addr, (u_char *)info_str, size, ftype);
  }
}

void edt_flash_program_prominfo(EdtDev *edt_p, int promcode, int segment, EdtPromData *pdata)
{
  Edt_prominfo *ep = edt_get_prominfo(promcode);
  EdtPromIdAddresses addr;

  edt_flash_reset(edt_p, ep->ftype);

  /* get addresses */
  addr.id_addr = edt_flash_get_promaddrs(edt_p, promcode, segment, &addr);

  EDTPRINTF(edt_p, 2,
            ("\nedt_flash_program_prominfo s%d addrs: maclist %x optsn %06x, xtrasize %06x, "
             "xtratag %06x,esn %06x, osn %06x, id %06x\n",
             segment, addr.maclist_addr, addr.optsn_addr, addr.extra_size_addr, addr.extra_tag_addr,
             addr.esn_addr, addr.osn_addr, addr.id_addr));

  edt_program_info_str(edt_p, addr.osn_addr, pdata->osn, "OSN:", OSN_SIZE, ep->ftype);
  EDTPRINTF(edt_p, 0, ("."));
  edt_program_info_str(edt_p, addr.esn_addr, pdata->esn, "ESN:", ESN_SIZE, ep->ftype);
  EDTPRINTF(edt_p, 0, ("."));
  edt_program_info_str(edt_p, addr.optsn_addr, pdata->optsn, "OPT:", OPTSN_SIZE, ep->ftype);
  EDTPRINTF(edt_p, 0, ("."));
  edt_program_extra(edt_p, promcode, pdata, segment);
  EDTPRINTF(edt_p, 0, ("."));
  edt_program_info_str(edt_p, addr.maclist_addr, pdata->maclist, "MAC:", MACLIST_SIZE, ep->ftype);
  EDTPRINTF(edt_p, 0, ("."));
}

/*
 * return a string containing the flash type; mainly for debug output
 */
char *edt_flash_type_string(int ftype)
{
  switch (ftype)
  {
    case FTYPE_X:
      return ("FTYPE_X");
    case FTYPE_BT:
      return ("FTYPE_BT");
    case FTYPE_LTX:
      return ("FTYPE_LTX");
    case FTYPE_SPI:
      return ("FTYPE_SPI");
    case FTYPE_F16A:
      return ("FTYPE_F16A");
    case FTYPE_F16:
      return ("FTYPE_F16");
    case FTYPE_BT2:
      return ("FTYPE_BT2");
    case FTYPE_MIC:
      return ("FTYPE_MIC");
    default:
      return ("UNKNOWN");
  }
}

/** Get the onboard PROM device info, as written to unused PROM space
 * by EDT before shipping, or thereafter via -I or -i options to
 * pciload. Caller should pass in a pointter to an EdtPromData
 * struct (defined in libedt.h); this subroutine reads the info out
 * from the specified segment and fill in the fields in the struct.
 *
 * @param edt_p pointer to edt device structure returned by #edt_open
 * @param promcode prom device code for this device, as returned by #edt_flash_prom_detect.
 * @param segment which segment to read the info from; for the default, call #edt_get_prominfo,
 * and use the defaultseg element from the Edt_prominfo struct it returns
 * @param pdata pointer to EdtPromData struct, into which the info will be read
 * @return void
 *
 */
void edt_read_prom_data(EdtDev *edt_p, int promcode, int segment, EdtPromData *pdata)

{
  Edt_prominfo *ep = edt_get_prominfo(promcode);
  char idbuf[PCI_ID_SIZE + 1];
  char oemsnbuf[OSN_SIZE + 1];
  char edtsnbuf[ESN_SIZE + 1];
  char optsnbuf[OPTSN_SIZE + 1];
  char macbuf[MACLIST_SIZE + 1];

  char *ret;

  EdtPromIdAddresses addr;

  edt_zero(pdata, sizeof(EdtPromData));

  /* get addresses */
  addr.id_addr = edt_flash_get_promaddrs(edt_p, promcode, segment, &addr);

  EDTPRINTF(edt_p, 2,
            ("\nedt_read_prom_data s%d addrs: maclist %x optsn %06x, xtrasize %06x, xtratag "
             "%06x,esn %06x, osn %06x, id %06x\n",
             segment, addr.maclist_addr, addr.optsn_addr, addr.extra_size_addr, addr.extra_tag_addr,
             addr.esn_addr, addr.osn_addr, addr.id_addr));

  /* BITFILE ID */
  if ((ret = edt_flash_string_read(edt_p, addr.id_addr, idbuf, PCI_ID_SIZE, ep->ftype)) != NULL)
    strncpy(pdata->id, ret, PCI_ID_SIZE);

  /* OSN */
  if ((ret = edt_flash_string_read(edt_p, addr.osn_addr, oemsnbuf, OSN_SIZE, ep->ftype)) != NULL)
  {
    if (strncmp(ret, "OSN:", 4) == 0)
    {
      strncpy(pdata->osn, &ret[4], OSN_SIZE);
      if (pdata->osn[strlen(pdata->osn) - 1] == ':')
        pdata->osn[strlen(pdata->osn) - 1] = '\0';
    }
  }

  /* ESN */
  if ((ret = edt_flash_string_read(edt_p, addr.esn_addr, edtsnbuf, ESN_SIZE, ep->ftype)) != NULL)
    if (strncmp(ret, "ESN:", 4) == 0)
      strncpy(pdata->esn, &ret[4], ESN_SIZE);

  /* EXTRA  */
  if (addr.extra_size)
  {
    if ((ret = (char *)edt_flash_block_read(edt_p, addr.extra_data_addr, pdata->extra_buf,
                                            addr.extra_size, ep->ftype)) != NULL)
    {
      pdata->extra_size = addr.extra_size;
    }
  }

  /* OPT SN */
  if ((ret = edt_flash_string_read(edt_p, addr.optsn_addr, optsnbuf, OPTSN_SIZE, ep->ftype)) !=
      NULL)
    if (strncmp(ret, "OPT:", 4) == 0)
      strncpy(pdata->optsn, &ret[4], OPTSN_SIZE);

  /* MACLIST */
  if ((ret = edt_flash_string_read(edt_p, addr.maclist_addr, macbuf, MACLIST_SIZE, ep->ftype)) !=
      NULL)
    if (strncmp(ret, "MAC:", 4) == 0)
      strncpy(pdata->maclist, &ret[4], MACLIST_SIZE - 4);

#ifndef _KERNEL
  if (pdata->esn)
    edt_parse_devinfo(pdata->esn, &pdata->ei);
#endif
}

/*************************************************************************
 * general /utility routines
 *
 *************************************************************************
 */

/*
 * check a string for forward or back slashes
 */
int has_slashes(char *name)
{
  char *p = name;

  while (*p)
  {
    if ((*p == '/') || (*p == '\\'))
      return 1;
    ++p;
  }
  return 0;
}

#ifndef _KERNEL

static int isdigit_str(char *s)
{
  u_int i;

  for (i = 0; i < strlen(s); i++)
    if ((s[i]) < '0' || s[i] > '9')
      return 0;
  return 1;
}

/** Parse the board's embedded information string.
 *
 * During manufacturing programming, EDT embeds selected information is embedded into an unused
 * area of the FPGA PROM. This information is preserved across reloads
 * (via pciload, hubload, etc.) unless options to overwrite are invoked in one
 * of those utilities. This subroutine takes as an argument the full information
 * string, as retrieved from #edt_get_esn, #edt_get_osn or #edt_get_sns, into the
 * fields indicated by the #Edt_embinfo structure.
 *
 * (To see the information string, run \e pciload with no arguments.)
 *
 * @note Information embedding was implemented in Sept. 2004; boards shipped before
 * that time will yield a string with all NULLS unless the board's FPGA
 * has since been updated with the embedded information.
 *
 * @param str embedded inforamtion string, with information from one of
 * the serial number retrieval subroutines (edt_get_esn, etc.)
 * @param ei #Edt_embinfo structure into which the the parsed information will be put
 * @see edt_readinfo, edt_get_esn, edt_fmt_pn
 * @return 0 on success, -1 on error (input string not valid or too long)
 */
int edt_parse_devinfo(char *str, Edt_embinfo *ei)
{
  u_int i, j = 0;
  int n;
  int nfields = 0;
  char sn[EDT_STRBUF_SIZE], pn[EDT_STRBUF_SIZE];
  char ifx[EDT_STRBUF_SIZE], rev[EDT_STRBUF_SIZE], clock[EDT_STRBUF_SIZE], options[EDT_STRBUF_SIZE];
  char tmpstr[EDT_STRBUF_SIZE * 10];

  if (strlen(str) >= EDT_STRBUF_SIZE * 4)
    return -1;

  sn[0] = pn[0] = clock[0] = options[0] = ifx[0] = '\0';

  for (i = 0; i < strlen(str); i++)
  {
    tmpstr[j++] = str[i];
    if (str[i] == ':')
    {
      ++nfields;
      if (str[i + 1] == ':') /* insert space if empty field */
        tmpstr[j++] = ' ';
    }
  }
  tmpstr[j] = '\0';

  if (nfields < 4)
  {
    return -1;
  }

  n = sscanf(tmpstr, "%[^:]:%[^:]:%[^:]:%[^:]:%[^:]:%[^:]:", sn, pn, clock, options, rev, ifx);

  /* first pass we had 10-digit p/ns; still may be some around that have that */
  if (strlen(sn) > 10)
    sn[10] = '\0';

  if ((strlen(pn) > 10) || (strlen(options) > 10) || !isdigit_str(clock))
    return -1;

  if (n < 4)
    return -1;

  if (n < 5)
    rev[0] = '\0';

  if (n < 6)
    ifx[0] = '\0';

  strcpy(ei->sn, sn);
  strcpy(ei->pn, pn);
  strcpy(ei->opt, options);
  ei->clock = edt_strtol(clock, 0, 0);
  ei->rev = edt_strtol(rev, 0, 0);
  strcpy(ei->ifx, ifx);

  return 0;
}

/* alias to old name in case anyone still uses it */
int edt_parse_esn(char *str, Edt_embinfo *ei)
{
  return edt_parse_devinfo(str, ei);
}

#endif

/***************************************************************************
 * Micron flash: VisionLink, A5, ??
 ***************************************************************************/

int mic_get_manufacturer_info(EdtDev *edt_p)
{
  u_char read_buffer[20];

  micron_flash_size = 0;

  mic_reset(edt_p);

  mic_exit_4_byte_mode(edt_p);
  micron_4byte_addressing = 0;

  mic_read_flash(edt_p, MIC_RDID, 0, read_buffer,
                 20);  // if the last read was aborted midstream, it needs this twice
  mic_read_flash(edt_p, MIC_RDID, 0, read_buffer, 20);

  if (read_buffer[0] == MIC_JEDEC_ID)
  {
    if (read_buffer[2] == MICRON_JEDEC_SIZE_64MB)  // 0x17
    {
      micron_flash_size = 0x4000000;  // 64Mb
      micron_jedec_flash_type = "25Q064A";
    }

    else if (read_buffer[2] == MICRON_JEDEC_SIZE_128MB)  // 0x18
    {
      micron_flash_size = 0x8000000;  // 128Mb
      micron_jedec_flash_type = "25Q128A";
    }

    else if (read_buffer[2] == MICRON_JEDEC_SIZE_256MB)  // 0x19
    {
      micron_flash_size = 0x10000000;  // 256Mb
      micron_jedec_flash_type = "25Q256A";
      mic_enter_4_byte_mode(edt_p);
      micron_4byte_addressing = 1;
    }
    else
    {
      micron_flash_size = 0;
      micron_jedec_flash_type = "unknown";
      return 0;  // unknown device
    }

    // populate the prom-specific info string
    sprintf(micron_chip_specific_info, "mfg. ID 0x%02x, type 0x%02x, cap. 0x%02x, size 0x%02x",
            read_buffer[0], read_buffer[1], read_buffer[2], micron_flash_size);

    return 1;
  }
  else  // unknown JEDEC id?
  {
    EDTPRINTF(edt_p, 0, ("Unknown Micron JEDEC manufacturer ID"));
    return 0;
  }
}

static u_int mic_write(EdtDev *edt_p, u_int val)
{
  u_int cmd_data;

  cmd_data = (val & 0xffff);

  edt_set(edt_p, F16_P4_CMDDATA_REG, cmd_data);
  (void)edt_get(edt_p, F16_P4_CMDDATA_REG);

  return 0;
}

static u_int mic_read(EdtDev *edt_p)
{
  u_int read_data;

  (void)edt_get(edt_p, F16_P4_CMDDATA_REG);
  read_data = edt_get(edt_p, F16_P4_CMDDATA_REG);

  return read_data;
}

/*
 * The Write Enable operation sets the write enable latch bit.
 * To execute the write enable command s# is drive low, held low
 * until 8th bit of command is latched in, then driven high.
 * The write enable latch bit must be set before every program, erase and
 * write command. If s# is not driven high after command code has been
 * latched in then command is not executed, flag status register error bits
 * are not set and the write enable latch remains cleared to default setting
 * of 0
 */
static u_int mic_write_enable(EdtDev *edt_p)
{
  u_int stb;
  u_int read_data = 0;
  /* Enable SPI interface */
  mic_write(edt_p, MIC_EN);

  /* Set write enable */
  stb = MIC_STB;
  mic_write(edt_p, (MIC_EN + stb + SPI_WREN));

  /* Disable chip select */
  mic_write(edt_p, (MIC_EN + stb + MIC_CS));

  /* Enable chip select */
  mic_write(edt_p, (MIC_EN + stb));

  /* Check latch bit is set */
  stb ^= MIC_STB;
  mic_write(edt_p, (MIC_EN + stb + SPI_RDSR));

  /* Write out null byte to read status register */
  stb ^= MIC_STB;
  mic_write(edt_p, (MIC_EN + stb));
  read_data = mic_read(edt_p) & 0xFF;
  edt_delay(2);

  /* Disable SPI interface */
  mic_write(edt_p, MIC_CS);

  /* Return true if write enable latch bit(Status Register bit 1) is set */
  read_data &= 0x02;

  if (read_data)
    return 1;
  else
    return 0;
}

static u_int mic_write_disable(EdtDev *edt_p)
{
  u_int stb;
  u_int read_data = 0;

  /* Enable SPI interface */
  mic_write(edt_p, MIC_EN);

  /* Set write disable */
  stb = MIC_STB;
  mic_write(edt_p, (MIC_EN + stb + SPI_WRDI));

  /* Disable chip select */
  mic_write(edt_p, (MIC_EN + stb + MIC_CS));

  /* Enable chip select */
  mic_write(edt_p, (MIC_EN + stb));

  /* Check latch bit is cleared */
  stb ^= MIC_STB;
  mic_write(edt_p, (MIC_EN + stb + SPI_RDSR));

  /* Write out null byte, then read in data */
  stb ^= MIC_STB;
  mic_write(edt_p, (MIC_EN + stb));
  read_data = mic_read(edt_p) & 0xFF;
#ifndef _KERNEL
  EDTPRINTF(edt_p, 0, ("Write Disable Read Status : %d\n", read_data));
#endif

  /* Disable SPI interface */
  mic_write(edt_p, MIC_CS);

  /* Return true if write enable latch bit(Status Register bit 1) is cleared */
  read_data &= 0x02;

  if (read_data)
    return 0;
  else
    return 1;
}

/*
 * Function executes read status register, including update for 32/24 bit devices
 */
static u_int mic_read_flash(EdtDev *edt_p, u_int command, u_int address, u_char *read_bytes,
                            u_int read_num_bytes)
{
  u_int address_buf[4] = {0, 0, 0, 0};
  u_int addr_loop = 3;
  u_int i = 0;
  u_int stb;

  /* Does not apply to the PE8VLCLS_ID - the VL-CLS read MIC_RDID (in mic_get_manufacturer_info)
   * does not return accurate values; which causes micron_4byte_addressing to be 0 when it
   * should be 1 for the VL-CLS allowing entry into this loop which prints an error.
   */
  if (edt_p->devid != PE8VLCLS_ID && (!micron_4byte_addressing) && (command == SPI_READ))
  {
    /* For 256Mb part we must set the extended address register to access
     * the upper half of the part when using 3 byte addressing. */
    mic_write_ext_addr_register(edt_p, (address & 0x01000000) >> 24);
  }

  /* Enable SPI interface */
  mic_write(edt_p, MIC_EN);

  /* Send command */
  stb = MIC_STB;
  mic_write(edt_p, (MIC_EN + stb + command));

  if (command == SPI_READ)
  {
    if (micron_4byte_addressing)
    {
      address_buf[0] = ((address & 0xFF000000) >> 24);
      address_buf[1] = ((address & 0x00FF0000) >> 16);
      address_buf[2] = ((address & 0x0000FF00) >> 8);
      address_buf[3] = (address & 0x000000FF);
      addr_loop = 4;
    }
    else
    {
      address_buf[0] = ((address & 0x00FF0000) >> 16);
      address_buf[1] = ((address & 0x0000FF00) >> 8);
      address_buf[2] = (address & 0x000000FF);
      addr_loop = 3;
    }

    /* Write Address */
    for (i = 0; i < addr_loop; i++)
    {
      stb ^= MIC_STB;
      mic_write(edt_p, (MIC_EN + stb + address_buf[i]));
    }
  }

  /* Read data back (send a byte, get a byte) */
  for (i = 0; i < read_num_bytes; i++)
  {
    stb ^= MIC_STB;
    if (command == SPI_READ) /* Reverse bits that are read back in from main flash */
      mic_write(edt_p, (MIC_EN + stb + MIC_LSB));
    else
      mic_write(edt_p, (MIC_EN + stb));
    read_bytes[i] = (u_char)(mic_read(edt_p) & 0xFF);
  }

  /* Disable SPI interface */
  mic_write(edt_p, MIC_CS);
  /* mic_write(edt_p, 0); */

  return 1;
}

/* Get Status "register", micron flash devices.
 *
 * Micron flash boards (VisionLink and A5) don't have a status register, or at least
 * not like other devices. VisionLink has no PAL ID register so we just dummy that
 * up and dummy it up in a status "register" that emulates that.
 *
 * A5 (& possible future Micron boards?) does have the PAL ID register but the
 * protected jumper deal is different (for both) -- reads the status of the
 * protected sector(s) so we get that and tack it on to the status "register"
 * (jumper pos. in bits 0-1, PAL ID in 3-7)
 * */
static u_short mic_get_status(EdtDev *edt_p)
{
  u_short status;
  u_char cstat;
  u_short idbits = 0x0d;
  u_short xidbits = 0x01;

  mic_read_flash(edt_p, SPI_RDSR, 0, &cstat, 1);

  if ((edt_p->devid == PE4DVVL_ID) || (edt_p->devid == PE1DVVL_ID))
  {
    status = (xidbits << 8) | (idbits << 2);
  }
  else if (edt_p->devid == PE8VLCLS_ID)
  {
    // Reset xidbits and idbits to correspond to CLS (37th entry in EpInfo).
    idbits = 0x0e;
    xidbits = 0x07;
    status = (0x07 << 8) | (0x0e << 2);
  }
  else /* A5 and ...? */
  {
    status = (mic_get_pal_id(edt_p));
  }

  /* straps (jumpers) */
  status |= (cstat & 0x5c) ? 0x1 : 0x0;

  return status;
}

/*
 * Function executes write status register, write nonvolatile configuration register,
 * program and erase of the SPI flash
 * Checks the write in progress bit after the miniumum time, then checks it after each loop,
 * pausing between checks, time is in milliseconds
 */
static u_int mic_write_flash(EdtDev *edt_p, u_int command, u_int address, u_char *write_bytes,
                             u_int write_num_bytes, u_int time_min, u_int timeout_loops,
                             u_int timeout_sleep)
{
  u_int stb;
  u_int i = 0;
  u_int addr_loop = 3;
  u_int write_in_progress;
  u_int address_buf[4] = {0, 0, 0, 0};

  if ((!micron_4byte_addressing) &&
      ((command == SPI_PP) || (command == SPI_SE) || (command == SPI_SSE)))
  {
    /* For 256Mb part we must set the extended address register to access
     * the upper half of the part when using 3 byte addressing. Note,
     * mic_write_ext_addr_register calls this function but won't get into
     * infinite recursion due to this if statement. */
    mic_write_ext_addr_register(edt_p, (address & 0x01000000) >> 24);
  }

  /* Set write enable bit */
  if (!(mic_write_enable(edt_p)))
  {
#ifndef _KERNEL
    EDTPRINTF(edt_p, 0, ("Failure to set write enable bit\n"));
#endif
    return 0;
  }

  /* Enable SPI interface */
  mic_write(edt_p, MIC_EN);

  stb = MIC_STB;
  /* Send Command */
  mic_write(edt_p, (MIC_EN + stb + command));

  /* Write out address if needed */
  if ((command == SPI_PP) || (command == SPI_SE) || (command == SPI_SSE))
  {
    if (micron_4byte_addressing)
    {
      address_buf[0] = ((address & 0xFF000000) >> 24);
      address_buf[1] = ((address & 0x00FF0000) >> 16);
      address_buf[2] = ((address & 0x0000FF00) >> 8);
      address_buf[3] = (address & 0x000000FF);
      addr_loop = 4;
    }
    else
    {
      address_buf[0] = ((address & 0x00FF0000) >> 16);
      address_buf[1] = ((address & 0x0000FF00) >> 8);
      address_buf[2] = (address & 0x000000FF);
      addr_loop = 3;
    }

    /* Address */
    for (i = 0; i < addr_loop; i++)
    {
      stb ^= MIC_STB;
      mic_write(edt_p, (MIC_EN + stb + address_buf[i]));
    }
  }

  /* Write out bytes if needed */
  for (i = 0; i < write_num_bytes; i++)
  {
    stb ^= MIC_STB;
    if (command == SPI_PP)
    {
      /* program seding out LSB first for writing Altera data to flash */
      mic_write(edt_p, (MIC_EN + stb + MIC_LSB + write_bytes[i]));
    }
    else
    {
      mic_write(edt_p, (MIC_EN + stb + write_bytes[i]));
    }
  }

  /* Disable CS */
  mic_write(edt_p, (MIC_EN + stb + MIC_CS));

  edt_delay(time_min);

  /* Enable  CS */
  mic_write(edt_p, (MIC_EN + stb));

  /* Read status register */
  stb ^= MIC_STB;
  mic_write(edt_p, (MIC_EN + stb + SPI_RDSR));

  i = 0;
  write_in_progress = 1;
  while ((i < timeout_loops) && write_in_progress)
  {
    stb ^= MIC_STB;
    mic_write(edt_p, (MIC_EN + stb));
    write_in_progress = (mic_read(edt_p) & 0x01);
    if (write_in_progress)
    {
      edt_delay(timeout_sleep);
      i++;
    }
  }

  /* Disable SPI interface */
  mic_write(edt_p, MIC_CS);

  if (write_in_progress)
    return -1;
  else
    return 0;
}

/* Sub sector erase, cycle time is from .25 to .8 seconds */
static u_int mic_subsector_erase(EdtDev *edt_p, u_int address)
{
  return mic_write_flash(edt_p, SPI_SSE, address, 0, 0, 250, 15, 50);
}

/* Sector erase, cycle time is from .7 to 3 seconds */
static u_int mic_sector_erase(EdtDev *edt_p, u_int address)
{
  return mic_write_flash(edt_p, SPI_SE, address, 0, 0, 700, 50, 50);
}

/* Bulk erase, cycle time is from 60-120 seconds */
static u_int mic_bulk_erase(EdtDev *edt_p, u_int address)
{
  return mic_write_flash(edt_p, SPI_BE, 0, 0, 0, 60000, 650, 100);
}

/* Page program a max of 256 bytes(one page)
 * Programming cycle time is .5 to 5 ms
 * Bytes are sent out LSB first from mic_write_flash function for Altera active serial programming
 */
static u_int mic_page_program(EdtDev *edt_p, u_int address, u_char *write_bytes,
                              u_int write_num_bytes)
{
  u_int write_count;
  if (write_num_bytes > 256)
    write_count = 256;
  else
    write_count = write_num_bytes;

  return mic_write_flash(edt_p, SPI_PP, address, write_bytes, write_count, 1, 6, 1);
}

/* Write Status Register, write cycle time is from 1.3 to 8 ms */
static u_int mic_write_status_register(EdtDev *edt_p, u_char status_val)
{
  return mic_write_flash(edt_p, SPI_WRSR, 0, &status_val, 1, 2, 8, 1);
}

void mic_reset(EdtDev *edt_p)
{
  /* don't think we need to do anything */
}

void edt_dump_flash(EdtDev *edt_p, u_int addr, u_int size)
{
  u_int i;
  u_char cbuf[16];
  u_int nblocks = (size + 15) / 16;
  int ftype = edt_flash_get_ftype(edt_p);

  if (ftype == FTYPE_F16A)
  {
    f16a_dump_flash(edt_p, addr, size);
    return;
  }

  addr &= ~0xf;

  for (i = 0; i < nblocks; i++)
  {
    int j;

    EDTPRINTF(edt_p, 0, ("%08x: ", addr));
    edt_flash_block_read(edt_p, addr, cbuf, 16, ftype);
    for (j = 0; j < 16; j++)
    {
      EDTPRINTF(edt_p, 0, ("%02x ", (u_char)(cbuf[j])));
    }

    EDTPRINTF(edt_p, 0, (" "));

    for (j = 0; j < 16; j++)
    {
      EDTPRINTF(edt_p, 0, ("%c", ((cbuf[j] >= 0x20) && (cbuf[j] <= 0x7e)) ? (char)(cbuf[j]) : '.'));
    }

    if (edt_is_16bit_prom(edt_p))
      addr += 8;
    else
      addr += 16;
    EDTPRINTF(edt_p, 0, ("\n"));
  }
  EDTPRINTF(edt_p, 0, ("\n"));
}

/* Read extended address register */
static u_char mic_read_ext_addr_register(EdtDev *edt_p)
{
  u_char read_buffer[1];
  mic_read_flash(edt_p, SPI_RDEA, 0, read_buffer, 1);

  return read_buffer[0];
}

/* Write extended address register, write cycle time is 40ns typ */
static u_int mic_write_ext_addr_register(EdtDev *edt_p, u_char config_val)
{
  u_char write_buffer[1];

  write_buffer[0] = (config_val & 0xFF);

  return mic_write_flash(edt_p, SPI_WREA, 0, write_buffer, 1, 2, 5, 1);
}

static u_int mic_enter_4_byte_mode(EdtDev *edt_p)
{
  return mic_write_flash(edt_p, MIC_EN4B, 0, 0, 0, 1, 5, 1);
}

static u_int mic_exit_4_byte_mode(EdtDev *edt_p)
{
  return mic_write_flash(edt_p, MIC_EX4B, 0, 0, 0, 1, 5, 1);
}

/*
 * mic_a5 stuff lifted from micron_flash_pe8a5.c
 */
#define MIC_CMD_NOP 0x0
#define MIC_CMD_LD_CTRL_REG 0x2
#define MIC_CMD_RD_PAL_ID 0x8
#define MIC_CMD_RD_PAL_VER 0x9
#define MIC_CMD_RD_CTRL_REG 0xC
#define MIC_CMD_IDLE 0xF

/*
 * Read which flash memory chip is addressed.  Like selecting the "sector"
 * on most other EDT boards.
 *  When sector = 0, then addressing boot flash "sector".
 *  When sector = 1, then addressing protected flash "sector".
 */
static uint_t mic_get_jumper(EdtDev *edt_p)
{
  return mic_rd_prog_reg(edt_p, MIC_CMD_RD_CTRL_REG);
}

static uint_t mic_get_pal_id(EdtDev *edt_p)
{
  return mic_rd_prog_reg(edt_p, MIC_CMD_RD_PAL_ID);
}

/*
 * Read indirect register in FPGA that mimics CPLD registers on S5. Valid read
 * registers include:
 *  PAL_ID:
 *      15-12:  reserved
 *      11- 8:  build config (set by resistors)
 *       7- 0:  reserved (hardcoded to 0x30)
 *  PAL_VER:
 *      15-12:  version
 *      11- 4:  reserved
 *       3- 0:  build config (set by resistors)
 *  CTRL_REG:
 *      15- 4:  reserved
 *       3- 1:  reserved (but r/w)
 *          0:  sector select; set to 1 to address protected flash "sector"
 *  others:  (MIC_CMD_IDLE, MIC_CMD_NOP, etc)
 *      Flash data/interface.
 */
static uint_t mic_rd_prog_reg(EdtDev *edt_p, u_short reg)
{
  uint_t rdata = 0;
  uint_t wdata = 0;
  wdata = (reg & 0xf) << 16;
  edt_set(edt_p, F16_P4_CMDDATA_REG, wdata);
  rdata = edt_get(edt_p, F16_P4_CMDDATA_REG);
  rdata &= 0xffff;
  // release interface
  wdata = (MIC_CMD_NOP & 0xf) << 16;  // or MIC_CMD_IDLE
  edt_set(edt_p, F16_P4_CMDDATA_REG, wdata);
  return rdata;
}

/*
 * Select which flash memory chip is addressed.  Like selecting the "sector"
 * on most other EDT boards.
 *  When sector = 0, then addressing boot flash "sector".
 *  When sector = 1, then addressing protected flash "sector".
 */
static void mic_set_device(EdtDev *edt_p, uint_t sector)
{
  if (sector)
    mic_wr_prog_reg(edt_p, MIC_CMD_LD_CTRL_REG, 1);
  else
    mic_wr_prog_reg(edt_p, MIC_CMD_LD_CTRL_REG, 0);
}

/*
 * Write indirect register in FPGA that mimics CPLD registers on S5. Valid
 * write registers include:
 *  CTRL_REG:
 *      15- 4:  reserved
 *       3- 1:  reserved (but r/w)
 *          0:  sector select; set to 1 to address protected flash "sector"
 *  others:  (CMD_IDLE, MIC_CMD_NOP, etc)
 *      Flash data/interface.
 */
static void mic_wr_prog_reg(EdtDev *edt_p, uint_t reg, uint_t val)
{
  uint_t wdata = 0;
  wdata = ((reg & 0xf) << 16) | (val & 0xffff);
  edt_set(edt_p, F16_P4_CMDDATA_REG, wdata);
  // release interface
  wdata = (MIC_CMD_NOP & 0xf) << 16;  // or CMD_IDLE
  edt_set(edt_p, F16_P4_CMDDATA_REG, wdata);
}
