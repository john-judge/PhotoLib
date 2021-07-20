/*
 * initcam.c -- initialize the device driver, camera and
 * PCI DV board for the camera (or simulator) in use
 * 
 * (C) 1997-2000 Engineering Design Team, Inc.
 */
#include "edtinc.h"

static void    usage(char *progname);

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
initcam(char *command_line)
#else
    int
main(int argc, char *argv[])
#endif
{
    int     unit = 0;
    int     verbose = 1;
    int     no_bitload = 0;
    char    cfgname[256];
    char    bitdir[256];
    char    edt_devname[256];
    char    errstr[64];
    char   *progname = "initcam" ;
    char    logname[256];
    EdtDev *edt_p = NULL;
    Edtinfo edtinfo;
    char   *unitstr = "0";
    int	    channel = 0 ;
    static char *debug_env = NULL;
    Dependent *dd_p;
    int     pdv_debug = 0;
#ifdef NO_FS
    int     nofs_cfg = 0;
#endif
    int     level;
#ifdef NO_MAIN
    char **argv  = 0 ;
    int argc = 0 ;
    opt_create_argv("initcam",command_line,&argc,&argv);
#endif

    *cfgname = '\0';
    *edt_devname = '\0';
    *bitdir = '\0';
    *logname = '\0';

    /* process arguments */
    --argc;
    ++argv;
    while (argc && argv[0][0] == '-')
    {
        switch (argv[0][1])
        {
            case 'u':		/* unit (board) number */
                ++argv;
                --argc;
                if (argc < 1) {
                    fprintf(stderr, "Error: option 'u' requires argument\n");
                    usage(progname);
                    exit(1);
                }
                unitstr = argv[0];
                break;

            case 'c':		/* channel number for multi-channel devices */
                ++argv;
                --argc;
                if (argc < 1) {
                    fprintf(stderr, "Error: option 'c' requires argument\n");
                    usage(progname);
                    exit(1);
                }
                channel = atoi(argv[0]);
                break;

            case 'O':		/* logfile name */
                ++argv;
                --argc;
                if (argc < 1) {
                    fprintf(stderr, "Error: option 'O' requires file argument\n");
                    usage(progname);
                    exit(1);
                }
                strcpy(logname, argv[0]);
                break;

            case 'q':		/* quiet mode */
                verbose = 0;
                break;

            case 'v':		/* verbose mode */
                verbose = 1;
                break;

            case 'V':		/* really verbose mode */
                verbose = 2;
                break;

            case 'f':		/* config filename */
                ++argv;
                --argc;
                if (argc < 1) {
                    fprintf(stderr, "Error: option 'f' requires argument\n");
                    usage(progname);
                    exit(1);
                }
                strcpy(cfgname, argv[0]);
#ifdef NO_FS
                strcpy(bitdir, "_NOFS_");
                nofs_cfg = 1;
#endif
                break;

            case 'e':		/* no file system embedded bitfile name */
#ifdef NO_FS
                ++argv;
                --argc;
                if (argc < 1) {
                    fprintf(stderr, "Error: option 'e' requires argument\n");
                    usage(progname);
                    exit(1);
                }
                strcpy(cfgname, argv[0]);
                strcpy(bitdir, "_NOFS_");
                nofs_cfg = 1;
#else
                fprintf(stdout, "\n-e specified but not compiled with nofs configs.\nrecompile with -DNO_FS and try again\n\n");
                usage(progname);
                exit(1);
#endif
                break;

            case 'B':		/* don't load bitfile */
                no_bitload = 1;
                break;

            case 'b':
            case 'd':		/* compat */
                ++argv;
                --argc;
                if (argc < 1) {
                    fprintf(stderr, "Error: options 'b' or 'd' require argument\n");
                    usage(progname);
                    exit(1);
                }
                strcpy(bitdir, argv[0]);
                break;

            case 'h':		/* help */
                usage(progname);
                exit(0);
                break;

            case '-':
                if (strcmp(argv[0], "--help") == 0) {
                    usage(progname);
                    exit(0);
                } else {
                    fprintf(stderr, "unknown option: %s\n", argv[0]);
                    usage(progname);
                    exit(1);
                }
                break;

            default:
                fprintf(stdout, "unknown flag -'%c'\n", argv[0][1]);
                usage(progname);
                exit(1);
        }
        argc--;
        argv++;
    }

    if (!(*cfgname))
    {
        usage(progname);
        exit(1);
    }

    /*
     * not using pdv_open to open, but still using pdvlib calls, so force
     * debug if -v or -V or PDVDEBUG envvar
     */
    if (debug_env == NULL
            && ((debug_env = (char *) getenv("PDVDEBUG")) != NULL)
            && *debug_env != '0')
        pdv_debug = atoi(debug_env);

    /*
     * normally the pdvlib error handle gets initialized in pdv_open, but
     * initcam is a special case since it calls edt_open but also uses
     * pdvlib calls -- see edt_msg library.
     */
    level = edt_msg_default_level();

    if ((verbose > 1) && (pdv_debug < verbose))
        pdv_debug = verbose;

    if ((!verbose) && (!pdv_debug))
        level = 0;
    else
    {
        if (verbose > 0)
        {
            level |= EDTAPP_MSG_INFO_1;
            level |= PDVLIB_MSG_INFO_1;
            level |= PDVLIB_MSG_WARNING;
            level |= PDVLIB_MSG_FATAL;
        }
        if (verbose > 1)
            level |= EDTAPP_MSG_INFO_2;
        if (pdv_debug > 1)
            level |= PDVLIB_MSG_INFO_2;
    }

    edt_msg_set_level(edt_msg_default_handle(), level);
    if (*logname)
        edt_msg_set_name(edt_msg_default_handle(), logname);

    /* kind of kludgy... since pdv_setdebug doesn't actually USE edt_p,
     * its okay that it hasn't been opened yet , hence the NULL pointer
     */
    if (pdv_debug)
        pdv_setdebug(NULL, pdv_debug);

    /*
     * if porting this code to an application, be sure to free this 
     * and reallocate if you call pdv_initcam multiple times.
     */
    if ((dd_p = pdv_alloc_dependent()) == NULL)
    {
        edt_msg(PDVLIB_MSG_FATAL, "alloc_dependent FAILED -- exiting\n");
        exit(1);
    }

#ifdef NO_FS
    if (nofs_cfg)
    {
        int ret ; /* randall how should we deal with this? */
        ret = pdv_readcfg_emb(cfgname, dd_p, &edtinfo);
    }
    else
#endif
        if (pdv_readcfg(cfgname, dd_p, &edtinfo) != 0)
        {
            edt_msg(PDVLIB_MSG_FATAL, "readcfg FAILED -- exiting\n");
            exit(1);
        }

    if (no_bitload)
        strcpy(dd_p->rbtfile, "_SKIPPED_");

    /*
     * open the device
     */
    unit = edt_parse_unit_channel(unitstr, edt_devname, "pdv", &channel);
    edt_msg(EDTAPP_MSG_INFO_1, "opening %s unit %d....\n", edt_devname, unit);

    /*
     * IMPORTANT: pdv_initcam is a special case in that it requies a device pointer returned by use
     * edt_open_channel (or edt_open), NOT pdv_open_channel (or etc.). If you port this code to an
     * application that subsequently performs other operations (e.g. image capture) on the device,
     * edt_close should be called after pdv_initcam, then reopen with pdv_open_channel or pdv_open.
     */
    if ((edt_p = edt_open_channel(edt_devname, unit, channel)) == NULL)
    {
        sprintf(errstr, "edt_open(%s%d)", edt_devname, unit);
        edt_perror(errstr);
        return (1);
    }

    if (pdv_is_simulator(edt_p))
    {
        edt_msg(EDTAPP_MSG_FATAL,"\ninitcam is for framegrabbers. To initialize an EDT simulator board, use clsiminit.\n");
        edt_close(edt_p);
        exit(1);
    }

    if (edt_p->devid == PDVFOI_ID)
    {
#ifdef _FOI_SUPPORTED
        pdv_initcam_set_rci(edt_p, channel) ;
#else
        edt_msg(EDTAPP_MSG_FATAL,"FOI not supported after pkg v4.1.5.9\n");
        edt_close(edt_p);
        exit(1);
#endif
    }


    if (pdv_initcam(edt_p, dd_p, unit, &edtinfo, cfgname, bitdir,
                pdv_debug) != 0)
    {
        edt_msg(EDTAPP_MSG_FATAL,"initcam failed. Run with '-V' to see complete debugging output\n");
        edt_close(edt_p);
        exit(1);
    }

    edt_close(edt_p);
    edt_msg(EDTAPP_MSG_FATAL, "done\n");
#ifdef NO_MAIN
    return(0) ;
#else
    exit(0);
#endif
}

    static void
usage(char *progname)
{
#ifdef NO_FS
    printf("usage: %s [options] [-e <cfg_name> | -f <cfg_path>]\n", progname);
#else
    printf("usage: %s [options] -f <cfg_path>\n", progname);
#endif
    printf("     -b <bit_dir>  alternate bitfiles directory\n");
    printf("     -B            don't load the bitfile\n");
    printf("     -f <cfg_path> config file pathname (required)\n");
    printf("     -e <cfg_name> embedded config (for no-filesystem operation -- must be compiled with -DNO_FS)\n");
    printf("     -u <unit>     pdv unit number (default 0). A full device pathname\n");
    printf("                   or filename can be substituted \n");
    printf("     -c <channel>  channel #, for multi-channel boards\n");
#ifdef _FOI_SUPPORTED
    printf("     -F <foiunit>  FOI (RCI) unit number, when multiple RCI units exist\n");
#endif
    printf("                   (usually -F and -c are used together, with both specifying the same #)\n");
    printf("     -v, -V        verbose, and really verbose\n");
    printf("     -O logfile    output log file, will redirect any console output to specified file\n");
    printf("     -q            quiet; no output\n");
    printf("     -h, --help    this help message\n");
    printf("\n");
}

