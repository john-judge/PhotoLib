#include "edtinc.h"

static void usage(char *progname);


int	count = 0 ;
int     loops = 1;
int     timeouts;
int     last_timeouts = 0;
int finished = 0;

/*
 * main
 */
main(int argc, char **argv)
{
    int     unit = 0;
 
	char   *progname = argv[0];
    char   *cameratype;
    PdvDev *pdv_p;
    char    errstr[64];
    int     width, height, depth;

    u_char *image_p;
    int size = 0;

	char edt_devname[128];
	int channel = 0;
	edt_devname[0] = '\0';


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

	  case 'l':
	    ++argv;
	    --argc;
	    loops = atoi(argv[0]);
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
    edt_enable_event(pdv_p, EDT_EVENT_ACQUIRE) ;
    
    /*
     * allocate four buffers for optimal pdv ring buffer pipeline (reduce
     * if memory is at a premium)
     */
    pdv_multibuf(pdv_p, 4);

    printf("reading %d image%s from '%s'\nwidth %d height %d depth %d\n",
	   loops, loops == 1 ? "" : "s", cameratype, width, height, depth);

	/* Launmch acquisition thread */


    pdv_start_image(pdv_p);

    for (count = 0; count < loops; count++)
    {
	printf("waiting for acquire %d\n",count+1) ; 
	edt_wait_event(pdv_p, EDT_EVENT_ACQUIRE,0) ;
	printf("%8.3f: got event\n", edt_timestamp()) ;
	printf("waiting for image %d\n",count+1) ; 
	image_p = pdv_wait_image(pdv_p);
	printf("%8.3f: image %d\n", edt_timestamp(), count + 1);
	fflush(stdout);

	if (count < loops - 1)
	{
		pdv_start_image(pdv_p);
	}
	timeouts = pdv_timeouts(pdv_p);

	if (timeouts > last_timeouts)
		last_timeouts = timeouts;
   }	

    printf("%d images %d timeouts\n", loops, last_timeouts);

    /*
     * if we got timeouts it indicates there is a problem
     */
    if (last_timeouts)
	printf("check camera and connections\n");
    pdv_close(pdv_p);
    exit(0);
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
    printf("  EDT didital imaging interface board (PCI DV, PCI DVK, etc.)\n");
    puts("");
    printf("usage: %s\n", progname);
#ifdef _NT_
    printf("  -b fname 	      output to MS bitmap file\n");
#else
    printf("  -b fname        output to Sun Raster file\n");
#endif
    printf("  -l loops        number of loops (images to take)\n");
    printf("  -u unit         %s unit number (default 0)\n", EDT_INTERFACE);
}
