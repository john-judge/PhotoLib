/**
 * @file
 * An example program to show usage of EDT PCI DV library to acquire and
 * optionally save single or multiple images from devices connected to EDT
 * high speed digital imaging interface. This variation of simple_take.c
 * shows use of the IRIG2 footer to check for the magic number at the end
 * of a frame and check /reset frame synch and optionally check the irig
 * time stamp (not applicable if no IRIG signal present)
 * 
 * Note that the IRIG2 functionality is only available on PCIe4 & 8 DVa C-Link
 * and VisionLink boards, and likely newer but not older boards such as the
 * PCIe 4/8 DV (no 'a') C-Link and PCIe4 C-Link and PCI boards.
 *
 * As of library versions 5.3.9.3 and later, IRIG2 frame validataion
 * functionality is embedded in the libary and activated via config file
 * directive, making this explicitly-coded example less useful if all you
 * are using it for is as a check for frame synchronization (see
 *  method_framesync in the Camera Configuration guide, and
 * pdv_enable_framesync in the API.)
 * 
 * (C) 2010-2012 Engineering Design Team, Inc.
 */
#include "edtinc.h"
#include "pdv_irig.h"

static void usage(char *progname);
static void 
save_image(u_char * image_p, int width, int height, int depth,
        char *basename, int count);

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
simple_take(char *command_line)
#else
    int
main(argc, argv)
    int     argc;
    char  **argv;
#endif
{
    int     i;
    int     unit = 0;
    int     overrun, overruns=0;
    int     timeouts, last_timeouts = 0;
    int     recovering_timeout = FALSE;
    char   *progname ;
    char   *cameratype;
    char    bmpfname[128];
    int     numbufs = 4;
    int     started;
    u_char *image_p;
    PdvDev *pdv_p;
    char    errstr[64];
    int     loops = 1;
    int     width, height, depth;
    char    edt_devname[128];
    int     channel = 0;
#ifdef NO_MAIN
    char **argv  = 0 ;
    int argc = 0 ;
    opt_create_argv("simple_take",command_line,&argc,&argv);
#endif

    progname = argv[0];


    edt_devname[0] = '\0';
    *bmpfname = '\0';

    /*
     * process command line arguments
     */
    --argc;
    ++argv;
    while (argc && ((argv[0][0] == '-') || (argv[0][0] == '/')))
    {
        switch (argv[0][1])
        {
            case 'u':		/* device unit number */
                ++argv;
                --argc;
                if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
                    unit = atoi(argv[0]);
                else
                    strncpy(edt_devname, argv[0], sizeof(edt_devname) - 1);
                break;

            case 'c':		/* device unit number */
                ++argv;
                --argc;
                if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
                    channel = atoi(argv[0]);
                break;

            case 'N':
                ++argv;
                --argc;
                numbufs = atoi(argv[0]);
                break;

            case 'i':
            case 'b':		/* bitmap or raster save filename */
                ++argv;
                --argc;
                strcpy(bmpfname, argv[0]);
                break;

            case 'l':
                ++argv;
                --argc;
                loops = atoi(argv[0]);
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
                fprintf(stderr, "unknown flag -'%c'\n", argv[0][1]);
            case '?':
            case 'h':
                usage(progname);
                exit(0);
        }
        argc--;
        argv++;
    }

    /*
     * open the interface
     * 
     * EDT_INTERFACE is defined in edtdef.h (included via edtinc.h)
     *
     * edt_parse_unit_channel and pdv_open_channel) are equivalent to
     * edt_parse_unit and pdv_open except for the extra channel arg and
     * would normally be 0 unless there's another camera (or simulator)
     * on the second channel (camera link) or daisy-chained RCI (PCI FOI)
     */
    if (edt_devname[0])
    {
        unit = edt_parse_unit_channel(edt_devname, edt_devname, EDT_INTERFACE, &channel);
    }
    else
    {
        strcpy(edt_devname, EDT_INTERFACE);
    }

    if ((pdv_p = pdv_open_channel(edt_devname, unit, channel)) == NULL)
    {
        sprintf(errstr, "pdv_open_channel(%s%d_%d)", edt_devname, unit, channel);
        pdv_perror(errstr);
        return (1);
    }

    /*
     * get image size and name for display, save, printfs, etc.
     */
    width = pdv_get_width(pdv_p);
    height = pdv_get_height(pdv_p);
    depth = pdv_get_depth(pdv_p);
    cameratype = pdv_get_cameratype(pdv_p);

    /*
     * enable irig2 footer (note: will provide magic # for and framecount for sync
     * whether or not irig timesource is actually here. Note2: can also enable in
     * config file via method_header_type: IRIG2
     */
    pdv_set_header_type(pdv_p, HDR_TYPE_IRIG2, 1, 2, 0);

    /*
     * allocate four buffers for optimal pdv ring buffer pipeline (reduce if
     * memory is at a premium)
     */
    pdv_multibuf(pdv_p, numbufs);

    printf("reading %d image%s from '%s'\nwidth %d height %d depth %d\n",
            loops, loops == 1 ? "" : "s", cameratype, width, height, depth);

    /*
     * prestart the first image or images outside the loop to get the
     * pipeline going. Start multiple images unless force_single set in
     * config file, since some cameras (e.g. ones that need a gap between
     * images or that take a serial command to start every image) don't
     * tolerate queueing of multiple images
     */
    if (pdv_p->dd_p->force_single)
    {
        pdv_start_image(pdv_p);
        started = 1;
    }
    else
    {
        pdv_start_images(pdv_p, numbufs);
        started = numbufs;
    }

    for (i = 0; i < loops; i++)
    {
        Irig2Record *irp;	/* from pdv_irig.h */

        /*
         * get the image and immediately start the next one (if not the last
         * time through the loop). Processing (saving to a file in this case)
         * can then occur in parallel with the next acquisition
         */
        printf("image %d\r", i + 1);
        fflush(stdout);
        image_p = pdv_wait_image(pdv_p);

        if (edt_has_irigb(pdv_p) && (pdv_p->dd_p->header_type == HDR_TYPE_IRIG2))
        {
            irp = (Irig2Record *)(image_p + pdv_get_header_offset(pdv_p));

            printf ("magic %s framecnt %d\n", irp->magic == IRIG2_MAGIC? "OK ": "BAD", irp->framecnt);
        }
        else printf("Not pcie8 or header type not enabled in config\n");

        if ((overrun = (edt_reg_read(pdv_p, PDV_STAT) & PDV_OVERRUN)))
            ++overruns;

        if (i < loops - started)
        {
            pdv_start_image(pdv_p);
        }
        timeouts = pdv_timeouts(pdv_p);

        /*
         * check for timeouts or data overruns -- timeouts occur when data
         * is lost, camera isn't hooked up, etc, and application programs
         * should always check for them. data overruns usually occur as a
         * result of a timeout but should be checked for separately since
         * ROI can sometimes mask timeouts
         */
        if (timeouts > last_timeouts)
        {
            /*
             * pdv_timeout_cleanup helps recover gracefully after a timeout,
             * particularly if multiple buffers were prestarted
             */
            pdv_timeout_restart(pdv_p, TRUE);
            last_timeouts = timeouts;
            recovering_timeout = TRUE;
            printf("\ntimeout....\n");
        } else if (recovering_timeout)
        {
            pdv_timeout_restart(pdv_p, TRUE);
            recovering_timeout = FALSE;
            printf("\nrestarted....\n");
        }
        if (*bmpfname)
            save_image(image_p, width, height, depth, bmpfname, i);
    }
    puts("");

    printf("%d images %d timeouts %d overruns\n", loops, last_timeouts, overruns);

    /*
     * if we got timeouts it indicates there is a problem
     */
    if (last_timeouts)
        printf("check camera and connections\n");
    pdv_close(pdv_p);

    exit(0);
}

    static void
save_image(u_char * image_p, int s_width, int s_height, int s_depth, char *tmpname, int count)
{
    int     s_db = bits2bytes(s_depth);
    char    fname[256];

#ifdef _NT_
    u_char *bbuf = NULL;
    if ((strcmp(&tmpname[strlen(tmpname) - 4], ".bmp") == 0)
            || (strcmp(&tmpname[strlen(tmpname) - 4], ".BMP") == 0))
        tmpname[strlen(tmpname) - 4] = '\0';
    sprintf(fname, "%s%02d.bmp", tmpname, count);
#else
    if ((strcmp(&tmpname[strlen(tmpname) - 4], ".ras") == 0)
            || (strcmp(&tmpname[strlen(tmpname) - 4], ".RAS") == 0))
        tmpname[strlen(tmpname) - 4] = '\0';
    sprintf(fname, "%s%02d.ras", tmpname, count);
#endif

    /*
     * write bmp file on Windows systems, or Sun Raster on Unux/Linux
     * systems. Switch on number of bytes per pixel
     */
    switch (s_db)
    {
        case 1:
#ifdef _NT_

            dvu_write_bmp(fname, image_p, s_width, s_height);
#else
            printf("writing %dx%dx%d raster file to %s\n",
                    s_width, s_height, s_depth, fname);

            dvu_write_rasfile(fname, (u_char *) image_p, s_width, s_height);
#endif
            break;

        case 2:
            printf("converting %dx%dx%d image to 8 bits, writing to %s\n",
                    s_width, s_height, s_depth, fname);

#ifdef _NT_
            if (!bbuf)
                bbuf = (u_char *) pdv_alloc(s_width * s_height);

            if (bbuf == NULL)
            {
                pdv_perror("data buf malloc");
                exit(1);
            }
            dvu_word2byte((u_short *) image_p, (u_char *) bbuf,
                    s_width * s_height, s_depth);
            dvu_write_bmp(fname, bbuf, s_width, s_height);
#else
            dvu_write_rasfile16(fname, (u_char *) image_p, s_width, s_height, s_depth);
#endif
            break;

        case 3:
            printf("writing %dx%dx%d bmp file to %s\n",
                    s_width, s_height, s_depth, fname);

#ifdef _NT_
            dvu_write_bmp_24(fname, (u_char *) image_p, s_width, s_height);
#else
            dvu_write_rasfile24(fname, (u_char *) image_p, s_width, s_height);
#endif

            break;

        default:
            printf("invalid image depth for file write...!\n");
            break;
    }
}


    static void
usage(char *progname)
{
    puts("");
    printf("%s: simple example program that acquires images from an\n", progname);
    printf("EDT digital imaging interface board (PCI DV, PCI DVK, etc.)\n");
    puts("");
    printf("usage: %s [-b fname] [-l loops] [-N numbufs] [-u unit] [-c channel]\n", progname);
#ifdef _NT_
    printf("  -b fname 	      output to MS bitmap file\n");
#else
    printf("  -i fname        output to Sun Raster file\n");
#endif
    printf("  -l loops        number of loops (images to take)\n");
    printf("  -N numbufs      number of ring buffers (see users guide) (default 4)\n");
    printf("  -u unit         %s unit number (default 0)\n", EDT_INTERFACE);
    printf("  -c channel      %s channel number (default 0)\n", EDT_INTERFACE);
    printf("  -h              this help message\n");
}
