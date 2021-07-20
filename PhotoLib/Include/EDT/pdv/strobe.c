/*
 * strobe.c
 * 
 * Example program to show usage of EDT PCI DV library to acquire images using
 * the optional strobe functionality. Only usable with bitfiles that have
 * strobe functionality (aiags.bit).
 * 
 * History 02/02/1999 rwh -- creation (started with simple_take.c)
 * 
 * (C) 1999 Engineering Design Team, Inc.
 */
#include "edtinc.h"

static void usage(char *progname);
static void save_image(u_char * image_p,
	int width, int height, int depth, char *basename, int count);
static void save_raw_image(u_char * image_p,
	int width, int height, int depth, char *basename, int count);

volatile int Strobe_inter = 0;	/* global strobe interrupt flag */

void
strobe_done()
{
    Strobe_inter = 1;
    puts("strobe_done");
}

void
acquire_done()
{
    puts("acquire done");
}

/*
 * main
 */
main(int argc, char **argv)
{
    int     i, ret;
    int     unit = 0;
    int     timeouts, last_timeouts = 0;
    char   *progname = argv[0];
    char   *cameratype;
    char    bmpfname[128];
    char    rawfname[128];
    u_char *image_p;
    PdvDev *pdv_p;
    char    errstr[64];
    int     width, height, depth;
    int     count[100], interval[100];
    int	    ncounts = 0;
    u_int   cfgreg, mcl;
    u_int   dac;
    int     set_dac = 0;

	char edt_devname[128];
	int channel = 0;
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


	  case 'd':		/* strobe DAC voltage */
	    ++argv;
	    --argc;
	    sscanf(argv[0], "%d", &dac);
	    set_dac = 1;
	    break;


	  case 'b':		/* bitmap save filename (.bmp and .ras) */
	    ++argv;
	    --argc;
	    strcpy(bmpfname, argv[0]);
	    break;

	  case 'f':		/* bitmap save filename (.raw) */
	    ++argv;
	    --argc;
	    strcpy(rawfname, argv[0]);
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

    if (argc < 2)
    {
	usage(progname);
	exit(0);
    }

    while (argc > 1)
    {
	count[ncounts] = atoi(argv[0]); --argc; ++argv;
	interval[ncounts++] = atoi(argv[0]); --argc; ++argv;
    }

 
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

    width = pdv_get_width(pdv_p);
    height = pdv_get_height(pdv_p);
    depth = pdv_get_depth(pdv_p);
    cameratype = pdv_get_cameratype(pdv_p);

    /*
     * allocate two buffers; this allows one to be set up while the other is
     * going. With the way we are doing program controlled shutter, multiple
     * buffers are of limited use but can still be used to reduce start time
     */
    pdv_multibuf(pdv_p, 2);

    printf("reading %d image%s from '%s'\nwidth %d height %d depth %d\n",
	   ncounts, ncounts == 1 ? "" : "s", cameratype, width, height, depth);

    /*
     * make sure it's in controlled mode (make sure device is present and initialized)
     * config file though.)
     */
    if (pdv_is_kodak_i(pdv_p))
	pdv_serial_command(pdv_p, "MDE CD");
    else if (pdv_is_atmel(pdv_p))
    {
	pdv_serial_command(pdv_p, "T=2");
	pdv_serial_command(pdv_p, "S=0");
    }

#ifdef  USE_THREADS
    edt_set_event_func(pdv_p, EDT_PDV_STROBE_EVENT,
		       (EdtEventFunc) strobe_done, NULL, 1);
#else
	edt_enable_event(pdv_p, EDT_PDV_STROBE_EVENT) ;
#endif

    /*
     * disable triggered mode in config register
     */
    cfgreg = edt_reg_read(pdv_p, PDV_CFG);
    edt_reg_write(pdv_p, PDV_CFG, cfgreg & ~PDV_TRIG);

    /*
     * With this xilinx, mode control register works a little differently.
     * Instead of the top nibble controlling which is the EXPOSE line,
     * it is used to clock in the DAC voltage level for the strobe.
     * But we do use the bottom nibble to manually control the shutter.
     * So when doing this, make sure to preserve the top nibble so we
     * don't screw up the voltage inadvertently.
     */
    mcl = edt_reg_read(pdv_p, PDV_MODE_CNTL) & 0xf0;
    edt_reg_write(pdv_p, PDV_MODE_CNTL, mcl);

    /*
     * set DAC voltage level. Valid values are 0-4095 decimal
     */
    if (set_dac)
    {
	printf("setting dac to 0x%x\n", dac);
	pdv_set_strobe_dac(pdv_p, dac);
    }


    for (i = 0; i < ncounts; i++)
    {
	/*
	 * since mode control exe bit is off this will set up the DMA but
	 * nothing else
	 */
	printf("strobe count %d interval %d\n", count[i], interval[i]);
	pdv_start_image(pdv_p);

	/*
	 * manually toggle shutter open
	 */
	edt_reg_write(pdv_p, PDV_MODE_CNTL, mcl | 0x1);	/* open shutter */

	/*
	 * Set and fire the strobe. Number of pulses will be count+1,
	 * range for count is 0-3ff hex, so real # of pulses is 1-4096 dec.
	 * delay will be 100us * ((interval & 0x7f)+10), range for
	 * delay is 0-0x7f so real interval will be 10-137 dec.
	 */
	if ((ret = pdv_strobe(pdv_p, count[i], interval[i])) != 0)
	{
	    printf("pdv_strobe returned ERROR.\n");
	    printf("Make sure device is installed and initialized.\n");
	    exit(1);
	}

	/*
	 * wait for strobe_int to be set (by func set via set_event_func)
	 * then clear it for next time
	 */
#ifdef USE_THREADS
	while (!Strobe_inter)
	    ;
	Strobe_inter = 0;
#else
	edt_wait_event(pdv_p, EDT_PDV_STROBE_EVENT,0) ;
#endif

	edt_reg_write(pdv_p, PDV_MODE_CNTL, mcl);	/* close shutter */

	if (pdv_is_atmel(pdv_p))
	    pdv_serial_command(pdv_p, "S=1");

	image_p = pdv_wait_image(pdv_p);

	timeouts = pdv_timeouts(pdv_p);

	if (timeouts > last_timeouts)
	    last_timeouts = timeouts;
	else if (*bmpfname)
	    save_image(image_p, width, height, depth, bmpfname, i);
	else if (*rawfname)
	    save_raw_image(image_p, width, height, depth, rawfname, i);
    }
    puts("");

    printf("%d images %d timeouts\n", ncounts, last_timeouts);
    if (last_timeouts)
	printf("check camera and connections\n");
    pdv_close(pdv_p);
    exit(0);
}

/*
 * write an image to Windows bitmap format file. update the filename if
 * indicated
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
    if (depth > 8)
	dvu_write_rasfile16(fname, image_p, width, height, depth);
    else dvu_write_rasfile(fname, image_p, width, height);
#endif
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
    printf("usage: %s [-u unit] [-c channel] [-b fname] [-f fname] [-d dac] <count interval>...\n", progname);
#ifdef _NT_
    printf("  -b fname 	      output to MS bitmap file\n");
#else
    printf("  -b fname        output to Sun Raster file\n");
#endif
		printf("  -f fname        output to raw image file\n");
    printf("  -d dac          DAC voltage level for strobe (0-4095)\n");
    printf("  -u unit         %s unit number (default 0)\n", EDT_INTERFACE);
		printf("  -c channel      %s channel number (default 0)\n", EDT_INTERFACE);
    printf("  countN          no. of pulses (1-4096) (required)\n");
    printf("  intervalN       delay between and before/after pulses (10-265) (required)\n");
    printf("                  (Multiple count/interval pairs can be specified)\n");
}
