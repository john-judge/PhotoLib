/*
* pdvterm.c - send commands to serial camera over AIA serial lines
* Copyright (c) 1998, EDT, Inc.
*/

#include "edtinc.h"
#include <stdio.h>

/* prototypes */
void    print_ascii_string(char *buf_p);

long    isascii_str(u_char *buf, int n);
long    ack_nak_str(u_char *buf, int n);

#define CR '\r'

#define DEFAULT_ESCAPE  ".:"

#ifndef WIN32

#include <sys/ioctl.h>
#ifdef __linux__

#include <asm/ioctls.h>

#elif defined(VXWORKS)

#include "ioLib.h"

#else

#include <sys/filio.h>		/* for kbhitedt() */

#endif


#include <termios.h>
int kbhit()
{
    struct termios stored_settings;
    struct termios new_settings;
    int i;
    int numchars = 0;
    int rc;

    tcgetattr(0, &stored_settings);

    new_settings = stored_settings;
    // modify flags
    // first, disable canonical mode
    // (canonical mode is the typical line-oriented input method)
    new_settings.c_lflag &= (~ICANON);
    new_settings.c_lflag &= (~ECHO); // don't echo the character

    tcsetattr(0, TCSANOW, &new_settings);

    rc = ioctl(0, FIONREAD, &numchars) ;

    tcsetattr(0, TCSANOW, &new_settings);

    return(numchars);


}



int getch()

{
    struct termios stored_settings;
    struct termios new_settings;
    int i;

    tcgetattr(0, &stored_settings);

    new_settings = stored_settings;
    // modify flags
    // first, disable canonical mode
    // (canonical mode is the typical line-oriented input method)
    new_settings.c_lflag &= (~ICANON);
    new_settings.c_lflag &= (~ECHO); // don't echo the character

    tcsetattr(0, TCSANOW, &new_settings);

    i = getc(stdin);


    tcsetattr(0, TCSANOW, &new_settings);

    return i;

}

#else

#include <conio.h>

#endif

int
edt_set_use_serial_fifo(PdvDev *pdv_p, int state)

{
    return edt_ioctl(pdv_p, EDTS_SERIAL_FIFO, &state);
}

int 
edt_get_use_serial_fifo(PdvDev *pdv_p)

{
    int state;
    edt_ioctl(pdv_p, EDTG_SERIAL_FIFO, &state);
    return state;
}


void
printify(u_char *buf, u_char *dest, int n, int show_nonprint)

{
    int     has_lf = FALSE;
    int     has_cr = FALSE;

    int     i, j;

    for (i = 0; i < n; i++)
    {
        if (buf[i] == 13)
            has_cr = TRUE;
        if (buf[i] == 10)
            has_lf = TRUE;
    }

    for (i = 0, j = 0; i < n; i++)
    {

        if (buf[i] == 13)
        {
            if (!has_lf)
                dest[j++] = '\n';
        }
        else if (buf[i] == 0x09 || buf[i] == 0x0a || buf[i] == 0x0d || (buf[i] >= ' ' && buf[i] < 127))
            dest[j++] = buf[i];
        else if (show_nonprint)
        {
            switch (buf[i])
            {
            case 0x06:
                strcpy((char *) dest + j, "<ACK>");
                break;
            case 0x15:
                strcpy((char *) dest + j, "<NAK>");
                break;
            case 0x02:
                strcpy((char *) dest + j, "<STX>");
                break;
            case 0x03:
                strcpy((char *) dest + j, "<ETX>");
                break;
            default:
                sprintf((char *) dest + j, "<%02x>", buf[i]);
            }

            j = (int)strlen((char *) dest);

        }

        dest[j] = 0;
    }


}

char    *printify_tmpbuf = NULL;

/*
* print ascii string. Replace \r or Ctrl-M with \n for consistency and
* pretty printing
*/
void
print_ascii_string(char *buf_p)
{
    if (!printify_tmpbuf)
        printify_tmpbuf = malloc(4096);

    printify((u_char *) buf_p, (u_char *) printify_tmpbuf, (int)strlen(buf_p), 1);

    printf("%s", printify_tmpbuf);
    fflush(stdout);
}

typedef struct _serial_options {

    u_char     basler_framing;
    u_char     duncan_framing;
    u_char     no_return;
    u_char	   hexin;
    u_char     nobuffering;
    u_char     use_serial_fifo;
    int	baud;
    int     parity;



} PdvSerialOptions;

long
MainHelp(int argc, char *argv[], char *edt_devname)
{
    printf("Usage:   pdvterm [-h | -?] [-u unit] [-x] [\"cmd\"]\n");

    printf(
        "    -?, /?, -h       - Help message\n"
        "    -u N             - Unit number (default 0)\n"
        "    -f N             - FOI (fiber optic remote) unit #\n"
        "    -c N             - channel #\n"
        "    -b baud          - Set baud rate\n"
        "    -p <e|o|n>       - parity <e>ven | <o>dd | <n>one \n"
        "    -v               - Verbose\n"
        "    -t               - Serial timeout override value\n"
        );
    printf(
        "    -S file          - Send the contents of a file\n"
        "    -R file [s] [bs] - Receive, copy into the named file, optional size & blocksize\n"
        );


    printf(
        "    -B               - Basler framing mode -- cmd should be hex cmd (bytes separated by\n"
        "    -D               - Duncan framing mode -- cmd should be hex cmd (bytes separated by\n"
        "                       spaces, with NO framing info (pdvterm will add STX and Checksum)\n"
        "    -x               - Hex mode -- cmd should be hex bytes separared by spaces\n"
        "   \"cmd\"             - ASCII (default) or hex command(s) to send\n"
        );
    return 0;
}

int check_console(char * buffer)

{
    if (kbhit())
    {
        int len = (int)strlen(buffer);

        buffer[len] = getch();
        buffer[len+1] = 0;
        putchar(buffer[len]); fflush(stdout);
        return buffer[len];
    }

    return 0;

}

void pdv_transmit_msg(PdvDev *pdv_p, char *ibuf_p, PdvSerialOptions *opts, int verbose)

{
    int nbytes;
    char    bs[32][3];
    u_char  hbuf[2048];
    int i;

    if (opts->hexin)
    {
        nbytes = sscanf(ibuf_p, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
            bs[0], bs[1], bs[2], bs[3], bs[4], bs[5], bs[6], bs[7],
            bs[8], bs[9], bs[10], bs[11], bs[12], bs[13], bs[14], bs[15]);

        /*
        * change 5/28/99 one serial_binary_command for the whole
        * thing -- before it did one write per byte which was dumb
        * and didn't work on FOI anyway
        */

        for (i = 0; i < nbytes; i++)
        {
            if (strlen(bs[i]) > 2)
            {
                printf("hex string format error\n");
                break;
            }
            hbuf[i] = (u_char) (strtoul(bs[i], NULL, 16) & 0xff);

        }

        /*
        * using pdv_serial_binary_command instead of
        * pdv_serial_write because it prepends a 'c' if FOI
        */
        if (opts->duncan_framing)
            pdv_send_duncan_frame(pdv_p, hbuf, nbytes);
        else if (opts->basler_framing)
            pdv_send_basler_frame(pdv_p, hbuf, nbytes);
        else
            pdv_serial_binary_command(pdv_p, (char *) hbuf, nbytes);
    }
    else
    {
        char tmpbuf[2048];
        if (verbose)
            printf("writing <%s>\n", ibuf_p);
        if (!opts->no_return)
        {
            sprintf(tmpbuf, "%s\r", ibuf_p);
            pdv_serial_write(pdv_p, tmpbuf, (int)strlen(tmpbuf));
        }else
            pdv_serial_write(pdv_p, ibuf_p, (int)strlen(ibuf_p));

    }

}

int
pdv_serial_setup(PdvDev *pdv_p, PdvSerialOptions *opts)
{
    if (opts->baud)
    {
        pdv_set_baud(pdv_p, opts->baud);
    }

    if (opts->parity)
    {
        pdv_set_serial_parity(pdv_p, opts->parity);
    }

    if (opts->use_serial_fifo)
    {
        int x;
        x = edt_get_use_serial_fifo(pdv_p);
        printf("serial fifo was %d\n", x);
        x = 1;
        edt_set_use_serial_fifo(pdv_p, x);
    }

    return 0;
}


long
PdvSerialWriteRead(PdvDev *pdv_p, PdvSerialOptions *opts,
                   int timeout, int sleepval, int verbose, char *escape )
{
    int     ret;
    char    *inbuf;
    char    *buf;

    u_char sendbuf = FALSE;
    u_char done = FALSE;
    char ch;

    pdv_serial_setup(pdv_p, opts);

    if ((buf = malloc(4096)) == NULL) {
        fprintf(stderr, "malloc(4096) failed (line %d)", __LINE__);
        exit(1);
    }
    if ((inbuf = malloc(4096)) == NULL) {
        fprintf(stderr, "malloc(4096) failed (line %d)", __LINE__);
        exit(1);
    }


    /* flush any junk */
    buf[0] = 0;
    inbuf[0] = 0;
    printf("> "); 
    fflush(stdout);

    /*  enable read */

    pdv_serial_read_enable(pdv_p);
    pdv_reset_serial(pdv_p);

    while (!done)
    {
        /* check keyboard */

        if ((ch = check_console(buf)) != 0)
        {
            if (opts->nobuffering)
                sendbuf = TRUE;
            else if (ch == CR)
            {
                sendbuf = TRUE;
            }


            if (sendbuf)
            {
                if (escape && (strncmp(buf,escape,strlen(escape)) == 0))
                {
                    if (buf[strlen(escape)] == 'q')
                        done = TRUE;

                }
                else
                    pdv_transmit_msg(pdv_p,buf,opts, verbose);

                if (!opts->nobuffering)
                {
                    printf("\n>");
                    fflush(stdout);

                }

                sendbuf = FALSE;
                buf[0] = 0;

            }


        }
        /* check pdv */

        /*
        * serial_timeout comes from the config file (or -t override flag in
        * this app), or if not present defaults to 500 unless readonly
        * defaults to 60000
        */

        ret = pdv_serial_wait(pdv_p, timeout, 1);

	if (ret > 0)
            ret = pdv_serial_read(pdv_p, inbuf, 2047);
        else
            edt_msleep(sleepval);


        if (ret != 0)
        {
            inbuf[ret + 1] = 0;
            if (opts->hexin || opts->basler_framing || opts->duncan_framing)
            {
                int     i;

                if (ret)
                {
                    printf("resp <");
                    for (i = 0; i < ret; i++)
                        printf("%s%02x", i ? " " : "", (u_char) inbuf[i]);
                    printf(">\n");
                }
            }
            else		/* simple ASCII */
                print_ascii_string(inbuf);
        }

    }

    return (1);
}


long
isascii_str(u_char * buf, int n)
{
    int     i;

    for (i = 0; i < n; i++)
        if ((buf[i] < ' ' || buf[i] > '~')
            && (buf[i] != '\t')
            && (buf[i] != '\n')
            && (buf[i] != '\r'))
            return 0;
    return 1;
}

long
ack_nak_str(u_char *buf, int n)
{
    int     i;

    /* check for all ASCII between the 1st and last chars, or ACK or NAK */
    for (i = 0; i < n; i++)
        if ((buf[i] < ' ' || buf[i] > '~')
            && (buf[i] != 0x15)	/* NAK */
            && (buf[i] != 0x6)	/* ACK */
            && (buf[i] != '\n')	/* LF */
            && (buf[i] != '\r'))/* CR */
            return 0;
    return 1;
}

int
pdv_serial_write_file_to_port(PdvDev *pdv_p, char *filename, int chunk)

{
    FILE *f = fopen(filename,"rb");

    if (f)
    {
        char *buffer;

        unsigned long fsize;

        double t1;

        fseek(f,0,SEEK_END);

        fsize = ftell(f);

        fseek(f,0,SEEK_SET);


        if (fsize)
        {
            buffer = malloc(fsize);

            if (buffer)
            {
                fread(buffer, fsize,1,f);

                t1 = edt_timestamp();

                pdv_serial_write(pdv_p, buffer, fsize);

                t1 = edt_timestamp() - t1;

                printf("Wrote %ld chars in %f s, %f chars/sec\n",
                    fsize, t1, fsize/t1);

            }

        }

    }

    return 0;

}

int
pdv_serial_read_file_from_port(PdvDev *pdv_p, char *filename, int fsize, int chunk)

{
    FILE *f = fopen(filename,"wb");

    if (f)
    {
        char *buffer;

        double t1;



        if (fsize)
        {
            buffer = malloc(fsize);

            if (buffer)
            {

                int avail = pdv_serial_wait(pdv_p,1000,1);

                while (!avail)
                {
                    avail = pdv_serial_wait(pdv_p,1000,1);
                }

                t1 = edt_timestamp();

                pdv_serial_read_blocking(pdv_p, buffer, fsize);

                t1 = edt_timestamp() - t1;

                printf("Read %d chars in %f s, %f chars/sec\n",
                    fsize, t1, fsize/t1);

                fwrite(buffer, fsize,1,f);

                fclose(f);

		free(buffer);
            }

        }

    }

    return 0;

}

int main(int argc, char *argv[])
{
    long    status;		/* return status from command */
    int     unit = 0;
    int     verbose = 0;
    int     do_wait = 0;
    int     timeout = 20;
    int     channel = 0;
    int     sleepval = 2;
    int chunk = 512;
    PdvDev *pdv_p = NULL;
    char    escape[4];

    char send_file[256];
    char rcv_file[256];
    int readsize = 512;

    PdvSerialOptions options;

    memset(&options,0,sizeof(options));
    options.no_return = TRUE;
    options.nobuffering = TRUE;
    options.use_serial_fifo = FALSE;

    send_file[0] = 0;
    rcv_file[0] = 0;

    strcpy(escape,DEFAULT_ESCAPE);

    --argc;
    ++argv;
    while ((argc > 0) && ((argv[0][0] == '-') || (argv[0][0] == '/')))
    {
        switch (argv[0][1])
        {
        case 'u':		/* device unit number */
            ++argv;
            --argc;
            if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
                unit = atoi(argv[0]);
            break;

        case 'x':
            options.hexin = 1;
            break;

        case 'S':
            ++argv;
            --argc;
            if (argc)
            {
                strcpy(send_file,argv[0]);

            }
            if (argc > 1)
            {
                if (argv[1][0] != '-')
                {
                    ++argv;
                    --argc;
                    chunk = atoi(argv[0]);

                }
            }

            break;
        case 'R':
            ++argv;
            --argc;
            if (argc)
            {
                strcpy(rcv_file,argv[0]);

            }
            if (argc > 1)
            {
                if (argv[1][0] != '-')
                {
                    ++argv;
                    --argc;
                    readsize = atoi(argv[0]);

                    if (argc > 1)
                    {
                        if (argv[1][0] != '-')
                        {
                            ++argv;
                            --argc;
                            chunk = atoi(argv[0]);

                        }
                    }
                }
            }

            break;

        case 'w':
            do_wait = 1;
            break;

        case 't':
            ++argv;
            --argc;
            if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
                timeout = atoi(argv[0]);
            break;

        case 'b':
            ++argv;
            --argc;
            options.baud = atoi(argv[0]);
            break;

        case 'p':
            ++argv;
            --argc;
            options.parity = argv[0][0];
            break;

        case 'v':
            verbose = 1;
            break;

        case 'h':
        case '?':
            MainHelp(argc, argv, NULL);
            exit(0);
            break;

        case 's':
            ++argv;
            --argc;
            sleepval = atoi(argv[0]);
            break;
        case 'c':
            ++argv;
            --argc;
            if (argc < 1)
            {
                fprintf(stderr, "Error - channel # expected\n");
                exit(-1);
            }
            channel = atoi(argv[0]);
            break;

        case 'B':
            options.basler_framing = 1;
            break;

        case 'D':
            options.duncan_framing = 1;
            break;

        case 'F':
            options.use_serial_fifo = TRUE;
            break;

        default:
            fprintf(stderr, "unknown flag -'%c'\n", argv[0][1]);
            MainHelp(argc, argv, NULL);
            exit(0);
            break;
        }
        argc--;
        argv++;
    }

    /* open a handle to the device     */
    pdv_p = pdv_open_channel(EDT_INTERFACE, unit, channel);
    if (pdv_p == NULL)
    {
        pdv_perror(EDT_INTERFACE);
        return -1;
    }

    if (timeout < 1)
        timeout = pdv_p->dd_p->serial_timeout;

    if (verbose)
        printf("serial timeout %d\n", timeout);

    if (send_file[0])
    {
        pdv_serial_setup(pdv_p, &options);
        pdv_serial_write_file_to_port(pdv_p, send_file, chunk);
    }
    else if (rcv_file[0])
    {
        pdv_serial_setup(pdv_p, &options);
        pdv_serial_read_file_from_port(pdv_p, rcv_file, readsize, chunk);

    }
    else
    {

        status = PdvSerialWriteRead(pdv_p, &options, 
            timeout, sleepval, verbose, escape);
    }

    pdv_close(pdv_p);

    exit(status);
    return (0);
}


