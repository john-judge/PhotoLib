/*
 * taketwo.c
 * 
 * Example program to show merge from multiple boards to an image
 * (C) 1997-2003 Engineering Design Team, Inc.
 */
#include "edtinc.h"

static void usage(char *progname);
static void save_image(u_char * image_p, int width, int height, int depth,
	   char *basename, int count);

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
    int	    numbufs = 2;
    int     started;
    u_char *image_p;
    u_char *image_p1;
    PdvDev *pdv_p;
    PdvDev *pdv_p1;
    char    errstr[64];
    int     loops = 1;
    int     width0, height0, depth;
    int     width1, height1 ;
    int     mergeheight ;
    int     bytedepth ;
    u_char *buffer_ptrs[8] ;
    u_int   bufsize ;

    char edt_devname[128];
    int channel = 0;
    u_int line_offset0, line_offset1 ;
    int line_span0, line_span1 ;
    u_int line_size0, line_size1 ;
    u_int line_count0, line_count1 ;
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
    if ((pdv_p1 = pdv_open_channel(edt_devname, unit+1, channel)) == NULL)
    {
	  sprintf(errstr, "pdv_open_channel(%s%d_%d)", edt_devname, unit, channel);
	  pdv_perror(errstr);
	  return (1);
    }

    /*
     * get image size and name for display, save, printfs, etc.
     */
    width0 = pdv_get_width(pdv_p);
    height0 = pdv_get_height(pdv_p);
    depth = pdv_get_depth(pdv_p);
    width1 = pdv_get_width(pdv_p1);
    height1 = pdv_get_height(pdv_p1);
    bytedepth = bits2bytes(depth) ;
    cameratype = pdv_get_cameratype(pdv_p);

    mergeheight = (height0 > height1) ? height0 : height1 ;

    line_count0 = mergeheight ;
    line_size0 = width0 * bytedepth ;
    line_span0 = (width0 + width1) * bytedepth ;
    line_offset0 = 0 ;

    line_count1 = mergeheight ;
    line_size1 = width1 * bytedepth ;
    line_span1 = (width0 + width1) * bytedepth ;
    line_offset1 = width0 * bytedepth ;


    bufsize = (width0 + width1) * mergeheight * bytedepth ;

    for (i = 0 ; i < numbufs ; i++)
    {
	buffer_ptrs[i] = edt_alloc(bufsize) ;
    }

    /*
     * allocate four buffers for optimal pdv ring buffer pipeline (reduce
     * if memory is at a premium)
     */
    /* set unit 0 to fill in first line_size, followed by 1 */
    edt_set_merge(pdv_p, line_size0, line_span0, line_offset0, line_count0) ;
    edt_set_merge(pdv_p1, line_size1, line_span1, line_offset1, line_count1) ;
    pdv_set_buffers_x(pdv_p, numbufs, bufsize, buffer_ptrs) ;
    pdv_set_buffers_x(pdv_p1, numbufs, bufsize, buffer_ptrs) ;

    printf("reading %d image%s from '%s'\nwidth %d height %d depth %d\n",
	   loops, loops == 1 ? "" : "s", cameratype, width0, height0, depth);
    printf("and width %d height %d\n",width1,height1) ;

    pdv_start_images(pdv_p,numbufs);
    pdv_start_images(pdv_p1,numbufs);
    started = numbufs;

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
	image_p1 = pdv_wait_image(pdv_p1);

	if (i < loops - started)
	{
	    pdv_start_image(pdv_p);
	    pdv_start_image(pdv_p1);
	}
	timeouts = pdv_timeouts(pdv_p);

	if (timeouts > last_timeouts)
	{
	    /*
	     * pdv_timeout_cleanup helps recover gracefully after a
	     * timeout, particularly if multiple buffers were prestarted
	     */
	    if (numbufs > 1)
	    {
		int pending = pdv_timeout_cleanup(pdv_p);
		pdv_start_images(pdv_p, pending);
	    }
	    last_timeouts = timeouts;
	}
	if (*bmpfname)
	{
	    save_image(image_p, width0 + width1, mergeheight, depth, bmpfname, i);
	}
    }
    puts("");

    printf("%d images %d timeouts\n", loops, last_timeouts);

    /*
     * if we got timeouts it indicates there is a problem
     */
    if (last_timeouts)
	printf("check camera and connections\n");
    pdv_close(pdv_p);
    pdv_close(pdv_p1);

    exit(0);
}

static void 
save_image(u_char * image_p, int s_width, int s_height, int s_depth, char *tmpname, int count)
{
    int     s_db = bits2bytes(s_depth);
    u_char *bbuf = NULL;
    char fname[256];

#ifdef _NT_
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

#ifdef RAW
static void 
save_image(u_char * image_p, int s_width, int s_height, int s_depth, char *tmpname, int count)
{
    int     s_db = bits2bytes(s_depth);
    u_char *bbuf = NULL;
    char fname[256];

#ifdef _NT_
    if ((strcmp(&tmpname[strlen(tmpname) - 4], ".bmp") == 0)
	    || (strcmp(&tmpname[strlen(tmpname) - 4], ".BMP") == 0))
	tmpname[strlen(tmpname) - 4] = '\0';
    sprintf(fname, "%s%02d.bmp", tmpname, count);
#else
    if ((strcmp(&tmpname[strlen(tmpname) - 4], ".raw") == 0)
	    || (strcmp(&tmpname[strlen(tmpname) - 4], ".RAW") == 0))
	tmpname[strlen(tmpname) - 4] = '\0';
    sprintf(fname, "%s%02d.raw", tmpname, count);
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
	printf("writing %dx%dx%d raw file to %s\n",
	       s_width, s_height, s_depth, fname);

	dvu_write_raw(s_width * s_height, (u_char *) image_p, fname) ;
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
#endif


void
usage(char *progname)
{
    puts("");
    printf("%s: simple example program that acquires images from an\n", progname);
    printf("EDT digital imaging interface board (PCI DV, PCI DVK, etc.)\n");
    puts("");
    printf("usage: %s\n", progname);
#ifdef _NT_
    printf("  -b fname 	      output to MS bitmap file\n");
#else
    printf("  -b fname        output to Sun Raster file\n");
#endif
    printf("  -l loops        number of loops (images to take)\n");
    printf("  -N numbufs      number of ring buffers (see users guide) (default 4)\n");
    printf("  -u unit         %s unit number (default 0)\n", EDT_INTERFACE);
}
