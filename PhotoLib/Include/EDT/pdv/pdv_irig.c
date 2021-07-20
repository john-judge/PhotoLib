/* version 1 of dma_frame count control */
#include "edtinc.h"

#include "pdv_irig.h"

#define IRIG_REGISTER(pdv_p,reg) \
    ((pdv_p->channel_no == 0)?(reg):(reg)-0x40)
int
pdv_reset_dma_framecount(PdvDev *pdv_p)

{
    u_int util;

    util = edt_reg_read(pdv_p, PDV_UTILITY);
    edt_reg_write(pdv_p, PDV_UTILITY, util & ~0xC0);
    edt_reg_write(pdv_p, PDV_UTILITY, util);
    return 0;
}

pdv_irig_set_offset(PdvDev *pdv_p, int offset)
{
    edt_set_timecode_seconds_offset(pdv_p,offset);    
    pdv_p->dd_p->irig_offset = offset;
    return 0;
}

int
pdv_irig_set_bcd(PdvDev *pdv_p, int bcd)

{

    if (pdv_p->spi_reg_base == 0)
        pdv_p->spi_reg_base = IRIG_REGISTER(pdv_p,PDV_IRIG_SPI_BASE);

    edt_set_timecode_raw(pdv_p, bcd);             
    pdv_p->dd_p->irig_raw = bcd;
    return 0;
}

Irig2Record *
pdv_irig_get_footer(PdvDev *pdv_p, u_char * imagedata)

{
    Irig2Record *footer;

    int offset = pdv_get_header_offset(pdv_p);

    footer = (Irig2Record *) (imagedata + offset);

    return footer;

}

double pdv_irig_process_time(Irig2Record * footer)

{
    footer->timestamp = footer->t.seconds + (double) footer->clocks/(double)footer->tickspps;
    return footer->timestamp;
}

void pdv_irig_reset_errors(PdvDev *pdv_p)

{
	edt_reg_write(pdv_p, IRIG_REGISTER(pdv_p,IRIG2_FIFO), IRIG2_RESET_ERRS);
	edt_reg_write(pdv_p, IRIG_REGISTER(pdv_p,IRIG2_FIFO), 0);

}

void pdv_irig_set_slave(PdvDev *pdv_p, int onoff)

{
    u_char mask = edt_reg_read(pdv_p, IRIG_REGISTER(pdv_p,IRIG2_CTRL));

	mask &= ~IRIG2_SLAVE;

	mask |= (onoff)?IRIG2_SLAVE:0;

	edt_reg_write(pdv_p, IRIG_REGISTER(pdv_p,IRIG2_CTRL), mask);

        pdv_p->dd_p->irig_slave = onoff;
}

int pdv_irig_is_slave(PdvDev *pdv_p)

{
	return (edt_reg_read(pdv_p, IRIG_REGISTER(pdv_p,IRIG2_CTRL)) & IRIG2_SLAVE)?1:0;
}

int pdv_irig_get_current(PdvDev *pdv_p, Irig2Record *footer)

{
	int i;
	u_char data[16];
	u_int *wp = (u_int *) data;

	u_char mask = edt_reg_read(pdv_p, IRIG_REGISTER(pdv_p,IRIG2_CTRL)) & 0x80;
	edt_reg_write(pdv_p, IRIG_REGISTER(pdv_p,IRIG2_READWRITE), 0);

	for (i=15 ; i>= 0 ; i--)
	{
		edt_reg_write(pdv_p, IRIG_REGISTER(pdv_p,IRIG2_CTRL), mask | i);
		data[i] = edt_reg_read(pdv_p, IRIG_REGISTER(pdv_p,IRIG2_READWRITE));
	}

	footer->t.seconds = wp[2];
	footer->clocks = wp[3];
	footer->tickspps = wp[1];
	memcpy(&footer->status, &data[3], 1);

	return 0;
	
}
