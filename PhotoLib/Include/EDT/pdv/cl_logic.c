/**
 * @file
 * Runs a logic analyzer on the PCI DV C-Link to analyze camera input data.
 * Expects camera to be connected and providing continuous pixel clock,
 * line valid and frame valid signals.  Depends on special logic analyzer
 * firmware loaded on the EDT PCI DV C-Link board -- can optionally
 * load the firmware itself -- see -b option in the usage.
 */

#include <stdio.h>
#include <string.h>

#include "edtinc.h"

#include "cl_logic_lib.h"

ClLogicSummary cls;

void
usage(char *progname, char *err)
{
    printf("%s", err);
    printf("usage: %s \n", progname);
    printf("  -u unit         - %s unit number (default 0)\n", EDT_INTERFACE);
    printf("  -l loops        - number of loops (default 1)\n");
    printf("  -f filename     - read data from filename (default pdv output)\n");
    printf("  -b              - load logic analyzer flash bitfile\n");
    printf("  -t              - timeout value (default 2000, 0=forever)\n");
    printf("  -m              - max number of timeouts before bailing out (default 5)\n");
    printf("  -s size         - buffer size to use (default 10000)\n");
    printf("  -v              - verbose\n");
    printf("  -q              - quiet\n");
    printf("  -d              - use testmask (?)\n");
    printf("  -o              - name of the file to store the captured logic analyzer data \n");
    printf("  -F frequency    - frequency to set the pixel clock value \n");
}

int 
main(int argc, char **argv)
{

    FILE   *f = NULL;
    FILE   *outfile = NULL;

    char    filename[200];
    char    outfilename[200];

    int     verbose = 0;
    int     timeout = 2000;
    int     got_timeouts;
    int     max_timeouts = 5;

    PdvDev *pdv_p = NULL;
    int     unit = 0;
    int     channel = 0;
    unsigned int loops = 10;
    int     numbufs = 8;
    int	    bufsize = CL_LOGIC_DEFAULT_BUFSIZE;

    ClLogicStat last_stats;
    int     load = 0;

    int i;
    int retval;

    double pixel_clock = 20000000.0;

    int quiet = 0;

    int testmask = FV_MASK | LV_MASK;

    filename[0] = 0;
    outfilename[0] = 0;


    for (i = 1; i < argc; i++)
    {
	if (argv[i][0] == '-')
	{
	    switch (argv[i][1])
	    {
	    case 'l':
		if (++i < argc)
		    loops = strtol(argv[i], 0, 0);
		break;

	    case 'o':
		if (++i < argc)
		    strcpy(outfilename, argv[i]);
		break;

	    case 'f':
		if (++i < argc)
		    strcpy(filename, argv[i]);
		break;

	    case 'd':
		testmask |= DV_MASK;
		
		break;

	    case 'm':
		if (++i < argc)
		    max_timeouts = strtol(argv[i], 0, 0);
		break;

	    case 't':
		if (++i < argc)
		    timeout = strtol(argv[i], 0, 0);
		break;

	    case 's':
		if (++i < argc)
		    bufsize = strtol(argv[i], 0, 0);
		break;

	    case 'b':
		load = 1;
		break;

	    case 'F':
		if (++i < argc)
		    pixel_clock = atof(argv[i]);
		break;

	    case 'v':
		verbose = 1;
		break;

	    case 'q':
		quiet = 1;
		break;

	    case 'u':
		if (++i < argc)
		    unit = strtol(argv[i], 0, 0);
		break;

        case 'h':
        case 'H':
        case '?':
            usage(argv[0], "");
            exit(0);

	    default:
            usage(argv[0], "Error: unknown option\n");
		    exit(0);
	    }
	}
    }

    if (filename && filename[0])
    {

	if (strcmp(filename, "stdin") == 0)
	    f = stdin;
	else
	    f = fopen(filename, "rb");
    }
    else
    {
	pdv_p = pdv_open_channel("pdv", unit, channel);

	/* TODO: When bug #132 is fixed (so camlkla bitfile is ported to
	 * PCIe), use edt_is_dvcl(); until then we only support the
	 * PDVCL_ID. */
	/* if (! edt_is_dvcl(pdv_p)) */
	if (pdv_p->devid != PDVCL_ID)
	{
	    char errstr[128];

	    sprintf(errstr, "exiting cl_logic (does not support this device type)\n");
	    edt_msg(EDTAPP_MSG_FATAL, errstr);
	    return (1);
	}

    }

    cl_logic_summary_init(&cls, testmask, numbufs, bufsize, 0, pixel_clock);


    retval = pdv_cl_logic_sample(pdv_p, f, &cls, verbose, quiet, load, 
	outfilename, loops, timeout, max_timeouts);
    if (retval != 0) {
	return retval;
    }	


    if (pdv_p)
    {
	got_timeouts = edt_timeouts(pdv_p);
    }
    else
	got_timeouts = 0;
    if (outfile)
	fclose(outfile);

    /* print summary */
    if (filename[0])
	fclose(f);

    else if (pdv_p)
    {
	pdv_close(pdv_p);
    }

   if (got_timeouts >= 5)
   {
	printf("Failed with %d %d msec timeouts.\n", got_timeouts, timeout);
	printf("Make sure camera is freerunning, or if triggered\n");
	printf("has a trigger pulse with < 2 sec. period.\n");
	printf("Also see -m and -t options.\n");
   }
   else
   {   

	printf("\nPixel clock speed is %f Hz\n",
	    cls.pixel_clock);


	if (cls.totalframeclocks.mean)
	    printf("\n\nFrame rate : %f fps\n\n", (double) cls.pixel_clock / (double) cls.totalframeclocks.mean);

	printf(" Frame Data\n");
	printf("      Width Height HBlank VBlank\n");
	printf("                  (pixels) (lines)\n\n");

	for (i=0;i<cls.nframes;i++)
	{
	    printf("%4d: %6d %6d %6d %6d\n",
		i, cls.frames[i].width, cls.frames[i].height, cls.frames[i].line_blank, cls.frames[i].frame_blank);
	}

	cl_logic_stat_print("\nFrame Clocks", &cls.frameclocks, cls.pixel_clock, verbose);
	cl_logic_stat_print("Total Frame Clocks", &cls.totalframeclocks, cls.pixel_clock, verbose);

	cl_logic_stat_print("Width", &cls.width, cls.pixel_clock, verbose);
	cl_logic_stat_print("Height", &cls.height, cls.pixel_clock, verbose);
	cl_logic_stat_print("Horizontal Blank", &cls.hblank, cls.pixel_clock, verbose);
	cl_logic_stat_print("Total Line (Width + Blank)", &cls.totallineclocks, cls.pixel_clock, verbose);

	cl_logic_stat_print("Vertical Blank", &cls.frame_gap, cls.pixel_clock, verbose);

	printf("%-30s: %8d lines\n","Vertical Blank Lines", cls.frame_gap.mean / cls.totallineclocks.mean);

	
	if (verbose)
	{
	    cl_logic_stat_print("Start Line Gap", &cls.start_hblank, cls.pixel_clock, verbose);
	    cl_logic_stat_print("End Line Gap", &cls.end_hblank, cls.pixel_clock, verbose);
	    cl_logic_stat_print("Line Width (     0 )", &cls.line_stats[0], cls.pixel_clock, verbose);
	    last_stats = cls.line_stats[0];
	    for (i = 1; i < cls.height.high; i++)
	    {
		if (cl_logic_stats_neq(&last_stats, &cls.line_stats[i]))
		{
		    char    label[20];

		    sprintf(label, "Line Width (%6d )", i);
		    cl_logic_stat_print(label, &cls.line_stats[i], cls.pixel_clock, verbose);

		    last_stats = cls.line_stats[i];

		}
	    }
	}
   }
}
