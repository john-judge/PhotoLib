/**
 * @file
 * PCI DV Library routines that enable decoding of bayer formatted images read in from
 * a PCI DV digital imaging board.
 */

#include "edtinc.h"
#include "pdv_interlace_methods.h"

#include <math.h>


/*********************
* BAYER STRIPE ORDER (from upper left pixel)
* Letters define a box like:
*     Blue Green
*     Green Red  => PDV_BAYER_ORDER_BGGR
*
* Can be computed from the dd_p->kbs_red_row_first << 1
*                           | dd_p->kbs_green_pixel_first
*
*********************/

#define PDV_BAYER_ORDER_BGGR 0
#define PDV_BAYER_ORDER_GBRG 1
#define PDV_BAYER_ORDER_RGGB 2
#define PDV_BAYER_ORDER_GRBG 3

#define PDV_BAYER_ORDER(dd_p) ((((Dependent *) dd_p)->kbs_red_row_first << 1) \
    | ((Dependent *) dd_p)->kbs_green_pixel_first)


/********************************************
*
*
*
********************************************/



u_char * pdv_red_lut = NULL;
u_char * pdv_green_lut = NULL;
u_char * pdv_blue_lut = NULL;


#define USING_LUTS

#ifdef USING_LUTS

#define RED_LUT(x) pdv_red_lut[(x)]
#define GREEN_LUT(x) pdv_green_lut[(x)]
#define BLUE_LUT(x) pdv_blue_lut[(x)]

#else

#define RED_LUT(x) (u_char)(x)
#define GREEN_LUT(x) (u_char)(x)
#define BLUE_LUT(x) (u_char)(x)

#endif

int     bayer_current_base_size = 0;

double  default_rgb_scale[3] = {1.0, 1.0, 1.0};
double  bayer_rgb_scale[3] = {1.0, 1.0, 1.0};

double bayer_even_row_scale = 1.0, bayer_odd_row_scale = 1.0;

int bayer_red_first = 0;
int bayer_green_first = 0;

static void allocate_luts(int outsize)
			  

{
 
    /* assume luts are allocated */

    pdv_red_lut = edt_alloc(outsize);
    pdv_green_lut = edt_alloc(outsize);
    pdv_blue_lut = edt_alloc(outsize);

    bayer_current_base_size = outsize;


}

void get_bayer_luts(u_char **red, u_char **green, u_char **blue)

{
    
    if (!pdv_red_lut)
	allocate_luts(256);

    *red   = pdv_red_lut;
    *green = pdv_green_lut;
    *blue  = pdv_blue_lut;
}

u_char 
byte_gamma_function(double val, double scale, double gamma, int blackoffset)

{
    double  v = val;

    v -= blackoffset;

    if (v < 0)
	v = 0;

    v = 255 * pow((double) (scale * v) / 255.0, 1.0 / gamma);

    if (v > 255)
	v = 255;

    return (u_char) v;

}


void
set_bayer_parameters(int input_bits,
		     double rgb_scale[3],
		     double gamma,
		     int blackoffset,
		     int red_row_first,
		     int green_pixel_first)

{

    int     base_size = 256; /* for now, all bayer out is 888 */

    double  maxvalue;
    double  delta;
    double  val = 0;
    u_char  rval, gval, bval;
    int     i;

#ifndef USE_MMX

    base_size = 1 << input_bits;

#endif

    maxvalue = base_size;
    delta = (1.0 / maxvalue) * 256;

    bayer_red_first = red_row_first;
    bayer_green_first = green_pixel_first;

    if (gamma <= 0)
	gamma = 1.0;

    for (i = 0; i < 3; i++)
	bayer_rgb_scale[i] = rgb_scale[i];

    if (!pdv_red_lut)
	allocate_luts(base_size);

    for (i = 0; i < base_size; i++)
    {
	rval = byte_gamma_function(val, rgb_scale[0], gamma, blackoffset);
	gval = byte_gamma_function(val, rgb_scale[1], gamma, blackoffset);
	bval = byte_gamma_function(val, rgb_scale[2], gamma, blackoffset);

	pdv_red_lut[i] = rval;
	pdv_green_lut[i] = gval;
	pdv_blue_lut[i] = bval;

	val += delta;

    }


#ifdef USE_MMX

    if (bayer_can_use_mmx())
	set_bayer_parameters_mmx(input_bits,
	rgb_scale,
	gamma,
	blackoffset,
	red_row_first,
	green_pixel_first);

#endif

}

/**
 * Sets the full bayer parameters for images for PCI DV library decoding
 * of bayer formatted color image data. Bayer decoding by the library is
 * typically enabled by setting the config file directive \p
 * method_interlace to \p BGGR or \p BGGR_WORD; this subroutine can be
 * used to manipulate the specific Bayer decoding parameters.  Images
 * captured with #pdv_image, #pdv_wait_images or othe PCI DV library
 * acquisition routines (excepting \p _raw routines) will be preprocessed to
 * RGB color before the image pointer is returned.
 * 
 * The \p bRedRowFirst and \p bGreenPixelFirst parameters are typically
 * initialized by the \p kbs_red_row_first and \p kbs_green_pixel_first
 * configuration file directives.  Current values can be found in the
 * \p PdvDev \p dd_p->kbs_green_pixel_first and \p dd_p->kbs_dd_p->red_row_first
 * structure elements.
 * 
 * The most common operation for #pdv_set_full_bayer_parameters is
 * adjusting the white balance.  To do so, the calling application should
 * provide a method for acquiring an image of a white background,
 * calcluate the average of all pixels in each of the R, G and B
 * components, then set \p scale[0] (green) to 1.0, and adjust \p
 * scale[1-2] (red/blue) such that red and blue will be scaled
 * appropriately. Click on the color wheel toolbar icon in PdvShow to
 * see an example of  such an implementation.
 * 
 * Note that the Bayer decoding functionality uses MMX instructions when
 * run under the Windows environment, providing greater efficiency and
 * more algorithm (quality) options. Only one algorithm is defined in the
 * Linux/Unix implementation so the \p quality parameter will be ignored
 * on those platforms.
 * 
 * @param nSourceDepth depth in bits of source (unfiltered) data
 * @param scale array of 3 values (R,G,B) for scaling (gain); default 1.0, 1.0, 1.0
 * @param gamma gamma value -- default 1.0
 * @param nBlackOffset Black Offset (black level); 1 is default
 * @param bRedRowFirst 1 if red/green row is first on the sensor, 0 if blue/green is first
 * @param bGreenPixelFirst 1 if green pixel is first on sensor, 0 if red or blue
 * @param quality selects one of 3 Bayer decoding algorithms: 0=Bilinear, 1=Gradient, 2=Bias-corrected
 * -- MS Windows only. Note that in Linux/Unix, only Bilinear is implemented and this parameter is ignored
 * @param bias selects the bias for bias method Bayer algorithm; (MS Windows only)
 * @param gradientcolor selects the gradient for the gradient Bayer algorithm (MS Windows
 * only)
 * 
 * @see \b method_interlace, \b kbs_red_row_first, \b kbs_green_pixel_first camera configuration directives -- see the <a href="ftp://ftp.edt.com/pub/manuals/PDV/camconfig.pdf">Camera configuration guide</A>
 *
 * @ingroup settings
 */
void
pdv_set_full_bayer_parameters(int nSourceDepth,
			      double scale[3],
			      double gamma,
			      int nBlackOffset,
			      int bRedRowFirst,
			      int bGreenPixelFirst,
			      int quality,
			      int bias,
			      int gradientcolor)

{
    set_bayer_parameters(nSourceDepth,
	scale,
	gamma,
	nBlackOffset,
	bRedRowFirst,
	bGreenPixelFirst);


#ifdef USE_MMX

    // not running on Linux yet
    set_bayer_quality(quality);
    set_bayer_gradient_color(gradientcolor);

    set_bayer_bias_value(bias);
#endif

}

void
set_bayer_even_odd_row_scale(double evenscale, double oddscale)

{
    bayer_even_row_scale = evenscale;
    bayer_odd_row_scale = oddscale;

}

void 
clear_bayer_parameters()

{

}



void
convert_red_row_16_green(u_short * lst_p,
			 u_short * cur_p,
			 u_short * nxt_p,
			 u_char * dest_p,
			 int width)

{
    u_short *end_p = cur_p + (width - 4);

    do
    {

	/* blue pixels */

	dest_p[0] = BLUE_LUT((lst_p[0] + nxt_p[0]) >> 1);
	dest_p[3] = BLUE_LUT((lst_p[0] + nxt_p[0] + lst_p[2] + nxt_p[2]) >> 2);
	dest_p[6] = BLUE_LUT((lst_p[2] + nxt_p[2]) >> 1);
	dest_p[9] = BLUE_LUT((lst_p[2] + nxt_p[2] + lst_p[4] + nxt_p[4]) >> 2);

	dest_p[1] = GREEN_LUT(cur_p[0]);
	dest_p[7] = GREEN_LUT(cur_p[2]);
	dest_p[4] = GREEN_LUT((cur_p[0] + cur_p[2] + lst_p[1] + nxt_p[1]) >> 2);
	dest_p[10] = GREEN_LUT((cur_p[2] + cur_p[4] + lst_p[3] + nxt_p[3]) >> 2);

	/* red pixels */

	dest_p[5] = RED_LUT(cur_p[1]);
	dest_p[11] = RED_LUT(cur_p[3]);
	dest_p[2] = RED_LUT((cur_p[-1] + cur_p[1]) >> 1);
	dest_p[8] = RED_LUT((cur_p[1] + cur_p[3]) >> 1);


	dest_p += 12;
	lst_p += 4;
	cur_p += 4;
	nxt_p += 4;

    } while (cur_p < end_p);

}

void
convert_red_row_16(u_short * lst_p, u_short * cur_p, u_short * nxt_p,
		   u_char * dest_p, int width)

{
    int     x;

    for (x = 1; x < width - 4; x += 4)
    {

	dest_p[0] = BLUE_LUT((lst_p[-1] + nxt_p[-1] + lst_p[1] + nxt_p[1]) >> 2);
	dest_p[3] = BLUE_LUT((lst_p[1] + nxt_p[1]) >> 1);
	dest_p[6] = BLUE_LUT((lst_p[1] + nxt_p[1] + lst_p[3] + nxt_p[3]) >> 2);
	dest_p[9] = BLUE_LUT((lst_p[3] + nxt_p[3]) >> 1);

	dest_p[1] = GREEN_LUT((cur_p[-1] + cur_p[1] + lst_p[0] + nxt_p[0]) >> 2);
	dest_p[4] = GREEN_LUT(cur_p[1]);
	dest_p[7] = GREEN_LUT((cur_p[1] + cur_p[3] + lst_p[2] + nxt_p[2]) >> 2);
	dest_p[10] = GREEN_LUT(cur_p[3]);

	dest_p[2] = RED_LUT(cur_p[0]);
	dest_p[5] = RED_LUT((cur_p[0] + cur_p[2]) >> 1);
	dest_p[8] = RED_LUT(cur_p[2]);
	dest_p[11] = RED_LUT((cur_p[2] + cur_p[4]) >> 1);

	dest_p += 12;
	lst_p += 4;
	cur_p += 4;
	nxt_p += 4;

    }


}


void
convert_blue_row_16_green(u_short * lst_p, u_short * cur_p, u_short * nxt_p,
			  u_char * dest_p, int width)

{
    int     x;

    for (x = 1; x < width - 4; x += 4)
    {

	dest_p[0] = BLUE_LUT((cur_p[-1] + cur_p[1]) >> 1);
	dest_p[3] = BLUE_LUT(cur_p[1]);
	dest_p[6] = BLUE_LUT((cur_p[1] + cur_p[3]) >> 1);
	dest_p[9] = BLUE_LUT(cur_p[3]);

	dest_p[1] = GREEN_LUT(cur_p[0]);
	dest_p[4] = GREEN_LUT((cur_p[0] + cur_p[2] + lst_p[1] + nxt_p[1]) >> 2);
	dest_p[7] = GREEN_LUT(cur_p[2]);
	dest_p[10] = GREEN_LUT((cur_p[2] + cur_p[4] + lst_p[3] + nxt_p[3]) >> 2);

	dest_p[2] = RED_LUT((lst_p[0] + nxt_p[0]) >> 1);
	dest_p[5] = RED_LUT((lst_p[0] + nxt_p[0] + lst_p[2] + nxt_p[2]) >> 2);
	dest_p[8] = RED_LUT((lst_p[2] + nxt_p[2]) >> 1);
	dest_p[11] = RED_LUT((lst_p[2] + nxt_p[2] + lst_p[4] + nxt_p[4]) >> 2);

	dest_p += 12;
	lst_p += 4;
	cur_p += 4;
	nxt_p += 4;

    }

}



void
convert_blue_row_16(u_short * lst_p, u_short * cur_p, u_short * nxt_p,
		    u_char * dest_p, int width)

{
    int     x;

    for (x = 1; x < width - 4; x += 4)
    {

	dest_p[0] = BLUE_LUT(cur_p[0]);
	dest_p[3] = BLUE_LUT((cur_p[0] + cur_p[2]) >> 1);
	dest_p[9] = BLUE_LUT((cur_p[2] + cur_p[4]) >> 1);
	dest_p[6] = BLUE_LUT(cur_p[2]);
	dest_p[1] = GREEN_LUT((cur_p[-1] + cur_p[1] + lst_p[0] + nxt_p[0]) >> 2);
	dest_p[4] = GREEN_LUT(cur_p[1]);
	dest_p[7] = GREEN_LUT((cur_p[1] + cur_p[3] + lst_p[2] + nxt_p[2]) >> 2);
	dest_p[10] = GREEN_LUT(cur_p[3]);
	dest_p[2] = RED_LUT((lst_p[-1] + nxt_p[-1] + lst_p[1] + nxt_p[1]) >> 2);
	dest_p[5] = RED_LUT((lst_p[1] + nxt_p[1]) >> 1);
	dest_p[8] = RED_LUT((lst_p[1] + nxt_p[1] + lst_p[3] + nxt_p[3]) >> 2);
	dest_p[11] = RED_LUT((lst_p[3] + nxt_p[3]) >> 1);

	dest_p += 12;
	lst_p += 4;
	cur_p += 4;
	nxt_p += 4;

    }

}


int
convert_bayer_image_16_BGR(u_short * src, 
			   int width, int rows, int pitch, 
			   u_char * dest, 
			   int order, int depth)


{
    u_short *cur_p;
    u_short *nxt_p;
    u_short *lst_p;

    u_char *dest_p = dest;

    u_char red_row;
    u_char green_start;

    u_char testgreen;

    int     y;

    if (depth == 8)
	return convert_bayer_image_8_BGR((u_char *) src,
	    width,rows,pitch,dest, order);


#ifdef USE_MMX

    if (bayer_can_use_mmx())
    {
	return convert_bayer_image_BGR_mmx((u_char *) src, 
	    width, rows, pitch, 
	    dest, 0, order, depth);

    }

#endif


    if (order >= 0)
    {
	red_row = ((order & 2) != 0);
	green_start = ((order & 1) != 0);
    }
    else
    {
	red_row = bayer_red_first;
	green_start = bayer_green_first;

    }

    testgreen = green_start;

    if (bayer_current_base_size == 0)
    {

	set_bayer_parameters(depth,
	    default_rgb_scale,
	    1.0,
	    0,
	    red_row,
	    green_start);

    }

    cur_p = src + pitch + 1;
    lst_p = src + 1;
    nxt_p = cur_p + pitch;

    green_start = !green_start;		

    if (green_start)
    {
	if (red_row)
	{
	    for (y = 1; y < rows - 1; y += 2)
	    {
		convert_blue_row_16(lst_p, cur_p, nxt_p, dest_p, width);
		lst_p += pitch;
		cur_p += pitch;
		nxt_p += pitch;
		dest_p += pitch*3;
		convert_red_row_16_green(lst_p, cur_p, nxt_p, dest_p, width);
		lst_p += pitch;
		cur_p += pitch;
		nxt_p += pitch;
		dest_p += pitch*3;

	    }
	}
	else
	{
	    for (y = 1; y < rows - 1; y += 2)
	    {
		convert_red_row_16(lst_p, cur_p, nxt_p, dest_p, width);
		lst_p += pitch;
		cur_p += pitch;
		nxt_p += pitch;
		dest_p += pitch*3;
		convert_blue_row_16_green(lst_p, cur_p, nxt_p, dest_p, width);
		lst_p += pitch;
		cur_p += pitch;
		nxt_p += pitch;
		dest_p += pitch*3;
	    }
	}
    }
    else
    {
	if (red_row)
	{
	    for (y = 1; y < rows - 1; y += 2)
	    {
		convert_blue_row_16_green(lst_p, cur_p, nxt_p, dest_p, width);
		lst_p += pitch;
		cur_p += pitch;
		nxt_p += pitch;
		dest_p += pitch*3;
		convert_red_row_16(lst_p, cur_p, nxt_p, dest_p, width);
		lst_p += pitch;
		cur_p += pitch;
		nxt_p += pitch;
		dest_p += pitch*3;
	    }
	}
	else
	{
	    for (y = 1; y < rows - 1; y += 2)
	    {
		convert_red_row_16_green(lst_p, cur_p, nxt_p, dest_p, width);
		lst_p += pitch;
		cur_p += pitch;
		nxt_p += pitch;
		dest_p += pitch*3;
		convert_blue_row_16(lst_p, cur_p, nxt_p, dest_p, width);
		lst_p += pitch;
		cur_p += pitch;
		nxt_p += pitch;
		dest_p += pitch*3;
	    }

	}
    }

    return (0);
}



void
convert_red_row_8_green(u_char * lst_p,
			u_char * cur_p,
			u_char * nxt_p,
			u_char * dest_p,
			int width)

{
    u_char *end_p = cur_p + (width - 4);

    do
    {

	/* blue pixels */

	dest_p[0] = BLUE_LUT((lst_p[0] + nxt_p[0]) >> 1);
	dest_p[3] = BLUE_LUT((lst_p[0] + nxt_p[0] + lst_p[2] + nxt_p[2]) >> 2);
	dest_p[6] = BLUE_LUT((lst_p[2] + nxt_p[2]) >> 1);
	dest_p[9] = BLUE_LUT((lst_p[2] + nxt_p[2] + lst_p[4] + nxt_p[4]) >> 2);

	dest_p[1] = GREEN_LUT(cur_p[0]);
	dest_p[7] = GREEN_LUT(cur_p[2]);
	dest_p[4] = GREEN_LUT((cur_p[0] + cur_p[2] + lst_p[1] + nxt_p[1]) >> 2);
	dest_p[10] = GREEN_LUT((cur_p[2] + cur_p[4] + lst_p[3] + nxt_p[3]) >> 2);

	/* red pixels */

	dest_p[5] = RED_LUT(cur_p[1]);
	dest_p[11] = RED_LUT(cur_p[3]);
	dest_p[2] = RED_LUT((cur_p[-1] + cur_p[1]) >> 1);
	dest_p[8] = RED_LUT((cur_p[1] + cur_p[3]) >> 1);


	dest_p += 12;
	lst_p += 4;
	cur_p += 4;
	nxt_p += 4;

    } while (cur_p < end_p);

}

void
convert_red_row_8(u_char * lst_p, u_char * cur_p, u_char * nxt_p,
		  u_char * dest_p, int width)

{
    int     x;

    for (x = 1; x < width - 4; x += 4)
    {

	dest_p[0] = BLUE_LUT((lst_p[-1] + nxt_p[-1] + lst_p[1] + nxt_p[1]) >> 2);
	dest_p[3] = BLUE_LUT((lst_p[1] + nxt_p[1]) >> 1);
	dest_p[6] = BLUE_LUT((lst_p[1] + nxt_p[1] + lst_p[3] + nxt_p[3]) >> 2);
	dest_p[9] = BLUE_LUT((lst_p[3] + nxt_p[3]) >> 1);

	dest_p[1] = GREEN_LUT((cur_p[-1] + cur_p[1] + lst_p[0] + nxt_p[0]) >> 2);
	dest_p[4] = GREEN_LUT(cur_p[1]);
	dest_p[7] = GREEN_LUT((cur_p[1] + cur_p[3] + lst_p[2] + nxt_p[2]) >> 2);
	dest_p[10] = GREEN_LUT(cur_p[3]);

	dest_p[2] = RED_LUT(cur_p[0]);
	dest_p[5] = RED_LUT((cur_p[0] + cur_p[2]) >> 1);
	dest_p[8] = RED_LUT(cur_p[2]);
	dest_p[11] = RED_LUT((cur_p[2] + cur_p[4]) >> 1);

	dest_p += 12;
	lst_p += 4;
	cur_p += 4;
	nxt_p += 4;

    }


}


void
convert_blue_row_8_green(u_char * lst_p, u_char * cur_p, u_char * nxt_p,
			 u_char * dest_p, int width)

{
    int     x;

    for (x = 1; x < width - 4; x += 4)
    {

	dest_p[0] = BLUE_LUT((cur_p[-1] + cur_p[1]) >> 1);
	dest_p[3] = BLUE_LUT(cur_p[1]);
	dest_p[6] = BLUE_LUT((cur_p[1] + cur_p[3]) >> 1);
	dest_p[9] = BLUE_LUT(cur_p[3]);

	dest_p[1] = GREEN_LUT(cur_p[0]);
	dest_p[4] = GREEN_LUT((cur_p[0] + cur_p[2] + lst_p[1] + nxt_p[1]) >> 2);
	dest_p[7] = GREEN_LUT(cur_p[2]);
	dest_p[10] = GREEN_LUT((cur_p[2] + cur_p[4] + lst_p[3] + nxt_p[3]) >> 2);

	dest_p[2] = RED_LUT((lst_p[0] + nxt_p[0]) >> 1);
	dest_p[5] = RED_LUT((lst_p[0] + nxt_p[0] + lst_p[2] + nxt_p[2]) >> 2);
	dest_p[8] = RED_LUT((lst_p[2] + nxt_p[2]) >> 1);
	dest_p[11] = RED_LUT((lst_p[2] + nxt_p[2] + lst_p[4] + nxt_p[4]) >> 2);

	dest_p += 12;
	lst_p += 4;
	cur_p += 4;
	nxt_p += 4;

    }

}



void
convert_blue_row_8(u_char * lst_p, u_char * cur_p, u_char * nxt_p,
		   u_char * dest_p, int width)

{
    int     x;

    for (x = 1; x < width - 4; x += 4)
    {

	dest_p[0] = BLUE_LUT(cur_p[0]);
	dest_p[3] = BLUE_LUT((cur_p[0] + cur_p[2]) >> 1);
	dest_p[9] = BLUE_LUT((cur_p[2] + cur_p[4]) >> 1);
	dest_p[6] = BLUE_LUT(cur_p[2]);
	dest_p[1] = GREEN_LUT((cur_p[-1] + cur_p[1] + lst_p[0] + nxt_p[0]) >> 2);
	dest_p[4] = GREEN_LUT(cur_p[1]);
	dest_p[7] = GREEN_LUT((cur_p[1] + cur_p[3] + lst_p[2] + nxt_p[2]) >> 2);
	dest_p[10] = GREEN_LUT(cur_p[3]);
	dest_p[2] = RED_LUT((lst_p[-1] + nxt_p[-1] + lst_p[1] + nxt_p[1]) >> 2);
	dest_p[5] = RED_LUT((lst_p[1] + nxt_p[1]) >> 1);
	dest_p[8] = RED_LUT((lst_p[1] + nxt_p[1] + lst_p[3] + nxt_p[3]) >> 2);
	dest_p[11] = RED_LUT((lst_p[3] + nxt_p[3]) >> 1);

	dest_p += 12;
	lst_p += 4;
	cur_p += 4;
	nxt_p += 4;

    }

}

int
convert_bayer_image_8_BGR(u_char * src, 
			  int width, int rows, int pitch, 
			  u_char * dest, 
			  int order)


{
    u_char *cur_p;
    u_char *nxt_p;
    u_char *lst_p;

    u_char *dest_p = dest;

    u_char red_row;
    u_char green_start;

    u_char testgreen;

    int     y;

#ifdef USE_MMX

    if (bayer_can_use_mmx())
    {
	return convert_bayer_image_BGR_mmx((u_char *) src, 
	    width, rows, pitch, 
	    dest, 0, order, 8);

    }

#endif

    if (order >= 0)
    {
	red_row = ((order & 2) != 0);
	green_start = ((order & 1) != 0);
    }
    else
    {
	red_row = bayer_red_first;
	green_start = bayer_green_first;

    }

    testgreen = green_start;

    if (bayer_current_base_size == 0)
    {

	set_bayer_parameters(8,
	    default_rgb_scale,
	    1.0,
	    0,
	    red_row,
	    green_start);

    }


    cur_p = src + pitch + 1;
    lst_p = src + 1;
    nxt_p = cur_p + pitch;

    green_start = !green_start;		

    if (green_start)
    {
	if (red_row)
	{
	    for (y = 1; y < rows - 1; y += 2)
	    {
		convert_blue_row_8(lst_p, cur_p, nxt_p, dest_p, width);
		lst_p += pitch;
		cur_p += pitch;
		nxt_p += pitch;
		dest_p += pitch*3;
		convert_red_row_8_green(lst_p, cur_p, nxt_p, dest_p, width);
		lst_p += pitch;
		cur_p += pitch;
		nxt_p += pitch;
		dest_p += pitch*3;

	    }
	}
	else
	{
	    for (y = 1; y < rows - 1; y += 2)
	    {
		convert_red_row_8(lst_p, cur_p, nxt_p, dest_p, width);
		lst_p += pitch;
		cur_p += pitch;
		nxt_p += pitch;
		dest_p += pitch*3;
		convert_blue_row_8_green(lst_p, cur_p, nxt_p, dest_p, width);
		lst_p += pitch;
		cur_p += pitch;
		nxt_p += pitch;
		dest_p += pitch*3;
	    }
	}
    }
    else
    {
	if (red_row)
	{
	    for (y = 1; y < rows - 1; y += 2)
	    {
		convert_blue_row_8_green(lst_p, cur_p, nxt_p, dest_p, width);
		lst_p += pitch;
		cur_p += pitch;
		nxt_p += pitch;
		dest_p += pitch*3;
		convert_red_row_8(lst_p, cur_p, nxt_p, dest_p, width);
		lst_p += pitch;
		cur_p += pitch;
		nxt_p += pitch;
		dest_p += pitch*3;
	    }
	}
	else
	{
	    for (y = 1; y < rows - 1; y += 2)
	    {
		convert_red_row_8_green(lst_p, cur_p, nxt_p, dest_p, width);
		lst_p += pitch;
		cur_p += pitch;
		nxt_p += pitch;
		dest_p += pitch*3;
		convert_blue_row_8(lst_p, cur_p, nxt_p, dest_p, width);
		lst_p += pitch;
		cur_p += pitch;
		nxt_p += pitch;
		dest_p += pitch*3;
	    }

	}
    }


    return (0);
}

