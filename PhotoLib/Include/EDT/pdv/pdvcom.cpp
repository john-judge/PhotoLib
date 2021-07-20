/*
* pdvcom.c - EDT Camera Link serial to COM port wrapper
*
* Copyright (c) 2014, EDT, Inc.
*/

#include <afx.h>
#include <cstring>
#include <WinBase.h>
#include <iostream>
#include <tchar.h>
#include <cstdio>
#include "edtinc.h"

using namespace std;

#define CR '\r'

#define DEFAULT_ESCAPE  ".:"

#include <conio.h>


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


typedef struct _serial_options {

    u_char     no_return;
    u_char	   hexin;
    u_char     nobuffering;
    u_char     use_serial_fifo;
    int        baud;
    int        parity;



} PdvSerialOptions;

long
MainHelp(int argc, char *argv[], char *edt_devname)
{
    printf("Usage:   pdvcom [options] port\n", argv[0]);

    printf("port is required, usually 'comX' where 'X' is the port number, e.g. com4.\n");

    printf(
        "    -?, /?, -h       - Help message\n"
        "    -u N             - Unit number (default 0)\n"
        "    -c N             - channel number (default 0)\n"
        "    -p portname      - Com port name (default com5)\n"
        "    -b baud          - Set baud rate (default 9600)\n"
        "    -o <e|o|n>       - parity <e>ven | <o>dd | <n>one \n"
        "    -v               - Verbose\n"
        "    -t               - Serial timeout override value\n"
        );

    return 0;
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

DWORD baudList[] = {
    110,
    300,
    600,
    1200,
    2400,
    4800,
    9600,
    14400,
    19200,
    38400,
    57600,
    115200,
    128000,
    256000,
    0
};

DWORD
validateBaud(DWORD baud)
{
    int i;

    for (i=0; baudList[i] != 0; i++)
        if (baudList[i] == baud)
            return baud;
    return 9600;
}

bool
pdv_open_com_port(HANDLE *hSerial, CString portSpecifier, int baud, OVERLAPPED *ov)
{

    DWORD baudRate = validateBaud(baud);
    DCB dcb = {0};

    *hSerial = CreateFile(
        portSpecifier,
        GENERIC_READ | GENERIC_WRITE,
        0,
        0,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        0);

    if (*hSerial==INVALID_HANDLE_VALUE)
    {
        int err;
        if((err = GetLastError()) == ERROR_FILE_NOT_FOUND)
        {
            printf("serial port %s doesn't exist\n", portSpecifier);
        }
        else
            printf("error %d on port %s\n", err, portSpecifier);

        return FALSE;
    }


    dcb.DCBlength=sizeof(dcb);
    if (!GetCommState(*hSerial, &dcb))
    {
        printf("error getting state\n");
        return FALSE;
    }
    dcb.BaudRate=baudRate;
    dcb.ByteSize=8;
    dcb.StopBits=ONESTOPBIT;
    dcb.Parity=NOPARITY;
    if(!SetCommState(*hSerial, &dcb))
    {
        printf("error setting state\n");
        return FALSE;
    }
    else
    {
        COMMTIMEOUTS ct = {0} ;

        ct.ReadIntervalTimeout = MAXDWORD;
        ct.ReadTotalTimeoutMultiplier = 0;
        ct.ReadTotalTimeoutConstant = 0;
        ct.WriteTotalTimeoutMultiplier = 0;
        ct.WriteTotalTimeoutConstant = 0;

        SetCommTimeouts (*hSerial, &ct);
        printf("open/set %s at %d OK\n", portSpecifier, baudRate);
    }

    return TRUE;
}

void
pdv_close_com_port(HANDLE hSerial)
{
}

void
read_com_write_pdv(PdvDev *pdv_p, HANDLE hSerial, OVERLAPPED *ov, u_char *buf, bool verbose)
{
    DWORD bytesRead;
    BOOL retVal = ReadFile(hSerial,buf,1,&bytesRead, ov);

    if (retVal && bytesRead)
    {
        pdv_serial_write(pdv_p, (char *)buf, bytesRead);

        if (verbose)
        {
            int i;
            for (i=0; i<bytesRead; i++)
            {
                if (isascii(buf[i]))
                    printf("< %02x '%c' ", buf[i], buf[i]);
                else printf("< %02x ", buf[i], buf[i]);
            }
            printf("\n");
        }
    }
}

void
read_pdv_write_com(PdvDev *pdv_p, HANDLE hSerial, OVERLAPPED *ov, u_char *buf, bool verbose)
{
    CString cbuf;
    DWORD bytesWritten;
    BOOL retVal;
    int ret;

    ret = pdv_serial_wait(pdv_p, 1, 1);

	if (ret > 0)
    {
        int rem = pdv_serial_read_nullterm(pdv_p, (char *)buf, 4095, FALSE);
        int err;

        if (((retVal = WriteFile(hSerial, buf, ret, &bytesWritten, ov)) == FALSE) && ((err = GetLastError()) != ERROR_IO_PENDING))
        {
            printf("error %d on write\n", err);
            return;
        }

        if (verbose)
        {
            int i;
            for (i=0; i<ret; i++)
            {
                if (isascii(buf[i]))
                    printf("> %02x '%c' ", buf[i], buf[i]);
                else printf("> %02x ", buf[i], buf[i]);
            }
            printf("\n");
        }
    }
    else
        edt_msleep(1);
}

long
PdvSerialWriteRead(PdvDev *pdv_p, HANDLE hSerial, OVERLAPPED *ov, PdvSerialOptions *opts,
                   int timeout, int sleepval, int verbose, char *escape)
{
    int       ret;
    u_char    *inbuf;
    u_char    *outbuf;

    pdv_serial_setup(pdv_p, opts);

    if ((outbuf = (u_char *)malloc(4096)) == NULL) {
        fprintf(stderr, "malloc(4096) failed (line %d)", __LINE__);
        exit(1);
    }
    if ((inbuf = (u_char *)malloc(4096)) == NULL) {
        fprintf(stderr, "malloc(4096) failed (line %d)", __LINE__);
        exit(1);
    }


    outbuf[0] = 0;
    inbuf[0] = 0;

    /*  enable read */

    pdv_serial_read_enable(pdv_p);
    pdv_reset_serial(pdv_p);


    while (1)
    {
        read_pdv_write_com(pdv_p, hSerial, ov, inbuf, verbose);
        read_com_write_pdv(pdv_p, hSerial, ov, outbuf, verbose);

    }

    return (1);
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
    HANDLE hSerial = NULL;
    OVERLAPPED ov = {0};
    char    escape[4];
    CString  comPort = "com5";
    int readsize = 512;

    PdvSerialOptions options;

    memset(&options,0,sizeof(options));
    options.no_return = TRUE;
    options.nobuffering = TRUE;
    options.use_serial_fifo = FALSE;
    options.baud = 9600;

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
            comPort = argv[0];
            break;

        case 'o':
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
    printf("opening %s%d_%d\n", EDT_INTERFACE, unit, channel); 
    pdv_p = pdv_open_channel(EDT_INTERFACE, unit, channel);
    if (pdv_p == NULL)
    {
        pdv_perror(EDT_INTERFACE);
        return -1;
    }

    pdv_set_baud(pdv_p, options.baud);

    if (timeout < 1)
        timeout = pdv_p->dd_p->serial_timeout;

    if (verbose)
        printf("serial timeout %d\n", timeout);


    if (pdv_open_com_port(&hSerial, comPort, options.baud, &ov) == FALSE)
    {
        printf("exiting\n");
        exit(1);
    }

    status = PdvSerialWriteRead(pdv_p, hSerial, &ov, &options, 
        timeout, sleepval, verbose, escape);

    pdv_close_com_port(hSerial);
    pdv_close(pdv_p);

    exit(status);
    return (0);
}


