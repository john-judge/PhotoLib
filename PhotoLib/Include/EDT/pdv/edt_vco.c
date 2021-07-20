/* #pragma ident "@(#)edt_vco.c	1.11 11/13/00 EDT" */

/*
 * edt_vco.c
 * 
 * library routine to find settings for VCO frequencies for the AV9110 as used
 * on the PCI CD20/40/60.
 */

#include <math.h>
#include "edtinc.h"

#include "edt_vco.h"
#include <assert.h>

static int loops = 0;

typedef struct _target
{
    double  target;
    double  actual;

    edt_pll *pll;

    int     hits;
    int     finished;
}target_struct;

#define freq_calc(n,m,v,r,h,l,x,xtal) ((n * v * xtal) / (m * r * h * l * x * 2))


/* all possible factorings */

static int factors[64 * 256 + 1];

static int factorsfilled = 0;

static void 
fill_factors()

{
    int     l;
    int     x;
    int     product;
    int     lshift;

    /* store factors as l << 16 + x */

    memset(&factors[0], 0, sizeof(factors));

    for (l = 1; l <= 64; l++)
    {
	product = l;
	lshift = (l << 16);

	for (x = 1; x <= 256; x++)
	{

	    factors[product] = lshift | x;

	    product += l;
	}
    }

    factorsfilled = 1;
}


/*
 * foreach possible setting of the L (1 to 64) and X (1 to 256) dividers
 * check if the output frequency is closer to the target than the last
 * closest. Notice the last divide by 2 is fixed for clock symetry. note that
 * the F_LOW restriction only applies when either X or L is greater than 1.
 */

static void
div_out(int h, int r, int n, int v, int m,
        double  xtal, 
        double  vco, 
        double  ref, 
        double  av_out,
        target_struct *targ)
{
    int     val;
    double  oddclkout;
    double  serialclk;
    double  xl;
    int     minx = 1;
    int     maxx = 64 * 256;



    oddclkout = av_out / h;

    /* target xl value */

    xl = oddclkout / (targ->target * 2);

    minx = (int) xl - 1;
    if (minx < 1)
	minx = 1;
    maxx = (int) xl + 1;

    if (maxx > 64 * 256)
	maxx = 64 * 256;

    for (val = minx; val <= maxx; val++)
    {

	if (val == 1 || ((oddclkout <= F_LOW) && factors[val]))

	{
	    serialclk = oddclkout / (val * 2);

	    /*
	     * -debug printf("vco = %3.5f av9110 out = %3.5f, odd divide out
	     * = %3.5f\n", vco, av_out, oddclkout); printf("serial clk =
	     * %3.5f (m=%d,  n=%d, v=%d, r=%d, h=%d, l=%d, x=%d)\n" ,
	     * serialclk, m, n, v, r, h, l, x);
	     */
	    if ((targ->hits == 0) ||
		(fabs(targ->actual - targ->target) > fabs(serialclk - targ->target)))
	    {
		/* update the target structure */
		targ->actual = serialclk;
		targ->pll->m = m;
		targ->pll->n = n;
		targ->pll->v = v;
		targ->pll->r = r;
		targ->pll->h = h;

		targ->pll->l = (factors[val] >> 16);
		targ->pll->x = (factors[val] & 0xffff);

		targ->hits++;

		if (targ->actual == targ->target)
		    targ->finished = 1;

#if 0
		printf("vco = %3.5f av9110 out = %3.5f, odd divide out = %3.5f\n",
		       vco, av_out, oddclkout);
		printf("serial clk = %3.5f val = %d (%d,  n=%d, v=%d, r=%d, h=%d, l=%d, x=%d)\n"
		,serialclk, val, m, n, v, r, h, targ->pll->l, targ->pll->x);
#endif

	    }
	}

	loops++;

    }

}

/*
 * check all av9110 output frequencies for legality to enter the xilinx. If
 * ok call all possible high speed odd dividers
 */

static void
av_outputs(int r, int n, int v, int m,
        double  xtal, 
        double  vco, 
        double  ref, 
        target_struct *targ)
            

{
    double  av_out = vco / r;

    if ((av_out) <= F_XILINX)
    {

	/*
	 * for all possible divsors of the high speed odd divider in the
	 * xilinx
	 */

	div_out(1, r, n, v, m, xtal, vco, ref, av_out, targ);
	if (targ->finished)
	    return;

	div_out(3, r, n, v, m, xtal, vco, ref, av_out, targ);
	if (targ->finished)
	    return;

	div_out(5, r, n, v, m, xtal, vco, ref, av_out, targ);
	if (targ->finished)
	    return;

	div_out(7, r, n, v, m, xtal, vco, ref, av_out, targ);
    }
}


/*
 * do all n to find legal vco frequencies for given ref and v
 */
static void
all_n(int v, int m,
        double  xtal, 
        double  ref, 
        target_struct *targ)
{
    int     n = 3;
    double  vco;
    double  vref = v * ref;


    vco = vref * 3;
    while ((n < 128) && ((vco) <= HI_VCO))
    {
	if ((vco) >= LOW_VCO)
	{
	    /*
	     * for each legal vco freq check all values of r
	     */
	    /*
	     * -debug printf("vco = %3.5f (m=%d,  n=%d, v=%d)\n", vco, m, n,
	     * v);
	     */


	    av_outputs(1, n, v, m, xtal, vco, ref, targ);
	    if (targ->finished)
		return;
	    av_outputs(2, n, v, m, xtal, vco, ref, targ);
	    if (targ->finished)
		return;
	    av_outputs(4, n, v, m, xtal, vco, ref, targ);
	    if (targ->finished)
		return;
	    /* canceled out if v == 8 */
	    /* same as v == 1, r == 1 */
	    if (v != 8)
		av_outputs(8, n, v, m, xtal, vco, ref, targ);
	    if (targ->finished)
		return;
	}
	n++;
	vco += vref;
    }
}

/*
 * print all possible vco frequencies for the refernce frequency passed
 * (xtal/m).
 */

static void
vcofreq( int m, double  xtal, target_struct *targ)

{
    double  ref;

    /*
     * call n rates for v is 1 and 8
     */
    ref = xtal / m;
    all_n(1, m, xtal, ref, targ);
    if (targ->finished)
	return;
    all_n(8, m, xtal, ref, targ);
}

double
edt_find_vco_frequency(EdtDev * edt_p, double target,
		       double xtal, edt_pll * pll, int verbose)

{

    target_struct targ;
    edt_pll dummypll;

    int     m, minm, maxm;


    if (!pll)
	pll = &dummypll;

    edt_dtime();

    if (!factorsfilled)
    {
	fill_factors();
	factorsfilled = 1;
    }

    if (xtal == 0)
    {
	switch (edt_p->devid)
	{
	case PCD20_ID:
	    xtal = XTAL20;
	    break;
	case PDV_ID:		/* probably never do output from dv (and family) */
	case PDVK_ID:		/* but sticking in here just in case */
	case PDV44_ID:
	case PDVFOI_ID:
	case PCD40_ID:
	    xtal = XTAL40;
	    break;
	case PCD60_ID:
	case PGP_ECL_ID:
	case PCD_16_ID:
	case PSS16_ID:
	    xtal = XTAL60;
	    break;
	default:
	    xtal = XTAL20;
	    fprintf(stderr, "WARNING -- vco set got invalid device ID (%d) -- using 20...?\n", edt_p->devid);

	    break;

	}
    }

    if ((minm = (int) (xtal / HI_REF)) < 3)
	minm = 3;
    if ((maxm = (int) (xtal / LOW_REF)) > 127)
	maxm = 127;

    targ.hits = 0;		/* zero means no updates have been made */
    targ.finished = 0;
    targ.target = target;

    targ.pll = pll;

    for (m = minm; m <= maxm; m++)
    {

	vcofreq(m, xtal, &targ);

	if (targ.finished)
	    break;
    }

    if (verbose)
    {
	printf("\nClosest frequency to target frequency %3.5f is %3.5f\n",
	       targ.target, targ.actual);
	printf("N=%d, M=%d, V=%d, R=%d, H=%d, L=%d, X=%d, updates=%d loops = %d\n",
	       pll->n, pll->m, pll->v, pll->r, pll->h, pll->l, pll->x,
	       targ.hits, loops);
	printf("Elapsed: %10.6f seconds\n", edt_dtime());
    }

    return targ.actual;

}

/*
 * set phase lock loop clock
 */
void
edt_set_pll_clock(EdtDev * edt_p, int ref_xtal, edt_pll * clkset, int verbose)
{
    double  freq = 0;
    double  vco = 0;

    /* set output clock */
    if (verbose)
    {
	printf("setting pll clock\n");
	printf("clock parameters - \n");
	printf("\tAV9110 refernce divide(-M) = %d\n", clkset->m);
	printf("\tAV9110 VCO feedback divide(-N) = %d\n", clkset->n);
	printf("\tAV9110 VCO feedback prescale divide(-V) = %d\n", clkset->v);
	printf("\tAV9110 VCO output  divide(-R) = %d\n", clkset->r);
	printf("\tXilinx low speed divide(-L)  = %d\n", clkset->l);
	printf("\tXilinx odd divide(-H) = %d\n", clkset->h);
	printf("\tXilinx tclk prescaler(-X) = %d\n", clkset->x);
    }
    vco = (ref_xtal * clkset->v * clkset->n) / (clkset->m);
    if (vco < 45e+06 || vco > 250e+06)
	printf("edt_set_pll_clock: VCO FREQUENCY OUT OF RANGE at %g Hz\n", vco);
    else if (verbose)
	printf("VCO Frequency at %g Hz\n", vco);
    freq = vco / (2 * clkset->r * clkset->x * clkset->h * clkset->l);
    if (verbose)
	printf("setting write unit output clock %g Hertz\n", freq);
    edt_set_out_clk(edt_p, clkset);
    edt_set_funct_bit(edt_p, EDT_FUNCT_PLLCLK);
}

