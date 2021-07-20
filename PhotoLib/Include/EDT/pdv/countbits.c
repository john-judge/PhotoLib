#include "edtinc.h"

/* countbit.c: looks at transitions between data bytes, allowing
 * user to determine if there is a stuck bit, loose connection,
 * or some other type of hardware problem in the data aquisition
 * pipeline. */

u_int bitcount[32] ;
u_int ibuf[40960] ;
u_short wbuf[40960] ;
u_char cbuf[40960] ;
u_char rgbbuf[40960*3] ;
u_int bytecount ;

/* prototypes */
void countbits8(u_char *buf,int count, u_char *l8) ;
void countbits16(u_short *buf,int count, u_short *l16) ;
void countbits24(u_char *buf,int count, u_char *lb24, u_char *lg24, u_char *lr24) ;
void countbits32(u_int *buf,int count, u_int *l32);
void usage();
		

int main(int argc, char **argv)
{
    int ret ;
    int i ;
    int do_16 = 0 ;
    int do_24 = 0 ;
    int do_32 = 0 ;
    int outtofile = 0;
    u_char l8=0;
    u_short l16=0;
    u_int l32=0;
    u_char lb24=0, lg24=0, lr24=0;
    FILE *infile ;
    FILE *ofp = stdout;
    char *filename ;
    char ofname[256];

    ofname[0] = '\0';
    
    --argc;
    ++argv;
    while (argc	&& argv[0][0] == '-')
    {
	switch (argv[0][1])
	{
	  case 'w': /* word */
		do_16 = 1 ;
		break ;
	  case 'c': /* color */
		do_24 = 1 ;
		break ;
	  case 'i': /* ?? */
		do_32 = 1 ;
		break ;
	  case 'o': /* output file */
	        outtofile = 1;
		if ((argc > 2) && (argv[1][0] != '-'))
		{
		    --argc;
		    ++argv;
		    strcpy(ofname, argv[0]);
		    if ((ofp = fopen(ofname, "w")) == NULL)
		    {
			perror(ofname);
			exit(1);
		    }
		}
		break ;
	  case '-': /* -- long option */
		if (strcmp(argv[0], "--help") == 0) {
			usage();
			exit(0);
		} else {
			fprintf(stdout, "unknown option: %s\n", argv[0]);
			usage();
			exit(1);
		}
		break;

	  default:
		fprintf(stderr, "unknown flag -'%c'\n", argv[0][1]);
		usage();
		exit(1);
	}
	argc--;
	argv++;
    }

    if (argc < 1)
    {
	    usage();
	    exit(1);
    }
    filename = argv[0] ;
    if ((infile = fopen(filename, "rb")) == NULL)
    {
	    printf("couldn't open %s\n",filename)  ; 
	    exit(1) ;
    }
    if (do_32)
    {
	    while(ret = fread(ibuf,4,40960,infile))
	    {
		    countbits32(ibuf,ret, &l32) ;
		    bytecount += ret * 4 ;
		    if (!outtofile)
		    {
			fprintf(ofp,"%d\r",bytecount) ;
			fflush(ofp) ;
		    }
	    }
	    fprintf(ofp,"\n") ;
	    for(i = 0 ; i < 32 ; i++)
	    {
		    fprintf(ofp,"bit %02d: %d\n",i,bitcount[i]) ;
	    }
    }
    else if (do_24)
    {
	    while(ret = fread(rgbbuf,3,40960,infile))
	    {
		    countbits24(rgbbuf,ret, &lb24, &lg24, &lr24) ;
		    bytecount += ret * 3 ;
		    if (!outtofile)
		    {
			fprintf(ofp,"%d\r",bytecount) ;
			fflush(ofp) ;
		    }
	    }
	    fprintf(ofp,"\n") ;
	    fprintf(ofp,"Blue:\n") ;
	    for(i = 0 ; i < 8 ; i++)
	    {
		    fprintf(ofp,"bit %02d: %d\n",i,bitcount[i]) ;
	    }
	    fprintf(ofp,"Green:\n") ;
	    for(i = 0 ; i < 8 ; i++)
	    {
		    fprintf(ofp,"bit %02d: %d\n",i,bitcount[8+i]) ;
	    }
	    fprintf(ofp,"Red:\n") ;
	    for(i = 0 ; i < 8 ; i++)
	    {
		    fprintf(ofp,"bit %02d: %d\n",i,bitcount[16+i]) ;
	    }
    }
    else if (do_16)
    {
	    while(ret = fread(wbuf,2,40960,infile))
	    {
		    countbits16(wbuf,ret, &l16) ;
		    bytecount += ret * 2 ;
		    if (!outtofile)
		    {
			fprintf(ofp,"%d\r",bytecount) ;
			fflush(ofp) ;
		    }
	    }
	    fprintf(ofp,"\n") ;
	    for(i = 0 ; i < 16 ; i++)
	    {
		    fprintf(ofp,"bit %02d: %d\n",i,bitcount[i]) ;
	    }
    }
    else
    {
	    while(ret = fread(cbuf,1,40960,infile))
	    {
		    countbits8(cbuf,ret,&l8) ;
		    bytecount += ret ;
		    if (!outtofile)
		    {
			fprintf(ofp,"%d\r",bytecount) ;
			fflush(ofp) ;
		    }
	    }
	    fprintf(ofp,"\n") ;
	    for(i = 0 ; i < 8 ; i++)
	    {
		    fprintf(ofp,"bit %02d: %d\n",i,bitcount[i]) ;
	    }
    }
    fclose(infile) ;
    return(0) ;
}



void usage()
{
    puts("Usage: countbits [-w] [-c] [-i] filename");
    puts("-w        interpret data as 2-byte words");
    puts("-c        interpret data as color data");
    puts("-i        interpret data as 4-byte integers");
    puts("-o [file] format for output to file and optionally specify the filename");
    puts("--help    this help message\n");
    puts("");
    return;
}



void
countbits16(u_short *buf,int count, u_short *last)
{
    register u_short *ptr, *end ;
    register int i ;
    register u_short cur ;
    ptr = buf ;
    end = buf + count ;
    for(ptr = buf ; ptr < end ; ptr++)
    {
	cur = *ptr ;
	/* printf("0 %d\n",cur & 0x3ff) ;*/
	for(i = 0 ; i < 16 ; i++)
	{
	    if((cur & (1 << i)) != (*last & (1 << i)))
	    {
		bitcount[i]++ ;
	    }
	}
	*last = cur ;
    }
}



void
countbits8(u_char *buf,int count, u_char *last)
{
    register u_char *ptr, *end ;
    register int i ;
    register u_char cur ;
    ptr = buf ;
    end = buf + count ;
    for(ptr = buf ; ptr < end ; ptr++)
    {
	cur = *ptr ;
	/* printf("0 %d\n",cur & 0x3ff) ;*/
	for(i = 0 ; i < 8 ; i++)
	{
	    if((cur & (1 << i)) != (*last & (1 << i)))
	    {
		bitcount[i]++ ;
	    }
	}
	*last = cur ;
    }
}



void
countbits24(u_char *buf,int count, u_char *last_b, u_char *last_g, u_char *last_r)
{
    register u_char *ptr, *end ;
    register int i ;
    register u_char cur_b ;
    register u_char cur_g ;
    register u_char cur_r ;
    ptr = buf ;
    end = buf + (count * 3) ;
    for(ptr = buf ; ptr < end ; ptr += 3)
    {
	cur_b = ptr[0] ;
	cur_g = ptr[1] ;
	cur_r = ptr[2] ;
	for(i = 0 ; i < 8 ; i++)
	{
	    if((cur_b & (1 << i)) != (*last_b & (1 << i)))
	    {
		bitcount[i]++ ;
	    }
	    if((cur_g & (1 << i)) != (*last_g & (1 << i)))
	    {
		bitcount[8+i]++ ;
	    }
	    if((cur_r & (1 << i)) != (*last_r & (1 << i)))
	    {
		bitcount[16+i]++ ;
	    }
	}
	*last_b = cur_b ;
	*last_g = cur_g ;
	*last_r = cur_r ;
    }
}



void
countbits32(u_int *buf,int count, u_int *last)
{
    register u_int *ptr, *end ;
    register int i ;
    register u_int cur ;
    ptr = buf ;
    end = buf + count ;
    for(ptr = buf ; ptr < end ; ptr++)
    {
	cur = *ptr ;
	for(i = 0 ; i < 32 ; i++)
	{
	    if((cur & (1 << i)) != (*last & (1 << i)))
	    {
		bitcount[i]++ ;
	    }
	}
	*last = cur ;
    }
}
