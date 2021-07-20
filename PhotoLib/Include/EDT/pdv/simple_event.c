/*
 * simple_event.c
 * 
 * Variation on the simple_take.c example program, that shows usage of
 * the EDT PCI DV and DMA library to acqire and save single or multiple
 * images from device connected to EDT high speed digital imaging interface,
 * using an event function to determine when the camera has finished
 * acquiring the data and has begun the transfer. An example of where this
 * would be used is if there is a "stage" that can be moved once the camera
 * has finished taking the image.
 *
 * History 4/8/99 -- creation
 * 
 * (C) 1999 Engineering Design Team, Inc.
 */
#include "edtinc.h"

static void usage(char *progname);

int nacquires = 0;
int nfvals = 0;
int ntrigints = 0;
int ndmas = 0;
int count = 0;


/*
 * event callback for acquire
 */
    int
got_acquire(void *p)
{
    nacquires++;
    printf("%8.3f %d acquire done\n\n", edt_timestamp(), nacquires);
    fflush(stdout);
    return 0;
}

/*
 * event callback for frame valid
 */
    int
got_fval(void *p)
{
    nfvals++;
    printf("%8.3f %d frame valid\n\n", edt_timestamp(), nfvals);
    fflush(stdout);
    return 0;
}

/*
 * event callback for trigger interrupt
 */
    int
got_trigint(void *p)
{
    ntrigints++;
    printf("%8.3f %d trigger\n\n", edt_timestamp(), ntrigints);
    fflush(stdout);
    return 0;
}

/*
 * event callback for dma done
 */
    int
dma_done(void *p)

{
    ndmas++;
    printf("%8.3f %d end of dma\n", edt_timestamp(), ndmas);
    fflush(stdout);
    return 0;
}

int loops = 1;
int timeouts;
int last_timeouts = 0;
int finished = 0;
int image_sleep = 10;


/*
 * main
 */
main(int argc, char **argv)
{
    int     unit = 0;

    char   *progname = argv[0];
    char   *cameratype;
    char    bmpfname[128];
    PdvDev *pdv_p;
    char    errstr[64];
    int     width, height, depth;
    int     do_fval = 0;


    u_char *image_p;
    int size = 0;
    char edt_devname[128];
    int channel = 0;
    edt_devname[0] = '\0';

    *bmpfname = '\0';

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
                    puts("Error: option 'u' requires a numerical argument");
                    usage(progname);
                }
                if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
                    unit = atoi(argv[0]);
                else
                    strncpy(edt_devname, argv[0],sizeof(edt_devname)-1);
                break;

            case 'c':		/* device channel number */
                ++argv;
                --argc;
                if (argc < 1) 
                {
                    puts("Error: option 'c' requires a numerical argument");
                    usage(progname);
                }
                if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
                    channel = atoi(argv[0]);
                break;

            case 'b':		/* bitmap save filename */
                ++argv;
                --argc;
                strcpy(bmpfname, argv[0]);
                break;

            case 'f':		/* fval not trigint */
                do_fval = 1;
                break;

            case 'l':
                ++argv;
                --argc;
                if (argc < 1) 
                {
                    puts("Error: option 'l' requires a numerical argument");
                    usage(progname);
                }
                if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
                    loops = atoi(argv[0]);
                else
                {
                    puts("Error: option 'l' requires a numerical argument");
                    usage(progname);
                }
                break;

            case 's':
                ++argv;
                --argc;
                image_sleep = atoi(argv[0]);
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
     */

    if (edt_devname[0])
    {
        unit = edt_parse_unit_channel(edt_devname, edt_devname, EDT_INTERFACE, &channel);
    }
    else
    {
        strcpy(edt_devname,EDT_INTERFACE);
    }

    if ((pdv_p = pdv_open_channel(edt_devname, unit, channel)) == NULL)
    {
        sprintf(errstr, "pdv_open_channel(%s%d_%d)", EDT_INTERFACE, unit, channel);
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

    /* set event callback -- 1 in last arg means do it every time,
     * if 0 it would clear the callback after the first one 
     */
    edt_set_event_func(pdv_p, EDT_EVENT_ACQUIRE,
            (EdtEventFunc) got_acquire, NULL, 1);

    edt_set_event_func(pdv_p, EDT_EODMA_EVENT,
            (EdtEventFunc) dma_done, NULL, 1);

    /* FVAL and TRIGINT events are mutually exclusive */
    if (do_fval)
        edt_set_event_func(pdv_p, EDT_PDV_EVENT_FVAL,
                (EdtEventFunc) got_fval, NULL, 1);
    else
        edt_set_event_func(pdv_p, EDT_PDV_EVENT_TRIGINT,
                (EdtEventFunc) got_trigint, NULL, 1);

    /*
     * allocate four buffers for optimal pdv ring buffer pipeline (reduce
     * if memory is at a premium)
     */
    pdv_multibuf(pdv_p, 4);

    printf("reading %d image%s from '%s'\nwidth %d height %d depth %d\n",
            loops, loops == 1 ? "" : "s", cameratype, width, height, depth);

    pdv_start_image(pdv_p);

    for (count = 0; count < loops; count++)
    {
        /*
         * get the image and immediately start the next one (if not the
         * last time through the loop). Processing (saving to a file in
         * this case) can then occur in parallel with the next acquisition
         */
        if (image_sleep >= 0)
            edt_msleep(image_sleep);

        image_p = pdv_wait_image(pdv_p);

        printf("%8.3f %d: image %d\n", edt_timestamp(), nacquires, count + 1);
        fflush(stdout);

        if (count < loops - 1)
            pdv_start_image(pdv_p);

        timeouts = pdv_timeouts(pdv_p);

        if (timeouts > last_timeouts)
            last_timeouts = timeouts;
    }	
    puts("");

    printf("%d images %d timeouts\n", loops, last_timeouts);

    /*
     * if we got timeouts it indicates there is a problem
     */
    if (last_timeouts)
        printf("check camera and connections\n");
    pdv_close(pdv_p);
    exit(0);
}



    void
usage(char *progname)
{
    puts("");
    printf("%s: simple example program that acquires images from an\n", progname);
    printf("  EDT digital imaging interface board (PCI DV, PCI DVK, etc.)\n");
    puts("");
    printf("usage: %s [-u unit] [-c channel] [-r] [-s]\n", progname);
    printf("  -l loops        number of loops (images to take)\n");
    printf("  -u unit         %s unit number (default 0)\n", EDT_INTERFACE);
    printf("  -c channel      %s channel number (default 0)\n", EDT_INTERFACE);
    printf("  -f              set frame valid event instead of (mutally exclusive) trigint\n");
    printf("  -b bmpfname     save to file <bmpfname>\n");
    printf("  -s              sleep between images (diagnostic)\n");
    exit(1);
}
