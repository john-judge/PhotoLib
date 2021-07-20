/**
 * @file
 * An example program to show usage of EDT PCI DV library to acquire and
 * optionally save single or multiple images from devices connected to EDT
 * high speed digital imaging interface such as the PCI DV C-Link or PCI DV
 * FOX / RCX.
 * 
 * Provided as a starting point example for adding digital image acquisition
 * to a user application.  Includes optimization strategies that take
 * advantage of the EDT ring buffer library subroutines for pipelining image
 * acquisition and subsequent processing. This allows you to achieve higher
 * performance than would normally be possible through a basic acquire/process
 * scheme.
 *
 * The name is somewhat misleading -- because of the parallel aspect,
 * it really isn't the simplest way to do image acquisition.  For a
 * stone-simple example, see simplest_take.c.
 * 
 * For more more complex operations, including error detection, diagnostics,
 * changing camera exposure times, and tuning the acquisition in various
 * ways, refer to the take.c utility. For serial, see serial_cmd.c.
 * 
 * For a sample Windows GUI application code, see wintake.
 * 
 * (C) 1997-2007 Engineering Design Team, Inc.
 */
#include "edtinc.h"

#include "libedt_timing.h"

#include "pdv_irig.h"


static void usage(char *progname);

static void
print_format_header()

{
    printf("-------------------------------------------------------------------------------------\n");
    printf("program  header  type IRIG PPS IRIG PPS image    counts    year.dd/hh:mm:ss.fraction (bcd)\n");
    printf("frame #  frame#        OK  OK  ERR  ERR counts   per sec   Unix seconds.fraction     (unix)\n");
    printf("-------------------------------------------------------------------------------------\n");
}

static void
usage(char *progname)
{
    puts("");
    printf("%s: simple example program that acquires images from an\n", progname);
    printf("EDT PCIe8 C-Link board with IRIG input, printing Status and Time\n");
    puts("");
    printf("usage: %s [-b fname] [-l loops] [-N numbufs] [-u unit] [-c channel]\n", progname);
    printf("  -l loops        number of loops (images to take)\n");
    printf("  -N numbufs      number of ring buffers (see users guide) (default 4)\n");
    printf("  -u unit         %s unit number (default 0)\n", EDT_INTERFACE);
    printf("  -c channel      %s channel number (default 0)\n", EDT_INTERFACE);
    printf("  \n");
    printf("  -s set IRIG to unix seconds\n");
    printf("  -b set IRIG to raw BCD values\n");
    printf("  -e reset IRIG error flags\n");
    printf("  -d direct read (no DMA)\n");
    printf("  -f print frequency (interval between printed IRIG footer - default 1)\n");
    printf("  -t <seconds> timing_offset ( default 2)\n");
    printf("\n");
    printf("  -h              this help message\n");

    printf("Output Format\n");

    print_format_header();
    printf("\n");
}

void
print_irig_header(Irig2Record *rec)

{
    
    int raw = (rec->status.type == IRIG2_RAW_BCD);

    printf("%8d %4s   %s    %s   %s   %s    ",
           rec->framecnt, 
           (rec->status.type == 5) ? "bcd":"unix",
           rec->status.irig_ok? "Y" : "-",
           rec->status.pps_ok? "Y" : "-",
           rec->status.had_irig_error? "Y" : "-",
           rec->status.had_pps_error? "Y" : "-"
           );

    printf("%8u %9d ",
        rec->clocks, rec->tickspps);

    if (raw)
    {
       printf( "%d.%d/%02d:%02d:%02d.%06d",
        rec->t.raw.years + 2000,
        rec->t.raw.days, rec->t.raw.hours, rec->t.raw.minutes,
        rec->t.raw.seconds, 
        (int) (((double) rec->clocks/(double) rec->tickspps) * 1000000.0));
    }
    else
    {
        printf("%8d.%06d",
        rec->t.seconds, 
        (int) (((double) rec->clocks/(double) rec->tickspps) * 1000000.0));
    }
    
}


/*
 * main
 */
int
main(argc, argv)
int     argc;
char  **argv;

{
    int     i;
    int     unit = 0;
    int     overrun, overruns=0;
    int     timeouts, last_timeouts = 0;
    int     recovering_timeout = FALSE;
    char   *progname ;
    char   *cameratype;
    int     numbufs = 4;
    int     started;
    u_char *image_p;
    PdvDev *pdv_p;
    char    errstr[64];
    int     loops = 1;
    int     width, height, depth;
    char    edt_devname[128];
    int     channel = 0;
    int     raw = 0;
    int set_raw = 0;
    int timing_offset = 0;
    int set_offset = 0;
    int irig_reset = 0;
    int print_delta = 1;
	int slave = -1;
	int read_direct = 0;

    u_int spi_reg_base = 0xb1;

    progname = argv[0];


    edt_devname[0] = '\0';

    /*
     * process command line arguments
     */
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
		strncpy(edt_devname, argv[0], sizeof(edt_devname) - 1);
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

	case 'l':
	    ++argv;
	    --argc;
	    loops = atoi(argv[0]);
	    break;

        case 'e':
            irig_reset = 1;
            break;

		case 'b':
            raw = 1;
            set_raw = 1;
            break;

        case 's':
            raw = 0;
            set_raw = 1;
            break;

		case 'S':
			slave = 1;
		break;

		case 'M':
			slave = 0;
		break;

        case 't':
	    ++argv;
	    --argc;
	    timing_offset = atoi(argv[0]);
            set_offset = 1;
	    break;
            
		case 'd':
			read_direct = TRUE;
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

        case 'f':
	    ++argv;
	    --argc;
	    print_delta = atoi(argv[0]);
            if (print_delta <= 0)
                print_delta = 1;
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
     *
     * edt_parse_unit_channel and pdv_open_channel) are equivalent to
     * edt_parse_unit and pdv_open except for the extra channel arg and
     * would normally be 0 unless there's another camera (or simulator)
     * on the second channel (camera link) or daisy-chained RCI (PCI FOI)
     */
    if (edt_devname[0])
    {
	unit = edt_parse_unit_channel(edt_devname, edt_devname, EDT_INTERFACE, &channel);
    }
    else
    {
	strcpy(edt_devname, EDT_INTERFACE);
    }

    if ((pdv_p = pdv_open_channel(edt_devname, unit, channel)) == NULL)
    {
	sprintf(errstr, "pdv_open_channel(%s%d_%d)", edt_devname, unit, channel);
	pdv_perror(errstr);
	return (1);
    }

    pdv_p->spi_reg_base = spi_reg_base;



    if (set_raw)
    {
        edt_set_timecode_raw(pdv_p, raw);
    }

    if (irig_reset)
    {
        pdv_irig_reset_errors(pdv_p);
    }

	if (slave != -1)
	{
		pdv_irig_set_slave(pdv_p, slave);
	}
    if (set_offset)
        edt_set_timecode_seconds_offset(pdv_p, timing_offset);

    pdv_reset_dma_framecount(pdv_p);

    /*
     * get image size and name for display, save, printfs, etc.
     */
    width = pdv_get_width(pdv_p);
    height = pdv_get_height(pdv_p);
    depth = pdv_get_depth(pdv_p);
    cameratype = pdv_get_cameratype(pdv_p);


	if (read_direct)

	{
		Irig2Record rec;

		memset(&rec, 0, sizeof(rec));

		for (i=0;i<loops;i++)
		{
			pdv_irig_get_current(pdv_p, &rec);
            printf("%6d ", i + 1);
            print_irig_header(&rec);
			printf("\n"); fflush(stdout);
		}

		return 0;
	}


    /*
     * allocate four buffers for optimal pdv ring buffer pipeline (reduce if
     * memory is at a premium)
     */
    pdv_multibuf(pdv_p, numbufs);

    printf("reading %d image%s from '%s'\nwidth %d height %d depth %d\n",
	   loops, loops == 1 ? "" : "s", cameratype, width, height, depth);

    /*
     * prestart the first image or images outside the loop to get the
     * pipeline going. Start multiple images unless force_single set in
     * config file, since some cameras (e.g. ones that need a gap between
     * images or that take a serial command to start every image) don't
     * tolerate queueing of multiple images
     */
    if (pdv_p->dd_p->force_single)
    {
	pdv_start_image(pdv_p);
	started = 1;
    }
    else
    {
	pdv_start_images(pdv_p, numbufs);
	started = numbufs;
    }

    print_format_header();

    for (i = 0; i < loops; i++)
    {
	/*
	 * get the image and immediately start the next one (if not the last
	 * time through the loop). Processing (saving to a file in this case)
	 * can then occur in parallel with the next acquisition
	 */

        Irig2Record *hdr;
        u_char *hdrp;

	
	
	image_p = pdv_wait_image(pdv_p);

        hdr = pdv_irig_get_footer(pdv_p, image_p);
        hdrp = (u_char *) hdr;

        if ((i % print_delta) == 0)
        {
            printf("%6d ", i + 1);
            print_irig_header(hdr);
	    printf("\n"); fflush(stdout);
        }

	if ((overrun = (edt_reg_read(pdv_p, PDV_STAT) & PDV_OVERRUN)))
	    ++overruns;

	if (i < loops - started)
	{
	    pdv_start_image(pdv_p);
	}
	timeouts = pdv_timeouts(pdv_p);

	/*
	 * check for timeouts or data overruns -- timeouts occur when data
	 * is lost, camera isn't hooked up, etc, and application programs
	 * should always check for them. data overruns usually occur as a
	 * result of a timeout but should be checked for separately since
	 * ROI can sometimes mask timeouts
	 */
	if (timeouts > last_timeouts)
	{
	    /*
	     * pdv_timeout_cleanup helps recover gracefully after a timeout,
	     * particularly if multiple buffers were prestarted
	     */
	    pdv_timeout_restart(pdv_p, TRUE);
	    last_timeouts = timeouts;
	    recovering_timeout = TRUE;
	    printf("\ntimeout....\n");
	} else if (recovering_timeout)
	{
	    pdv_timeout_restart(pdv_p, TRUE);
	    recovering_timeout = FALSE;
	    printf("\nrestarted....\n");
	}
    }
    puts("");

    printf("%d images %d timeouts %d overruns\n", loops, last_timeouts, overruns);

    /*
     * if we got timeouts it indicates there is a problem
     */
    if (last_timeouts)
	printf("check camera and connections\n");
    pdv_close(pdv_p);

    exit(0);
}

