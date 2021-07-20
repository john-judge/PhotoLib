/* #pragma ident "@(#)pdb.c	1.64 10/28/10 EDT" */

#include "edtinc.h"

#include <ctype.h> /* isspace() */

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
    u_short dbgflg =0;
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
  puts("  q                    Quit the program \n");
}



	else if (strcmp(tbuf, "dbgflg") == 0)	{
	    sscanf(bufp,"%hd", &dbgflg);
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

