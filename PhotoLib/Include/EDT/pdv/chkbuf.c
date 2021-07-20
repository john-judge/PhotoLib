/* #pragma ident "@(#)chkbuf.c	1.21 02/05/08 EDT" */

#include "edtinc.h"

#include "chkbuf.h"

/*
 * dump 64 longs
 */
void
dumpblk(u_char *ptr, int index, int maxindex)

{
	int *p;
	int i;

	/* test index for limits */
	if (index < 0) index = 0;
	if (index + 256 > maxindex) index = maxindex - 256;
	index = index & 0xfffffffc ; /* make an even word boundary */

	p = (int *)(&ptr[index]);
	for(i=0; i<64; i++)
	{
		if ( i % 8 == 0) printf("\n%d -\t", index + (i*4));
		printf("%08x ", *p);
		p++;
	}
	printf("\n");
}

/*
 * check buffersize -1 that each byte increments over the next
 */
int
chkbuf(unsigned char *align, int bufsize, int bufnum, int verbose)

{
	unsigned char shdbe, *p;
	int cnt;
	int err = 0;
	int firsterr = 1;

	p = align;
	shdbe = (*p+1) & 0xff; p++;
	for(cnt=1; cnt<bufsize-1; cnt++)
	{
		if (*p != shdbe)
		{
			err++;
			if (verbose) 
			  {
				if (firsterr)
				  {
					printf("offset %d in buffer %d was %02x, s/b %02x\n",
					   cnt, bufnum, *p, shdbe);
					dumpblk(align, cnt-16, bufsize);
					printf("first error - hit cr to continue"); getchar();
				  }
				firsterr = 0;
			  }
		}
		shdbe = (*p+1) & 0xff; p++;
	}

	return err;
}

int
chkbuf_short(unsigned short *align, int bufsize, int bufnum, int verbose)

{
	unsigned short shdbe;
	int cnt;
	int err = 0;
	int firsterr = 1;

	shdbe = (align[0]+1) & 0xffff; 
	for(cnt=1; cnt<bufsize-1; cnt++)
	{
		if (align[cnt] != shdbe)
		{
			err++;
			if (verbose)
			  {

				if (firsterr)
				  {
					printf("offset %d in buffer %p was %04x, s/b %04x\n",
					   cnt, align, align[cnt], shdbe);
					dumpblk((unsigned char *) align, 
							(cnt-16) * 2, bufsize * 2);
					printf("first error - hit cr to continue"); getchar();
				  }
				firsterr = 0;
			  }
		}
		
		shdbe = (align[cnt]+1) & 0xffff; 
	}

	return err;
}

int
chkbuf_long(unsigned long *align, int bufsize, int bufnum, int verbose)

{
	unsigned long shdbe;
	int cnt;
	int err = 0;
	int firsterr = 1;

	shdbe = align[0]+1; 
	for(cnt=1; cnt<bufsize-1; cnt++)
	{
		if (align[cnt] != shdbe)
		{
			err++;
			if (verbose)
			  {

				if (firsterr)
				  {
					printf("offset %d in buffer %p was %08lx, s/b %08lx\n",
					   cnt, align, align[cnt], shdbe);
					dumpblk((unsigned char *) align, 
							(cnt-16) * 2, bufsize * 2);
					printf("first error - hit cr to continue"); getchar();
				  }
				firsterr = 0;
			  }
		}
		
		shdbe = align[cnt]+1; 
	}

	return err;
}

/*
 * find the bit alignment in a byte for incrementing bytes
 * (look for the bit that flips each time, it is the lsb)
 */
int
bitoffset(unsigned char *bptr)
{
	int i;
	int bit;
	unsigned char *p;
	unsigned char mask, last;
	int maxbit;
	int trans;
	int maxtrans;

	mask = 0x1;
	maxbit = 0;
	maxtrans = 0;
	for(bit=0; bit< 8; bit++)
	{
		trans = 0;
		p = bptr;
		/*
		 * look at the first 9 bytes 
		 * if bufsize is less than 8 to bad
		 */
		last = *p; p++;
		for(i=0; i< 256; i++)
		{
			if ( ((*p ^ last) & mask) != 0)
			{
				trans++;
			}
			last = *p; p++;
		}
#if 0
	printf("bit %d has %d transitions of mask %02x\n", bit, trans, mask);
#endif
		if (trans > maxtrans) 
		{
			maxbit = bit;
			maxtrans = trans;
		}
		mask = (mask << 1);
	}
	return(maxbit);
}

int
bitoffset32(unsigned int *bptr)
{
	int i;
	int bit;
	unsigned int *p;
	unsigned int mask, last;
	int maxbit;
	int trans;
	int maxtrans;

	mask = 0x1;
	maxbit = 0;
	maxtrans = 0;
	for(bit=0; bit< 32; bit++)
	{
		trans = 0;
		p = bptr;
		/*
		 * look at the first 9 bytes 
		 * if bufsize is less than 8 to bad
		 */
		last = *p; p++;
		for(i=0; i< 128*1024/4; i++)
		{
			if ( ((*p ^ last) & mask) != 0)
			{
				trans++;
			}
			last = *p; p++;
		}
#if 0
	printf("bit %d has %d transitions of mask %08x\n", bit, trans, mask);
#endif
		if (trans > maxtrans) 
		{
			maxbit = bit;
			maxtrans = trans;
		}
		mask = (mask << 1);
	}
	return(maxbit);
}

int
bitoffset64(unsigned int *bptr)
{
	int i;
	int bit;
	u_longlong_t *p;
	u_longlong_t mask, last;
	int maxbit;
	int trans;
	int maxtrans;

	mask = 0x1;
	maxbit = 0;
	maxtrans = 0;
	for(bit=0; bit< 64; bit++)
	{
		trans = 0;
		p = (u_longlong_t *) bptr;
		/*
		 * look at the first 9 bytes 
		 * if bufsize is less than 8 to bad
		 */
		last = *p; p++;
		for(i=0; i< 128*1024/8; i++)
		{
			if ( ((*p ^ last) & mask) != 0)
			{
				trans++;
			}
			last = *p; p++;
		}
#if 0
#ifdef _MSC_VER
	printf("bit %d has %d transitions of mask %I64x\n", bit, trans, mask);
#else
	printf("bit %d has %d transitions of mask %016llx\n", bit, trans, mask);
#endif
#endif
		if (trans > maxtrans) 
		{
			maxbit = bit;
			maxtrans = trans;
		}
		mask = (mask << 1);
	}
	return(maxbit);
}
		
/*
 * copy a buffer andshift a by n bits to the left
 * result buffer is one byte less
 * n is assumed to be less than 8
 */
void
cp_shift(unsigned char *from, unsigned char*to,
        int shift, int blen)

{
	unsigned short reg, tmp, mask;
	int i;

	/*
	printf("cp_shift from %08x to %08x shift %d length %d\n",
		from, to, shift, blen);
	*/
	mask = 0xff << shift;
	reg = ( *from << 8); from++;
	/*
	printf("mask = %04x, start reg = %04x\n", mask, reg);
	*/
	for(i=0; i< blen - 1; i++)
	{
		reg |= *from; from++;
		tmp = reg & mask;
		*to = (unsigned char)(tmp >> shift); to++;
		reg = reg<<8;
	}
}

void
cp_shift32(unsigned int *from,unsigned int *to,int shift, int blen)
{



	u_longlong_t reg, tmp, mask;
	int i;
	u_longlong_t tmp2 ;

#if 0
	printf("cp_shift32 from %08x to %08x shift %d length %d\n",
		from, to, shift, blen);
#endif
	mask = (u_longlong_t)0xffffffff << shift;
	tmp2 = (u_longlong_t)*from ;
	reg = ( tmp2 << 32); from++;
	/* printf("mask = %016llx, start reg = %016llx\n", mask, reg);*/
	for(i=0; i< blen - 1; i++)
	{
		reg |= (u_longlong_t)*from; from++;
		tmp = reg & mask;
		tmp2 = tmp >> shift ;
		*to = (unsigned int)(tmp >> shift); 
		to++;
		reg = reg<<32;
	}
}

void
cp_shift64(u_longlong_t *from, u_longlong_t *to, int shift, int blen)
{
    int i, maskshift ;
    u_longlong_t upper, lower, mask ;
    

    upper = 0 ;
    lower = 0 ;
    mask = ((u_longlong_t) 1 << (u_longlong_t) shift) - 1 ;
    maskshift = 64 - shift ;

    for (i = 0; i < blen; i++, to++, from++)
    {
	*to = upper | (*from >> shift) ;
	upper = (*from & mask) << maskshift ;

	if ((u_longlong_t)*to & 
#ifdef _MSC_VER
	  (u_longlong_t)0xeeeeeeeeeeeeeeeei64) 
	  printf("Bad value:  %016I64x %016I64x\n", *from, *to) ;
#else
	  (u_longlong_t)0xeeeeeeeeeeeeeeeeLL) 
	   printf("Bad value:  %016llx %016llx\n", *from, *to) ;
#endif
    }
}
void
intswap(u_char *buf, int size)
{
    u_longlong_t *tmpp ;
    u_longlong_t *endp ;
    tmpp = (u_longlong_t *)buf ;
    endp = tmpp + (size / 8) ;
    while (tmpp < endp)
    {
	*tmpp = iswap(*tmpp) ;
	tmpp++ ;
    }
}
u_longlong_t
iswap(u_longlong_t val)
{
#ifdef _MSC_VER
    return (
	    ((val & 0x00000000ffffffffi64) << 32)
	    | ((val & 0xffffffff00000000i64) >> 32));
#else
    return (
	    ((val & 0x00000000ffffffffLL) << 32)
	    | ((val & 0xffffffff00000000LL) >> 32));

#endif
}
