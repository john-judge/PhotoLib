
/*
 * simple_etrig.c
 * 
 * Variation on the simple_take.c example program, that shows usage of
 * the EDT PCI DV and DMA library to acqire and save single or multiple
 * images from device connected to EDT high speed digital imaging interface,
 * using an event function to determine when an external trigger event has
 * occurred, and initiating the acquire upon recepit of the event.
 * 'Clearing' functionality is included, involving a dummy acquire before
 * the real one, for devices that need to clear the array before taking
 * subsequent images (e.g. hamamatsu 7942 xray detector).
 *
 * History 4/12/2004 -- creation
 *         5/2/2005 -- added some comments about 'clearit'
 * 
 * (C) 2004 Engineering Design Team, Inc.
 */
#include "edtinc.h"

static void usage(char *progname);
static void save_image(u_char * image_p, int width, int height, int depth,
	   char *basename, int count);

int ntrigints = 0;
int nimages = 0;

int loops = 1;
int timeouts;
int last_timeouts = 0;
int image_sleep = 100;
int pulse_sleep = 50;
int clearit = 0;


u_char *image_p = NULL;


/*
 * event callback for trigger interrupt
 */
int
got_trigint(void *p)
{
    PdvDev *pdv_p = (PdvDev *)p;

    ++ntrigints;
    edt_dtime();
    edt_ref_tmstamp(pdv_p,0xdead);
    printf("\n%8.4f %d got trigger interrupt p = %p\n", edt_timestamp(), 
    ntrigints,
    p);

    /* Hamamatsu Xray detector requires a dummy 'clearing' acquire before
     * the real acquire. with other cameras, leave 'clearit' off by
     * skipping the -C flag.
     */
    if (clearit)
    {
	pdv_start_images(pdv_p, 2);
	printf("started two images\n");
    }
    else
    {
	pdv_start_images(pdv_p, 1);
	printf("started one image\n");
    }
    ntrigints++;

    if (clearit)
    {
	printf("waiting for dummy image"); fflush(stdout);
	pdv_wait_image(pdv_p);
	printf(" got it\n");
    }
    printf("waiting for real image"); fflush(stdout);
    image_p = pdv_wait_image(pdv_p);
    printf(" got it\n");
    ++nimages;

    printf("\n%8.4f (%8.4f): trigs %d image %d\n", edt_timestamp(), edt_dtime(), ntrigints, nimages);
    fflush(stdout);
    return 0;
}


#ifdef _NT_

#include <process.h>


unsigned
__stdcall heartbeat(void *pdv)

{
    while (1)
    {
	    Sleep(pulse_sleep);
	    printf("%8.4f pulse %d\n", edt_timestamp(), edt_done_count((EdtDev *)pdv));
	    fflush(stdout);
    }

    return 0;
}

#endif

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
#ifdef __APPLE__
    pthread_t  pulse_thread;
#else
    thread_t pulse_thread;
#endif
    unsigned int thrdid;

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
	    if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
			unit = atoi(argv[0]);
		else
			strncpy(edt_devname, argv[0],sizeof(edt_devname)-1);
	    break;

	  case 'c':		/* device unit number */
	    ++argv;
	    --argc;
	    if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
			channel = atoi(argv[0]);
	    break;

	  case 'b':		/* bitmap save filename */
	    ++argv;
	    --argc;
	    strcpy(bmpfname, argv[0]);
	    break;

	  case 'C':
	    clearit = 1;
	    break;

	  case 's':
	    ++argv;
	    --argc;
	    image_sleep = atoi(argv[0]);
		break;

	  case 'p':
	    ++argv;
	    --argc;
	    pulse_sleep = atoi(argv[0]);
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

    edt_set_event_func(pdv_p, EDT_PDV_EVENT_TRIGINT,
		       (EdtEventFunc) got_trigint, pdv_p, 1);

    /*
     * allocate four buffers for optimal pdv ring buffer pipeline (reduce
     * if memory is at a premium)
     */
    pdv_multibuf(pdv_p, 4);

    printf("reading images from '%s'\nwidth %d height %d depth %d\n",
	   cameratype, width, height, depth);

	/* Launch acquisition thread */

#ifdef _NT_

	if ((pulse_thread = (thread_t)
	 _beginthreadex(NULL, 0, heartbeat, pdv_p, CREATE_SUSPENDED,
			&thrdid)) == NULL)
    {

	return -1;
    }

	if (pulse_sleep)
		ResumeThread(pulse_thread);

#endif

    nimages = 0;
    
    while (1) /* just loop forever */
    {
	/*
	 * get the image and immediately start the next one (if not the
	 * last time through the loop). Processing (saving to a file in
	 * this case) can then occur in parallel with the next acquisition
	 */
	if (image_sleep >= 0)
	    edt_msleep(image_sleep);
	printf(".");

	fflush(stdout);

	timeouts = pdv_timeouts(pdv_p);

	if (timeouts > last_timeouts)
	{
	    last_timeouts = timeouts;
	    printf("check camera and connections\n");
	}
    }	
}

/*
 * write an image to Windows bitmap or Sun Raster format file. update the
 * filename if indicated
 */
void
save_image(u_char * image_p, int width, int height, int depth,
	   char *basename, int count)
{
    char    fname[128];

#ifdef _NT_
    if ((strcmp(&basename[strlen(basename) - 4], ".bmp") == 0)
	    || (strcmp(&basename[strlen(basename) - 4], ".BMP") == 0))
	basename[strlen(basename) - 4] = '\0';
    sprintf(fname, "%s%02d.bmp", basename, count);
    printf("writing %dx%dx%d bmp file to %s\n", width, height, depth, fname);
    dvu_write_bmp(fname, image_p, width, height);
#else
    if ((strcmp(&basename[strlen(basename) - 4], ".ras") == 0)
	    || (strcmp(&basename[strlen(basename) - 4], ".RAS") == 0))
	basename[strlen(basename) - 4] = '\0';
    sprintf(fname, "%s%02d.ras", basename, count);
    printf("writing %dx%dx%d bmp file to %s\n", width, height, depth, fname);
    dvu_write_rasfile(fname, image_p, width, height);
#endif
}


void
usage(char *progname)
{
    puts("");
    printf("%s: simple example program that acquires images from an\n", progname);
    printf("  EDT digital imaging interface board (PCI DV, PCI DVK, etc.)\n");
    puts("");
    printf("usage: %s\n", progname);
    printf("  -u unit         %s unit number (default 0)\n", EDT_INTERFACE);
    printf("  -c channel      channel number (default 0)\n");
    printf("  -l loops        number of loops (images to take)\n");
    printf("  -C              take an extra dummy image first to clear the array\n");
#ifdef _NT_
    printf("  -b fname 	      output to MS bitmap file\n");
#else
    printf("  -b fname        output to Sun Raster file\n");
#endif
}
