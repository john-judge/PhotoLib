#include "edtinc.h"
#include <stdlib.h>


int
main(int argc, char **argv)
{
	EdtDev *edt_p ;
	int retval, i, loop ;
	unsigned char *buf_p ;
	int bufsize = 1024*1024	;
	unsigned short first, last ;
	int timeout = 5000 ;	/* default timeout 5 seconds */

    if (argc > 1)
    {
	    bufsize = atoi(argv[1]);
	    printf("bufsize %d (0x%x)\n",bufsize,bufsize);
	}
    if (argc > 2)
    {
	    timeout = atoi(argv[2]);
	    printf("timeout %d ms\n",timeout) ;
	}

	if ((edt_p = edt_open(EDT_INTERFACE, 0)) == NULL)
	{
		edt_perror(EDT_INTERFACE) ;
		puts("Hit return to exit") ;
		getchar() ;
		return(1) ;
	}
	
	/* only used if have no constant clock on the pcd */
	if (argc > 4)
	{
		u_char cfg ;
		cfg = edt_intfc_read(edt_p, PCD_CONFIG) ;
		cfg |= PCD_EN_IDVDLY ;
		printf("setting cfg %x for idvdly\n",cfg) ;
		edt_intfc_write(edt_p, PCD_CONFIG, cfg) ;
	}
	edt_flush_fifo(edt_p) ;

	printf("setting timeout to %d ms\n",timeout) ;
	
	(void)edt_set_rtimeout(edt_p, timeout) ;

	buf_p = (u_char *)malloc(bufsize) ;

	printf("Hit return to do simple read %d (%f Mbytes) ",
		bufsize,(float)bufsize/(float)(1024*1024)) ; 
	getchar();
	if ((retval = edt_read(edt_p, buf_p, bufsize)) == 0)
	{
		edt_perror("edt_read") ;
		return(0) ;
	}

	printf("edt_read() returns %d\n", retval) ;
	for (i = 0; i < 20; i += 2)
	{
		first = buf_p[i] | (buf_p[i+1] << 8) ;
		printf("0x%x ", first) ;
	}
	printf("\n") ;

	puts("Hit return to continue to ring buffer example") ;
	getchar() ;
	free(buf_p) ;


	/*  Construct a four element ring buffer */
	retval = edt_configure_ring_buffers(edt_p, bufsize, 4, EDT_READ, 0) ;

	if (retval == -1)
	{
		edt_perror("configure ring buffers") ;
		puts("Hit return to continue") ;
		getchar() ;
		return(2) ;
	}

	/*  Initialize part of the zeroth buffer to known values */
	buf_p =	edt_p->ring_buffers[0] ;
	for (i = 0; i <	100; i++)
		buf_p[i] = i ;
	
	/* Start sequential DMA on the four buffers */
	edt_flush_fifo(edt_p) ;
	retval = edt_start_buffers(edt_p, 4) ;

	for (loop = 0; loop < 10; loop++)
	{
	    buf_p = edt_wait_for_buffers(edt_p,	1) ;

	    printf("buf_p %p\n", buf_p) ;
	    first = buf_p[0] | (buf_p[1] << 8) ;
	    last = buf_p[bufsize - 2] |	(buf_p[bufsize - 1] << 8) ;
	    printf("first 0x%x	last 0x%x\n", first, last) ;

		/*  As we finish processing each buffer, schedule new DMA on it */
	    edt_start_buffers(edt_p, 1)	;
	}
	
	edt_disable_ring_buffers(edt_p) ;
	puts("Hit return to continue") ;
	getchar() ;
	return 0 ;
}
