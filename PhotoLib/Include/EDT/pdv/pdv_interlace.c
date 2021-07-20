/**
 * @file
 * PCI DV Library routines to reorder/interpolate image buffers from cameras
 *
 */

#include "edtinc.h"

#include "pdv_interlace_methods.h"

/*********************************************/
/* Bayer Filter interpolation code           */
/*********************************************/

#ifdef USE_MMX

#include "bayer_filter_asm.h"

#endif


#include <math.h>
/* shorthand debug level */

#define PDVWARN PDVLIB_MSG_WARNING
#define PDVFATAL PDVLIB_MSG_FATAL
#define DBG1 PDVLIB_MSG_INFO_1
#define DBG2 PDVLIB_MSG_INFO_2

#define PDV_BAYER_ORDER_BGGR 0
#define PDV_BAYER_ORDER_GBRG 1
#define PDV_BAYER_ORDER_RGGB 2
#define PDV_BAYER_ORDER_GRBG 3

#define PDV_BAYER_ORDER(dd_p) ((((Dependent *) dd_p)->kbs_red_row_first << 1) \
        | ((Dependent *) dd_p)->kbs_green_pixel_first)





    static int
pdv_default_interlace_inplace(int interlace_type)

{
#ifdef USE_MMX
    if (interlace_type == 0 ||
            interlace_type == PDV_BYTE_INTLV ||
            /* interlace_type == PDV_PIRANHA_4CH_INTLV || */
            interlace_type == PDV_INTLV_BGR_2_RGB)
#else
        if (interlace_type == 0)
#endif
        {
            return 1;
        }

    return 0;

}

    int
pdv_process_inplace(PdvDev *pdv_p)
{
    EdtPostProc *pCntl;

    if ((pCntl = (EdtPostProc *) pdv_p->pInterleaver))
    {
        if (pCntl->doinplace)
            return pCntl->doinplace();

    }

    return pdv_default_interlace_inplace(pdv_p->dd_p->swinterlace);
}


    void
pdv_alloc_tmpbuf(PdvDev * pdv_p)

{
    Dependent *dd_p = pdv_p->dd_p;


    if (dd_p->swinterlace)
    {
        int     size = pdv_get_dmasize(pdv_p);

        if (dd_p->imagesize > size)
            size = dd_p->imagesize;

        if (size != (int) pdv_p->tmpbufsize)
        {
            if (pdv_p->tmpbuf)
                edt_free(pdv_p->tmpbuf);

            if (size)
            {
                pdv_p->tmpbuf = edt_alloc(size);
            }
            else
                pdv_p->tmpbuf = NULL;

            pdv_p->tmpbufsize = size;

        }
    }
}



/* byte -> byte */

    int
deIntlv_ES10_8(u_char * src, int width, int rows, u_char * dest)
{



#ifdef USE_MMX

    if (bayer_can_use_mmx())
    {
        edt_msg(DBG2, "Using MMX\n");
        return deIntlv_ES10_8_mmx_inplace(src, width, rows, dest);

    }
    else

#endif
    {
        u_char *even_dst;
        u_char *odd_dst;
        int     x, y;

        edt_msg(DBG2, "ES10deInterLeave()\n");

        even_dst = dest;
        odd_dst = dest + width;

        for (y = 0; y + 1 < rows; y += 2)
        {
            for (x = 0; x < width; x++)
            {
                *even_dst++ = *src++;
                *odd_dst++ = *src++;
            }
            even_dst += width;
            odd_dst += width;
        }
    }
    return (0);
}


/*
 * words are arranged in source as row0pix0, row1,pix0, row0pix1, row1pix1
 */
    int
deIntlv_ES10_16(u_short * src, int width, int rows, u_short * dest)
{
    u_short *even_dst;
    u_short *odd_dst;
    int     x, y;

    edt_msg(DBG2, "ES10_word_deIntlv()\n");

    even_dst = dest;
    odd_dst = dest + width;

    for (y = 0; y < rows; y += 2)
    {
        for (x = 0; x < width; x++)
        {
            *even_dst++ = *src++;
            *odd_dst++ = *src++;
        }
        even_dst += width;
        odd_dst += width;
    }
    return (0);
}

/*
 * same as ES10_word_deIntlv but odd line first
 */
    int
deIntlv_ES10_16_odd(u_short * src, int width, int rows, u_short * dest)
{
    u_short *even_dst;
    u_short *odd_dst;
    int     x, y;

    edt_msg(DBG2, "ES10_word_deIntlv_odd()\n");

    odd_dst = dest;
    even_dst = dest + width;

    for (y = 0; y < rows; y += 2)
    {
        for (x = 0; x < width; x++)
        {
            *even_dst++ = *src++;
            *odd_dst++ = *src++;
        }
        odd_dst += width;
        even_dst += width;
    }
    return (0);
}

/*
 * word deinterleave to two images one above the other
 */
    int
deIntlv_ES10_16_hilo(u_short * src, int width, int rows, u_short * dest)
{
    u_short *even_dst;
    u_short *odd_dst;
    int     x, y;

    edt_msg(DBG2, "ES10_word_deIntlv_hilo()\n");

    even_dst = dest;
    odd_dst = dest + (width * (rows / 2));

    for (y = 0; y < rows; y += 2)
    {
        for (x = 0; x < width; x++)
        {
            *even_dst++ = *src++;
            *odd_dst++ = *src++;
        }
    }
    return (0);
}

/*
 * byte deinterleave top down / bottom up
 */
    int
deIntlv_TopBottom8(u_char * src, int width, int rows, u_char * dest)
{
    u_char *even_dst;
    u_char *odd_dst;
    int     x, y;

    edt_msg(DBG2, "deIntlv_TopBottom8()\n");

    even_dst = dest;
    odd_dst = dest + (width * (rows-1));

    for (y = 0; y < rows; y += 2)
    {
        for (x = 0; x < width; x++)
        {
            *even_dst++ = *src++;
            *odd_dst++ = *src++;
        }
        odd_dst -= width*2;
    }
    return (0);
}

/*
 * word deinterleave top down / bottom up 
 */
    int
deIntlv_TopBottom16(u_short * src, int width, int rows, u_short * dest)
{
    u_short *even_dst;
    u_short *odd_dst;
    int     x, y;

    edt_msg(DBG2, "deIntlv_TopBottom16()\n");

    even_dst = dest;
    odd_dst = dest + (width * (rows-1));

    for (y = 0; y < rows; y += 2)
    {
        for (x = 0; x < width; x++)
        {
            *even_dst++ = *src++;
            *odd_dst++ = *src++;
        }
        odd_dst -= width*2;
    }
    return (0);
}

/*
 * byte deinterleave from the center out horizontally
 */
    int
deIntlv_InOut8(u_char * src, int width, int rows, u_char * dest)
{
    u_char *even_dst;
    u_char *odd_dst;
    int     x, y;

    edt_msg(DBG2, "deIntlv_InOut8()\n");

    for (y = 0; y < rows; y ++)
    {
        odd_dst = dest + (width / 2);
        even_dst = odd_dst - 1;
        for (x = 0; x < width; x++)
        {
            *even_dst-- = *src++;
            *odd_dst++ = *src++;
        }
        dest += width;
    }
    return (0);
}

/*
 * word deinterleave from the center out horizontally
 */
    int
deIntlv_InOut16(u_short * src, int width, int rows, u_short * dest)
{
    u_short *even_dst;
    u_short *odd_dst;
    int     x, y;

    edt_msg(DBG2, "deIntlv_InOut16()\n");

    for (y = 0; y < rows; y ++)
    {
        odd_dst = dest + (width / 2);
        even_dst = odd_dst - 1;
        for (x = 0; x < width; x++)
        {
            *even_dst-- = *src++;
            *odd_dst++ = *src++;
        }
        dest += width;
    }
    return (0);
}


/*
 * piranha/atmel m4 8 bit deinterleave -- lhs side normal, rhs in from left and swapped
 */
    int
deIntlv_piranha_8(u_char *src, int width, int rows, u_char *dest)

{
    int x, y;
    u_char *sx=src;
    u_char *dxl, *dxr;

    edt_msg(DBG2, "deIntlv_piranha_8()\n");

    for (y=0;y<rows;y++)
    {
        dxl = dest + (width * y);
        dxr = (dxl + width) - 1;
        for (x=0;x<width;x+=4)
        {
            *(dxl++)  = *(sx++);
            *(dxl++)  = *(sx++);
            *(dxr-1)  = *(sx++);	/* rhs gets swapped */
            *(dxr)    = *(sx++);
            dxr -= 2;
        }
    }
    return (0);
}

/*
 * 4-pixel groups: P0L0, P0L1, P0L2, P0L3, P1L0, P1L1, P1L2, P1L3, etc.
 */
    int
deIntlv_dalsa_4ch_8(u_char * src, int width, int rows, u_char * dest)
{
    int     x, y;
    int     dx = 0;
    int     swidth = width / 4 - 1;
    u_char *sp = src;

    edt_msg(DBG2, "deIntlv_dalsa_4ch_8() -- %d pixel bands\n", swidth);

    sp = src;
    for (y = 0; y < rows; y++)
    {
        dx = y * width;
        for (x = 0; x < width / 4; x++, dx++)
        {
            dest[dx] = *sp++;
            dest[dx + swidth] = *sp++;
            dest[dx + (swidth * 2)] = *sp++;
            dest[dx + (swidth * 3)] = *sp++;
        }
    }
    return (0);
}


/*
 * words are arranged in source as pix0, pix1, pix4094, pix4095, ...
 */
    int
deIntlv_dalsa_ls_4ch_8(u_char * src, int width, int rows, u_char * dest)
{
    int     x, y;
    u_char *sp = src;
    u_char *dp0, *dp1, *dp2, *dp3;

    edt_msg(DBG2, "deIntlv_dalsa_ls_4ch_8()");

    for (y=0; y<rows; y++)
    {
        dp0 = dest + (y * width);
        dp1 = dp0 + 1;
        dp2 = dp0 + width - 2;
        dp3 = dp0 + width - 1;

        for (x = 0; x < width; x+= 4)
        {
            *dp0 = *sp++;
            *dp1 = *sp++;
            *dp2 = *sp++;
            *dp3 = *sp++;
            dp0 += 2;
            dp1 += 2;
            dp2 -= 2;
            dp3 -= 2;
        }
    }
    return (0);
}


/*
 * 4 port spectral instruments upper left, upper right, lower left, lower
 * right, moving in to the center 2 bytes per pixel
 */
    int
deIntlv_specinst_4ch_16(u_short * src, int width, int rows, u_short * dest)
{
    int     x, y;
    int     dxul, dxur, dxll, dxlr;
    u_short *sp = src;

    edt_msg(DBG2, "deIntlv_specinst_4ch_16()\n");

    for (y = 0; y < rows / 2; y++)
    {
        dxul = y * width;
        dxur = dxul + (width - 1);
        dxll = (width * ((rows - y) - 1));
        dxlr = dxll + (width - 1);
        for (x = 0; x < width / 2; x++)
        {
            dest[dxll++] = *sp++;
            dest[dxlr--] = *sp++;
            dest[dxul++] = *sp++;
            dest[dxur--] = *sp++;
        }
    }
    return (0);
}

/*
 * 10-bit 8-tap packed data to 2 bytes-per-pixel  (short) data buffer
 * packed format is: 8x8 MSbits, then 2 bytes with 2 LSbits for each pixel,
 * ordered 3,2,1,0  7,6,5,4
 */
    int
deIntlv_10bit_8tap_packed(u_char * src, int width, int rows, u_short * dest)
{
    u_char *sp = src;
    u_short *dp = dest;
    u_int x, y;

    for (y=0; y<(u_int)rows; y++)
    {
        for (x=0; x<(u_int)width; x+=8)
        {
            u_char *lsb0 = sp+8;
            u_char *lsb1 = sp+9;

            *dp++ = ((u_short)(*sp++) << 2) | (((u_short)(*lsb0) & 0x03)     );
            *dp++ = ((u_short)(*sp++) << 2) | (((u_short)(*lsb0) & 0x0c) >> 2);
            *dp++ = ((u_short)(*sp++) << 2) | (((u_short)(*lsb0) & 0x30) >> 4);
            *dp++ = ((u_short)(*sp++) << 2) | (((u_short)(*lsb0) & 0xc0) >> 6);
            *dp++ = ((u_short)(*sp++) << 2) | (((u_short)(*lsb1) & 0x03)     );
            *dp++ = ((u_short)(*sp++) << 2) | (((u_short)(*lsb1) & 0x0c) >> 2);
            *dp++ = ((u_short)(*sp++) << 2) | (((u_short)(*lsb1) & 0x30) >> 4);
            *dp++ = ((u_short)(*sp++) << 2) | (((u_short)(*lsb1) & 0xc0) >> 6);
            sp +=2;
        }
    }
    return 0;
}

/*
 * 10-bit 8-tap packed data to 1 byte-per-pixel (u_char) data buffer
 * packed format (see above); with this method we just strip out the LSbits
 * leaving 8-bit data.
 */
    int
deIntlv_10bit_8tap_to_8bit(u_char * src, int width, int rows, u_char * dest)
{
    u_char *sp = src;
    u_char *dp = dest;
    u_int x, y;

    for (y=0; y<(u_int)rows; y++)
    {
        for (x=0; x<(u_int)width + (width/4); x+=10)
        {
            int i;

            for (i=0; i<8; i++)
                *dp++ = *sp++;
            sp +=2;
        }
    }
    return 0;
}


/******************************************************************************************************
 * quadrant deinterleave, lr,ur,ll, ul, iterating towards the center
 ******************************************************************************************************
 */
    int
deIntlv_quad_16(u_short *src, int width, int rows, u_short *dest)
{
    int i,r,c, nrows,ncols,qrows,qcols;

    edt_msg(DBG2, "deIntlv_quad_16\n");

    nrows = rows;
    ncols = width;
    qrows = nrows>>1;
    qcols = ncols>>1;

    for (r = 0,i = 0; r < qrows*ncols; r += ncols)
        for (c = 0; c < qcols; ++c, i += 4)
            dest[r+c] = src[i];
    for (r = 0,i = 1; r < qrows*ncols; r += ncols)
        for (c = ncols-1; c >= qcols; --c, i += 4)
            dest[r+c] = src[i];
    for (r = (nrows-1)*ncols,i = 2; r >= qrows*ncols; r -= ncols)
        for (c = 0; c < qcols; ++c, i += 4)
            dest[r+c] = src[i];
    for (r = (nrows-1)*ncols,i = 3; r >= qrows*ncols; r -= ncols)
        for (c = ncols-1; c >= qcols; --c, i += 4)
            dest[r+c] = src[i];
    return (0);
}

    int
deIntlv_quad_8(u_char *src, int width, int rows, u_char *dest)
{
    int i,r,c, nrows,ncols,qrows,qcols;

    edt_msg(DBG2, "deIntlv_quad_8\n");

    nrows = rows;
    ncols = width;
    qrows = nrows>>1;
    qcols = ncols>>1;

    for (r = 0,i = 0; r < qrows*ncols; r += ncols)
        for (c = 0; c < qcols; ++c, i += 4)
            dest[r+c] = src[i];
    for (r = 0,i = 1; r < qrows*ncols; r += ncols)
        for (c = ncols-1; c >= qcols; --c, i += 4)
            dest[r+c] = src[i];
    for (r = (nrows-1)*ncols,i = 2; r >= qrows*ncols; r -= ncols)
        for (c = 0; c < qcols; ++c, i += 4)
            dest[r+c] = src[i];
    for (r = (nrows-1)*ncols,i = 3; r >= qrows*ncols; r -= ncols)
        for (c = ncols-1; c >= qcols; --c, i += 4)
            dest[r+c] = src[i];
    return (0);
}

/********************************************************************
 * 4 port deinterleave, TopLeft, TopMiddle, MiddleLeft, MIddleMiddle,
 * iterating down and to the right
 ********************************************************************
 */
    int
deIntlv_quad2_16(u_short *src, int width, int height, u_short *dest)
{
    int x, y, qwidth, qheight;
    u_short *ul, *um, *ml, *mm;
    u_short *sp = src;

    edt_msg(DBG2, "deIntlv_quad2_8\n");

    qwidth = width/2;
    qheight = height/2;

    for (y=0; y<qheight; y++)
    {
        ul = dest + (y * width);
        um = ul + qwidth;
        ml = ul + (width * y / 2);
        mm = ml + qwidth;

        for (x=0; x<qwidth; x++)
        {
            *(ul++) = *sp++;
            *(um++) = *sp++;
            *(ml++) = *sp++;
            *(mm++) = *sp++;
        }
    }

    return (0);
}

    int
deIntlv_quad2_8(u_char *src, int width, int height, u_char *dest)
{
    int x, y, qwidth, qheight;
    u_char *ul, *um, *ml, *mm;
    u_char *sp = src;

    edt_msg(DBG2, "deIntlv_quad2_8\n");

    qwidth = width/2;
    qheight = height/2;

    for (y=0; y<qheight; y++)
    {
        ul = dest + (y * width);
        um = ul + qwidth;
        ml = ul + (width * y / 2);
        mm = ml + qwidth;

        for (x=0; x<qwidth; x++)
        {
            *(ul++) = *sp++;
            *(um++) = *sp++;
            *(ml++) = *sp++;
            *(mm++) = *sp++;
        }
    }

    return (0);
}

/********************************************************************
 * 4 port deinterleave, similar to deIntlv_quad16 (and 8) but starting
 * in the center and iterating out to the 4 corners
 ********************************************************************
 */
    int
deIntlv_quad3_16(u_short *src, int width, int height, u_short *dest)
{
    int x, y, qwidth, qheight;
    u_short *ul, *ur, *ll, *lr;
    u_short *sp = src;

    edt_msg(DBG2, "deIntlv_quad3_16\n");

    qwidth = width/2;
    qheight = height/2;

    for (y=0; y<qheight; y++)
    {
        ur = dest + (((qheight - y) - 1) * width) + qwidth;
        ul = ur - 1;
        lr = dest + (( qheight + y)      * width) + qwidth;
        ll = lr - 1;

        for (x=0; x<qwidth; x++)
        {
            *(ul--) = *sp++;
            *(ur++) = *sp++;
            *(ll--) = *sp++;
            *(lr++) = *sp++;
        }
    }

    return (0);
}

    int
deIntlv_quad3_8(u_char *src, int width, int height, u_char *dest)
{
    int x, y, qwidth, qheight;
    u_char *ul, *ur, *ll, *lr;
    u_char *sp = src;

    edt_msg(DBG2, "deIntlv_quad3_8\n");

    qwidth = width/2;
    qheight = height/2;

    for (y=0; y<qheight; y++)
    {
        ur = dest + (((qheight - y) - 1) * width) + qwidth;
        ul = ur - 1;
        lr = dest + (( qheight + y)      * width) + qwidth;
        ll = lr - 1;

        for (x=0; x<qwidth; x++)
        {
            *(ul--) = *sp++;
            *(ur++) = *sp++;
            *(ll--) = *sp++;
            *(lr++) = *sp++;
        }
    }

    return (0);
}


/*
 * 4 port deinterleave  -- 4 corners, iterating in towards the center
 * order ul, ur, ll, lr
 */
    int
deIntlv_4ch_ill_16(u_short *src, int width, int height, u_short *dest)
{
    int x, y, qwidth, qheight;
    u_short *ul, *ur, *ll, *lr;
    u_short *sp = src;

    edt_msg(DBG2, "deIntlv_4ch_ill_16\n");

    qwidth = width/2;
    qheight = height/2;

    for (y=0; y<qheight; y++)
    {
        ul = dest + (y * width);
        ur = ul + width - 1;
        ll = dest + ((height - y) - 1) * width;
        lr = ll + width - 1;

        for (x=0; x<qwidth; x++)
        {
            *(lr-x) = *sp++;
            *(ur-x) = *sp++;
            *(ll+x) = *sp++;
            *(ul+x) = *sp++;
        }
    }

    return (0);
}

    int
deIntlv_4ch_ill_8(u_char *src, int width, int height, u_char *dest)
{
    int x, y, qwidth, qheight;
    u_char *ul, *ur, *ll, *lr;
    u_char *sp = src;

    edt_msg(DBG2, "deIntlv_4ch_ill_8\n");

    qwidth = width/2;
    qheight = height/2;

    for (y=0; y<qheight; y++)
    {
        ul = dest + (y * width);
        ur = ul + width - 1;
        ll = dest + ((height - y) - 1) * width;
        lr = ll + width - 1;

        for (x=0; x<qwidth; x++)
        {
            *(ul+x) = *sp++;
            *(ll+x) = *sp++;
            *(ur-x) = *sp++;
            *(lr-x) = *sp++;
        }
    }

    return (0);
}



    int
deintlv_line_taps_8x4(u_char *src, int width, int rows, u_char *dest, int ntaps, PdvInterleaveTap *taps)

{
    int i0, i1, i2, i3, x;

    int row;
    u_char *sp, *dp;

    static int last_alloc = 0;

    static u_char * line_buffer;

    if (width != last_alloc)
    {
        line_buffer = edt_alloc(width);
        last_alloc = width;
    }

    sp = src;
    dp = dest;

    for (row = 0;row < rows;row ++)
    {
        i0 = taps[0].startx;
        i1 = taps[1].startx;
        i2 = taps[2].startx;
        i3 = taps[3].startx;
        for (x = 0; x < width;x += 4)
        {
            line_buffer[i0] = sp[x];
            line_buffer[i1] = sp[x+1];
            line_buffer[i2] = sp[x+2];
            line_buffer[i3] = sp[x+3];

            i0 += taps[0].dx;
            i1 += taps[1].dx;
            i2 += taps[2].dx;
            i3 += taps[3].dx;
        }

        memcpy(dp, line_buffer, width);
        sp += width;
        dp += width;

    }

    return 0;

}

/*
 * dalsa p3 linescan, 4 taps 8 bit arranged with taps 0 & 1 = 1st and 2nd quarter of data,
 *  taps 2 & 3 are 3rd and 4th quarter of data inverted right-left
 */
    int
deIntlv_line_taps_8x4_inv_rt(u_char * src, int width, int rows, u_char * dest)
{
    int i, y, t1, t2, t3, t4;
    int offs2 = width/4;
    int offs3 = ((width/4)*3) - 1;
    int offs4 = width-1;

    static int last_alloc = 0;
    static u_char * line_buffer;

    u_char *sp = src;
    u_char *dp = dest;

    if (width != last_alloc)
    {
        line_buffer = edt_alloc(width);
        last_alloc = width;
    }

    for (y=0; y<rows; y++)
    {
        t1 = 0;
        t2 = offs2;
        t3 = offs3;
        t4 = offs4;

        for (i=0; i<width; i+=4)
        {
            line_buffer[t1++] = *(sp++);
            line_buffer[t2++] = *(sp++);
            line_buffer[t3--] = *(sp++);
            line_buffer[t4--] = *(sp++);
        }
        memcpy(dp, line_buffer, width);
        dp += width;
    }

    return (0);
}


/**
  Convert 24 bit Camera Link to 2 channel 10 or 12 bit

 * channel 0 is first half of line, channel 1 is second half reversed

 */
    int
deIntlv_2ch_inv_rt_24_12(u_char * src,
        int width,
        int rows,
        u_short * dest)
{
    int     y;
    u_short *lp, *rp;
    u_char *sp = src;


#if 0
#ifdef USE_MMX

    if (bayer_can_use_mmx())
    {
        edt_msg(DBG2, "Using MMX\n");
        return Inv_Rt_2ch_deIntlv_2ch_24_12_mmx(src, width, rows, dest);

    }
#endif
#endif

    for (y = 0; y < rows; y++)
    {
        lp = dest + (y * width);
        rp = lp + width - 1;

        while (lp < rp)
        {
            *rp-- = sp[0] + ((sp[1] & 0xf0) << 4);
            *lp++ = sp[2] + ((sp[1] & 15) << 8);

            sp += 3;
        }
    }
    return (0);
}

/**
  Convert 24 bit Camera Link to 2 channel 10 or 12 bit

 * channel 0 is first pixel, channel 1 second

 */
    int
deIntlv_2ch_24_12(u_char * src,
        int width,
        int rows,
        u_short * dest)
{
    int     x, y;
    u_short *lp;
    u_char *sp = src;


    for (y = 0; y < rows; y++)
    {

        lp = dest + (y * width);

        for (x = 0; x < width-1; x +=2)
        {
            lp[x] = sp[0] + ((sp[1] & 0xf0) << 4);
            lp[x+1] = sp[2] + ((sp[1] & 15) << 8);
            sp += 3;
        }
    }
    return (0);
}


    int
deIntlv_1_8_msb7(u_char * src,
        int width,
        int rows,
        u_char * dest)
{
    int     x, y;
    u_char *lp;
    u_char *sp = src;

    for (y = 0; y < rows; y++)
    {

        lp = dest + (y * width);

        for (x = 0; x < width-7; x += 8)
        {
            lp[x]   = (sp[0] & 0x80) ? 255:0;
            lp[x+1] = (sp[0] & 0x40) ? 255:0;
            lp[x+2] = (sp[0] & 0x20) ? 255:0;
            lp[x+3] = (sp[0] & 0x10) ? 255:0;
            lp[x+4] = (sp[0] & 0x8)  ? 255:0;
            lp[x+5] = (sp[0] & 0x4)  ? 255:0;
            lp[x+6] = (sp[0] & 0x2)  ? 255:0;
            lp[x+7] = (sp[0] & 0x1)  ? 255:0;

            sp ++;
        }
    }
    return (0);
}

/**
  Convert 1 bit input into 8 bit for display

 */
    int
deIntlv_1_8_msb0(u_char * src,
        int width,
        int rows,
        u_char * dest)
{
    int     x, y;
    u_char *lp;
    u_char *sp = src;


    for (y = 0; y < rows; y++)
    {

        lp = dest + (y * width);

        for (x = 0; x < width-7; x += 8)
        {
            lp[x]   = (sp[0] & 0x1)  ? 255:0;
            lp[x+1] = (sp[0] & 0x2)  ? 255:0;
            lp[x+2] = (sp[0] & 0x4)  ? 255:0;
            lp[x+3] = (sp[0] & 0x8)  ? 255:0;
            lp[x+4] = (sp[0] & 0x10) ? 255:0;
            lp[x+5] = (sp[0] & 0x20) ? 255:0;
            lp[x+6] = (sp[0] & 0x40) ? 255:0;
            lp[x+7] = (sp[0] & 0x80) ? 255:0;

            sp ++;
        }
    }
    return (0);
}

/*
 * channel 0 is first half of line, channel 1 is second half reversed
 (will work in place)

 */
    int
deIntlv_inv_rt_8(u_char * src, int width, int rows, u_char * dest)
{
    int     y;
    u_char *lp, *rp;
    u_char *sp = src;

    u_char *line_buffer;

    /* use line buffer to be able to do in place */

    line_buffer = edt_alloc(width);

    edt_msg(DBG2, "inv_rt_deIntlv()\n");

    for (y = 0; y < rows; y++)
    {
        lp = line_buffer;
        rp = lp + width - 1;
        while (lp < rp)
        {
            *lp++ = *sp++;
            *rp-- = *sp++;
        }
        memcpy(dest + (y * width), line_buffer, width);

    }

    edt_free(line_buffer);

    return (0);
}

/*
 * channel 0 is first half of line, channel 1 is second half reversed
 (will work in place)
 */
    int
deIntlv_inv_rt_16(u_short * src, int width, int rows, u_short * dest)
{
    int     y;
    u_short *lp, *rp;
    u_short *sp = src;
    u_short *line_buffer;

    edt_msg(DBG2, "deIntlv_inv_rt_16()\n");


    line_buffer = (u_short *) edt_alloc(width * sizeof(u_short));

    if (!line_buffer)
        return -1;

    for (y = 0; y < rows; y++)
    {
        lp = line_buffer;
        rp = lp + width - 1;
        while (lp < rp)
        {
            *lp++ = *sp++;
            *rp-- = *sp++;
        }

        memcpy(dest + y * width,
                line_buffer,
                width * sizeof(u_short));

    }

    edt_free((unsigned char *)line_buffer);

    return (0);
}


/*
 * channel 0 is first half of line, channel 1 is second half reversed
 (will work in place)
 */
    int
deIntlv_inv_rt_16_BGR(u_short * src,
        int width, int rows,
        u_char * dest,
        int order,
        int src_depth)
{
    int rc;

    if ((rc = deIntlv_inv_rt_16(src, width, rows, src)) == 0)
    {

        return convert_bayer_image_16_BGR(src, width, rows,  width, dest,
                order, src_depth);
    }
    else
        return rc;
}

/*
 * channel 0 is first half of line, channel 1 is second half
 */
    int
deIntlv_inv_rt_8_BGR(u_char * src, int width,
        int rows,
        u_char * dest,
        int order)
{
    int rc;

    if ((rc = deIntlv_inv_rt_8(src, width, rows, src)) == 0)
    {

        return convert_bayer_image_8_BGR(src, width, rows,  width, dest,
                order);
    }
    else
        return rc;
}

/*
 * channel 0 is first half of line, channel 1 is second half
 */
    int
deIntlv_2ch_even_rt_8(u_char * src, int width, int rows, u_char * dest)
{
    int     y;
    u_char *lp, *rp, *ep;
    u_char *sp = src;

    edt_msg(DBG2, "deIntlv_2ch_even_rt_8()\n");

    for (y = 0; y < rows; y++)
    {
        lp = dest + (y * width);
        rp = lp + (width / 2);
        ep = lp + width ;
        while (rp < ep)
        {
            *lp++ = *sp++;
            *rp++ = *sp++;
        }
    }
    return (0);
}
/*
 * channel 0 is first half of line, channel 1 is second half
 */
    int
deIntlv_2ch_even_rt_16(u_short * src, int width, int rows, u_short * dest)
{
    int     y;
    u_short *lp, *rp, *ep;
    u_short *sp = src;

    edt_msg(DBG2, "deIntlv_2ch_even_rt_816()\n");

    for (y = 0; y < rows; y++)
    {
        lp = dest + (y * width);
        rp = lp + (width / 2);
        ep = lp + width ;
        while (rp < ep)
        {
            *lp++ = *sp++;
            *rp++ = *sp++;
        }
    }
    return (0);
}

/*
 * Takes a full line from middle, iterating to the top, next line
 * from the middle + 1, iterating to the bottom.
 */
    int
deIntlv_MidTop_Line16(u_short *src, int width, int rows, u_short *dest)
{
    int x, y;
    int dlower_idx = width * (rows/2);
    int dupper_idx = dlower_idx - width;
    u_short *p = src;

    for (y=0; y<rows; y+=2)
    {
        for (x=0; x<width; x++) /* first copy upper half, middle to top */
            dest[dupper_idx+x] = *p++;
        dupper_idx -= width;

        for (x=0; x<width; x++) /* next copy lower half, middle to top */
            dest[dlower_idx+x] = *p++;
        dlower_idx += width;
    }

    return(0);
}

    int
deIntlv_MidTop_Line8(u_char *src, int width, int rows, u_char *dest)
{
    int x, y;
    int dlower_idx = width * (rows/2);
    int dupper_idx = dlower_idx - width;
    u_char *p = src;

    for (y=0; y<rows; y+=2)
    {
        for (x=0; x<width; x++) /* first copy upper half, middle to top */
            dest[dupper_idx+x] = *p++;
        dupper_idx -= width;

        for (x=0; x<width; x++) /* next copy lower half, middle to top */
            dest[dlower_idx+x] = *p++;
        dlower_idx += width;
    }

    return(0);
}

/*
 * Takes a full line from top and then one from the
 * middle of the frame
 */
    int
deIntlv_TopMid_Line16(u_short *src, int width, int rows, u_short *dest)
{
    int x, y;
    int sfront_idx = 0;
    int dfront_idx = 0;
    int dhalf_idx = (width * (rows / 2)) ;

    for (y=0; y<rows; ++y)
    {
        for (x=0; x<width; ++x)
        {
            if (y % 2)
            {
                dest[dhalf_idx+x] = src[sfront_idx+x];
            }
            else
            {
                dest[dfront_idx+x] = src[sfront_idx+x];
            }
        }
        if (y % 2)
            dhalf_idx += width;
        else
            dfront_idx += width;

        sfront_idx += width;
    }

    return(0);

}

/*
 * Takes a full line from top and then one from the
 * bottom of the frame
 */
    int
deIntlv_HiLo_Line16(u_short *src, int width, int rows, u_short *dest)
{
    int x, y;
    int sfront_idx = 0;
    int dfront_idx = 0;
    int dend_idx = (width * (rows - 1)) - 1;

    for (y=0; y<rows; ++y)
    {
        for (x=0; x<width; ++x)
        {
            if (y % 2)
            {
                dest[dend_idx+x] = src[sfront_idx+x];
            }
            else
            {
                dest[dfront_idx+x] = src[sfront_idx+x];
            }
        }
        if(y % 2)
            dend_idx -= width;
        else
            dfront_idx += width;

        sfront_idx += width;
    }

    return(0);

}

/*
 * arbitrary, but usually 20 or 21 -band 16-bit custom
 */
    int
deIntlv_XXband_16(u_short * src, int width, int rows, int bands, u_short * dest)
{
    int     b, x, y, lpb = rows/bands;
    u_short *sp = src;
    u_short *dp = dest;

    /* edt_msg(DBG2, "deIntlv_XXband_16()\n"); */

    for (y=0; y<lpb; y++)
    {
        for (x=0;x<width;x++)
        {
            for (b=0;b<bands;b++)
            {
                int idx = x + (b * width * lpb) + (y * width);
                dest[idx] = *sp++;
            }
        }
    }

    return (0);
}

/*
 * merges image which is split into odd,even fields to destination
 */
int
deIntlv_merge_fields(
        u_char * evenptr,
        u_char * oddptr,
        int width,
        int rows,
        int depth,
        u_char * dest,
        int offset
        )
{
    int     depth_bytes = ((int) depth + 7) / 8;
    int     partial = 0;

    edt_msg(DBG2, "deIntlv_merge_fields() interlace %d offset %d\n",
            oddptr - evenptr, offset);

    width *= depth_bytes;

    /* what should we do with odd number of rows? */
    if (rows & 1)
        rows--;

    /* first line may be partial */
    if (offset)
    {
        partial = 2;
        offset *= depth_bytes;
        memset(dest, 0, offset);
        memcpy(dest + offset, evenptr, width - offset);
        dest += width;
        evenptr += width - offset;

        memcpy(dest, oddptr, width);
        dest += width;
        oddptr += width;

        rows -= 2;
    }

    for (; rows > partial; rows -= 2)
    {
        /* Zeroth line is at start of even field */
        memcpy(dest, evenptr, width);
        dest += width;
        evenptr += width;

        memcpy(dest, oddptr, width);
        dest += width;
        oddptr += width;
    }

    /* last line may be partial */
    if (partial)
    {
        memcpy(dest, evenptr, width);
        dest += width;
        oddptr += width;

        memcpy(dest, evenptr, offset);
        memset(dest + offset, 0, width - offset);
    }
    return (0);
}


/****************************************
 * wrappers
 ****************************************/
pp_deintlv_midtop_line8(void *p_src,
        int width,
        int rows,
        void *p_dest,
        EdtPostProc *pCtrl)
{
    u_char *src = (u_char *) p_src;
    u_char *dest = (u_char *) p_dest;

    return deIntlv_MidTop_Line8(src, width, rows, dest);
}
pp_deintlv_midtop_line16(void *p_src,
        int width,
        int rows,
        void *p_dest,
        EdtPostProc *pCtrl)
{
    u_short *src = (u_short *) p_src;
    u_short *dest = (u_short *) p_dest;

    return deIntlv_MidTop_Line16(src, width, rows, dest);
}

pp_deintlv_topmid_line16(void *p_src,
        int width,
        int rows,
        void *p_dest,
        EdtPostProc *pCtrl)
{
    u_short *src = (u_short *) p_src;
    u_short *dest = (u_short *) p_dest;

    return deIntlv_TopMid_Line16(src, width, rows, dest);
}

pp_deintlv_hilo_line16(void *p_src,
        int width,
        int rows,
        void *p_dest,
        EdtPostProc *pCtrl)
{
    u_short *src = (u_short *) p_src;
    u_short *dest = (u_short *) p_dest;

    return deIntlv_HiLo_Line16(src, width, rows, dest);
}

pp_deintlv_line_topbottom16(void * p_src,
        int width,
        int rows,
        void * p_dest,
        EdtPostProc *pCtrl)

{
    u_short *src = (u_short *) p_src;
    u_short *dest = (u_short *) p_dest;

    return deIntlv_TopBottom16(src, width, rows, dest);

}
pp_deintlv_line_topbottom8(void * p_src,
        int width,
        int rows,
        void * p_dest,
        EdtPostProc *pCtrl)

{
    u_char *src = (u_char *) p_src;
    u_char *dest = (u_char *) p_dest;

    return deIntlv_TopBottom8(src, width, rows, dest);

}
pp_deintlv_line_inout16(void * p_src,
        int width,
        int rows,
        void * p_dest,
        EdtPostProc *pCtrl)

{
    u_short *src = (u_short *) p_src;
    u_short *dest = (u_short *) p_dest;

    return deIntlv_InOut16(src, width, rows, dest);

}
pp_deintlv_line_inout8(void * p_src,
        int width,
        int rows,
        void * p_dest,
        EdtPostProc *pCtrl)

{
    u_char *src = (u_char *) p_src;
    u_char *dest = (u_char *) p_dest;

    return deIntlv_InOut8(src, width, rows, dest);

}

    int
pp_deintlv_line_taps_8x4(void * p_src, int width, int rows,
        void * p_dest, EdtPostProc *pCtrl)



{

    u_char *src = (u_char *) p_src;
    u_char *dest = (u_char *) p_dest;

    return deintlv_line_taps_8x4(src, width, rows, dest,
            pCtrl->nTaps, pCtrl->taps);

}

    int
pp_deintlv_line_taps_8x4_inv_rt(void *p_src, int width, int rows, void *p_dest,
        EdtPostProc *pCtrl)

{

    u_char *src = (u_char *) p_src;
    u_char *dest = (u_char *) p_dest;

    return deIntlv_line_taps_8x4_inv_rt(src, width, rows, dest);
}

    int
pp_convert_bayer_image_16_BGR(void * p_src, int width, int rows,
        void * p_dest, EdtPostProc *pCtrl)

{

    u_short *src = (u_short *) p_src;
    u_char *dest = (u_char *) p_dest;

    return convert_bayer_image_16_BGR(src, width, rows,  width, dest,
            pCtrl->order, pCtrl->src_depth);

}

    int
pp_convert_bayer_image_8_BGR(void * p_src, int width, int rows,
        void * p_dest, EdtPostProc *pCtrl)

{

    u_char *src = (u_char *) p_src;
    u_char *dest = (u_char *) p_dest;

    return convert_bayer_image_8_BGR(src, width, rows,  width, dest,
            pCtrl->order);

}

    int
pp_deIntlv_inv_rt_16_BGR(void * p_src, int width, int rows,
        void * p_dest, EdtPostProc *pCtrl)

{

    u_short *src = (u_short *) p_src;
    u_char *dest = (u_char *) p_dest;

    return deIntlv_inv_rt_16_BGR(src, width, rows, dest,
            pCtrl->order, pCtrl->src_depth);

}

    int
pp_deIntlv_inv_rt_8_BGR(void * p_src, int width, int rows,
        void * p_dest, EdtPostProc *pCtrl)

{

    u_char *src = (u_char *) p_src;
    u_char *dest = (u_char *) p_dest;

    return deIntlv_inv_rt_8_BGR(src, width, rows, dest,
            pCtrl->order);

}

    int
pp_deIntlv_quad_16(void *p_src,
        int width, int rows,
        void *p_dest,
        EdtPostProc *pCtrl)

{

    u_short *src = (u_short *) p_src;
    u_short *dest = (u_short *) p_dest;

    return deIntlv_quad_16(src, width, rows, dest);

}

    int
pp_deIntlv_quad_8(void *p_src,
        int width, int rows,
        void *p_dest,
        EdtPostProc *pCtrl)

{

    u_char *src = (u_char *) p_src;
    u_char *dest = (u_char *) p_dest;

    return deIntlv_quad_8(src, width, rows, dest);

}

    int
pp_deIntlv_quad2_16(void *p_src,
        int width, int rows,
        void *p_dest,
        EdtPostProc *pCtrl)

{

    u_short *src = (u_short *) p_src;
    u_short *dest = (u_short *) p_dest;

    return deIntlv_quad2_16(src, width, rows, dest);

}

    int
pp_deIntlv_quad2_8(void *p_src,
        int width, int rows,
        void *p_dest,
        EdtPostProc *pCtrl)

{

    u_char *src = (u_char *) p_src;
    u_char *dest = (u_char *) p_dest;

    return deIntlv_quad2_8(src, width, rows, dest);

}

    int
pp_deIntlv_quad3_16(void *p_src,
        int width, int rows,
        void *p_dest,
        EdtPostProc *pCtrl)

{

    u_short *src = (u_short *) p_src;
    u_short *dest = (u_short *) p_dest;

    return deIntlv_quad3_16(src, width, rows, dest);

}

    int
pp_deIntlv_quad3_8(void *p_src,
        int width, int rows,
        void *p_dest,
        EdtPostProc *pCtrl)

{

    u_char *src = (u_char *) p_src;
    u_char *dest = (u_char *) p_dest;

    return deIntlv_quad3_8(src, width, rows, dest);

}
    int
pp_deIntlv_4ch_ill_16(void *p_src, int width, int rows, void * p_dest,
        EdtPostProc *pCtrl)

{

    u_short *src = (u_short *) p_src;
    u_short *dest = (u_short *) p_dest;

    return deIntlv_4ch_ill_16(src, width, rows, dest);

}


    int
pp_deIntlv_4ch_ill_8(void *p_src, int width, int rows, void *p_dest,
        EdtPostProc *pCtrl)

{

    u_char *src = (u_char *) p_src;
    u_char *dest = (u_char *) p_dest;

    return deIntlv_4ch_ill_8(src, width, rows, dest);

}


    int
pp_deIntlv_piranha_8(void *p_src, int width, int rows, void *p_dest,
        EdtPostProc *pCtrl)

{

    u_char *src = (u_char *) p_src;
    u_char *dest = (u_char *) p_dest;

    return deIntlv_piranha_8(src, width, rows, dest);

}

    int
pp_deIntlv_dalsa_4ch_8(void *p_src, int width, int rows, void *p_dest,
        EdtPostProc *pCtrl)

{

    u_char *src = (u_char *) p_src;
    u_char *dest = (u_char *) p_dest;

    return deIntlv_dalsa_4ch_8(src, width, rows, dest);

}

    int
pp_deIntlv_dalsa_ls_4ch_8(void *p_src, int width, int rows, void *p_dest,
        EdtPostProc *pCtrl)

{

    u_char *src = (u_char *) p_src;
    u_char *dest = (u_char *) p_dest;

    return deIntlv_dalsa_ls_4ch_8(src, width, rows, dest);

}


    int
pp_deIntlv_inv_rt_16(void * p_src, int width, int rows, void * p_dest,
        EdtPostProc *pCtrl)

{

    u_short *src = (u_short *) p_src;
    u_short *dest = (u_short *) p_dest;

    return deIntlv_inv_rt_16(src, width, rows, dest);

}


    int
pp_deIntlv_inv_rt_8(void * p_src, int width, int rows, void * p_dest,
        EdtPostProc *pCtrl)

{

    u_char *src = (u_char *) p_src;
    u_char *dest = (u_char *) p_dest;

    return deIntlv_inv_rt_8(src, width, rows, dest);

}


    int
pp_deIntlv_2ch_even_rt_16(void * p_src, int width, int rows, void * p_dest,
        EdtPostProc *pCtrl)

{

    u_short *src = (u_short *) p_src;
    u_short *dest = (u_short *) p_dest;

    return deIntlv_2ch_even_rt_16(src, width, rows, dest);

}


    int
pp_deIntlv_2ch_even_rt_8(void * p_src, int width, int rows, void * p_dest,
        EdtPostProc *pCtrl)

{

    u_char *src = (u_char *) p_src;
    u_char *dest = (u_char *) p_dest;

    return deIntlv_2ch_even_rt_8(src, width, rows, dest);

}


    int
pp_deIntlv_2ch_inv_rt_24_12(void * p_src, int width, int rows,
        void * p_dest,
        EdtPostProc *pCtrl)

{

    u_char *src = (u_char *) p_src;
    u_short *dest = (u_short *) p_dest;

    return deIntlv_2ch_inv_rt_24_12(src, width, rows, dest);

}


    int
pp_deIntlv_2ch_24_12(void * p_src, int width, int rows, void * p_dest,
        EdtPostProc *pCtrl)

{

    u_char *src = (u_char *) p_src;
    u_short *dest = (u_short *) p_dest;

    return deIntlv_2ch_24_12(src, width, rows, dest);

}


    int
pp_deIntlv_1_8_msb7(void * p_src,
        int width,
        int rows,
        void * p_dest,
        EdtPostProc *pCtrl)

{

    u_char *src = (u_char *) p_src;
    u_char *dest = (u_char *) p_dest;

    return deIntlv_1_8_msb7(src, width, rows, dest);

}

    int
pp_deIntlv_1_8_msb0(void * p_src,
        int width,
        int rows,
        void * p_dest,
        EdtPostProc *pCtrl)

{

    u_char *src = (u_char *) p_src;
    u_char *dest = (u_char *) p_dest;

    return deIntlv_1_8_msb0(src, width, rows, dest);

}


    int
pp_deIntlv_ES10_8(void * p_src,
        int width,
        int rows,
        void * p_dest,
        EdtPostProc *pCtrl)

{
    u_char *src = (u_char *) p_src;
    u_char *dest = (u_char *) p_dest;

    return deIntlv_ES10_8(src, width, rows, dest);

}

    int
pp_ES10deIntlv_16(void * p_src,
        int width,
        int rows,
        void * p_dest,
        EdtPostProc *pCtrl)

{
    u_short *src = (u_short *) p_src;
    u_short *dest = (u_short *) p_dest;

    return deIntlv_ES10_16(src, width, rows, dest);

}

    int
pp_ES10deIntlv_16_odd(void * p_src,
        int width,
        int rows,
        void * p_dest,
        EdtPostProc *pCtrl)

{
    u_short *src = (u_short *) p_src;
    u_short *dest = (u_short *) p_dest;

    return deIntlv_ES10_16_odd(src, width, rows, dest);

}

    int
pp_ES10deIntlv_16_hilo(void * p_src,
        int width,
        int rows,
        void * p_dest,
        EdtPostProc *pCtrl)

{
    u_short *src = (u_short *) p_src;
    u_short *dest = (u_short *) p_dest;

    return deIntlv_ES10_16_hilo(src, width, rows, dest);

}

    int
pp_deIntlv_ES10_8_BGGR(void * p_src,
        int width,
        int rows,
        void * p_dest,
        EdtPostProc *pCtrl)

{
    u_char *src = (u_char *) p_src;
    u_char *dest = (u_char *) p_dest;

    /* work in place */

    deIntlv_ES10_8(src, width, rows, src);

    return convert_bayer_image_8_BGR(src, width, rows, width,  dest,
            pCtrl->order);
}

    int
pp_deIntlv_specinst_4ch_16(void * p_src,
        int width,
        int rows,
        void * p_dest,
        EdtPostProc *pCtrl)

{
    u_short *src = (u_short *) p_src;
    u_short *dest = (u_short *) p_dest;

    return deIntlv_specinst_4ch_16(src, width, rows, dest);

}

    int
pp_deIntlv_10bit_8tap_packed(void * p_src,
        int width,
        int rows,
        void * p_dest,
        EdtPostProc *pCtrl)

{
    u_char *src = (u_char *) p_src;
    u_short *dest = (u_short *) p_dest;

    return deIntlv_10bit_8tap_packed(src, width, rows, dest);

}

    int
pp_deIntlv_10bit_8tap_to_8bit(void * p_src,
        int width,
        int rows,
        void * p_dest,
        EdtPostProc *pCtrl)

{
    u_char *src = (u_char *) p_src;
    u_char *dest = (u_char *) p_dest;

    return deIntlv_10bit_8tap_to_8bit(src, width, rows, dest);

}


    int
pp_merge_fields(void * p_src,
        int width,
        int rows,
        void * p_dest,
        EdtPostProc *pCtrl)

{
    u_char *src = (u_char *) p_src;
    u_char *dest = (u_char *) p_dest;

    return deIntlv_merge_fields(src, src + pCtrl->interlace, width, rows, pCtrl->src_depth, dest,
            pCtrl->offset);

}




    int
pp_bgr_2_rgb(void *p_src,
        int width,
        int rows,
        void * p_dest,
        EdtPostProc *pCtrl)

{
    u_char *src = (u_char *) p_src;
    u_char *dest = (u_char *) p_dest;
    int x, y;
    int w = width * 3;

    for (y=0;y<rows;y++)
    {
        for (x = 0; x < w; x+=3)
        {
            u_char t = src[x];
            dest[x] = src[x+2];
            dest[x+2] = t;
        }
        src += w;
        dest += w;
    }

    return 0;
}

pp_deIntlv_20band_16(void * p_src,
        int width,
        int rows,
        void * p_dest,
        EdtPostProc *pCtrl)
{
    u_short *src = (u_short *) p_src;
    u_short *dest = (u_short *) p_dest;

    return deIntlv_XXband_16(src, width, rows, 20, dest);
}

pp_deIntlv_21band_16(void * p_src,
        int width,
        int rows,
        void * p_dest,
        EdtPostProc *pCtrl)
{
    u_short *src = (u_short *) p_src;
    u_short *dest = (u_short *) p_dest;

    return deIntlv_XXband_16(src, width, rows, 21, dest);
}



    int
pdv_deinterlace(PdvDev *pdv_p,
        PdvDependent *dd_p,
        u_char *dmabuf,
        u_char *output_buf)

{

    EdtPostProc *pCtrl = NULL;


    int frame_height = dd_p->height;

    if (pdv_p)
        pCtrl = (EdtPostProc *) pdv_p->pInterleaver;

    if (pCtrl == NULL)
    {
        if (dd_p->swinterlace)
            pCtrl = pdv_setup_postproc(pdv_p, dd_p, NULL);

        if (pCtrl == NULL)
            return 0;
    }

    if (dd_p->frame_height != 0)
    {
        frame_height = dd_p->frame_height;
    }

    if (pCtrl->process)
    {
        return pCtrl->process(dmabuf, dd_p->width, frame_height, output_buf, pCtrl);
    }
    else
    {
        return -1;
    }

}

    int
deIntlv_buffers(EdtPostProc *pCtrl, void *src_p, void *dest_p, int width, int height)

{

    if (pCtrl->process)
    {
        return pCtrl->process(src_p, width, height, dest_p, pCtrl);
    }
    else
    {
        return -1;
    }

}


/* Set of default de-interleave functions */

static EdtPostProc built_in_postproc[] = {
    {PDV_BYTE_INTLV, EDT_TYPE_BYTE, EDT_TYPE_BYTE,
        pp_deIntlv_ES10_8, 0},
    {PDV_ES10_BGGR, EDT_TYPE_BYTE, EDT_TYPE_BGR,
        pp_deIntlv_ES10_8_BGGR, 0},
    {PDV_WORD_INTLV, EDT_TYPE_USHORT, EDT_TYPE_USHORT,
        pp_ES10deIntlv_16, 0},
    {PDV_WORD_INTLV_ODD, EDT_TYPE_USHORT, EDT_TYPE_USHORT,
        pp_ES10deIntlv_16_odd, 0},
    {PDV_WORD_INTLV_HILO, EDT_TYPE_USHORT, EDT_TYPE_USHORT,
        pp_ES10deIntlv_16_hilo, 0},
    {PDV_FIELD_INTLC, EDT_TYPE_BYTE, EDT_TYPE_BYTE,
        pp_merge_fields, 0},
    {PDV_FIELD_INTLC, EDT_TYPE_USHORT, EDT_TYPE_USHORT,
        pp_merge_fields, 0},
    {PDV_FIELD_INTLC, EDT_TYPE_BGR, EDT_TYPE_BGR,
        pp_merge_fields, 0},
    {PDV_BGGR_WORD, EDT_TYPE_USHORT, EDT_TYPE_BGR,
        pp_convert_bayer_image_16_BGR, 0},
    {PDV_BGGR, EDT_TYPE_BYTE, EDT_TYPE_BGR,
        pp_convert_bayer_image_8_BGR, 0},
    {PDV_DALSA_4CH_INTLV, EDT_TYPE_BYTE, EDT_TYPE_BYTE,
        pp_deIntlv_dalsa_4ch_8, 0},
    {PDV_DALSA_LS_4CH_INTLV, EDT_TYPE_BYTE, EDT_TYPE_BYTE,
        pp_deIntlv_dalsa_ls_4ch_8, 0},
    {PDV_INVERT_RIGHT_INTLV, EDT_TYPE_BYTE, EDT_TYPE_BYTE,
        pp_deIntlv_inv_rt_8, 0},
    {PDV_INVERT_RIGHT_INTLV, EDT_TYPE_USHORT, EDT_TYPE_USHORT,
        pp_deIntlv_inv_rt_16, 0},
    {PDV_INVERT_RIGHT_BGGR_INTLV, EDT_TYPE_BYTE, EDT_TYPE_BGR,
        pp_deIntlv_inv_rt_8_BGR, 0},
    {PDV_INVERT_RIGHT_BGGR_INTLV, EDT_TYPE_USHORT, EDT_TYPE_BGR,
        pp_deIntlv_inv_rt_16_BGR, 0},
    {PDV_EVEN_RIGHT_INTLV, EDT_TYPE_BYTE, EDT_TYPE_BYTE,
        pp_deIntlv_2ch_even_rt_8, 0},
    {PDV_EVEN_RIGHT_INTLV, EDT_TYPE_USHORT, EDT_TYPE_USHORT,
        pp_deIntlv_2ch_even_rt_16, 0},
    {PDV_PIRANHA_4CH_INTLV, EDT_TYPE_BYTE, EDT_TYPE_BYTE,
        pp_deIntlv_piranha_8, 0},
    {PDV_PIRANHA_4CH_INTLV, EDT_TYPE_USHORT, EDT_TYPE_BYTE,
        pp_deIntlv_piranha_8, 0},
    {PDV_SPECINST_4PORT_INTLV, EDT_TYPE_USHORT, EDT_TYPE_USHORT,
        pp_deIntlv_specinst_4ch_16, 0},
    {PDV_QUADRANT_INTLV, EDT_TYPE_BYTE, EDT_TYPE_BYTE,
        pp_deIntlv_quad_8, 0},
    {PDV_QUADRANT_INTLV, EDT_TYPE_USHORT, EDT_TYPE_USHORT,
        pp_deIntlv_quad_16, 0},
    {PDV_QUADRANT2_INTLV, EDT_TYPE_BYTE, EDT_TYPE_BYTE,
        pp_deIntlv_quad2_8, 0},
    {PDV_QUADRANT3_INTLV, EDT_TYPE_BYTE, EDT_TYPE_BYTE,
        pp_deIntlv_quad3_8, 0},
    {PDV_QUADRANT3_INTLV, EDT_TYPE_USHORT, EDT_TYPE_USHORT,
        pp_deIntlv_quad3_16, 0},
    {PDV_ILLUNIS_INTLV, EDT_TYPE_BYTE, EDT_TYPE_BYTE,
        pp_deIntlv_4ch_ill_8, 0},
    {PDV_ILLUNIS_INTLV, EDT_TYPE_SHORT, EDT_TYPE_USHORT,
        pp_deIntlv_4ch_ill_16, 0},
    {PDV_INV_RT_INTLV_24_12, EDT_TYPE_BGR, EDT_TYPE_USHORT,
        pp_deIntlv_2ch_inv_rt_24_12, 0},
    {PDV_INTLV_24_12, EDT_TYPE_BGR, EDT_TYPE_USHORT,
        pp_deIntlv_2ch_24_12, 0},
    {PDV_INTLV_1_8_MSB7, EDT_TYPE_BYTE, EDT_TYPE_BYTE,
        pp_deIntlv_1_8_msb7, 0},
    {PDV_INTLV_1_8_MSB0, EDT_TYPE_BYTE, EDT_TYPE_BYTE,
        pp_deIntlv_1_8_msb0},
    {PDV_INTLV_BGR_2_RGB, EDT_TYPE_BGR, EDT_TYPE_BGR, /* use EDT_TYPE_BGR since display expects it */
        pp_bgr_2_rgb},
    {PDV_LINE_INTLV, EDT_TYPE_BYTE, EDT_TYPE_BYTE,
        pp_deintlv_line_taps_8x4},
    {PDV_LINE_INTLV_P3_8X4, EDT_TYPE_BYTE, EDT_TYPE_BYTE,
        pp_deintlv_line_taps_8x4_inv_rt},
    {PDV_WORD_INTLV_HILO_LINE, EDT_TYPE_USHORT, EDT_TYPE_USHORT,
        pp_deintlv_hilo_line16},
    {PDV_WORD_INTLV_TOPMID_LINE, EDT_TYPE_USHORT, EDT_TYPE_USHORT,
        pp_deintlv_topmid_line16},
    {PDV_WORD_INTLV_MIDTOP_LINE, EDT_TYPE_USHORT, EDT_TYPE_USHORT,
        pp_deintlv_midtop_line16},
    {PDV_BYTE_INTLV_MIDTOP_LINE, EDT_TYPE_BYTE, EDT_TYPE_BYTE,
        pp_deintlv_midtop_line8},
    {PDV_WORD_INTLV_TOPBOTTOM, EDT_TYPE_USHORT, EDT_TYPE_USHORT,
        pp_deintlv_line_topbottom16},
    {PDV_BYTE_INTLV_TOPBOTTOM, EDT_TYPE_BYTE, EDT_TYPE_BYTE,
        pp_deintlv_line_topbottom8},
    {PDV_WORD_INTLV_INOUT, EDT_TYPE_USHORT, EDT_TYPE_USHORT,
        pp_deintlv_line_inout16},
    {PDV_BYTE_INTLV_INOUT, EDT_TYPE_BYTE, EDT_TYPE_BYTE,
        pp_deintlv_line_inout8},
    {PDV_INTLV_10BIT_8TAP_PACKED, EDT_TYPE_BYTE, EDT_TYPE_USHORT,
        pp_deIntlv_10bit_8tap_packed},
    {PDV_INTLV_10BIT_8TAP_TO_8BIT, EDT_TYPE_BYTE, EDT_TYPE_BYTE,
        pp_deIntlv_10bit_8tap_to_8bit},
    {PDV_INTLV_20BAND, EDT_TYPE_USHORT, EDT_TYPE_USHORT,
        pp_deIntlv_20band_16},
    {PDV_INTLV_21BAND, EDT_TYPE_USHORT, EDT_TYPE_USHORT,
        pp_deIntlv_21band_16},
    {0,0,0,0}
};

/* default pixel types from depth */

int pdv_pixel_type_from_depth(int depth)
{
    int type = EDT_TYPE_BYTE;

    if (depth <= 8)
        type =  EDT_TYPE_BYTE;
    else if (depth >8 && depth <= 16)
        type =  EDT_TYPE_USHORT;
    else if (depth > 16 && depth <= 24)
        type =  EDT_TYPE_BGR;
    else
        type =  EDT_TYPE_BGRA;

    return type;
}


    EdtPostProc *
pdv_lookup_postproc(int func_type, int src_depth, int depth)

{
    EdtPostProc *pCtrl;

    int src_type = pdv_pixel_type_from_depth(src_depth);
    int dest_type = pdv_pixel_type_from_depth(depth);

    pCtrl = &built_in_postproc[0];

    while (pCtrl->func_type != 0 &&
            (pCtrl->func_type != func_type ||
             pCtrl->src_type != src_type ||
             pCtrl->dest_type != dest_type))
    {
        pCtrl ++;
    }

    if (pCtrl->func_type == 0)

        return NULL;

    return pCtrl;

}


int
pdv_set_postproc(EdtPostProc *pCtrl,
        int depth,
        int extdepth,
        int frame_height,
        int interlace,
        int image_offset,
        int order,
        int n_intlv_taps,
        PdvInterleaveTap *taps
        )

{


    pCtrl->interlace = interlace;
    pCtrl->order = order;

    pCtrl->frame_height = frame_height;

    pCtrl->src_type = pdv_pixel_type_from_depth(extdepth);
    pCtrl->dest_type = pdv_pixel_type_from_depth(depth);

    pCtrl->src_depth = extdepth;
    pCtrl->dest_depth = depth;

    pCtrl->offset = image_offset;

    pCtrl->nTaps = n_intlv_taps;
    if (n_intlv_taps)
        memcpy(pCtrl->taps, taps, n_intlv_taps * sizeof(PdvInterleaveTap));

    return 0;


}

    int
pdv_update_postproc(PdvDev *pdv_p, PdvDependent *dd_p,  EdtPostProc *pCtrl)

{
    /* fill in the width, height, depth, etc. */

    if (dd_p == NULL)
    {
        if (pdv_p)
            dd_p = pdv_p->dd_p;
        else
            return -1;
    }

    pdv_set_postproc(pCtrl,
            dd_p->depth,
            dd_p->extdepth,
            dd_p->frame_height,
            dd_p->interlace,
            dd_p->image_offset,
            PDV_BAYER_ORDER(dd_p),
            dd_p->n_intlv_taps,
            dd_p->intlv_taps);



    return 0;


}


    EdtPostProc *
pdv_setup_postproc(PdvDev *pdv_p, PdvDependent *dd_p, EdtPostProc *pInCtrl)

{

    EdtPostProc *pCtrl = pInCtrl;

    /* look up swinterlace */
    if (dd_p == NULL)
        dd_p = pdv_p->dd_p;

    if (dd_p->interlace_module[0])
    {
        if (!pInCtrl)
        {
            pCtrl = (EdtPostProc *) edt_alloc(sizeof(*pInCtrl));

            memset(pCtrl,0,sizeof(*pCtrl));

            if (pdv_load_postproc_module(pCtrl, dd_p->interlace_module,
                        dd_p->extdepth, dd_p->depth) == 0)
            {


                if (pdv_p)
                    pdv_p->pInterleaver = pCtrl;

                pdv_update_postproc(pdv_p, dd_p, pCtrl);

                if (pCtrl->defaultInit)
                    pCtrl->defaultInit(pdv_p, pCtrl);

                return pCtrl;
            }

        }


    }

    if (dd_p->swinterlace)
    {

        if (pInCtrl == NULL)
            pInCtrl = pdv_lookup_postproc(dd_p->swinterlace,
                    dd_p->extdepth, dd_p->depth);

    }

    /* update  */


    if (pInCtrl)
    {

        pCtrl = (EdtPostProc *) edt_alloc(sizeof(*pCtrl));

        memcpy(pCtrl, pInCtrl, sizeof(*pCtrl));

        if (pdv_p)
            pdv_p->pInterleaver = pCtrl;

        pdv_update_postproc(pdv_p, dd_p, pCtrl);

        if (pCtrl->defaultInit)
            pCtrl->defaultInit(pdv_p, pCtrl);

    }

    return pCtrl;
}

    int
pdv_unload_postproc_module(EdtPostProc *pCtrl)

{
    return 0;
}

    int
pdv_load_postproc_module(EdtPostProc *pCtrl,
        char *name,
        int srcdepth,
        int destdepth)

{

#ifndef EDT_CAMLINK_EXPORTS /* don't need this for clserial */

#ifdef WIN32
    char process_name[80];
    char filename[MAX_PATH];


    pCtrl->dll_handle = LoadLibrary(name);

    if (!pCtrl->dll_handle)
    {
        sprintf(filename,"postproc/%s", name);
        pCtrl->dll_handle = LoadLibrary(filename);
    }

    if (pCtrl->dll_handle)
    {
        strcpy(pCtrl->dll_name, name);

        if (srcdepth > 8 && srcdepth < 16)
            srcdepth = 16;

        if (destdepth > 8 && destdepth < 16)
            destdepth = 16;

        sprintf(process_name, "post_process_%d_%d", srcdepth, destdepth);

        pCtrl->process = (post_process_f) GetProcAddress(pCtrl->dll_handle,
                process_name);

        pCtrl->doinplace = (int (*)()) GetProcAddress(pCtrl->dll_handle,
                "post_process_doinplace");

    }
    else
    {
        return -1;

    }

#elif defined(__linux__)

#include <dlfcn.h>

    char process_name[80];

    pCtrl->dll_handle = dlopen(name, RTLD_LAZY);

    if (pCtrl->dll_handle)
    {
        strcpy(pCtrl->dll_name, name);

        if (srcdepth > 8 && srcdepth < 16)
            srcdepth = 16;

        if (destdepth > 8 && destdepth < 16)
            destdepth = 16;

        sprintf(process_name, "post_process_%d_%d", srcdepth, destdepth);

        pCtrl->process = (post_process_f) dlsym(pCtrl->dll_handle,
                process_name);

        pCtrl->doinplace = (int (*)()) dlsym(pCtrl->dll_handle,
                "post_process_doinplace");

    }
    else
    {
        return -1;

    }


#endif /* WIN_32 or __linux__ */

#endif /* NO_POSTPROC */

    return 0;

}

