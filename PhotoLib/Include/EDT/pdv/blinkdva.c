/*
 * blink_dva.c
 * 
 * Blinks the dva's LED. That's all.
 * 
 * (C) 2003 Engineering Design Team, Inc.
 */
#include "edtinc.h"

static void usage(char *progname);
static void 
save_image(u_char * image_p, int width, int height, int depth,
	   char *basename, int count);

/*
 * main
 */
main(int argc, char **argv)
{
    int     i, u2;
    int     unit = 0;
    int    loops = 10;
    char   *progname = argv[0];
    char    errstr[64];
    char    edt_devname[128];
    PdvDev *pdv_p;

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

    if (argc && (argv[0][0] >= '0') && (argv[0][0] <= '9'))
    	loops = atoi(argv[0]);
    	

    /*
     * open the interface
     */
    if (edt_devname[0])
    {
	unit = edt_parse_unit(edt_devname, edt_devname, EDT_INTERFACE);
    }
    else
    {
	strcpy(edt_devname, EDT_INTERFACE);
    }

    if ((pdv_p = pdv_open(edt_devname, unit)) == NULL)
    {
	sprintf(errstr, "pdv_open(%s%d)", edt_devname, unit);
	pdv_perror(errstr);
	return (1);
    }
    if (pdv_p->devid != PDVA_ID)
    {
    	printf("%s%d is not a PCI DVa!\n", edt_devname, unit);
	exit(1);
    }

    u2 = edt_reg_read(pdv_p, PDV_UTIL2);

    for (i=0; i<loops; i++)
    {
	edt_reg_write(pdv_p, PDV_UTIL2, (u2 | 0x80) & ~0x40); /* OFF */
	edt_msleep(100);
	edt_reg_write(pdv_p, PDV_UTIL2, (u2 | 0x40) & ~0x80); /* ON */
	edt_msleep(100);
    }
    edt_reg_write(pdv_p, PDV_UTIL2, u2);

    pdv_close(pdv_p);

    exit(0);
}


void
usage(char *progname)
{
    puts("");
    printf("%s: blink the dva's LED (generally to identify which board", progname);
    printf("is which in multiple-board installations)\n");
    puts("");
    printf("usage: %s [-u unit] [count]\n", progname);
    printf("  -u unit         %s unit number (default 0)\n", EDT_INTERFACE);
    printf("  count           number of blinks (default 10)\n");
}
