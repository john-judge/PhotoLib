/*
 * watchstat
 *
 * diagnostic program that monitors the status of the EDT PCI DV
 * registsters. 
 *
 * (C) 1997-2001 Engineering Design Team, Inc.
 *
 */

#include "edtinc.h"
#include "edtreg.h"

#ifdef __sun
#include <sys/filio.h>
#include <signal.h>

void
gotsig(int sig)
{
    puts("INTERRUPT");
    system("stty echo icanon min 4");
    exit(0);
}
#endif



int     x;

/*
 * report status reports changes in status register
 */
void
report_status(u_int stat)
{
    if (stat & PDV_OVERRUN)
	printf("\tOVERRUN ");
    if (stat & PDV_FRAME_VALID)
	printf("\tFRAME_VALID ");
    if (stat & PDV_EXPOSURE)
	printf("\tEXPOSURE ");
    if (stat & PDV_GRAB_ARMED)
	printf("\tGRAB_ARMED ");
    if (stat & PDV_AQUIRE_IP)
	printf("\tAQUIRE_IP ");
    if (stat & PDV_SW_ARMED)
	printf("\tSW_ARMED ");
    printf("\n");
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
int
watchstat(char *command_line)
#else
int
main(int argc, char **argv)
   
#endif
{

    EdtDev *edt_p;
    u_int   curaddr;
    u_int   sgaddr;
    u_int   last_curaddr = 0;
    u_int   last_sgaddr = 0;
    u_int   last_stat = 0;
    u_int   last_count = 0;
    int     last_done = 0;
    u_int   last_bytecount = 0;
    u_int   last_frmcnt = 0 ;
    u_int   frmcnt ;
    u_int   curcount;
    int     done;
    u_int   stat;
    int     unit = 0;
    int     channel = 0;
    int     terse = 0;
    int     fv = 0;
    int     mssleep = 10;
    char   *progname;
    u_int   bytecount;
    u_int   dtmp;

#ifdef __sun
    u_char  serial_stat;
    u_char  serial_cntl;
#endif
#ifdef NO_MAIN
    char **argv  = 0 ;
    int argc = 0 ;
    opt_create_argv("watchstat",command_line,&argc,&argv);
#endif

    progname = argv[0];

    if (argc < 2)
    {
	puts("Usage: watchstat [-u unit] [-s msleep]\n");
	exit(1);
    }

    --argc;
    ++argv;
    while (argc && argv[0][0] == '-')
    {
	switch (argv[0][1])
	{
	case 'u':
	    ++argv;
	    --argc;
	    unit = atoi(argv[0]);
	    break;

	case 'C':
	    ++argv;
	    --argc;
	    channel = atoi(argv[0]);
	    break;

	case 's':
	    ++argv;
	    --argc;
	    mssleep = atoi(argv[0]);
	    break;

	case 't':
	    terse = 1;
	    break;

	case 'f':
	    fv = 1;
	    break;

	default:
	    puts("Usage: watchstat [-u unit] [-C unit] [-s msleep]\n");
	    exit(0);
	}
	argc--;
	argv++;
    }
    printf("unit %d channel %d mssleep %d\n", unit, channel, mssleep);


    if ((edt_p = edt_open_channel(EDT_INTERFACE, unit, channel)) == NULL)
    {
	char    str[64];

	sprintf(str, "%s%d", EDT_INTERFACE, unit);
	perror(str);
	exit(1);
    }


#ifdef __sun
    sigset(SIGINT, gotsig);
    system("stty -echo -icanon min 1");
#endif

    /* fv to record status without printing */
    if (fv)
    {
	int svstat[256] ;
	double stattm[256] ;
	int ix ;
	u_int laststat ;
	stat = edt_reg_read(edt_p, PDV_STAT);
	edt_dtime();
	for (ix = 0 ; ix < 256 ; ix++)
	{
	    laststat = stat ;
	    svstat[ix] = stat ;
	    stattm[ix] = edt_dtime();
	    while (laststat == stat)
		stat = edt_reg_read(edt_p, PDV_STAT) ;
	}
	for (ix = 0 ; ix < 256 ; ix++)
	{
	    printf("%.4fms\t%04x",stattm[ix] * 1000.0,svstat[ix]) ;
	    report_status(svstat[ix]) ;
	}
#ifndef NO_MAIN
	exit(0);
#endif
	return(0) ;
    }
    for (;;)
    {
	stat = edt_reg_read(edt_p, PDV_STAT);
	/* stat &= ~0x38 ; */
	curcount = edt_reg_read(edt_p, EDT_DMA_CUR_CNT) & 0xffff;
	done = edt_done_count(edt_p);
	bytecount = edt_get_bytecount(edt_p);
	frmcnt = edt_intfc_read(edt_p, PDV_CL_FRMCNT);
	curaddr = edt_reg_read(edt_p, EDT_DMA_CUR_ADDR);
	sgaddr = edt_reg_read(edt_p, EDT_SG_CUR_ADDR);

	if (last_stat != stat 
	    || (!terse && last_count != bytecount)
	    || last_frmcnt != frmcnt
	    || last_bytecount != bytecount
	    || last_done != done
	    || last_curaddr != curaddr
	    || last_sgaddr != sgaddr
		)
	{
	    last_stat = stat;
	    last_frmcnt = frmcnt;
	    last_count = bytecount;
	    last_done = done;
	    last_bytecount = bytecount;
	    last_curaddr = curaddr;
	    last_sgaddr = sgaddr;

	    dtmp = (u_int) (edt_dtime() * 1000000.0);

	    printf("%8dus %6ddn st:%04x fc:%04x bc:%08x cc:%04x da:%08x sga:%08x\n\t",
		   dtmp, done, stat, frmcnt, bytecount, curcount, curaddr, sgaddr);
	    report_status(stat);
	}

#ifdef __sun
	{
	int count ;
	/* Command processing code */
	ioctl(0, FIONREAD, &count);
	if (count)
	{
	    int     ch, arg;
	    char    s[32];

	    ch = getchar();
	    if (ch == ':')
	    {
		fputs(": ", stdout);
		fflush(stdout);

		ch = getchar();
		putchar(ch);
		fflush(stdout);

		system("stty echo");
		switch (ch)
		{
		case 'l':
		    gets(s);
		    sscanf(s, "%x", &arg);
		    edt_intfc_write(edt_p, PDV_DATA_LSB, arg);
		    printf("PDV_DATA_LSB set to 0x%x\n", arg);
		    break;

		case 'h':
		case 'm':
		    gets(s);
		    sscanf(s, "%x", &arg);
		    edt_intfc_write(edt_p, PDV_DATA_MSB, arg);
		    printf("PDV_DATA_MSB set to 0x%x\n", arg);
		    break;

		case 'c':
		    gets(s);
		    sscanf(s, "%x", &arg);
		    edt_intfc_write(edt_p, PDV_SERIAL_DATA_CNTL, arg);
		    printf("PDV_SERIAL_DATA_CNTL set to 0x%x\n", arg);
		    break;

		case 'q':
		    system("stty echo icanon min 4");
		    exit(0);
		    break;
		case '\r':
		    break;
		}
		system("stty -echo");
	    }
	    else
		putchar('\n');
	    /* Make sure new line is output */
	    serial_stat = ~serial_stat;
	    serial_cntl = ~serial_cntl;
	}
	}
#endif
	if (mssleep)
	    edt_msleep(mssleep);
    }
    /* NOTREACHED */
    return (0);
}
