/* #pragma ident "@(#)xtest.c	1.49 07/02/08 EDT" */

#include "edtinc.h"
#include <stdlib.h>
#include <string.h>
#ifndef _NT_
#include <unistd.h>
#include <signal.h>
#endif

#ifdef __linux__
/* hmmm... couldn't find swab in string.h or unistd.h, or anywhere for that matter.
 * including this so that gcc compiles without errors, may be removed if it causes more. 
 * does cause problems under nt, so putting in ifdef */
extern void swab (__const void *__restrict __from, void *__restrict __to,
                  ssize_t __n) __THROW;
#endif


static EdtDev *edt_p;
static int     ret;
static int     count;
static int     doswap = 0;
static int     interactive = 1;
static int     backio = 0;
static int     dowordswap = 0;
static int     dobig = 0;
static int     doinvert = 0;
static int     dowrite = 0;
static int     do_channel = 0;
static int     bitload = 1;
static int     verbose = 0;
static int     numbufs = 4;
static u_short *buf_p;
static u_int   channel;
static char    bitdir[256] = "";
static char    dir_arg[256] = "";
static int     kernel_buffers = 0;

/*
 * rotate a one then a zero through  unsigned short write offset, readoffset,
 * error message string flash offset (address that a write won't hurt)
 */
static int
test_short(EdtDev * edt_p, int wptr, int rptr, int fptr, char *errormsg)
{
    int     i;
    unsigned short value;
    unsigned short read;
    int     err = 0;

    printf("testing %s at %x %x\n", errormsg, wptr, rptr);
    for (i = 0; i != 16; i++)
    {
	value = 0x1 << i;

	edt_intfc_write_short(edt_p, wptr, value);
	/* flash the bus high - stat is read only */
	if (fptr)
	    edt_intfc_write(edt_p, fptr, 0xff);
	edt_msleep(1);
	read = edt_intfc_read_short(edt_p, rptr);
	if (read != value)
	{
	    err++;
	    printf("ERROR - %s r offset %02x  w/r, was %04x s/b %04x\n",
		   errormsg, rptr, read, value);
	    if (interactive)
	    {
		printf("return to continue ");
		fflush(stdout);
		getchar();
	    }
	}
    }
    for (i = 0; i != 16; i++)
    {
	value = (~(0x1 << i));

	edt_intfc_write_short(edt_p, wptr, value);
	/* flash the bus high - stat is read only */
	if (fptr)
	    edt_intfc_write(edt_p, fptr, 0xff);
	edt_msleep(1);
	read = edt_intfc_read_short(edt_p, rptr);
	if (read != value)
	{
	    err++;
	    printf("ERROR - %s r offset %02x  w/r, was %04x s/b %04x\n",
		   errormsg, rptr, read, value);
	}
    }
    return (err);
}

#ifdef IS_UNUSED
static void 
gotit(int sig)
{
#if 1
    printf("gotit\n");
    return;
#else
    FILE   *outfile;

    outfile = fopen("/tmp/partial", "wb");
    fwrite(buf_p, 1, count * 2, outfile);
    fclose(outfile);
    edt_close(edt_p);
    exit(1);
#endif
}
#endif

static void
usage()
{
    puts("xtest -- EDT PCI interface test/diagnostic program");
    puts("usage: xtest [optional args] size");
    puts("-u N     sets unit number - default is 0");
    puts("-d file  datafile specifies output file for data");
    puts("-D dir   sets alternate directory to search for xtest.bit file");
    puts("-n bufs  number of ring buffers to use (default 4)");
    puts("-b       32 bit mode");
    puts("-i       inverts data");
    puts("-s       byteswaps data");
    puts("-w       wordswaps data");
    puts("-W       tests speed with write");
    puts("-N       don't download bitfile xtest.bit");
    puts("-a       non-interactive (runs straight through with no waits)");
    puts("size is  the size of read in words (16 bits)");
}

int     loopcount = 100;
int     testloop = 1;
int
testspeed(EdtDev * edt_p, int unit);
char    filename[100];
char    resp[50];

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
xtest(char *command_line)
#else
int
main(int argc, char **argv)
#endif
{
    int     i;
    FILE   *outfile;
    FILE   *outcheck;
    char    checkname[100];
    char    edt_devname[100];
    char   *unitstr = "0";
    char    cmdstr[100];
    int     unit = 0;
    u_short cmd;
    u_short chkval;
    int     errcnt;
    int     tmpcount;
    int     loop;
    int     cfg;
    int     disable_burst_en = 0;
#ifdef NO_MAIN
    char **argv  = 0 ;
    int argc = 0 ;
    opt_create_argv("xtest",command_line,&argc,&argv);
#endif

    /*
     * set default options
     */
    outfile = 0;
    outcheck = 0;
#ifndef _NT_
    /* signal(SIGINT,gotit) ; */
#endif

    --argc;
    ++argv;
    while (argc && argv[0][0] == '-')
    {
	switch (argv[0][1])
	{
	case 'i':
	    doinvert = 1;
	    break;

	case 'W':
	    dowrite = 1;
	    break;

	case 's':
	    doswap = 1;
	    break;

	case 'w':
	    dowordswap = 1;
	    break;

	case 'b':
	    dobig = 1;
	    break;
	    
	case 'B':
	    disable_burst_en = 1;
	    break;

	case 'v':
	    verbose = 1;
	    break;

	case 'N':
	    bitload = 0;
	    break;

	case 'a':
	    interactive = 0;
	    break;

	/* pmc board enable p2 io */
	case 'p':
	    backio = 1;
	    break;

	case 'd':
	    ++argv;
	    --argc;
	    strcpy(filename, argv[0]);
	    if ((outfile = fopen(filename, "wb")) == NULL)
	    {
		perror(filename);
		exit(1);
	    }
	    break;

	case 'u':
	    ++argv;
	    --argc;
	    unit = atoi(argv[0]);
	    break;

	case 'c':
	    ++argv;
	    --argc;
	    channel = atoi(argv[0]);
	    do_channel = 1;
	    bitload = 0;
	    break;

	case 'l':
	    ++argv;
	    --argc;
	    loopcount = atoi(argv[0]);
	    break;

	case 't':
	    ++argv;
	    --argc;
	    testloop = atoi(argv[0]);
	    break;

	case 'n':
	    ++argv;
	    --argc;
	    numbufs = atoi(argv[0]);
	    break;

	case 'D':
	    ++argv;
	    --argc;
	    strcpy(bitdir, argv[0]);
	    break;

	case 'k':
	  kernel_buffers = 1;
	  break;


	default:
	    usage();
	    exit(0);
	}
	argc--;
	argv++;
    }

    if (argc < 1)
    {
	usage();
	exit(0);
    }
    count = atoi(argv[0]);


 	if ((edt_p = edt_open_channel(EDT_INTERFACE, unit, channel)) == NULL)
	{
	    edt_perror("edt_open()");
	    if (interactive)
	    {
		puts("Hit return to exit");
		fflush(stdout);
		getchar();
	    }
	    return (1);
	}

	switch (edt_p->devid)
	  {
	  case PDV_ID:
	  case PDVK_ID:
	  case PDV44_ID:
	  case PDVA_ID:
	  case PDVFOX_ID:
	  case PE4DVFOX_ID:
	  case PE8DVFOX_ID:
	  case PC104ICB_ID:
	  case PDVA16_ID:
	  case PDVFCI_AIAG_ID:
	  case PDVFCI_USPS_ID:
	  case PCD20_ID:
	  case PCD40_ID:
	  case PCD60_ID:
	  case PCDA_ID:
		break;

	  default:
		fprintf(stderr,"This edt device is not suitable for testing with xtest, which works with the\n");
		fprintf(stderr,"PCI CD20, PCI CD40,PCI  CD60, PCI DV, PCI DVK, and PCI DV44\n");
		return 1;

	
	  }


    if ((*bitdir) && edt_access(bitdir, 0) == 0)
	sprintf(dir_arg, "-b %s", bitdir);
    else if (strcmp(edt_devname, "pdv") == 0)
	strcpy(dir_arg, "-d camera_config");

    /*
     * in case pdv or other unit type was specified, use the '-u devN' format
     * for bitload
     */
    if (bitload)
    {
#ifdef _NT_
	sprintf(cmdstr, ".\\bitload -u %d %s %s xtest.bit",
			    unit, verbose? "-v":"", dir_arg);
#else
	sprintf(cmdstr, "./bitload -u %d %s %s xtest.bit",
			    unit, verbose? "-v":"", dir_arg);
#endif

	if (verbose)
	    puts(cmdstr);

	system(cmdstr);
    }

    if (doinvert || doswap)
    {
	strcpy(checkname, argv[0]);
	strcat(checkname, ".chk");
	if ((outcheck = fopen(checkname, "wb")) == NULL)
	{
	    perror(checkname);
	    exit(1);
	}
	printf("output check to %s\n", checkname);
    }

    
	printf("reading %d words\n", count);

    if (outfile)
	printf("output to %s\n", filename);

    buf_p = (u_short *) edt_alloc(count * 2);
    if (buf_p == NULL)
    {
	perror("Data buffer malloc error");
	exit(1);
    }
    printf("buf at %p\n", buf_p);

	if (kernel_buffers)
	{

		int i = 1;
		edt_ioctl(edt_p, EDTS_DRV_BUFFER, &i);

	}


    edt_set_autodir(edt_p, 1);
    
    edt_set_burst_enable(edt_p,!disable_burst_en);
	
    errcnt = 0;
    for (loop = 0; loop < testloop || testloop == 0; loop++)
    {
	cmd = 0;
	edt_reg_write(edt_p,PCD_CMD,0);

	if (backio)
		edt_reg_or(edt_p, PCD_CMD, XTEST_EN_BACK_IO);
	else
		edt_reg_and(edt_p,PCD_CMD,~XTEST_EN_BACK_IO);

	if (edt_p->devid == PDVK_ID
	 || edt_p->devid == PDV44_ID
	 || edt_p->devid == PDVCL_ID
	 || edt_p->devid == PE4DVCL_ID
	 || edt_p->devid == PE8DVCL_ID
	 || edt_p->devid == PC104ICB_ID
	 || edt_p->devid == PDVFOI_ID)
	    printf("Skipping looparound for %s\n", edt_idstr(edt_p->devid));
	else
	{
	edt_intfc_write_short(edt_p, XTEST_DIRREG, 0xcfff);
	test_short(edt_p, XTEST_DIRREG, XTEST_DIRREG, XTEST_NOTUSED, "dirreg");
	test_short(edt_p, XTEST_RDDO, XTEST_RDDO, XTEST_NOTUSED, "dirout");
	test_short(edt_p, XTEST_RDDO, XTEST_RDDI, XTEST_NOTUSED, "dirin");

	/* enable control out 12-15 and control out 8-11 */
	edt_intfc_write_short(edt_p, XTEST_DIRREG, 0xcfff);

	test_short(edt_p, XTEST_RDCO, XTEST_RDCO, XTEST_NOTUSED, "ctlout");
	test_short(edt_p, XTEST_RDCO, XTEST_RDCI, XTEST_NOTUSED, "ctlin");
	}

	/* reset interface fifo and dma fifo */
	edt_ioctl(edt_p, EDTG_PROG, &cfg);
	cfg &= ~EDT_RFIFO_ENB;
	edt_ioctl(edt_p, EDTS_PROG, &cfg);
	cfg |= EDT_RFIFO_ENB;
	edt_ioctl(edt_p, EDTS_PROG, &cfg);


	/* Reset the fifos to clear out old data */
	cmd &= ~XTEST_UN_RESET_FIFO ;
	edt_intfc_write_short(edt_p, XTEST_CMD, cmd);
	cmd |= XTEST_UN_RESET_FIFO ;
	edt_intfc_write_short(edt_p, XTEST_CMD, cmd);

	edt_ioctl(edt_p, EDTG_PROG, &cfg);
	cfg &= ~EDT_RFIFO_ENB;
	edt_ioctl(edt_p, EDTS_PROG, &cfg);


	cmd &= ~XTEST_UN_RESET_FIFO ;
	/* enable data generation */
#ifdef __hpux
	if (!doswap)
	    cmd |= XTEST_SWAPBYTES;
#else
	if (doswap)
	    cmd |= XTEST_SWAPBYTES;
#endif
	if (dowordswap)
	    cmd |= XTEST_SWAPWORDS;
	if (doinvert)
	    cmd |= XTEST_INVERT;
	if (dobig)
	    cmd |= XTEST_BIT32;
	if (dowrite)
	    cmd |= XTEST_DEV_READ;
	edt_intfc_write_short(edt_p, XTEST_CMD, cmd);
	cmd |= XTEST_UN_RESET_FIFO;
	edt_intfc_write_short(edt_p, XTEST_CMD, cmd);
	cmd |= XTEST_EN_DATA;
	edt_intfc_write_short(edt_p, XTEST_CMD, cmd);

	if (doinvert)
	    chkval = 0xffff;
	else
	    chkval = 0;

	if (testloop)
	{
	    if (dowrite)
		printf("return to write ");
	    else
		printf("return to read ");
	    if (dowrite)
		printf("DEVREAD ");
	    if (doswap)
		printf("BYTESWAP ");
	    if (dowordswap)
		printf("WORDSWAP ");
	    if (doinvert)
		printf("INVERT ");
	    if (dobig)
		printf("BIT32 ");
	    printf(": ");
	    fflush(stdout);
	    if (interactive)
		getchar();
	}

	if (dowrite)
	{
	    if ((ret = edt_write(edt_p, (u_char *) buf_p, count*2)) != count*2)
		edt_perror("edt_write");
	}
	else
	{
	    if ((ret = edt_read(edt_p, (u_char *) buf_p, count*2)) != count*2)
		edt_perror("edt_read");
	}
	printf("Done.\n");
	if (ret > 0)
	{
	    for (i = 0; i < 8; i++)
	    {
		printf("%04x ", buf_p[i]);
	    }
	    printf("\n");
	    if (outfile)
	    {
		printf("saving file %s\n", filename);
		fwrite(buf_p, 1, ret, outfile);
		fclose(outfile);
		outfile = 0;
	    }
	    if (dowordswap)
	    {
		printf("swapping words to check -w\n");
		{
		    u_short *tmp_p;
		    u_short tmp1;
		    u_short tmp2;

		    tmpcount = count / 2;
		    tmp_p = buf_p;
		    while (tmpcount--)
		    {
			tmp1 = *tmp_p;
			tmp2 = *(tmp_p + 1);
			*tmp_p = tmp2;
			*(tmp_p + 1) = tmp1;
			tmp_p += 2;
		    }
		}
	    }
	    if (doinvert)
	    {
		printf("inverting data to check -i\n");
		for (i = 0; i < count; i++)
		{
		    printf("%x -> ", buf_p[i]);
		    buf_p[i] ^= 0xffff;
		    printf("%x\n", buf_p[i]);
		}
	    }
	    if (doswap)
	    {
		printf("swapping bytes to check -s\n");
		swab((char *) buf_p, (char *) buf_p, count * 2);
	    }
	    if (dobig)
	    {
		printf("skipping words to check -b\n");
		{
		    u_short *tmp_p;
		    u_short *tmp_p2;

		    tmpcount = count / 2;
		    tmp_p = buf_p;
		    tmp_p2 = buf_p;
		    for (i = 0; i < 4; i++) {
				 if (*tmp_p2 == 0x1 || *tmp_p2 == 0x100) {
					 printf("found %04x at word %d\n",
							  *tmp_p2, i);
					 break;
				 }
				 /* *tmp_p2++; should this be (*tmp_p2)++ or *(tmp_p2++)? I think the compiler is doing the second */
				 tmp_p2++;
		    }
		    if (i == 4) {
				 printf("can't find start of data\n");
		    } else {
				 if (i >= 2)
					 tmp_p2 -= 2;
				 while (tmpcount--) {
					 *tmp_p++ = *tmp_p2;
					 tmp_p2 += 2;
				 }
		    }
		}
	    }
	    if (buf_p[1] == 0x100)
	    {
		printf("Notice: need to swap bytes ");
		if (!doswap)
		    printf(" (big endian)\n");
		else
		    printf(" (little endian)\n");
		swab((char *) buf_p, (char *) buf_p, count * 2);
	    }
	    printf("checking data\n");
	    resp[0] = 0;
	    if (dobig)
		tmpcount = count / 2;
	    else
		tmpcount = count;
	    for (i = 0; i < tmpcount; i++)
	    {
		if (buf_p[i] != chkval)
		{
		    if (resp[0] == 'y' || errcnt < 10)
		    {
			printf("ERROR - word %d (byte 0x%x) was %x, should be %x\n",
			       i, i, buf_p[i], chkval);
		    }
		    if (errcnt == 10)
		    {
			if (interactive)
			{
			    printf("continue to print errors? ");
			    fgets(resp,49,stdin);
			}
			else puts("10+ errors, skipping the rest\n");
		    }
		    chkval = buf_p[i];
		    errcnt++;
		}
		if (doinvert)
		    chkval--;
		else
		    chkval++;
	    }
	    printf("%d words %d errors loop %d\n", tmpcount, errcnt, loop);
	}

    }


    edt_free((u_char *) buf_p);


    if (interactive)
    {
	printf("start speedtest? : ");
	fgets(resp,49,stdin);
	if (resp[0] != 'y')
	    return (0);
    }

    testspeed(edt_p, unit);
    edt_close(edt_p);

    if (interactive)
    {
	puts("Hit return to exit");
	fflush(stdout);
	getchar();
    }
    return (0);
}


int
testspeed(EdtDev * edt_p, int unit)
{
    int     size;
    int     loop;
    double  dtime;
    double  tmpcount;
    int     dir;
	int rc;

    /*
     * set defaults
     */
    /* default to 100 reads of 1 Megabyte */
    size = 1024 * 1024;
    if (dowrite)
	dir = EDT_WRITE;
    else
	dir = EDT_READ;

    /* Construct a ring buffer */
	rc = edt_configure_ring_buffers(edt_p, size, numbufs, dir, 0);

	if (rc == -1)
    {
	edt_perror("edt_configure_ring_buffers");
	return (1);
    }


    if (dowrite)
	printf("reading %d buffers of %d bytes from unit %d with %d bufs\n",
	       loopcount, size, unit, numbufs);
    else
	printf("reading %d buffers of %d bytes from unit %d with %d bufs\n",
	       loopcount, size, unit, numbufs);
    fputs("return to start: ", stdout);
    fflush(stdout);
    if (interactive)
	getchar();

    /* queue the first reads */
    edt_start_buffers(edt_p, numbufs);


    (void) edt_dtime();

    for (loop = 0; loop < loopcount; loop++)
    {
	u_short *tmp_p;

	tmp_p = (u_short *) edt_wait_for_buffers(edt_p, 1);
	if (verbose)
	{
	    int tmpx = size / 2 ;
	    printf("%p:%04x%04x%04x%04x%04x%04x%04x%04x\n",
		   tmp_p,
		   tmp_p[0], tmp_p[1], tmp_p[2], tmp_p[3],
		   tmp_p[tmpx-4], tmp_p[tmpx-3], tmp_p[tmpx-2], tmp_p[tmpx-1]) ;
	    tmp_p[0] = 0x1111 ;
	    tmp_p[1] = 0x2222 ;
	    tmp_p[2] = 0x3333 ;
	    tmp_p[3] = 0x4444 ;
	}
	else
	{
	    putchar('.');
	    fflush(stdout);
	}

	if (loop < loopcount - numbufs)
	    edt_start_buffers(edt_p, 1);
    }

    dtime = edt_dtime();
    tmpcount = loopcount;
    tmpcount *= (double) size;
    printf("\n%f bytes/sec\n", tmpcount / dtime);
    printf("time %f\n", dtime);
	return (0);
}
