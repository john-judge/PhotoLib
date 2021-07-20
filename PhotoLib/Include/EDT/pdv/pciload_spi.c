#include "edtinc.h"
#include "pciload.h"
#include <stdlib.h>


/*
* program the pci express board with SPI 25PI6VS1G. It has two megabytes: 
* ID info is in the first 1024 up to (0x400), then 1Meg minus 1024 of data, then the
* 2nd (protected) sector (same layout as the first), and NO 3.3v/5v BS, thank goodness.
*
* TODO: take advantage of greater ID space (PCI wuz only 224 or some such ... urgh)
*/
int 
program_verify_SPI(EdtDev *edt_p, 
                   EdtBitfile *bitfile, 
                   EdtPromData *pdata, 
                   int promcode, 
                   int sector, 
                   int verify_only, 
                   int warn, 
                   int verbose)
{

    int     size = bitfile->hdr.filesize;
    char   *id = bitfile->hdr.promstr;
    int     idsize = (int) strlen(id);
    extern int quiet ;
    int     errs = 0;
    u_char *inptr;
    u_int    addr;


    if (strcmp(bitfile->hdr.filename, "ERASE") == 0)
    {
        printf("\n  erasing sector %d\n", sector);
        edt_sector_erase(edt_p, sector, SPI_SECTOR_SIZE, FTYPE_SPI);
        edt_flash_program_prominfo(edt_p, SPI_XC3S1200E, sector, pdata);
        edt_flash_reset(edt_p, FTYPE_SPI);
        /* OR SOMETHING LIKE THIS -- NOT DONE YET */
    }

    else /* program or verify */
    {
        int i, size = bitfile->data_size + PCIE_INFO_SIZE;
        u_char *buf = (u_char *) malloc(size);

        if (verify_only)
            printf("  verifying to %x\n", (sector * SPI_SECTOR_SIZE) + size);
        else printf("\n  erasing/programming sector %d\n", sector);

        memset(buf, '\0', PCIE_INFO_SIZE);
        strcpy((char *)buf, id);
        if (*pdata->osn)
            sprintf((char *)buf + PCIE_PID_SIZE, "OSN:%s", pdata->osn);
        if (*pdata->esn)
            sprintf((char *)buf + PCIE_PID_SIZE + OSN_SIZE, "ESN:%s", pdata->esn);
        sprintf((char *)buf + PCIE_PID_SIZE + OSN_SIZE + PROM_EXTRA_SIZE,
            "XTR:");
        memcpy((char *) buf + PCIE_PID_SIZE + OSN_SIZE + PROM_EXTRA_SIZE + 4, &pdata->extra_size, 4);
        memcpy(buf + PCIE_PID_SIZE + OSN_SIZE + PROM_EXTRA_SIZE + 8,
            pdata->extra_buf, pdata->extra_size);

        memcpy(buf + PCIE_INFO_SIZE, bitfile->data, bitfile->data_size);

        if (verbose)
            edt_flash_print16(edt_p, sector * SPI_SECTOR_SIZE, FTYPE_SPI);

        addr = (sector * SPI_SECTOR_SIZE);

        /* jump past the header and flip the data */
        inptr = buf + PCIE_INFO_SIZE;
        for (i = 0; i < (int) bitfile->data_size; i++)
            *inptr = edt_flipbits(*inptr);

        /*
        * finally, program the data (at start of sector)
        */
        if (verify_only)
        {
            edt_flash_verify(edt_p, addr, buf, size, FTYPE_SPI);
        }
        else
        {
            edt_flash_block_program(edt_p, addr, buf, size, FTYPE_SPI);
            edt_flash_reset(edt_p, FTYPE_SPI);
        }
    }

    return errs;
}
