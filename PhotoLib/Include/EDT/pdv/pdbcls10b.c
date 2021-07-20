/* #pragma ident "@(#)pdb.c	1.64 10/28/10 EDT" */

#include "edtinc.h"

#include <ctype.h> /* isspace() */





int  spi_res();
void spi_rdid(char *rvals);
int  spi_rdsr();
void spi_wrsr(int val);
void spi_erase(int addr);
void spi_read(int addr,  int rcnt);
void spi_write(int addr, int wcnt);

unsigned char  wbufa[260];		/* Need 3 addr + 256 data for PP */
unsigned char* wbuf=wbufa+3;		/* Just the 256 bytes of data */
unsigned char  rbuf[260];		/* Read buffer, could use 256 Kbytes? */


int selsectn=0xE;



/*
Make it possible to say "pdb 0" ?

rp assumes incoming packet is printable ascii
need wpt for text out, assumes routing byte of 0x00


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

/**
* A convenience routine to access the EDT LCR_DEMOD registers.  Passed the Block
* and offset for a 32-bit word.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param block     integer register block selector
* @param offset    integer block offset
*
* @return The value of the 32-bit register.
*
* \Example
* u_int reg24 = edt_lcr_demod_read(edt_p, 2, 1);
*
* @see edt_lcr_demod_write, edt_reg_read.
*/


u_int
edt_lcr_demod_read(EdtDev * edt_p, u_int block, u_int offset)
{

    //u_int   regAddr = ((block & 0x0f) << 23) | (offset & 0x007fffff);
    u_int   regAddr = ((block & 0x0f) << 25) | (offset & 0x01ffffff);
    u_int data;

    //regAddr <<= 2;
 

    //if (edt_p->mapaddr == 0)

	//edt_p->mapaddr = ( caddr_t)edt_mapmem(edt_p, 0x10000, edt_get_mappable_size(edt_p, 1)) ;


    //printf("edt_lcr_demod_read:  addr %08x\n", regAddr);

    //*((unsigned int *)((u_char *)edt_p->mapaddr + 0xE0004)) = regAddr;
    edt_bar1_write(edt_p, 0xE0004, regAddr);

    //return *((unsigned int *)((u_char *)edt_p->mapaddr + 0xE0008));
    data = edt_bar1_read(edt_p, 0xE0008);
    //printf("edt_lcr_demod_read:  addr %08x data %08x\n", regAddr, data);
    return data;

}


/**
* A convenience routine to access the EDT LCR_DEMOD registers.  Passed the Block
* and offset for a 32-bit word.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param block     integer register block selector
* @param offset    integer block offset
* @param data      32-bit value to set register with.
*
* \Example
* @code
* u_int reg24 = 0xb01d_bee;
* edt_lcr_demod_write(edt_p, 2, 1, reg24);
* @endcode
*
* @see edt_demod_read, edt_reg_write.
*/

void
edt_lcr_demod_write(EdtDev * edt_p, u_int block, u_int offset, u_int data)
{

    //u_int   regAddr = ((block & 0x0f) << 23) | (offset & 0x007fffff);
    u_int   regAddr = ((block & 0x0f) << 25) | (offset & 0x01ffffff);


    //regAddr <<= 2;

    //printf("edt_lcr_demod_write:  addr %08x\n", regAddr);


//    if (edt_p->mapaddr == 0)

//	edt_p->mapaddr = ( caddr_t)edt_mapmem(edt_p, 0x10000, edt_get_mappable_size(edt_p, 1)) ;


    //printf("edt_lcr_demod_write:  addr %08x data %08x\n", regAddr, data);

    //*((unsigned int *)((u_char *)edt_p->mapaddr + 0xE0004)) = regAddr;
    edt_bar1_write(edt_p, 0xE0004, regAddr);

    //*((unsigned int *)((u_char *)edt_p->mapaddr + 0xE0008)) = data;
    edt_bar1_write(edt_p, 0xE0008, data);

}

/**
* A convenience routine to access the EDT MZDEMOD registers.  Passed the Block
* and offset for a 32-bit word.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param block     integer register block selector
* @param offset    integer block offset
*
* @return The value of the 32-bit register.
*
* \Example
* u_int reg24 = edt_mzdemod_read(edt_p, 2, 1);
*
* @see edt_mzdemod_write, edt_reg_read.
*/
// SAVE:  regVal = edt_reg_read(edt_p, EDT_MZDEMOD_REGISTER | ((block & 0x0f) << 23) | (offset & 0x007fffff));



u_int
edt_mzdemod_read(EdtDev * edt_p, u_int block, u_int offset)
{

    int i;
    int count       = 0;
    u_int   regAddr = ((block & 0x0f) << 25) | (offset & 0x01ffffff);
    u_int   tmpVal  = 0;
    u_int   regVal  = 0;


    // Clear interface enable bit
    edt_intfc_write(edt_p, 0x4c, 0x00);


    /* 27-bit address */
    edt_reg_write(edt_p, PCD_CMD | 0x04000000 | 0x44, regAddr);


    // Set interface enable and direction bits
    edt_intfc_write(edt_p, 0x4c, 0x01);


    // Wait for register data valid, but not too long
    while ((edt_intfc_read(edt_p, 0x4c) & 0x04) == 0)
	if (++count == 10)
	    break;


    /* 32-bit data */
    for (i = 0; i < 4; ++i)
    {

	tmpVal <<= 8;

	tmpVal |= edt_intfc_read(edt_p, 0x48 + (3 - i));

    }


    regVal = tmpVal;


    // Leave interface enable bit cleared
    edt_intfc_write(edt_p, 0x4c, 0x00);


    return regVal;

}



/**
* A convenience routine to access the EDT MZDEMOD registers.  Passed the Block
* and offset for a 32-bit word.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param block     integer register block selector
* @param offset    integer block offset
* @param data      32-bit value to set register with.
*
* \Example
* @code
* u_int reg24 = 0xb01d_bee;
* edt_mzdemod_write(edt_p, 2, 1, reg24);
* @endcode
*
* @see edt_demod_read, edt_reg_write.
*/
// SAVE:    edt_reg_write(edt_p, EDT_MZDEMOD_REGISTER | ((block & 0x0f) << 23) | (offset & 0x007fffff), data);

void
edt_mzdemod_write(EdtDev * edt_p, u_int block, u_int offset, u_int data)
{
 

    int i;
    u_int   regAddr = ((block & 0x0f) << 25) | (offset & 0x01ffffff);


    // Clear interface enable bit
    edt_intfc_write(edt_p, 0x4c, 0x00);


    /* 27-bit address */
    edt_reg_write(edt_p, PCD_CMD | 0x04000000 | 0x44, regAddr);



    /* 32-bit data */
    for (i = 0; i < 4; ++i)
    {

	edt_intfc_write(edt_p, 0x48 + i, data & 0xff);
	data >>= 8;

    }


    // Set interface enable and direction bits
    edt_intfc_write(edt_p, 0x4c, 0x03);
    edt_intfc_write(edt_p, 0x4c, 0x00);

}


volatile int throwaway_int;
volatile char *throwaway_charp;

EdtDev *edt_p ;
int verbose_reg_read = 0;


/* for addr=0x040100C0     04:byte_count 01:interface_Xilinx, C0:PCI_address */
void
xw(u_int addr, u_int data)
{
    u_int	dmy;
    edt_reg_write(edt_p, addr, data) ;
    dmy = edt_reg_read(edt_p, 0x04000080) ;	/*flush chipset write buffers*/
}

u_int
xr(u_int addr)
{
    return(edt_reg_read(edt_p, addr)) ;
}


#ifndef _NT_

#ifdef __linux__

#include <asm/ioctls.h>
#include <sys/ioctl.h>

#elif defined(VXWORKS)

#include "ioLib.h"

#else

#include <sys/filio.h>		/* for kbhitedt() */

#endif

static int kbhitedt()
{
    int numchars;
	/* int n, ch ; */
    ioctl(0, FIONREAD, &numchars) ;

    /* for (n=0; n<numchars; n++)  ch=getchar() ;*/
    return(numchars);
}

#else

#include <conio.h>

int kbhitedt()
{
    return(kbhit()) ;
}

#endif /* _NT_ */

int wait_for_first = 0 ;
int runit = 0;
static char sendmsg_buf[512] ;
static char msgbuf[512] ;

u_short
pr8(int addr)
{
	return(edt_intfc_read(edt_p, (u_int)addr)) ;
}

void
pw8(int addr, int value)
{
	edt_intfc_write(edt_p, (u_int)addr, (u_char)value) ;
}

/* See Indirect Register Access in edtreg.h */
u_short
ipr8(int addr)
{
	return(edt_reg_read(edt_p, (u_int)addr | INDREG_1BYTE)) ;
}

u_int
ipr16(int addr)
{
	return(edt_reg_read(edt_p, (u_int)addr | INDREG_2BYTE)) ;
}

u_int
ipr32(int addr)
{
	return(edt_reg_read(edt_p, (u_int)addr | INDREG_4BYTE)) ;
}

/* See Indirect Register Access in edtreg.h */
void
ipw8(int addr, int value)
{
	edt_reg_write(edt_p, (u_int)addr | INDREG_1BYTE, (u_char)value) ;
}

void
ipw16(int addr, int value)
{
	edt_reg_write(edt_p, (u_int)addr | INDREG_2BYTE, value) ;
}

void
ipw32(int addr, int value)
{
	edt_reg_write(edt_p, (u_int)addr | INDREG_4BYTE, value) ;
}


/* See Indirect Register Access in edtreg.h */
u_int
Ipr8(int addr)
{
	return(edt_reg_read(edt_p, (u_int)addr | IND2REG_TEST0)) ;
}

/* See Indirect Register Access in edtreg.h */
void
Ipw8(int addr, u_int value)
{
	edt_reg_write(edt_p, (u_int)addr | IND2REG_TEST0, (u_char)value) ;
}

u_short
pr16(int addr)
{
	return(edt_intfc_read_short(edt_p, (u_int)addr)) ;
}



/* The driver treats edt_intfc_write_short differently for PCIFOI,
 * setting it up as a command packet out to the RCI interface Xilinx.
*/

void
pw16(int addr, int value)
{
	edt_intfc_write_short(edt_p, (u_int)addr, (u_short)value) ;
}


u_int
pr_cfg(int addr)
{
    int ret ;
    edt_buf buf ;
    buf.desc = addr ;
    if ((addr<0) || (addr>0x7c)) {
	printf("pr_cfg: addr out of range\n");
	return(0);
    } 
    ret = edt_ioctl(edt_p,EDTG_CONFIG,&buf) ;
    if (ret < 0)
    {
	perror("EDTG_CONFIG") ;
    }
    return (u_int) (buf.value) ;
}


u_int
pr_cfg_copy(int addr)
{
    int ret ;
    edt_buf buf ;
    buf.desc = addr ;
    if ((addr<0) || (addr>0x7c)) {
	printf("pr_cfg_copy: addr out of range\n");
	return(0);
    } 
    ret = edt_ioctl(edt_p,EDTG_CONFIG_COPY,&buf) ;
    if (ret < 0)
    {
	perror("EDTG_CONFIG_COPY") ;
    }
    return (u_int) (buf.value) ;
}


void
pw_cfg(int addr, int value)
{
    int ret ;
    edt_buf buf ;
    buf.desc = addr ;
    if ((addr<0) || (addr>0x7c)) {
	printf("pw_cfg: addr out of range\n");
	return;
    } 
    buf.value = value ;
    ret = edt_ioctl(edt_p,EDTS_CONFIG,&buf) ;
    if (ret < 0)
    {
	perror("EDTS_CONFIG") ;
    }
}

static volatile caddr_t mapaddr = 0 ;
static volatile caddr_t mapaddr1 = 0 ;

void
mmap_setup(EdtDev *edt_p)
{
    mapaddr = (caddr_t)edt_mapmem(edt_p, 0, edt_get_mappable_size(edt_p, 0)) ;
    if (mapaddr == 0)
    {
	printf("edt_mapmem failed\n") ;
	return ;
    }
    
	if (edt_get_mappable_size(edt_p, 1) > 0)
		mapaddr1 = ( caddr_t)edt_mapmem(edt_p, 0x10000, edt_get_mappable_size(edt_p, 1)) ;
	else
		mapaddr1 = 0;

}

int check_mmap(int space)
{
    if (mapaddr == 0) mmap_setup(edt_p) ;
    if (space == 0)
    {
	if (mapaddr == 0)
	{
	    fprintf(stderr,"bad mmap for space 0\n");
	    return(1) ;
	}
	else return(0) ;
    }
    else
    {
	if (mapaddr1 == 0)
	{
	    fprintf(stderr,"bad mmap for space 1\n");
	    return(1) ;
	}
	else return(0) ;
    }
}


u_short
mmap_read_16(EdtDev *edt_p, int addr, int space)
{
    if (mapaddr == 0) mmap_setup(edt_p) ;
    if (space == 0)
    {
	if (mapaddr == 0) return(0xdead) ;
	else return (*(u_short *)(mapaddr + addr)) ;
    }
    else
    {
		if (mapaddr1 == 0) return(0xbeef) ;
		else return (*(u_short *)(mapaddr1 + addr)) ;
    }
}
u_int
mmap_read_32(EdtDev *edt_p, int addr, int space)
{
    if (mapaddr == 0) mmap_setup(edt_p) ;
    if (space == 0)
    {
	if (mapaddr == 0) return(0xdeadbeef) ;
	else return (*(u_int *)(mapaddr + addr)) ;
    }
    else
    {
	if (mapaddr1 == 0) return(0xcafebeef) ;
	else return (*(u_int *)(mapaddr1 + addr)) ;
    }
}
void
mmap_write_16(EdtDev *edt_p, int addr, u_short data, int space)
{
    if (mapaddr == 0) mmap_setup(edt_p) ;
    if (space == 0)
    {
	if (mapaddr == 0) printf("mmap failed\n") ;
	else *((u_short *)(mapaddr + addr)) = data ;
    }
    else
    {
	if (mapaddr1 == 0) printf("mmap failed\n") ;
	else *((u_short *)(mapaddr1 + addr)) = data ;
    }
}
void
mmap_write_32(EdtDev *edt_p, int addr, u_int data, int space)
{
    if (mapaddr == 0) mmap_setup(edt_p) ;
    if (space == 0)
    {
	if (mapaddr == 0) printf("mmap failed\n") ;
	else *((u_int *)(mapaddr + addr)) = data ;
    }
    else
    {
	if (mapaddr1 == 0) printf("mmap failed\n") ;
	else *((u_int *)(mapaddr1 + addr)) = data ;
    }
}


static void
usage(char *s)
{
    int ret = 0 ;
    printf("pdb -u unit [-c channel] [-f batchfile] [-v]\n");
    if (s && *s)  
    {
        puts(s);
	ret = 1 ;
    }
#ifdef VXWORKS
    return;
#else
    exit(ret);
#endif
}
    
	
#define PDBBUFSIZE 256

#define DMABUFSZ  (1024*1024*16)
unsigned char *dmabuf;

int gettok(char *tbuf, char **pbufp) ;

#ifndef _NT_
#ifndef VXWORKS
void
gotit(int dmy)
{
    printf("got sig\n") ;
    exit(0);
}
#endif
#endif

unsigned int  dbgflg = 0;
char spi_selectn = 0x0E;		// Select first of four spi flash chips

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
pdb(char *command_line)
#else
int main(int argc,char *argv[])
#endif
{
    FILE *xfd;			/* command file for reading */
    char *progname;
    int repeat=0; 		/* Counter for number of times to repeat */
    int repeat_flag=0;		/* Boolean, false if should get new_one command */
    char  buf[PDBBUFSIZE];		/* Buffer for incoming command line */
    char  *bufp;		/* pointer into buf[] */
    char  tbuf[PDBBUFSIZE];	/* Buffer for one token from buf[] */
    u_short *readbuf = 0 ;
    int docheck = 0 ;


/* ######################################################################## */
    int    i;
    char   edt_devname[256] = "";
    char   errbuf[32];
    char   cmdfile[32] = "";
    int    channel=-1;
    char   *unitstr = "";
    int    unit = 0;
#ifdef NO_MAIN
    char **argv  = 0 ;
    int argc = 0 ;
    opt_create_argv("pdb",command_line,&argc,&argv);
#endif

    progname = argv[0];
#ifndef _NT_
#if defined(VXWORKS)
 /* ioctl(stdin,FIOSETOPTIONS,OPT_LINE|OPT_CRMOD|OPT_ECHO) ;*/
#else
    signal(SIGINT,gotit) ;
#endif
#endif

    /*
     * COMMAND LINE ARGUMENTS
     */

    for (i = 1; i < argc; i++)
    {
	if (argv[i][0] != '-')
	{
		/* Check for old style command line of "pdb unit [batfile]" */
		if ((i==1) && (argc<=3)) { 
	            unitstr = argv[1];
		    if (argc==3)  strcpy(cmdfile, argv[2]);
		    break;
		} else {
		    sprintf(errbuf, "Badly formed argument of %s", argv[i]);
		    usage(errbuf);
		}
	}
	else
	{
	    if (argv[i][1] == '-')
	    {
		if (strcmp(argv[i], "--help") == 0)
		    usage(NULL);
		else
		{
		    sprintf(errbuf, "unknown option \"%s\"", argv[i]);
		    usage(errbuf);
		}
	    }
	    if (argv[i][1] == 0 || argv[i][2] != 0)
		usage(NULL);
	    switch (argv[i][1])
	    {
	    case 'u':
		if (++i == argc)
		    usage("unfinished -u option");
		unitstr = argv[i];
		break;
	    case 'c':
		if (++i == argc)
		    usage("unfinished -c option");
		channel = atoi(argv[i]);
		break;
	    case 'f':
		if (++i == argc)
		    usage("unfinished -f option");
		strcpy(cmdfile, argv[i]);
		break;
	    case 'v':
		verbose_reg_read = 1;
		break;
/*
	    case 'V':
		verbose = verbose ? 0 : 1;
		break;
	    case 'q':
		quiet=1;
		break;
*/
	    case 'h':
		usage(NULL);
		break;
	    default:
		sprintf(errbuf, "\nunknown option \"-%c\"", argv[i][1]);
		usage(errbuf);
	    }
	}
    }


    if (*unitstr) 
	unit = edt_parse_unit(unitstr, edt_devname, NULL);
    else
    {
	unit=0;
	strcpy(edt_devname, EDT_INTERFACE) ;
    }



/* ######################################################################## */

    /*  Open the command file to read from */
    if (!*cmdfile)    xfd = stdin;
    else
    {
        if ((xfd = fopen(cmdfile, "r")) == NULL)
        {
	    fprintf(stderr,"couldn't open  command file %s\n", cmdfile);
	    fprintf(stderr,"aborting test - no command file\n");
	    exit(2);
        }
    }


    if (channel==-1) {
        printf("edt_open( edt_devname:%s unit:%d)\n", edt_devname, unit);
        edt_p = edt_open(edt_devname, unit);
    } else {
        printf("edt_open_channel( edt_devname:%s unit:%d channel:%d)\n", 
				  edt_devname,   unit,   channel);
        edt_p = edt_open_channel(edt_devname, unit, channel);
    }
    if (edt_p == NULL) {
	    puts("Hit return to exit") ;
	    getchar() ;
	    return(1) ;
    }
	
    dmabuf = edt_alloc(DMABUFSZ);


    while (1)  {
	if (kbhitedt())   { repeat=0;  repeat_flag=0; }

	/* note that fgets() returns a string that includes a final '\n' */
	if (!repeat_flag)  {
	    int n;
	    printf(": ");  fflush(stdout);
	    if (fgets(buf, PDBBUFSIZE, xfd) == NULL)  exit(0);
	    for (n=0; n<PDBBUFSIZE; n++) {		/* strip out comments */
		if (buf[n]=='#')  {		/* back up through whitespace*/
		    while ((n>0)&&(isspace(buf[n-1])))  n--;
		    buf[n]='\n';		/* add end of line */
		    buf[n+1]='\0';		/* add end of string */
		    break;
		}
		if (buf[n]=='\0') break;	/* normal end of string */
	    }
	}

        if (repeat!=0)  {
            if (repeat != -1) repeat--;         /* loop forever if -1 */
	    repeat_flag=repeat;
	} else repeat_flag=0;

	bufp = buf;
	if (gettok(tbuf, &bufp) == 0)  continue;    /* skip blank lines */


	/* Start of dispatch table */
	if (strcmp(tbuf, "h") == 0)  {


puts("Commands to access the pci space:");
  puts("  r addr [count]       Read 8 bits from interface Xilinx ");
  puts("  w addr data          Write 8 bits to interface Xilinx ");
  puts("  ir   addr [count]      Read 8 bits from interface Xilinx indirect");
  puts("  ir16 addr [count]    Read 16 bits from interface Xilinx indirect");
  puts("  ir32 addr [count]    Read 32 bits from interface Xilinx indirect");
  puts("  irv  addr [count]    Read 8 bits from interface Xilinx indirect (verbose)");
  puts("  iw   addr data       Write 8 bits to interface Xilinx indirect");
  puts("  iw16 addr data       Write 16 bits to interface Xilinx indirect");
  puts("  iw32 addr data       Write 32 bits to interface Xilinx indirect");
  puts("  ib addr              Set the interface Xilinx indirect base register to addr");
  puts("  r16 addr             Read 16 bits from interface Xilinx ");
  puts("  w16 addr data        Write 16 bits to interface Xilinx ");
  puts("  mr16 addr            Read 16 bits from MAC8101 ");
  puts("  mw16 addr data       Write 16 bits to MAC8101 ");
  puts("  xr8/16/32 addr       Read 8/16/32 bits from pci register ");
  puts("  xw8/16/32 addr data  Write 8/16/32 bits to pci register ");
  puts("  xwcfg addr data      Write 32 bits to config space");
  puts("  xrcfg addr           Read 32 bits from config space");
  puts("  rm016/032 addr       Read 16/32 bits from mmap space 0");
  puts("  wm016/032 addr data  Write 16/32 bits to mmap space 0");
  puts("  rm116/132 addr       Read 16/32 bits from mmap space 1");
  puts("  wm116/132 addr data  Write 16/32 bits to mmap space 1");
  puts("");
  puts("  rmzdemod blk off [count]       Read 32 bits from MZDEMOD block and offset");
  puts("  wmzdemod blk off data [count]  Write 32 bits to MZDEMOD block and offset");
  puts("");
  puts("  rp                   Read incoming command packet from fiber ");
  puts("  wp rb d d d          Write command packet out, routing byte of 20 first ");
  puts("  sniff16              same, but only sync word + 1st 60 bytes of each packet");
  puts("  rdma fname bcount    dma read of bcount bytes to file fname ");
  puts("  c                    Connect to remote debugger");
  puts("  A                    Do Auto Configure");
  puts("  R                    Reinitialize Mac8101");
  puts("  U n                  Change default unit addressing to ");
  puts("  x w h n              Sending width height as dma");
  puts("  g w h n              Get as dma width/height");
  puts("  d                    Dump regs");
  puts("  b  filename          Execute a batch file of debug commands  ");
  puts("  sys  command         Execute a shell command ");
  puts("  dbgflg b             b=0:normal, b=1:debug_msgs, b=2:no_print ");
  puts("  h                    Print this help message ");
  puts("  L n                  Loop next command n times, -1=forever  ");
  puts("  cfgr/cfgw            Copy from driver or /tmp/p53dbg.cfg to PCI config space");
  puts("  reboot               cfgr, xw8 85 40, cfgw ");
  puts("  spi_res spi_rdid spi_rdsr spi_wdsr spir spie spiw spiv spiwo spis ");
  puts("  q                    Quit the program \n");
}


//	else if (strcmp(tbuf, "spif") == 0)  {
//	    int n, val;
//	    pw8(0x05, 0x02);
//	    for (n=0; n<16; n++) {
//	        val = spi_res();
//	        printf("%02x ", val);
//	    }
//	    printf("\n");
//	}

	else if (strcmp(tbuf, "spi_res") == 0)  {
	    int val;
	    val = spi_res();
	    if (!(dbgflg & 0x02)) printf("%02x\n", val);
	}
	else if (strcmp(tbuf, "spi_rdid") == 0)  {
	    char rvals[32];   int n;
	    spi_rdid(rvals);
	    if (!(dbgflg & 0x02)) 
	        for (n=0; n<19; n++)    printf(" %02x", rvals[n]&0xff);
		putchar('\n');
	}
	else if (strcmp(tbuf, "spis") == 0)  {
	    int val, n;
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &val);
	    if (n!=1)  {
	        printf("selectn: %02x\n", spi_selectn);
	    } else {
	        if (val==0) spi_selectn=0xE;		// 1110
	        else if (val==1) spi_selectn=0xD;	// 1101
	        else if (val==2) spi_selectn=0xB;	// 1011
	        else if (val==3) spi_selectn=0x7;	// 0111
	        else printf("  Expected a value of 0,1,2,3\n");
	    }
	}
	else if (strcmp(tbuf, "spi_rdsr") == 0)  {
	    int val;
	    val = spi_rdsr();
	    printf("%02x\n", val);
	}

	else if (strcmp(tbuf, "spi_wrsr") == 0)  {
	    /* "spi_wrsr 08" inhibits writes to sectors 0,1 "till spi_wrsr 00" */
	    /* "spi_wrsr 88" also disables spi_wrsr's if write protect pin low */
	    int val;
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &val);
	    spi_wrsr(val);
	}


	else if (strcmp(tbuf, "spir") == 0)  {
	    int addr, cnt, c, n, val;
	    char pbuf[32];	  		/* For ASCII printing */
	    char* p;
	    char* pbufp;
            FILE   *xfd = NULL;

	    // /* Firmware needs ok parity for 256 SPI clocks to unlock access */
 	    // for (n=0; n<300/(5*8); n++)  spi_res();   /* 5 bytes per spi_res */

	    val = spi_res();
            if (val != 0x20) {
		printf("Error, spi_res returned 0x%02x, not 0x20 \n", val);
		continue;
	    }

	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &addr);
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &cnt);
	    if (n!=1)  cnt=1;

	    if (gettok(tbuf, &bufp)) {
	        if ((xfd = fopen(tbuf, "wb")) == NULL)
	        {
		    printf("Couldn't open file %s for writing\n", tbuf);
		    continue;
	        }
	    }

	    for (n=0; n<16; n++) pbuf[n]=0x20;		/* init print buffer */
	    pbufp = pbuf+(addr&0xf);
	    for (n=0; n<(addr&0xf); n++) printf("   ");

	    while (cnt > 0) {
		spi_read(addr, 16);
		for (n=0; n<16; n++) {
		    if (xfd != NULL) {
		    	putc(rbuf[n], xfd);
		    } else {
		        if ((addr & 0xf)== 0)  {
			    printf("    #");
			    printf("%06x  ", addr-16);
			    p = pbuf;
			    while (p<pbufp) {
				if ((*p>=0x20) && (*p<0x7f))  putchar(*p);
				else  putchar('.');
				p++;
			    }
			    pbufp = pbuf;
			    putchar('\n');
			}
	                printf("%02x ", rbuf[n]);
			*pbufp++ = rbuf[n];
		    }
		    addr++;  cnt--;
		    if (cnt==0) break;
		}
	    }
	    if (xfd != NULL)   fclose(xfd);
	    else   printf("\n");
	}

	else if (strcmp(tbuf, "spie") == 0)  {
	    int sect, n, val;

	    val = spi_res();
            if (val != 0x20) {
		printf("Error, spi_res returned 0x%02x, not 0x20\n", val);
                if (*cmdfile)  exit(1);		/* abort if script file */
		continue;
	    }

	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &sect);
	    if ((n==1) && ((sect>=0) && (sect<=15))) {
		printf("erasing sect %x\n", sect);
		spi_erase(sect<<16);
	    }  else  {
		printf("ERROR:  spie saw bad sector number %s \n", tbuf);
                if (*cmdfile)  exit(1);		/* abort if script file */
	    }
	}


	else if (strcmp(tbuf, "smps") == 0)  {
	    int n, data;

	    // Get four 8bit pshift values, one for each of 4 ADC chips on VHDCI-D and VHDCI-C
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &data);
	    if (n == 1) 
	        mmap_write_32(edt_p, 0x1250, data, 1);
	    else
		continue;

	    // Optional four 8bit pshift values, one for each of 4 ADC chips on VHDCI-B and VHDCI-A
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &data);
	    if (n == 1) 
	        mmap_write_32(edt_p, 0x1240, data, 1);

            data = mmap_read_32(edt_p, 0x1210, 1);		// delay
	    mmap_write_32(edt_p, 0x1210, 0x200, 1);		// reset all the PLL's
            data = mmap_read_32(edt_p, 0x1210, 1);		// delay
	    mmap_write_32(edt_p, 0x1210, 0x000, 1);		// clear the reset strobe
	}



//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
	// TODO:  move to 4 byte SPI chip addressing
	// Write Altera rbf file across four parallel n25q256 spi flash chips
	// At boot, n26q256 Quad Output Fast Read gets 4 msbs of each byte first
	// Altera FFPx16 bit ordering as on p287 of av_5v2.pdf (little endian)
 	else if (strcmp(tbuf, "spiwcls") == 0) {

            static char rbf2d[4][0x500000];		/* 4*5mbytes total for bitfile*/
            FILE *xfd;				/* Bitfile for altera FPGA */
 	    int addr0, addr, c, n, got, bcnt, errorcnt, val, first, last, spichip; 

	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &addr0);
	    if ((!(gettok(tbuf,&bufp))) || ((xfd = fopen(tbuf, "rb"))==NULL)) {
	    	printf("   Error, could not open file %s for reading\n", tbuf);
                if (*cmdfile)  exit(1);		/* abort if script file */
		continue;
	    }

	    // Load up a two dimensional array with bitfile data
	    // Array has 4 rows, each row stores up to 5mbytes for one spi chip
  	    got=0;
  	    while ( (c=getc(xfd)) != EOF) {
		rbf2d[0][got] = (c<<4) & 0xf0;	// spi nibble mode read sends 4 msb's first to fpga during boot
		rbf2d[1][got] = c & 0xf0;	// so first bitfile byte goes to 4 msbs of first 2 flash chips
	
		c=getc(xfd);
		rbf2d[2][got] = (c<<4) & 0xf0;	// second bitfile byte to 4 msbs of other 2 flash chips
		rbf2d[3][got] = c & 0xf0;

		c=getc(xfd);
		rbf2d[0][got] |= c & 0xf;	// next 2 bitfile bytes go to 4 lsbs of 4 flash chips
		rbf2d[1][got] |= (c>>4) & 0xf;

		c=getc(xfd);
		rbf2d[2][got] |= c & 0xf;
		rbf2d[3][got] |= (c>>4) & 0xf;

		got++;
   	    }	

	    errorcnt=0;
	    for (spichip=0; spichip<4; spichip++) {		// For each of the four chips
	        spi_selectn = 0xf & ( ~(1<<spichip));		// values of 0xE, 0xD, 0xB, 0x7
	        val = spi_res();
                if (val != 0x20) {
		    printf("Error, spi_rdid for chip %d returned 0x%02x, not 0x20\n", spichip, val);
		    errorcnt++;
		}
	    }
	    if (errorcnt!=0) {
		if (*cmdfile)  exit(1);		// Abort program if from script file
		else continue;			// Get next command at command prompt
	    }


	    first = (addr0&0xff0000)>>16;	// First sector number, 64kbyte sectors
	    last  = ((addr0+got)&0xff0000)>>16;	// Last sector number
	    printf("First sector is %d, last is %d, erase-and-write:w verify-only:v abort:n >", first, last);
	    fflush(stdout);
 	    c = getchar();   getchar();
	    if ((c!='w') && (c!='v'))   {
                if (*cmdfile)  exit(1);		// Abort program if from script file
		else continue;			// Get next command at command prompt
	    }

	    // Write corresponding row of array to each of the four spi flash chips
	    if (c=='w')     for (spichip=0; spichip<4; spichip++) {
	        spi_selectn = 0xf & ( ~(1<<spichip));	// values of 0xE, 0xD, 0xB, 0x7
		printf("Chip %d: erase ",  spichip); fflush(stdout);
		for (n=first; n<=last; n++)  {   // 66 sectors of 64kbytes each
		    spi_erase(n<<16);
		    if ((n&0x7)==0) { printf("."); fflush(stdout); }
		}
		printf(" write "); fflush(stdout);
                for (bcnt=0; bcnt<got; bcnt++) {}
	        bcnt=0;  addr=addr0;
	        while (bcnt<got)  {
	          n=0;
	          while (bcnt<got)  {
		    if ((n!=0) && (((addr+n)&0xff)==0)) break;  /* page end */
		    wbuf[n] = rbf2d[spichip][bcnt]; 
		    n = n+1;
		    bcnt = bcnt+1;
	          }
		  if (n>0) spi_write(addr, n);	/* Write 1-256 bytes from wbuf*/
		  addr = addr + n;
		  if ((addr & 0x1ffff)==0) { printf("."); fflush(stdout); }
	    	};
		printf("\n");
	    }

	    // Verify the data that was written to each of the four chips
	    for (spichip=0; spichip<4; spichip++) {
	      spi_selectn = 0xf & ( ~(1<<spichip));	// values of 0xE, 0xD, 0xB, 0x7
	      printf("Chip %d: verify ",  spichip); fflush(stdout);
	      bcnt=0;  addr=addr0; errorcnt=0;
	      while (bcnt<got)  {
		spi_read(addr, 256);
		n=0;
	        while (bcnt<got)  {
		    if ((n!=0) && (((addr+n)&0xff)==0)) break;  /* page end */
		    if ((rbuf[n]&0xff) != (rbf2d[spichip][bcnt]&0xff)) {
			if (errorcnt++ <= 10) {
			    printf("Error at 0x%x: wrote:%02x, read:%02x\n", 
					addr0+bcnt, rbf2d[spichip][bcnt], rbuf[n]);
			}
		    }
		    n = n+1;
		    bcnt = bcnt+1;
		}
		addr = addr + n;
		if ((addr & 0xffff)==0) { printf("."); fflush(stdout); }
	      }
	      printf("    Saw %d errors\n", errorcnt);
              if ((*cmdfile!='\0') && (errorcnt!=0)) 
		exit(1);  	/* abort if script file */
	    }
	}


#define SPISIZE (20*1024*1024)	  /* Our n25q256 chips can hold 32mbytes */
 	else if ((strcmp(tbuf, "spiw") == 0) || (strcmp(tbuf, "spiv") == 0)) {
 	    int addr0, addr, c, n, got, bcnt, errorcnt, val, first, last, wren;
            FILE   *xfd;
	    char *spibuf;
 	    int sectors[4];

 	    if (strcmp(tbuf, "spiw") == 0)  wren=1;  else wren=0;

	    val = spi_res();
            if (val != 0x20) {
		printf("Error, spi_res returned 0x%02x, not 0x20\n", val);
                if (*cmdfile)  exit(1);		/* abort if script file */
		continue;
	    }
	    printf("    spi_res() returned 0x%02x\n", val);

	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &addr0);
	    if ((!(gettok(tbuf,&bufp))) || ((xfd = fopen(tbuf, "rb"))==NULL)) {
	    	printf("   Error, could not open file %s for reading\n", tbuf);
                if (*cmdfile)  exit(1);		/* abort if script file */
		continue;
	    }

	    spibuf = malloc(SPISIZE);
	    if (malloc==NULL) {
		printf("    Error on malloc(0x%x) bytes for spibuf\n", SPISIZE);
                if (*cmdfile)  exit(1);		/* abort if script file */
		continue;
	    }

            /*size_t fread(void *ptr,size_t size,size_t nmemb,FILE *stream);*/
     	    got = fread((void*)spibuf, 1, SPISIZE, xfd);
	    if (got<=0) {
		printf("    Error reading file %s\n", tbuf);
                if (*cmdfile)  exit(1);		/* abort if script file */
		continue;
	    }


 	if (wren) {
	    first = (addr0&0xff0000)>>16;
	    last  = ((addr0+got)&0xff0000)>>16;
	    printf("First sector is %d, last is %d,   erase:y overwrite:o abort:n >", 
				first, last);
	    fflush(stdout);

            if (dbgflg&0x400)  { c='y';  printf("\n"); }
 	    else {   c = getchar();   getchar();  }

	    if ((c=='y') || (c=='Y'))   {
		for (n=first; n<=last; n++) {
		    printf("Erasing sector %x\n", n);
		    spi_erase(n<<16);
		}
	    }
	    if (!((c=='o') || (c=='O') || (c=='y') || (c=='Y')))  {
		printf("  aborting\n");
                if (*cmdfile)  exit(1);		/* abort if script file */
		continue;
	    }

	    printf("  Writing 0x%x bytes to spiflash at 0x%x from %s\n",got,addr0, tbuf);
	    bcnt=0;  addr=addr0;
	    while (bcnt<got)  {
	        n=0;
	        while (bcnt<got)  {
		    if ((n!=0) && (((addr+n)&0xff)==0)) break;  /* page end */
		    wbuf[n] = spibuf[bcnt]; 
		    n = n+1;
		    bcnt = bcnt+1;
	        }
		if (n>0) spi_write(addr, n);	/* Write 1-256 bytes from wbuf*/
		addr = addr + n;
	    };
	}

	    printf("    Verifying:    ");
	    bcnt=0;  addr=addr0; errorcnt=0;
	    while (bcnt<got)  {
		spi_read(addr, 256);
		n=0;
	        while (bcnt<got)  {
		    if ((n!=0) && (((addr+n)&0xff)==0)) break;  /* page end */
		    if ((rbuf[n]&0xff) != (spibuf[bcnt]&0xff)) {
			if (errorcnt++ <= 10) {
			    printf("Error at 0x%x: wrote:%02x, read:%02x\n", 
					addr0+bcnt, spibuf[bcnt], rbuf[n]);
			}
		    }
		    n = n+1;
		    bcnt = bcnt+1;
		}
		addr = addr + n;
	    }
	    printf("    Saw %d errors\n", errorcnt);
            if ((*cmdfile!='\0') && (errorcnt!=0)) 
		exit(1);  	/* abort if script file */
	    free(spibuf);
	}

 	else if (strcmp(tbuf, "spiwo") == 0) {
 	    int addr0, addr, c, n, got, bcnt, errorcnt, val, first, last, wren;
            FILE   *xfd;
	    char *spibuf;
 	    int sectors[4];

 	    wren=1;

	    val = spi_res();
            if (val != 0x20) {
		printf("Error, spi_res returned 0x%02x, not 0x20\n", val);
                if (*cmdfile)  exit(1);		/* abort if script file */
		continue;
	    }
	    printf("    spi_res() returned 0x%02x\n", val);

	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &addr0);
	    if ((!(gettok(tbuf,&bufp))) || ((xfd = fopen(tbuf, "rb"))==NULL)) {
	    	printf("   Error, could not open file %s for reading\n", tbuf);
                if (*cmdfile)  exit(1);		/* abort if script file */
		continue;
	    }

	    spibuf = malloc(SPISIZE);
	    if (malloc==NULL) {
		printf("    Error on malloc(0x%x) bytes for spibuf\n", SPISIZE);
                if (*cmdfile)  exit(1);		/* abort if script file */
		continue;
	    }

            /*size_t fread(void *ptr,size_t size,size_t nmemb,FILE *stream);*/
     	    got = fread((void*)spibuf, 1, SPISIZE, xfd);
	    if (got<=0) {
		printf("    Error reading file %s\n", tbuf);
                if (*cmdfile)  exit(1);		/* abort if script file */
		continue;
	    }


 	if (wren) {
            first = (addr0&0xff0000)>>16;
            last  = ((addr0+got)&0xff0000)>>16;
	    printf("First sector is %d, last is %d,   erase:y overwrite:o abort:n >", 
				first, last);
	    fflush(stdout);

            { c='o';  printf("\n"); }

	    if ((c=='y') || (c=='Y'))   {
		for (n=first; n<=last; n++) {
		    printf("Erasing sector %x\n", n);
		    spi_erase(n<<16);
		}
	    }
	    if (!((c=='o') || (c=='O') || (c=='y') || (c=='Y')))  {
		printf("  aborting\n");
                if (*cmdfile)  exit(1);		/* abort if script file */
		continue;
	    }

	    printf("  Writing 0x%x bytes to spiflash at 0x%x from %s\n",got,addr0, tbuf);
	    bcnt=0;  addr=addr0;
	    while (bcnt<got)  {
	        n=0;
	        while (bcnt<got)  {
		    if ((n!=0) && (((addr+n)&0xff)==0)) break;  /* page end */
		    wbuf[n] = spibuf[bcnt]; 
		    n = n+1;
		    bcnt = bcnt+1;
	        }
		if (n>0) spi_write(addr, n);	/* Write 1-256 bytes from wbuf*/
		addr = addr + n;
	    };
	}

	    printf("    Verifying:    ");
	    bcnt=0;  addr=addr0; errorcnt=0;
	    while (bcnt<got)  {
		spi_read(addr, 256);
		n=0;
	        while (bcnt<got)  {
		    if ((n!=0) && (((addr+n)&0xff)==0)) break;  /* page end */
		    if ((rbuf[n]&0xff) != (spibuf[bcnt]&0xff)) {
			if (errorcnt++ <= 10) {
			    printf("Error at 0x%x: wrote:%02x, read:%02x\n", 
					addr0+bcnt, spibuf[bcnt], rbuf[n]);
			}
		    }
		    n = n+1;
		    bcnt = bcnt+1;
		}
		addr = addr + n;
	    }
	    printf("    Saw %d errors\n", errorcnt);
            if ((*cmdfile!='\0') && (errorcnt!=0)) 
		exit(1);  	/* abort if script file */
	    free(spibuf);
	}

	else if (strcmp(tbuf, "spi_syncbytes") == 0)  {
	    int c, n;
            FILE   *xfd;

	    if ((!(gettok(tbuf,&bufp))) | ((xfd = fopen(tbuf,"rb"))==NULL)) {
	    	printf("Error, could not open file for writing\n");
	    }

	    /* FF FF FF FF  AA 99 55 66       sync bytes at start of stream */
	    for (n=0; n<256; n++)  {
		wbuf[n] = c = getc(xfd);
		if (c == EOF) break;
	    }
	    if (n!=256) {
		printf("Error, less than 256 bytes in file.\n");
		continue;
	    }
	    for (n=0; n<256; n++)    if (wbuf[n]==0xAA)  break;
	    if ((n>=4) && (n<=252) && 
	         (wbuf[n-4]==0xFF) && (wbuf[n-3]==0xFF) &&
	         (wbuf[n-2]==0xFF) && (wbuf[n-1]==0xFF) &&
	         (wbuf[n+0]==0xAA) && (wbuf[n+1]==0x99) &&
	         (wbuf[n+2]==0x55) && (wbuf[n+3]==0x66)
               )  {
	        printf("Sync bytes start at %02x bytes (hex) into file\n", n-4); 
	        printf(" so starting address for file should be %04x\n", 260-n);
		continue;
	    } else {
		printf("Didn't detect SpartanIIe sync bytes, aborting\n");	
		continue;
	    }
	}




	else if (strcmp(tbuf, "dbgflg") == 0)	{
	    int data, n;
	    gettok(tbuf, &bufp);
	    n = sscanf(tbuf,"%x", &data);
	    if (n==1)  
		dbgflg = data;
	    else
		printf("%x \n", dbgflg);
	}
	else if (strcmp(tbuf, "v") == 0)	{
	    verbose_reg_read ^= 1;
	    printf("\nVerbose register read display is now %s\n\n", (verbose_reg_read) ? "on" : "off");
	}
	else if (strcmp(tbuf, "r") == 0)	{
	    int addr, data, n, cnt;
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &addr);
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &cnt);
	    if (n!=1)  cnt=1;
	    for (n=0; n<cnt; n++) {
	        data = pr8(addr) ;
	        if (!(dbgflg & 0x02)) {
		    if (verbose_reg_read)
			printf("%02x: %02x\n", addr, data&0xff);
		    else
			printf("%02x ", data&0xff);
		}
		addr += 1;
	    }
	    if (!(dbgflg & 0x02))   printf("\n");
	}
	else if (strcmp(tbuf, "rlcrdemod") == 0)	{
	    u_int block, offset, data, n, cnt;
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &block);
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &offset);
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &cnt);
	    if (n!=1)  cnt=1;
	    for (n=0; n<cnt; n++) {
	        data = edt_lcr_demod_read(edt_p, block, offset);
	        if (!(dbgflg & 0x02)) {
		    if (verbose_reg_read)
			printf("%01x/%06x: %08x\n", block, offset, data);
		    else
			printf("%08x ", data);
		}
		offset += 1;
	    }
	    if (!(dbgflg & 0x02))   printf("\n");
	}
	else if (strcmp(tbuf, "wlcrdemod") == 0)	{
	    u_int block, offset, data, n, cnt;
 	    int first=1;
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &block);
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &offset);
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &data);
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &cnt);
	    if (n!=1)  cnt=1;
	    for (n=0; n<cnt; n++) {
	        edt_lcr_demod_write(edt_p, block, offset, data);
		++ offset;
		++ first;
	    }
	    if (!(dbgflg & 0x02))   printf("\n");
	    if (first==1)  printf("  nothing written\n");
	}
	else if (strcmp(tbuf, "rmzdemod") == 0)	{
	    u_int block, offset, data, n, cnt;
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &block);
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &offset);
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &cnt);
	    if (n!=1)  cnt=1;
	    for (n=0; n<cnt; n++) {
	        data = edt_mzdemod_read(edt_p, block, offset);
	        if (!(dbgflg & 0x02)) {
		    if (verbose_reg_read)
			printf("%01x/%06x: %08x\n", block, offset, data);
		    else
			printf("%08x ", data);
		}
		offset += 1;
	    }
	    if (!(dbgflg & 0x02))   printf("\n");
	}
	else if (strcmp(tbuf, "wmzdemod") == 0)	{
	    u_int block, offset, data, n, cnt;
 	    int first=1;
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &block);
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &offset);
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &data);
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &cnt);
	    if (n!=1)  cnt=1;
	    for (n=0; n<cnt; n++) {
	        edt_mzdemod_write(edt_p, block, offset, data);
		++ offset;
		++ first;
	    }
	    if (!(dbgflg & 0x02))   printf("\n");
	    if (first==1)  printf("  nothing written\n");
	}
	else if (strcmp(tbuf, "rlcr") == 0)	{
	    u_int block, offset, data, n, cnt;
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &block);
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &offset);
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &cnt);
	    if (n!=1)  cnt=1;
	    for (n=0; n<cnt; n++) {
	        data = edt_lcr_read(edt_p, block, offset);
	        if (!(dbgflg & 0x02)) {
		    if (verbose_reg_read)
			printf("%01x/%06x: %08x\n", block, offset, data);
		    else
			printf("%08x ", data);
		}
		offset += 1;
	    }
	    if (!(dbgflg & 0x02))   printf("\n");
	}
	else if (strcmp(tbuf, "wlcr") == 0)	{
	    u_int block, offset, data, n, cnt;
 	    int first=1;
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &block);
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &offset);
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &data);
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &cnt);
	    if (n!=1)  cnt=1;
	    for (n=0; n<cnt; n++) {
	        edt_lcr_write(edt_p, block, offset, data);
		++ offset;
		++ first;
	    }
	    if (!(dbgflg & 0x02))   printf("\n");
	    if (first==1)  printf("  nothing written\n");
	}

	else if (strcmp(tbuf, "rv") == 0)	{
	    int addr, data, n, cnt;
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &addr);
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &cnt);
	    if (n!=1)  cnt=1;
	    for (n=0; n<cnt; n++) {
	        data = pr8(addr) ;
	        if (!(dbgflg & 0x02))   printf("%02x: %02x\n", addr, data&0xff);
		addr += 1;
	    }
	    if (!(dbgflg & 0x02))   printf("\n");
	}
	else if (strcmp(tbuf, "ib") == 0)	{
	    int addr, ret;
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &addr);
	    printf("\nSetting indirect register access base to 0x%x\n\n", addr);

	    edt_ioctl(edt_p, EDTG_INDIRECT_REG_BASE, &addr);
	    printf("\nIndirect register access base was 0x%x\n", addr);

	    ret = edt_ioctl(edt_p, EDTS_INDIRECT_REG_BASE, &addr);
	    if (ret)
	    {
		edt_perror("edt_ioctl(EDTS_INDIRECT_REG_BASE)");
	    }
	    addr = 0;
	    edt_ioctl(edt_p, EDTG_INDIRECT_REG_BASE, &addr);
	    printf("Indirect register access base now 0x%x\n\n", addr);
	}
	else if (strcmp(tbuf, "ir") == 0)	{
	    int addr, data, n, cnt;
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &addr);
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &cnt);
	    if (n!=1)  cnt=1;
	    for (n=0; n<cnt; n++) {
	        data = ipr8(addr) ;
	        if (!(dbgflg & 0x02)) {
		    if (verbose_reg_read)
			printf("%02x: %02x\n", addr, data&0xff);
		    else
			printf("%02x ", data&0xff);
		}
		addr += 1;
	    }
	    if (!(dbgflg & 0x02))   printf("\n");
	}
	else if (strcmp(tbuf, "ir16") == 0)	{
	    int addr, data, n, cnt;
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &addr);
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &cnt);
	    if (n!=1)  cnt=1;
	    for (n=0; n<cnt; n++) {
	        data = ipr16(addr) ;
	        if (!(dbgflg & 0x02)) {
		    if (verbose_reg_read)
			printf("%02x: %04x\n", addr, data&0xffff);
		    else
			printf("%04x ", data&0xffff);
		}
		addr += 2;
	    }
	    if (!(dbgflg & 0x02))   printf("\n");
	}
	else if (strcmp(tbuf, "ir32") == 0)	{
	    int addr, n, cnt;
	    u_int data;
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &addr);
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &cnt);
	    if (n!=1)  cnt=1;
	    for (n=0; n<cnt; n++) {
	        data = ipr32(addr) ;
	        if (!(dbgflg & 0x02)) {
		    if (verbose_reg_read)
			printf("%02x: %08x\n", addr, data&0xffffffff);
		    else
			printf("%08x ", data&0xffffffff);
		}
		addr += 4;
	    }
	    if (!(dbgflg & 0x02))   printf("\n");
	}

	else if (strcmp(tbuf, "irv") == 0)	{
	    int addr, data, n, cnt;
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &addr);
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &cnt);
	    if (n!=1)  cnt=1;
	    for (n=0; n<cnt; n++) {
	        data = ipr8(addr) ;
	        if (!(dbgflg & 0x02))   printf("%02x: %02x\n", addr, data&0xff);
		addr += 1;
	    }
	    if (!(dbgflg & 0x02))   printf("\n");
	}
	else if (strcmp(tbuf, "Ir") == 0)	{
	    int addr, n, cnt;
	    u_int data;

	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &addr);
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &cnt);
	    if (n!=1)  cnt=1;
	    for (n=0; n<cnt; n++) {
	        data = Ipr8(addr) ;
	        if (!(dbgflg & 0x02)) {
		    if (verbose_reg_read)
			printf("%02x: %08x\n", addr, data);
		    else
			printf("%08x ", data);
		}
		addr += 1;
	    }
	    if (!(dbgflg & 0x02))   printf("\n");
	}

	else if (strcmp(tbuf, "Irv") == 0)	{
	    int addr, n, cnt;
	    u_int data;

	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &addr);
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &cnt);
	    if (n!=1)  cnt=1;
	    for (n=0; n<cnt; n++) {
	        data = Ipr8(addr) ;
	        if (!(dbgflg & 0x02))   printf("%02x: %08x\n", addr, data);
		addr += 1;
	    }
	    if (!(dbgflg & 0x02))   printf("\n");
	}

	else if (strcmp(tbuf, "w") == 0)	{
	    int addr, data, n;
 	    int first=1;
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &addr);
	    if (n==1) do {
	        gettok(tbuf, &bufp);
	        n=sscanf(tbuf,"%x", &data);
	        if (n==1)   {
	    	    pw8(addr,data) ;
		    addr += 1;
		    first=0;
		}
	    } while (n==1);
	    else printf("    bad address");
	    if (first==1)  printf("  nothing written\n");
	}

	else if (strcmp(tbuf, "iw") == 0)	{
	    int addr, data, n;
 	    int first=1;
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &addr);
	    if (n==1) do {
	        gettok(tbuf, &bufp);
	        n=sscanf(tbuf,"%x", &data);
	        if (n==1)   {
	    	    ipw8(addr,data) ;
		    addr += 1;
		    first=0;
		}
	    } while (n==1);
	    else printf("    bad address");
	    if (first==1)  printf("  nothing written\n");
	}

	else if (strcmp(tbuf, "iw16") == 0)	{
	    int addr, data, n;
 	    int first=1;
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &addr);
	    if (n==1) do {
	        gettok(tbuf, &bufp);
	        n=sscanf(tbuf,"%x", &data);
	        if (n==1)   {
	    	    ipw16(addr,data) ;
		    addr += 2;
		    first=0;
		}
	    } while (n==1);
	    else printf("    bad address");
	    if (first==1)  printf("  nothing written\n");
	}

	else if (strcmp(tbuf, "iw32") == 0)	{
	    int addr, n;
	    u_int data;
 	    int first=1;
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &addr);
	    if (n==1) do {
	        gettok(tbuf, &bufp);
	        n=sscanf(tbuf,"%x", &data);
	        if (n==1)   {
	    	    ipw32(addr,data) ;
		    addr += 4;
		    first=0;
		}
	    } while (n==1);
	    else printf("    bad address");
	    if (first==1)  printf("  nothing written\n");
	}

	else if (strcmp(tbuf, "Iw") == 0)	{
	    int addr, n;
	    u_int data;
 	    int first=1;
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &addr);
	    if (n==1) do {
	        gettok(tbuf, &bufp);
	        n=sscanf(tbuf,"%x", &data);
	        if (n==1)   {
	    	    Ipw8(addr,data) ;
		    addr += 1;
		    first=0;
		}
	    } while (n==1);
	    else printf("    bad address");
	    if (first==1)  printf("  nothing written\n");
	}

	else if (strcmp(tbuf, "r16") == 0)	{
	    int addr, data, n, cnt;
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &addr);
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &cnt);
	    if (n!=1)  cnt=1;
	    for (n=0; n<cnt; n++) {
	        data = pr16(addr) ;
	        if (!(dbgflg & 0x02))   printf("%04x ", data&0xffff);
		addr += 2;
	    }
	    if (!(dbgflg & 0x02))   printf("\n");
	}

	else if (strcmp(tbuf, "w16") == 0)	{
	    int addr, data, n;
 	    int first=1;
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &addr);
	    if (n==1) do {
	        gettok(tbuf, &bufp);
	        n=sscanf(tbuf,"%x", &data);
	        if (n==1)   {
	    	    pw16(addr,data) ;
		    addr += 2;
		    first=0;
		}
	    } while (n==1);
	    else printf("    bad address");
	    if (first==1)  printf("  nothing written\n");
	}

	else if (strcmp(tbuf, "xr8") == 0)	{
	    int addr, data, n, cnt;
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &addr);
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &cnt);
	    if (n!=1)  cnt=1;
	    for (n=0; n<cnt; n++) {
	        data = xr(0x01000000+addr) ;
	        if (!(dbgflg & 0x02))   printf("%02x ", data);
		addr += 1;
	    }
	    if (!(dbgflg & 0x02))   printf("\n");
	}

	else if (strcmp(tbuf, "xw8") == 0)	{
	    int addr, data, n;
 	    int first=1;
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &addr);
	    if (n==1) do {
	        gettok(tbuf, &bufp);
	        n=sscanf(tbuf,"%x", &data);
	        if (n==1)   {
	    	    xw(0x01000000+addr, data) ;
		    addr += 1;
		    first=0;
		}
	    } while (n==1);
	    else printf("    bad address");
	    if (first==1)  printf("  nothing written\n");
	}

	else if (strcmp(tbuf, "xr16") == 0)	{
	    int addr, data, n, cnt;
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &addr);
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &cnt);
	    if (n!=1)  cnt=1;
	    for (n=0; n<cnt; n++) {
	        data = xr(0x02000000+addr) ;
	        if (!(dbgflg & 0x02))   printf("%04x ", data);
		addr += 2;
	    }
	    if (!(dbgflg & 0x02))   printf("\n");
	}

	else if (strcmp(tbuf, "xw16") == 0)	{
	    int addr, data, n;
 	    int first=1;
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &addr);
	    if (n==1) do {
	        gettok(tbuf, &bufp);
	        n=sscanf(tbuf,"%x", &data);
	        if (n==1)   {
	    	    xw(0x02000000+addr, data) ;
		    addr += 2;
		    first=0;
		}
	    } while (n==1);
	    else printf("    bad address");
	    if (first==1)  printf("  nothing written\n");
	}

	else if (strcmp(tbuf, "xr32") == 0)	{
	    int addr, data, n, cnt;
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &addr);
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &cnt);
	    if (n!=1)  cnt=1;
	    for (n=0; n<cnt; n++) {
	        data = xr(0x04000000+addr) ;
	        if (!(dbgflg & 0x02))   printf("%08x ", data);
		addr += 4;
	    }
	    if (!(dbgflg & 0x02))   printf("\n");
	}

	else if (strcmp(tbuf, "xw32") == 0)	{
	    int addr, data, n;
 	    int first=1;
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &addr);
	    if (n==1) do {
	        gettok(tbuf, &bufp);
	        n=sscanf(tbuf,"%x", &data);
	        if (n==1)   {
	    	    xw(0x04000000+addr, data) ;
		    addr += 4;
		    first=0;
		}
	    } while (n==1);
	    else printf("    bad address");
	    if (first==1)  printf("  nothing written\n");
	}

	else if (strcmp(tbuf, "xrcfg") == 0)	{
	    int addr, data, n, cnt;
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &addr);
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &cnt);
	    if (n!=1)  cnt=1;
	    for (n=0; n<cnt; n++) {
	        data = pr_cfg(addr) ;
	        if (!(dbgflg & 0x02))   printf("%08x ", data);
		addr += 4;
	    }
	    if (!(dbgflg & 0x02))   printf("\n");
	}

	else if (strcmp(tbuf, "xwcfg") == 0)	{
	    int addr, data, n;
 	    int first=1;
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &addr);
	    if (n==1) do {
	        gettok(tbuf, &bufp);
	        n=sscanf(tbuf,"%x", &data);
	        if (n==1)   {
	    	    pw_cfg(addr, data) ;
		    addr += 4;
		    first=0;
		}
	    } while (n==1);
	    else printf("    bad address");
	    if (first==1)  printf("  nothing written\n");
	}
	else if (strcmp(tbuf, "rm016") == 0)	{
	    int addr, data, n, cnt;
	    if (check_mmap(0)) continue ;
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &addr);
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &cnt);
	    if (n!=1)  cnt=1;
	    for (n=0; n<cnt; n++) {
	    	data = mmap_read_16(edt_p, addr, 0);
	        if (!(dbgflg & 0x02))   printf("%04x ", data&0xffff);
		addr += 2;
	    }
	    if (!(dbgflg & 0x02))   printf("\n");
	}

	else if (strcmp(tbuf, "wm016") == 0)	{
	    int addr, data, n;
 	    int first=1;
	    if (check_mmap(0)) continue ;
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &addr);
	    if (n==1) do {
	        gettok(tbuf, &bufp);
	        n=sscanf(tbuf,"%x", &data);
	        if (n==1)   {
	    	    mmap_write_16(edt_p, addr, data, 0);
		    addr += 2;
		    first=0;
		}
	    } while (n==1);
	    else printf("    bad address");
	    if (first==1)  printf("  nothing written\n");
	}
	else if (strcmp(tbuf, "rm032") == 0)	{
	    int addr, data, n, cnt;
	    if (check_mmap(0)) continue ;
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &addr);
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &cnt);
	    if (n!=1)  cnt=1;
	    for (n=0; n<cnt; n++) {
	    	data = mmap_read_32(edt_p, addr, 0);
	        if (!(dbgflg & 0x02))   printf("%08x ", data) ;
		addr += 4;
	    }
	    if (!(dbgflg & 0x02))   printf("\n");
	}

	else if (strcmp(tbuf, "wm032") == 0)	{
	    int addr, data, n;
 	    int first=1;
	    if (check_mmap(0)) continue ;
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &addr);
	    if (n==1) do {
	        gettok(tbuf, &bufp);
	        n=sscanf(tbuf,"%x", &data);
	        if (n==1)   {
	    	    mmap_write_32(edt_p, addr, data, 0);
		    addr += 4;
		    first=0;
		}
	    } while (n==1);
	    else printf("    bad address");
	    if (first==1)  printf("  nothing written\n");
	}
	else if (strcmp(tbuf, "rm116") == 0)	{
	    int addr, data, n, cnt;
	    if (check_mmap(1)) continue ;
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &addr);
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &cnt);
	    if (n!=1)  cnt=1;
	    for (n=0; n<cnt; n++) {
	    	data = mmap_read_16(edt_p, addr, 1);
	        if (!(dbgflg & 0x02))   printf("%04x ", data&0xffff);
		addr += 2;
	    }
	    if (!(dbgflg & 0x02))   printf("\n");
	}

	else if (strcmp(tbuf, "wm116") == 0)	{
	    int addr, data, n;
 	    int first=1;
	    if (check_mmap(1)) continue ;
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &addr);
	    if (n==1) do {
	        gettok(tbuf, &bufp);
	        n=sscanf(tbuf,"%x", &data);
	        if (n==1)   {
	    	    mmap_write_16(edt_p, addr, data, 1);
		    addr += 2;
		    first=0;
		}
	    } while (n==1);
	    else printf("    bad address");
	    if (first==1)  printf("  nothing written\n");
	}
	else if (strcmp(tbuf, "rm132") == 0)	{
	    int addr = 0, data, n, cnt;
	    if (check_mmap(1)) continue ;
	    gettok(tbuf, &bufp);
	    sscanf(tbuf,"%x", &addr);
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &cnt);
	    if (n!=1)  cnt=1;
	    for (n=0; n<cnt; n++) {
	    	data = mmap_read_32(edt_p, addr, 1);
	        if (!(dbgflg & 0x02))   printf("%08x ", data) ;
		addr += 4;
	    }
	    if (!(dbgflg & 0x02))   printf("\n");
	}

	else if (strcmp(tbuf, "wm132") == 0)	{
	    int addr, data, n;
 	    int first=1;
	    if (check_mmap(1)) continue ;
	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &addr);
	    if (n==1) do {
	        gettok(tbuf, &bufp);
	        n=sscanf(tbuf,"%x", &data);
	        if (n==1)   {
	    	    mmap_write_32(edt_p, addr, data, 1);
		    addr += 4;
		    first=0;
		}
	    } while (n==1);
	    else printf("    bad address");
	    if (first==1)  printf("  nothing written\n");
	}



	else if (strcmp(tbuf, "rp") == 0)  {
	    int d, n;   int buf[16];
	    char c;
	    for (n=0; n<16; n++) {
		d=xr(0x020000C2);
		buf[n]=d;
		if (!(d & 0x8000))  break;
		printf("%04x ", d);
	    }
	    printf("%04x\n", d);

	    for (n=0; n<16; n++) {	/* Print text, skipping routing byte */
		if (!(buf[n] & 0x8000)) break;
		c = buf[n];
		if ((c<0x20) || (c>0x7e))  c='.';
		if ((n>1) || ((n==1) && ((buf[0]&0xff)==0x3F)))  putchar(c);
	    }
	    if (n>1)  putchar('\n');
	}

	else if (strcmp(tbuf, "wp") == 0)	{
	    int data, n;
 	    int first=1;

	    data=xr(0x010000C5);
	    if (data&0xa0) {
		printf("Clearing interrupts,  REG_C5:=02\n");
	    	xw(0x010000c5, 0x02);
	    }

	    do {
	        gettok(tbuf, &bufp);
	        n=sscanf(tbuf,"%x", &data);
	        if (n==1)   {
	    	    xw(0x020000C0, data & 0x00ff) ;
		    first=0;
		}
	    } while (n==1);
	    xw(0x020000C0, 0x0100) ;	/* Terminate packet */
	    
	    if (first==1)  printf("    empty packet sent");
	}

	else if (strcmp(tbuf, "rdma") == 0)	{
	    int bcnt, gotcnt, n;
	    FILE *fd2;

	    n=gettok(tbuf, &bufp);
	    if (n!=1) { 
		fprintf(stderr, "Syntax error\n"); 
		continue; 
	    }
	    if ((fd2 = fopen(tbuf, "wb")) == NULL)	{
		fprintf(stderr, "Error opening file %s for output\n", tbuf);
		continue;
	    }

	    gettok(tbuf, &bufp);
	    n=sscanf(tbuf,"%x", &bcnt);
	    if (n!=1) { 
		fprintf(stderr, "Syntax error\n"); 
		continue; 
	    }
	    if (bcnt >DMABUFSZ) {
		fprintf(stderr, "Max bytecount of %x\n", DMABUFSZ);
		continue;
	    }

	    edt_set_rtimeout(edt_p, 0);
printf("edtread( edt_p:%p, dmabuf:%p, bcnt:%d \n", edt_p, dmabuf, bcnt);
	    gotcnt = edt_read(edt_p, dmabuf, bcnt);
gotcnt = edt_get_bytecount(edt_p) ;
printf("bytecount %d\n", gotcnt) ;

	    if (gotcnt != bcnt) {
	        fprintf(stderr, "asked for %x bytes, got %x bytes", bcnt, gotcnt);
	    }
	    n = fwrite(dmabuf, 1, gotcnt, fd2);
	    fclose(fd2);
	    if (n!=bcnt) fprintf(stderr, "failed on writing to output file\n");
	}


	/* Read current PCI config registers to /tmp/p53dbg_unit#.cfg */
	else if	(strcmp(tbuf, "cfgr") == 0)	{
	    char cfg_filename[32];
	    int  addr, data;
	    FILE *fd2;
	    sprintf(cfg_filename, "/tmp/p53dbg_%d.cfg", unit); 
	    if ((fd2 = fopen(cfg_filename,	"wb")) == NULL)	{
		fprintf(stderr,"cfgr: Couldn't write to %s\n", cfg_filename);
		continue;
	    }
	    for	(addr=0; addr<=0x3c; addr+=4) {
		data  = pr_cfg(addr);
		putc(data, fd2);
		putc(data>>8, fd2);
		putc(data>>16, fd2);
		putc(data>>24, fd2);
		printf("%02x:  %08x\n", addr, data);
	    }
	    fclose(fd2);
	    printf("Wrote config space state out to %s\n", cfg_filename);
	}

	/* Write /tmp/p53dbg_unit#.cfg out to PCI config registers */
	else if	(strcmp(tbuf, "cfgw") == 0)	{
	    char cfg_filename[32];
	    int addr, old, copy, new_one;
	    FILE *fd2;
	    sprintf(cfg_filename, "/tmp/p53dbg_%d.cfg", unit); 
	    if ((fd2 = fopen(cfg_filename,	"rb")) == NULL)	{
		fprintf(stderr,"cfgw: Couldn't read from %s\n", cfg_filename);
		continue;
	    }
	    printf("Overwriting config space registers with %s\n", cfg_filename);
	    printf("Hit any key to continue: "); fflush(stdout); 
	    getchar(); printf("\n");
	    
	    printf("	 old	   copy	     new_one\n");
	    for	(addr=0; addr<=0x3c; addr+=4) {
		old  = pr_cfg(addr);

		copy  =  getc(fd2)&0xff;
		copy |= (getc(fd2)&0xff)<<8;
		copy |= (getc(fd2)&0xff)<<16;
		copy |= (getc(fd2)&0xff)<<24;
		pw_cfg(addr, copy) ;

		new_one  = pr_cfg(addr);

		printf("%02x:  %08x  %08x  %08x	 ", addr, old, copy, new_one);
		if	(copy != new_one)	printf("ERROR\n");
		else if	(old  != new_one)	printf("changed\n");
		else			printf("\n");

	    }
	    fclose(fd2);
	}

	else if	(strcmp(tbuf, "reboot") == 0)	{
	    char cfg_filename[32];
	    int  addr, data;
	    int  buf[0x40];
 	    int  old, copy, new_one;
	    FILE *fd2;
	    sprintf(cfg_filename, "./pdb_reboot_%d.cfg", unit); 
	    if ((fd2 = fopen(cfg_filename, "wb")) == NULL)	{
		fprintf(stderr,"cfgr: Couldn't write to %s\n", cfg_filename);
		continue;
	    }
	    for	(addr=0; addr<=0x3c; addr+=4) {
		data  = pr_cfg(addr);
		buf[addr] = data;
		putc(data, fd2);
		putc(data>>8, fd2);
		putc(data>>16, fd2);
		putc(data>>24, fd2);
		/* printf("%02x:  %08x\n", addr, data); */
	    }
	    fclose(fd2);
	    printf("Wrote config space state out to %s\n", cfg_filename);

	    edt_reg_write(edt_p, 0x01000085, 0x40) ;
	    edt_msleep(500) ;

	    printf("	 old	   copy	     new_one\n");
	    for	(addr=0; addr<=0x3c; addr+=4) {
		old  = pr_cfg(addr);
		copy  =  buf[addr];
		pw_cfg(addr, copy) ;
		new_one  = pr_cfg(addr);

		printf("%02x:  %08x  %08x  %08x	 ", addr, old, copy, new_one);
		if	(copy != new_one)	printf("ERROR\n");
		else if	(old  != new_one)	printf("changed\n");
		else			printf("\n");

	    }
	}

	


	else if (strcmp(tbuf, "L") == 0)	{  /* loop on next command */
	    int n;
	    n=sscanf(bufp,"%d", &repeat);
	    if (n<=0)  repeat=0;
            if (n==0)  repeat=-1;
	    repeat_flag = 0;
	}

	else if (strcmp(tbuf, "b") == 0)  {
	    char sbuf[PDBBUFSIZE];
	    int n;
	    n=gettok(tbuf, &bufp);
	    if (n==1)	{
	        sprintf(sbuf, "%s %d %s", progname, unit, tbuf);
	        throwaway_int = system(sbuf);
	    }
	    else
	    {
		fprintf(stderr, "usage: b  batchfile\n");
	    }
	}

	else if (strcmp(tbuf, "sys") == 0)  {
	    throwaway_int = system(bufp);
	}

	else if (strcmp(tbuf, "q") == 0)  {
	    if (edt_p) edt_close(edt_p) ;
#ifdef NO_MAIN
		return(0) ;
#else
		exit(0) ;
#endif
	}

	else fprintf(stderr, "huh?\n");

    }
#ifdef NO_MAIN
    return(0) ;
#else
    exit(0) ;
#endif
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
		if (c=='\0')  { *tbuf = 0;  return(1);  }
    }  while (!isspace(c));

    *tbuf=0;
    return(1);
}



#if 0
#define DQ7 0x80
#define DQ5 0x20
#define DQ3 0x08

static u_char
flipbits(u_char val)
{
    int             i;
    u_char          ret = 0;

    for (i = 0; i < 8; i++)
    {
	if (val & (1 << i))
	    ret |= 0x80 >> i;
    }
    return (ret);
}
#endif




/*

spi_res:  read electronic signature
spi_rdsr: read status reg
spi_wdsr XX: write status reg


  reboot	cfgr, xw8 85 40, cfgw, spi_res, spi_wrsr 00/88/90, spi_rdsr\n\
  spie 0/1/2/3, spiw/spiv addr file, spir addr cnt [file] \n\
 Also have:    spi_syncbytes file.bit 
      
        else if (strcmp(tbuf, "spi_res") == 0)  {
        else if (strcmp(tbuf, "spi_rdsr") == 0)  {
        else if (strcmp(tbuf, "spi_wrsr") == 0)  {
        else if (strcmp(tbuf, "spir") == 0)  {
        else if (strcmp(tbuf, "spie") == 0)  {
        else if ((strcmp(tbuf, "spiw") == 0) || (strcmp(tbuf, "spiv") == 0)) {
            if (strcmp(tbuf, "spiw") == 0)  wren=1;  else wren=0;
        else if (strcmp(tbuf, "spiwo") == 0) {
        else if (strcmp(tbuf, "spi_syncbytes") == 0)  {
*/




/* spi_res spi_rdsr spi_wrsr spir spie spiw spi_syncbytes */













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
#define SPI_RDID  0x9E	/* Read ID,       19 rbytes */
// #define SPI_DP    0xB9	/* Power Down,    0 params                   */
// #define SPI_RES   0xAB	/* Read Sig,      3 wdummy,         1 rbyte  (DP off)*/


/*   Inside the PAL:
  fs(0)<=not fpg(0);  fs(1)<=not fpg(1);  fs(2)<=not fpg(2);  fs(3)<=not fpg(3);
  fc(0) <= fpg4q;   
  fd0<=fpg(5); 
  fpg7<=(fd1 and fpg(0))or(fd5 and fpg(1))or(fd9 and fpg(2))or(fd13 and fpg(3));
  ngrnled<= not fpg(6);
*/

#define SPI_WREG  0x0001200
#define SPI_SOFF  0x0F		/* All select lines high, so flash disabled */
#define SPI_C     0x10		/* Clock */
#define SPI_D     0x20		/* Data Write */

#define SPI_RREG  0x0001200
#define SPI_Q     0x100		/* Data bit back from spi flash */



/* Bit definitions for register IO to SPI control lines */
//  #define SPI_WREG  0x07		/* Write bits out to aiag Mode Reg */
//  #define SPI_EN    0x00		/* was 0x08,    Enable S,D,C out to PAL */
//  #define SPI_S     0x04		/* Select, low true */
//  #define SPI_D     0x02		/* Data Write */
//  #define SPI_C     0x01		/* Clock */
//    (data)  wm132(0x101200, data)
//  
//  #define SPI_RREG  0x01		/* Read bit from aiag Status Reg */
//  #define SPI_Q     0x02		/* Data is read from FV bit */



// mmap_write_32(edt_p, addr, data, 1);
// data = mmap_read_32(edt_p, addr, 1);


void spi_wreg(int v)	/* Write to IO port to drive S,C,D pins */
{  /* Make bit 3 parity over bits 1,2, gets checked by RCX-CLink Xilinx */
   // int p;  p=((v>>2)&1)+((v>>1)&1);  v=(v&0xf7)|((p&1)<<3);
   // if (dbgflg&0x100)    v |= 0x08;


   // Slow down write strobes to spiflash with a read
   int val;
   val = mmap_read_32(edt_p, SPI_RREG, 1) & SPI_Q;

   v |= spi_selectn;
   if ((dbgflg&0x80) == 0) {
       // edt_msleep(1);
       mmap_write_32(edt_p, SPI_WREG, v, 1);
       // pw8(SPI_WREG, SPI_EN | v);
   }
   if (dbgflg&0x200)    {
	printf("%d %d %d %d  %d %d %d %d  ",(v>>7)&1,(v>>6)&1,(v>>5)&1,(v>>4)&1,(v>>3)&1,(v>>2)&1,(v>>1)&1,(v>>0)&1);
	printf("    %02x\n", v&0xff);
    }
}

int spi_rreg()		/* Read the IO port to get Q pin status */
{
    int val;
    static int fakein = 0x456789AB;

    if (dbgflg&0x80) {
        fakein = ((fakein<<1) & 0xFFFFFFFE) | ((fakein>>31) & 1); 
        val = fakein & 1;
    }
    else    {
        val = mmap_read_32(edt_p, SPI_RREG, 1) & SPI_Q;
        val = mmap_read_32(edt_p, SPI_RREG, 1) & SPI_Q;
        // val = pr8(SPI_RREG) & SPI_Q;
    }
    if (val)  val=1;

    if (dbgflg & 0x10) {
    	printf("        %d    %08x\n", val, fakein);
    }
    return(val);
}

void printbuf(char* rbuf, int rbytes)
{
    int n;
    printf("    printbuf:");
    for (n=0; n<rbytes; n++)  {
        printf("%02x ", rbuf[n]&0xff);
    }
    printf("\n");
}



void spiprim(int opcode, int wbytes, int rbytes, char* wbuf, char* rbuf) 
{
    int c, b, cdat;

    spi_wreg(SPI_SOFF);		/* Make sure Select is high */
    spi_wreg(0);		/*  then lower the Select line to select device */

    cdat = opcode;
    for (b=0; b<8; b++)  {		/* for each bit in the char */
        if (cdat & 0x80) {
	    spi_wreg(SPI_D);
	    spi_wreg(SPI_D | SPI_C);
        } else {
	    spi_wreg(0);
	    spi_wreg(SPI_C);
        }
        cdat <<= 1;
    }

    for (c=0; c<wbytes; c++) {		/* for each char in wbuf */
	cdat = wbuf[c];
	for (b=0; b<8; b++)  {		/* for each bit in the char */
	    if (cdat & 0x80) {
    		spi_wreg(SPI_D);
    		spi_wreg(SPI_D | SPI_C);
	    } else {
    		spi_wreg(0);
    		spi_wreg(SPI_C);
	    }
	    cdat <<= 1;
	}
    }
    spi_wreg(0);

    for (c=0; c<rbytes; c++) {		/* for each char in rbuf */
	cdat = 0;
	for (b=0; b<8; b++)  {		/* for each bit in the char */
	    cdat <<= 1;
	    if (spi_rreg() & 1)  
		cdat |= 1;
	    if ((b!=7) || (c!=(rbytes-1)))  {
    	        spi_wreg(SPI_C);
    	        spi_wreg(0);
	    }
	}
	rbuf[c] = cdat;
    }

    spi_wreg(SPI_SOFF);		/* Raise the Select line (to deselect) */
}


int spi_res()		 	           /* Read electronic signature */
{
    char rval;
    // spiprim(SPI_RES, 3, 1, "555", &rval);   /* For M25P20 should be 0x11/0x13/0x14 */
    spiprim(SPI_RDID, 0, 1, NULL, &rval);   /* For M25P20 should be 0x11/0x13/0x14 */
    return(rval&0xff);                      /* Also out of Power Down mode  */
}

void spi_rdid(char *rvals)	           /* Read electronic signature */
{
    spiprim(SPI_RDID, 0, 19, NULL, rvals);
    return;
}

int spi_rdsr()
{
    char rval;
    spiprim(SPI_RDSR, 0, 1, NULL, &rval);    	/* Read Status reg */
    return(rval&0xff);
}

void spi_wrsr(int val)
{
    char wval;
    wval = val;
    spiprim(SPI_WREN, 0, 0, NULL, NULL);	/* write-enable */
    spiprim(SPI_WRSR, 1, 0, &wval, NULL);    	/* Write Status reg */
}


/* Read rcnt bytes starting from address addr, store in global rbuf[] */
void spi_read(int addr,  int rcnt)
{
    if ((rcnt<1) || (rcnt > 256))   {
	printf("Error, bad spi_read() rcnt of %d\n", rcnt);
	exit(1);
    }
    wbufa[0]=addr>>16; wbufa[1]=addr>>8; wbufa[2]=addr;     /* 24 bit address */
    wbufa[3]=0;  				            /* dummy byte */
    spiprim(SPI_FREAD, 4, rcnt, wbufa, rbuf);    
}

void spi_write(int addr, int wcnt)
{
    int statcnt;

    if (dbgflg & 1) {
	printf("spi_write(%06x %x)\n", addr, wcnt); 
    }
    if ((wcnt<1) || (wcnt>256))   {
	printf("Error, bad spi_write() wcnt of %d\n", wcnt);
	exit(1);
    }
    spiprim(SPI_WREN, 0, 0, NULL, NULL);		    /* write-enable */
    wbufa[0]=addr>>16; wbufa[1]=addr>>8; wbufa[2]=addr;     /* 24 bit address */

/***debug***/ 
if (dbgflg&1) {
  int n; 
  for (n=0; n<10; n++) 
      printf("%02x ", wbufa[n]);  
  printf("\n"); 
}

    spiprim(SPI_PP,  3+wcnt, 0, wbufa, NULL);		    /* page program */

    statcnt=0;
    do {
        spiprim(SPI_RDSR, 0, 1, NULL, rbuf);    	/* Read Status reg */
	statcnt++;
    } while ((rbuf[0] & 1) == 1);		/* Could hang here forever? */
    if (dbgflg & 1) {
        printf("Final status was %02x, read %d times\n",  rbuf[0], statcnt);
    }
}

void spi_erase(int addr)
{
    int statcnt;
    if (dbgflg & 1) {
	printf("spi_erase(%06x)\n", addr); 
    }
    spiprim(SPI_WREN, 0, 0, NULL, NULL);		    /* write-enable */
    wbufa[0]=addr>>16; wbufa[1]=addr>>8; wbufa[2]=addr;     /* 24 bit address */
    spiprim(SPI_SE,  3, 0, wbufa, NULL);    

    statcnt=0;
    do {
        spiprim(SPI_RDSR, 0, 1, NULL, rbuf);    	/* Read Status reg */
	statcnt++;
    } while ((rbuf[0] & 1) == 1);		/* Could hang here forever? */
    if (dbgflg & 1) {
        printf("Final status was %02x, read %d times\n",  rbuf[0], statcnt);
    }
}


// void spi_test()
// {
//     int val, c, n;
// 
//     /* Verify we can talk to the SPI part by reading the part ID */
//     val = spi_res();
//     if ((val != 0x11) && (val != 0x13) && (val != 0x14))  {
// 	printf("Expecting ID of 0x11/0x13/0x14 from M25P20 flash, got %02x\n", val);
// 	printf("Continue anyway?  (Y to continue, N to quit)\n", rbuf[0]);
// 	c = getchar();
//         if ((c!='Y') && (c!='y'))  {
// 	    printf("Aborting routine\n");
// 	    return;   /* exit(1); */
// 	}
//     }
// 
//     spi_read(0x000102, 16);
//     printbuf(rbuf, 16);
// 
//     spi_erase(0x000102);
// 
//     spi_read(0x000102, 16);
//     printbuf(rbuf, 16);
// 	    
//     for (n=0; n<256; n++)   wbuf[n]=n;
//     spi_write(0x000102, 256);
// 
//     spi_read(0x000102, 16);
//     printbuf(rbuf, 16);
// 
// }

