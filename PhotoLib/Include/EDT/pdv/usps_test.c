/*
 * usps_grab.c
 * 
 * Example program to show usage of EDT PCI DV library to acquire and save
 * single or multiple images from device connected to EDT high speed
 * digital imaging interface, with frame termination on a frame valid false
 * interrupt event.
 *
 * Modified from simple_take.c, with the difference being that the
 * frame valid event function is set and the image is aborted when
 * a frame valid going false event occurs. Used for cameras that output a
 * variable number of lines per frame (for example, line scan cameras with
 * externally generated frame valid).
 *
 * See also pdvevent.c for other events and testing of event functs.
 *
 * See simple_take.c for simple example that doesn't use the frame valid
 * interrupt.
 * 
 * History 3/16/00 -- creation (starting with simple_take.c) 
 * 
 * (C) 2000 Engineering Design Team, Inc.
 */
#include "edtinc.h"

#define FCI_USPS_LINES	PDV_VSKIP
#define FCI_USPS_WIDTH	PDV_HACTV

typedef struct {

	u_short hactive,
		rasters,
		underruns,
		overruns,
		lv_edges,
		fv_edges,
		framecount;

} USPSFrameData;
USPSFrameData *fd;

USPSFrameData * get_raster_data(PdvDev *pdv_p, u_char *buf) ;

void print_raster_data(USPSFrameData *fd) ;

static void usage(char *progname);
static void save_image(u_char * image_p, int width, int height, int depth,
	   char *basename, int count);

int forced_timeouts = 0;

int fv=0;



/*
 * main
 */
main(int argc, char **argv)
{
    int     i;
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
	int started = 1;
	int timeout = -1;
	int verbose = 0;
	int fval_use = 1;

	char edt_devname[128];
	int channel = 0;

	double *times = NULL;

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

	  case 's':
	    ++argv;
	    --argc;
	    started = atoi(argv[0]);
	    break;

	  case 't':
	    ++argv;
	    --argc;
	    timeout = atoi(argv[0]);
	    break;

	  case 'v':
	    verbose = 1;
	    break;

	  case 'f':
	    fval_use = 0;
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


 //   if (edt_set_event_func(pdv_p, EDT_PDV_EVENT_FVAL,
//	     (EdtEventFunc)print_raster_data, pdv_p, 1) < 0)
  //   {
//	 printf("ERROR: set_event_func(fval) FAILED\n");
//	 exit(1);
   //  }

	{
		u_char i;
		i = fval_use;
		edt_ioctl(pdv_p, EDTS_FVAL_DONE, &i);

		edt_reg_and(pdv_p, PDV_UTIL3, ~PDV_TRIGINT);
	}
	edt_ref_tmstamp(pdv_p,0xbeeffeed);

	times = calloc(loops, sizeof(double));

    /*
     * allocate four buffers for optimal pdv ring buffer pipeline (reduce
     * if memory is at a premium)
     */
    pdv_multibuf(pdv_p, numbufs);

    printf("reading %d image%s from '%s'\nwidth %d height %d depth %d\n",
	   loops, loops == 1 ? "" : "s", cameratype, width, height, depth);

    /*
     * start the first acquisition outside the loop
     */

	if (timeout >= 0)
		edt_set_rtimeout(pdv_p, timeout);

	edt_dtime();

    pdv_start_images(pdv_p,started);

	
    for (i = 0; i < loops; i++)
    {
		u_int timevec[2];
		u_int rasterlines;

		/*
		 * get the image and immediately start the next one (if not the
		 * last time through the loop). Processing (saving to a file in
		 * this case) can then occur in parallel with the next acquisition
		 */
		fflush(stdout);
		image_p = pdv_wait_image_timed(pdv_p, timevec);
		
		if (i < loops-started)
		{
			/* check start time */

			pdv_start_image(pdv_p);
			
		}


		times[i] = (double) timevec[0] + (double) timevec[1] * 0.000000001;

		rasterlines = pdv_get_lines_xferred(pdv_p);
		width = pdv_get_width_xferred(pdv_p);

		if (i > 0)
			printf("%-4d: Lines %5d width = %5d Delta t = %f\n", i, rasterlines, width, times[i] - times[i-1]);
		else
			printf("%-4d: Lines %5d\n", i, rasterlines);

		fd = get_raster_data(pdv_p, image_p);

		if (verbose) 
			print_raster_data(fd);

		timeouts = pdv_timeouts(pdv_p) - forced_timeouts;

		width = pdv_get_width(pdv_p);

		if (timeouts > last_timeouts)
			last_timeouts = timeouts;
		
		if (*bmpfname)
			save_image(image_p, width, rasterlines, depth, bmpfname, i);

    }
    puts("");

    printf("%d images %d timeouts\n", loops, last_timeouts);

    pdv_close(pdv_p);

    exit(0);
}



/*
 * Get the raster data
 */
USPSFrameData *
get_raster_data(PdvDev *pdv_ptr, u_char *buf)
{
	PdvDev *pdv_p = (PdvDev *) pdv_ptr;

 	int nlines;
	int nwidth;

	int pitch = pdv_get_pitch(pdv_p);
	

	/* read how many lines came in so we can find the last line */

    nlines = edt_reg_read((PdvDev *)pdv_p, FCI_USPS_LINES);
    nwidth = edt_reg_read((PdvDev *)pdv_p, FCI_USPS_WIDTH);
	/* get a pointer to the start of the current buffer */

	/* get the start of the last line with frame data */

	buf += (nlines * pitch);

	/* cast to USPSFrameData structure */

	return (USPSFrameData *) buf;


}

void
print_raster_data(USPSFrameData *fd)

{

/* get the current buffer */
 
	printf("Data from last line:\n");
	printf("	hactive    %d\n",fd->hactive);
	printf("	rasters    %d\n", fd->rasters);
	printf("	underruns  %d\n", fd->underruns);
	printf("	overruns   %d\n", fd->overruns);
	printf("	lv_edges   %d\n", fd->lv_edges);
	printf("	fv_edges   %d\n",fd->fv_edges);
	printf("	framecount %d\n",fd->framecount & 0xff);
	printf("	FV seen    %d\n",fd->framecount >> 8);

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
    printf("EDT digital imaging interface board (PCI DV, etc.)\n");
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
