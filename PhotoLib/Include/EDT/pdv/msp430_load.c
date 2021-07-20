/* #pragma ident "@(#)msp430_load.c	1.17 03/01/11 EDT" */
/*
* msp430_load.c  
*
* (C) 1997-2011 Engineering Design Team, Inc.
*/

#include "edtinc.h"
#include "libedt_timing.h"

int debug  = 0;
int verify = 0;
int verbose = 0;
int version = 0;

void
usage(char *err)
{
    puts(err);
    puts("usage: ") ;
    puts("    -u <unit>             - specifies edt board unit number") ;
    puts("    -b <spi_reg_base>     - specifies alternate spi register base address") ;
    puts("    -f <msp430.txt file>  - program flash using named file") ;
    puts("    -p                    - do not apply the rev10 patch");
    puts("    -d                    - debugging mode - internal use only");
    puts("    -v                    - do not apply the rev10 patch");
    puts("    -V                    - Verbose messages");
    exit(1);
}

#define SEG_SIZE 512
#define ISHEX(x) ((x >= '0' && x <= '9') || (x >= 'a' && x <= 'f') || (x >= 'A' && x <= 'F'))

/* Linked list of segments to be erased. */
typedef struct segment_list {
    u_short saddr;					/* Segment address (16-bit address). */
    struct segment_list *next;				/* Next linked-list element. */
} segment_list_t;

/* Linked list of pointers to flash write command strings. */
typedef struct write_list {
    char * write_str;					/* String pointer to "@ADDR XX YY ZZ..." flash write command. */
    struct write_list *next;				/* Next linked-list element. */
} write_list_t;


void
error_exit(int call, int offset, char *expected, int offence)
{
    printf("\n%d: Error at offset %d, expected %s got 0x%02x; exiting.\n\n", call, offset, expected, offence);
    exit(1);
}

int
error_return(int call, int offset, char *expected, char *offence)
{
    printf("\nCode section %d: Error at offset %04x, expected %2.2s got %2.2s (%x/%x); exiting.\n\n", call, offset, expected, offence, offence[0], offence[1]);
    return call;
}


segment_list_t *segment_list_head = NULL;
write_list_t   *write_list_head = NULL;

void
add_segment_to_list(u_int saddr)
{
    segment_list_t *sptr = segment_list_head;

    if (segment_list_head == NULL)
    {
	sptr = segment_list_head = calloc(1, sizeof(segment_list_t));
	sptr->saddr = saddr;
    }
    else
    {
	sptr = segment_list_head;

	do
	{
	    if (sptr->saddr == saddr)					/* Stop if this value is in this list element */
		break;

	    if (sptr->next != NULL)
		sptr = sptr->next;

	} while (sptr->next != NULL);

	if (sptr->saddr != saddr)
	{
	    sptr->next = calloc(1, sizeof(segment_list_t));
	    sptr->next->saddr = saddr;
	}
    }

    return;
}

segment_list_t *
build_segment_erase_list(char *fbuf, int size)					/* Construct list of segments to erase in advance */
{
    int i;
    char *p;
    u_int addr;
    u_int segment_addr;
    
    for (p = fbuf; p != NULL && *p != '\0' && *p != 'q' && *p != 'Q'; )	/* Loop when *p points to '@' in "@ADDR" */
    {
	sscanf(p, "@%4x", &addr);

	segment_addr = addr & ~(SEG_SIZE - 1);

	add_segment_to_list(segment_addr);

	while (*p > ' ' && *p != '\0')					/* Advance to first white space past @ADDR */
	    ++p;

	while (*p <= ' ' && *p != '\0')					/* Advance to first non-white space past @ADDR */
	    ++p;


	/* Finish inner loop at end of @ADDR command */
	while (p != NULL && p < fbuf + size && *p != '\0' && *p != 'q' && *p != 'Q' && *p != '@')
	{
	    for (i = 0; i < 2; ++i, ++p)				/* Byte values to be written are ascii-hex "XY"  */
	    {
		if (!ISHEX(*p))  					/* Verify first and second ascii-hex nibbles */
		    error_exit(i, p - fbuf, "HEX", *p);
	    }

	    while (*p <= ' ' && *p != '\0')				/* Advance to next non-white space character */
		++p;


	    ++ addr;							/* Advance address one byte */


	    if (addr < 0xFFFF)						/* Stay in msp430 memory range */
	    {
		if (segment_addr != (addr & ~(SEG_SIZE - 1)))
		{
		    segment_addr = (addr & ~(SEG_SIZE - 1));
		    add_segment_to_list(segment_addr);
		}
	    }
	}
    }

    return segment_list_head;
}

void
add_write_cmd_to_list(char *ptr)
{
    write_list_t *wptr;

    if (write_list_head == NULL)					/* Add first element to head of the list */
    {
	wptr = write_list_head = calloc(1, sizeof(write_list_t));
	wptr->write_str = ptr;
    }
    else
    {
	for (wptr = write_list_head; wptr->next != NULL; wptr = wptr->next)
	   /* Find end of list */ ;

	wptr->next = calloc(1, sizeof(write_list_t));			/* Add remaining new elements to end of list */
	wptr->next->write_str = ptr;
    }

    return;
}

write_list_t *
build_flash_write_list(char *fbuf, int size)					/* Construct list of flash write strings. */
{
    char *p, *q, *old_p;
    
    for (p = fbuf; p != NULL && p < fbuf + size; p = strchr(p, '@'))
    {
	old_p = p;
	add_write_cmd_to_list(p);

	if (p != fbuf)
	{
	    q = p - 1;
	    *q = '\0';							/* Null terminate each "@ADDR XX YY ZZ..." command. */
	}

	++ p;								/* Move past current '@' */
    }

    q = strchr(old_p, 'q');						/* Null terminate the last "@ADDR XX YY ZZ..." command. */
    if (q == NULL)
	q = strchr(old_p, 'Q');
    if (q == NULL)
	q = strchr(old_p, '\r');
    if (q == NULL)
	q = strchr(old_p, '\n');
    if (q != NULL)
	*q = '\0';

    return write_list_head;
}


int
erase_segments(EdtDev *edt_p, segment_list_t *segment_list)
{
    segment_list_t *sptr;
    char cmd[16];

    for (sptr = segment_list; sptr != NULL; sptr = sptr->next)
    {
	printf("Erasing segment 0x%4hx...                \n", sptr->saddr);
	fflush(stdout);

	edt_msleep(250);

	sprintf(cmd, "*%4hx", sptr->saddr);
	if (debug)
	{
	    puts("\ncmd: ");
	    puts(cmd);
	    puts("");
	}
	else
	    edt_spi_putstr(edt_p, cmd);
    }
    puts("\nDone");
    return 0;
}

int
program_flash(EdtDev *edt_p, write_list_t *write_list)
{
    int ch;

    write_list_t *wptr;

    for (wptr = write_list; wptr != NULL; wptr = wptr->next)
    {
	printf("Writing from address %1.4s    \r", wptr->write_str + 1);
	fflush(stdout);

	edt_msleep(250);

	if (debug)
	{
	    puts("");
	    puts(wptr->write_str);
	    puts("");
	}
	else
	{
	    edt_spi_putstr(edt_p, wptr->write_str);
	    edt_msleep(500);
	
	    /* Check for errors */
	    edt_spi_put_byte(edt_p, 0x80);
	    ch = edt_spi_get_byte_nointr(edt_p);

	    /*printf("Return code %02x\n", ch); */
	    /*puts("Hit return to continue"); */
	    /*getchar(); */


	    while (ch < 0x80) /* should be no ascii timecode bytes.  Discard and note if there are. */
	    {
		fprintf(stderr, "Flushing SPI timecode byte 0x%x\n", ch);
		edt_spi_put_byte(edt_p, 0x80);
		ch = edt_spi_get_byte_nointr(edt_p);
	    }

	    if (ch > 0x90)
	    {
		int errors = ch - 0x90;

		fprintf(stderr, "\n\nFlash write command to address 0x%4.4s failed , %d errors.  Exiting\n\n", wptr->write_str + 1, errors);
		fprintf(stderr, "Expected 0x90 got 0x%x\n", ch);
		return -1;
	    }
	}
    }

    puts("\nDone");


    return 0;
}

int
verify_flash(EdtDev *edt_p, char *fbuf, FILE *flash_fp, int fsize)
{
    int i;
    char *p;
    u_int addr;
    int bytes = 0;
    char cmd[16];
    char flash_byte[3] = {0, 0, 0};
    char file_byte[3]  = {0, 0, 0};


    puts("\n\nVerifying flash:\n\n");
    memset(fbuf, 0, fsize);						

    rewind(flash_fp);
    if ((fread(fbuf, sizeof(char), fsize, flash_fp)) != fsize)		/* File contents now stored as one large char string */
    {
	edt_perror("fread fbuf");
	exit(1);
    }

    p = fbuf;

    while (*p <= ' ' && *p != '\0')					/* Advance to first non-white space */
	++p;

    for (p = fbuf; p != NULL && *p == '@'; )				/* Loop when *p points to '@' in "@ADDR" */
    {
	sscanf(p, "@%4x", &addr);

	while (*p > ' ' && *p != '\0')					/* Advance to first white space past @ADDR */
	    ++p;

	while (*p <= ' ' && *p != '\0')					/* Advance to first non-white space past @ADDR */
	    ++p;

	/* Loop over each byte attached to each "@ADDR XX YY ZZ..." command */
	while (p != NULL && p < fbuf + fsize && *p != '\0' && *p != 'q' && *p != 'Q' && *p != '@')
	{
	    printf("\rFlash address: %04x  ok  ", addr);			/* Print the byte address */

	    sprintf(cmd, "R%04x", addr);				/* Set the byte address */

	    if (debug)
		fputs(cmd, stdout);
	    else
		edt_spi_putstr(edt_p, cmd);				/* Send a "read byte" command to the msp430 */

	    fflush(stdout);


	    /* The byte values are returned as two ascii-hex bytes from the msp430 */
	    for (i = 0; i < 2; i++, p++)
	    {
		if (ISHEX(*p))  					/* Verify first and second ascii-hex nibble */
		{
		    if (debug)
			flash_byte[i] = *p;
		    else
			flash_byte[i] = edt_spi_get_byte_nointr(edt_p);	/* Put each byte from the flash into a two-char string */

		    file_byte[i] = *p;					/* Put each byte from the file into a two-char string */
		}
		else
		{
		    file_byte[0] = *p;					/* Print the offending file character  */
		    file_byte[1] = '\0';
		    return error_return(5, p - fbuf, "HEX", file_byte);
		}
	    }

	    if (strcmp(flash_byte, file_byte) != 0)			/* Compare the flash and the file strings */
	    {
		int retval = error_return(4, addr, file_byte, flash_byte);

		if (!verbose)
		    return retval;
	    }

	    ++ addr;
	    ++ bytes;

	    while (*p <= ' ' && *p != '\0')				/* Advance to next non-white space character */
		++p;
	}
    }

    printf("\n%d bytes verified\n\n", bytes);

    return 0;
}

char str[128];

int
main(argc, argv)
int argc;
char **argv;
{
    EdtDev *edt_p;
    int unit 	= 0, patch = 0;
    u_int spi_reg_base = 0; 	/* 0 value specifies default spi register base of 0x60 */
    int retval 	= 0;
    int fsize;
    char *fbuf;
    char *flash_name;
    FILE *flash_fp = NULL ;
    segment_list_t *segment_erase_list = NULL;
    write_list_t *flash_write_list = NULL;

    while (argc > 1 && argv[1][0] == '-')
    {
        switch (argv[1][1])
        {
        case 'u':
            ++argv;
            --argc;
            if (argc < 2) 
            {
		        usage("Error: option 'u' requires a numeric argument\n");
	        }
	        if ((argv[1][0] >= '0') && (argv[1][0] <= '9'))
            {
                unit = atoi(argv[1]);
            }
            else 
            {
		        usage("Error: option 'u' requires a numeric argument\n");
	        }
	        break;

        case 'b':
            ++argv;
            --argc;
            spi_reg_base = strtol(argv[1], NULL, 16);
            break ;

        case 'p':
	        patch = 1;
            break ;

        case 'd':
	        debug = 1;
            break ;

        case 'v':
	        verify = 1;
            break ;

        case 'V':
	        verbose = 1;
            break ;

	    case 'f':
            ++argv;
            --argc;
	        flash_name = argv[1];
	        if ((flash_fp  = fopen(flash_name, "rb")) == NULL)
	        {
		    perror(argv[1]) ;
		    exit(1) ;
	        }
            break ;

        case 'h':
        case 'H':
            usage("");
            break;

        default:
            usage("unknown option") ;
        }
        --argc ;
        ++argv ;
    }

    if (flash_fp == NULL)
    {
	fputs("Error:  -f <msp430.txt file> argument missing from command line\n", stderr);
	exit(1);
    }

    if ((edt_p = edt_spi_open(EDT_INTERFACE, unit, spi_reg_base)) == NULL)
    {
	fputs("msp430 firmware load failed: edt_spi_open()\n\n", stderr);
	fflush(stderr);
	exit(1);
    }

    if (!verify && !patch)
    {
	int firmware_version;

	firmware_version = edt_get_timecode_version(edt_p);

	if (firmware_version < 10)
	{
	    /* Must apply patch before reprogramming with new firmware */
	    puts("\nA patch must be applied to this firmware before proceeding.\n");

	    fputs("Hit return to apply the patch: ", stdout);
	    fflush(stdout);
	    fgets(str, 127, stdin);
	    puts("");

	    if (firmware_version <= 5)
	    {
		printf("Applying patch firmware %s:\n", "msp430_r15patch.txt");
		sprintf(str, "./msp430_load -p -u %d -b %2x -f msp430_r15patch.txt", unit, spi_reg_base);
		edt_system(str);
	    }
	    else
	    {
		printf("Applying patch firmware %s:\n", "msp430_r17patch.txt");
		sprintf(str, "./msp430_load -p -u %d -b %2x -f msp430_r17patch.txt", unit, spi_reg_base);
		edt_system(str);
	    }

	    printf("\nPatch applied; now loading flash file \"%s\"\n\n", flash_name);
	}
    }


    /* Transfer flash file to fbuf in char * string format */
    fseek(flash_fp, 0, SEEK_END);					/* Get file size in bytes */
    fsize = ftell(flash_fp);
    rewind(flash_fp);

    if ((fbuf = calloc(fsize + 1, sizeof(char))) == NULL)		/* Allocate for file and trailing NULL */
    {
	edt_perror("calloc fbuf");
	exit(1);
    }

    if (!verify)
    {
	if ((fread(fbuf, sizeof(char), fsize, flash_fp)) != fsize)		/* File contents now stored as one large char string */
	{
	    edt_perror("fread fbuf");
	    exit(1);
	}


	segment_erase_list = build_segment_erase_list(fbuf, fsize);		/* Construct list of segments to erase in advance */


	flash_write_list = build_flash_write_list(fbuf, fsize);			/* Construct list of flash write strings */


	if (segment_erase_list == NULL || flash_write_list == NULL)
	{
	    fputs("Flash file parse error; exiting\n", stderr);
	    exit(1);
	}
    }


    if (!debug)
    {
	if (edt_spi_invoke_flash_loader(edt_p))
	    exit(1);
    }

    if (verify)
    {
	retval = verify_flash(edt_p, fbuf, flash_fp, fsize);

	puts("\nRestarting the msp430");
	edt_spi_put_byte(edt_p, 'q');
	edt_msleep(500);
    }
    else
    {
	if (!patch)
	{
	    fputs("Hit return to reprogram the msp430 flash memory: ", stdout);
	    fflush(stdout);
	    fgets(str, 127, stdin);
	}

	puts("\nDO NOT INTERRUPT THIS PROGRAM DURING FLASH ERASE OR REPROGRAMMING\n");
	puts("Erasing the msp430 flash memory:");
	fflush(stdout);
	edt_msleep(100);
	

	erase_segments(edt_p, segment_erase_list);
	puts("");


	puts("Programming the msp430 flash memory:");
	fflush(stdout);
	edt_msleep(100);
	

	if (program_flash(edt_p, flash_write_list) != 0)
	    exit(2);


	puts("");
	puts("msp430 flash load complete");

	verify_flash(edt_p, fbuf, flash_fp, fsize);


	puts("");
	puts("");
	edt_msleep(100);


	puts("Restarting the msp430");
	edt_spi_put_byte(edt_p, 'q');
	edt_msleep(500);

	puts("Done.\n");
    }

    return retval;
}
