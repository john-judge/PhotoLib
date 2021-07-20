
#include "edtinc.h"

/*
* change 1024x1024 8 bit with row 0 row0:	BGBGBGBG row1:	GRGRGRGR to
* 24 bit BGR,BGR first pass 512/512
*/

/* shorthand debug level */

#define PDVWARN PDVLIB_MSG_WARNING
#define PDVFATAL PDVLIB_MSG_FATAL
#define DBG1 PDVLIB_MSG_INFO_1
#define DBG2 PDVLIB_MSG_INFO_2


int
exp2_image(u_char * src, int width, int rows, u_char * dest)
{
    u_char *cur_p;
    u_char *nxt_p;
    u_char *lst_p;
    u_char *dest_p = dest;
    int     x, y;

    edt_msg(DBG2, "exp2_image\n");

    if (width == 512)
    {
	cur_p = src;
	nxt_p = src + 1024;

	for (y = 0; y < rows; y++)
	{
	    for (x = 0; x < width; x++)
	    {

		*dest_p++ = *cur_p;	/* blue */
		/* use average of two greens */
		*dest_p++ = *(cur_p + 1) + (*(nxt_p) >> 1);
		*dest_p++ = *(nxt_p + 1);	/* red */
		cur_p += 2;
		nxt_p += 2;
	    }
	    cur_p += 1024;
	    nxt_p += 1024;
	}
    }
    else
    {
	cur_p = src;
	lst_p = src;
	nxt_p = src + width;
	for (y = 0; y < rows - 2; y += 2)
	{
	    for (x = 0; x < width; x += 2)
	    {

		*dest_p++ = *cur_p;
		*dest_p++ = *nxt_p;	/* green from below */
		*dest_p++ = *(nxt_p + 1);
		*dest_p++ = *cur_p;
		*dest_p++ = *(cur_p + 1);	/* green from cur row */
		*dest_p++ = *(nxt_p + 1);
		cur_p += 2;
		nxt_p += 2;
	    }

	    for (x = 0; x < width; x += 2)
	    {

		*dest_p++ = *lst_p;	/* blue from above */
		*dest_p++ = *cur_p;
		*dest_p++ = *(cur_p + 1);
		*dest_p++ = *nxt_p;	/* blue from below */
		*dest_p++ = *(nxt_p + 1);	/* green from below */
		*dest_p++ = *(cur_p + 1);
		cur_p += 2;
		nxt_p += 2;
		lst_p += 2;
	    }
	    lst_p += width;
	}
    }
    return (0);
}

/*
* change 1024x1024 8 bit dual with BGGRBGGRBGGR to 24 bit BGR,BGR first pass
* 512/512
*/
int
exp2dual_image(u_char * src, int width, int rows, u_char * dest)
{
    u_char *cur_p;
    u_char *nxt_p;
    u_char *lst_p;
    u_char *dest_p = dest;
    int     x, y;

    edt_msg(DBG2, "exp2dual_image\n");


    if (width == 512)
    {
	cur_p = src;

	for (y = 0; y < rows; y++)
	{
	    for (x = 0; x < width; x++)
	    {

		*dest_p++ = cur_p[0];	/* blue */
		/* use average of two greens */
		*dest_p++ = (cur_p[1] + cur_p[2]) >> 1;
		*dest_p++ = cur_p[3];	/* red */
		cur_p += 4;
	    }
	}
    }
    else
    {
	cur_p = src;
	lst_p = src;
	nxt_p = src + 1024;
	for (y = 0; y < rows - 2; y += 2)
	{
	    for (x = 0; x < width; x += 2)
	    {

		*dest_p++ = *cur_p;
		*dest_p++ = *nxt_p;	/* green from below */
		*dest_p++ = *(nxt_p + 1);
		*dest_p++ = *cur_p;
		*dest_p++ = *(cur_p + 1);	/* green from cur row */
		*dest_p++ = *(nxt_p + 1);
		cur_p += 2;
		nxt_p += 2;
	    }

	    for (x = 0; x < width; x += 2)
	    {

		*dest_p++ = *lst_p;	/* blue from above */
		*dest_p++ = *cur_p;
		*dest_p++ = *(cur_p + 1);
		*dest_p++ = *nxt_p;	/* blue from below */
		*dest_p++ = *(nxt_p + 1);	/* green from below */
		*dest_p++ = *(cur_p + 1);
		cur_p += 2;
		nxt_p += 2;
		lst_p += 2;
	    }
	    lst_p += 1024;
	}
    }
    return (0);
}

/*
* fill in first half of image with value, then two smaller with value - 1
* and value + 1 ;
*/
static int tog = 0;
void
do_data8(u_char * buf, int width, int height, int scansize, int left, int right)
{

    u_char *tmpp;
    u_char *p;
    int     x;
    int     y;
    int     delta;
    int     midpoint;
    u_char *p0;
    u_char *p1;

    /* edt_msg(DBG2, "do_data8\n");*/

    if (width == 0 || left == right - 1)
	return;
    midpoint = (left + right) / 2;
    delta = (right - left) / 2;

    /* fill in top half */
    p = buf;
    for (y = 0; y < height; y++)
    {
	tmpp = p;
	for (x = 0; x < width; x++)
	    tmpp[x] = midpoint;
	p += scansize;
    }

    if (tog)
    {
	p0 = buf + (scansize * height);
	p1 = p0 + width / 2;
    }
    else
    {
	p1 = buf + (scansize * height);
	p0 = p1 + width / 2;
    }


    do_data8(p0, width / 2, height, scansize, left, midpoint);
    do_data8(p1, width / 2, height, scansize, midpoint, right);
}

void
do_data16(u_short * buf, int width, int height, int scansize, int left, int right)
{

    u_short *tmpp;
    u_short *p;
    int     x;
    int     y;
    int     delta;
    int     midpoint;
    u_short *p0;
    u_short *p1;

    if (width == 0 || left == right - 1)
	return;
    midpoint = (left + right) / 2;
    delta = (right - left) / 2;
    /* fill in top half */
    p = buf;

    /* edt_msg(DBG2, "do_data16\n"); */

    for (y = 0; y < height; y++)
    {
	tmpp = p;
	for (x = 0; x < width; x++)
	    tmpp[x] = midpoint;
	p += scansize;
    }

    if (tog)
    {
	p0 = buf + (scansize * height);
	p1 = p0 + width / 2;
    }
    else
    {
	p1 = buf + (scansize * height);
	p0 = p1 + width / 2;
    }

    do_data16(p0, width / 2, height, scansize, left, midpoint);
    do_data16(p1, width / 2, height, scansize, midpoint, right);
}

/**
 * Fills buffer buf with software generated data. 
 * @param buf Dummy device image buffer.
 * @param width number of bytes per line
 * @param height number of lines within image buffer.
 * @param depth number of bits per pixel
 */
void
pdv_dmy_data(void *buf, int width, int height, int depth)
{
    edt_msg(DBG2, "dmy_data\n");
    if (depth == 8)
    {
	do_data8(buf, width, height / depth, width, 0, 1 << depth);
	tog ^= 1;
    }
    else if (depth > 8 && depth <= 16)
    {
	/* fill in interesting data for histogram */
	do_data16(buf, width, height / depth, width, 0, 1 << depth);
	tog ^= 1;
    }
    else if (depth == 24)
    {
	int     x;
	int     y;
	int     v;
	int     row;
	int     col;
	static int cnt = 0;
	char   *tmpp = buf;
	char *oldcolor = getenv("OLDCOLOR") ;

	if (oldcolor)
	{
	    for (y = 0; y < height; y++)
	    {
		for (x = 0; x < width; x++)
		{
		    switch (cnt)
		    {
		    case 0:
			*tmpp++ = x;
			*tmpp++ = y;
			*tmpp++ = (x + y) / 2;
			break;
		    case 1:
			*tmpp++ = (x + y) / 2;
			*tmpp++ = x;
			*tmpp++ = y;
			break;
		    case 2:
			*tmpp++ = y;
			*tmpp++ = (x + y) / 2;
			*tmpp++ = x;
			break;
		    }
		}
	    }
	}
	else
	{
	    for (y = 0; y < height; y++)
	    {
		row = y >> 6;

		for (x = 0; x < width; x++)
		{
		    col = x >> 6;

		    v = ((row + col) % 3);


		    switch (cnt)
		    {
		    case 0:
			switch (v)
			{
			case 0:
			    *tmpp++ = (char) 0xc0;
			    *tmpp++ = (char) 0x40;
			    *tmpp++ = (char) 0x40;
			    break;

			case 1:
			    *tmpp++ = (char) 0x40;
			    *tmpp++ = (char) 0xc0;
			    *tmpp++ = (char) 0x40;
			    break;

			case 2:
			    *tmpp++ = (char) 0x40;
			    *tmpp++ = (char) 0x40;
			    *tmpp++ = (char) 0xc0;
			    break;
			}
			break;
		    case 1:
			switch (v)
			{
			case 1:
			    *tmpp++ = (char) 0xc0;
			    *tmpp++ = (char) 0x40;
			    *tmpp++ = (char) 0x40;
			    break;

			case 2:
			    *tmpp++ = (char) 0x40;
			    *tmpp++ = (char) 0xc0;
			    *tmpp++ = (char) 0x40;
			    break;

			case 0:
			    *tmpp++ = (char) 0x40;
			    *tmpp++ = (char) 0x40;
			    *tmpp++ = (char) 0xc0;
			    break;
			}
			break;
		    case 2:
			switch (v)
			{
			case 2:
			    *tmpp++ = (char) 0xc0;
			    *tmpp++ = (char) 0x40;
			    *tmpp++ = (char) 0x40;
			    break;

			case 0:
			    *tmpp++ = (char) 0x40;
			    *tmpp++ = (char) 0xc0;
			    *tmpp++ = (char) 0x40;
			    break;

			case 1:
			    *tmpp++ = (char) 0x40;
			    *tmpp++ = (char) 0x40;
			    *tmpp++ = (char) 0xc0;
			    break;
			}
			break;
		    }
		}
	    }
	}
	if (++cnt == 3)
	    cnt = 0;
    }
    else if (depth == 32)
    {
	int     x;
	int     y;
	static int cnt = 0;
	char   *tmpp = buf;

	for (y = 0; y < height; y++)
	{
	    for (x = 0; x < width; x++)
	    {
		switch (cnt)
		{
		case 0:
		    *tmpp++ = x;
		    *tmpp++ = 0;
		    *tmpp++ = 0;
		    *tmpp++ = 0;
		    break;
		case 1:
		    *tmpp++ = 0;
		    *tmpp++ = x;
		    *tmpp++ = 0;
		    *tmpp++ = 0;
		    break;
		case 2:
		    *tmpp++ = 0;
		    *tmpp++ = 0;
		    *tmpp++ = x;
		    *tmpp++ = 0;
		    break;
		}
	    }
	}
	if (++cnt == 3)
	    cnt = 0;
    }
}
