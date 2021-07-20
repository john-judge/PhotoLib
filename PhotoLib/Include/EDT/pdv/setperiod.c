
#include "edtinc.h"

static void usage(char *progname);

/*
 * Main module. NO_MAIN is typically only defined when compiling for vxworks; if you
 * want to use this code outside of a main module in any other OS, just copy the code
 * and modify it to work as a standalone subroutine, including adding parameters in
 * place of the command line arguments
 */
#ifdef NO_MAIN
#include "opt_util.h"
char *argument ;
int option ;
int
setperiod(char *command_line)
#else
int
main(int argc, char **argv)
#endif
{
    int     unit = 0;

    char    bmpfname[128];
    PdvDev *pdv_p;
    char    errstr[64];
    int period = 0;
    char edt_devname[128];
    int channel = 0;
#ifdef NO_MAIN
    char **argv  = 0 ;
    int argc = 0 ;
    char   *progname = argv[0];
    opt_create_argv("setperiod",command_line,&argc,&argv);
#else
    char   *progname = argv[0];
#endif
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
	    if (argc < 1) 
        {
		    puts("Error: option 'u' requires a numerical argument");
            usage(progname);
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
		    puts("Error: option 'c' requires a numerical argument");
            usage(progname);
	    }
        if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
			channel = atoi(argv[0]);
        else
        {
            puts("Error: option 'c' requires a numerical argument");
            usage(progname);
        }
	    break;


	  case '?':
	  case 'h':
	    usage(progname);
	    exit(0);

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
	}
	argc--;
	argv++;
    }

	if (argc) {
		period = atoi(argv[0]);
	}

    if (argc == 0 || period == 0)
	{	
	usage(progname);
	exit(1);
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

	pdv_set_frame_period(pdv_p, period, PDV_FVAL_ADJUST);

    pdv_close(pdv_p);

#ifndef NO_MAIN
    exit(0);
#endif
    return (0);
}

void
usage(char *progname)
{
    printf("usage: %s [-u unit] [-c channel] <period in microseconds>\n", progname);
    exit(1);
}
