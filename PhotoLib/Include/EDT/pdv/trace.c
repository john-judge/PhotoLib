/* #pragma ident "@(#)setdebug.c	1.115 03/21/06 EDT" */

#include "edtinc.h"
#include <stdlib.h>

#include "edt_trace.h"

EdtDev *edt_p;


struct {
    char *label;
    u_int value;
} trace_names[] = {
    {"READ_GRAB",TR_READ_GRAB},
    {"ISR_START",TR_ISR_START},
    {"ISR_END",TR_ISR_END},
    {"DPC_START",TR_DPC_START},
    {"DPC_END",TR_DPC_END},
    {"START_BUF",TR_START_BUF},
    {"DOGRAB",TR_DOGRAB},
    {"WAIT_START",TR_WAIT_START},
    {"NEED_WAIT",TR_NEED_WAIT},
    {"WAIT_END",TR_WAIT_END},
    {"LOADED",TR_LOADED},
    {"DONE",TR_DONE},
    {"SERIAL",TR_SERIAL},
    {"TIMEOUT",TR_TIMEOUT},
    {"SET",TR_SET},
    {"SER_READ",TR_SER_READ},
    {"SER_WRITE",TR_SER_WRITE},
    {"SER_READ_END",TR_SER_READ_END},
    {"SER_WRITE_END",TR_SER_WRITE_END},
    {"ISRGRAB",TR_ISRGRAB},
    {"DEVINT",TR_DEVINT},
    {"SNDMSG",TR_SNDMSG},
    {"TMSTAMP",TR_TMSTAMP},
    {"P11W_ATTNINT",TR_P11W_ATTNINT},
    {"P11W_CNTINT",TR_P11W_CNTINT},
    {"TR_ENABLE_INT",TR_ENABLE_INT},
    {"WAKEUP",TR_WAKEUP},
    {"LOCK",TR_LOCK},
    {"UNLOCK",TR_UNLOCK},
    {"ENDCLR",TR_ENDCLR},
    {"STARTCLR",TR_STARTCLR},
    {"FLUSH",TR_FLUSH},
    {"SETEVENT",TR_SETEVENT},
    {"EVENTSLP",TR_EVENTSLP},
    {"EVENTWK",TR_EVENTWK},
    {"REFTMSTAMP",TR_REFTMSTAMP},
    {"SETBUF",TR_SETBUF},
    {"SERWAIT",TR_SERWAIT},
    {"SEREND",TR_SEREND},
    {"CONTSTATE",TR_CONTSTATE},
    {"STATE_CHANGE",TR_STATE_CHANGE},
    {"DISABLE_INT",TR_DISABLE_INT},
    {"CLEANUP",TR_CLEANUP},
    {"ABORT",TR_ABORT},
    {"EVENTWARN",TR_EVENTWARN},
    {"NOT_OUR_INTR",TR_NOT_OUR_INTR},
    {"NO_ACTION",TR_NO_ACTION},
    {"CHECK_SG_NXT",TR_CHECK_SG_NXT},
    {"ISR_START_RT",TR_ISR_START_RT},
    {"ISR_END_RT",TR_ISR_END_RT},
    {"RT_INTR_START",TR_RT_INTR_START},
    {"RT_INTR_END",TR_RT_INTR_END},
    {"REG_SET",TR_REG_SET},
    {"REG_GET",TR_REG_GET},
    {"LOAD_SG",TR_LOAD_SG},
    {0,0}
};

void
usage()
{
      fprintf(stderr, "\t-u <unit>    - unit number \n");
      fprintf(stderr, "\t-c <channel> - channel number \n");
      fprintf(stderr, "\t-G           - print driver trace buffer and loop waiting for updates \n");
      fprintf(stderr, "\t-r           - raw mode: output binary representation of driver  \n");
      fprintf(stderr, "\t                 trace buffer\n");
      fprintf(stderr, "\t-o           - output to file; useful with -r \n");
      fprintf(stderr, "\t-i           - specify raw input file; use after -r\n");
      fprintf(stderr, "\t-f           - \n");
      fprintf(stderr, "\t-x           - \n");
}
u_int  lookup_trace_name(char *name)

{
    int i;
    for (i=0;trace_names[i].label;i++)
	if (strcasecmp(trace_names[i].label, name) == 0)
	    return trace_names[i].value;

    return 0;

}

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
trace(char *command_line)
#else
int
main(int     argc,char  **argv)
#endif
{
    int     unit = 0;

    int     channel = 0;
    char   *unitstr = "0";
    char    edt_devname[100];
    int     wait_trace = 0;
    int     do_raw = 0;
    char    outfname[256];
    char    infname[256];
    FILE * outfile = NULL, 
	* infile = NULL;
    u_int *filter = NULL;

    u_int filter_store[100];
    u_int filter_count = 0;
    u_int filter_mode = FILTER_INCLUDE;

    outfname[0] = 0;
    infname[0] = 0;

    --argc;
    ++argv;
    while (argc && argv[0][0] == '-')
    {
	switch (argv[0][1])
	{
	case 'u':
	    ++argv;
	    --argc;
	    unitstr = argv[0];
	    break;

	case 'c':
	    ++argv;
	    --argc;
	    channel = atoi(argv[0]);
	    break;

	case 'G':
	    wait_trace = 1;
	    break;

	case 'r':
	    do_raw = 1;
	    break;

	case 'o':
	    ++argv;
	    --argc;
	    if (argc)
		strcpy(outfname,argv[0]);
	    break;

	case 'i':
	    ++argv;
	    --argc;
	    if (argc)
		strcpy(infname, argv[0]);
	    break;

	case 'f':
	    ++argv;
	    --argc;
	    if (argc)
	    {
		u_int v = lookup_trace_name(argv[0]);
		if (v)
		{
		    filter_store[filter_count] = v;
		    filter_count++;
		}
	    }

	    break;

	case 'x':
	    filter_mode = FILTER_EXCLUDE;
	    break;

	default:
        usage();
	    exit(0);
	}
	argc--;
	argv++;
    }

    unit = edt_parse_unit(unitstr, edt_devname, EDT_INTERFACE);
    if ((edt_p = edt_open_channel(edt_devname, unit, channel)) == NULL)
    {
	char    str[64];

	sprintf(str, "%s%d_%d", edt_devname, unit, channel);
	perror(str);
	exit(1);
    }

    if (outfname[0])
	outfile = fopen(outfname, "wb");
    else    
	outfile = stdout;

    if (infname[0])
	infile = fopen(infname, "rb");
    else    
	infile = NULL;

    if (do_raw && !outfname[0])
    {
	fprintf(stderr,"Can't do raw to stdout\n");
	exit(1);
    }

    if (filter_count)
	filter = filter_store;

    if (infile)
	edt_trace_file(infile, outfile, filter, filter_mode);
    else
	edt_trace(edt_p, outfile, do_raw, (wait_trace)?0:1, filter, filter_mode);

    return (0);
}
