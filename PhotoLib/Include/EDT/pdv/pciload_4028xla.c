#include "edtinc.h"

#include "pciload.h"
#include <stdlib.h>
#include <ctype.h>


static int program_verify(EdtDev *edt_p, EdtBitfile *bitfile, EdtPromData *pdata, int segment,
                          int sectors, u_int sectorsize, int vfonly, int warn, int verbose,
                          int promcode, int ftype);

void program(EdtDev *edt_p, EdtBitfile *bitfile, EdtPromData *pdata, int segment, int sectors,
             u_int sectorsize, int promcode, int ftype, int verbose);
int verify(EdtDev *edt_p, int promcode, EdtBitfile *bitfile, EdtPromData *pdata, int segment,
           int sectors, u_int sectorsize, int ftype, int verbose);

void edt_program_data(EdtDev *edt_p, EdtBitfile *bitfile, u_int addr, int ftype, int verbose);
void edt_program_flashid(EdtDev *edt_p, u_int addr, u_char *idbuf, u_int idsize, int ftype,
                         int verbose);

#if 0
/*
 * XC2S200 uses the 29LV081B -- 8 sectors per bitfile
 * All others use the 29LV040B -- XC2S200_4M uses 4 sectors per bitfile,
 * 4028XLA and XC2S150 use 2
 */
int
program_verify_XC2S300E(EdtDev *edt_p, EdtBitfile *bitfile, EdtPromData *pdata, int promcode, int segment, int vfonly, int warn, int verbose)
{
    return program_verify(edt_p, bitfile, pdata, segment, 4, AMD_SECTOR_SIZE, vfonly, warn, verbose, AMD_XC2S300E, FTYPE_LTX);
}
int
program_verify_XC2S200(EdtDev *edt_p, EdtBitfile *bitfile, EdtPromData *pdata, int promcode, int segment, int vfonly, int warn, int verbose)
{
    return program_verify(edt_p, bitfile, pdata, segment, 4, AMD_SECTOR_SIZE, vfonly, warn, verbose, AMD_XC2S200_4M, FTYPE_BT);
}
int
program_verify_XC2S150(EdtDev *edt_p, EdtBitfile *bitfile,EdtPromData *pdata,  int promcode, int segment, int vfonly, int warn, int verbose)
{
    return program_verify(edt_p, bitfile, pdata, segment, 2, AMD_SECTOR_SIZE, vfonly, warn, verbose, AMD_XC2S150, FTYPE_BT);
}
int
program_verify_4028XLA(EdtDev *edt_p, EdtBitfile *bitfile, EdtPromData *pdata,  int promcode, int segment, int vfonly, int warn, int verbose)
{
    return program_verify(edt_p, bitfile, pdata, segment, 2, AMD_SECTOR_SIZE, vfonly, warn, verbose, AMD_4028XLA, FTYPE_BT2);
}
#endif

int program_verify_default(EdtDev *edt_p, EdtBitfile *bitfile, EdtPromData *pdata, int promcode,
                           int segment, int vfonly, int warn, int verbose)
{
  Edt_prominfo *ep = edt_get_prominfo(promcode);
  return program_verify(edt_p, bitfile, pdata, segment, ep->sectsperseg, ep->sectorsize, vfonly,
                        warn, verbose, promcode, ep->ftype);
}

/*
 * program the as follows:
 * 4085: 29LV040B 4Meg PROM, 8 64K sectors:
 * 0 & 1: 3.3 volt protected boot code sector
 * 2 & 3: 5 volt protected code sector
 * 4 & 5: 3.3 volt current code
 * 6 & 7: 5 volt current code
 *
 * XC2S200_4M: 29LV040B 4Meg PROM, 8 64K sectors:
 * 0-3: 3.3 volt current code
 * 4-7: 5 volt current code
 * (no protected mode sectors)
 *
 * XC2S200: 29LV081B 8 Meg PROM, 16 64K sectors:
 * with a 29LV081B, which has 16 64K sectors:
 * 0-3: 3.3 volt protected boot code sector
 * 4-7: 5 volt protected code sector
 * 8-11: 3.3 volt current code
 * 12-15: 5 volt current code
 *
 * The id string is stored in the last 128 bytes of the last sector the code is in.
 *
 * RETURNS: exits on failure, returns 0 on success or 1 if successfull and xilinx rebooted
 */
int program_verify(EdtDev *edt_p, EdtBitfile *bitfile, EdtPromData *pdata, int segment, int sectors,
                   u_int sectorsize, int vfonly, int warn, int verbose, int promcode, int ftype)
{
  int size = bitfile->hdr.filesize;
  extern int quiet;

  if ((strcmp(bitfile->filename, "ERASE") != 0) && (size < MIN_BIT_SIZE_XLA))
  {
    edt_msg(EDT_MSG_FATAL, "  bit file %s is too small(%d bytes) for this FPGA\n",
            bitfile->filename, size);
    return (2);
  }

  /*
   * printf("  %sing board with boot controller\n",
   * vfonly?"Verify":"Program");
   */

  if (verbose)
    edt_msg(EDT_MSG_INFO_1,
            "  preparing to %s logical sector %d, %d physical sectors starting at %d.\n",
            vfonly ? "verify" : "re-burn", segment, sectors, segment * sectors);

  if (!vfonly)
  {
    program(edt_p, bitfile, pdata, segment, sectors, sectorsize, promcode, ftype, verbose);
    edt_msg(EDT_MSG_INFO_1, "\n");
  }

  if (strcmp(bitfile->filename, "ERASE") == 0)
    return 0;

  return verify(edt_p, promcode, bitfile, pdata, segment, sectors, sectorsize, ftype, verbose);
}

void program(EdtDev *edt_p, EdtBitfile *bitfile, EdtPromData *pdata, int psegment, int sectors,
             u_int sectorsize, int promcode, int ftype, int verbose)
{
  EdtPromIdAddresses paddr;
  int size = bitfile->hdr.filesize;
  int ftype_offset = (ftype == FTYPE_BT2) ? 256 : 0;
  int segment = (ftype == FTYPE_MIC) ? 0 : psegment;
  char *id = bitfile->hdr.promstr;
  u_int idsize = (int)strlen(pdata->id) + 1;
  int s;
  u_int id_addr, addr;
  int retry = 0;
  int done = 0;
  int erase_only = (strcmp(bitfile->filename, "ERASE") == 0);
  char tmpid[PCI_ID_SIZE];

  /* make sure id is on an even boundary, for f16 writes */
  if (idsize % 2)
    ++idsize;

  if (idsize > MAX_STRING)
  {
    edt_msg(EDT_MSG_FATAL, "  prom ID size out of range (%d)\n", idsize);
#ifdef VXWORKS
    return;
#else
    exit(2);
#endif
  }

  /************************************************************/
  /**** ERASE before programming anything *********************/
  /************************************************************/

  edt_msg(EDT_MSG_WARNING, "  \nerase segment %d at %06x", segment, segment * sectors * sectorsize);
  fflush(stdout);
  edt_flash_reset(edt_p, ftype);
  for (s = 0; s < sectors; s++)
  {
    edt_msg(EDT_MSG_WARNING, ".");
    fflush(stdout);

    retry = 0;
    done = 0;
    while (done == 0)
    {
      edt_msleep(1);
      if (edt_sector_erase(edt_p, segment * sectors + s, sectorsize, ftype) != 0)
      {
        if (retry < 5)
        {
          if (retry == 0)
            edt_msg(EDT_MSG_WARNING, "\n");
          retry++;
          edt_msg(EDT_MSG_WARNING, "Erase sector %d failed (timeout) retrying\n",
                  segment * sectors + s);
          /* try a few resets in case FPROM command is out of sync */
          edt_flash_reset(edt_p, ftype);
          edt_msleep(1);
          edt_flash_reset(edt_p, ftype);
          edt_msleep(1);
        }
        else
        {
          done = 1;
          retry++;
          edt_msg(EDT_MSG_WARNING,
                  "Erase sector %d failed %d times new flash load may be corrupt\n",
                  segment * sectors + s, retry);
          edt_msg(EDT_MSG_WARNING,
                  "Retry pciload before power off, call EDT for assistance if failure persists\n");
#ifdef VXWORKS
          return;
#else
          exit(2);
#endif
        }
      }
      else
      {
        done = 1;
      }
    }
  }

  /* if erasing, we're done */
  if (strcmp(bitfile->filename, "ERASE") == 0)
    return ;

  /************************************************************/
  /****  PROGRAM DATA at start of sector (+ offset, if any) ****/
  /************************************************************/
  addr = ftype_offset + (segment * sectors * sectorsize);
  edt_msg(EDT_MSG_WARNING, "\nprogram data");
  fflush(stdout);
  edt_program_data(edt_p, bitfile, addr, ftype, verbose);

  /***************************************************************************/
  /**** PROGRAM ID -- overwrites (presumably unused) data at the end of sector */
  /***************************************************************************/
  id_addr = edt_get_id_addr(promcode, segment);
  edt_msg(EDT_MSG_WARNING, "\nprogram id", id_addr);
  fflush(stdout);

  edt_zero(tmpid, PCI_ID_SIZE);
  strncpy(tmpid, pdata->id, PCI_ID_SIZE - 1);
  edt_program_flashid(edt_p, id_addr, pdata->id, PCI_ID_SIZE, ftype, verbose);

/************************************************************/
/****  PROGRAM INFO *****************************************/
/************************************************************/
program_sns:
  edt_flash_get_promaddrs(edt_p, promcode, segment, &paddr);
  edt_msg(EDT_MSG_WARNING, "\nprogram info");
  fflush(stdout);
  edt_flash_program_prominfo(edt_p, promcode, segment, pdata);
  printf("\ndone\n");
}

/* for newset (F16) boards, program the whole thing.
 * for others (BT, BT2), skip the header (irritating, but necessary for backwards compat)
 */
void edt_program_data(EdtDev *edt_p, EdtBitfile *bitfile, u_int addr, int ftype, int verbose)
{
  u_char *inptr = bitfile->full_buffer;
  u_int size = bitfile->hdr.filesize;
  u_int count, bytes_programmed = 0;
  const u_int blocksize = (ftype == FTYPE_MIC) ? 256 : 0x10000;
  u_char *flipbuf = NULL;

  /*
   * with most devices, we strip off the header
   */
  if ((ftype != FTYPE_F16) && (ftype != FTYPE_F16A))
  {
    inptr = &bitfile->full_buffer[bitfile->hdr.data_start];
    size = bitfile->hdr.filesize - bitfile->hdr.data_start;
  }

  if (ftype == FTYPE_BT)
    if ((flipbuf = edt_alloc(blocksize)) == NULL)
    {
      edt_msg(EDT_MSG_FATAL, "Memory allocation error -- contact EDT at tech@edt.com\n");
      return;
    }

  while (bytes_programmed < size)
  {
    count = (size - bytes_programmed >= blocksize) ? blocksize : size - bytes_programmed;

    if (ftype != FTYPE_BT)
      edt_flash_block_program(edt_p, addr, inptr, count, ftype);

    else /* FTYPE_BT: gotta flip em */
    {
      u_int i;
      for (i = 0; i < count; i++)
        flipbuf[i] = edt_flipbits(inptr[i]);
      edt_flash_block_program(edt_p, addr, flipbuf, count, ftype);
    }

    if (!(bytes_programmed % 0x10000))
    {
      edt_msg(EDT_MSG_WARNING, ".");
      fflush(stdout);
    }

    bytes_programmed += count;
    inptr += count;
    addr += (count / edt_flash_writesize(ftype));
  }
  if (flipbuf)
    edt_free(flipbuf);

  edt_msg(EDT_MSG_WARNING, "%d bytes", bytes_programmed);
}

/*
 * verify the xilinx in the prom against the one in the file
 *
 * RETURNS # of errors or 0 on success
 */
int verify(EdtDev *edt_p, int promcode, EdtBitfile *bitfile, EdtPromData *pdata, int psegment,
           int sectors, u_int sectorsize, int ftype, int verbose)
{
  int ftype_offset = (ftype == FTYPE_BT2) ? 256 : 0;
  int segment = (ftype == FTYPE_MIC) ? 0 : psegment;
  char *id = bitfile->hdr.promstr;
  int idsize = (int)strlen(id);
  u_int i, idx;
  u_char *inptr = bitfile->full_buffer;
  u_int size = bitfile->hdr.filesize;
  u_int addr, last_addr;
  u_char *readbuf = NULL;
  const u_int blocksize = 0x10000;
  u_int count, bytes_verified = 0;
  int errs = 0, lasterrs = 0, waserr = 0;
  u_char val;

  if (idsize > MAX_STRING)
  {
    edt_msg(EDT_MSG_FATAL, "  prom ID size out of range (%d)\n", idsize);
    exit(2);
  }

  if ((readbuf = edt_alloc(blocksize)) == NULL)
  {
    edt_msg(EDT_MSG_FATAL, "  memory alloc error in verify\n");
    exit(2);
  }

  /*
   * with most devices, we strip off the header
   */
  if (ftype != FTYPE_F16 && ftype != FTYPE_F16A)
  {
    inptr = &bitfile->full_buffer[bitfile->hdr.data_start];
    size = bitfile->hdr.filesize - bitfile->hdr.data_start;
  }

  /* reset start */
  edt_flash_reset(edt_p, ftype);

  /* set address; offset 256 for these BT2 types */
  addr = last_addr = ftype_offset + (segment * sectors * sectorsize);

  if (verbose)
    printf("\n  verify %d bytes from %x to %x\n", size, addr,
           segment * sectors * sectorsize + size);
  else
  {
    printf("verify");
    fflush(stdout);
  }

  idx = 0;

  while (bytes_verified < size)
  {
    count = (size - bytes_verified >= blocksize) ? blocksize : size - bytes_verified;

    edt_flash_block_read(edt_p, addr, readbuf, count, ftype);

    for (i = 0; i < count; i++)
    {
      if ((ftype != FTYPE_BT2) && (ftype != FTYPE_F16) && (ftype != FTYPE_F16A) &&
          (ftype != FTYPE_MIC)) /* flip em */
        val = (edt_flipbits(readbuf[i]));
      else
        val = readbuf[i];

      if (val != *inptr)
      {
        errs++;
        if (0) /* (verbose) */
        {
          printf("%06x: fpga %02x(%c) != file %02x(%c)\n", addr, val, isascii(val) ? val : '?',
                 *inptr, isascii(*inptr) ? *inptr : '?');
        }
      }

      ++bytes_verified;
      ++inptr;
      last_addr = addr;
      if ((i % edt_flash_writesize(ftype)) ==
          0) /* only increment every other addr for 16-bit devices */
        last_addr = addr++;
    }

    if (errs > lasterrs)
    {
      if (lasterrs == 0)
        printf("\n%x\n", bytes_verified);
      printf("!");
      lasterrs = errs;
      waserr = 1;
    }
    else if (waserr)
    {
      printf("\n%x\n", bytes_verified);
      waserr = 0;
    }
    else
      printf(".");
    fflush(stdout);
  }

  printf(" %d errors\n\n", errs);

  edt_flash_reset(edt_p, ftype);

  edt_free(readbuf);

  return (errs);
}
