/* #pragma ident "@(#)set_ss_vco.c	1.17 01/17/05 EDT" */

#include <stdio.h>
#include <math.h>

#include <string.h>

#include <stdlib.h>

#include "edtinc.h"

#include <stddef.h>

#include "edt_si570.h"


EdtRegisterDescriptor si570regs_map[] = {
    {7,5,3,"hs_div"},
    {7,6,7,"n1"},
    {8,4,10,"rfreq_int"},
    {9,0,28,"rfreq_frac"},
    {135,7,1,"reset"},
    {135,6,1,"newfreq"},
    {135,0,1,"recall"},
    {137,4,1,"freeze"},
    {0,0,0,0}
};

int
edt_set_out_clk_si570(EdtDev *edt_p, EdtSI570 *sip)

{
    return 0;
}


/* return valid bwsel */
/* return 0 if not OK */


void
edt_si570_read_values(EdtDev *edt_p, 
		      u_int base_desc, 
		      u_int device, 
		      EdtSI570 *sip)

{
    sip->hs_div = edt_get_two_wire_value(edt_p, 
	si570regs_map,
	base_desc, 
	device);

    sip->n1 = edt_get_two_wire_value(edt_p,
	si570regs_map+1,
	base_desc, 
	device);

    sip->rfreq_hi = edt_get_two_wire_value(edt_p,
	si570regs_map+2,
	base_desc, 
	device);

    sip->rfreq_fraction = edt_get_two_wire_value(edt_p,
	si570regs_map+3,
	base_desc, 
	device);
}

void
edt_si570_reset(EdtDev *edt_p, u_int base_desc, u_int device)

{
    int r;
    int t;
    
    edt_set_two_wire_value(edt_p, si570regs_map + 4, base_desc, device, 1);

    t = 0;
    while (r = edt_get_two_wire_value(edt_p, si570regs_map+4, base_desc, device))
    {
	edt_msleep(50);
	t += 50;
    }

    edt_msleep(100);
}

void
edt_si570_write_values(EdtDev *edt_p, 
		       u_int base_desc, 
		       u_int device, 
		       EdtSI570 *sip)

{
    /* freeze */
    edt_set_two_wire_value(edt_p, si570regs_map + 7, base_desc, device, 1);

    edt_set_two_wire_value(edt_p, si570regs_map, base_desc, device, sip->hs_div);
    edt_set_two_wire_value(edt_p, si570regs_map+1, base_desc, device, sip->n1);
    edt_set_two_wire_value(edt_p, si570regs_map+2, base_desc, device, sip->rfreq_hi);
    edt_set_two_wire_value(edt_p, si570regs_map+3, base_desc, device, sip->rfreq_fraction);

    /* unfreeze */
    edt_set_two_wire_value(edt_p, si570regs_map + 7, base_desc, device, 0);
    /* assert new freq */
    edt_set_two_wire_value(edt_p, si570regs_map + 5, base_desc, device, 1);
   
}

int
edt_si570_dump(EdtDev *edt_p, u_int base_desc, u_int device)

{
    edt_two_wire_reg_dump_raw(edt_p, si570regs_map, base_desc, device);
    edt_two_wire_reg_dump(edt_p, si570regs_map, base_desc, device);
    return 0;
}

double
edt_si570_rfreq(u_int freq_hi, u_int freq_fraction)

{
    double scalar = (double) (1 << 28);

    return (double) freq_hi + ((double) freq_fraction)/scalar;

}

double edt_si570_fxtal(EdtSI570 *sip, double refclock)

{
    double rfreq = edt_si570_rfreq(sip->rfreq_hi,sip->rfreq_fraction);

    return ((sip->n1 + 1) * (sip->hs_div + 4) * refclock) / rfreq;

}

double edt_si570_current_freq(EdtSI570 *sip, double fxtal)

{
    double rfreq = edt_si570_rfreq(sip->rfreq_hi,sip->rfreq_fraction);
    double output;

    output = (rfreq * fxtal) / ((sip->hs_div + 4) * (sip->n1 + 1));

    return output;
}


void 
edt_si570_print(EdtSI570 *sip, double refclock)
{
    double rfreq = edt_si570_rfreq(sip->rfreq_hi,sip->rfreq_fraction);

    printf("n1       = %d\nhs_div    = %d\nrfreq_int = %d\nfraction  = %08x\nrfreq   = %f\n",
	sip->n1,
	sip->hs_div,
	sip->rfreq_hi,
	sip->rfreq_fraction,
	rfreq);

    printf("\nF1 = %f\n",
	((sip->n1 + 1) * (sip->hs_div + 4) * refclock) / rfreq);


}

#define foscmin 4850000000.0
#define foscmax 5670000000.0

#define N_HS_DIV_VALUES 6

static int hs_divs[N_HS_DIV_VALUES] = 
{
    4,
    5,
    6,
    7,
    9,
    11
};

double edt_vco_si570_compute(double xtal, double target, EdtSI570 *parms)

{
    double actual = target;
    double fxtal = edt_si570_fxtal(parms, xtal);
    int best_hdiv = -1;
    int best_n1;
    double best_fosc;
    int i;
    int n1;
    double fosc;
    double rfreq;

    for (i=0;i<N_HS_DIV_VALUES;i++)
    {
	int hs_div = hs_divs[i];

	for (n1=0;n1<128;n1++)
	{
	    if (n1==1 || (n1 & 1) == 0)
	    {
		fosc = target * hs_div * n1;

		if (fosc >= foscmin && fosc <= foscmax)
		{
		    if (best_hdiv == -1 || (fosc < best_fosc) || (fosc == best_fosc && n1 < best_n1) )
		    {
			best_hdiv = hs_div;
			best_n1   = n1;
			best_fosc = fosc;
			edt_msg(EDTLIB_MSG_INFO_1, "Selected fosc = %10f n1 = %d hdiv = %d\n",
			    best_fosc,best_n1,best_hdiv);
		    } 
		    else
			edt_msg(EDTLIB_MSG_INFO_1,"Rejected fosc = %10f n1 = %d hdiv = %d\n",
			    fosc,n1,hs_div);
		}
	    }
	}
    }

    rfreq = best_fosc / fxtal;

    edt_msg(EDTLIB_MSG_INFO_1,"new rfreq %f\n", rfreq);

    parms->hs_div = best_hdiv - 4;
    parms->n1 = best_n1 - 1;
    parms->rfreq_fraction = (u_int)((rfreq - floor(rfreq)) * (double) (1 << 28)) ;
    parms->rfreq_hi = (u_int) (floor(rfreq));

    return actual;

}

int
edt_si570_set_clock(EdtDev *edt_p, u_int base_desc, u_int device,
		    double nominal, double target, EdtSI570 *parms)

{

    EdtSI570 *sip;
    EdtSI570 localparms;

    if (base_desc < 0x100)
	base_desc |= 0x01010000;

    if (parms)
	sip = parms;
    else
	sip = &localparms;

    edt_si570_reset(edt_p, base_desc, device);

    edt_si570_read_values(edt_p, base_desc, device, sip);

    edt_vco_si570_compute(nominal, target, sip);

    edt_si570_write_values(edt_p, base_desc, device, sip);

    return 0;
}

/* somewhat complicated method to read current output - without making any
   assumption aboout calibrated value */

double 
edt_si570_get_clock(EdtDev *edt_p, u_int base_desc, u_int device, double nominal, 
		    EdtSI570 *parms)

{
    EdtSI570 *sip;
    EdtSI570 localparms;
    EdtSI570 storeparms;
    double fxtal;
    double output;

    if (parms)
	sip = parms;
    else
	sip = &storeparms;


    edt_si570_read_values(edt_p, base_desc, device, sip);


    /* return to factory state */

    edt_si570_reset(edt_p, base_desc, device);

    edt_si570_read_values(edt_p, base_desc, device, &localparms);

    /* compute the correct internal clock */

    fxtal = edt_si570_fxtal(&localparms, nominal);

    output = edt_si570_current_freq(sip,  fxtal);

    edt_si570_write_values(edt_p, base_desc, device, sip);

    return output;
}





