

/*
 * Query camera for x and y dimentions using camtest2.rbt.
 */

#define ABOUT_TEXT "Determines pixels/line and lines/frame from camera"


#include "edtinc.h"
#include "edtregbits.h"
#include <stdlib.h>


/* 
 * setting expsize will keep statistics and toggle mode code line to trigger logic analyzer
 * setting rowsize will cause hardware to put out pulse on u71 pin 15 when miscompare
 */

void usage(char *progname, char *err) {
	
    puts("usage: checkcam [-u pdv_unit_no] [-s expsize] [-r rowsize]\n");
	puts(err);
    puts(ABOUT_TEXT);
	puts("-u <unit>	    - pdv unit number (default 0)");
	puts("-s <expsize>	- setting expsize will keep statistics and toggle mode code line to trigger logic analyzer");
	puts("-r <rowsize>	- setting rowsize will cause hardware to put out pulse on u71 pin 15 when miscompare");
	exit(1);
}

int i,x ;
int expsize = 0 ;
int rowsize = 0 ;
int done = 0 ;

void
gotsig(sig)
int sig ;
{
    done = 1 ;
}


EdtDev *edt_p ;

void checkcount() ;
void checkclcount() ;
static void PrintRegister(EdtDev *edt_p, char *name, int offset, BitLabels *labels);
static void GetFlagsString(char *s, BitLabels * labels, long flags);

int
main(argc,argv)
int argc;
char *argv[];
{
    int pdv_unit = 0 ;
    char *unitstr = "0";
	char *progname = argv[0];
	char device[32]; 

#ifndef _NT_
    signal(SIGINT,gotsig) ;
#endif

    while (argc > 1)
    {
	if (strcmp(argv[1], "-u") == 0)
	{
	    argc--;
	    argv++;
	    if (argc < 2) 
        {
		    usage(progname, "Error: option 'u' requires a numeric argument\n");
	    }
	    if ((argv[1][0] >= '0') && (argv[1][0] <= '9'))
        {
            unitstr = argv[1];
	        argc--;
	        argv++;
        }
        else 
        {
		    usage(progname, "Error: option 'u' requires a numeric argument\n");
	    }
	}
	else if (strcmp(argv[1], "-s") == 0)
	{
	    argc--;
	    argv++;
	    if (argc < 2) 
        {
		    usage(progname, "Error: option 's' requires a numeric argument\n");
	    }
	    if ((argv[1][0] >= '0') && (argv[1][0] <= '9'))
        {
            expsize = atoi(argv[1]);
	        argc--;
	        argv++;
        }
        else 
        {
		    usage(progname, "Error: option 's' requires a numeric argument\n");
	    }
	}
	else if (strcmp(argv[1], "-r") == 0)
	{
	    argc--;
	    argv++;
	     if (argc < 2) 
        {
		    usage(progname, "Error: option 's' requires a numeric argument\n");
	    }
	    if ((argv[1][0] >= '0') && (argv[1][0] <= '9'))
        {
            rowsize = atoi(argv[1]);
	        argc--;
	        argv++;
        }
        else 
        {
		    usage(progname, "Error: option 's' requires a numeric argument\n");
	    }
	}
	else
	{
		usage(progname, "unknown option");
	}
    }
	    
    pdv_unit = edt_parse_unit(unitstr, device, NULL);
    
    if ((edt_p = edt_open(device, pdv_unit)) == NULL)
    {
	    char errstr[80] ;
	    sprintf(errstr, "edt_open(%s%d)", EDT_INTERFACE, pdv_unit) ;
	    edt_perror(errstr) ;
	    puts("Hit return to exit") ;
	    getchar() ;
	    return(1) ;
    }

    /* check for what kind of board and do the appropriate thing */
    switch(edt_p->devid)
    {
	case PDV_ID:
	case PDVK_ID:
	case PDV44_ID:
	case PDVFCI_USPS_ID:
	case PDVFCI_AIAG_ID:
	case PC104ICB_ID:
	    checkcount();
	    break;

	case PDVFOX_ID:
	case PDVCL_ID:
	case PE1DVVL_ID:
	case PE4DVVL_ID:
	case PE4DVCL_ID:
	case PE8DVCL_ID:
	case PE4DVFCI_ID:
	case PE8DVFCI_ID:
	case PE4DVFOX_ID:
	case PE8DVFOX_ID:
	case PE4DVVLFOX_ID:
	    checkclcount();
	    break;

	default:
	    printf("unit %d device is of a type (0x%02x) that is not supported by checkcam\n",
	    							pdv_unit, edt_p->devid); 
	    break;
    }

    edt_close(edt_p);
    puts("Done\n");
    exit(0);
}

/*
 * checkcount -- camera link version
 * just look at camera counter registers (added 2/10/2010 RWH since no one seems to read 
 * the part about checkcam isn't for CL)
 */
void
checkclcount()
{
    PrintRegister(edt_p, "CLOCKSPERLINE", PDV_CL_PIXELSPERLINE, NULL);
    PrintRegister(edt_p, "LINESPERFRAME", PDV_CL_LINESPERFRAME, NULL);
}

/*
 * checkcount -- non-camera link version
 */
void
checkcount()
{
    u_long msvalue ;
    u_long lsvalue ;
    u_long lines ;
    u_long pixels ;
    u_long stat ;
    int new ;
    int have_fv = 1 ;
    int have_lv = 1 ;
    int frame ;
    int good ;
    int bad ;
    double dtime ;
    int framecnt = 0 ;
    int linecnt = 0 ;
    int pixelcnt = 0 ;

    frame = 0 ;
    good = 0 ;
    bad = 0 ;


    /* set for comparison - will trigger la if not equal */
    edt_reg_write(edt_p, CAM_COMPARE, rowsize) ;

	(void)edt_dtime() ;
    /* wait for start of frame valid */
    printf("waiting for frame valid\n") ;
    dtime = 0 ;
    while (((stat = edt_reg_read(edt_p, CAM_STAT)) & CAM_FV) == 0)
    {
		dtime += edt_dtime() ;
		if (dtime > 5.0)
		{
			printf("no frame valid in 5 seconds - looking for line valid\n") ;
			dtime = 0.0 ;
			have_fv = 0 ;
			while (((stat = edt_reg_read(edt_p, CAM_STAT)) & CAM_LV) == 0)
			{
				dtime += edt_dtime() ;
				if (dtime > 5.0)
				{
					printf("no line valid in 5 seconds\n") ;
					have_lv = 0 ;
					break ;
				}
			}
			printf("got LV\n") ;
			break ;
		}
    }
	printf("waiting for not frame valid\n") ;
	dtime = 0.0 ;
    while ((stat = edt_reg_read(edt_p, CAM_STAT)) & CAM_FV)
    {
		dtime += edt_dtime() ;
		if (!have_fv || dtime > 5.0)
		{
			printf("no drop in frame valid  - looking for line valid\n") ;
			have_fv = 0 ;
			dtime = 0.0 ;
			while ((stat = edt_reg_read(edt_p, CAM_STAT)) & CAM_LV)
			{
				dtime += edt_dtime() ;
				if (dtime > 5.0)
				{
					printf("no line valid in 5 seconds\n") ;
					have_lv = 0 ;
					break ;
				}
			}
			printf("got not LV\n") ;
			break ;
		}
    }

    dtime = 0 ;
    (void)edt_dtime() ;
    while(!done)
    {
		if (have_fv)
		{
			/* wait for start of frame valid */
			while ((edt_reg_read(edt_p, CAM_STAT) & CAM_FV) == 0)
				;
			/* wait for end of frame valid */
			while (edt_reg_read(edt_p, CAM_STAT) & CAM_FV)
				;
			framecnt++ ;
		}
		else if (have_lv)
		{
			/* wait for start of line valid */
			while ((edt_reg_read(edt_p, CAM_STAT) & CAM_LV) == 0)
				;
			/* wait for end of line valid */
			while (edt_reg_read(edt_p, CAM_STAT) & CAM_LV)
				;
			linecnt++ ;
		}
		else
		{
			while ((edt_reg_read(edt_p, CAM_STAT) & CAM_PCLK) == 0)
				;
			while ((edt_reg_read(edt_p, CAM_STAT) & CAM_PCLK)) ;
				;
			pixelcnt++ ;
		}

		lsvalue = edt_reg_read(edt_p, CAM_LSPIXELS) ;
		msvalue = edt_reg_read(edt_p, CAM_MSPIXELS) & 0xff ;
		lines = edt_reg_read(edt_p, CAM_HEIGHT) ;
		pixels = edt_reg_read(edt_p, CAM_WIDTH) ;

		new = msvalue << 16 | lsvalue ;
		dtime = edt_dtime() ;
		if (expsize && new != expsize)
		{
			/* trigger logic analyzer */
			edt_reg_write(edt_p, CAM_STAT, 0) ;
			edt_reg_write(edt_p, CAM_CMD, 0) ;
			printf(" BAD frame %d size %d\n",frame,new) ;
			printf("return to continue") ; getchar();
			/* turn on mode code */
			edt_reg_write(edt_p, CAM_STAT, 0) ;
			edt_reg_write(edt_p, CAM_CMD, 0x10) ;
			bad++ ;
		}
		else good++ ;
		frame++ ;
		if (framecnt)
		printf("frame %3d count %3d bad %3d lines/frame %ld pixels/line %ld ",
			framecnt,new,bad,lines,pixels) ;
		if (linecnt)
		printf("line %3d pixels/line %ld ",
			linecnt,pixels) ;
		if (pixelcnt)
		printf("pixel %3d count %3d bad %3d lines/frame %ld pixels/line %ld ",
			pixelcnt,new,bad,lines,pixels) ;
		if (frame > 0) printf("dtime %5.3f\n",dtime) ;
		else printf("\n") ;
#if 0
	{
	    FILE *dbg ;
	    dbg = fopen("checkcam.log","w") ;
	    if (!dbg) printf("can't open checkcam.log\n") ;
	    else
	    {
		fprintf(dbg,"frame %d count %d bad %d lines %d first line %d\n",
			frame,new,bad,lines,pixels) ;
		fclose(dbg) ;
	    }
	}
#endif
    }
}

static void
GetFlagsString(char *s, BitLabels * labels, long flags)

{

    int     i = 0;
    int     started = 0;

    s[0] = 0;

    for (i = 0; labels[i].label != NULL; i++)
    {
	if (flags & labels[i].bit)
	{
	    if (started)
		sprintf(s, " %s", labels[i].label);
	    else
		sprintf(s, "%s", labels[i].label);
	    started = 1;

	    s += strlen(s);

	}
    }
}

static void
PrintRegister(EdtDev * edt_p, char *name, int offset, BitLabels * labels)

{
    char    flags[1024];
    char    format[80];
    int     value;

    int     size = (offset >> 24) & 3;

    if (!size)
	size = 4;

    if (size == 1)
	    printf("%02x     ", offset & 0xff);
	else printf("%02x-%02x  ", offset & 0xff, (offset+size-1) & 0xff);

    size *= 2;

    sprintf(format, "%%-20s: %%0%dx", size);

    value = edt_reg_read(edt_p, offset);

    printf(format, name, value);

    if (labels)
    {
	GetFlagsString(flags, labels, value);
	printf(" %s", flags);
    }

    printf("\n");

}
