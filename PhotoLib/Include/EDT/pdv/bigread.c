#include "edtinc.h"

EdtDev *edt_p ;

main(
int argc,
char **argv)
{
	unsigned char *buf_p, *bufs[50] ;
	unsigned short first, last ;
	int bufsize = 1228800 ;
	int i, count = 5 ;

    if ((edt_p = edt_open("pcd", 0)) == NULL)
    {
		perror("scd") ;
		exit(1) ;
    }

	if ((buf_p = (unsigned char *) malloc(bufsize*50)) == NULL)
	{
		edt_perror("malloc") ;
		return(1) ;
	}

	for (i = 0; i < 50; i++)
		bufs[i] = buf_p + (bufsize*i) ;

    edt_configure_ring_buffers(edt_p, bufsize, 50, EDT_READ, bufs) ;

	for(;;)
	{

		edt_start_buffers(edt_p, 30) ;

		for (i = 0; i < 30; i++)
		{
			while (!(buf_p = edt_check_for_buffers(edt_p, 1)))
				;
			printf("%d done:  buf_p 0x%p ", i, buf_p) ;
			first = buf_p[0] | (buf_p[1] << 8) ;
			last = buf_p[bufsize - 2] |	(buf_p[bufsize - 1] << 8) ;
			printf("first 0x%x last 0x%x\n", first, last) ;

		}
		puts("") ;
		puts("") ;

		puts("Hit return to loop") ;
		if (getchar() == 'q')
		{
			edt_close(edt_p) ;
			return(0) ;
		}

		edt_set_buffer(edt_p, count += 5) ;
	}
	return 0 ;
}
