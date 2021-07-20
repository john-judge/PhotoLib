/*
 * simple_mem.c
 * 
 * Example program to show usage of EDT PCI DV library to acquire and save
 * single or multiple images from device connected to EDT high speed
 * digital imaging interface.
 * 
 * This example is provided as a starting point example for adding digital
 * imaging acquisition to a user program. For more complex operations,
 * including saving data, error detection, diagnostics, and tuning the
 * acquisition in various ways, refer to the "take.c" example.
 * 
 * History 1/26/99 -- substantial rewrite. Uses pdv_multibuf() and
 * pdv_start/wait_image to reflect recommended acquisition method
 * 
 * (C) 1997-1999 Engineering Design Team, Inc.
 */
#include "edtinc.h"

static void usage(char *progname);
static void save_image(u_char * image_p, int width, int height, int depth,
	   char *basename, int count);

u_char **buffers = NULL;


/*
 * main
 */
#ifndef PAGESIZE 
#define PAGESIZE 0x1000
#endif
main(int argc, char **argv)
{
    int     i, j;
    int     unit = 0;
    int     timeouts, last_timeouts = 0;
    char   *progname = argv[0];
    char   *cameratype;
    char    bmpfname[128];
    u_char *image_p;
    PdvDev *pdv_p;
    char    errstr[64];
    int     loops = 1;
    int     width, height, depth;
	int     nbuffers = 16;
	int		nringbuffers = 16;
	int 	nperbuffer = 1;
	int		buffersize;
	int		imagesize;
	int		started;

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
	  case 'o':		/* bitmap save filename */
	    ++argv;
	    --argc;
	    strcpy(bmpfname, argv[0]);
	    break;

	  case 'l':
	    ++argv;
	    --argc;
	    loops = atoi(argv[0]);
		if (loops <= 0)
		{
			fprintf(stderr, "Loops must be greater than 0\n");
			exit(-1);
		}

		break;

	  case 'r':
	    ++argv;
	    --argc;
	    nringbuffers = atoi(argv[0]);
	    break;
		
	  case 'n':
	    ++argv;
	    --argc;
	    nbuffers = atoi(argv[0]);
	    break;

	case 'p':
	    ++argv;
	    --argc;
	    nperbuffer = atoi(argv[0]);
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
	imagesize = pdv_get_imagesize(pdv_p);

    /*
     * allocate nringbuffers buffers for optimal pdv ring buffer pipeline (reduce
     * if memory is at a premium)
     */

    pdv_multibuf(pdv_p, nringbuffers);

    printf("reading %d image%s from '%s'\nwidth %d height %d depth %d\n",
	   loops, loops == 1 ? "" : "s", cameratype, width, height, depth);

	/* 
	 * allocate and touch the secondary buffers
	 */

	nbuffers = loops / nperbuffer;

	if (loops % nperbuffer)
		nbuffers ++ ;

	buffers = (u_char **) calloc(nbuffers, sizeof(u_char *));

	buffersize = nperbuffer * imagesize;	

	for (i = 0; i < nbuffers ; i++)
	{
		buffers[i] = edt_alloc(buffersize);
		for (j=0;j<buffersize;j+= PAGESIZE)
			buffers[i][j] = 0xff;
	}

	printf("Saving %d buffers of size %d\n", nbuffers, buffersize);

    /*
     * start the first acquisition outside the loop
     */
    pdv_start_images(pdv_p, nringbuffers - 1);

	started = nringbuffers - 1;

    for (i = 0; i < loops; i++)
    {
		/*
		 * get the image and immediately start the next one (if not the
		 * last time through the loop). Processing (saving to a file in
		 * this case) can then occur in parallel with the next acquisition
		 */
		printf("image %d\r", i + 1);
		fflush(stdout);
		image_p = pdv_wait_image(pdv_p);

		if (started < loops)
		{
			pdv_start_image(pdv_p);
			started ++ ;
		}

		timeouts = pdv_timeouts(pdv_p);

		if (timeouts > last_timeouts)
			last_timeouts = timeouts;
		else 
		{
			int startindex = (i % nperbuffer) * imagesize;
			int whichbuf = i / nperbuffer;

			memcpy(buffers[whichbuf] + startindex, image_p, imagesize); 

		}
    }

    puts("");

    printf("%d images %d timeouts\n", loops, last_timeouts);

 
	if (bmpfname[0])
	{
		FILE *f = fopen(bmpfname, "w+b");

		for (i=0;i<nbuffers;i++)
		{
			fwrite(buffers[i],1,buffersize, f);
			edt_free(buffers[i]);
		}

		fclose(f);
		
		free(buffers);

	}

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
    printf("EDT digital imaging interface board (PCI DV, PCI DVK, etc.)\n");
    puts("");
    printf("usage: %s\n", progname);
    printf("  -b fname 	      output to raw file\n");
	printf("  -l loops        number of loops (images to take)\n");
    printf("  -u unit         %s unit number (default 0)\n", EDT_INTERFACE);
	printf("  -p per buffer   # of frame per intermediate buffer\n");
	printf("  -r ring buffers # of ring buffers to use\n");
}
