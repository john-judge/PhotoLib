/*
 * new_strobe.c
 * 
 * Example program to show usage of EDT PCI DV library to acquire images using
 * the optional strobe functionality -- NEW method. Only usable with bitfiles that
 * have the NEW strobe functionality.
 * 
 * History 04/15/2002 rwh -- creation (started with strobe.c)
 * 
 * (C) 2002 Engineering Design Team, Inc.
 */
#include "edtinc.h"

static void usage(char *progname);
static int write_image_file(u_char * image_p,
	int width, int height, int depth, char *basename, int count);
static void save_raw_image(u_char * image_p,
	int width, int height, int depth, char *basename, int count);

volatile int Strobe_inter = 0;	/* global strobe interrupt flag */

/*
 * main
 */
main(int argc, char **argv)
{
    int     i;
    int     unit = 0;
    int     timeouts, last_timeouts = 0;
    int     user_timeout = -1;
    char   *progname = argv[0];
    char   *cameratype;
    char    bmpfname[128];
    char    rawfname[128];
    u_char *image_p;
    PdvDev *pdv_p;
    char    errstr[64];
    int     width, height, depth;
    int     *count, *delay, *period;
    int	    ncounts = 0;
    int     loops = 1;
    u_int   dac;
    int     set_dac = 0;
    int     leave_enabled = 0;

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
	    if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
		unit = atoi(argv[0]);
	    break;

	  case 'l':		/* loops */
	    ++argv;
	    --argc;
	    loops = atoi(argv[0]);
	    break;

	  case 'd':		/* strobe DAC voltage */
	    ++argv;
	    --argc;
	    sscanf(argv[0], "%d", &dac);
	    set_dac = 1;
	    break;


	  case 'b':		/* bitmap save filename */
	    ++argv;
	    --argc;
	    strcpy(bmpfname, argv[0]);
	    break;

	  case 't':		/* user timeout */
	    ++argv;
	    --argc;
	    user_timeout = atoi(argv[0]);
	    break;

	  case 'e':		/* leave strobe enabled after close */
	    leave_enabled = 1;
	    break;

	  case 'f':		/* bitmap save filename */
	    ++argv;
	    --argc;
	    strcpy(rawfname, argv[0]);
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

    if (argc < 3)
    {
	usage(progname);
	exit(0);
    }

    count = malloc(sizeof(int) * ((argc+2)/3 * loops));
    delay = malloc(sizeof(int) * ((argc+2)/3 * loops));
    period = malloc(sizeof(int) * ((argc+2)/3 * loops));

    while (argc > 1)
    {
	int l = loops;
	while (l--)
	{
	    count[ncounts] = atoi(argv[0]);
	    delay[ncounts] = atoi(argv[1]);
	    period[ncounts++] = atoi(argv[2]);
	}
	argc -=3;
	argv +=3;
    }

    if ((pdv_p = pdv_open(EDT_INTERFACE, unit)) == NULL)
    {
	sprintf(errstr, "pdv_open(%s%d)", EDT_INTERFACE, unit);
	pdv_perror(errstr);
	return (1);
    }

    width = pdv_get_width(pdv_p);
    height = pdv_get_height(pdv_p);
    depth = pdv_get_depth(pdv_p);
    cameratype = pdv_get_cameratype(pdv_p);

    if (width * height <= 0)
    {
	printf("%s%d: Invalid image size. Make sure device has been initialized.\n", EDT_INTERFACE, unit);
	exit(1);
    }

    if (user_timeout >= 0)
	pdv_set_timeout(pdv_p, user_timeout);

    /*
     * allocate two buffers; this allows one to be set up while the other is
     * going. Since we're usually doing single acquires, multiple buffers
     * are of limited use but can still be used to reduce start time
     */
    pdv_multibuf(pdv_p, 2);

    printf("reading %d image%s from '%s'\nwidth %d height %d depth %d\n",
	   ncounts, ncounts == 1 ? "" : "s", cameratype, width, height, depth);

    /*
     * make sure it's in controlled mode (make sure device is present and
     * NOTE: with this method you could also use triggered mode
     */
    if (pdv_is_kodak_i(pdv_p))
	pdv_serial_command(pdv_p, "MDE CD");
    else if (pdv_is_atmel(pdv_p))
    {
	pdv_serial_command(pdv_p, "T=2");
	pdv_serial_command(pdv_p, "S=0");
    }

    /*
     * Set up strobe counters and DV registers. Different depending
     * on which method is applicable with this board and Xilinx 
     */
    if (pdv_strobe_method(pdv_p) != PDV_LHS_METHOD2)
    {
	printf("ERROR: strobe not implemented this Xilinx!\n");
	if (pdv_p->devid == PDVCL_ID)
	    printf("Check for newer (April 15 2002 or later) camera link PCI Xilinx\n");
	else
	{
	    printf("Check for post April-2002 aiag.bit with new strobe method implemented\n");
	}
	exit(1);
    }

    pdv_enable_strobe(pdv_p, 1);

    /*
     * set DAC voltage level. Valid values are 0-4095 decimal
     * This is the same regardless of which strobe method is in use
     */
    if (set_dac)
    {
	printf("setting dac to 0x%x\n", dac);
	pdv_set_strobe_dac(pdv_p, dac);
    }

    for (i = 0; i < ncounts; i++)
    {
	pdv_set_strobe_counters(pdv_p, count[i], delay[i], period[i]);
	printf("strobe count %d delay %d period %d\n", count[i], delay[i], period[i]);

	image_p = pdv_image(pdv_p);

	if ((timeouts = pdv_timeouts(pdv_p)) > last_timeouts)
	    last_timeouts = timeouts;
	else if (*bmpfname)
	    write_image_file(image_p, width, height, depth, bmpfname, i);
	else if (*rawfname)
	    save_raw_image(image_p, width, height, depth, rawfname, i);
    }
    puts("");

    /* BE SURE to disable the strobe when done; otherwise it will be
     * activated by any subsequent acquires by other programs -- okay
     * if its really what you want but dangerous if you have a program
     * that, say, goes into continuous mode as it could wear out the
     * strobe.
     */
    if (!leave_enabled)
	pdv_enable_strobe(pdv_p, 0);

    printf("%d images %d timeouts\n", ncounts, last_timeouts);
    if (last_timeouts)
	printf("check camera and connections\n");
    pdv_close(pdv_p);
    exit(0);
}


int 
write_image_file(u_char * image_p, int s_width, int s_height, int s_depth,
						char *basename, int count)
{
    int     s_db = bits2bytes(s_depth);
    u_char *bbuf = NULL;
    char fname[128];

#ifdef _NT_
    if ((strcmp(&basename[strlen(basename) - 4], ".bmp") == 0)
	|| (strcmp(&basename[strlen(basename) - 4], ".BMP") == 0))
	basename[strlen(basename) - 4] = '\0';
    sprintf(fname, "%s%02d.bmp", basename, count);
    printf("writing %dx%dx%d bmp file to %s\n", s_width, s_height, s_depth, fname);
    dvu_write_bmp(fname, image_p, s_width, s_height);
#else
    if ((strcmp(&basename[strlen(basename) - 4], ".ras") == 0)
	|| (strcmp(&basename[strlen(basename) - 4], ".RAS") == 0))
	basename[strlen(basename) - 4] = '\0';
    sprintf(fname, "%s%02d.ras", basename, count);
    printf("writing %dx%dx%d bmp file to %s\n", s_width, s_height, s_depth, fname);
    if (s_depth > 8)
	dvu_write_rasfile16(fname, image_p, s_width, s_height, s_depth);
    else dvu_write_rasfile(fname, image_p, s_width, s_height);
#endif

    /*
     * write bmp file on Windows systems, or Sun Raster on Unux/Linux
     * systems. Switch on number of bytes per pixel
     */
    switch (s_db)
    {
    case 1:
#ifdef _NT_
	printf("writing %dx%dx%d bmp file to %s\n",
	       s_width, s_height, s_depth, fname);

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
	return -1;
	break;
    }

    return 0;
}

/*
 * write an image, raw
 */
void
save_raw_image(u_char * image_p, int width, int height, int depth,
	   char *basename, int count)
{
    char    fname[128];

    if ((strcmp(&basename[strlen(basename) - 4], ".raw") == 0)
	|| (strcmp(&basename[strlen(basename) - 4], ".RAW") == 0))
	basename[strlen(basename) - 4] = '\0';
    sprintf(fname, "%s%02d.raw", basename, count);
    printf("writing %dx%dx%d raw file to %s\n", width, height, depth, fname);
    dvu_write_raw(width * height * bits2bytes(depth), image_p, fname);
}


void
usage(char *progname)
{
    puts("");
    printf("%s: simple example program for EDT digital imaging interface, showing\n", progname);
    printf("how to use the optional strobe functionality\n");
    puts("");
    printf("usage: %s [-u unit] [-b fname] [-d dac] count delay [period]...\n", progname);
#ifdef _NT_
    printf("  -b fname 	      output to MS bitmap file\n");
#else
    printf("  -b fname        output to Sun Raster file\n");
#endif
    printf("  -e              leave strobe counters enabled after close\n");
    printf("  -l loops        number of times to run each sequence\n");
    printf("  -t timeout      timeout (milliseconds) -- overrides the autwo timeout value\n");
    printf("  -d dac          DAC voltage level for strobe (0-4095)\n");
    printf("  -u unit         %s unit number (default 0)\n", EDT_INTERFACE);
    printf("  countN          no. of pulses (1-4096) (required)\n");
    printf("  delayN          delay before/after pulses (10-265) (required)\n");
    printf("  periodN         period between each pulse (0-255) (required)\n");
    printf("                  (Multiple count/interval pairs can be specified)\n");
}
