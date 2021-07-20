/*
 * rcxload
 *
 * load rcx FPGA code from properly formatted file
 *
 * Each file starts out with 64 bytes of arbitrary data, could be an
 * ASCII description of what is in the file. The first 32 bits is
 * a std magic number for unix, selected to not confuse vi ("RCX0")
 *
 * RCXLOAD file format (fname.rcx):
 *
 * HEADER: 4 bytes magic number: "RCX\0" (0x42 0x43 0x58 0x00)
 *         60 bytes arbitrary data (ASCII description)
 *
 * BLOCK0: 4 byte magic #:  0xASC1C0DE for ASCII cmd block, 0x0101C0DE for binary
 *         44 bytes filename
 *         4 byte bytecount
 *         4 byte checksum (CRC?)
 *         8 bytes spare (currently zero)
 *         binary data of length specified above in bytecount
 *
 * BLOCK1 (same as above)
 *    .
 *    .
 *    .
 * BLOCKN  (last block)
 *
 * ASCII codes for command block include the following:
 *  burn xxxxx name			; burn flash at xxxxxx with named binary block (file)
 *  verify xxxxx name			; verify flash at xxxxxx with named binary block (file)
 *  erase xxxxx				; erase a sector of flash at xxxx (0000,1000,...f000)
 *  if xxxxxx = "string" goto label	; jump if *flash_addr (hex) = string
 *  echo string                     	; echoes string to console
 *  echob string                     	; echoes string to console if not verify
 *  echov string                     	; echoes string to console if verify
 *  @label				; target of jump 
 *  exit 			 	; exit
 *
 * RCX PROM binary blocks are organized in blocks of 28000 hex bytes. The first block
 * is 0x10000, the second is 0x38000, then 0x60000, and so on with the last block being
 * 0xd8000. rcxload stores various ID info in the last 1024 bytes of the first block (i.e.
 * 0x37c00-0x37fff .
 */
#include "edtinc.h"

#define RCXMAGIC_FILE "RCX\0"
#define RCXMAGIC_ASC 0xA5C1C0DE
#define RCXMAGIC_BIN 0x0101C0DE

typedef struct {
    u_int magic;
    char name[44];
    u_int bytecount;
    u_int cksum;
    u_char spare[8];
} BLOCK_HEADER;

#define BLOCK_HEADER_SIZE 64 /* it darned well better be */

static int  spi_res(EdtDev *edt_p);
static int  spi_rdsr(EdtDev *edt_p);
static void spi_read(EdtDev *edt_p, int addr, u_char *rbuf, int rcnt);
static void spi_write(EdtDev *edt_p, int addr, int wcnt);
static int  spi_erase(EdtDev *edt_p, int addr);
static void spi_write(EdtDev *edt_p, int addr, int wcnt);
static int spi_rreg(EdtDev *edt_p);
static void spi_wreg(EdtDev *edt_p, int v);
static int spi_rdsr(EdtDev *edt_p);
static void spi_wrsr(EdtDev *edt_p, int val);
static void usage();
static int spir(EdtDev *edt_p, u_int addr, int cnt);
static void spiprim(EdtDev *edt_p, int opcode, int wbytes, int rbytes, unsigned char* wbuf, unsigned char* rbuf);

unsigned char  wbufa[260];		/* Need 3 addr + 256 data for PP */
unsigned char* wbuf=wbufa+3;		/* Just the 256 bytes of data */
unsigned char  rbuf[260];		/* Read buffer, could use 256 Kbytes? */

static u_short dbgflg =0;
static u_short verbose = 0;
static u_short use_mmap = 0;
static u_short just_testing = 0;


/*
   Should consider making foicount=-1 at initialization, -2 on error.
   The value of 0 is valid when a loopback cable is installed.

   Make it possible to say "pdb 0" ?

   Recent changes to pcifoi xilinx:
   made ROMD[] output mux flip on what register last written (in romdecode)
   cleaned up how reset strobe cleared that same register (in macio)
   disabled incomming broadcast command packets (in cmdin)
   hacked RXRBABC to ignore DIRIN

   currently have ignoreDIRIN, and ignoreUNIT plus DIRIN to be matched
   should be a way to ignore broadcast packets?  No

   rp assumes incomming packet is printable ascii
   need wpt for text out, assumes routing byte of 0x00

   should remov 150 ohm resistor from SD on pcifoi,
   must move resistor away from optical tranciever

##Old code to read boot strap and bus voltage
else if	(strcmp(tbuf, "rp") == 0) {    
    uint_t  rdval;
    u_char  stat;
    xw(EDT_FLASHROM_DATA, 0);
    xw(EDT_FLASHROM_DATA, BT_EN_READ | BT_READ);
    rdval = xr(EDT_FLASHROM_DATA);
    stat = (u_char) rdval;
    printf("pal:%02x\n", stat);
    xw(EDT_FLASHROM_DATA, BT_A0 | BT_A1);
}

 */


#ifndef _NT_

#ifdef __linux__

#include <asm/ioctls.h>

#endif /* __linux__ */

#endif /* ndef _NT_ */

int wait_for_first = 0 ;
int runit = 0;
static char sendmsg_buf[512] ;
static char msgbuf[512] ;

/*
 * mmap stuff
 */
volatile caddr_t Mapaddr = 0;


/*
 * mmap interface read/write, modified to work on a per-channel
 * basis (PCI DV C-Link / PCI DV FOX)
 *
 * Note: NOT SAFE! Specifically other programs that access the board
 * should NOT be run while these subroutines are being used. But...
 * this is way faster. So just use with extreme care.
 */
    u_char
rcxload_mmap_intfc_read(EdtDev *edt_p, u_int desc)
{
    caddr_t off_p ;
    caddr_t data_p ;
    u_char val ;

    /* interface must set offset then read data */
    off_p = Mapaddr + (EDT_REMOTE_OFFSET & 0xff) + (EDT_CL_INTFC_OFFSET * edt_p->channel_no);
    data_p = Mapaddr + (EDT_REMOTE_DATA & 0xff) + (EDT_CL_INTFC_OFFSET * edt_p->channel_no);
    *off_p = desc & 0xff ;
    /* to flush */
    val = *off_p ;
    val = *data_p ;
    return(val) ;
}

    void
rcxload_mmap_intfc_write(EdtDev *edt_p, u_int desc, u_char val)
{
    caddr_t off_p ;
    caddr_t data_p ;
    u_char tmp ;

    /* interface must set offset then read data */
    off_p = Mapaddr + (EDT_REMOTE_OFFSET & 0xff) + (EDT_CL_INTFC_OFFSET * edt_p->channel_no);
    data_p = Mapaddr + (EDT_REMOTE_DATA & 0xff) + (EDT_CL_INTFC_OFFSET * edt_p->channel_no);
    *off_p = desc & 0xff ;
    /* to flush */
    tmp = *off_p ;
    *data_p = val ;
}


    u_int
wswap(u_int val)
{
    return (
            ((val & 0x0000ffff) << 16)
            | ((val & 0xffff0000) >> 16));
}

    void
byteswap(u_char *buf, int size)
{
    swab((char *)buf,(char *)buf,size) ;
}


    void
shortswap(u_char *buf, int size)
{
    u_int *tmpp ;
    u_int *endp ;
    tmpp = (u_int *)buf ;
    endp = tmpp + (size / 4) ;
    while (tmpp < endp)
    {
        *tmpp = wswap(*tmpp) ;
        tmpp++ ;
    }
}



    void
pw8(EdtDev *edt_p, int addr, int value)
{
    if (verbose > 1)
        printf("pw8(%x, %x)\n", addr, value);

    if (!just_testing)
    {
        if (use_mmap)
        {
            if (Mapaddr == 0)
            {
                if ((Mapaddr = edt_mapmem(edt_p, 0, 256)) == 0)
                    exit(1);
                printf("Mapaddr %p\n", Mapaddr);
            }
            rcxload_mmap_intfc_write(edt_p, (u_int)addr, (u_char)value) ;
        }
        else
            edt_intfc_write(edt_p, (u_int)addr, (u_char)value) ;
    }
}

    u_short
pr8(EdtDev *edt_p, int addr)
{
    u_short ret;

    if (verbose > 1)
        printf("pr8(%05x)\n", addr);

    if (just_testing)
        return 0;

    if (use_mmap)
    {
        if (Mapaddr == 0)
        {
            if ((Mapaddr = edt_mapmem(edt_p, 0, 256)) == 0)
                exit(1);
            printf("Mapaddr %p\n", Mapaddr);
        }
        ret = rcxload_mmap_intfc_read(edt_p, (u_int)addr) ;
    }
    else ret = edt_intfc_read(edt_p, (u_int)addr) ;

    return ret;
}

int gettok(char *tbuf, char **pbufp)		/* use strtok_r() instead? */
{
    int c;

    do   {  
        c = *(*pbufp)++;   
        if (c=='\0')  { *tbuf = 0;  return(0);  }
    } while (isspace(c));

    do  { 
        *tbuf++ = c;
        c = *(*pbufp)++;   
        if (c=='\0')  { *tbuf = 0; return(1);  }
    }  while (!isspace(c));

    *tbuf=0;
    return(1);
}

/***************************************************************************/
/* Instruction set for ST M25P20 SPI Flash Memory used on EDT TLKRCI */
#define SPI_WREN  0x06	/* Write Enable,  0 params                   */
#define SPI_WRDI  0x04	/* Write Disable, 0 params                   */
#define SPI_RDSR  0x05	/* Read Status,   0 wbytes,         1 rbyte  */
#define SPI_WRSR  0x01	/* Write Status,  1 wbyte                    */
#define SPI_READ  0x03	/* Read data,     3 addr,           n rbytes */
#define SPI_FREAD 0x0B	/* Fast Read,     3 addr + 1 dummy, n rbytes */
#define SPI_PP    0x02	/* Page Program,  3 addr + 1-256 wbytes      */
#define SPI_SE    0xD8	/* Sector Erase,  3 addr                     */
#define SPI_BE    0xC7	/* Bulk Erase,    0 params                   */
#define SPI_DP    0xB9	/* Power Down,    0 params                   */
#define SPI_RES   0xAB	/* Read Sig,      3 wdummy,         1 rbyte  (DP off)*/


/* Bit definitions for register IO to SPI control lines */
#define SPI_WREG  0x07		/* Write bits out to aiag Mode Reg */
#define SPI_EN    0x00		/* was 0x08,    Enable S,D,C out to PAL */
#define SPI_S     0x04		/* Select, low true */
#define SPI_D     0x02		/* Data Write */
#define SPI_C     0x01		/* Clock */

#define SPI_RREG  0x01		/* Read bit from aiag Status Reg */
#define SPI_Q     0x02		/* Data is read from FV bit */

    void
spi_wreg(EdtDev *edt_p, int v)	/* Write to IO port to drive S,C,D pins */
{  /* Make bit 3 parity over bits 1,2, gets checked by RCX-CLink Xilinx */
    int p;  p=((v>>2)&1)+((v>>1)&1);  v=(v&0xf7)|((p&1)<<3);
    if (dbgflg&0x100)    v |= 0x08;

    if (dbgflg&0x200)    
        printf("%d %d %d %d   ", (v>>3)&1, (v>>2)&1, (v>>1)&1, (v>>0)&1);

    if ((dbgflg&0x80) == 0)
        pw8(edt_p, SPI_WREG, SPI_EN | v);
}

int spi_rreg(EdtDev *edt_p)		/* Read the IO port to get Q pin status */
{
    int val;
    static int fakein = 0x456789AB;

    if (dbgflg&0x80) {
        fakein = ((fakein<<1) & 0xFFFFFFFE) | ((fakein>>31) & 1); 
        val = fakein & 1;
    }
    else    {
        val = pr8(edt_p, SPI_RREG) & SPI_Q;
    }
    if (val)  val=1;

    if (dbgflg & 0x10) {
        printf("        %d    %08x\n", val, fakein);
    }
    return(val);
}


    void
spiprim(EdtDev *edt_p, int opcode, int wbytes, int rbytes, unsigned char* wbuf, unsigned char* rbuf) 
{
    int c, b, cdat;

    spi_wreg(edt_p, SPI_S);		/* Make sure Select is high */
    spi_wreg(edt_p, 0);		/*  then lower the Select line to select device */

    cdat = opcode;
    for (b=0; b<8; b++)  {		/* for each bit in the char */
        if (cdat & 0x80) {
            spi_wreg(edt_p, SPI_D);
            spi_wreg(edt_p, SPI_D | SPI_C);
        } else {
            spi_wreg(edt_p, 0);
            spi_wreg(edt_p, SPI_C);
        }
        cdat <<= 1;
    }

    for (c=0; c<wbytes; c++) {		/* for each char in wbuf */
        cdat = wbuf[c];
        for (b=0; b<8; b++)  {		/* for each bit in the char */
            if (cdat & 0x80) {
                spi_wreg(edt_p, SPI_D);
                spi_wreg(edt_p, SPI_D | SPI_C);
            } else {
                spi_wreg(edt_p, 0);
                spi_wreg(edt_p, SPI_C);
            }
            cdat <<= 1;
        }
    }
    spi_wreg(edt_p, 0);

    for (c=0; c<rbytes; c++) {		/* for each char in rbuf */
        cdat = 0;
        for (b=0; b<8; b++)  {		/* for each bit in the char */
            cdat <<= 1;
            if (spi_rreg(edt_p) & 1)  
                cdat |= 1;
            if ((b!=7) || (c!=(rbytes-1)))  {
                spi_wreg(edt_p, SPI_C);
                spi_wreg(edt_p, 0);
            }
        }
        rbuf[c] = cdat;
    }

    spi_wreg(edt_p, SPI_S);		/* Raise the Select line (to deselect) */
}


    int
spi_res(EdtDev *edt_p)		 	           /* Read electronic signature */
{
    unsigned char rval;
    char *magic = "555";   		/* For M25P20 should be 0x11/0x13 */

    if (verbose)
        printf("spi_res()\n"); 

    if (just_testing)
        return 0x13; /* fake */

    spiprim(edt_p, SPI_RES, 3, 1, (unsigned char *)magic, &rval);
    return(rval&0xff);                      /* Also out of Power Down mode  */
}

int spi_rdsr(EdtDev *edt_p)
{
    unsigned char rval;
    spiprim(edt_p, SPI_RDSR, 0, 1, NULL, &rval);    	/* Read Status reg */
    return(rval&0xff);
}

void spi_wrsr(EdtDev *edt_p, int val)
{
    unsigned char wval;
    wval = val;
    spiprim(edt_p, SPI_WREN, 0, 0, NULL, NULL);		/* write-enable */
    spiprim(edt_p, SPI_WRSR, 1, 0, &wval, NULL);    	/* Write Status reg */
}


/* Read rcnt bytes starting from address addr, store in global rbuf[] */
void spi_read(EdtDev *edt_p, int addr, u_char *rbuf, int rcnt)
{
    if ((rcnt<1) || (rcnt > 256))   {
        printf("Error, bad spi_read() rcnt of %d\n", rcnt);
        exit(1);
    }
    wbufa[0]=addr>>16; wbufa[1]=addr>>8; wbufa[2]=addr;     /* 24 bit address */
    wbufa[3]=0;  				            /* dummy byte */
    spiprim(edt_p, SPI_FREAD, 4, rcnt, wbufa, rbuf);    
}

void spi_write(EdtDev *edt_p, int addr, int wcnt)
{
    int statcnt;

    if (verbose) {
        printf("spi_write(%05x %04x)\n", addr, wcnt); 
    }
    if ((wcnt<1) || (wcnt>256))   {
        printf("Error, bad spi_write() wcnt of %d\n", wcnt);
        exit(1);
    }
    spiprim(edt_p, SPI_WREN, 0, 0, NULL, NULL);		    /* write-enable */
    wbufa[0]=addr>>16; wbufa[1]=addr>>8; wbufa[2]=addr;     /* 24 bit address */

    /***debug***/ 
    if (dbgflg&1) {
        int n; 
        for (n=0; n<10; n++) 
            printf("%02x ", wbufa[n]);  
        printf("\n"); 
    }

    spiprim(edt_p, SPI_PP,  3+wcnt, 0, wbufa, NULL);		    /* page program */

    statcnt=0;
    do {
        spiprim(edt_p, SPI_RDSR, 0, 1, NULL, rbuf);    	/* Read Status reg */
        statcnt++;
    } while ((rbuf[0] & 1) == 1);		/* Could hang here forever? */
    if (dbgflg & 1) {
        printf("Final status was %02x, read %d times\n",  rbuf[0], statcnt);
    }
}

#define TOO_MANY 100000

    int
spi_erase(EdtDev *edt_p, int addr)
{
    int statcnt;
    if (verbose) {
        printf("spi_erase(%05x)\n", addr);
    }

    if (just_testing)
        return (0);

    spiprim(edt_p, SPI_WREN, 0, 0, NULL, NULL);		    /* write-enable */
    wbufa[0]=addr>>16; wbufa[1]=addr>>8; wbufa[2]=addr;     /* 24 bit address */
    spiprim(edt_p, SPI_SE,  3, 0, wbufa, NULL);    

    statcnt=0;
    do {
        spiprim(edt_p, SPI_RDSR, 0, 1, NULL, rbuf);    	/* Read Status reg */
        if (statcnt++ > TOO_MANY)
        {
            printf("erase spun for too long, bailing out\n");
            return(-1);
        }
    } while ((rbuf[0] & 1) == 1);		/* Could hang here forever? */
    /* ALERT also need to check all 8 status bits says JG */ 
    if (verbose) {
        printf("Final status was %02x, read %d times\n",  rbuf[0], statcnt);
    }

    return(0);
}


spir(EdtDev *edt_p, u_int addr, int cnt)
{
    int val;
    u_int n;
    char pbuf[32];	  		/* For ASCII printing */
    char* p;
    char* pbufp;
    u_char *rb = rbuf;
    FILE   *xfd = NULL;

    rbuf[0] = '\0';

    if (verbose)
        printf("spir(%05x, %d)\n", addr, cnt);

    if (just_testing)
    {
        strcpy((char *)rbuf, "fud03");
        return 0;
    }

    /* Firmware needs ok parity for 256 SPI clocks to unlock access */
    for (n=0; n<300/(5*8); n++)  spi_res(edt_p);   /* 5 bytes per spi_res */

    val = spi_res(edt_p);
    if ((val != 0x11) && (val != 0x13))  {
        if (verbose)
            printf("spi_res returned 0x%02x, not 0x11/0x13\n", val);
        return -1;
    }

    for (n=0; n<16; n++) pbuf[n]=0x20;		/* init print buffer */
    pbufp = pbuf+(addr&0xf);
    if (verbose)
        for (n=0; n<(addr&0xf); n++) printf("   ");

    while (cnt > 0) {
        spi_read(edt_p, addr, rb, 16);
        rb += 16;
        for (n=0; n<16; n++)
        {
            if ((addr & 0xf)== 0)
            {
                if (verbose)
                    printf("    #%05x  ", addr-16);
                p = pbuf;
                while (p<pbufp) {
                    if (verbose)
                    {
                        if ((*p>=0x20) && (*p<0x7f))
                            putchar(*p);
                        else  putchar('.');
                    }
                    p++;
                }
                pbufp = pbuf;
                if (verbose)
                    putchar('\n');
            }
            if (verbose)
                printf("%02x ", rbuf[n]);
            *pbufp++ = rbuf[n];
            addr++;  cnt--;
            if (cnt==0) break;
        }
    }
    /* printf("\n"); */
    return 0;
}


spie(EdtDev *edt_p, int sect)
{
    int val, ret=0;

    if (verbose)
        printf("spie(%05x)\n", sect);

    val = spi_res(edt_p);
    if ((val != 0x11) && (val != 0x13))
    {
        printf("Error, spi_erase: spi_res returned 0x%02x, not 0x11/0x13\n", val);
        return(-1);
    }

    if ((sect>=0x0) && (sect<=0xf0000))
    {
        if (verbose)
            printf("erasing sect %05x\n", sect);
        return spi_erase(edt_p, sect);
    }
    else
    {
        printf("ERROR:  spie saw bad sector number %05x\n", sect);
        return (-1);
    }
}

spiw(EdtDev *edt_p, int addr0, int size, u_char *spibuf, int verify_only)
{
    int addr;
    int n;
    int bcnt, errorcnt, val, first, last;

    if (verbose)
        printf("spiw(%05x, %d, [%02x, %02x, %02x, %02x, %02x, %02x, ...])\n\n", addr0, size, spibuf[0], spibuf[1], spibuf[2], spibuf[3], spibuf[4], spibuf[5]);

    if (just_testing)
        return (0);

    val = spi_res(edt_p);	/* Read_electronic_signature from spi flash */

    if ((val != 0x11) && (val != 0x13)) {
        printf("    Error, did not detect SPI flash device, \n");
        printf("      spi_res() returned 0x%02x, not 0x11/0x13\n", val);
        exit(1); /* really? */
    }
    if (verbose)
        printf("    spi_res() returned 0x%02x\n", val);

    if (!verify_only)
    {
        first = (addr0&0xf0000)>>16;
        last  = ((addr0+size)&0xf0000)>>16;
        printf("First sector is %d, last is %d\n", first, last);
        fflush(stdout);

        printf("    Writing %d bytes to spiflash addr %05x... ",size,addr0);
        fflush(stdout);
        bcnt=0;  addr=addr0;
        while (bcnt<size)  {
            n=0;
            while (bcnt<size)  {
                if ((n!=0) && (((addr+n)&0xff)==0)) break;  /* page end */
                wbuf[n] = spibuf[bcnt]; 
                n = n+1;
                bcnt = bcnt+1;
            }
            if (n>0) spi_write(edt_p, addr, n);	/* Write 1-256 bytes from wbuf*/
            addr = addr + n;
        };
        printf("done\n");
    }

    printf("    Verifying %d bytes at spiflash addr %05x... ",size,addr0);
    fflush(stdout);
    bcnt=0;  addr=addr0; errorcnt=0;
    while (bcnt<size)  {
        spi_read(edt_p, addr, rbuf, 256);
        n=0;
        while (bcnt<size)  {
            if ((n!=0) && (((addr+n)&0xff)==0)) break;  /* page end */
            if ((rbuf[n]&0xff) != (spibuf[bcnt]&0xff)) {
                if (errorcnt++ <= 10) {
                    printf("Error at %05x: read:%02x, s/b %02x\n", 
                            addr0+bcnt, rbuf[n], spibuf[bcnt]);
                }
            }
            n = n+1;
            bcnt = bcnt+1;
        }
        addr = addr + n;
    }
    printf("%d errors\n", errorcnt);

    if (errorcnt > 0)
        return -1;

    return(0);
}

    void
usage(char *progname, char *msg)
{
    if (*msg)
        puts(msg);
    printf("%s: Update firmware on EDT RCX C-Link remote fiber optic module, through\n", progname);
    printf("an EDT PCI DV series Camera Link or Fiber (DVFOX) interface board. RCX C-Link\n");
    printf("module should be powered on, set to blink code 3-3, and connected to the inter-\n");
    printf("face board with a fiber optic cable.\n\n");
    printf("usage: %s -u unit [options] [filename]\n", progname);
    printf("  -u unit     unit number of device (default 0)\n");
    printf("  -c channel  channel number of device (default 0)\n");
    printf("  -d dirname  base directory name for interface board FPGA files (if specified)\n");
    printf("  -b          force load of interface board  FPGA (otherwise will do so only if\n");
    printf("              not already loaded)\n");
    printf("  -n          force no bitload of interface board FPGA\n");
    printf("  -y          don't wait for ENTER to begin\n");
    printf("  -V          verbose\n");
    printf("  -v fname    verify RCX against given .rcx filename\n");
    printf("  [filename]  specifies the .rcx file to load, typically flash/rcx/<file>.rcx\n");
    printf("  If no filename given, reads & lists the currently loaded RCX module firmware\n");

    exit(1);
}

/* check string for comment or whitespace only; return 1 if is, 0 if not */
    int
comment_or_whitespace(char *str)
{
    char *p = str;

    while (*p)
    {
        if ((*p == '\n') || (*p == '\r') || (*p == '#'))
            return 1;
        else if ((*p != ' ') && (*p != '\t'))
            return 0;
    }
    return 1;
}

int uartcnt=0;

char fname[128];
char   edt_devname[256] = "";
char   errbuf[32];
char *progname;

typedef struct block_struct {
    int number;
    BLOCK_HEADER header;
    u_char *data;
    struct block_struct *next;
} BLOCK_ENTRY;


    char *
get_line(char *bufp, char *line, int *nb)
{
    char *p = bufp;
    char *lp = line;
    int leading_whitespace = 1;

    memset(line, '\0', 256);

    while (*p)
    { 
        if (*p == '\n')
        {
            *lp = '\0';
            ++(*nb);

            return (++p);
        }

        if (*p != ' ')
            leading_whitespace = 0;

        if (!leading_whitespace)
            *lp++ = *p;
        ++p;
        ++(*nb);
    }
    *lp = '\0';

    return p;
}

/* for verify only, change any burn commands to verify */
    void
change_burn_to_verify(char *buf)
{
    char tmpbuf[256];
    char tbuf[256];
    char *bufp = buf;

    if (gettok(tbuf, &bufp) == 0)
        return;

    if (strcmp(tbuf, "burn") == 0)
    {
        strcpy(tmpbuf, bufp);
        sprintf(buf, "verify %s\n", tmpbuf);
    }
}


error_out(int ln, char *msg)
{
    printf("command error on line %d: %s, bailing out\n", ln, msg);

    exit(1);
}

    void
swap_block_header(BLOCK_HEADER *bh)
{
    byteswap((u_char *)&(bh->magic), 4);
    shortswap((u_char *)&(bh->magic), 4);
    byteswap((u_char *)&(bh->bytecount), 4);
    shortswap((u_char *)&(bh->bytecount), 4);
    byteswap((u_char *)&(bh->cksum), 4);
    shortswap((u_char *)&(bh->cksum), 4);
}

    void
print_block_header(BLOCK_HEADER *bh)
{
    printf("  magic no:  %08x (%s)\n", bh->magic, bh->magic == RCXMAGIC_BIN? "binary":"ascii");
    printf("  name:     '%s'\n", bh->name);
    printf("  bytecount: %08x\n", bh->bytecount);
    printf("  cksum:     %08x\n", bh->cksum);
}

    void
print_block_entry(BLOCK_ENTRY *be)
{
    printf("block %d:\n", be->number);
    print_block_header(&be->header);
    if (be->header.magic == RCXMAGIC_ASC)
        printf("  data:\n%s", be->data);
    else printf("  data:      %02x %02x %02x %02x %02x %02x %02x %02x ... %02x\n", be->data[0], be->data[1], be->data[2], be->data[3], be->data[4], be->data[5], be->data[6], be->data[7], be->data[be->header.bytecount-1]);
    printf("--------------------------------\n\n");
}

    void
scary_warning(int unit, int channel, char *edt_devname, int yes)
{

    char s[16];

    puts("");
    puts("Preparing to rewrite the firmware flash prom on the RCX C-Link");
    printf("module connected to channel %d of %s unit %d. Once started, the\n", channel, edt_devname, unit);
    puts("firmware update should NOT be interrupted.\n");
    puts("");
    puts("IMPORTANT: If errors occur during a CRITICAL BURN section, do NOT");
    puts("cycle power on the module -- leave the module on and try again. If");
    puts("errors occur outside a CRITICAL BURN section, cycle power on the");
    puts("module before trying again.");
    puts("");
    if (!yes)
    {
        printf("press <ENTER> to continue, 'c' to cancel > ");
        fflush(stdout);
        fgets(s, 15, stdin);
        if (*s && (*s != '\n'))
            exit(0);
    }
}


/*
 * ************************* main module ***********************
 */
main(int argc, char **argv)
{
    EdtDev *edt_p ;
    char fh[68];
    char *dir_arg = "camera_config";
    char *bitfile = "aiagcl.bit";
    int ret, bi=0, n, bn=0, numblocks, verify_only=0;
    BLOCK_ENTRY *bl_new, *bl_head = NULL, *bl_last, *bl_cmd, *bp;
    BLOCK_HEADER block_header;
    u_char *bd;
    u_int flashaddr;
    char errstr[256];
    char goto_label[64];
    int   ln=0;
    int   yes = 0, firsttime = 1;
    int    unit=0, channel=0;
    int    do_bitload = 0; /* neutral */
    char   *unitstr = "";
    char errmsg[128];
    FILE *fp = NULL;

    progname = argv[0];
    errmsg[0] = '\0';

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
                    usage(progname, "Error: option 'u' requires a numeric argument\n");
                }
                if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
                {
                    unitstr = argv[0];
                    unit = edt_parse_unit_channel(unitstr, edt_devname, "pdv", &channel);
                }
                else 
                {
                    usage(progname, "Error: option 'u' requires a numeric argument\n");
                }
                break;

            case 'c':		/* channel */
                ++argv;
                --argc;
                if (argc < 1) 
                {
                    usage(progname, "Error: option 'c' requires a numeric argument\n");
                }
                if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
                {
                    channel = atoi(argv[0]);
                }
                else 
                {
                    usage(progname, "Error: option 'c' requires a numeric argument\n");
                }
                break;

            case 'd':		/* bitfile dir */
                ++argv;
                --argc;
                dir_arg = argv[0];
                break;

            case 'b':		/* bitload */
                do_bitload = 1;
                break;

            case 'n':		/* don't do bitload */
                do_bitload = -1;
                break;

            case 'y':		/* don't wait for enter to begin */
                yes = 1;
                break;

            case 'm':		/* use mmap (not working? */
                use_mmap = 1;
                break;

            case 'v':		/* verify */
                verify_only=1;
                break;

            case 'V':		/* debug (output debug info) */
                verbose=1;
                if ((argv[0][2]) && (argv[0][2] == 'V'))
                    verbose = 2;
                break;

            case 't':		/* testing only */
                just_testing=1;
                break;

            default:
                sprintf(errmsg, "unknown flag '-%c'\n", argv[0][1]);
            case '?':
            case 'h':
                usage(progname, errmsg);
                exit(0);
        }
        argc--;
        argv++;
    }


    fname[0] = '\0';
    if (argc)
    {
        strcpy(fname, argv[0]);
        if ((fp = fopen(fname, "rb")) == NULL)
        {
            edt_perror(fname);
            exit(1);
        }
    }

    if ((edt_p = edt_open_channel(EDT_INTERFACE, unit, channel)) == NULL)
    {
        sprintf(errstr, "edt_open_channel(%s%d_%d)", edt_devname, unit, channel);
        edt_perror(errstr);
        exit (1);
    }

    if (!just_testing)
    {
        edt_bitpath pathbuf ;

        if (!(edt_is_dvfox(edt_p) || edt_is_dvcl(edt_p)))
        {
            printf("pdv%d is not an EDT Camera Link framegrabber board\n", unit);
            exit(0);
        } 

        if (edt_is_dvcl(edt_p))
        {
            do_bitload = 0;
            strcpy(pathbuf, "no_bitfile");
        }
        else edt_get_bitpath(edt_p, pathbuf, sizeof(edt_bitpath)) ;

        if ((do_bitload > 0) || ((do_bitload == 0) && (strlen(pathbuf) < 4)))
        {
            printf("loading bitfile xilinx %s....\n", bitfile);
            if ((ret = edt_bitload(edt_p, dir_arg, bitfile, 0, 0)) != 0)
            {
                printf("bitload failed with error %d; exiting\n", ret);
                exit(ret);
            }
            edt_flush_fifo(edt_p);
        }
        edt_msleep(700);
        edt_close(edt_p);
    }


    /* if no file arg, just open the device and ping the RCX(s) */
    if (!fp)
    {
        int i, nunits = 0, saw_rcx = 0;
        u_int ids[] = {PDVFOX_ID, PE4DVAFOX_ID, PE8DVAFOX_ID, PDVCL_ID, PE4DVVLFOX_ID, PE4DVCL_ID, PE8DVCL_ID, PE1DVVL_ID, PE4DVVL_ID, 0xFFFF};
        Edt_bdinfo *boards = edt_detect_boards_ids(NULL, -1, ids, &nunits, verbose);

        for (i=0; i<nunits; i++)
        {
            int num_channels = 4;

            if (boards == NULL)
                continue;

            channel = 0;
            while (channel < num_channels)
            {
                if ((edt_p = edt_open_channel(EDT_INTERFACE, boards[i].id, channel)) == NULL)
                {
                    sprintf(errstr, "edt_open_channel(%s%d_%d)", edt_devname, boards[i].id, channel);

                    edt_perror(errstr);
                }
                else
                {
                    int firstone = 1;

                    if (edt_is_dvcl(edt_p))
                        num_channels = 2;

                    if (verbose)
                        printf("opened channel %d\n", edt_p->channel_no);

                    /* dva fox defaults to 2.5GB but RCXCL program mode is  1.25, so ... */
                    /* (what about PE4DVVLFOX? If ever) */
                    if (edt_p->devid == PE4DVAFOX_ID || edt_p->devid == PE8DVAFOX_ID)
                    {
                        u_short reg = pr8(edt_p, 0xf4);
                        pw8(edt_p, 0xf4, reg & ~0x01);
                    }

                    pw8(edt_p, 5,2) ;		/* go into "load RCX/RCX-CLink mode" */
                    if (spir(edt_p, 0, 10) == 0) /* read 16 bytes, unlocks spi flash for writes */
                    {
                        int sector=0;

                        flashaddr = 0x10000 ;
                        while (flashaddr < 0xd8001)
                        {
                            if (spir(edt_p, flashaddr, 128) == 0)
                            {
                                char header[256];
                                int size = 128;

                                if (firstone)
                                {
                                    printf("%s unit %d (%s) chan. %d RCX module:\n", boards[i].type, boards[i].id,
                                            edt_idstr(boards[i].bd_id), channel);
                                    firstone = 0;
                                }
                                saw_rcx = 1;

                                if (edt_get_x_array_header(rbuf, header, &size) == 0)
                                    printf(" sector %d [%05x]: <%s>\n", sector++, flashaddr, header);
                                else printf(" sector %d [%05x]: <no data>\n", sector++, flashaddr);
                            }
                            flashaddr += 0x28000;
                        }
                    }
                    else if (verbose) printf(" RCX %d %d not found -- make sure module is connected, powered on\n and set to blink code 3-3 (see Users Guide)\n", unit, channel);
                    edt_close(edt_p);
                }
                ++channel;
            }
        }
        if (!saw_rcx)
            printf(" No RCX modules found.\n Make sure module is connected, powered on and set to blink code 3-3\n");
        exit(0);
    }

    /* read the file description header (ASCII) */
    if ((n = fread(fh, 1, 64, fp)) != 64)
    {
        edt_perror(fname);
        puts("ERROR -- read failed, exiting");
        exit(1);
    }
    fh[64] = '\0';

    if (strncmp(fh, RCXMAGIC_FILE, 4) != 0)
    {
        puts("bad magic number (file), exiting");
        exit(1);
    }

#if 0 /* extract filename from RCX PROM here -- should be <fname>.rcx after magic no. and '# ' */
    if ((*fname == 0) && verify_only)
        sscanf(fh, "%s ", fname);
#endif

    if (verbose > 1)
        printf("\n%s\n", &fh[4]);

    /* read blocks until done */
    numblocks = 0;

    if (sizeof(block_header) != BLOCK_HEADER_SIZE) /* reality check */
    {
        printf("Incorrect block header size -- internal error, contact EDT\n");
        printf("exiting\n");
        exit(1);
    }

    while ((n = fread(&block_header, 1, 64, fp)) != 0)
    {
        if (n != sizeof(block_header))
        {
            printf("Truncated block header -- expected %d bytes, got %d)\n", (int)sizeof(block_header), (int)n);
            printf("exiting\n");
            exit(1);
        }

        if ((block_header.magic != RCXMAGIC_BIN)
                && (block_header.magic != RCXMAGIC_ASC))
            swap_block_header(&block_header);

        if ((block_header.magic != RCXMAGIC_BIN)
                && (block_header.magic != RCXMAGIC_ASC))
        {
            printf("Block %d: bad block magic number (%05x); exiting\n", numblocks,  block_header.magic);
            exit(1);
        }

        if (block_header.bytecount > 8)
        {
            if (verbose > 1)
                printf("alloc %d\n", block_header.bytecount);
            bd = malloc(block_header.bytecount);
        }
        else
        {
            if (verbose > 1)
                printf("alloc %d\n", 8);
            bd = malloc(8);
        }

        if ((n = fread(bd, 1, block_header.bytecount, fp)) != (int)block_header.bytecount)
        {
            printf("Data block truncated -- expected %0d read %d; exiting\n", block_header.bytecount, n);
            exit(1);
        }

        /*
         * block header read in okay, so now create and load new linked list entry
         */
        bl_new = (BLOCK_ENTRY *)malloc(sizeof(BLOCK_ENTRY));
        if (bl_head == NULL)
            bl_head = bl_last = bl_new;
        else bl_last->next = bl_new;
        memcpy((void *)&bl_new->header, &block_header, sizeof(block_header));
        if (block_header.magic != RCXMAGIC_ASC)
            bl_new->number = bi++;
        bl_new->data = bd;
        bl_new->next = NULL;
        bl_last = bl_new;
        ++numblocks;
    }

    /*
     * print out all list entries for debug/test
     */
    if (verbose > 1)
    {
        bp = bl_head;
        while (bp != NULL)
        {
            print_block_entry(bp);
            bp = bp->next;
        }
    }

    /*
     * now interpret command block(s)
     */
    bl_cmd = bl_head;
    if (bl_cmd->header.magic == RCXMAGIC_ASC)
    {
        int bytesread=0;
        char *bufp, *lp;
        char buf[256];
        char tbuf[256];
        char blockname[256];
        int  bytestread = 0;

        lp = (char *)bl_cmd->data;
        if (verbose > 1)
            printf("\nInterpreting commands:\n %s\n", lp);

        if ((edt_p = edt_open_channel(EDT_INTERFACE, unit, channel)) == NULL)
        {
            sprintf(errstr, "edt_open_channel(%s%d_%d)", edt_devname, unit, channel);
            edt_perror(errstr);
            exit (1);
        }

        pw8(edt_p, 5,2) ;		/* go into "load RCX/RCX-CLink mode" */
        if (spir(edt_p, 0, 10) != 0)	/* read 16 bytes, unlocks spi flash for writes */
        {
            if (!verbose)
                printf("spir returned error -- run with -d for debug info\n");
            exit(0);
        }

        *goto_label = '\0';

        while(bytesread < (int)bl_cmd->header.bytecount)
        {
            if (*lp == 0)
            {
                if (goto_label[0])
                {
                    sprintf(errstr, "goto label '%s' not found", goto_label);
                    error_out(ln, errstr);
                }
            }

            ++ln;
            lp = get_line(lp, buf, &bytestread);
            if (verify_only)
                change_burn_to_verify(buf);

            bufp = buf;

            if (gettok(tbuf, &bufp) == 0)  continue;    /* skip blank lines */

            /* first check for label if we previously got a goto (forward jumps only) */
            if (*goto_label)
            {
                if ((tbuf[0] == '@') && (strcmp(&tbuf[1], goto_label) == 0))
                {
                    *goto_label = '\0';
                    if (verbose)
                        printf("goto: jumped to %s\n", tbuf);
                }
                else
                {
                    if (verbose > 1)
                        printf("jumping over '%s'\n", buf);
                }
                continue;
            }

            /* burn blockname flashaddr */
            if (strcmp(tbuf, "burn") == 0)
            {
                int i;

                if (firsttime && !verify_only)
                {
                    scary_warning(unit, channel, "pdv", yes);
                    firsttime = 0;
                }


                printf("> %s\n", buf);

                if (gettok(tbuf, &bufp))
                {
                    if (sscanf(tbuf,"%x", &flashaddr) != 1)
                        error_out(ln, "burn (bad flashaddr argument)");

                    if (gettok(tbuf, &bufp))
                    {
                        if (sscanf(tbuf,"%s", (char *)&blockname) != 1)
                            error_out(ln, "burn (bad block name)");

                        bp = bl_cmd->next;
                        for (i=0; i<numblocks; i++)
                        {
                            if (strcmp(blockname, bp->header.name) == 0)
                                break;
                            bp = bp->next;
                        }
                        if (bp == NULL)
                        {
                            fprintf(stderr, "ERROR: command: '%s' -- binary file label '%s' not found\n", buf, blockname);
                            exit(1);
                        }
                        else if (bp->header.magic != RCXMAGIC_BIN)
                        {
                            fprintf(stderr, "ERROR -- burn command specified non-binary block:\n");
                            fprintf(stderr, "  '%s'; aborting\n", blockname);
                            exit(1);
                        }

                        /* write/verify a bitfile to flash */
                        if (verbose)
                            printf("%s\n", bp->header.name);
                        if (spiw(edt_p, flashaddr, bp->header.bytecount, bp->data, 0) != 0)
                            exit (1);
                    }
                    else error_out(ln, "burn (bad or missing blockname argument)");
                }
                else error_out(ln, "burn (bad or missing flashaddr argument)");
            }

            else if (strcmp(tbuf, "verify") == 0)
            {
                int i;

                printf("> %s", buf);

                if (gettok(tbuf, &bufp))
                {
                    if (sscanf(tbuf,"%x", &flashaddr) != 1)
                        error_out(ln, "verify (bad flashaddr argument)");

                    if (gettok(tbuf, &bufp))
                    {
                        if (sscanf(tbuf,"%s", (char *)&blockname) != 1)
                            error_out(ln, "verify (bad block name)");

                        bp = bl_cmd->next;
                        for (i=0; i<numblocks; i++)
                        {
                            if (strcmp(blockname, bp->header.name) == 0)
                                break;
                            bp = bp->next;
                        }
                        if (bp == NULL)
                        {
                            fprintf(stderr, "ERROR: command: '%s' -- binary file label '%s' not found\n", buf, blockname);
                            exit(1);
                        }
                        else if (bp->header.magic != RCXMAGIC_BIN)
                        {
                            fprintf(stderr, "ERROR -- verify command specified non-binary block:\n");
                            fprintf(stderr, "  '%s'; aborting\n", blockname);
                            exit(1);
                        }

                        /* verify a bitfile in flash */
                        if (verbose)
                            printf("%s\n", bp->header.name);
                        if (spiw(edt_p, flashaddr, bp->header.bytecount, bp->data, 1) != 0)
                        {
                            printf("IMPORTANT: If errors occurred during a CRITICAL BURN secton, leave the module powered ON and call EDT before proceeding\n");
                            exit (1);
                        }
                    }
                    else error_out(ln, "verify (bad or missing blockname argument)");
                }
                else error_out(ln, "verify (bad or missing flashaddr argument)");
            }

            /* echo string */
            else if (strcmp(tbuf, "echob") == 0)
            {
                if (!verify_only)
                    puts(bufp);
            }

            /* echo string if verify only */
            else if (strcmp(tbuf, "echov") == 0)
            {
                if (verify_only)
                    puts(bufp);
            }

            /* echo string regardless */
            else if (strcmp(tbuf, "echo") == 0)
                puts(bufp);

            /* erase flashaddr */
            else if (strcmp(tbuf, "erase") == 0)
            {
                if (firsttime && !verify_only)
                {
                    scary_warning(unit, channel, "pdv", yes);
                    firsttime = 0;
                }

                if (gettok(tbuf, &bufp))
                {
                    if (!verify_only)
                    {
                        printf("> %s\n", buf);
                        if (sscanf(tbuf,"%x", &flashaddr) != 1)
                            error_out(ln, "erase (bad or missing flashaddr)");
                        if (flashaddr % 0x10000)
                            error_out(ln, "erase (not a page-aligned address)");
                        if (spie(edt_p, flashaddr) < 0)
                            error_out(ln, ""); 
                    }

                }
                else error_out(ln, "erase (expected 1 argument, got 0)");
            }

            /* goto */
            else if (strcmp(tbuf, "goto") == 0)
            {
                if (gettok(tbuf, &bufp))
                {
                    strcpy(goto_label, tbuf);

                    if (verbose)
                        printf("> %s\n", buf);
                    if (verbose > 1)
                        printf("jumping to @%s\n", goto_label);
                }
                else error_out(ln, "goto (label)");
            }

            /* if addr = str goto label */
            else if (strcmp(tbuf, "if") == 0)
            {
                if (gettok(tbuf, &bufp))
                {
                    if (sscanf(tbuf,"%x", &flashaddr) != 1)
                        error_out(ln, "if (bad or missing flashaddr)");
                    if (gettok(tbuf, &bufp) && strcmp(tbuf, "=") == 0)
                    {
                        if (gettok(tbuf, &bufp))
                        {
                            char tmpstr[260];
                            char str[32];

                            strcpy(tmpstr, tbuf);
                            if (tmpstr[0] == '"')
                            {
                                strcpy(str, &tmpstr[1]);
                                str[strlen(str)-1] = '\0';
                            }
                            else strcpy(str, tmpstr);

                            if (gettok(tbuf, &bufp) && strcmp(tbuf, "goto") == 0)
                            {
                                if (gettok(tbuf, &bufp))
                                {
                                    strcpy(goto_label, tbuf);

                                    if (verbose)
                                        printf("> %s\n", buf);
                                    if (verbose > 1)
                                        printf("'if' cmd decomposed:` flashaddr %05x str '%s' goto label '%s'\"\n", flashaddr, str, goto_label); 
                                    if (spir(edt_p, flashaddr, strlen(str)) != 0) /* read the string at the addr specified */
                                        printf("spir returned error (run with -d for debug info\n");
                                    strncpy(tmpstr, (char *)rbuf, strlen(str));
                                    tmpstr[strlen(str)] = '\0';
                                    if (strcmp(str, tmpstr) == 0)	/* saw the string, set the goto label */
                                    {
                                        if (verbose > 1)
                                            printf("read %s @ %05x; jumping to @%s\n", str, flashaddr, goto_label);
                                    }
                                    else
                                    {
                                        if (verbose)
                                            printf("*%05x \"%s\" != \"%s\";skipping goto\n", flashaddr, tmpstr, str);
                                        goto_label[0] = '\0';
                                    }
                                }
                                else error_out(ln, "if (label)");
                            }
                            else error_out(ln, "if (goto)");
                        }
                        else error_out(ln, "if (str)");
                    }
                    else error_out(ln, "if (=)");
                }
                else error_out(ln, "if (addr)");
            }

            /* end */
            else if ((strcmp(tbuf, "end") == 0) || (strcmp(tbuf, "exit") == 0) || (strcmp(tbuf, "quit") == 0) || (strcmp(tbuf, "done") == 0))
            {
                printf("> %s\n", buf);
                exit(0);
            }

            /* Comment or whitespace*/
            else if (comment_or_whitespace(tbuf))
            {
                /* comment; ignore */
            }
            else
            {
                printf("ERROR -- unknown command: %s\n", buf);
            }
        }
        edt_close(edt_p);
    }
    else
    {
        printf("first block %d not ASCII...?\n", bn++);
        exit(1);
    }

    exit(0);
}

