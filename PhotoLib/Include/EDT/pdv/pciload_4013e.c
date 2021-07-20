#include "edtinc.h"
#include "pciload.h"
#include <stdlib.h>

/*
 * program the 4013e board with a 29F010 it has 8 16K sectors. sector 0 and 1
 * have the protected code sectoe 4 and 5 are reprogrammed with the current
 * code sector 7 has the id string
 */
int program_verify_4013E(EdtDev *edt_p, EdtBitfile *bitfile, EdtPromData *pdata, int promcode,
                         int segment, int verify_only, int warn, int verbose)
{
  u_char *data_start = &bitfile->full_buffer[bitfile->hdr.data_start];
  int size = bitfile->hdr.filesize;
  char *id = bitfile->hdr.promstr;
  size_t idsize = strlen(id);
  extern int quiet;

  int i, idx;
  int errs = 0;
  u_char val, flipval;
  u_char *inptr;
  int id_addr = (8 * E_SECTOR_SIZE) - PCI_ID_SIZE;

  if (!verify_only)
  {
    /* put id at the end of sector 7 */
    id_addr = (int)((8 * E_SECTOR_SIZE) - idsize);
    printf("  erasing/programming sector 7 id\n");
    edt_sector_erase(edt_p, 7, E_SECTOR_SIZE, FTYPE_X);
    edt_flash_program_prominfo(edt_p, AMD_4013E, 7, pdata);
    edt_flash_reset(edt_p, FTYPE_X);
    for (i = 0; i < (int)idsize; i++)
    {
      edt_flash_byte_program(edt_p, id_addr, id[i], FTYPE_X);
      id_addr++;
    }
    edt_flash_reset(edt_p, FTYPE_X);
    printf("  erasing sector 4\n");
    edt_sector_erase(edt_p, 4, E_SECTOR_SIZE, FTYPE_X);
    edt_flash_reset(edt_p, FTYPE_X);
    printf("  erasing sector 5\n");
    edt_sector_erase(edt_p, 5, E_SECTOR_SIZE, FTYPE_X);
    edt_flash_reset(edt_p, FTYPE_X);

    /* start data in sector 4 */
    id_addr = 4 * E_SECTOR_SIZE;
    if (strcmp(bitfile->filename, "ERASE") == 0)
    {
      if (verbose)
        printf("  erased %08x - %08x\n", id_addr, id_addr + size);
    }
    else
    {
      /* skip the header */
      inptr = data_start;
      printf("  programming to %x\n", 4 * E_SECTOR_SIZE + size);
      for (i = 0; i < size; i++)
      {
        val = *inptr;
        flipval = edt_flipbits(val);
        edt_flash_byte_program(edt_p, id_addr, flipval, FTYPE_X);
        if (verbose && ((i % 0x100) == 0))
        {
          printf("%08x\r", id_addr);
          fflush(stdout);
        }
        id_addr++;
        inptr++;
      }
      if (verbose)
        printf("%08x\n", id_addr - 1);
    }
  }

  if (strcmp(bitfile->filename, "ERASE") != 0)
  {
    edt_flash_reset(edt_p, FTYPE_X);

    /* Verify */
    /* start at  sector 4 */
    id_addr = 4 * 0x4000;
    /* skip the header */
    inptr = data_start;
    printf("  verifying to %x... ", 4 * 0x4000 + size);
    idx = 0;
    for (i = 0; i < size - PCI_ID_SIZE; i++)
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
        if (i > size - 32)
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
      printf("last addr %08x\n", id_addr - 1);

    edt_flash_reset(edt_p, FTYPE_X);
  }

  return errs;
}
