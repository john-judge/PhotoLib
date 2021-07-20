/*
* bitload.c -- load the EDT PCI interface Xilinx .bit file
* 
* If full path is specified (i.e. if there are any slashes in <file>) then it
* will just look for the file. Otherwise it uses the following search path
* (dir is . by default, or -d <dir> if specified):
* 
* dir/40xx/file if dv(k) dir/bitfiles/dv(k)/40xx/file
* dir/40xx/file otherwise
* 
* if it finds but fails to load anywhere above the 40xx directories, then
* it will quit trying. Otherwise it tries the next thing.
* 
* (C) 1997-2000 Engineering Design Team, Inc.
*/

#include "edtinc.h"
#include <stdlib.h>
#include "edt_bitload.h"

#ifndef _NT_
#include <dirent.h>
#endif



static void
usage(char *progname, char *err)
{
    fprintf(stderr, "%s", err);
    fprintf(stderr, "usage: %s [options] [bitfile]\n", progname);
    fprintf(stderr, "If bitfile is specified, it will be loaded.\n");
    fprintf(stderr, "If bitfile isn't specified, the name of loaded bitfile is displayed.\n");
    fprintf(stderr, "-u unit     %s unit number (default 0)\n", EDT_INTERFACE);
    fprintf(stderr, "-d dir      base directory for search for bitfile (default ., then ./bitfiles\n");
    fprintf(stderr, "-e          use nofs (embedded) array not file (must be compiled with -DNO_FS)\n");
    fprintf(stderr, "-v, -V      verbose, and more Verbose output\n" );
    fprintf(stderr, "-o          override ss/gs size check (debug)\n" );
    fprintf(stderr, "-q          quiet (no output at all)\n" );
    fprintf(stderr, "-S          sleep(0) while programming\n");
    fprintf(stderr, "            Improves interactivity on some systems.\n");

    fprintf(stderr, "-s          slow - readback while progamming\n");
    fprintf(stderr, "            (Fast is the default - it may not work on all motherboards)\n");
    fprintf(stderr, "bitfile     bit file to load. If this argument is a path, will try to load\n");
    fprintf(stderr, "            it directly. If just a bitfile name, will search for the\n");
    fprintf(stderr, "            appropriate version of that file, starting in <dir>/bitfiles\n");
    fprintf(stderr, "            (PCI CD or GP), <dir>/dv/bitfiles (PCI DV), or <dir>/dvk/bitfiles\n");
    fprintf(stderr, "            (PCI DVK or PCI DV44) (see -d flag).\n");

    exit(1);
}

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
int
bitload(char *command_line)
#else
int
main(int argc, char **argv)
#endif
{
    int     unit = 0;
    int     ret, len;
    int     verbose=1;
    int     nofs = 0;
    int     ovr = 0;
    int     flags = 0;
    int     skip_load = 0;
    unsigned int edt_dma_cfg_save;
    char    edt_devname[128];
    char    *basedir = NULL;
    char    bitname[256];
    char    errstr[128];
    char   *unitstr = "0";
    char   *progname;
    EdtDev *edt_p;
    u_int do_readback = 0; 
    u_int do_sleep = 0; 
#ifdef NO_MAIN
    char **argv  = 0 ;
    int argc = 0 ;
    opt_create_argv("bitload",command_line,&argc,&argv);
#endif

    /* check args */
    progname = argv[0];

    --argc;
    ++argv;
    while (argc && argv[0][0] == '-')
    {
        switch (argv[0][1])
        {
        case 'q':
            verbose = 0;
            break;

        case 'v':
        case 'V':
            verbose = 3;
            break;


        case 'd':
        case 'b':		/* compat */
            ++argv;
            --argc;
            basedir = strdup(argv[0]);
            break;

        case 'u':
            ++argv;
            --argc;
            if (argc < 1) 
            {
		        usage(progname, "Error: option 'u' requires a numeric argument\n");
	        }
	        if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
            {
                unitstr = argv[0];
            }
            else 
            {
		        usage(progname, "Error: option 'u' requires a numeric argument\n");
	        }
	        break;

        case 'e':
#ifdef NO_FS
            nofs = 1;
#else
            printf("'e' option specified but not compiled with NO_FS\n");
            exit(1);
#endif
            break;

        case 'S':
            do_sleep = 1;
            break;

        case 's':
            do_readback = 1;
            break;

        case 'o':
            ovr = 1;	/* debug use only */
            break;

        case 'h':
        case 'H':
        case '?':
            usage(progname, "");
            break;

        default:
            usage(progname, "unknown option\n");
        }
        argc--;
        argv++;
    }

    unit = edt_parse_unit(unitstr, edt_devname, EDT_INTERFACE);

    if (argc < 1)
    {
        if ((edt_p = edt_open_quiet(edt_devname, unit)) != NULL) {


	    if ( ID_HAS_COMBINED_FPGA(edt_p->devid) )
	    {
		printf("Interface bitfile on %s%d:  Combined with PCI/PCIe FPGA; use pciload instead.\n", edt_devname, unit);
	    }
	    else
	    {

		edt_bitpath pathbuf;
		edt_get_bitpath(edt_p, pathbuf, sizeof(edt_bitpath));
		printf("Interface bitfile on %s%d: %s\n", 
		    edt_devname, unit, pathbuf[0] ? pathbuf : "None loaded");

	    }

            edt_close(edt_p);

        } else {
            sprintf(errstr, "Opening card to get bitfile name failed: edt_open(%s%d)", 
                edt_devname, unit);
            edt_perror(errstr);
        }

        printf("For usage message, run '%s -h'.\n", progname);
        exit(0);
    }

    len = (int) strlen(argv[0]);

    strcpy(bitname, argv[0]);

#ifdef NO_FS
    if (nofs) /* strip off .bit from name if nofs */
    {
        strcpy(bitname, argv[0]);
        if ((len >= 4) && (strcasecmp(&bitname[len-4], ".bit") == 0))
            bitname[len-4] = '\0';
    }

#endif

    if ((edt_p = edt_open(edt_devname, unit)) == NULL)
    {
        sprintf(errstr, "edt_open(%s%d)", edt_devname, unit);
        edt_perror(errstr);
        edt_msg(EDTAPP_MSG_FATAL, "couldn't load xilinx");
        exit(1);
    }


    edt_set_verbosity(verbose);

    if (nofs)
        flags |= BITLOAD_FLAGS_NOFS;
    if (ovr)
        flags |= BITLOAD_FLAGS_OVR;

    if (do_readback)
        flags |= BITLOAD_FLAGS_READBACK;

    if (do_sleep)
        flags |= BITLOAD_FLAGS_SLEEP;

    ret = edt_bitload(edt_p, basedir, bitname, flags, skip_load);

    edt_close(edt_p);

    if (ret)
    {
        edt_msg(EDTAPP_MSG_FATAL, "bitload failed");
        if (verbose < 2)
            edt_msg(EDTAPP_MSG_FATAL, " (run with -v to look for the cause of the failure)\n");
        else printf("\n");
#ifdef NO_MAIN
        return(1) ;
#else
        exit(1);
#endif
    }

#ifdef NO_MAIN
    return(0) ;
#else
    exit(0);
#endif
}



