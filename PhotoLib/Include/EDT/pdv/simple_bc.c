/*
 * simple_bc.c
 * 
 * Example program to show usage of EDT PCI DV library to acquire and save
 * single or multiple images from device connected to EDT high speed
 * digital imaging interface, with frame termination on the frame valid going
 * false interrupt event, counting bytes as they come in. Generally useful
 * for line scan cameras with variable number of lines, but could be adapted
 * for use by area cameras as well, which just wouldn't need the force_timeout
 * part of it.
 *
 * Modified from simple_fv.c (which is a modificaiton of simple_take.c),
 * the main difference being the addition of code that counts the bytes
 * as they come in, and access to the buffers as the data is being read
 * in to them. This can be useful in cases where data processing of
 * incomplete buffers is required.
 *
 * See also simple_take.c for simple acquire of fixed image size,
 * simple_fv.c for simple acquire with frame valid going false interrupt,
 * and pdvevent.c for other events and testing of event functs.
 *
 * History 5/31/01 -- creation (starting with simple_fv.c) 
 *         8/9/01 -- added code to check/discard out of range bc value 
 *                   cleaned up printfs
 * 
 * (C) 2001 Engineering Design Team, Inc.
 */
#include "edtinc.h"

void got_fval(void *) ;
static void usage(char *progname);
static void save_image(u_char * image_p, int width, int height, int depth,
	   char *basename, int count);

int forced_timeouts = 0;
int got_fv = 0;

u_int    last;
u_int    bn;
u_int    bufsize ;
int      verbose=0;
/*
 * main
 */
main(int argc, char **argv)
{
    int     i;
    u_int    bufnum;
    u_char **bufs;
    u_int bc, nbytes;
    int     unit = 0;
    int     timeouts, last_timeouts = 0;
    char   *progname = argv[0];
    char   *cameratype;
    char    bmpfname[128];
    int	    numbufs = 1;
    u_char *image_p = NULL;
    int    imagesize = 0;
    PdvDev *pdv_p;
    char    errstr[64];
    int     loops = 1;
    int     width, height, depth;
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

	  case 'N':
	    ++argv;
	    --argc;
	    numbufs = atoi(argv[0]);
	    break;

	  case 'b':		/* bitmap save filename */
	    ++argv;
	    --argc;
	    strcpy(bmpfname, argv[0]);
	    break;

	  case 'l':
	    ++argv;
	    --argc;
	    loops = atoi(argv[0]);
	    break;

	  case 'v':
	  case 'V':
	    verbose=1;
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

    if (edt_set_event_func(pdv_p, EDT_PDV_EVENT_FVAL,
	     (EdtEventFunc)got_fval, pdv_p, 1) < 0)
     {
	 printf("ERROR: set_event_func(fval) FAILED\n");
	 exit(1);
     }

    /*
     * allocate 1 buffer. use more to increase parallelism
     */
    pdv_multibuf(pdv_p, numbufs);
    bufs = pdv_buffer_addresses(pdv_p);
    bufsize = pdv_get_allocated_size(pdv_p) ;
    printf("bufsize is %d\n\n", bufsize) ;


    printf("reading %d image%s from '%s'\nwidth %d height %d depth %d\n",
	   loops, loops == 1 ? "" : "s", cameratype, width, height, depth);

    /*
     * start the first acquisition outside the loop
     */
    pdv_start_image(pdv_p);

    for (i = 0; i < loops; i++)
    {
	/*
	 * get the image and immediately start the next one (if not the
	 * last time through the loop). Processing (saving to a file in
	 * this case) can then occur in parallel with the next acquisition
	 */
	fflush(stdout);
	while (!got_fv)
	{
	    int j=0;

	    /*
	     * get # of bytes read so far. check for > bufsize (unsigned) to catch
	     * and ignore occasional spurious bad bufcount reads (benign edge
	     * condition in driver)
	     */
	    while ((bc = edt_get_bufbytecount(pdv_p, &bn)) > bufsize)
	    {
		if (++j > 10000)
		{
		    printf("too many bad reads -- exiting\n");
		    exit(1);
		}
	    }

	    bufnum = (bn) % numbufs;
	    if (verbose)
		printf("buf %d (%d) bytecount %d\n", bufnum, bn, bc);
	    fflush(stdout);

	    /* process buffer data on bufs[bufnum] here */
	}

	/* got_fv */
	{
	    if (verbose)
		puts("");
	    bc = edt_get_bytecount(pdv_p);
	    nbytes = edt_get_timeout_count((PdvDev *)pdv_p);
    printf("frame %d buffer %d done; got %d bytes (bc %d) (%0.1f lines)\r", i, bufnum, nbytes, bc, (float)nbytes/(float)pdv_get_width(pdv_p));
	    got_fv = 0;
	}

	if (i < loops-1)
	{
	    if (verbose)
		puts("called start_image") ;
	    pdv_start_image(pdv_p);
	}
	timeouts = pdv_timeouts(pdv_p) - forced_timeouts;

	if (timeouts > last_timeouts)
	    last_timeouts = timeouts;
	if (*bmpfname)
	    save_image(image_p, width, height, depth, bmpfname, i);
    }

    printf("\n%d images %d timeouts\n", loops, last_timeouts);

    /*
     * if we got timeouts it indicates there is a problem
     */
    if (last_timeouts)
	printf("check camera and connections\n");

    edt_msleep(10); /* wait a little longer for last FV before closing */

    pdv_close(pdv_p);

    exit(0);
}

int fv=0;

/*
 * callback function. force a timeout so the pending wait_image will
 * return immediately.
 */
void
got_fval(void *pdv_p)
{
    int nbytes;

    if (verbose)
	printf("\nfval #%d, ", ++fv);
    ++forced_timeouts;
    edt_do_timeout((PdvDev *)pdv_p);
    got_fv = 1;

    /*
     * this code recovers from the timeout, especially
     * when you've prestarted multiple buffers or in this
     * case if you've forced a timeout
     */
    {
	int curdone, curtodo;
	curdone = edt_done_count(pdv_p) ;
	curtodo = edt_get_todo(pdv_p) ;
	pdv_stop_continuous(pdv_p) ;
	edt_set_buffer(pdv_p,curdone) ;
	edt_reg_write(pdv_p, PDV_CMD, PDV_RESET_INTFC) ;
	pdv_setup_continuous(pdv_p) ;
	/* pdv_start_images(pdv_p, curtodo - curdone); */
    }


    /* don't need since we're reading it out as it comes in */
    /* pdv_wait_image((PdvDev *)pdv_p); */

    last = edt_get_timeout_count(pdv_p);
    bn = edt_done_count(pdv_p) ;
    if (verbose)
	printf("final byte count %d\n", last);
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
    printf("EDT DV series digital imaging interface board\n");
    puts("");
    printf("usage: %s\n", progname);
#ifdef _NT_
    printf("  -b fname 	      output to MS bitmap file\n");
#else
    printf("  -b fname        output to Sun Raster file\n");
#endif
    printf("  -l loops        number of loops (images to take -- default 1)\n");
    printf("  -N numbufs      number of ring buffers (see users guide) (default 1)\n");
    printf("  -u unit         %s unit number (default 0)\n", EDT_INTERFACE);
}
