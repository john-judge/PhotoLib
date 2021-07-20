/* #pragma ident "@(#)dumpit.c	1.27 05/21/09 EDT" */

#if 0
#ifdef __sun
#include <sys/types.h>
#include <stdio.h>
#else
/* #include "edtinc.h"*/
#define _NT_ 1
#include <stdio.h>
#endif
#endif
#include "edtinc.h"
#include <stdlib.h>

#ifdef _NT_
typedef unsigned short u_short ;
typedef unsigned char u_char ;
typedef unsigned long u_long ;
typedef unsigned int u_int ;
typedef __int64 longlong_t;

#endif
#if defined (sgi)
typedef __uint64_t longlong_t;
#endif

#if defined(__linux__) || defined(__APPLE__)
typedef long long longlong_t ;
#endif

void display(u_char *data, u_int type, u_int offset) ;
void genbuf(u_char *buf, u_int type, int size) ;
static void byteswap(u_char *buf, int size) ;
static void shortswap(u_char *buf, int size) ;
static void intswap(u_char *buf, int size) ;
static void endswap(u_char *buf, u_int type, int size) ;
static void unpackrgb(u_char *buf, int size) ;



int
little_endian()
{
    u_short test;
    u_char *byte_p;

    byte_p = (u_char *)	& test;
    *byte_p++ =	0x11;
    *byte_p = 0x22;
    if (test ==	0x1122)
	return (0);
    else
	return (1);
}


#define BYTES	0
#define SHORTS	1
#define INTS	2
#define LONGS	3
int display_bits = 0 ;
int nrows = 20 ;
u_char *rawbuf ;

u_int
wswap(u_int val)
{
    return (
	    ((val & 0x0000ffff) << 16)
	    | ((val & 0xffff0000) >> 16));
}

static longlong_t
iswap(longlong_t val)
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


u_int
atoh(char *p)
{
    u_int val = 0 ;
    int     i;

    for (i = 0; i < 8; i++)
    {
	if (*p)
	val <<= 4;
	if (*p >= '0' && *p <= '9')
	    val |= (*p - '0');
	else if (*p >= 'A' && *p <= 'F')
	    val |= (*p - 'A') + 0xa;
	else if (*p >= 'a' && *p <= 'f')
	    val |= (*p - 'a') + 0xa;
	else
	    break;
	++p;
    }
    return (val);
}


void
lshift(u_char *buf, int blen, int shift)
{
    u_short reg, tmp ;
    u_char *from = buf ;
    u_char *to = buf ;
    int skip = 0 ;
    int     i;

    if (shift >= 8)
    {
	skip = shift / 8 ;
	from += skip ;
	shift %= 8 ;
    }
    reg = (*from++ << 8);

    for (i = 0; i < blen - skip; i++)
    {
	reg |= *from++;
	tmp = reg;
	*to++ = (u_char) ((tmp << shift) >> 8) & 0xff;
	reg <<= 8 ;
    }
    for (i = 0 ; i < skip ; i++) *to++ = 0 ;
}


void
rshift(u_char *buf, int blen, int shift)
{
	unsigned short reg, tmp, mask;
	unsigned char *from = buf+blen-1 ;
	unsigned char *to = buf+blen-1 ;
	int skip = 0 ;

	int i;

	if (shift >= 8)
	{
	    skip = shift / 8 ;
	    from -= skip ;
	    shift %= 8 ;
	}

	mask = 0xff << shift;
	reg = *from-- ;
	for(i=0; i< blen - skip; i++)
	{
		reg |= *from-- << 8 ;
		tmp = (reg & mask) ;
		*to-- = (unsigned char)(tmp >> shift);
		reg >>= 8 ;
	}
	for(i = 0 ; i < skip ; i++) *to-- = 0 ;
}


/*
 * find the bit alignment in a byte for incrementing bytes
 * (look for the bit that flips each time, it is the lsb)
 */
static int
bitoffset(u_char *bptr, int size)
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
		for(i=0; i< size; i++)
		{
			if ( ((*p ^ last) & mask) != 0)
			{
				trans++;
			}
			last = *p; p++;
		}
#if 1
	if (trans)
	printf("bit %d: %d mask %02x\n", bit, trans, mask);
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


static int
bitoffset16(u_short *bptr, int size)
{
	int i;
	int bit;
	unsigned short *p;
	unsigned short mask, last;
	int maxbit;
	int trans;
	int maxtrans;

	mask = 0x1;
	maxbit = 0;
	maxtrans = 0;
	for(bit=0; bit< 16; bit++)
	{
		trans = 0;
		p = bptr;
		/*
		 * look at the first 9 bytes 
		 * if bufsize is less than 8 to bad
		 */
		last = *p; p++;
		for(i=0; i< size/2; i++)
		{
			if ( ((*p ^ last) & mask) != 0)
			{
				trans++;
			}
			last = *p; p++;
		}
#if 1
	if (trans)
	printf("bit %d: %d mask %02x\n", bit, trans, mask);
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



static int
bitoffset32(u_int *bptr, int size)
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
		for(i=0; i< size/4; i++)
		{
			if ( ((*p ^ last) & mask) != 0)
			{
				trans++;
			}
			last = *p; p++;
		}
#if 1
	if (trans)
	printf("bit %d: %d mask %08x\n", bit, trans, mask);
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


static int
bitoffset64(longlong_t *bptr, int size)
{
	int i;
	int bit;
	longlong_t *p;
	longlong_t mask, last;
	int maxbit;
	int trans;
	int maxtrans;

	mask = 0x1;
	maxbit = 0;
	maxtrans = 0;
	for(bit=0; bit< 64; bit++)
	{
		trans = 0;
		p = bptr;
		/*
		 * look at the first 9 bytes 
		 * if bufsize is less than 8 to bad
		 */
		last = *p; p++;
		for(i=0; i< size / 8; i++)
		{
			if ( ((*p ^ last) & mask) != 0)
			{
				trans++;
			}
			last = *p; p++;
		}
#if 1
	if (trans)
	{
#ifdef _MSC_VER
	printf("bit %d: %d mask %016I64x\n", bit, trans, mask);
#else
	printf("bit %d: %d mask %016llx\n", bit, trans, mask);
#endif
	}	
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



void bitfreq(u_char *buf, u_int type, u_int size)
{
    int ret ;
    switch(type)
    {
    case BYTES:
	ret = bitoffset(buf,size) ;
	break ;
    case SHORTS:
	ret = bitoffset16((u_short *)buf,size) ;
	break ;
    case INTS:
	ret = bitoffset32((u_int *)buf,size) ;
	break ;
    case LONGS:
	ret = bitoffset64((longlong_t *)buf,size) ;
	break ;
    }
}



void usage()
{
	printf("usage: dumpit [filename]\n") ;
	printf("interactive commands:\n") ;
	printf(".	toggle bit display\n") ;
	printf("b	display bytes\n") ;
	printf("s	display shorts (16 bits)\n") ;
	printf("w	display shorts (16 bits)\n") ;
	printf("i	display ints (32 bits)\n") ;
	printf("l	display longs (64 bits)\n") ;
	printf("o	get original file\n") ;
	printf("f	check bit frequency to find lsb,msb,swaps\n") ;
	printf("g	generate incrementing data\n") ;
	printf("B	byte swap data\n") ;
	printf("S	short or word swap data\n") ;
	printf("E	swap lsb/msb order in bytes\n") ;
	printf(">[n]	shift right n bits (default 1)\n") ;
	printf("<[n]	shift left n bits (default 1)\n") ;
	printf("u	page up\n") ;
	printf("d	page down\n") ;
	printf("-	up one line\n") ;
	printf("<CR>	down one line\n") ;
	printf("h	this help message\n") ;
	printf("+nnnnnnnn	new offset\n") ;
	printf("rnnn	new number of rows\n") ;
	printf("q	quit\n") ;
}


unsigned short table[65536];
void init_table(u_short *table_ptr) ;
int prnchkbuf(u_short *sptr, int length, u_short lastval) ;
#define CHECK_INCREMENT	1
#define CHECK_XEDAR	2
#define CHECK_PRN	3
#define CHECK_STUCK	4 /* check for 2 bits tied together */

int main(int argc, char **argv)
{
    FILE *fdin ;
    int size = 0xff ;
    char ans[80] ;
    u_char *rawbuf ;
    u_char *newbuf ;
    u_char *dispbuf ;
    int shift ;
    int offset = 0 ;
    u_short lastval = 0 ;
    int newoffset ;
    int disp = BYTES ;
    u_int searchval = 0 ;
    int do_search = 0 ;
    int do_check = 0 ;
    int do_check_type = 0 ;
    int prn_table_init = 0 ;
    int sv_offset = 0 ;
    int soff ;
    if (argc > 1)
    {
	if (argv[1][0] == '-') {
		if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
			usage();
			return 0;
		}
	}
	fdin = fopen(argv[1],"rb") ;
	if (fdin == NULL)
	{
	    printf("can't open %s\n",argv[1]) ;
	    exit(1) ;
	}
	fseek(fdin, 0, SEEK_END);
	size = ftell(fdin)  ;
	fseek(fdin, 0, SEEK_SET);
	printf("size %x\n",size);
	rawbuf = (u_char *)malloc(size) ;
	/* make newbuf big enough for expanding rgb */
	newbuf = (u_char *)malloc(size*4/3) ;

	printf("reading %x\n",size) ;
	fread(rawbuf, 1, size, fdin) ;
    }
    else
    {
	printf("type h for usage\n") ;
	rawbuf = (u_char *)malloc(size) ;
	newbuf = (u_char *)malloc(size*4/3) ;
    }
    memcpy(newbuf, rawbuf, size) ;
    dispbuf = newbuf ;
    offset = 0 ;
    sv_offset = offset ;
    ans[0] = 0 ;
    for(;;)
    {
	if (do_search)
	{
	    u_char *bp = (u_char *)(dispbuf + offset) ;
	    u_short *sp = (u_short *)(dispbuf + offset) ;
	    u_int *ip = (u_int *)(dispbuf + offset) ;
	    switch(disp)
	    {
		case BYTES:
		for(soff = offset ; soff < size ; soff++)
		{
		    if (*bp == searchval)
		    {
			offset  = bp - dispbuf ;
			do_search = 0 ;
			break ;
		    }
		    else bp++ ;
		}
		break ;
		case SHORTS:
		for(soff = offset ; soff < size ; soff += 2)
		{
		    if (*sp == searchval)
		    {
			offset  = (u_char *)sp - dispbuf ;
			do_search = 0 ;
			break ;
		    }
		    else sp++ ;
		}
		break ;
		case INTS:
		for(soff = offset ; soff < size ; soff += 4)
		{
		    if (*ip == searchval)
		    {
			offset  = (u_char *)ip - dispbuf ;
			do_search = 0 ;
			break ;
		    }
		    else ip++ ;
		}
		break ;
	    }
	    if (do_search) 
	    {
		printf("\ncouldn't find %x\n",searchval) ;
		do_search = 0 ;
		offset = sv_offset ;
	    }
	}
	if (do_check)
	{
	    u_char *bp = (u_char *)(dispbuf + offset) ;
	    u_short *sp = (u_short *)(dispbuf + offset) ;
	    u_int *ip = (u_int *)(dispbuf + offset) ;
	    u_char bval ;
	    u_short sval ;
	    u_int ival ;
	    switch(disp)
	    {
		case BYTES:
		switch (do_check_type)
		{
		case CHECK_INCREMENT:
		    printf("checking increment\n") ;
		    bval = *bp ;
		    for(soff = offset ; soff < size ; soff += 1)
		    {
			if (*bp != bval)
			{
			    offset  = (u_char *)bp - dispbuf ;
			    do_check = 0 ;
			    break ;
			}
			else bp++ ;
			bval++ ;
		    }
		    break ;
		default:
		    printf("only increment check supported\n") ;
		    break ;
		}
		if (do_check)
		{
		printf("no errors\n") ;
		do_check = 0 ;
		}
		break ;

		case SHORTS:
		switch (do_check_type)
		{
		case CHECK_INCREMENT:
		    printf("checking increment\n") ;
		    sval = *sp ;
		    for(soff = offset ; soff < size ; soff += 2)
		    {
			if (*sp != sval)
			{
			    offset  = (u_char *)sp - dispbuf ;
			    do_check = 0 ;
			    break ;
			}
			else sp++ ;
			sval++ ;
		    }
		    break ;
		case CHECK_XEDAR:
		    printf("checking xedar\n") ;
		    sval = *sp ;
		    for(soff = offset ; soff < size ; soff += 2)
		    {
			if ((*sp  & 0xff) != (sval & 0xff))
			{
			    offset  = (u_char *)sp - dispbuf ;
			    do_check = 0 ;
			    break ;
			}
			else sp++ ;
			if ((*sp & 0xff) != (sval & 0xff))
			{
			    offset  = (u_char *)sp - dispbuf ;
			    do_check = 0 ;
			    break ;
			}
			else sp++ ;
			sval++ ;
			    if (sval == 0x1f5) sval = 1 ;
		    }
		    break ;
		case CHECK_PRN:
		    if (!prn_table_init)
		    {
			init_table(table) ;
			prn_table_init = 1 ;
		    }
		    prnchkbuf(sp, size - offset, lastval) ;
		    lastval = sp[size/2-1] ;
		    break ;
		}
		if (do_check)
		{
		printf("no errors\n") ;
		do_check = 0 ;
		}
		break ;

		case INTS:
		switch (do_check_type)
		{
		case CHECK_INCREMENT:
		    printf("checking increment\n") ;
		    ival = *ip ;
		    for(soff = offset ; soff < size ; soff += 4)
		    {
			if (*ip != ival)
			{
			    offset  = (u_char *)ip - dispbuf ;
			    do_check = 0 ;
			    break ;
			}
			else ip++ ;
			ival++ ;
		    }
		    break ;
		default:
		    printf("only increment check supported\n") ;
		    break ;
		}
		if (do_check)
		{
		printf("no errors\n") ;
		do_check = 0 ;
		}
		break ;
	    }
	}
	if (ans[0] != 'f' && ans[0] != 'h')
	display(dispbuf,disp,offset) ;
	fgets(ans,79,stdin) ;
	switch(ans[0])
	{
	case 'h': usage() ; break ;
	case '.': display_bits ^= 1 ; break ;
	case 'b': disp = BYTES ; break ;
	case 's': disp = SHORTS ; break ;
	case 'w': disp = SHORTS ; break ;
	case 'i': disp = INTS ; break ;
	case 'l': disp = LONGS ; break ;
	case 'o': 
	    offset = 0 ;
	    memcpy(newbuf, rawbuf, size) ;
	    break ;
	case 'r':
	    nrows = atoi(&ans[1]) ;
	    break ;
	case 'f': 
	    bitfreq(newbuf,disp,size) ;
	    break ;
	case 'g': 
	    genbuf(newbuf,disp,size) ;
	    break ;
	case 'B':
	    byteswap(newbuf,size) ;
	    break ;
	case 'E':
	    endswap(newbuf,disp,size) ;
	    break ;
	case 'S':
	    shortswap(newbuf,size) ;
	    break ;
	case 'I':
	    intswap(newbuf,size) ;
	    break ;
	case 'R':
	    unpackrgb(newbuf,size) ;
	    break ;
	case '>':
	    if (ans[1] && ans[1] != '\r' && ans[1] != '\n') shift = atoi(&ans[1]) ;
	    else shift = 1 ;
	    if (little_endian())
	    {
	    switch(disp)
	    {
	    case SHORTS:
		byteswap(newbuf,size) ;
		break ;
	    case INTS:
		byteswap(newbuf,size) ;
		shortswap(newbuf,size) ;
		break ;
	    case LONGS:
		byteswap(newbuf,size) ;
		shortswap(newbuf,size) ;
		intswap(newbuf,size) ;
		break ;
	    }
	    }
	    rshift(newbuf,size,shift) ;
	    if (little_endian())
	    {
	    switch(disp)
	    {
	    case SHORTS:
		byteswap(newbuf,size) ;
		break ;
	    case INTS:
		byteswap(newbuf,size) ;
		shortswap(newbuf,size) ;
		break ;
	    case LONGS:
		byteswap(newbuf,size) ;
		shortswap(newbuf,size) ;
		intswap(newbuf,size) ;
		break ;
	    }
	    }
	    break ;
	case '<':
	    if (ans[1] && ans[1] != '\r' && ans[1] != '\n') shift = atoi(&ans[1]) ;
	    else shift = 1 ;
	    if (little_endian())
	    {
	    switch(disp)
	    {
	    case SHORTS:
		byteswap(newbuf,size) ;
		break ;
	    case INTS:
		byteswap(newbuf,size) ;
		shortswap(newbuf,size) ;
		break ;
	    case LONGS:
		byteswap(newbuf,size) ;
		shortswap(newbuf,size) ;
		intswap(newbuf,size) ;
		break ;
	    }
	    }
	    lshift(newbuf,size,shift) ;
	    if (little_endian())
	    {
	    switch(disp)
	    {
	    case SHORTS:
		byteswap(newbuf,size) ;
		break ;
	    case INTS:
		byteswap(newbuf,size) ;
		shortswap(newbuf,size) ;
		break ;
	    case LONGS:
		byteswap(newbuf,size) ;
		shortswap(newbuf,size) ;
		intswap(newbuf,size) ;
		break ;
	    }
	    }
	    break ;
	case 'u':
	    offset -= nrows * 16 ;
	    if (offset < 0) offset = 0 ;
	    break ;
	case 'd':
	    offset += nrows * 16 ;
	    if (offset < 0) offset = 0 ;
	    break ;
	case '-':
	    offset -= 16 ;
	    if (offset < 0) offset = 0 ;
	    break ;
	case '/':
	    do_search = 1 ;
	    sv_offset = offset ;
	    searchval = atoh(&ans[1]) ;
	    printf("search for %x\n",searchval) ;
	    break ;
	case 'c':
	    do_check = 1 ;
	    do_check_type = CHECK_INCREMENT ;
	    sv_offset = offset ;
	    break ;
	case 'x':
	    do_check = 1 ;
	    do_check_type = CHECK_XEDAR ;
	    sv_offset = offset ;
	    break ;
	case 'p':
	    do_check = 1 ;
	    do_check_type = CHECK_PRN ;
	    sv_offset = offset ;
	    break ;
	case 't':
	    do_check = 1 ;
	    do_check_type = CHECK_STUCK ;
	    sv_offset = offset ;
	    break ;
	case '+':
	    newoffset = atoh(&ans[1]) ;
	    if (newoffset >= 0)
	    {
		if (newoffset > size)
		{
		    printf("offset can't exceed size %x\n",size) ;
		}
		else
		    offset = newoffset ;
	    }
	    break ;
	case 'q':
	    exit(1) ;
	case 0:
	    offset += 16 ;
	    break ;
	default:
	    newoffset = atoh(ans) ;
	    if (newoffset >= 0)
	    {
		if (newoffset > size)
		{
		    printf("offset can't exceed size %x\n",size) ;
		}
		else
		    offset = newoffset ;
	    }
	    else printf("unknown command %c %02x\n",ans[0],ans[0]) ;
	    break ;
	}
    }
    return 0;
}


static void
byteswap(u_char *buf, int size)
{
    swab((char *)buf,(char *)buf,size) ;
}


static void
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


static void
intswap(u_char *buf, int size)
{
    longlong_t *tmpp ;
    longlong_t *endp ;
    tmpp = (longlong_t *)buf ;
    endp = tmpp + (size / 8) ;
    while (tmpp < endp)
    {
	*tmpp = iswap(*tmpp) ;
	tmpp++ ;
    }
}


static void
unpackrgb(u_char *buf, int size)
{
    u_char *tmpp ;
    u_char *from_p ;
    tmpp = buf + ((size * 4) / 3) - 1 ;
    from_p = buf + size - 1 ;
    /* we need to copy from end to do inplace  */
	printf("size %x from %p buf %p tmpp %p\n",
		size, from_p, buf, tmpp) ;
    /* DEBUG */
    while (from_p >= buf)
    {
	if (((unsigned long)tmpp % 4) == 0)
	    *tmpp-- = 0 ;
	*tmpp-- = *from_p-- ;
    }
    *tmpp = 0 ;
    printf("done\n") ;
}


u_char swapbuf[256] ;
int swapinit = 0 ;
u_char
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


static void
endswap(u_char *buf, u_int type, int size)
{
    u_char *tmpp ;
    u_char *endp ;
    int i ;
    if (!swapinit)
    {
	for(i = 0 ; i < 256 ; i++)
	{
	    swapbuf[i] = flipbits((u_char)i) ;
	}
	swapinit = 1 ;
    }

    tmpp = (u_char *)buf ;
    endp = tmpp + size ;
    while (tmpp < endp)
    {
	*tmpp = swapbuf[*tmpp] ;
	tmpp++ ;
    }
    if (type == SHORTS)
    {
	byteswap(buf,size) ;
    }
    else if (type == INTS)
    {
	byteswap(buf,size) ;
	shortswap(buf,size) ;
    }
    else if (type == LONGS)
    {
	byteswap(buf,size) ;
	shortswap(buf,size) ;
	intswap(buf,size) ;
    }
}


void
genbuf(u_char *buf, u_int type, int size)
{
    u_char bval = 0 ;
    u_short sval = 0 ;
    u_int ival = 0 ;
    longlong_t lval = 0 ;
    u_char *bp = (u_char *)buf ;
    u_short *sp = (u_short *)buf ;
    u_int *ip = (u_int *)buf ;
    int i ;
    longlong_t *lp = (longlong_t *)buf ;
    switch (type)
    {
    case BYTES:
	for(i = 0 ; i < size ; i++)
	    *bp++ = bval++ ;
	break ;
    case SHORTS:
	for(i = 0 ; i < size/2 ; i++)
	    *sp++ = sval++ ;
	break ;
    case INTS:
	for(i = 0 ; i < size/4 ; i++)
	    *ip++ = ival++ ;
	break ;
    case LONGS:
	for(i = 0 ; i < size/8 ; i++)
	    *lp++ = lval++ ;
	break ;
    }
}


void
display(u_char *data, u_int type, u_int offset)
{
    u_char *bp = (u_char *)(data + offset) ;
    u_short *sp = (u_short *)(data + offset) ;
    u_int *ip = (u_int *)(data + offset) ;
    longlong_t *lp = (longlong_t *)(data + offset) ;
    int row ;
    int i ;
    int j ;
    u_char bval = 0 ;
    u_short sval = 0 ;
    u_int ival = 0 ;
    longlong_t lval = 0 ;

    for(row = 0 ; row < nrows ; row++)
    {
	printf("%06x: ",offset) ;
	if (!display_bits) switch(type)
	{
	case BYTES:
	    for(i = 0 ; i < 16 ; i++)
	    {
		printf("%02x ",*bp++) ;
	    }
	    printf("\n") ;
	    break ;
	case SHORTS:
	    for(i = 0 ; i < 8 ; i++)
	    {
		printf("%04x ",*sp++) ;
	    }
	    printf("\n") ;
	    break ;
	case INTS:
	    for(i = 0 ; i < 4 ; i++)
	    {
		printf("%08x ",*ip++) ;
	    }
	    printf("\n") ;
	    break ;
	case LONGS:
	    for(i = 0 ; i < 2 ; i++)
	    {
#ifdef _MSC_VER
		printf("%016I64x ",*lp++) ;
#else
		printf("%016llx ",*lp++) ;
#endif
	    }
	    printf("\n") ;
	    break ;
	}
	else
	{
	    /* bit display */
	    switch (type)
	    {
	    case BYTES:
	    for(i = 0 ; i < 16 ; i++)
	    {
		bval = *bp++ ;
		for(j = 7 ; j >= 0 ; j--)
		{
		    if (bval & (1 << j)) printf("1") ;
		    else printf("0") ;
		}
		printf(" ") ;
		if (i == 7) printf("\n%06x: ",offset+8) ;
	    }
	    printf("\n") ;
	    break ;

	    case SHORTS:
	    for(i = 0 ; i < 8 ; i++)
	    {
		sval = *sp++ ;
		for(j = 15 ; j >= 0 ; j--)
		{
		    if (sval & (1 << j)) printf("1") ;
		    else printf("0") ;
		}
		printf(" ") ;
		if (i == 3) printf("\n%06x: ",offset+8) ;
	    }
	    printf("\n") ;
	    break ;

	    case INTS:
	    for(i = 0 ; i < 4 ; i++)
	    {
		ival = *ip++ ;
		for(j = 31 ; j >= 0 ; j--)
		{
		    if (ival & (1 << j)) printf("1") ;
		    else printf("0") ;
		}
		printf(" ") ;
		if (i == 1) printf("\n%06x: ",offset+8) ;
	    }
	    printf("\n") ;
	    break ;

	    case LONGS:
	    for(i = 0 ; i < 2 ; i++)
	    {
		lval = *lp++ ;
		for(j = 63 ; j >= 0 ; j--)
		{
		    if (lval & ((longlong_t)1 << j)) printf("1") ;
		    else printf("0") ;
		}
		printf(" ") ;
		if (i == 0) printf("\n%06x: ",offset+8) ;
	    }
	    printf("\n") ;
	    break ;
	    }
	    if (row == nrows/2) return ;
	}
	offset += 16 ;
    }
}



/*
 * routines for checking prn
 */
/*
 * calculate the next 16 bit number for a prbs15 code
 */

unsigned short calnext( unsigned short in)
{
	unsigned short out = 0;
	unsigned short mask = 0x0001;
	unsigned short a, b;
	int loop;
	
	for (loop=0; loop<16; loop++)
	{
		if (loop<15)
			a = in>>(loop+1) & 0x0001;
		else
			a = out & 0x0001;
		if (loop<14)
			b = in>>(loop+2) & 0x0001;
		else
			b = out>>(loop-14) &0x0001;

		if ((a ^ b) == 1) 
		out |= mask << loop;
	}
	return(out);
}


/*
 * fill out a lookup table for checking a prbs buffer
 * address _> next data -> nextaddress ... etc
 */

/* memory for the 16 bit look up table */
/* zero code is an illegal path */

void init_table(u_short *table_ptr)
{
	int loop;
	unsigned short tmp, nxt;
	
	/* initialize to zero entry */
	for( loop=0; loop<65536; loop++)
	{
		table_ptr[loop] = 0;
	}
	/* foreach address see if it is legal and calculate next*/
 	/* follow next value until we find a filled in value */
	/* zero entry is always illegal */
	for( loop=1; loop<65536; loop++)
	{
		if (table_ptr[loop] == 0) 
		{
			/* check for legal address - bit 0 xor bit1 = bit15 */
			if ( ((loop & 0x0001) ^ (loop>>1 & 0x0001)) 
				== (loop>>15 & 0x0001))
			{
				nxt = loop;
				while(table_ptr[nxt] == 0)
				{
					tmp = calnext(nxt);
					table_ptr[nxt] = tmp;
					nxt = tmp;
				}
			}
		}
	}
}


int prnchkbuf(u_short *sptr, int length, u_short lastval)
{
	unsigned short index;
	int syncflag=1;
	int count=0;
	int error=0;
	int ofs_count=0;

	if (lastval == 0) /* start with first value of this buffer */
	{
		index = *sptr++;
		count++;
		while (index == 0)
		{
			error++;
			index = *sptr++;
			count++;
		}
		if (error != 0)
			printf("%d zero values at buffer start\n", error);
	}
	else
	{
		index = lastval;
	}

	while (count < length)
	{
		if (syncflag == 0)
		{
			if (table[index] == 0)
			{
				index = *sptr++;
				count++;
				error++;
			}
			else
			{
				printf("in sync at %d(%x) after %d words\n",
					count, count, count-ofs_count);
				syncflag = 1;
			}
		}
		else 
		{
			if (table[index] == 0)
			{
				printf("out of sync at %d(%x) index %04x\n",
					count, count, index);
				syncflag = 0;
				index = *sptr++;
				ofs_count = count;
				count++;
				error++;
			}
			else
			{
				if ( *sptr != table[index] )
				{
					if (error < 8 ) 
					{
		printf("error at %d(%x) last %04x was %04x s/b %04x\n",
			count, count, index, *sptr, table[index]);
					}
					error++;
				}
				sptr++;
				count++;
				index = table[index];
			}
		}

	}
	return(error);
}
