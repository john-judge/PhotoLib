/*
 * simple_sequence.c
 * 
 * Example program to show usage of EDT PCI DV library to acquire and save
 * single or multiple images from device connected to EDT high speed
 * digital imaging interface.
 * 
 * This program takes a finite number of images from the camera at the
 * maximum frame rate and copies them to memory until done acquiring all
 * images. Once all images have been acquired, it saves them each in a
 * separate file. 
 *
 * For other operations, including error detection, diagnostics, and
 * tuning the acquisition in various ways, refer to the other sample
 * programs such as "take.c" and "simple_take.c".
 * 
 * History 6/13/02 -- creation -- started with simple_take.c
 * 
 * (C) 2002 Engineering Design Team, Inc.
 */
#include "edtinc.h"

/*
 * on Windows systems, save formatted file as Windows bitmap.
 * On others (e.g. Solaris, Linux), save as sun Raster
 */
#ifdef _NT_
#define EXT ".bmp"
#define EXTDESC "Windows Bitmap"
#else
#define EXT ".ras"
#define EXTDESC "Sun Raster"
#endif

static void usage(char *progname, char *err);
static int write_image_file(char *tmpfname, u_char * image_p,
		 int s_width, int s_height, int s_depth);
void resolve_fname(char *fname, char *newname, int loops, char *ext);

int     ASnum = 0;

/*
 * main
 */
main(int argc, char **argv)
{
    int     i;
    int     unit = 0;
    int     timeouts = 0;
    char   *progname = argv[0];
    char   *cameratype;
    char    bmpfname[128];
    char    rawfname[128];
    char    tmpfname[128];
    u_char **bufs;
    u_char *image_p;
    PdvDev *pdv_p;
    char    errstr[64];
    int     loops = 1;
    int     numbufs = 1;
    int     width, height, depth, imagesize;
    double  dtime;
    char    edt_devname[128];
    int     channel = 0;

    edt_devname[0] = '\0';
    *bmpfname = '\0';
    *rawfname = '\0';

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
		    usage(progname, "Error: option 'u' requires a numeric argument\n");
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
		    usage(progname, "Error: option 'c' requires a numeric argument\n");
	    }
	    if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
        {
            channel = atoi(argv[0]);
        }
        else 
        {
		    usage(progname, "Error: option 'c' requires a numeric argument\n");
	    }
	    break;

	  case 'i':		/* sun raster or  bitmap save filename */
	  case 'b':
	    ++argv;
	    --argc;
	    strcpy(bmpfname, argv[0]);
	    break;

	  case 'f':		/* raw save filename */
	    ++argv;
	    --argc;
	    strcpy(rawfname, argv[0]);
	    break;

	  case 'l':
	    ++argv;
	    --argc;
	    if (argc < 1) 
        {
		    usage(progname, "Error: option 'l' requires a numeric argument\n");
	    }
	    if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
        {
            numbufs = loops = atoi(argv[0]);
        }
        else 
        {
		    usage(progname, "Error: option 'l' requires a numeric argument\n");
	    }
	    break;

	  default:
	    fprintf(stderr, "unknown flag -'%c'\n", argv[0][1]);
	  case '?':
	  case 'h':
	    usage(progname, "");
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
    imagesize = pdv_get_imagesize(pdv_p);
    cameratype = pdv_get_cameratype(pdv_p);

    if (width <= 0)
    {
	printf("%s%d: Invalid image size. Make sure device has been initialized.\n", edt_devname, unit);
	exit(1);
    }

    /*
     * allocate four buffers for optimal pdv ring buffer pipeline 
     */
    pdv_multibuf(pdv_p, 4);

    /*
     * alloc a list of image buffer pointers then alloc and assign each one 
     * use edt_alloc which page aligns (for faster access)
     */
    bufs = (u_char **)malloc(loops * sizeof(u_char *));
    for (i=0; i<loops; i++)
    {
	if ((bufs[i] = edt_alloc(imagesize)) == NULL)
	{
	    printf("buffer allocation FAILED (probably too many images specified)\n");
	    exit(1);
	}
    }

    printf("reading %d image%s from '%s'....",
	   loops, loops == 1 ? "" : "s", cameratype);
    fflush(stdout);

    /*
     * just one call to start, then get, all the images
     * (time the images too)
     */
    (void) edt_dtime();		/* init time for check */
    pdv_start_images(pdv_p, numbufs);
    for (i=0; i<loops; i++)
    {
	image_p = pdv_wait_image(pdv_p);
	memcpy(bufs[i], image_p, imagesize);
    }
    dtime = edt_dtime();
    printf(" done\n");

    printf("%f frames/sec\n", (double) (loops) / dtime);
    timeouts = pdv_timeouts(pdv_p);

    puts("");
    for (i=0; i<loops; i++)
    {
	if (*bmpfname)
	{
	    resolve_fname(bmpfname, tmpfname, loops, EXT);
	    write_image_file(tmpfname, bufs[i], width, height, depth);
	}
	else if (*rawfname)
	{
	    resolve_fname(rawfname, tmpfname, loops, ".raw");
	    printf("writing %dx%dx%d raw file to %s\n",
		   width, height, depth, tmpfname);

	     (void)dvu_write_raw(imagesize, bufs[i], tmpfname);
	}
    }

    printf("%d images %d timeouts\n", loops, timeouts);

    /*
     * if we got timeouts it indicates there is a problem
     */
    if (timeouts)
    {
	printf("Warning: got %d timeouts (incomplete images)\n", timeouts);
	printf("check camera and connections\n");
    }
    pdv_close(pdv_p);

    exit(0);
}

void
resolve_fname(char *fname, char *newname, int loops, char *ext)
{
    int l = strlen(fname);
    char tmpfname[256];
    char tmpext[5];

    strcpy(tmpfname, fname);
    if ((l > 4) && (tmpfname[l-4] == '.'))
    {
	strcpy(tmpext, &tmpfname[l-4]);
	tmpfname[l-4] = '\0';
    }
    else strcpy(tmpext, ext);
	
    sprintf(newname, "%s%04d%s", tmpfname, ASnum++, tmpext);
}

int 
write_image_file(char *tmpfname, u_char * image_p,
		 int s_width, int s_height, int s_depth)
{
    int     s_db = bits2bytes(s_depth);
    u_char *bbuf = NULL;

    /*
     * write bmp file on Windows systems, or Sun Raster on Unux/Linux
     * systems. Switch on number of bytes per pixel
     */
    switch (s_db)
    {
    case 1:
	printf("writing %dx%dx%d %s file to %s\n",
	       s_width, s_height, s_depth, EXTDESC, tmpfname);

#ifdef _NT_
	dvu_write_bmp(tmpfname, image_p, s_width, s_height);
#else
	dvu_write_rasfile(tmpfname, (u_char *)image_p, s_width, s_height);
#endif
	break;

    case 2:
	printf("converting %dx%dx%d image to 8 bits, writing to %s\n",
	       s_width, s_height, s_depth, tmpfname);

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
	dvu_write_bmp(tmpfname, bbuf, s_width, s_height);
#else
	dvu_write_rasfile16(tmpfname, (u_char *) image_p, s_width, s_height, s_depth);
#endif
	break;

    case 3:
	printf("writing %dx%dx%d bmp file to %s\n",
	       s_width, s_height, s_depth, tmpfname);

#ifdef _NT_
	dvu_write_bmp_24(tmpfname, (u_char *) image_p, s_width, s_height);
#else
	dvu_write_rasfile24(tmpfname, (u_char *) image_p, s_width, s_height);
#endif

	break;

    default:
	printf("invalid image depth for file write...!\n");
	return -1;
	break;
    }

    return 0;
}


void
usage(char *progname, char *err)
{
    puts(err);
    printf("%s: simple example program that acquires a sequence of images\n", progname);
    printf("from an EDT digital imaging interface board (PCI DV, PCI DVK, etc.), and\n");
    printf("saves the sequence in separate raw or %s (%s) format files\n", EXTDESC, EXT );
    puts("");
    printf("usage: %s\n", progname);
    printf("  -l loops        number of loops (images to acquire -- default 1)\n");
    printf("  -u unit         %s unit number (default 0)\n", EDT_INTERFACE);
    printf("  -c channel      channel number (default 0)\n");
    printf("  -b fname        output to %s file (one byte per pixel)\n", EXTDESC);
    printf("  -f fname        output to raw file (one byte per pixel for 8-bit\n");
    printf("                  devices, two bytes per pixel for 10-16 bit devices)\n", EXTDESC);
    exit(1);
}
