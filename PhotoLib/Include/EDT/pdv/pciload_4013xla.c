#include "edtinc.h"
#include "pciload.h"
#include <stdlib.h>

/*
 * program the 4013xla board with a 29LV040B it has 8 64K sectors. sector 0
 * has the 3.3 volt protected boot code sector 1 has the 3.3 volt current
 * code sector 2 has the 5 volt protected code sector 3 has the 5 volt
 * current code sector 4-7 are not used and reserved, The current xilinx can
 * not address them. They might be used for a larger xilinx as an additional
 * 64k bytes on top of sectors 0-3. the id string is stored in the last 128
 * bytes of the sector the code is in.
 */
int program_verify_4013XLA(EdtDev *edt_p, EdtBitfile *bitfile, EdtPromData *pdata, int promcode,
                           int sector, int verify_only, int warn, int verbose)
{
  u_char *data_start = &bitfile->full_buffer[bitfile->hdr.data_start];
  u_int size = bitfile->hdr.filesize;
  char *id = bitfile->hdr.promstr;
  int idsize = (int)strlen(id);
  int quiet;
  int i, idx;
  int errs = 0;
  u_char val, flipval;
  u_char *inptr;
  int id_addr;

  quiet = (edt_get_verbosity() == 0) ? 1 : 0;

  /* code borrowed from prog_4013xla() */

  /*
   * program the xilinx
   */
  if (!verify_only)
  {
    printf("\n  erasing/programming sector %d\n", sector);
    edt_sector_erase(edt_p, sector, AMD_SECTOR_SIZE, FTYPE_X);
    edt_flash_program_prominfo(edt_p, AMD_4013XLA, sector, pdata);
    edt_flash_reset(edt_p, FTYPE_X);

    if (strcmp(bitfile->filename, "ERASE") != 0)
    {
      if (verbose)
        edt_flash_print16(edt_p, sector * AMD_SECTOR_SIZE, FTYPE_X);
      printf("  id string %s size %d\n", id, idsize);
      id_addr = (sector * AMD_SECTOR_SIZE) + AMD_SECTOR_SIZE - idsize - 1;
      for (i = 0; i < idsize; i++)
      {
        edt_flash_byte_program(edt_p, id_addr, id[i], FTYPE_X);
        id_addr++;
      }
      edt_flash_byte_program(edt_p, id_addr, 0, FTYPE_X);

      /*
       * finally, program the data (at start of sector)
       */
      edt_flash_reset(edt_p, FTYPE_X);
      id_addr = sector * AMD_SECTOR_SIZE;
      /* skip the header */
      inptr = data_start;
      printf("  programming to %x\n", sector * AMD_SECTOR_SIZE + size);
      for (i = 0; i < (int)size; i++)
      {
        val = *inptr;
        flipval = edt_flipbits(val);
        edt_flash_byte_program(edt_p, id_addr, flipval, FTYPE_X);
        if (verbose && ((i % 0x128) == 0))
        {
          printf("%08x\r", id_addr);
          fflush(stdout);
        }
        id_addr++;
        inptr++;
      }
      if (verbose)
        printf("last addr %08x\n", id_addr - 1);
    }
  }
  edt_flash_reset(edt_p, FTYPE_X);

  if (strcmp(bitfile->filename, "ERASE") != 0)
  {
    /* Verify */
    /* reset start */
    id_addr = sector * AMD_SECTOR_SIZE;
    /* skip the header */
    inptr = data_start;
    printf("  verifying to %x... ", sector * AMD_SECTOR_SIZE + size);
    idx = 0;
    for (i = 0; i < (int)size; i++)
    {
      u_char tmpval;

      val = *inptr;
      flipval = edt_flipbits(val);
      tmpval = edt_flash_read8(edt_p, id_addr, FTYPE_X);
      if (tmpval != flipval)
      {
        errs++;
      }
      if (verify_only && verbose)
      {
        if (i < 32)
        {
          printf("%02x ", tmpval);
          if ((++idx % 16) == 0)
            printf("\n(0x%x)\n", id_addr);
        }
        if (i > (int)size - 32)
        {
          printf("%02x ", tmpval);
          if ((++idx % 16) == 0)
            printf("\n(0x%x)\n", id_addr);
        }
      }
      id_addr++;
      inptr++;
    }
    printf("%d errors\n\n", errs);
    if (verbose)
      printf("%08x\n\n", id_addr);
    edt_flash_reset(edt_p, FTYPE_X);
  }

  return errs;
}
