/*

*/

#include "edtinc.h"

#include "pdv_recplay.h"

static void
usage(char *progname)
{
    puts("");
    printf("%s: simple example program that acquires images from an\n", progname);
    printf("EDT digital imaging interface board (PCI DV, PCI DVK, etc.)\n\
	   and saves frames to a contiguous raw file\n");
    puts("");
    printf("usage: %s\n", progname);
    printf("  -f fname 	      output to raw data file\n");
    printf("  -l nimages      number of images to take\n");
    printf("  -N numbufs      number of ring buffers (see users guide) (default 4)\n");
    printf("  -u unit         %s unit number (default 0)\n", EDT_INTERFACE);
    printf("  -c channel      channel number (default 0)\n");
    printf("  -m              accumulate images in memory before writing\n");
}



/*
* main
*/
main(int argc, char **argv)
{
    int     unit = 0;
    char   *progname = argv[0];
    int	    numbufs = 8;
    char    errstr[64];
    int     nimages = 100;
    EdtRecPlay *rpb;
    char basename[512];
    u_int inmemory = FALSE;

    int filetype = FILETYPE_RAW;

    char edt_devname[128];
    int channel = 0;


    edt_devname[0] = '\0';

    strcpy(basename, DEFAULT_FILENAME);

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

	case 'f':		/* bitmap save filename */
	    ++argv;
	    --argc;
	    strcpy(basename, argv[0]);
	    break;

	case 'l':
	    ++argv;
	    --argc;
	    nimages = atoi(argv[0]);
	    break;

	case 'm':
	    inmemory = TRUE;
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

    if ((rpb = edt_record_playback_open(NULL,edt_devname, unit, channel, EDT_READ, NULL)) == NULL)
    {
	sprintf(errstr, "pdv_open_channel(%s%d_%d)", edt_devname, unit, channel);
	pdv_perror(errstr);
	return (1);
    }

    rpb->numbufs = numbufs;

    if (edt_record_init(rpb, basename, inmemory, nimages) == 0)
    {
        edt_record(rpb);
    }

    edt_record_playback_close(rpb);

    exit(0);
}

