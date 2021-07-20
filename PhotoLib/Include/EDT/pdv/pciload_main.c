/**
 * Copyright (c) 1997-2018 Engineering Design Team (EDT), Inc.
 * All rights reserved. This file is subject to the terms
 * and conditions of the EULA defined at edt.com/terms-of-use.
 *
 * Technical Contact: <tech@edt.com>
 */

/**
 * @file pciload_main.c
 *
 * Main module for the EDT pciload program, the PCI Interface
 * FPGA code programmer / verifier / lister for EDT boards
 */

#include "edtinc.h"
#include "pciload.h"
#include <stdlib.h>
#include <ctype.h>

extern volatile char *throwaway;

void print_maclist(char *label, char *list);
static u_short edt_print_prom_summary(EdtDev *edt_p, int sect);
void edt_pciload_printversion(char *devname, int unit, int do_lib_version);

#ifdef NO_FS
#include "nofs_flash.h"
#else
#define  MAPFLASHARRAY(name,ar, size) (ar = NULL, size = 0)
#endif

typedef struct {
    char fname[MAX_PATH];
    int sector;
} EdtLoadNames;


/* ALERT: return should only be called from main, otherwise this could break things.... */
#ifdef NO_MAIN
#define return(x) return(x)
#endif

/*
 * Given a file path or name or basename, find the actual bitfile paths to load
 * depending on the device type and where the bitfiles are found (std. subdirs,
 * or . or pathname provided) and populate the list of names to load. Return 
 * value is 1 for most newer fpgas, or 2 for dual-voltage fpgas, or 0 if no
 * matching file(s) found.
 */
int
get_names_to_load(int promcode, char *fname, char *flashdir, EdtLoadNames *names_to_load, int sect, int nofs)

{
    int sector;
    Edt_prominfo *ep = edt_get_prominfo(promcode);
    char *newfname;

    sector = (sect == IS_DEFAULT_SECTOR)? ep->defaultseg : sect;

    if (sect == IS_DEFAULT_SECTOR)
    {
        if (ep->load_seg1 != -1)
        {
            if (find_fname_to_load(promcode, 0, nofs, fname,  
                 flashdir, names_to_load[0].fname) == -1)
                return (0);
            names_to_load[0].sector = ep->load_seg0;

            if (find_fname_to_load(promcode, 1, nofs, fname,  
                 flashdir, names_to_load[0].fname) == -1)
                return (0);
            names_to_load[1].sector = ep->load_seg1;

            return 2;

        }
        else
        {
            if (find_fname_to_load(promcode, -1, nofs, fname,  
                 flashdir, names_to_load[0].fname) == -1)
                return (0);
            names_to_load[0].sector = ep->load_seg0;

            return 1;
        }
    }
    else
    {
        if (find_fname_to_load(promcode, -1, nofs, fname,  
                 flashdir, names_to_load[0].fname) == -1)
            return (0);

        names_to_load[0].sector = sector;

        return 1;

    }
}



EdtPciLoadVerify
get_program_verify(int promcode)
{

    EdtPciLoadVerify pvf = NULL;

    switch (promcode)
    {
        case AMD_4013E:
            return program_verify_4013E;

        case AMD_4013XLA:
            return program_verify_4013XLA;

        case SPI_XC3S1200E:
            return program_verify_SPI;

        default:
            return program_verify_default;

    }

    return pvf;
}


/*
 * extract the prom type / name for the array if embedded bitfile
 */
static int
load_bitfile(char *fname, EdtBitfile *bitfile, int promcode, int nofs)
{
    /* this code borrowed from main() */
    Edt_prominfo *ep = edt_get_prominfo(promcode);
    long   offset;
    /*
     * if ERASE don't do anything except copy the filename to the struct
     */
    if (strcmp(fname, "ERASE") == 0)
    {
      bitfile->filename = strdup("ERASE");
    }
    else
    {

        printf("  Bitfile:  %s\n", fname);


        /* get the header. opens and closes the file, and returns the
         * offset to the start of data 
         */
        edt_bitfile_init(bitfile);

        if (nofs)
        {
            char *p;
            u_char *ba;
            char tmpname[EDT_STRBUF_SIZE];
            char bname[EDT_STRBUF_SIZE];
            char pname[EDT_STRBUF_SIZE];
            char hname[EDT_STRBUF_SIZE];
            int size;

            strcpy(tmpname, fname);
            if ((p = strrchr(tmpname, '/')) != NULL)
            {
                strcpy(bname, p+1);
                *p = '\0';
                if ((p = strrchr(tmpname, '/')) != NULL)
                    strcpy(pname, p+1);
                else strcpy(pname, tmpname);

                sprintf(hname, "%s_%s", pname, bname);
                if ((p = strrchr(hname, '.')) != NULL)
                    *p = '\0';
            }

            MAPFLASHARRAY(hname, ba, size); /* see autogen header file nofs_flash.h */

            if (ba == NULL)
            {
                fprintf(stderr, "error: no array referenced to %s in nofs_flash.h\n", hname);
                return 1;
            }

            edt_bitfile_load_array(bitfile, ba, size);

            bitfile->filename = strdup(hname);

        }
        else
        {
            if (edt_bitfile_load_file(bitfile, fname) != 0)
            {
                edt_msg_perror(DEBUG2, fname);
                return -1;
            }

        }
        offset = edt_get_bitfile_header(bitfile, &bitfile->hdr);

        if (bitfile->hdr.magic != ep->magic)
        {
            printf("Error -- invalid magic number for this device (%d sb %d)\n", bitfile->hdr.magic, ep->magic);
            return 2;
        }

    }

    printf("  File  id: <%s>\n", bitfile->hdr.promstr);

    return 0;
}


static int
usage(char *s)
{
    printf("\n");
    printf("pciload: update, verify or query FPGA firmware on EDT PCI/PMC/cPCI boards\n\n");
    if (s && (*s))
        printf("%s\n\n", s);

    printf("usage:\n");
    printf("  pciload [options] [keyword | filename]\n");
    printf("\n");
    printf("  If no arguments are given, pciload finds all EDT boards in the system\n");
    printf("  and prints the prom ID for each.\n");
    printf("\n");
    printf("options:\n");
    printf("  -u <unit>        Specify the unit number (default 0), or device type/unit e.g. -u %s1\n", EDT_INTERFACE);
    printf("  -i               Program new embedded ID info (if loading flash with 'update' or filename')\n");
    printf("  -I               Program new embedded ID info; get defaults from file if present\n");
    printf("  -iX or -Ix <arg> Same as -i or -I, but X can be any of:\n");
    printf("      s <arg>         Serial number (4-10 digits)\n");
    printf("      p <arg>         Part number (10 digits)\n");
    printf("      v <arg>         Version number (1-4 digits)\n");
    printf("      c <arg>         Clock speed (1-4 digits)\n");
    printf("      o <arg>         Options (4-10 characters)\n");
    printf("      O <arg>         OSN (4-31 characters)\n");
    printf("      t <arg>         Option serial number (4-10 digits)\n");
    printf("      m <arg>         Mac address list, format nn,addr,addr1,...addr15 where nn is number of mac addresses\n");
    printf("  -d <dir>         Specify the directory to look for files (default ./flash)\n");
    printf("  -D [addr|i] [sz] Starting at addr (hex, default 0) read and dump sz bytes (dec., default 768), from FPGA,\n");
    printf("                   rounded to nearest 16. 'i' instead of an address dumps from the info space address\n");
    printf("  -p               Print a summary of installed boards (this is the default operation)\n");
    printf("  -q               Set quiet mode\n");
    printf("  -s <0-5>         Set the PROM sector number on boards using XLA or Alterra parts\n");
    printf("                   If no sector is specified (recommended), pciload will program\n");
    printf("                   the default sector(s) for that device with the specified file (or)\n");
    printf("                   with the specified file (or the default file if none specified)\n");
    printf("  -v               Verify on-board firmware against file (will not burn new f/w)\n");
    printf("  -c [fname]       Check on-board firmware ID against file ID and report\n");
    printf("  -V               Verbose mode\n");
    printf("  -F <0-8>         Add various rewrites/sleeps; works around problems with some systems'\n");
    printf("                   chipsets. If programming fails, try -F 2, 4 or 3. See also -S\n");
    printf("  -S               Set 'slow' mode -- necessary on some computers; try this if the load or\n"); 
    printf("                   verify seems to hang or indicates corrupt firmware and -F doesn't fix it\n");
#ifdef NO_FS
    printf("  -r               Force read FPGA bitfile instead of array: only applicable with NO_FS\n");
    printf("  -e               Force read from array (default) only applicable with NO_FS\n");
#endif
    printf("  -h               This help/usage message ('pciload --help' will also work)\n");
    printf("\n");
    printf("  The last argument may either be a full or partial filename to burn, or one of the\n");
    printf("  keywords 'update', 'verify', 'checkid', 'checkupdate', or 'ERASE'.\n");
    printf("\n");
    printf("  If a partial filename is specified (e.g. 'pcd',  or 'pe4dvacamlk', pciload\n");
    printf("  will search for the appropriate version of the FPGA file, and update to\n");
    printf("  that version (or compare if '-v' is specified). Complete/absolute pathnames\n");
    printf("  can also be used for filename, but it is recommended that you  us the par-\n");
    printf("  tial filename method to allow pciload to find and use the correct file from\n");
    printf("  possibly multiple files with the same or similar names under the the flash\n");
    printf("  subdirectory tree\n");
    printf("\n");
    printf("  The keywords 'update' and 'checkupdate' tell pciload to find and burn the\n");
    printf("  current version of the board's firmware in the flash/<fpga> directory into\n");
    printf("  the board's flash PROM. 'checkupdate' will check and burn only if the data\n");
    printf("  in the file is different from that in the PROM, whereas 'update' will re-burn\n");
    printf("  the new data regardless.\n");
    printf("\n");
    printf("  If the keyword 'verify' is present, pciload will compare the FPGA firmware in\n");
    printf("  the board's flash PROM to the one in the file (optionally the '-v' flag can be\n");
    printf("  used with a filename to compare to a specific FPGA file)\n");
    printf("\n");
    printf("  If the keyword 'checkid' is present, pciload will compare the FPGA ID the\n");
    printf("  PROM to the one in the file (optionally the '-c' flag can be used with a\n");
    printf("  filename to compare to a specific FPGA file).\n");
    printf("\n");
    printf("  If the keyword 'ERASE' is present, pciload will erase the currently selected\n");
    printf("  prom BOOT sector. Since this renders the board unusable after the next power\n");
    printf("  cycle, the ERASE function should only be used when security requirements\n");
    printf("  dictate. See README.erase_prom in your installation directory for details on\n");
    printf("  how to erase all volatile sectors of EEprom memory on a board.\n");
    printf("\n");
    printf("  The keyword 'help' prints out this message (equivalent to '-h').\n");
    printf("\n");
    printf("  Exit codes: pciload will exit with 0 on success, 1 on most errors, or 2\n");
    printf("  if the specified board can't be opened (single-board operations). For the\n");
    printf("  'checkid', -c, 'verify', -v, and programming options, the exit code is 0\n");
    printf("  if the IDs match or the verify succeeds, or 3 if the ID's don't match or\n");
    printf("  verification fails.\n");

    printf("\n");
    if (s == NULL)
        return 0;
    return 1;
}

/*
 * output the fpga's ID string (name / date) in <> brackets
 */
static void
print_segment_info(EdtDev *edt_p, int promcode, int segment, EdtPromData *pdata)
{
    edt_read_prom_data(edt_p, promcode, segment, pdata);

    if (pdata->id[0])
        printf(" <%s>", pdata->id);
    printf("\n");

}

/*
 * Output all the formatted FPGA PROM informaton for a specified sector
 * 
 *   Sector <sect> (<BOOT|Prot|User>)  <<id string, as read from flash>>
 *
 * (Note: this should really be table-driven...)
 */
static void
print_prominfo(EdtDev *edt_p, int promcode, u_short stat)
{
    int i;
    EdtPromData pdata;
    u_char jumpers = stat & 0x3;
    u_char idbits = (stat >>2) & 0x1f;
    u_char xidbits = stat >> 8;
    Edt_prominfo *ep = edt_get_prominfo(promcode);

    if (promcode > edt_get_max_promcode())
        printf("  Unknown flash ROM [%02x %02x] - no information available\n", xidbits, idbits);

    if (edt_pciload_info_na(edt_p))
    {
        int rc = 1;

        // if we don't have the bitfile ID string, use the option string in its place (if it exists)
        if ((edt_p->bfd.bitfile_name[0] == 0))
        {
            if (edt_get_board_description(edt_p, TRUE) == 0)
            {
                if ((rc = edt_read_option_string(edt_p, edt_p->bfd.optionstr, NULL)) == 0)
                {
                    printf("  Sector 0 <%s>\n", edt_p->bfd.optionstr);
                }
            }
        }

      if (rc != 0)
          printf("  Sector 0 <flash info for this device is not available via pciload>\n");

      return;
    }


    /* should all be table driven but first need to make sure all EPinfo values are correct */
    /* (and also this boot / non-boot stuff for earlier devices...? */
    switch (promcode)
    {
        case AMD_4013E:
            printf("  Sector 0");
            print_segment_info(edt_p, promcode, 0, &pdata);
            break;
        case AMD_4013XLA:
            printf("  Sector 1");
            print_segment_info(edt_p, promcode, 1, &pdata);
            printf("  Sector 3");
            print_segment_info(edt_p, promcode, 3, &pdata);
            break;
        case AMD_XC2S200_4M:
            printf("  Sector 0");
            print_segment_info(edt_p, promcode, 0, &pdata);
            printf("  Sector 1");
            print_segment_info(edt_p, promcode, 1, &pdata);
            break;
        case AMD_4028XLA:
        case AMD_XC2S150:
        case AMD_XC2S100_8M:
        case AMD_XC2S200_8M:
            printf("  Sector 0");
            print_segment_info(edt_p, promcode, 0, &pdata);
            printf("  Sector 1");
            print_segment_info(edt_p, promcode, 1, &pdata);
            printf("  Sector 2");
            print_segment_info(edt_p, promcode, 2, &pdata);
            printf("  Sector 3");
            print_segment_info(edt_p, promcode, 3, &pdata);
            break;
        case SPI_XC3S1200E:
            printf("  Sector 0");
            print_segment_info(edt_p, promcode, 0, &pdata);
            printf("  Sector 1");
            print_segment_info(edt_p, promcode, 1, &pdata);
            /* ALERT -- may be more if different bitfiles for 4/8 lane etc. */
            break;

        case AMD_XC5VLX70T:
            printf("  Sector 0");
            print_segment_info(edt_p, promcode, 0, &pdata);
            break;

        default:
            for (i=0; i<(int)ep->nsegments; i++)
            {
                printf("  Sector %d (%s) ", i, (jumpers == i)?"BOOT":(i==0)?"Prot":"User");
                print_segment_info(edt_p, promcode, i, &pdata);
            }
            break;
    }
}


/*
 * Gather the info about a specific board print the summary info (everything except
 * the name -- see comment for edt_enumerate_and_summarize)
 */
static u_short
edt_print_prom_summary(EdtDev *edt_p, int sect)
{
    int sector;
    int     promcode;
    char promcode_str[128];
    u_short  stat;
    Edt_prominfo *ep;
    EdtPromData pdata;

    promcode = edt_flash_prom_detect(edt_p, &stat);
    ep = edt_get_prominfo(promcode);
    sector = (sect == IS_DEFAULT_SECTOR)? ep->defaultseg:sect;
    edt_read_prom_data(edt_p, promcode, sector, &pdata);
    edt_fmt_promcode(promcode, promcode_str);
    printf("  ID 0x%02x, %s\n", edt_p->devid, promcode_str);
    edt_print_id_info(&pdata);
    edt_print_pcie_negotiated_link(edt_p);
    print_prominfo(edt_p, promcode, stat);
    edt_print_dev_flashstatus(edt_p, stat, sector);
    printf("\n");

    return stat;
}

/*
 * enumerate the boards found in the system, and print out a summary
 * of the information for each in a formatted way:
 * 
 * <dev> unit <u> (<devname>):
 *   ID 0x<id>, <fpga> <bus> FPGA, <flash> FPROM
 *   s/n <serial>, p/n <part>, i/f fpga <fpga>, rev <rev>, clock <clock> Mhz, opt <options>
 *   PCIe negotiated link width <width>, <speed> Gbps (if PCIe, skipped otherwise)
 *   Sector <s0> (<precedence>)  <<id string>>
 *   Sector <s1> (<precedence>)  <<id string>>
 *   ... for all active sectors
 */
static int
edt_enumerate_and_summarize(char *dev, int unit, int sect)

{
    int nunits = 0;
    int i;
    EdtDev *edt_p;
    char lasttype[15];

    Edt_bdinfo *boards = edt_detect_boards(dev, unit, &nunits, 0);

    if (boards[0].id == -1)
    {
        printf("No board detected\n");
        return (1);
    }

    lasttype[0] = '\0';

    for (i = 0; boards[i].id != -1; i++)
    {
        if (strncmp(lasttype, boards[i].type, 15) != 0)
        {
            edt_pciload_printversion(boards[i].type, boards[0].id, (i == 0));
            strncpy(lasttype, boards[i].type, 15);
        }
    }

    if (boards[1].id != -1)
        printf("\nMultiple EDT PCI boards detected:\n\n");

    for (i = 0; boards[i].id != -1; i++)
    {
        if ((unit < 0) || (unit == boards[i].id))
        {
            printf("%s unit %d (%s):\n", 
                    boards[i].type, 
                    boards[i].id,
                    edt_idstring(boards[i].bd_id, boards[i].promcode));

            if (boards[i].bd_id != P53B_ID) 
            {
                if ((edt_p = edt_open(boards[i].type, boards[i].id)) != NULL)
                {
                    edt_print_prom_summary(edt_p, sect);
                }
            }
        }
    }

    free(boards);

    return 0;
}


/*
 * returns 0 if match, 3 if not; for exit code from pdiload
 */
int
check_load_values(EdtDev *edt_p, 
        int promcode, 
        EdtBitfile *bitfile, 
        EdtPromData *pdata,
        int task,
        int warn,
        int segment)

{
    /* check for ok values */
    int idmatch = 0;
    int quiet = (edt_get_verbosity() == 0)?1:0;

    if (strlen(pdata->id) > MAX_STRING)
    {
        printf("  prom ID size out of range (%d)\n", (int)strlen(pdata->id));
        return 0;
    }
    if (warn || task == EDT_VerifyOnly || task == EDT_CheckIdOnly)
        idmatch = check_id_stuff(bitfile->hdr.promstr, pdata->id, edt_p->devid, task, bitfile->filename, segment);

    if (task == EDT_Erase)
      return 0;

    if (task == EDT_CheckIdOnly)
    {
        if (idmatch)
        {
            printf("\n  file ID and prom ID are the same\n\n");
            warn = 0;
        }
        else
        {
            printf("\n  file ID and prom ID differ\n\n");
        }
    }
    else if (task == EDT_CheckUpdate)
    {
        if (idmatch)
        {
            printf("\n  file ID and prom ID are the same; skipping update\n\n");
            warn = 0;
        }
        else
        {
            printf("\n  file ID and prom ID differ\n\n");
        }
    }
    else if (!idmatch)
    {
        if (task == EDT_VerifyOnly)
        {
            printf("\n  file ID and prom ID differ; skipping remainder of verify\n\n");
            return 3;
        }
        else printf("  file ID and prom ID differ\n");
    }

    if (!(task == EDT_VerifyOnly || task == EDT_CheckIdOnly))
    {
        if (warn && !quiet)
        {
            check_hw_fw_conflicts(edt_p, bitfile);
            warnuser(edt_p, bitfile->filename, segment);
        }
    }
    if (idmatch)
        return 0;
    return 3;
}



int     quiet = 0;

/*
 * Main module. NO_MAIN is typically only defined when compiling for vxworks; if you
 * want to use this code outside of a main module in any other OS, just copy the code
 * and modify it to work as a standalone subroutine, including adding parameters in
 * place of the command line arguments
 */
#ifdef NO_MAIN
#include "opt_util.h"
char *argument ;
int option ;
pciload(char *command_line)
#else
int
main(int argc, char *argv[])
#endif
{
    int     i, ret = 0;
    int     sector;
    int     skipUpdate = 0;
    int     nofs = 0;
    int     verify_status = 0, vb = 0, vfonly = 0, chkonly = 0;
    int     summary_only = 0;
    char    fname[MAX_STRING+1];
    int     promcode;
    int     saveinfo = 0;
    u_short  stat;
    EdtDev *edt_p = NULL;
    Edt_prominfo *ep;
    char    errbuf[EDT_STRBUF_SIZE];
    char   *unitstr = "";

    char   *argv0;
    char   *iarg;
    char    dev[EDT_STRBUF_SIZE];
    char    flash_dir[EDT_PATHBUF_SIZE];
    int     force_fast = 0;
    int     unit = -1;
    int     sect;
    int     proginfo = 0;
    u_int dump_addr = 0;
    u_int dump_size = 0;

    Edt_embinfo embinfo;
    EdtPromData existing;
    EdtPromData modified;

    EdtLoadState task = EDT_Enumerate;
    EdtBitfile    bitfile;
    char new_sn[16];
    char new_pn[16];
    char new_rev[8];
    char new_clock[8];
    char new_opts[16];
    char new_esn[EDT_STRBUF_SIZE];
    char new_osn[EDT_STRBUF_SIZE];
    char new_optsn[OPTSN_SIZE];
    char new_maclist[MACLIST_SIZE];

    int clear_extra = 0;

#ifdef NO_MAIN
    char **argv  = 0 ;
    int argc = 0 ;
    opt_create_argv("pciload",command_line,&argc,&argv);
#endif

#ifdef NO_FS
    nofs = 1;
#endif

    errbuf[0] = 0;

    flash_dir[0] = '\0';
    sect = IS_DEFAULT_SECTOR;
    argv0 = argv[0];
    fname[0] = '\0';
    dev[0] = '\0';

    new_esn[0] = new_osn[0] = new_sn[0] = new_pn[0] = new_opts[0] = new_rev[0] = new_clock[0] = new_optsn[0] = new_maclist[0] = '\0';

#ifdef OLD
    esn[0] = osn[0] = '\0';
#endif

    memset(&embinfo,0,sizeof(embinfo)) ;
    edt_bitfile_init(&bitfile);

    /*
     * COMMAND LINE ARGUMENTS
     */

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-')
        {
            if (i != argc - 1)
                usage("the fname must be the last argument");
            strcpy(fname, argv[i]);
        }
        else
        {
            if (argv[i][1] == '-')
            {
                if (strcmp(argv[i], "--help") == 0)
                {
                    usage(NULL);
                    return(0);
                }
                else
                {
                    sprintf(errbuf, "unknown option \"%s\"", argv[i]);
                    usage(errbuf);
                    return(1);
                }
            }
            if ((argv[i][1] == 0)
                    && (strcmp(&argv[i][1], "Ossp") != 0)
                    && (strcmp(&argv[i][1], "Odrs") != 0))
            {
                usage(NULL);
                return(1);
            }

            switch (argv[i][1])
            {
                case 'D':
                    dump_size = 0x300;

                    if ((i+1 == argc) || (argv[i+1][0] == '-'))
                    {
                        dump_addr = 0;
                    }
                    else
                    {
                        ++i;

                        if (argv[i][0] == 'i')
                        {
                            dump_addr = 0xffffffff; /* flag to dump starting at info space */
                        }
                        else if (argv[i][0] == 'a') /* dump all */
                        {
                            dump_addr = 0;
                            dump_size = 0xffffffff; /* flag; will be set to the actual flash size */
                        }
                        else /* should be an address */
                        {
                            dump_addr = strtoul(argv[i], NULL, 16);

                            /* check for size */
                            if ((i+1 != argc) && (argv[i+1][0] != '-'))
                            {
                                dump_size = atoi(argv[++i]);
                            }
                        }
                    }
                    task = EDT_DumpFlash;
                    break;

                case 'd':
                    if (++i == argc)
                    {
                        usage("unfinished -d option");
                        ret = 1;
                    }
                    else strcpy(flash_dir, argv[i]);
                    break;
                case 'u':
                    if (++i == argc)
                    {
                        usage("unfinished -u option");
                        ret = 1;
                    }
                    else unitstr = argv[i];
                    break;
                case 's':
                    if (++i == argc)
                    {
                        usage("unfinished -s option");
                        ret = 1;
                    }
                    else sect = atoi(argv[i]);
                    break;
                case 'e':
#ifdef NO_FS
                    nofs = 1;
#else
                    sprintf(errbuf, "-e specified but not compiled with embedded bitfiles.\nrecompile with -DNO_FS and try again\n");
                    usage(errbuf);
                    ret = 1;
#endif
                    break;
                case 'v':
                    vfonly = 1;
                    task = EDT_VerifyOnly;
                    break;

                case 'c':
                    chkonly = 1;
                    task = EDT_CheckIdOnly;
                    break;

                case 'r':
#ifdef NO_FS
                    nofs = 0;
#else
                    sprintf(errbuf, "-r specified but not compiled with embedded bitfiles.\nrecompile with -DNO_FS and try again\n");
                    usage(errbuf);
                    ret = 1;
#endif
                    break;
                case 'V': /* verbose */
                    vb = vb ? 0 : 1;
                    break;

                case 'I':
                case 'i':
                    if (strlen(iarg = argv[i]) > 2)
                    {
                        if (++i == argc)
                        {
                            printf("unfinished %s option. takes one argument\n", iarg);
                            ret = 1;
                        }
                        else switch(iarg[2])
                        {
                            case 's': strncpy(new_sn, argv[i], 11); 
                                      break;
                            case 'p': strncpy(new_pn, argv[i], 11); 
                                      break;
                            case 'v': strncpy(new_rev, argv[i], 5); 
                                      break;
                            case 'c': strncpy(new_clock, argv[i], 5); 
                                      break;
                            case 'o': strncpy(new_opts, argv[i], 11); 
                                      break;
                            case 'O': strncpy(new_osn, argv[i], 31); 
                                      break;
                            case 'n': strncpy(new_optsn, argv[i], 11); 
                                      break;
                            case 'm': strncpy(new_maclist, argv[i], MACLIST_SIZE); 
                                      break;
                            default:
                                      usage("invalid -Ix  or -ix option\n");
                        }
                    }
                    else if (iarg[0] == 'I')
                        proginfo = 2;
                    else proginfo = 1;
                    break;
                case 'p':             /* print a summary of installed boards */
                    summary_only=1;
                    break;
                case 'q':
                    quiet=1;
                    break;
                case 'f':
                    force_fast=1;
                    edt_flash_set_do_fast(1) ;
                    break;

                    /* DEBUG add delays, rewrites or rereads to debug fast mode */
                case 'F':
                    if (++i == argc)
                    {
                        printf("unfinished -F option. takes 1 int arg, set bits as follows:\n");
                        printf("0 (0x1): add extra writes in tst_write\n");
                        printf("1 (0x2): add extra dummy reads in tst_write\n");
                        printf("2 (0x4): add extra reads in tst_read\n"); 
                        printf("3 (0x8): unassigned\n");
                        printf("4 (0x10): msleep(1) before every tst_write\n");
                        printf("5 (0x20): msleep(1) before every tst_read\n");
                        ret = 1;
                    }
                    edt_flash_set_debug_fast(strtol(argv[i], NULL, 16));
                    break;

#ifdef PSN
#include "pciload_sn.c"
#endif

                case 'S':
                    edt_flash_set_force_slow(1) ;
                    edt_flash_set_do_fast(0) ;
                    break;

                case 'h':
                    usage(errbuf);
                    break;

                case 'X':
                    clear_extra = 1;
                    break;

                default:
                    sprintf(errbuf, "\nunknown option \"-%c\"", argv[i][1]);
                    usage(errbuf);
            }
        }
    }


    if (ret)
        return (ret);


    /* check for valid values */
    if (sect != IS_DEFAULT_SECTOR &&
            (sect < 0 || sect > 3))
    {
        printf("Sector number %d should be between 0 and 3\n", sect);
        return(1);
    }

    /* Determine task */

    if (strcmp(fname, "update") == 0)
    {
        task = EDT_AutoUpdate;
        fname[0] = '\0';
    } 
    else if (strcmp(fname, "checkupdate") == 0)
    {
        task = EDT_CheckUpdate;
        fname[0] = '\0';
    } 
    else if ((strcmp(fname, "erase") == 0) || (strcmp(fname, "ERASE") == 0))
    {
        task = EDT_Erase;
    }
    else if (strcmp(fname, "verify") == 0)
    {
        task = EDT_VerifyOnly;
        fname[0] = '\0';
        vfonly = 1;
    }
    else if ((strcmp(fname, "checkid") == 0))
    {
        task = EDT_CheckIdOnly;
        fname[0] = '\0';
        chkonly = 1;
    }
    else if (fname[0])
    {
        if ((task != EDT_VerifyOnly) && (task != EDT_CheckIdOnly))
            task = EDT_LoadProm;
    }
    else if (strcmp(fname, "help") == 0)
    {
        usage(errbuf);
        return(0);
    }

    if (dev[0] == 0)
        strcpy(dev, EDT_INTERFACE); 

    if (*unitstr)
        unit = edt_parse_unit(unitstr, dev, NULL);
    else
        unit = 0;

    /* create critical defaults */
    if (task == EDT_Enumerate)
    {
        if (*unitstr)
            edt_enumerate_and_summarize(dev, unit, sect);
        else 
            edt_enumerate_and_summarize("", -1, sect);

        return 0;
    }


    /* DO SOMETHING FOR ONLY ONE BOARD */
    /* if no name, and task == update or verify or ?? */
    if (!fname[0])
    {
        if (unit == -1)
            unit = 0 ;

        if (dev[0] == 0)
            strcpy(dev, EDT_INTERFACE); 

        if ((edt_p = edt_open(dev, unit)) == NULL)
        {
            char str[EDT_STRBUF_SIZE];
            sprintf(str, "%s unit %d", EDT_INTERFACE, unit) ;
            edt_perror(str) ;
            return (1) ;
        }

        if (task == EDT_DumpFlash)
        {
            promcode = edt_flash_prom_detect(edt_p, &stat);
            ep = edt_get_prominfo(promcode);

            if (dump_addr == 0xffffffff)
               dump_addr = edt_get_id_addr(promcode, ep->defaultseg);

           if (dump_size == 0xffffffff)
               dump_size = edt_get_flashsize(promcode);

            edt_dump_flash(edt_p, dump_addr, dump_size);
            edt_close(edt_p);
            return 0;
        }
        else if ((task != EDT_Erase) && edt_flash_get_fname(edt_p, fname) != 0)
        {
            edt_msg(EDT_MSG_FATAL,"Unable to...? \n");
            return (1) ;
        }

        edt_close(edt_p);
    }

    if ((edt_p = edt_open(dev, unit)) == NULL)
    {
        char    str[64];

        sprintf(str, "edt_open(%s%d)", dev, unit);
        edt_perror(str);
        return(2);
    }

    promcode = edt_flash_prom_detect(edt_p, &stat);
    printf("\n%s unit %d (%s):\n", dev, unit, edt_idstring(edt_p->devid, promcode));

    /* get & print board info */
    ep = edt_get_prominfo(promcode);
    sector = (sect == IS_DEFAULT_SECTOR)? ep->defaultseg:sect;
    edt_read_prom_data(edt_p, promcode, sector, &existing);

    if (clear_extra)
    {
        existing.extra_size = 0;
        memset(existing.extra_buf,0, sizeof(existing.extra_buf));
    }

    edt_print_prom_summary(edt_p, sect);

    /* some devices can't program with jumper in protected position */
    if ((task != EDT_VerifyOnly) && (task != EDT_CheckIdOnly) && edt_flash_is_protected(edt_p))
    {
        if (edt_is_micron_prom(edt_p) && (task == EDT_LoadProm))
        {
            /* special case A5 -- can't get the jumper easily and it doesn't work the same as
             * others in all revs */
            if (edt_p->devid == PE8G3A5_ID)
            {
              printf("WARNING: Protected ROM jumper is in the PROT position. If this is a rev 10\n");
              printf("or newer A5 board, the default (non-protected) boot sector will be programmed.\n");
              printf("If that's your intent, press ENTER now. If this is a rev 09 or earlier A5 board,\n");
              printf("it will attempt to program the PROTECTED sector which is likely NOT what you\n");
              printf("want. In that case, press 'q' now to quit, then withOUT powering down, move the\n");
              printf("jumper to the NORM position before trying again. In any case, once programming\n");
              printf("is complete you must power down the computer then ensure the jumper is back\n");
              printf("in the NORM position before powering up and using the PCIe G3 A5 board again.\n\n");
            }
            else
            {
              printf("Error: Protected ROM jumper is in the PROTECTED position.\n");
              printf("Before programming this device, you must move the jumper to the NORM\n");
              printf("position, or use an alternate method to program the protected sector.\n\n");
              exit(1);
            }
        }
        else
        {
          printf("Caution: Protected ROM jumper is in the PROTECTED position.\n");
          printf("Some EDT devices don't allow programming in this mode -- if programming\n");
          printf("fails, remove the jumper (without powering down the system) and re-run\n");
          printf("the pciload programming command.\n\n");
        }
    }

    modified = existing;

    if (edt_merge_esns(new_esn, modified.esn, new_sn, new_opts, new_pn, new_rev, new_clock) != NULL)
        strcpy(modified.esn, new_esn);
    if (new_osn[0])
        strcpy(modified.osn, new_osn);
    if (new_optsn[0])
        strcpy(modified.optsn, new_optsn);
    if (new_maclist[0])
        edt_copy_maclist(modified.maclist, new_maclist);

    /*
     * things to take care of in the next switch statement:
     * 
     * 1) if no fname, just get info.  otherwise, 2) program the sectors as
     * needed--- fix_fname's middle arg is 0 for 3.3volt file, 1 for 5volt
     * file, and -1 for non-XLA boards(no voltage) otherwise,
     * program_verify_PROMTYPE()
     * 
     * things like making sure values are within range should be taken care of
     * inside the getinfo and program_verify functions.
     */

    if (fname[0])
    {
        EdtLoadNames names_to_load[2];
        EdtPciLoadVerify pvf;

        int n_names = 0;

        /* dflt to fast with this xilinx */
        if (edt_flash_get_force_slow()) 
            edt_flash_set_do_fast(0) ;
        else 
            edt_flash_set_do_fast(1) ;

        /* get list of file names and sectors to load */
        if (!flash_dir[0])
            sprintf(flash_dir, "%s%cflash", edt_home_dir(edt_p), DIRECTORY_CHAR);

        n_names = get_names_to_load(promcode, fname, flash_dir, names_to_load, sect, nofs);

        pvf = get_program_verify(promcode);

        /* ask for serial # if needed */

        sector = (sect == IS_DEFAULT_SECTOR)? ep->defaultseg:sect;

        if (task == EDT_LoadProm)
        {
            if (n_names == 0)
            {
                fprintf(stderr, "Error: invalid basename or filename argument\n");
                exit(1);
            }

            if ((saveinfo = edt_check_ask_info(edt_p,
                            promcode,
                            sector,
                            &modified, 
                            0,
                            proginfo,
                            nofs)) < 0)
                return -1;
        }

        for (i=0;i<n_names;i++)
        {
            if ((ret = load_bitfile(names_to_load[i].fname, &bitfile, promcode, nofs)) != 0)
                return(ret);     

            /* make sure it makes sense */
            if (task == EDT_VerifyOnly || task == EDT_CheckIdOnly)
                edt_read_prom_data(edt_p, promcode, names_to_load[i].sector, &modified);

            ret = check_load_values(edt_p, 
                    promcode, 
                    &bitfile, 
                    &modified, 
                    task,
                    (i == 0),
                    names_to_load[i].sector);

            if (task == EDT_CheckUpdate && ret == 0)
                skipUpdate = 1;

            if (task == EDT_VerifyOnly && ret != 0)
                skipUpdate = 1;

            if (!(task == EDT_VerifyOnly || task == EDT_CheckIdOnly))
                strncpy(modified.id, bitfile.hdr.promstr, sizeof(modified.id)-1);

            if (!(task == EDT_CheckIdOnly) && !skipUpdate)
            {
                verify_status += pvf(edt_p, &bitfile,  &modified, promcode, 
                        names_to_load[i].sector,
                        (task == EDT_VerifyOnly), (i==0), vb);
            }
        }
    }

    if (nofs)
        saveinfo = 0;

    /* save the serial # info to a file if changed and programmed */
    if ((!(vfonly || chkonly)) && (verify_status == 0) && (saveinfo))
    {
        edt_save_info_file(edt_p->devid, modified.esn, vb);
    }

    edt_close(edt_p);

    if (*fname && !(vfonly || chkonly) && !skipUpdate)
    {
        if (verify_status != 0)
        {
            printf("One or more sectors failed verification. Errors may be due to slow host\n");
            printf("writes/reads. Try -F <level> or -S (pciload --help to see all options)\n");
            ret = 3;
        }
        else if (strcmp(fname, "ERASE") == 0)
        {
            printf("\n");
            printf("This board will be non-functional after the next power-down, and will remain\n");
            printf("so until the appropriate EDT PCI FPGA firmware is loaded back into the\n");
            printf("Board's EEPROM.\n\n");
            printf("To reload the firmware, power the system down, move the protected mode jumper\n");
            printf("on the board to the \"protected sector\" positon (pins 1-2). Power the system\n");
            printf("up again, move the jumper back to the non-protected position (pins 2-3),\n");
            printf("then load the appropriate firmware using the pciload program.\n");
        }
        else
        {
            printf("The firmware update will take effect after the next time you cycle power.\n");
            ret = 0;
        }
    }
    return(ret);
}


void
edt_pciload_printversion(char *devname, int unit, int do_lib_version)
{
    u_int v;
    edt_version_string version;
    edt_version_string build;
    EdtDev *edt_p = edt_open(devname, unit);

    edt_get_library_version(NULL, version, sizeof(version));
    v = edt_get_version_number();

    if (do_lib_version)
        printf("\nLibrary version %s\n", version);

    if (edt_p)
    {
        edt_get_driver_version (edt_p, version, sizeof (version));
        edt_get_driver_buildid (edt_p, build, sizeof (version));
    }
    else strcpy(version, "unknown");

    printf("%s driver version %s\n", devname, version);

}


