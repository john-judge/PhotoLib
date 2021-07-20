/* "@(#)clsiminit.c	1.20 06/18/07 EDT" */
#include "edtinc.h"
#include <stdlib.h>

#include "clsim_lib.h"

#define ABOUT_TEXT "program to initialize camera link simulator board"

    static void
usage(char *progname)
{
    printf("%s: %s\n", progname, ABOUT_TEXT);
    printf("usage: %s [-u unit] [-F freq] [-f <config file>] [other OPTIONS...]\n", 
            progname);
    puts("OPTIONS: \n");
    puts("-u unit - sets device number (default 0)");
    puts("-c chan - sets device channel (MDR connector, 0 [default] or 1)");
    puts("-B - do NOT load bitfile");
    puts("-B bitfile - load user specified bitfile");
    puts("-l - enable uartloop (simulator will loop back any serial sent to it");
    puts("-q - disables program output (quiet mode)");
    puts("-s - sets simulator so data comes from internal counter instead of DMA");
    puts("-C - sets first word of frame to be framecount");
    printf("-F freq - sets pixel clock in MHz, takes a floating point arg (default %2.1f)\n",
            PDV_CLS_DEFAULT_FREQ);
    puts("-n nominal - overrides the default nominal frequency for si570 oscillator (usually 100 or 125)");
    puts("-t #taps - sets number of taps to #taps.");

    puts("");

    puts("-w width (default 1024) \n");
    puts("-h height (default 1024) \n");
    puts("-d depth (default 8) \n");
    puts("-f <config file> Use camera config for parameters\n");

    printf("-v vblank - sets interval between frames in lines (default %d)\n", 
            PDV_CLS_DEFAULT_VGAP);
    printf("-V VcntMax - sets total # of lines in frame ");
    printf("(ignored if vblank specified instead)\n");

    puts("");

    printf("-g hblank - sets # of pixel clocks of horizontal blanking (default %d)\n", 
            PDV_CLS_DEFAULT_HGAP);
    printf("-H HcntMax - sets total # of pixel clocks per line including blanking \n\
            (ignored if hblank specified instead)\n");

    puts("");

    printf("-r - \"reset\" board - You probably DO NOT want to do this: \n");
    printf("\tall settings (width, height, etc) are zero'd.\n");
    printf("--verbose - prints more information");
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
clsiminit(char *command_line)
#else
    int
main(argc, argv)
    int     argc;
    char  **argv;
#endif
{
    EdtDev *pdv_p;

    char *progname = NULL;
    int     unit = 0;
    int     chan = 0;
    int setup = 0;
    int no_bitload = 0;
    char bitfile_name[256];
    char *camera = NULL;
    PdvDependent *dd_p;
    Edtinfo ei;
    int i;
    int reset = 0;
    int uartloop = 0;
    int quiet = 0;
    int verbose = 0;
    int level; /* output message level */

#ifdef NO_MAIN
    char **argv  = 0 ;
    int argc = 0 ;
    opt_create_argv("clsiminit",command_line,&argc,&argv);
#endif

    progname = argv[0];

    strcpy(bitfile_name, "clsim.bit");

    if ((dd_p = pdv_alloc_dependent()) == NULL)
    {
        printf("alloc_dependent FAILED -- exiting\n");
        exit(1);
    }

    /* first find unit and channel - so we can open board to get dd */

    for (i=1;i<argc;i++)
    {
        if (argv[i][0] == '-')
        {
            if (argv[i][1] == 'u')
            {
                if (i+1 >= argc) {
                    fprintf(stderr, "Error: option 'u' requires argument\n");
                    usage(progname);
                    exit(1);
                }
                unit = atoi(argv[i+1]);
            } 
            else if (argv[i][1] == 'c')
            {
                if (i+1 >= argc) {
                    fprintf(stderr, "Error: option 'c' requires argument\n");
                    usage(progname);
                    exit(1);
                }
                chan = atoi(argv[i+1]);
            } 
            else if (argv[i][1] == 'r')
            {
                reset = 1;
            } 
            else if (argv[i][1] == 'f')
            {
                if (i+1 >= argc) {
                    fprintf(stderr, "Error: option 'f' requires argument\n");
                    usage(progname);
                    exit(1);
                }
                camera = argv[i+1];
                setup = 1;
            }
            else if (argv[i][1] == '-') {
                if (strncmp(argv[i], "--help", 3) == 0) { /* only need check the first few chars */
                    usage(progname);
                    exit(0);
                } else if (strncmp(argv[i], "--verbose", 4) == 0) {
                    ++verbose;
                } else {
                    fprintf(stderr, "Unknown option \"%s\"\n", argv[i]);
                    usage(progname);
                    exit(1);
                }
            }
        }
    }

    pdv_p = pdv_open_channel("pdv", unit, chan);

    if (pdv_p == NULL)
    {
        fprintf(stderr, "Could not open EDT device for writes\n");
        exit(1);
    }

    if (!pdv_is_simulator(pdv_p))
    {
        fprintf(stderr, "\nclsiminit is for simulators. To initialize an EDT framegrabber board, use initcam.\n");
        pdv_close(pdv_p);
        exit(1);
    }


    dd_p = pdv_p->dd_p;

    if (reset)
        memset(dd_p, 0, sizeof(PdvDependent));


    /* Read this here so arguments can override values in config file */

    if (camera)
    {
        int retval = pdv_readcfg(camera, dd_p, &ei);

        if (retval == -1)
        {
            fprintf(stderr, "Error reading config file '%s'\n",
                    camera);
            pdv_close(pdv_p);
            exit(1);
        }
    }

    --argc;
    ++argv;
    while (argc && argv[0][0] == '-')
    {
        switch (argv[0][1])
        {
            /* skip options parsed earlier */
            case 'u':
            case 'f':
            case 'c':
                ++argv;
                --argc;
            case 'r':
                break;

            case 's':
                /* do dma */
                dd_p->cls.flags.Cfg.datacnt = 1;
                setup = 1;
                break;

            case 'C':
                dd_p->cls.flags.Cfg.firstfc = 1;
                setup = 1;
                break;

            case 'F':    /* Set PLL for desired pixel clock freq in MHz */
                ++argv;
                --argc;
                if (argc < 1) {
                    fprintf(stderr, "Error: option 'F' requires argument\n");
                    usage(progname);
                    exit(1);
                }
                setup=1;
                dd_p->cls.pixel_clock = (float)atof(argv[0]);    /* Convert arg to float */
                dd_p->pclock_speed = (int)(dd_p->cls.pixel_clock + 0.5); /* redundant dd_p var */
                break;

            case 'v':
                ++argv;
                --argc;
                if (argc < 1) {
                    fprintf(stderr, "Error: option 'v' requires argument\n");
                    usage(progname);
                    exit(1);
                }
                dd_p->cls.vblank = atoi(argv[0]);
                setup=1;

                break;

            case 'V':
                ++argv;
                --argc;
                dd_p->cls.Vcntmax = atoi(argv[0]);
                setup=1;
                break;

            case 'B':
                if (argc < 2)
                    no_bitload = 1;
                else
                {
                    if (argv[1][0] != '-')
                    {
                        /* set bitfile name */
                        ++argv;
                        --argc;
                        strcpy(bitfile_name,argv[0]);
                    }
                    else
                        no_bitload = 1;
                }
                break;

            case 'n':
                ++argv;
                --argc;
                dd_p->cls.si570_nominal = atof(argv[1]);
                if (dd_p->cls.si570_nominal < 2000.0)
                    dd_p->cls.si570_nominal *= 1000000.0;
                break;

            case 'q':
                quiet = 1;
                break;

            case 'l':
                dd_p->cls.flags.Cfg.uartloop = 1;
                break;

            case 'w':
                ++argv;
                --argc;
                if (argc < 1) {
                    fprintf(stderr, "Error: option 'w' requires argument\n");
                    usage(progname);
                    exit(1);
                }
                setup=1;
                dd_p->width = atoi(argv[0]);
                break;

            case 'H':
                ++argv;
                --argc;
                if (argc < 1) {
                    fprintf(stderr, "Error: option 'H' requires argument\n");
                    usage(progname);
                    exit(1);
                }
                dd_p->cls.Hcntmax = atoi(argv[0]);
                setup=1;
                break;

            case 'g':
                ++argv;
                --argc;
                if (argc < 1) {
                    fprintf(stderr, "Error: option 'G' requires argument\n");
                    usage(progname);
                    exit(1);
                }
                dd_p->cls.hblank = atoi(argv[0]);
                setup=1;
                break;

            case 'h':
                ++argv;
                --argc;
                if (argc < 1) {
                    fprintf(stderr, "Error: option 'h' requires argument\n");
                    usage(progname);
                    exit(1);
                }
                dd_p->height = atoi(argv[0]);
                setup=1;
                break;

            case 'd':
                ++argv;
                --argc;
                if (argc < 1) {
                    fprintf(stderr, "Error: option 'd' requires argument\n");
                    usage(progname);
                    exit(1);
                }
                dd_p->depth = atoi(argv[0]);
                setup=1;
                break;

            case 't':
                ++argv;
                --argc;
                if (argc < 1) {
                    fprintf(stderr, "Error: option 't' requires argument\n");
                    usage(progname);
                    exit(1);
                }
                dd_p->cls.taps = atoi(argv[0]);
                setup=1;
                break;


            case '-': /* these already parsed above; nothing to do here */
                break;

            default:
                usage(progname);
                exit(0);
        }
        argc--;
        argv++;
    }

    /* setup output message level */
    level = edt_msg_default_level();
    if (verbose) 
    {
        level |= EDTAPP_MSG_INFO_1;
        level |= EDTLIB_MSG_INFO_1;
    } 
    if (verbose > 1) 
    {
        level |= EDTAPP_MSG_INFO_1 
            | EDTAPP_MSG_INFO_2 
            | EDTLIB_MSG_INFO_1 
            | EDTLIB_MSG_INFO_2;
    }
    edt_msg_set_level(edt_msg_default_handle(), level);


    /* pci express dv clsim doesn't have interface xilinx to load */
    if (pdv_p->devid != PDVCLS_ID)
    {
        no_bitload = 1; 
    }

    if (setup || reset)
    {
        if (!no_bitload)
        {
            int flags = 0;
#ifdef NO_FS
            flags |= BITLOAD_FLAGS_NOFS;
#endif
            edt_bitload(pdv_p, "camera_config" /* basedir */,  bitfile_name, flags, 0 /* skip_load */);
        }

        if (pdv_cls_dep_sanity_check(pdv_p) == 0)
        {
            pdv_cls_set_dep(pdv_p);
        }
    }

    if (!quiet)
    {
        edt_msleep(500);
        pdv_cls_dump_state(pdv_p);
    }
    return(0) ;
}

