/*
 * @file edt_optstring.c 
 * FPGA configuration file option string functions (read/write/parse) for the edt library
 *
 * Copyright (c) 2018 Engineering Design Team (EDT), Inc.
 * All rights reserved.
 *
 * This file is subject to the terms and conditions of the EULA defined at
 * www.edt.com/terms-of-use
 *
 * Technical Contact: tech@edt.com
 */
#include "edtinc.h"

#include <ctype.h> /* isdigit */
#include <stdlib.h>

#include "edt_bitload.h"
#include "edt_optstring.h"
#ifdef PCD
#include "edt_threep.h"
#include "lib_e3t3.h"
#endif

/* extended board information. (configuration and installed options) */
char *srxl_extbdid_info[] = {
    "Spartan 3 XC3S1500 -4, Two 4-channel DDC Graychips (GC4016)", /* idx = 0 */
    "Spartan 3 XC3S1500 -4, One 4-channel DDC Graychip (GC4016)",  /* idx = 1 */
    "Spartan 3 XC3S1500 -4, NO DDC Graychips installed!",          /* idx = 2 */
    NULL};

int edt_ui_loaded(EdtDev *edt_p)

{
  return (edt_reg_read(edt_p, EDT_DMA_CFG) & X_DONE) != 0;
}

/**
* @brief edt_read_old_option_string
*
* @param edt_p pointer to device structure
*
* @param s String to fill in
*
* @returns 0 on success, -1 on failure
*
*
*/
int edt_read_old_option_string(EdtDev *edt_p, char *s)

{
  int index = 0;

  int ch;

  edt_reg_write(edt_p, EDT_OLD_BITFILE_STRING, 0);
  ch = edt_reg_read(edt_p, EDT_OLD_BITFILE_STRING);

  if (ch == 0 || ch == 0xff)
  {
    s[0] = 0;
    return -1;
  }
  else
  {
    while (ch && index < 16)
    {
      edt_reg_write(edt_p, EDT_OLD_BITFILE_STRING, index);
      ch = edt_reg_read(edt_p, EDT_OLD_BITFILE_STRING);
      s[index] = ch;
      index++;
    }
  }

  s[index] = 0;

  return 0;
}

/**
* @brief Read the option string from an OCM mezzanine bitfile
*
* @param edt_p pointer to device structure
*
* @param s String to fill in
*
* @param offset either 0 or  OFFSET_CH1_MEZ depending on channel
*
* @returns 0 on success, -1 on failure
*
*/
#ifdef PCD /* because mezz boards are N/A in pdv land */
int edt_read_mezz_option_string(EdtDev *edt_p, char *s, int offset)

{
  int index = 0;

  int ch;

  u_int reg_addr = EDT_MEZZ_BITFILE_STRING + offset;

  if (edt_p->mezz.id == MEZZ_THREEP)
    reg_addr = THREEP_MEZZ_STRING;

  edt_reg_write(edt_p, reg_addr, 0);
  ch = edt_reg_read(edt_p, reg_addr);

  if (ch == 0 || ch == 0xff)
  {
    return -1;
  }
  else
  {
    while (ch && index < 31)
    {
      edt_reg_write(edt_p, reg_addr, index);
      ch = edt_reg_read(edt_p, reg_addr);
      s[index] = ch;
      index++;
    }
  }

  s[index] = 0;
  /* set pointer back to first cfg */
  edt_reg_write(edt_p, reg_addr, 0);

  return 0;
}
#endif

/**
* @brief Read a new-type option string from the interface bitfile
*
* @param edt_p
*
* @param s String to fill in
*
* @param rev_p pointer to int to return the BITFILE_VERSION register
*
* @returns 0 for success, -1 for failure (string not present)
*
*/
int edt_read_option_string(EdtDev *edt_p, char *s, int *rev_p)

{
  int index = 0;
  int rev;

  int ch;

  rev = edt_reg_read(edt_p, EDT_BITFILE_VERSION);

  if (rev_p)
    *rev_p = rev;

  edt_reg_write(edt_p, EDT_BITFILE_STRING, 0);
  ch = edt_reg_read(edt_p, EDT_BITFILE_STRING);

  /* 0xff usually indicates bitfile hasn't implented that register;
  * 0xcc means no bitfile has been loaded */
  if (ch == 0 || ch == 0xff || ch == 0xcc)
  {
    if (rev_p)
      *rev_p = 0;

    return -1;
  }
  else
  {
    while (ch && index < 64)
    {
      edt_reg_write(edt_p, EDT_BITFILE_STRING, index);
      ch = edt_reg_read(edt_p, EDT_BITFILE_STRING);
      s[index] = ch;
      index++;
    }
  }

  s[index] = 0;

  return 0;
}

static char *baseboard_names[] = {"unknown", "pciss", "pcigs", "cda", NULL};

static char *mezzanine_names[] = {"RS422 I/O Board",
                                  "LVDS I/O Board",
                                  "ID is Extended, this is not a valid ID!",
                                  "RS422_12_LVDS_8",
                                  "SSE (high speed serial ECL) I/O Board",
                                  "HRC for E4/STM-1/OC-3 I/O",
                                  "OCM Board",
                                  "ComboII I/O LVDS Board",
                                  "ECL I/O Board",
                                  "TLK1501 I/O Board",
                                  "SRXL Board",
                                  "COMBOIII RS422",
                                  "COMBOIII LVDS",
                                  "COMBOIII ECL",
                                  "COMBOII I/O RS422",
                                  "COMBO I/O Board", /* idx = 15 */
                                  "16TE3IO Board",   /* idx = 16 */
                                  "OC192",           /* idx = 17 */
                                  "3X3G",            /* idx = 18 */
                                  "MSDV",
                                  "SRXL2",
                                  "NET10G",
                                  "DRX",
                                  "DDSP",
                                  "SRXL2_IDMLBM",
                                  "SRXL2_IDMIDM",
                                  "SRXL2_IMMIMM",
                                  "SRXL2_IMMLBM",
                                  "SRXL2_IDMIMM",
                                  "DRX16",
                                  "OCM2P7G",
                                  "THREEP",
                                  "FAN",
                                  "V4ACL",
                                  "",
                                  0};

/**
* @brief edt_mezz_name
*
* @param mezz.id The mezzanine id returned by edt_get_board_id
*
* @returns a static string identifying the board associated with mezz.id
*
*
*/
char *edt_mezz_name(int mezz_id)

{
  if (mezz_id >= 0 && mezz_id <= MEZZ_LAST && mezzanine_names[mezz_id])
    return mezzanine_names[mezz_id];

  return "unknown id";
}

char *edt_default_base_for_mezz(EdtDev *edt_p)

{
  switch (edt_p->mezz.id)
  {
    case MEZZ_OCM:
      return "ocm.bit";
      break;

    case MEZZ_OC192:
      return "oc192.bit";
      break;

    case MEZZ_SRXL:
      if (edtdev_channels_from_type(edt_p) == 16)
      {
        if (ID_IS_LX(edt_p->devid))
        {
          if (edt_p->devid == PE8LX16_ID)
            return "srxl_2in.bit";
          else
          {
            fputs(
                "\nedt_optstring.c: edt_default_base_for_mezz:\n\tERROR:  SRXL on LX requires PCI "
                "FPGA pe8lx16.bit loaded in flash\n\n",
                stderr);
            return NULL;
          }
        }
        else
          return "srxl_16io.bit";
      }
      else
      {
        return "srxl_4io.bit";
      }

    case MEZZ_SRXL2_IDM_LBM:
    case MEZZ_SRXL2_IDM_IDM:
    case MEZZ_SRXL2_IMM_IMM:
    case MEZZ_SRXL2_IMM_LBM:
    case MEZZ_SRXL2_IDM_IMM:
    case MEZZ_SRXL2:
      if (ID_IS_LX(edt_p->devid))
        return "srxl2_lx16.bit";
      else if (edtdev_channels_from_type(edt_p) == 16)
        return "srxl2_16in.bit";
      else
        return "srxl2_4in.bit";

    case MEZZ_DRX16:
      return "drx16_2in.bit";

    case MEZZ_SSE:
      return "eclopt_sse.bit";

    case MEZZ_NET10G:
      return "net10g.bit";
    case MEZZ_MSDV:
      return "msdv.bit";
    case MEZZ_DDSP:
      return "ddsp.bit";

    case MEZZ_THREEP:
      return "threep.bit";

    case MEZZ_V4ACL:
      return "v4acl_lx16.bit";
  }

  return NULL;
}

static char *get_next_field(char *work, int target, char *errorstr)

{
  char *next;

  next = strchr(work, target);

  if (next)
  {
    *next = 0;
    return next + 1;
  }
  else
  {
    edt_msg(EDTLIB_MSG_WARNING, "Parse Error: '%c' expected %s\n", target, errorstr);
    return NULL;
  }
}

/**


*/

/**
* @brief edt_parse_option_string
*
* @param s
*
* @param bfd
*
* @returns
*
*
*/
int edt_parse_option_string(char *s, EdtBitfileDescriptor *bfd)

{
  char work[80];

  char *dma_intfc;
  char *mezz_type;
  char *mezz_vhdl;
  char *version_major;
  char *version_rev;
  char *date;
  char *customchannels;
  char *totalchannels;

  bfd->string_type = 2;

  /* attempt to split string */

  if (s != bfd->optionstr)
    strcpy(bfd->optionstr, s);

  strcpy(work, bfd->optionstr);
  work[2] = 0;

  if (!strcasecmp(work, "ss"))
  {
    bfd->ostr.board_type = EDT_SS_TYPE;
  }
  else if (!strcasecmp(work, "lx"))
  {
    bfd->ostr.board_type = EDT_LX_TYPE;
  }
  else if (!strcasecmp(work, "gs"))
  {
    bfd->ostr.board_type = EDT_GS_TYPE;
  }
  else if (!strcasecmp(work, "cd"))
  {
    bfd->ostr.board_type = EDT_CD_TYPE;
  }
  else if (!strcasecmp(work, "LC"))
  {
    bfd->ostr.board_type = EDT_LC_TYPE;
  }
  else
  {
    edt_msg(EDTLIB_MSG_WARNING, "Parse error: first two option chars (%s) should be ss or gs\n",
            work);
    return -1;
  }

  strcpy(work, bfd->optionstr);

  dma_intfc = work + 2;

  if ((mezz_type = get_next_field(dma_intfc, '_', "after DMA interface type")) == NULL)
    return -1;
  if ((mezz_vhdl = get_next_field(mezz_type, '_', "after mezzanine type")) == NULL)
    return -1;
  if ((version_major = get_next_field(mezz_vhdl, ' ', "after vhdl name")) == NULL)
    return -1;
  if ((version_rev = get_next_field(version_major, '.', "after version major #")) == NULL)
    return -1;
  if ((date = get_next_field(version_rev, ' ', "after version rev #")) == NULL)
    return -1;
  if ((customchannels = get_next_field(date, ' ', "after date")) == NULL)
    return -1;

  if (*customchannels == '(')
    customchannels++;
  else
  {
    edt_msg(EDTLIB_MSG_WARNING, "Parse Error: '(' expected after date\n");
    return -1;
  }

  if ((totalchannels = get_next_field(customchannels, ',', "after custom channels")) == NULL)
    return -1;

  if (get_next_field(totalchannels, ')', "after total channels") == NULL)
    return -1;

  bfd->ostr.DMA_channels = atoi(dma_intfc);

  if (bfd->ostr.DMA_channels != 1 && bfd->ostr.DMA_channels != 4 && bfd->ostr.DMA_channels != 16 &&
      bfd->ostr.DMA_channels != 32)
  {
    edt_msg(EDTLIB_MSG_WARNING, "Parse error: DMA interface # (%d) should 1, 4, or 16\n",
            bfd->ostr.DMA_channels);
    return -1;
  }

  strcpy(bfd->ostr.mezzanine_type, mezz_type);
  strcpy(bfd->ostr.filename, mezz_vhdl);
  strncpy(bfd->ostr.date, date, 11);

  bfd->ostr.version_number = atoi(version_major);
  bfd->ostr.rev_number = atoi(version_rev);

  bfd->ostr.custom_DMA_channels = strtol(customchannels, 0, 16);
  bfd->ostr.available_DMA_channels = strtol(totalchannels, 0, 16);

  return 0;
}

/**
* @brief edt_print_board_description
*
* @param edt_p
*
* edt_print_board_description: Prints all known fields
* from an EdtDev device.
*
*/
void edt_print_board_description(EdtDev *edt_p)

{
  printf("%-24s : %02x (%s)\n", "Board Type", edt_p->devid, edt_idstr(edt_p->devid));
  printf("%-24s : %d\n", "PCI Channels", edt_p->DMA_channels);
  printf("%-24s : %s\n", "Bitfile Loaded", edt_p->bfd.bitfile_name);

  printf("%-24s : %02x %s\n", "Mezzanine ID", edt_p->mezz.id, edt_mezz_name(edt_p->mezz.id));

  printf("%-24s : %04x\n", "Revision Register", edt_p->bfd.revision_register);
  if (edt_p->bfd.string_type)
    printf("%-24s : %s\n", "Optionstr", edt_p->bfd.optionstr);

  if (edt_p->bfd.string_type == 2)
  {
    printf("Parsed Option String Values:\n");
    printf("    %-20s : %d (%s)\n", "Baseboard Type", edt_p->bfd.ostr.board_type,
           baseboard_names[edt_p->bfd.ostr.board_type]);
    printf("    %-20s : %d\n", "PCI channels", edt_p->DMA_channels);
    printf("    %-20s : %s\n", "Mezzanine Type", edt_p->bfd.ostr.mezzanine_type);
    printf("    %-20s : %s\n", "VHDL name", edt_p->bfd.ostr.filename);
    printf("    %-20s : %02x\n", "Release", edt_p->bfd.ostr.version_number);
    printf("    %-20s : %02x\n", "Revision", edt_p->bfd.ostr.rev_number);
    printf("    %-20s : %s\n", "Date", edt_p->bfd.ostr.date);
    printf("    %-20s : %d\n", "Custom DMA channels", edt_p->bfd.ostr.custom_DMA_channels);
    printf("    %-20s : %d\n", "Total DMA channels", edt_p->bfd.ostr.available_DMA_channels);
  }

  if (ID_IS_SS(edt_p->devid) || ID_IS_GS(edt_p->devid) || ID_IS_LX(edt_p->devid))
  {
    printf("%-24s : %s\n", "Mezz Chan 0 Bitfile", edt_p->bfd.mezz_name0);
    printf("%-24s : %s\n", "Mezz Chan 1 Bitfile", edt_p->bfd.mezz_name1);
    if (edt_p->bfd.mezz_optionstr0[0])
      printf("%-24s : %s\n", "Mezzanine Opt Str Ch. 0", edt_p->bfd.mezz_optionstr0);

    if (edt_p->bfd.mezz_optionstr1[0])
      printf("%-24s : %s\n", "Mezzanine Opt Str Ch. 1", edt_p->bfd.mezz_optionstr1);

    printf("%-24s : %02x\n", "Mezzanine Rev.", edt_p->mezz.extended_rev);
    if (edt_p->mezz.n_extended_words)
    {
      int i;
      printf("%-24s : %02x  ", "Mezzanine Extended Words", edt_p->mezz.n_extended_words);
      for (i = 0; i < edt_p->mezz.n_extended_words; i++)
        printf("%08x  ", edt_p->mezz.extended_data[i]);
      printf("\n");
    }
  }
}

/* return the best name for bitfile to use when
the mezz id is unknown */
char *edt_get_test_interface(EdtDev *edt_p)

{
  switch (edt_p->devid)
  {
    case PGS4_ID:
    case PSS4_ID:
      return "pciss4test.bit";

      break;

    case PGS16_ID:
    case PSS16_ID:
    case PE8LX16_ID:
    case PE8LX16_LS_ID:
    case PE4AMC16_ID:
      return "pciss16test.bit";
      break;

    case PE8LX1_ID:
      return "pe8lx1test.bit";
      break;

    case PE8G2V7_ID:
    case PE8LX32_ID:
      return "pciss32test.bit";
      break;
  }

  return NULL;
}

/* check the current bitfile to see if test file */

int edt_is_test_file_loaded(EdtDev *edt_p)

{
  edt_get_bitname(edt_p, edt_p->bfd.bitfile_name, sizeof(edt_p->bfd.bitfile_name));

  switch (edt_p->devid)
  {
    case PGS4_ID:
    case PSS4_ID:
      return !strcmp(edt_p->bfd.bitfile_name, "pciss4test.bit");
      break;

    case PGS16_ID:
    case PSS16_ID:
    case PE8LX16_ID:
    case PE8LX16_LS_ID:
    case PE4AMC16_ID:
      return !strcmp(edt_p->bfd.bitfile_name, "pciss16test.bit");
      break;

    case PE8G2V7_ID:
    case PE8LX32_ID:
      return !strcmp(edt_p->bfd.bitfile_name, "pciss64test.bit");
      break;
  }

  return 0;
}

/**
* @brief edt_get_board_description
*
* @param test_edt_p EdtDev pointer.
*
* @param force_mezzanine Attempt to load a bitfile to characterize
* the mezzanine board if no bitfile is loaded.
*
* @returns 0 on success, -1 on failure
*
* This attempts to learn as much as possible about the board referred to
* by test_edt_p. If the bitfile that's loaded has an option string defined,
* fill it in and parse its fields if possible. If test_edt_p is not channel 0,
* channel 0 is opened, examined, and its fields copied to test_edt_p. If the
* board is a GS or SS board, the mezzanine board is identified also. The
* bfd structure within the EdtDev structure is filled in as much as possible.
*
*
*/

int edt_test_3x3g(EdtDev *edt_p)

{
  int ret = edt_bitload(edt_p, NULL, "x3g.bit", 0, 0);
  int mezz_id;

  if (ret)
  {
    edt_msg(EDTAPP_MSG_FATAL, "%s bitload failed\n", "x3g.bit");

    return 0;
  }

  mezz_id = edt_get_board_id(edt_p); /* second try, now with x3g bitfile */

  if (mezz_id == MEZZ_3X3G)
    return 1;
  return 0;
}

int edt_get_board_description(EdtDev *edt_p, int force_mezzanine)

{
  int rc = 0;
  char *s;
  int test_interface = 0;

  memset(&edt_p->bfd, 0, sizeof(EdtBitfileDescriptor));

  edt_get_bitname(edt_p, edt_p->bfd.bitfile_name, sizeof(edt_p->bfd.bitfile_name));
  edt_get_mezz_chan_bitpath(edt_p, edt_p->bfd.mezz_name0, sizeof(edt_p->bfd.mezz_name0), 0);
  edt_get_mezz_chan_bitpath(edt_p, edt_p->bfd.mezz_name1, sizeof(edt_p->bfd.mezz_name1), 1);

  /* deal with OCM/OC192 hack*/

  if (ID_IS_SS(edt_p->devid) || ID_IS_GS(edt_p->devid) || ID_IS_LX(edt_p->devid))
  {
    if ((edt_p->devid != PE4AMC16_ID) &&
        (force_mezzanine && (edt_p->bfd.bitfile_name[0] == 0) || !edt_ui_loaded(edt_p)))
    {
      edt_msg(EDTLIB_MSG_INFO_1, "Forcing bitload...\n");

#ifdef USE_LAST_BITPATH
      /* see what the last interface was - if nothing, load sstest */
      if (s = edt_get_last_bitpath(edt_p))
      {
        edt_msg(EDTLIB_MSG_INFO_1, "Loading most recent bitfile %s\n", s);
      }
      else
#endif
          if (s = edt_get_test_interface(edt_p))
      {
        edt_msg(EDTLIB_MSG_INFO_1, "Loading test interface %s\n", s);
        test_interface = 1;
      }
      else
        goto fail;

      if ((rc = edt_bitload(edt_p, NULL, s, 0, 0)) != 0)
      {
        if (!test_interface)
        {
          if (s = edt_get_test_interface(edt_p))
          {
            if ((rc = edt_bitload(edt_p, NULL, s, 0, 0)) != 0)
            {
              edt_msg(EDTLIB_MSG_WARNING, "Unable to load %s\n", s);
              goto fail;
            }

            test_interface = TRUE;
          }
          else
            goto fail;
        }
        else
          goto fail;
      }
    }

    edt_p->mezz.id = edt_get_full_board_id(edt_p, NULL, NULL, NULL);

#ifdef PCD
    /* deal with kludges */
    /* for boards which don't work with standard bdid */

    if (edt_p->mezz.id == MEZZ_ERR_BAD_BITSTREAM)
    {
      if (edtdev_channels_from_type(edt_p) == 16)
      {
        if (edt_test_16te3(edt_p))
          edt_p->mezz.id = MEZZ_16TE3;
      }
      if (edt_p->mezz.id == MEZZ_ERR_BAD_BITSTREAM)
        if (edt_test_3x3g(edt_p))
          edt_p->mezz.id = MEZZ_3X3G;

      if (edt_p->mezz.id != MEZZ_ERR_BAD_BITSTREAM)
        edt_set_mezz_id(edt_p);
    }
#endif

    edt_get_bitname(edt_p, edt_p->bfd.bitfile_name, sizeof(edt_p->bfd.bitfile_name));

    if (edt_p->mezz.id == MEZZ_UNKN_EXTBDID)
    {
      rc = -1;
      goto fail;
    }

    if (test_interface)
    {
      s = edt_default_base_for_mezz(edt_p);

      edt_msg(EDTLIB_MSG_INFO_1, "Loading default interface %s\n", s);
      if (s && ((rc = edt_bitload(edt_p, NULL, s, 0, 0)) != 0))
      {
        edt_msg(EDTLIB_MSG_WARNING, "Unable to load %s, keeping test interface\n", s);
      }
    }

    edt_p->DMA_channels = edtdev_channels_from_type(edt_p);

    edt_get_bitname(edt_p, edt_p->bfd.bitfile_name, sizeof(edt_p->bfd.bitfile_name));
    edt_get_mezz_chan_bitpath(edt_p, edt_p->bfd.mezz_name0, sizeof(edt_p->bfd.mezz_name0), 0);
    edt_get_mezz_chan_bitpath(edt_p, edt_p->bfd.mezz_name1, sizeof(edt_p->bfd.mezz_name1), 1);

    if (edt_read_option_string(edt_p, edt_p->bfd.optionstr, &edt_p->bfd.revision_register))
    {
      edt_p->bfd.string_type = 0;
      if (edt_read_old_option_string(edt_p, edt_p->bfd.optionstr) == 0)
      {
        edt_p->bfd.string_type = 1;
      }
      rc = 0;
    }
    else
      edt_parse_option_string(edt_p->bfd.optionstr, &edt_p->bfd);

#ifdef PCD
    if (edt_p->mezz.id == MEZZ_OCM || edt_p->mezz.id == MEZZ_OC192 ||
        edt_p->mezz.id == MEZZ_THREEP || edt_p->mezz.id == MEZZ_SRXL ||
        edt_p->mezz.id == MEZZ_SRXL2)
      edt_read_mezz_option_string(edt_p, edt_p->bfd.mezz_optionstr0, 0);

    if (edt_p->mezz.id == MEZZ_OCM)
      edt_read_mezz_option_string(edt_p, edt_p->bfd.mezz_optionstr1, OFFSET_CH1_MEZ);
#endif
  }

fail:

  return rc;
}
