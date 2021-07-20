#include "edtinc.h"

EdtDev *edt_p ;
int loopcount =	100 ;
int testspeed(EdtDev *edt_p, int unit) ;

int main(int argc, char **argv)
{
    int				unit = 0;
	int				docount	= 0 ;

    --argc;
    ++argv;
    while (argc	&& argv[0][0] == '-')
    {
		switch (argv[0][1])
		{
		  case 'u':
			++argv;
			--argc;
			if (argc < 1) 
            {
		        puts("Error: option 'u' requires a numeric argument");
                puts("Usage: speedtest [-u unit_no] [-l loopcount]") ;
                exit(1);
	        }
	        if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
            {
                unit = atoi(argv[0]);
            }
            else 
            {
		        puts("Error: option 'u' requires a numeric argument\n");
                puts("Usage: speedtest [-u unit_no] [-l loopcount]") ;
                exit(1);
	        }
	        break;

		  case 'l':
			++argv;
			--argc;
			if (argc < 1) 
            {
		        puts("Error: option 'l' requires a numeric argument");
                puts("Usage: speedtest [-u unit_no] [-l loopcount]") ;
                exit(1);
	        }
	        if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
            {
                loopcount =	atoi(argv[0]);
            }
            else 
            {
		        puts("Error: option 'l' requires a numeric argument\n");
                puts("Usage: speedtest [-u unit_no] [-l loopcount]") ;
                exit(1);
	        }
	        break;

		  default:
			puts("Usage: speedtest [-u unit_no] [-l loopcount]") ;
			exit(0);
		}
		argc--;
		argv++;
    }

    if ((edt_p = edt_open(EDT_INTERFACE, unit)) == NULL)
    {
		char str[64] ;

		sprintf(str, "edt_open(%s%d)", EDT_INTERFACE, unit) ;
		edt_perror(str) ;
		return(2) ;
    }

    testspeed(edt_p, unit) ;
	edt_close(edt_p) ;

	puts("Hit return to exit") ;
	getchar() ;
    return(0) ;
}


#include <sys/timeb.h>


int
testspeed(EdtDev *edt_p, int unit)
{
    int	size ;
    int	numbufs	;
	int loop ;
	double dtime ;

    /*
     * set defaults
     */
    /* default to 100 reads of 1 Megabyte */
    size = 1024*1024 ;
    numbufs = 2	;

	/*  Construct a ring buffer */
	if (edt_configure_ring_buffers(edt_p, size, numbufs, EDT_READ, 0) == -1)
	{
		edt_perror("edt_configure_ring_buffers") ;
		return(0) ;
	}

    printf("reading %d buffers of %d bytes from	unit %d	with %d	bufs\n", loopcount, size, unit,	numbufs) ;
	fputs("return to start:	", stdout) ;
	fflush(stdout) ;
	getchar() ;

	/*  queue the first reads */
	edt_start_buffers(edt_p, numbufs) ;

	(void)edt_dtime();

    for(loop = 0 ; loop	< loopcount ; loop++)
    {
		edt_wait_for_buffers(edt_p, 1) ;
		putchar('.') ;
		fflush(stdout) ;

		if (loop < loopcount - numbufs)
			edt_start_buffers(edt_p, 1) ;
	}

	dtime = edt_dtime() ;
	printf("%f bytes/sec\n", (double)(loopcount * size) / dtime) ;
	return 0;
}
