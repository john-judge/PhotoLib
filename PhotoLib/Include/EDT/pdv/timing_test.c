/* #pragma ident "@(#)timing_test.c	1.17 03/01/11 EDT" */

/*
 * timing_test is a debugger for the timecode datapath.  A menu of commands
 * is printed at each prompt.
 */

/* Timing register offsets: */
/* 	Time Dist. Board:	0x60 (default) */
/* 	MSDV:			0xa4 */
/* 	Net10G:			0xac */
/* 	SRXL-2:			0x5c */
/* 	CLINK:			0xb1 */

#include "edtinc.h"
#include "libedt_timing.h"

void
usage()
{
    puts("usage: timing_test [-u <unit>] [-b <spi_reg_base>] [-s \"<str>\"] [-f <file>]") ;
    puts("    -u <unit>			- specifies edt board unit number") ;
    puts("    -b <spi_reg_base>		- specifies alternate spi register base address") ;
    puts("    -s \"<ascii string>\"	- send ascii string to UART") ;
}

int send_loopback_packet(EdtDev *edt_p, u_int N);
int verify_loopback_packet(EdtDev *edt_p, u_char *sent_buffer);

int
main(argc, argv)
int argc;
char **argv;
{

    EdtDev *edt_p;
    int unit 	= 0;
    u_int spi_reg_base = 0; 	/* 0 value specifies default spi register base of 0x60 */
    int loops	= -1;
    /*int i; */

    puts("0.2");
    while (argc > 1 && argv[1][0] == '-')
    {
        switch (argv[1][1])
        {
        case 'u':
            ++argv;
            --argc;
            unit = atoi(argv[1]);
            break ;

        case 'b':
            ++argv;
            --argc;
            spi_reg_base = strtol(argv[1], NULL, 16);
            break ;

	case 'L':
            ++argv;
            --argc;
	    loops = atoi(argv[1]) ;
            break ;

        default:
            usage() ;
            exit(1) ;
        }
        --argc ;
        ++argv ;
    }
    puts("0.1");

    if ((edt_p = edt_spi_open(EDT_INTERFACE, unit, spi_reg_base)) == NULL)
    {
        perror("edt_spi_open") ;
        exit(1) ;
    }

    if (loops >= 0)
	edt_spi_display_time(edt_p, loops);

    for (;;)
    {
	static char str[1024];
	u_char ch, ch1;

	puts("\n\nMenu:");
	puts("	r [N]		Read then strobe fifo N times (1 default, 0 forever)");
	puts("	w <hex byte>	Write value to fifo");
	puts("	W <binary byte>	Write binary byte value to fifo");
	puts("	c <ascii char>	Write ascii character to fifo");
	puts("	C <clock_sel>	Set msp430 system clock to either internal (0) or external (1) (menu driven)");
	puts("	s		Read status");
	puts("	S <ascii str>	Send ascii string to SPI (double quotes optional)");
	puts("	e		Enable 1 Hz timestamp from msp430 via SPI");
	puts("	d		Disable 1 Hz timestamp from msp430 via SPI");
	puts("	R N		Set timecode to raw BCD (N == 1) or Unix time (N == 0, default)");
	puts("	Y N		Enable programmable year and set to N > 2000 (requires v.21+)");
	puts("	y		Disable programmable year (use CONTROL FIELD of IRIG-B signal");
	puts("	t N		Retrieve and display time N times (N=0 forever)");
	puts("	f		Reset fifo");
	puts("	F		Enter msp430 flash programming mode");
	puts("	L N		Send loopback packet with 0 <= N <= 60 data bytes");
	puts("	v		Print timecode firmware version");
	puts("	q,Q		Quit");

	fputs("\nEnter command:  ", stdout);
	fflush(stdout);

	fgets(str, 1023, stdin);
	fputs(str, stdout);

	switch (*str)
	{
	    case 'r':
	    {
		int N = 1;

		sscanf(str, "r %d", &N);

		if (N)
		    ++ N;

		while (N == 0 || N-- > 1)
		    printf("				RD 0x%x\n",   edt_spi_get_byte(edt_p));
		break;
	    }

	    case 'R':
	    {
		int timecode_raw = 1;
		sscanf(str, "R %d", &timecode_raw);

		edt_set_timecode_enable(edt_p, 0);
		edt_msleep(2000);
		edt_spi_flush_fifo(edt_p);

		edt_set_timecode_raw(edt_p, timecode_raw);

		edt_set_timecode_enable(edt_p, 1);

		break;
	    }

	    case 'Y':
	    {
		int programmable_year;

		sscanf(str, "Y %d", &programmable_year);

		edt_set_timecode_enable(edt_p, 0);
		edt_msleep(2000);
		edt_spi_flush_fifo(edt_p);

		edt_enable_timecode_programmable_year(edt_p, (u_short) programmable_year);

		edt_set_timecode_enable(edt_p, 1);

		break;
	    }

	    case 'y':
	    {
		edt_set_timecode_enable(edt_p, 0);
		edt_msleep(2000);
		edt_spi_flush_fifo(edt_p);

		edt_disable_timecode_programmable_year(edt_p);

		edt_set_timecode_enable(edt_p, 1);

		break;
	    }

	    case 'w':
	    {
		int ch;

		sscanf(str, "w %2x", &ch);

		printf("				WR 0x%x\n",   edt_spi_put_byte(edt_p, ch));
		break;
	    }

	    case 'W':

		sscanf(str, "W %c%c", &ch, &ch1);

		printf("				WR 0x%x\n",   edt_spi_put_byte(edt_p, ch));
		printf("				WR 0x%x\n",   edt_spi_put_byte(edt_p, ch1));
		break;

	    case 'c':

		sscanf(str, "c %c", &ch);

		printf("				WR 0x%x\n",   edt_spi_put_byte(edt_p, ch));
		break;

	    case 'C':
	    {
		int clock_sel, clock_hz;

		sscanf(str, "C %d %d", &clock_sel, &clock_hz);
		edt_set_msp430_clock(edt_p, clock_sel, clock_hz);
		printf("edt_p %p\n", edt_p);

		break;
	    }

	    case 'e':					/* Enable 1 Hz timestamp from msp430/SPI. */
	    {
		edt_set_timecode_enable(edt_p, 1);

		break;
	    }
	    case 'd':					/* Disable 1 Hz timestamp from msp430/SPI */
	    {
		edt_set_timecode_enable(edt_p, 0);

		break;
	    }

	    case 't':					/* Capture and display timestamp for N loops (0 is forever) */
		sscanf(str, "t %d", &loops);

		edt_spi_display_time(edt_p, loops);

		break;

	    case 'f':
		edt_spi_flush_fifo(edt_p);
		puts("				Flushed.");
		break;

	    case 'F':	/* Put the msp430 into flash-loader mode (culminates in msp430 reboot). */

		printf("				WR 0x%x\n",   edt_spi_put_byte(edt_p, 0x80));
		edt_msleep(1000);
		edt_spi_reg_write(edt_p, SPI_STAT_CTRL, 0x01);	/* Reset SPI fifo */
		edt_spi_reg_write(edt_p, SPI_STAT_CTRL, 0x00);	/* Reset SPI fifo */
		edt_msleep(1000);
		printf("				WR 0x%x\n",   edt_spi_put_byte(edt_p, 0x80));
		edt_msleep(1000);
		printf("				RD 0x%x\n",   ch = edt_spi_get_byte(edt_p));
		if (ch == 0x90)
		    puts("Flash programming mode started.");
		else
		    puts("Flash programming mode faulted.  Please try again.");

		break;


	    case 's':	/* Get and print the SPI status register */
	        ch = edt_spi_reg_read(edt_p, SPI_STAT_CTRL);
		printf("				STAT 0x%x\n", edt_spi_get_stat(edt_p));
		break;


	    case 'S':	/* Send a single line of ascii string to the msp430 over the SPI */
	    {
		char *p = str + 1;

		puts("\nOutput string to SPI:\n");

		while (*p == ' ' || *p == '\t' || *p == '"')
		{
		    if (*p == '\0')
			break;
		    ++p;
		}

		while (*p != '"' && *p != '\0')
		{
		    while ((edt_spi_get_stat(edt_p) & FIFO_OUT_EMPTY) == 0)
			edt_msleep(1);

		    printf("%c",   ch = edt_spi_put_byte(edt_p, *p++));
		    fflush(stdout);
		}
		fflush(stdout);
		puts("\n");


		break;
	    }

	    case 'L':
	    {
		int N;

		sscanf(str, "L %d", &N);
		edt_spi_flush_fifo(edt_p);
	    	send_loopback_packet(edt_p, N);
		break;
	    }


	    case 'v':	/* Get and print the timecode firmware version */
	    {
		int version;

		edt_set_timecode_enable(edt_p, 0);

		version = edt_get_timecode_version(edt_p);

		edt_set_timecode_enable(edt_p, 1);

		if (version >= 0)
		    printf("				Timecode version %d\n", version);
		else
		    puts("\nCould not obtain version information.\n");

		break;
	    }


	    case 'q':
	    case 'Q':
		exit(0);
		break;
	}
	fflush(stdout);
    }
}


int
send_loopback_packet(EdtDev *edt_p, u_int N)
{
    int i, offset = 0;
    u_char loopback_buffer[64];

    if (N > 60)
    {
	printf("send_loopback_packet(%d) # data bytes must be >= 0 and <= 60; no action taken\n", N);
	return -1;
    }

    printf("\n\nsend_loopback_packet(%d)...  ", N);
    fflush(stdout);

    loopback_buffer[offset++] = 0x00;			/* Command 0 (loopback) */
    loopback_buffer[offset++] = N;			/* Number of data bytes */

    for (i = 0; i < N; ++i)
	loopback_buffer[offset++] = i;			/* Data counts from 0 to N-1 */
	

    edt_spi_send_packet(edt_p, loopback_buffer);

    puts("Done.");

    verify_loopback_packet(edt_p, loopback_buffer);


    return 0;
}

int
verify_loopback_packet(EdtDev *edt_p, u_char *sent_buffer)
{
    u_char val;
    int offset = 0, errors = 0, data_len = sent_buffer[1];

    printf("verify_loopback_packet:  data_len is %d, header_len is 4\n", data_len);

    while (offset < data_len + 4)
    {
	val = edt_spi_get_byte(edt_p);

	if (val == '\\')		/* if '\' seen, then two-byte escape for either '\0' or '\'. */
	{
	    val = edt_spi_get_byte(edt_p);

	    if (val == 'z')
		val = 0;
	}

	if (offset == data_len + 2)
	{
	    u_char crcl = edt_crc16_lowbyte(sent_buffer, data_len+2);	/* 16-bit CRC low byte */
	    if (val != crcl)
	    {
		printf("verify_loopback_packet:  crcl error at offset %d:  %x s/b %x\n", offset, val, crcl);
		++ errors;
	    }
	}
	else if (offset == data_len + 3)
	{
	    u_char crch = edt_crc16_highbyte(sent_buffer, data_len+2);	/* 16-bit CRC high byte */
	    if (val != crch)
	    {
		printf("verify_loopback_packet:  crch error at offset %d:  %x s/b %x\n", offset, val, crch);
		++ errors;
	    }
	}
	else if (val != sent_buffer[offset])
	{
	    printf("verify_loopback_packet:  error at offset %d:  %x s/b %x\n", offset, val, sent_buffer[offset]);
	    ++ errors;
	}

	++ offset;
    }

    printf("verify_loopback_packet:  found %d errors in %d bytes\n", errors, data_len + 4);
    return 0;
}
