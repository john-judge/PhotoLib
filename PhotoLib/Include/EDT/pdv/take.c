/**
 * @file
 * A demo/test/diagnostic program that uses the EDT PCI DV (pdvlib) and
 * DMA library (edtlib) to acquire and save single or multiple images from
 * device connected to EDT PCI DV high speed interface.
 *
 * The utility has a large number of options for different ways of acquiring
 * and saving data, tuning the acquire in various ways, and testing and
 * diagnosing acquisition problems. Some of the methods used are quite
 * esoteric, some are obsolete for current hardware but remain for historic
 * purposes. As a result, \b take is primarily a test/diagnostic application
 * and while the code can be referred to for examples of various methods,
 * it is not really the best example code for user applications. For more
 * useful sample code, see simple_take.c, simplest_take.c and the users
 * guide.
 * 
 * (C) 1997-2013 Engineering Design Team, Inc.
 */

#include "edtinc.h"
#ifdef DO_JPEG
#include "edt_jpeg.h"
#endif


/*
 * defines
 */
#define STARTSIG 0xa5
#define ENDSIG   0x5a

    int
get_usps_lines_xferred(PdvDev *pdv_p, int buffer)
{
    u_int rasterlines;

    rasterlines = buffer % pdv_p->ring_buffer_numbufs;

    edt_ioctl(pdv_p, EDTG_LINES_XFERRED, &rasterlines);

    return rasterlines;

}

/*
 * fwd ref
 */
void
crop_and_subsample(int subsample, int src_width, int src_height,
        int x0, int y0, int x1, int y1, u_char * src, u_char * dst);
void
resolve_fname(char *fname, char *newname, int loops, char *ext);
static void usage(char *progname, char *errmsg, int exitval);

/*
 * globals
 */
int     ASnum = 0;
int     ASinc = 1;

    void
dumpbuf(char *name, u_short * buf, int size)
{
    FILE   *fp = fopen(name, "wb");

    fwrite(buf, sizeof(short), size / 2, fp);
    fclose(fp);
}

static int quiet = 0;

/*******************************
 *
 * simple method to suppress messages
 * using quiet variable
 *
 *******************************/

    int
take_printf(char *format,...)

{
    va_list stack;

    if (quiet)
        return 0;

    va_start(stack, format);

    return vprintf(format, stack);

}

/* write a windows bitmap file */
    int
write_bmp_file(char *tmpname, u_char * image_p,
        int s_width, int s_height, int s_depth)
{
    int     s_db = bits2bytes(s_depth);
    u_char *bbuf = NULL;
    int retval;


    /*
     * write bmp file. Switch on number of bytes per pixel
     */
    switch (s_db)
    {
        case 1:
            take_printf("writing %dx%dx%d bmp file to %s\n",
                    s_width, s_height, s_depth, tmpname);

            retval = dvu_write_bmp(tmpname, image_p, s_width, s_height);
            break;

        case 2:
            take_printf("converting %dx%dx%d image to 8 bits, writing to %s\n",
                    s_width, s_height, s_depth, tmpname);

            if (!bbuf)
                bbuf = (u_char *) pdv_alloc(s_width * s_height);

            if (bbuf == NULL)
            {
                pdv_perror("data buf malloc");
                exit(1);
            }
            dvu_word2byte((u_short *) image_p, (u_char *) bbuf,
                    s_width * s_height, s_depth);
            retval = dvu_write_bmp(tmpname, bbuf, s_width, s_height);
            break;

        case 3:
            take_printf("writing %dx%dx%d bmp file to %s\n",
                    s_width, s_height, s_depth, tmpname);

            retval = dvu_write_bmp_24(tmpname, (u_char *) image_p, s_width, s_height);
            break;

        default:
            take_printf("invalid image depth for file write...!\n");
            return -1;
            break;
    }

    return retval;
}

/* keep this until we find out why without it, process cannot exit */
/* even though wakeup from WAITBUF detects interrupt and does wakeup */

int GetOut = 0 ;
#ifdef __APPLE__
    static int
gotit()
{
    GetOut = 1 ;
    return 0;
}
#endif

char cmdstr[80] ;

/*
 * Main module. NO_MAIN is typically only defined when compiling for vxworks; if you
 * want to use this code outside of a main module in any other OS, just copy the code
 * and modify it to work as a standalone subroutine, including adding parameters in
 * place of the command line arguments
 */
#ifdef NO_MAIN
#include "opt_util.h"
char   *argument;
int     option;

    int
take(char *command_line)
#else
    int
main(int argc, char *argv[])
#endif
{
    int     i, lc;
    int     waited = 0;
    int     unit = 0;
    char   *unitstr = "0";
    int     loops = 1;
    int     wait_for_enter = 0;
    int     user_delay = 0;
    int     numbufs = 4;
    int     do_ring = 1;
    int     verbose = 0;
    int     do_file = 0;
    int     rbtimeout = 0;
    int     bytesread;
    int     started;
    int     done;
    char    cfgfname[128];
    char    bmpfname[128];
    char    jpgfname[128];
    char    rawfname[128];
    char    edt_devname[256];
    char   *progname = "take";
    u_char *imagebuf = NULL;
    u_char *subimage = NULL;
    u_char *image_p = NULL;
    PdvDev *pdv_p;
    int     timeouts=0, last_timeouts=0;
    int     in_timeout_cleanup = 0;
    int     overruns=0, overrun;
    char    tmpname[128];
    int     subsample = 0, crop = 0;
    int     debug_timeout = 0;
    int     width, height, depth, imagesize, dmasize, bufsize, db;
    int     s_width, s_height, s_depth, s_size, s_db;
    int     crop_x0, crop_x1, crop_y0, crop_y1;
    int     exposure, gain, offset;
    int     set_exposure = 0, set_gain = 0, set_offset = 0;
    int     set_timeout = 0;
    int     do_timestamp = 0;
    int     timetype = 0;
    int     do_lasttest = 0;
    int     timeout_val = 0;
    int     enable_burst_en = 1;
    int     vskip, vactv, hskip, hactv;
    int     set_roi = 0;
    int     unset_roi = 0;
    int     checksig = 0, checkcount = 0;
    int     checksim = 0;
    int     checkclsim = 0;
    int     checkclk = 0;
    int     badstart = 0, badend = 0;
    int     just_print_the_last_one = 0;
    u_short startsig = STARTSIG, endsig = ENDSIG;
    u_int   sigcount;
    int     siginit = 0;
    char    errstr[64];
    u_int   timestamp[2];
    u_int   freqbuf[2];
    double  dtime;
    double  svtime = 0.0;
    double  curtime = 0.0;
    double  sumsize = 0.0;
    int     checkfrm = 0;
    int     binning = 0;
    int     opto_trigger = 0;
    int     fldid_trigger = 0;

    int     channel = 0;


    int     mosaic = 0;
    int     mosaic_sample = 0;
    int     sample_height = 10;
    int     mosaic_size = 0;
    int     mosaic_width = 0;
    int     mosaic_height = 0;
    char    dvcmode[8];
    unsigned char *mosaic_ptr = NULL;
    unsigned char *mosaic_image = NULL;
    int     arg_channel = -1;
    int     dl;

    int noreset = 0;

#ifdef NO_MAIN
    char  **argv = 0;
    int     argc = 0;

    opt_create_argv("take", command_line, &argc, &argv);
#endif


    cfgfname[0] = '\0';
    rawfname[0] = '\0';
    bmpfname[0] = '\0';
    jpgfname[0] = '\0';
    edt_devname[0] = '\0';
    dvcmode[0] = '\0';
    tmpname[0] = '\0';
    ASnum = 0;
    quiet = 0 ;
    siginit = 0 ;

#ifdef __APPLE__
    /* debug for interrupted wakeup */
    signal(SIGINT,gotit);
#endif

    --argc;
    ++argv;
    while (argc && ((argv[0][0] == '-') || (argv[0][0] == '/')))
    {
        switch (argv[0][1])
        {
            case 'u':		/* device unit number */
                ++argv;
                --argc;
                if (argc < 1) 
                {
                    usage(progname, "Error: option 'u' requires a numeric argument\n", 1);
                }
                if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
                {
                    unitstr = argv[0];
                }
                else 
                {
                    usage(progname, "Error: option 'u' requires a numeric argument\n", 1);
                }
                break;

            case 'B':		/* turn off burst enable */
                ++argv;
                --argc;
                if (argc > 0)
                    enable_burst_en = atoi(argv[0]);
                else
                    enable_burst_en = 1;
                break;

            case 'v':		/* verbose */
                verbose = 1;
                break;

            case 'w':		/* wait for user <cr> between frames */
                wait_for_enter = 1;
                break;

            case 'T':		/* on timeout, dump data to timeout.raw */
                debug_timeout = 1;
                break;

            case 'd':		/* timestamp */
                do_timestamp = 1;
                break;

            case 'D':		/* dvc mode setting (diagnostic) */
                ++argv;
                --argc;
                strcpy(dvcmode, argv[0]);
                break;

            case 'y':
                opto_trigger = 1;
                break;

            case 'Y':
                fldid_trigger = 1;
                break;

            case 'J':
                just_print_the_last_one = 1;
                break;

            case 'L':
                do_lasttest = 1;	/* freerun/bytecount with wait for next frame */
                break;

            case 'm':		/* microsec sleep between frames */
                ++argv;
                --argc;
                if (argc < 1) 
                {
                    usage(progname, "Error: option 'm' requires a numeric argument\n", 1);
                }
                if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
                {
                    user_delay = atoi(argv[0]);
                }
                else 
                {
                    usage(progname, "Error: option 'm' requires a numeric argument\n", 1);
                }
                break;

            case 'N':		/* specify numbufs */
                ++argv;
                --argc;
                if (argc < 1) 
                {
                    usage(progname, "Error: option 'N' requires a numeric argument\n", 1);
                }
                if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
                {
                    numbufs = atoi(argv[0]);
                    if (!numbufs)
                        do_ring = 0; /* if 0 just use pdv_read not ring buffers */
                }
                else 
                {
                    usage(progname, "Error: option 'N' requires a numeric argument\n", 1);
                }
                break;

            case 'H':		/* check count signature from testgen */
                checksim = 1;
                checksig = 0;
                checkcount = 0;
                break;

            case 'P':		/* check count signature from PCI CD sim */
                checkcount = 1;
                checksig = 0;
                break;

            case 'p':		/* check fixed signature from PCI CD sim */
                checksig = 1;
                checkcount = 0;
                break;

            case 'G':		/* check count signature from testgen */
                checkclsim = 1;
                checksim = 0;
                checksig = 0;
                checkcount = 0;
                break;

            case 'q':		/* binning */
                ++argv;
                --argc;
                binning = atoi(argv[0]);
                break;

            case 'Q':		/* turn on quiet mode */
                quiet = 1;
                break;

            case 't':		/* microsec sleep between frames */
                /* if no subarg, set to -1 (auto timeout) */
                if ((argc < 2) || (argv[1][0] == '-'))
                {
                    set_timeout = 1;
                    timeout_val = -1;
                }
                else
                {
                    ++argv;
                    --argc;
                    set_timeout = 1;
                    if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
                        timeout_val = atoi(argv[0]);
                }
                break;

            case 'l':		/* loops */
                ++argv;
                --argc;
                if (argc < 1) 
                {
                    usage(progname, "Error: option 'l' requires a numeric argument\n", 1);
                }
                if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
                {
                    loops = atoi(argv[0]);
                }
                else 
                {
                    usage(progname, "Error: option 'l' requires a numeric argument\n", 1);
                }
                break;

            case 'f':		/* save filename */
                ++argv;
                --argc;
                strcpy(rawfname, argv[0]);
                do_file = 1;
                break;

            case 'S':		/* subsample */
                ++argv;
                --argc;
                subsample = atoi(argv[0]);
                break;

            case 'C':		/* crop x0, y0, x1, y1 */
                crop = 1;
                ++argv;
                --argc;
                crop_x0 = atoi(argv[0]);

                ++argv;
                --argc;
                crop_y0 = atoi(argv[0]);

                ++argv;
                --argc;
                crop_x1 = atoi(argv[0]);

                ++argv;
                --argc;
                crop_y1 = atoi(argv[0]);
                break;

            case 'n':		/* autosequence start only */
                ++argv;
                --argc;
                ASnum = atoi(argv[0]);
                break;

            case 's':		/* autosequence start and inc */
                ++argv;
                --argc;
                ASnum = atoi(argv[0]);
                break;

            case 'e':		/* exposure time */
                ++argv;
                --argc;
                set_exposure = 1;
                exposure = atoi(argv[0]);
                break;

            case 'g':		/* gain */
                ++argv;
                --argc;
                set_gain = 1;
                gain = atoi(argv[0]);
                break;

            case 'o':		/* offset (blacklevel) */
                ++argv;
                --argc;
                set_offset = 1;
                offset = atoi(argv[0]);
                break;

            case 'I':		/* # to increment for multiple file writes */
                ++argv;
                --argc;
                ASinc = atoi(argv[0]);
                break;

#ifdef DO_JPEG
            case 'j':
                ++argv;
                --argc;
                strcpy(jpgfname, argv[0]);
                break;
#endif

            case 'b':		/* bitmap save filename */
            case 'i':
                ++argv;
                --argc;
                strcpy(bmpfname, argv[0]);
                do_file = 1;
                break;

            case 'c':		/* channel */
                ++argv;
                --argc;
                if (argc < 1) 
                {
                    usage(progname, "Error: option 'c' requires a numeric argument\n", 1);
                }
                if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
                {
                    arg_channel = atoi(argv[0]);
                }
                else 
                {
                    usage(progname, "Error: option 'c' requires a numeric argument\n", 1);
                }
                break;

            case 'r':		/* ROI */
                if (argc < 5)
                {
                    usage(progname, "Error: option 'r' requires subsequent arguments", 1);
                    exit(1);
                }
                ++argv;
                --argc;
                hskip = atoi(argv[0]);
                ++argv;
                --argc;
                hactv = atoi(argv[0]);
                ++argv;
                --argc;
                vskip = atoi(argv[0]);
                ++argv;
                --argc;
                vactv = atoi(argv[0]);
                set_roi = 1;
                break;

            case 'R':		/* disable ROI */
                unset_roi = 1;
                break;

            case 'O':		/* try noreset - will be obsolete on clink? */
                noreset = 1 ;
                break;

            case 'M':		/* mosaic */
                mosaic = 1;
                break;

            case 'a':		/* call pdv_checkfrm */
                checkfrm = 1;
                break;
            case 'A':
                checkclk = 1;
                break;

            default:
                fprintf(stderr, "unknown flag -'%c'\n", argv[0][1]);
            case '?':
            case 'h':
                usage(progname, "", 0);
                exit(0);
        }
        argc--;
        argv++;
    }


    unit = edt_parse_unit_channel(unitstr, edt_devname, "pdv", &channel);

    if (arg_channel != -1)
    {
        channel = arg_channel;
    }

    unit = edt_parse_unit(unitstr, edt_devname, "pdv");

    /*
     * pdv_open_channel is just pdv_open with an extra argument, the channel,
     * which is normally 0 unless you're running multiple cameras (2nd base
     * mode on a PCI DV C-Link or dasy-chained RCI boxes off a single PCI
     * FOI)
     */
    if ((pdv_p = pdv_open_channel(edt_devname, unit, channel)) == NULL)
    {
        sprintf(errstr, "pdv_open(%s%d_%d)", edt_devname, unit, channel);
        pdv_perror(errstr);
        return (1);
    }

    if (pdv_is_simulator(pdv_p))
    {
        fprintf(stderr, "%s unit %d is a simulator board; take is only for use with framegrbbers\n", edt_devname, unit);
        pdv_close(pdv_p);
        exit(1);
    }

    pdv_flush_fifo(pdv_p);

    if (*dvcmode)
        pdv_set_mode_dvc(pdv_p, dvcmode);

    if (set_exposure)
        pdv_set_exposure(pdv_p, exposure);

    if (set_gain)
        pdv_set_gain(pdv_p, gain);

    if (set_offset)
        pdv_set_blacklevel(pdv_p, offset);

    if (set_timeout)
        pdv_set_timeout(pdv_p, timeout_val);

    if (opto_trigger)
        pdv_enable_external_trigger(pdv_p, PDV_PHOTO_TRIGGER);

    else if (fldid_trigger)
        pdv_enable_external_trigger(pdv_p, PDV_FLDID_TRIGGER);


    if (set_roi)
    {
        pdv_set_roi(pdv_p, hskip, hactv, vskip, vactv);
        pdv_enable_roi(pdv_p, 1);
    }
    else if (unset_roi)
    {
        pdv_enable_roi(pdv_p, 0);
    }

    if (binning)
    {
        pdv_set_binning(pdv_p, binning, binning);
    }

    if (checksig)
        take_printf("checking PCI CD Simulator fixed signature -- use simple_putframe -p\n");
    if (checksim)
        take_printf("checking testgen signature\n");
    if (checkclsim)
        take_printf("checking cl simulator signature\n");
    if (checkcount)
        take_printf("checking PCI CD Simulator count signature -- use simple_putframe -P\n");

    /*
     * set size variables for image and initial subimage
     */
    width = s_width = pdv_get_width(pdv_p);
    height = s_height = pdv_get_height(pdv_p);
    depth = s_depth = pdv_get_depth(pdv_p);
    db = s_db = bits2bytes(depth);
    imagesize = s_size = pdv_get_imagesize(pdv_p);
    dmasize = s_size = pdv_get_dmasize(pdv_p);

    subimage = NULL;
    /*
     * turn off merge
     */
    edt_set_merge(pdv_p, 0, 0, 0, 0);

    if (s_size <= 0)
    {
        printf("%s%d: Invalid image size. Make sure device has been initialized.\n", edt_devname, unit);
        exit(1);
    }

    if (mosaic)
    {
        mosaic_sample = width * sample_height * db;

        mosaic_size = loops * mosaic_sample;

        mosaic_image = pdv_alloc(mosaic_size);

        mosaic_ptr = mosaic_image;

        mosaic_width = width * db;

        mosaic_height = loops * sample_height;

    }

    if (do_ring)
    {
        /* Construct a ring buffer */
        if (pdv_multibuf(pdv_p, numbufs) == -1)
        {
            char    msg[64];

            sprintf(msg, "pdv_multibuf(0x%p, %d)", pdv_p, numbufs);
            pdv_perror(msg);
            return (0);
        }
    }
    else
    {
        bufsize = ((dmasize > imagesize)? dmasize: imagesize);
        imagebuf = (u_char *) pdv_alloc(bufsize);
        if (imagebuf == NULL)
        {
            pdv_perror("data buf malloc");
            exit(1);
        }
    }

    pdv_cl_reset_fv_counter(pdv_p) ;

    /*
     * check for crazy subsample and crop values and sort out the params as
     * needed
     */

    if (do_file && (subsample || crop))
    {
        if ((subsample < 0)
                || (subsample > height / 4)
                || (subsample > width / 4))
        {
            printf("-S <subsample> arg out of range\n");
            usage(progname, "", 1);
            exit(1);
        }

        /* force subsample since its used as an arg in processing */
        if (crop && (subsample == 0))
            subsample = 1;

        if (crop)
        {
            int     tmp;

            if (crop_x0 > crop_x1)
            {
                tmp = crop_x1;
                crop_x1 = crop_x0;
                crop_x0 = tmp;
            }
            if (crop_y0 > crop_y1)
            {
                tmp = crop_y1;
                crop_y1 = crop_y0;
                crop_y0 = tmp;
            }

            if (crop_x0 < 0)
                crop_x0 = 0;
            if (crop_y0 < 0)
                crop_y0 = 0;
            if (crop_x1 > width - 1)
                crop_x1 = width - 1;
            if (crop_y1 > height - 1)
                crop_y1 = height - 1;
        }
        else
        {
            crop_x0 = 0;
            crop_y0 = 0;
            crop_x1 = width - 1;
            crop_y1 = height - 1;
        }

        s_width = ((crop_x1 - crop_x0) + subsample) / subsample;
        s_height = ((crop_y1 - crop_y0) + subsample) / subsample;
        s_size = s_width * s_height * s_db;
        subimage = (u_char *) malloc(s_size);
    }

    /*
     * should normally only be set from the config file as "start_delay:",
     * but for testing allow it to be set here too
     */
    if (user_delay)
        pdv_p->dd_p->start_delay = user_delay;

    if (pdv_p->dd_p->start_delay)
        take_printf("%d millisecond delay added between images\n", pdv_p->dd_p->start_delay);
    if (pdv_p->dd_p->force_single)
        take_printf("force_single set; prestarting one image at a time\n");

#ifdef WIN2K
    edt_get_kernel_event(pdv_p, EDT_EVENT_BUF);
#endif

    /*
     * acquire the image
     */
    {
        char header_size[32];
        if (pdv_get_header_size(pdv_p))
            sprintf(header_size,"hdr %d", pdv_get_header_size(pdv_p));
        else
            header_size[0] = 0;

        if (dmasize != imagesize)
            take_printf("reading image from %s\nwidth %d height %d depth %d %s total bytes %d (DMA %d)\n",
                    pdv_get_cameratype(pdv_p),
                    pdv_get_width(pdv_p),
                    pdv_get_height(pdv_p),
                    pdv_get_depth(pdv_p),
                    header_size,
                    imagesize,
                    dmasize);
        else
            take_printf("reading image from %s\nwidth %d height %d depth %d %s total bytes %d\n",
                    pdv_get_cameratype(pdv_p),
                    pdv_get_width(pdv_p),
                    pdv_get_height(pdv_p),
                    pdv_get_depth(pdv_p),
                    header_size,
                    imagesize);
    }

    if (do_ring)
    {
        if (pdv_p->dd_p->force_single)
        {
            pdv_start_image(pdv_p);
            started = 1;
        }
        else if (do_lasttest)
        {
            /* start freerun */
            pdv_start_images(pdv_p, 0);
        }
        else if (wait_for_enter)
        {
            printf("hit enter for first image");
            getchar();
            pdv_start_images(pdv_p, 1);
            started = 1;
        }
        else
        {
            pdv_start_images(pdv_p, numbufs);
            started = numbufs;
        }
    }
    if (do_lasttest)
    {
        for (;;)
        {
            int     ret;
            u_char  tmpbuf[80];

#if 0
            printf("return to check");
            getchar();
#endif
            take_printf("%d done count %x\n", edt_done_count(pdv_p), edt_get_bytecount(pdv_p));
            edt_send_msg(pdv_p, 0, "w a07a 6", 8);
            ret = edt_serial_wait(pdv_p, 1000, 0);
            if (ret > 0)
            {
                take_printf("got %d resp\n", ret);
                edt_get_msg(pdv_p, (char *) tmpbuf, sizeof(tmpbuf));
                take_printf("resp %x %x %x\n", tmpbuf[0], tmpbuf[1], tmpbuf[2]);
                if (tmpbuf[2])
                {
                    printf("got %s - return to continue", &tmpbuf[2]);
                    getchar();
                }
            }
            else
            {
                printf("bad resp from serial wait %d\n", ret);
                getchar();
            }
            edt_send_msg(pdv_p, 0, "w a07a 4", 8);
            ret = edt_serial_wait(pdv_p, 1000, 0);
            if (ret > 0)
            {
                printf("got %d resp\n", ret);
                edt_get_msg(pdv_p, (char *) tmpbuf, sizeof(tmpbuf));
                printf("resp %x %x %x\n", tmpbuf[0], tmpbuf[1], tmpbuf[2]);
                if (tmpbuf[2])
                {
                    printf("got %s - return to continue", &tmpbuf[2]);
                    getchar();
                }
            }
            else
            {
                printf("bad resp from serial wait %d\n", ret);
                getchar();
            }
        }
    }
    done = 0;
    (void) edt_dtime();		/* init time for check */
    for (lc = 0; lc < loops && !GetOut; lc++)
    {
        if (wait_for_enter)
        {
            printf("hit enter for next image");
            getchar();
        }

        if (do_ring)
        {
            if (do_timestamp)
            {
                imagebuf = pdv_wait_image_timed(pdv_p, timestamp);
                curtime = pdv_decode_timestamp(pdv_p, timestamp);

                take_printf("\ntime %3.9f ", curtime);
                if (svtime > 0)
                {
                    take_printf(" delta %3.9f ", (curtime - svtime));
                }
                svtime = curtime;
            }
            else
                imagebuf = pdv_wait_image(pdv_p);
            ++waited;
        }
        else
        {
            if (wait_for_enter && done == 0)
            {
                printf("hit enter for first image");
                getchar();
            } 

            bytesread = pdv_read(pdv_p, imagebuf, dmasize);

            if (bytesread != dmasize)
            {
                printf("dmasize 0x%x read 0x%x\n", dmasize, bytesread);
            }
            if (bytesread < 0)
            {
                pdv_perror(edt_devname);
                exit(1);
            }
        }
        if (checkfrm)
            pdv_checkfrm(pdv_p, (u_short *) imagebuf, width * height, verbose);
        timeouts = pdv_timeouts(pdv_p);
        if ((overrun = (edt_reg_read(pdv_p, PDV_STAT) & PDV_OVERRUN)))
        {
            ++overruns;
        }

        if (timeouts > last_timeouts)
        {
            bytesread = edt_get_timeout_count(pdv_p);
            if (debug_timeout)
            {
                FILE   *tmpf;

                tmpf = fopen("timeout.raw", "wb");
                if (tmpf)
                {
                    printf("saving timeout data\n");
                    fwrite(imagebuf, imagesize, 1, tmpf);
                    fclose(tmpf);
                }
                printf("got timeout xfer %d - return to continue: ",
                        edt_get_bytecount(pdv_p));
                getchar();
            }

            /*
             * pdv_timeout_cleanup recovers from the timeout, in particular
             * when you've prestarted multiple buffers returns how many
             * buffers were pending before the reset so we can restart that
             * many again
             */
            if (do_ring)
            {
                pdv_timeout_restart(pdv_p,TRUE);
                in_timeout_cleanup = TRUE;
            }

            last_timeouts = timeouts;
            rbtimeout = 1;

        }
        else if (in_timeout_cleanup)
        {
            pdv_timeout_restart(pdv_p,TRUE);
            in_timeout_cleanup = FALSE;
        }

        done++;

        if (verbose)
        {
            int     j;
            u_char *tmp_p;
            int     lastval = 0;

            tmp_p = imagebuf;

            for (j = 0; j < 16; j++)
                take_printf("%02x ", imagebuf[j]);
            take_printf("\n");
            for (j = imagesize - 16; j < imagesize; j++)
                take_printf("%02x ", imagebuf[j]);
            take_printf("\n");
            if (checkclk)
            {
                for (j = 0; j < 512; j++)
                {
                    static int k = 0;
                    u_short tmpval = tmp_p[1] << 8 | tmp_p[0];

                    if (tmpval != lastval + 1)
                        printf("*");
                    else
                        printf(" ");
                    lastval = tmpval;

                    printf("%04x", tmpval);

                    if ((++k % 16) == 0)
                        printf("\n");
                    tmp_p += 4;
                }
                printf("\n");
            }
        }
        /*
         * check signature pixels from PCI CD simulator if -p or -P flag
         * specified -- expects first and last pixel to be count if -p, or
         * 0xA5 and 0x5A,  if -P, bombs out on first frame where this is not
         * the case
         */
        if (checksig || checkcount || checksim || checkclsim)
        {
            u_short bitmask = 0;

            for (i = 0; i < s_depth; i++)
                bitmask += 1 << i;

            /*
             * signature counter -- pick up the first one we come to
             */
            if (checkcount)
            {
                if (!siginit)
                {
                    startsig = endsig = sigcount = ((u_short *) imagebuf)[0] & bitmask;
                    siginit = 1;
                }
                else
                    startsig = endsig = sigcount & bitmask;
                ++sigcount;
            }
            if (checksim)
            {
                if (!siginit)
                {
                    u_short *sp = (u_short *) imagebuf;

                    startsig = sp[0];
                    endsig = sp[(imagesize / 2) - 1];
                    siginit = 1;
                }
                else
                {
                    startsig ^= 0xffff;
                    endsig ^= 0xffff;
                }
            }
            if (checkclsim)
            {
                if (!siginit)
                {
                    u_short *sp = (u_short *) imagebuf;

                    startsig = sp[0];
                    /* find 3333 */
                    if (sp[1] == 0x3333)
                        printf("init start  to %04x\n", startsig) ;
                    else
                        printf("buf is already not aligned\n") ;

                    siginit = 1;
                }
                else
                {
                    if (startsig == 0x7fff) startsig = 0 ;
                    else startsig = startsig + 1 ;
                }
            }


            if (depth > 8 || checkclsim)
            {
                u_short *sp = (u_short *) imagebuf;
                u_short firstword = sp[0];

                /*
                 * -4 because of one test xilinx which went into grid mode
                 * when fifo empty
                 */
                u_short lastword = sp[(imagesize / 2) - 4];

                if (checkclsim)
                {
                    u_int frmcnt = pdv_cl_get_fv_counter(pdv_p) ;
                    int k ;

                    if (firstword != startsig)
                        badstart = 1;
                    k = 0 ;
                    /* deal with 3333rd image */
                    if (sp[0] == 0x3333 && sp[1] == 0x3333) k = 1 ;
                    for (; k < imagesize/2 ; k++)
                    {
                        if (sp[k] == 0x3333)
                        {
                            if (k != 1)
                            {
                                printf("image1 3333 at %x\n",k) ;
                                badend = 1 ;
                            }
                            printf("frm %x count %04x stat %04x reg %08x\n",
                                    frmcnt,
                                    sp[k-1],
                                    edt_reg_read(pdv_p,PDV_STAT),
                                    edt_reg_read(pdv_p,PDV_CL_JERRYDBG)) ;
                            break ;
                        }
                    }
                }
                else
                {
                    if (firstword != startsig)
                        badstart = 1;
                    if (lastword != endsig)
                        badend = 1;
                }

                if (badstart || badend)
                {
                    u_char **buf = edt_buffer_addresses(pdv_p);

                    printf("\n\nstat %04x reg %08x\n",
                            edt_reg_read(pdv_p,PDV_STAT),
                            edt_reg_read(pdv_p,PDV_CL_JERRYDBG)) ;

                    /*
                     * if we just missed a frame, report it and go on
                     */
                    if (checksim
                            && (firstword == (startsig ^ 0xffff))
                            && (lastword == (endsig ^ 0xffff)))
                    {
                        printf("\nMISSED a frame! start %04x s/b %04x, end %04x s/b %04x\n", firstword, startsig, lastword, endsig);
                        printf("buffers %d donecount %d PDV_STAT %02x\n",
                                waited, edt_done_count(pdv_p), edt_reg_read(pdv_p, PDV_STAT));
                        startsig ^= 0xffff;
                        endsig ^= 0xffff;
                    }
                    else
                    {
                        printf("\nbad signature! start %04x s/b %04x, end %04x s/b %04x\n",
                                firstword, startsig, lastword, endsig);
                        printf("bad buf addr at %p %04x %04x\n", sp, firstword, lastword);
                        printf("buffers %d donecount %d PDV_STAT %02x\n",
                                waited, edt_done_count(pdv_p), edt_reg_read(pdv_p, PDV_STAT));

                        edt_stop_buffers(pdv_p);

                        sprintf(cmdstr,"setdebug -d 0 -u %d -g >setdebug.out\n",unit);
                        printf("%s\n",cmdstr) ;
                        system(cmdstr) ;


                        printf("dumping %d buffers to buf01.raw, buf02.raw, ....\n",
                                numbufs);
                        for (i = 0; i < numbufs; i++)
                        {
                            char    bufname[32];

                            sprintf(bufname, "buf%02d.raw", i);
                            printf("dump buf %d from %p\n", i, buf[i]);
                            dumpbuf(bufname, (u_short *) buf[i], imagesize);
                        }

                        printf("return to continue");
                        getchar();
                        if (!(checkcount||checkclsim))
                        {
                            for (i = 0; i < imagesize / 2; i++)
                                if (((sp[i] & bitmask) == startsig)
                                        || (sp[i] & bitmask) == endsig)
                                    printf("found %04x at %d\n", sp[i], i);
                        }
                        exit(1);
                    }
                }
                /*
                 * sp[0] = 0xaaaa ; sp[imagesize/2 - 1] = 0x5555 ;
                 */
            }
            else
            {
                u_char *cp = imagebuf;

                if (cp[0] != startsig)
                    badstart = 1;
                if (cp[imagesize - 1] != endsig)
                    badend = 1;
                if (badstart || badend)
                {
                    printf("\nbad signature! start %02x s/b %02x, end %02x s/b %02x\n",
                            cp[0], startsig, cp[imagesize-1], endsig);

                    if (!checkcount)
                    {
                        for (i = 0; i < imagesize; i++)
                            if ((cp[i] == startsig)
                                    || (cp[i] == endsig))
                                printf("found %02x at %d\n", cp[i], i);
                    }
                    exit(1);
                }
            }
        }
        if (do_ring && started < loops)
        {
            if (!do_lasttest)
            {
                pdv_start_images(pdv_p, 1);
            }
            started++;
        }

        /*
         * process subimage if needed, set up args for write
         */
        if (do_file && (subimage != NULL))
        {
            crop_and_subsample(subsample, width, height,
                    crop_x0, crop_y0, crop_x1, crop_y1, imagebuf, subimage);
            image_p = subimage;
        }
        else
            image_p = imagebuf;


        if (pdv_p->dd_p->fval_done)
        {	
            int lines = pdv_get_lines_xferred(pdv_p);
            s_size = s_width * lines * s_db;
        }

        /*
         * write the image to raw data file (no formatting)
         */
        sumsize += s_size;

        if (*rawfname)
        {
            resolve_fname(rawfname, tmpname, loops, ".raw");
            take_printf("writing %dx%dx%d raw file to %s (actual size %d)\n",
                    s_width, s_height, s_depth, tmpname, s_size);

            (void) dvu_write_raw(s_size, image_p, tmpname);
        }

#if DO_JPEG
        if (*jpgfname)
        {
            char filename[256];

            if ((strcmp(&jpgfname[strlen(jpgfname) - 4], ".jpg") == 0)
            || (strcmp(&jpgfname[strlen(jpgfname) - 4], ".JPG") == 0))
                jpgfname[strlen(jpgfname)] = '\0';
                sprintf(filename, "%s%02d.jpg", jpgfname, i);

            edt_save_jpeg(filename, width, height, bits2bytes(depth), 0, image_p);
        }
#endif

        /*
         * write the image to Windows bitmap format file and
         * fix up the filename extension
         */
        if (*bmpfname && !mosaic)
        {
            resolve_fname(bmpfname, tmpname, loops, ".bmp");
            if (write_bmp_file(tmpname, image_p,
                    s_width, s_height, s_depth) != 0)
                        printf("file write failed\n");
        }
        else if (mosaic)
        {
            memcpy(mosaic_ptr, image_p, mosaic_sample);
            mosaic_ptr += mosaic_sample;
        }

        dl = pdv_debug_level();

        if (rbtimeout)
        {
            if (bytesread == 0)
                take_printf("%d images %d timeouts %d overruns 0 bytes", done, timeouts, overruns);
            else take_printf("%d images %d timeouts %d overruns %d bytes (%d short)", done, timeouts, overruns, bytesread, dmasize-bytesread);
        }
        else if (just_print_the_last_one)
        {
            if (lc == loops-1)
                take_printf("%d images %d timeouts %d overruns\n", done, timeouts, overruns);
        }
        else take_printf("%d images %d timeouts %d overruns", done, timeouts, overruns);

        if (!just_print_the_last_one)
        {
            if (verbose | rbtimeout | wait_for_enter | (pdv_debug_level() > 0))
                take_printf("\n");
            else take_printf("                                \r");
        }

        rbtimeout = 0;
        fflush(stdout);
    }
    take_printf("\n");

    if (mosaic && *bmpfname)
    {
        if (write_bmp_file(bmpfname, mosaic_image,
                s_width, mosaic_height, s_depth) != 0)
                        printf("file write failed\n");
    }
   
    if (loops > 3)
    {
        dtime = edt_dtime();
        printf("%lf bytes/sec\n", sumsize / dtime);
        printf("%lf frames/sec\n", (double) (loops) / dtime);
    }

    if (!do_ring)
        pdv_free(imagebuf);
    /* else */
    pdv_stop_continuous(pdv_p);

    if (binning)
    {
        pdv_set_binning(pdv_p, 1, 1);
    }

    pdv_close(pdv_p);
#ifdef NO_MAIN
    if (overruns || timeouts)
        return (2);
    return (0);
#else
    if (overruns || timeouts)
        exit(2);
    exit(0);
#endif
}

    void
resolve_fname(char *fname, char *newname, int loops, char *ext)
{
    int     l = (int)strlen(fname);
    char    tmpname[256];
    char    tmpext[5];

    strcpy(tmpname, fname);
    if ((l > 4) && (tmpname[l - 4] == '.'))
    {
        strcpy(tmpext, &tmpname[l - 4]);
        tmpname[l - 4] = '\0';
    }
    else
        strcpy(tmpext, ext);

    if (loops > 1)
    {
        sprintf(newname, "%s%04d%s", tmpname, ASnum, tmpext);
        ASnum += ASinc;
    }
    else
        sprintf(newname, "%s%s", tmpname, tmpext);
}


/*
 * process the image, return in subimage. Handles both subsampling and
 * cropping.
 * 
 * NOTE: this only works on single-byte images (so far)
 */

    void
crop_and_subsample(int subsample, int src_width, int src_height,
        int x0, int y0, int x1, int y1, u_char * src, u_char * dst)
{
    u_char *next_fromptr = src + (y0 * src_width) + x0;
    int     to_width = (x1 - x0) + 1;
    int     lines = (y1 - y0) + 1;
    u_char *to = dst;
    u_char *from;
    u_char *from_end;

    while (lines > 0)
    {
        from = (u_char *) next_fromptr;
        from_end = from + to_width;

        while (from < from_end)
        {
            *to++ = *from;
            from += subsample;
        }
        next_fromptr += src_width * subsample;
        lines -= subsample;
    }
}


    void
usage(char *progname, char *errmsg, int exitval)
{
    printf("\n");
    printf("%s", errmsg);
    printf("\n%s: Image acquisition and optional save for EDT PCI DV family digital\n", progname);
    printf("DMA interface products. Many options make it useful for diagnostics and\n");
    printf("also as an example for various methods of doing acquisition with EDT PCI DV\n");
    printf("family products. For a much a simpler example, see simple_take.c\n\n");
    printf("usage: %s\n", progname);
    printf("  -e exposuretime   - set exposure time before acquiring (if supported)\n");
    printf("  -g gain           - set camera gain (if supported)\n");
    printf("  -o offset         - set camera offset (if supported)\n");
    printf("  -l loopcount      - loop for loopcount images\n");
    printf("  -m msecs          - delay msecs milliseconds between images\n");
    printf("  -f out_fname      - output to raw file(s)\n");
    printf("  -b fname.bmp      - output to Microsoft bitmap file(s)\n");
#ifdef DO_JPEG
    printf("  -j fname.jpg      - output to jpeg format file(s)\n");
#endif
    printf("  -C x0 y0 x1 y1    - crop the output image to the coordinates given\n");
    printf("  -N n              - number of multiple buffers (default 4, 0 forces pdv_read()\n");
    printf("  -r [hs ha vs va]  - region of interest -- to disable use -r alone (no subargs)\n");
    printf("  -t [msecs]        - timeout # of msecs. if 0, disables timeouts, if no subarg,\n");
    printf("  -T                - if image times out, dump any recieved data to timeout.raw\n");
    printf("                        re-enables auto timeouts\n");
    printf("  -w                - wait for <CR> between images\n");
    printf("  -s start inc      - specify start and increment for auto-numbered file-\n");
    printf("                        names (use with -l of 2 or more for auto-numbered filenames)\n");
    printf("  -I inc            - specify increment for auto-numbered file (see -s)\n");
    printf("  -v                - verbose output\n");
    printf("  -q binval         - set binning (applicable cameras only)\n");
    printf("  -p                - check pci dv simulator signature, exit if bad\n");
    printf("  -d                - get and print a driver timestamp with every image\n");
    printf("  -J                - just output the last take line (for output to file)\n");
    printf("  -c channel        - channel # if multiple channels (2nd base Camera Link\n");
    printf("                        connector or multiple RCI units on one FOI card (default 0)\n");
    printf("  -u unit           - pdv unit number (default 0)\n");
    printf("                        A full device pathname can also be used\n");
    printf("  -B                - turn off burst enable\n");
    printf("  -X                - use timer\n");
    printf("  -L                - freerun/bytecount with wait for next frame\n");
    printf("  -H                - check count signature from testgen\n");
    printf("  -P                - check count signature from PCI CD sim\n");
    printf("  -G                - check count signature from testgen\n");
    printf("  -Q                - turn on quiet mode\n");
    printf("  -n                - autosequence start only\n");
    printf("  -R                - disable ROI\n");
    printf("  -O                - try noreset - will be obsolete on clink?\n");
    printf("  -x                - experimental - strobe testing\n");
    printf("  -M                - mosaic\n");
    printf("  -a                - call pdv_checkfrm\n");
    printf("  -D                - dvc mode setting - diagnostic\n");
    printf("  -y                - turns on triggering through the optical coupler\n");
    printf("  -Y                - turns on triggering via the field id\n");
    printf("  -h                - this help message\n");
    exit(exitval);
}
