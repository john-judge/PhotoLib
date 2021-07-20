/* #pragma ident "@(#)lib_two_wire.c	1.13 11/04/10 EDT" */

#include <stdio.h>
#include <math.h>

#include <string.h>

#include <stdlib.h>

/*#define DEBUG_CALLS */
#define DO_DUMP_CHANGES
#include "edtinc.h"

#include "edt_vco.h"
#include "edt_ss_vco.h"

#define EDTFATAL EDTLIB_MSG_FATAL
#define EDTWARN EDTLIB_MSG_WARNING

/* The Finisar sometimes keeps SDA high for a 
while - the routine edt_two_wire_wait_sda_hi loops 
until this goes low */

#define MAX_SDA_HI_RETRIES 100
int max_sda_hi_retries_found = 0;

/*
* Library to read a 2 wire protocol device
* usch as a serial eeprom or a diagnostic port on
* a serial trasciever.
*
* the port must be implemented as a register address
* like the PCISS/GS OCM
* bit one is the two wire clock, it is always an output
* bit 2 is the two wire serial data out, it is enable onto the serial data when
* bit 3 enables bit 2 onto the serial data bus when low
* bit 4 reads back the stat of the serial data wire, it should follow bit 2 when bit 3 is on
*
* The ocm has three 2 wire device addresses:
* 1) channel 0 SFP tranciever at 0x23
* 2) channel 1 SFP trancsiver at 0x33
* 3) serial eeprom calibration  - currently unimplemented
*
* some two wire devices have 1 byte address others 2 etc the 
* address_length field designates number of bytes
*
* the page write size varies from device to device setting to 1
* will work on anything but writes will be slow 
*/

#include "lib_two_wire.h"
/*
* init known two wire interfaces
*/

#include "edt_ocm.h"

EdtTwoWire two_wire_table[] = {
    { OCM_CH0_TRANSCEIVER, OCM_CH0_TRANSCEIVER, XCVR_SCL, XCVR_TS, XCVR_WDATA, XCVR_RDATA, 0, 0, 0, 1, 8},
    { OCM_CH1_TRANSCEIVER, OCM_CH1_TRANSCEIVER, XCVR_SCL, XCVR_TS, XCVR_WDATA, XCVR_RDATA, 0, 0, 0, 1, 8},
    /* sfp on oc192 is the same as ocm channel 0 */
    /* xfp on oc192 has clock stretching (tristate and readback on clock) */
    { OC192_CH0_TWO_WIRE_COM, OC192_CH0_TWO_WIRE_COM, XCVR_SCL, XCVR_TS, XCVR_WDATA, XCVR_RDATA, 0, 0, 0, 1, 8},
    { OC192_CH1_TWO_WIRE_COM, OC192_CH1_TWO_WIRE_COM, OC192_SCL, OC192_TS, OC192_WDATA, OC192_RDATA,
    OC192_SCL_TS, OC192_RCLK, TWO_WIRE_CLK_STRETCH, 1, 8}
};

/* 
* edt_open_two_wire is a routine to make the two wire 
* generic. Pass in the EdtDev pointer for the board and
* and define constant for the EDT board, mezzanine and bitfile.
* returns a ponter to a edt_two_wire structure which contains the relavant data
* for further calls.
*/
EdtTwoWire *
edt_open_two_wire(EdtDev *edt_p, int interface_id)
{
    int table_size;

    if (interface_id > (table_size = sizeof (two_wire_table) / sizeof(EdtTwoWire)))
    {
        return(NULL);
    }
    return(&two_wire_table[interface_id]);
} 



/*
* change the bits in a register 
* the bits set to 1 in the set mask are set after the bits in the clr mask are reset,
* then the new byte is written. There is only on write.
* the byte is read to insure the write has happened
*/
void
edt_two_wire_chg_bit(EdtDev * edt_p, u_int reg_add, u_int clr_mask, u_int set_mask)
{
    unsigned char obyte, sbyte, rbyte;

    obyte = edt_reg_read(edt_p, reg_add);
    sbyte = obyte;
    sbyte &= ~((unsigned char)clr_mask);
    sbyte |= (unsigned char)set_mask;
    edt_reg_write(edt_p, reg_add, sbyte);
    rbyte = edt_reg_read(edt_p, reg_add);

}


/*
* stop and then start the two wire interface
*/
void
edt_two_wire_start(EdtDev * edt_p, EdtTwoWire *tw_ptr)
{
    /* assume reset has been run, scl is enabled and low */
    FENTER("two_wire_start");
    /* set data high */
    edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, 0, tw_ptr->sda_out_mask);

    /* set data tristate on */
    edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, tw_ptr->tri_state_mask , 0);

    /* set clock high */
    edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, 0, tw_ptr->sclk_mask);

    /* data low for a start */
    edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, tw_ptr->sda_out_mask, 0);

    /* then clock low */
    edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, tw_ptr->sclk_mask, 0);

    FRETURN("two_wire_start");
}

/*
* the clock routine will cause clock hi. If 
* clock stretching is true the the clock line is tristate and
* waits for the clock to go high
*/

void
edt_two_wire_clock_hi(EdtDev * edt_p, EdtTwoWire *tw_ptr)
{
    unsigned char stat = 0;
    int n = 0;
    FENTER("two_wire_clock_hi");

    /*
    * if clock tri-state is implemented we must check for clock stretching.
    * we tristate the clock and wait for it to go high.
    * then we drive it high.
    */



    if ((tw_ptr->options & TWO_WIRE_CLK_STRETCH) != 0)
    {
        edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, 0, tw_ptr->scl_tri_state_mask);
        /* read status byte to check SCL in state */
        while ( ((stat = edt_reg_read(edt_p, tw_ptr->read_address)) & tw_ptr->scl_in_mask) == 0)
            n++;

    }
    /* set clock high  */
    edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, 0, tw_ptr->sclk_mask);
    /* tristate back on if turned off above */
    if ((tw_ptr->options & TWO_WIRE_CLK_STRETCH) != 0)
        edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, tw_ptr->scl_tri_state_mask, 0);

    FRETURN("two_wire_clock_hi");
}


/*
* this clock routine will cause clock low.
*/

void
edt_two_wire_clock_low(EdtDev * edt_p, EdtTwoWire *tw_ptr)
{
    FENTER("two_wire_clock_low");

    /* make sure tristate is on */
    if ((tw_ptr->options & TWO_WIRE_CLK_STRETCH) != 0)
        edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, tw_ptr->scl_tri_state_mask, 0);
    /* clock low  must tristate sda when clock goes low or xfp does not ack */
    edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, tw_ptr->sclk_mask, tw_ptr->tri_state_mask);
    FRETURN("two_wire_clock_low");
}

/*
* clock in a serial data bit to the eeprom
* clock is low  sda tristate on entry
*/

void
edt_two_wire_wr_databit(EdtDev * edt_p, EdtTwoWire *tw_ptr, int bit)
{
    FENTER_I("two_wire_wr_databit",bit);
    if (bit == 0)
        edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, tw_ptr->sda_out_mask, 0);
    else
        edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, 0, tw_ptr->sda_out_mask);

    /*  turn sda tristate on */
    edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, tw_ptr->tri_state_mask , 0);

    edt_two_wire_clock_hi(edt_p, tw_ptr);
    edt_two_wire_clock_low(edt_p, tw_ptr);

    /*  turn sda tristate off */
    edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, 0, tw_ptr->tri_state_mask);
    FRETURN("two_wire_wr_databit");
}

/* 
* The Finisar XFP module seems to drive the SDA signal after the
* Xilinx drives the clock low following the NACK cycle that signals
* the end of a read from the slave device. The slave obviously drives
* SDA during the read but during the 9th cycle of every read the
* SDA is not driven so the controller (the Xilinx) can drive
* it low (for ack to signal continued read) or high (for nack to signal
* end of the read). The Finisar part then drives the SDA for about 15us
* when the controller expects the SDA to be free to signal
* a stop or a restart. This routine is added to keep sampling the
* SDA until it is allowed to go high.
*/
int
edt_two_wire_wait_sda_hi( EdtDev * edt_p, EdtTwoWire *tw_ptr)
{
    unsigned char stat = 0;
    int retry = 0;

    FENTER("two_wire_wait_sda_hi");

    while ((((stat = edt_reg_read(edt_p, tw_ptr->read_address)) & tw_ptr->sda_in_mask) ==0) && (retry <= MAX_SDA_HI_RETRIES) )
    {
        retry++;
        /* pulse the SDA high in case the wire is floating or slow going high */
        edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, tw_ptr->tri_state_mask, tw_ptr->sda_out_mask);
        edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, 0, tw_ptr->tri_state_mask);
    }

    if (retry > max_sda_hi_retries_found)
        max_sda_hi_retries_found = retry;

    FRETURN_I("two_wire_wait_sda_hi", retry);
    return(retry);
}

/*
* stop
* assume clock is low, data can be either
*/
void
edt_two_wire_stop(EdtDev * edt_p, EdtTwoWire *tw_ptr)
{
    FENTER("two_wire_stop");
    /* data low  and tristate on */
    /* set data low  and tristate on */
    edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, tw_ptr->tri_state_mask | tw_ptr->sda_out_mask, 0);

    /* set clock high */
    edt_two_wire_clock_hi(edt_p, tw_ptr);

    /* data high for the stop */
    edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, 0, tw_ptr->sda_out_mask);

    /* set clock low */
    edt_two_wire_clock_low(edt_p, tw_ptr);

    /*  turn sda tristate off  */
    edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, 0, tw_ptr->tri_state_mask);
    FRETURN("two_wire_stop");
}

/*
* check ack returns TRUE if ack is asserted, FALSE otherwise
* assumes clock is low and data is as last set
*/
int
edt_two_wire_check_ack(EdtDev * edt_p, EdtTwoWire * tw_ptr)
{
    unsigned char stat = 0;
    int rc;

    FENTER("two_wire_check_ack");
    /* sda should already be tristate */

    /* pulse sda high to pullup a floating wire */
    edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, tw_ptr->tri_state_mask, tw_ptr->sda_out_mask);
    edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, 0, tw_ptr->tri_state_mask);

    /* set clock high */
    edt_two_wire_clock_hi(edt_p, tw_ptr);

    /* read status byte to check SDA state */
    stat = edt_reg_read(edt_p, tw_ptr->read_address);

    /* set clock low */
    edt_two_wire_clock_low(edt_p, tw_ptr);
    if ( (stat & tw_ptr->sda_in_mask) == 0)
    {
        rc = (TRUE);
    }
    else
    {
        rc = (FALSE);
    }
    FRETURN_I("two_wire_check_ack",rc);

    return rc;

}

/*
* read byte returns  next 8 bits read from the EEPROM
* assumes clock is low and data is as last set
*/
unsigned char
edt_two_wire_read_8bits(EdtDev * edt_p, EdtTwoWire * tw_ptr)
{
    unsigned char stat = 0;
    unsigned char ret = 0;
    int loop;

    FENTER("two_wire_read_8bits");

    /* already tristate */

    for(loop = 0; loop !=8; loop++)
    {
        ret = ret << 1;

        /* pulse sda high to pullup a floating wire */
        edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, tw_ptr->tri_state_mask, tw_ptr->sda_out_mask);
        edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, 0, tw_ptr->tri_state_mask);

        /* set clock high */
        edt_two_wire_clock_hi(edt_p, tw_ptr);

        /* read status byte to check SDA state */
        stat = edt_reg_read(edt_p, tw_ptr->read_address);
        if ( (stat & tw_ptr->sda_in_mask) != 0)
            ret |= 0x1;

        /* set clock low */
        edt_two_wire_clock_low(edt_p, tw_ptr);
    }

    FRETURN_I("two_wire_read_8bits",ret);

    return(ret);
}

/*
* write a byte to the two wire interface
*/

void
edt_two_wire_write_8bits(EdtDev * edt_p, EdtTwoWire * tw_ptr, unsigned char byte)
{
    int loop;

    FENTER_I("two_wire_write_8bits", byte);


    for(loop = 0; loop !=8; loop++)
    {
        if ( (byte & 0x80) != 0)
            edt_two_wire_wr_databit(edt_p, tw_ptr, 1);
        else
            edt_two_wire_wr_databit(edt_p, tw_ptr, 0);
        byte = byte << 1;
    }


    FRETURN("two_wire_write_8bits");
}

/*
* now the higher level commands
*/

/*
* reset interface
* send up to 9 sclks while reading sda until sda is high
* then enable sda and bring is low to make a start
* return a zero if successful.
*/ 
int
edt_two_wire_reset(EdtDev * edt_p, EdtTwoWire * tw_ptr)
{
    unsigned char stat = 0;
    int loop;
    int rc;

    FENTER("two_wire_reset");

    /* if scl has a tristate enable then enable it */
    if ((tw_ptr->options & TWO_WIRE_CLK_STRETCH) != 0)
        edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, 0, tw_ptr->scl_tri_state_mask);

    /* tristate fpga sda output */
    edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, 0, tw_ptr->tri_state_mask);

    loop = 0;
    while( (loop < 9) && (stat == 0) )
    {
        /* set clock high */
        edt_two_wire_clock_hi(edt_p, tw_ptr);

        /* set clock low */
        edt_two_wire_clock_low(edt_p, tw_ptr);

        /* pulse sda high to pullup a floating wire */
        edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, tw_ptr->tri_state_mask, tw_ptr->sda_out_mask);
        edt_two_wire_chg_bit(edt_p, tw_ptr->write_address, 0, tw_ptr->tri_state_mask);

        /* read status byte to check SDA state */
        stat = (edt_reg_read(edt_p, tw_ptr->read_address)) & tw_ptr->sda_in_mask;

        loop++;
    }

    if (stat == 0) 
    {
        /* reset failed */
        rc = (1);
    }
    else
    {
        rc = (0);
    }

    edt_two_wire_stop(edt_p, tw_ptr);
    FRETURN_I("two_wire_reset",rc);

    return rc;
}

/*
*  read
* requires device address (7 bits)
* address to read from (up to 32 bits)
* length of the address in bytes (1 to 4)
* number of bytes to read (u_int) 
* an char pointer to storage for the bytes read
* returns the length read (0 if fails)
*/ 
u_int
edt_two_wire_read(EdtDev * edt_p, EdtTwoWire * tw_ptr, char device, u_int address, u_int len_data, unsigned char *data)
{
    u_int ret = 0;
    u_int error = 0;
    u_int loop = 0;

    FENTER_I_I("two_wire_read",address,len_data);
    /* first do a dummy write to set the internal read pointer */
    /* a dummy write is a byte write with a stop or another start after the address */
    edt_two_wire_start(edt_p, tw_ptr);
    edt_two_wire_write_8bits(edt_p, tw_ptr, (device << 1) | 0x0); /* address write */
    if (edt_two_wire_check_ack(edt_p, tw_ptr) == FALSE)
    {
        edt_msg(EDTLIB_MSG_FATAL,"read: dummy write failed ack\n");
        FRETURN_I("two_wire_read",0);
        return(0);
    }else{
    }

    while((loop != tw_ptr->address_length) && (error==0))
    {
        edt_two_wire_write_8bits(edt_p, tw_ptr, address << (loop*8) );
        if (edt_two_wire_check_ack(edt_p, tw_ptr) == FALSE) 
        {
            edt_msg(EDTLIB_MSG_FATAL,"read: write address failed ack\n");
            error = 1;
        }
        loop++;
    }
    edt_two_wire_stop(edt_p, tw_ptr);

    if (error) 
    {
        FRETURN_I("two_wire_read",0);
        return(0);  /* return 0 bytes as an error */
    }
    /* now the read */
    edt_two_wire_start(edt_p, tw_ptr);
    edt_two_wire_write_8bits(edt_p, tw_ptr, (device << 1) | 0x1); 

    if (edt_two_wire_check_ack(edt_p, tw_ptr) == FALSE)
    {
        edt_msg(EDTLIB_MSG_FATAL,"read: write device failed ack\n");
        FRETURN_I("two_wire_read",0);
        return(0);
    }

    while(len_data)
    {
        *data = edt_two_wire_read_8bits(edt_p, tw_ptr);
        len_data--;
        data++;
        ret++;
        if (len_data) 
            edt_two_wire_wr_databit(edt_p, tw_ptr, 0); /* ack */
        else
        {
            edt_two_wire_wr_databit(edt_p, tw_ptr, 1); /* no ack */
        }
    }

    /* wait for finisar part to let go of SDA, error if it does not */
    if (edt_two_wire_wait_sda_hi(edt_p, tw_ptr) >= MAX_SDA_HI_RETRIES)
    {
        edt_msg(EDTLIB_MSG_FATAL,"read: SDA failed to tristate after read\n");
        FRETURN_I("two_wire_read",0);
        return(0);
    }

    edt_two_wire_stop(edt_p,tw_ptr);

    FRETURN_I("two_wire_read",ret);

    return(ret);
}

/*
*  write a single byte
* requires device address (7 bits)
* address to write to (up to 32 bits)
*  a byte to write
* returns  a 0 if successfull
*/ 
u_int
edt_two_wire_write_byte(EdtDev * edt_p, EdtTwoWire * tw_ptr, char device, u_int address, char data)
{
    u_int poll = FALSE;
    u_int loop = 0;
    u_int error = 0;

    FENTER_I_I("two_wire_write_byte", address, data);
    /* a start, enable device with write, check ack and then do start address */ 
    edt_two_wire_start(edt_p, tw_ptr);
    edt_two_wire_write_8bits(edt_p, tw_ptr, (device << 1) | 0x0); /* write */
    if (edt_two_wire_check_ack(edt_p, tw_ptr) == FALSE)
    {
        edt_msg(EDTLIB_MSG_FATAL,"write: device write failed ack\n");
        FRETURN_I("two_wire_write_byte", 1);
        return(1);
    }
    while((loop != tw_ptr->address_length) && (error==0))
    {
        edt_two_wire_write_8bits(edt_p, tw_ptr, address << (loop*8) );
        if (edt_two_wire_check_ack(edt_p, tw_ptr) == FALSE) 
        {
            error = 1;
        }

        loop++;
    }
    if (error)
    {
        edt_msg(EDTLIB_MSG_FATAL,"write: address write failed ack\n");
        FRETURN_I("two_wire_write_byte", 1);
        return(1);
    }

    /* write the data */
    edt_two_wire_write_8bits(edt_p, tw_ptr, data);
    if (edt_two_wire_check_ack(edt_p, tw_ptr) == FALSE) 
    {
        edt_two_wire_start(edt_p, tw_ptr); /* aborts write */
        edt_msg(EDTLIB_MSG_FATAL,"write: data write failed ack\n");
        FRETURN_I("two_wire_write_byte", 1);
        return(1);
    }
    edt_two_wire_stop(edt_p, tw_ptr);


    /* now poll with a read  as soon as ack is asserted write is done */
    while(poll == FALSE)
    {
        edt_two_wire_start(edt_p, tw_ptr);
        edt_two_wire_write_8bits(edt_p, tw_ptr, (device << 1) | 0x1); 
        poll = edt_two_wire_check_ack(edt_p, tw_ptr);
    }

    /* read a byte to complete the command ignore as it is not the same byte written */
    edt_two_wire_read_8bits(edt_p, tw_ptr);

    /* respond with no ack to terminate */
    edt_two_wire_wr_databit(edt_p, tw_ptr, 1); /* no ack */
    /* wait for finisar part to let go of SDA, error if it does not */
    if (edt_two_wire_wait_sda_hi(edt_p, tw_ptr) > MAX_SDA_HI_RETRIES)
    {
        edt_msg(EDTLIB_MSG_FATAL,"write: SDA failed to tristate after post write read\n");
        FRETURN_I("two_wire_write_byte",1);
        return(1);
    }

    edt_two_wire_stop(edt_p, tw_ptr);
    FRETURN_I("two_wire_write_byte",0);
    return(0);
}


/*
* read the first byte of the two wire device
* this will indicate the transceiver type for subsequent reads
*/

unsigned char
edt_two_wire_read_type(EdtDev * edt_p, EdtTwoWire * tw_ptr)
{
    unsigned char ret;
    FENTER("two_wire_read_type");
    edt_two_wire_read(edt_p, tw_ptr, 
        TWO_WIRE_SERIAL_ID_DEVICE, 0, 1,
        (unsigned char *) &ret); 

    FRETURN_I("two_wire_read_type",ret);

    return(ret);
}


/*
* write a register in a i2c or spi serial device
* the base_desc is the address decriptor for the base of the 4 bytes used to read and write the serial device
* the address is a 15bit address in the serial device or can be decoded into multiple devices by the FPGA
* the value is the 8 bit data
*/
/*
* the hardware implements 4 bytes with these offsets from the base_desc
* offset 0 includes 7 bits of extended address for larger serial devices or to decode multiple devices
* bit 7 of offset 0 reads back the transfer busy bit indicating an operation is already in progress
* writing the lower 8 bits of address to offset 1 initiates a read, the data is read from offset 1
* as well after the xfer busy is clear
* offset 2 initiates a write of the data in offset 3 to the address written in od\ffset 2
*/
#define	EXT_ADDRESS	0
#define BUSY_STAT	0
#define	SERIAL_XFER_BSY_MSK	0x80
#define	READ_ADD_DATA	1
#define	WRITE_ADD	2
#define	WRITE_DATA	3

void
edt_serial_dev_reg_write(EdtDev * edt_p, 
                         uint_t base_desc,
                         uint_t device_id,
                         uint_t address, 
                         uint_t value)
{
    /* check for transfer busy */
    while ( ((edt_reg_read(edt_p, base_desc+BUSY_STAT)) & SERIAL_XFER_BSY_MSK) != 0)
        ;
    edt_reg_write(edt_p, base_desc+EXT_ADDRESS, device_id);
    edt_reg_write(edt_p, base_desc+WRITE_ADD, address & 0xff);
    edt_reg_write(edt_p, base_desc+WRITE_DATA, value);
}

uint_t
edt_serial_dev_reg_read(EdtDev * edt_p, 
                        uint_t base_desc, 
                        uint_t device_id,
                        uint_t address)
{
    /* check for transfer busy */
    while ( ((edt_reg_read(edt_p, base_desc+BUSY_STAT)) & SERIAL_XFER_BSY_MSK) != 0)
        ;
    edt_reg_write(edt_p, base_desc+EXT_ADDRESS, device_id);
    edt_reg_write(edt_p, base_desc+READ_ADD_DATA, address & 0xff);

    /* wait for transfer complete */
    while ( ((edt_reg_read(edt_p, base_desc+BUSY_STAT)) & SERIAL_XFER_BSY_MSK) != 0)
        ;
    return(edt_reg_read(edt_p, base_desc+READ_ADD_DATA));
}

uint_t
edt_serial_dev_reg_read_block(EdtDev *edt_p,
                        uint_t base_desc, 
                        uint_t device_id,
                        uint_t address,
                        uint_t length,
                        u_char *block) 

{
    int i;
    uint_t addr = address;

    for (i=0;i<(int)length;i++)
    {
        block[i] = edt_serial_dev_reg_read(edt_p,
                        base_desc, 
                        device_id,
                        addr);
        addr++;
    }

    return length;
}

uint_t
edt_serial_dev_reg_write_block(EdtDev *edt_p,
                        uint_t base_desc, 
                        uint_t device_id,
                        uint_t address,
                        uint_t length,
                        u_char *block) 

{
    int i;
    uint_t addr = address;

    for (i=0;i<(int)length;i++)
    {
        edt_serial_dev_reg_write(edt_p,
                        base_desc, 
                        device_id,
                        addr,
                        block[i]);
        addr++;
    }

    return length;
}

u_int
edt_serial_dev_set_bits(EdtDev *edt_p, u_int base_desc, u_int device, int address, int value, int shift, int mask)

{
    int invmask = ~(mask << shift);
    int v = value;

    if (mask != 0xff )
    {
        v = edt_serial_dev_reg_read(edt_p, base_desc, device, address);

        v &= invmask;
        v |= (value << shift);
    }

    edt_serial_dev_reg_write(edt_p, base_desc, device, address, v);

    return v;

}


static u_int bitmasks[32] = {
    0x1,
    0x3,
    0x7,
    0xf,
    0x1f,
    0x3f,
    0x7f,
    0xff,
    0x1ff,
    0x3ff,
    0x7ff,
    0xfff,
    0x1fff,
    0x3fff,
    0x7fff,
    0xffff,
    0x1ffff,
    0x3ffff,
    0x7ffff,
    0xfffff,
    0x1fffff,
    0x3fffff,
    0x7fffff,
    0xffffff,
    0x1ffffff,
    0x3ffffff,
    0x7ffffff,
    0xfffffff,
    0x1fffffff,
    0x3fffffff,
    0x7fffffff,
    0xffffffff
};

/* get arbitrary value up to 32 bits wide from two wire 8 bit registers */

u_int
edt_get_two_wire_value(EdtDev *edt_p, EdtRegisterDescriptor *map, u_int base_desc, u_int device)

{
    u_int v = 0;
    u_int bits = map->nbits;
    u_int i = 0;
    u_int shiftbits;
    u_int nbytes;
    int offset = map->reg;

    int64_t value;

    if (bits > 32)
    {
        edt_msg(EDTLIB_MSG_FATAL,"Unable to use register description > 32 bits wide \n");
        return 0;
    }


    shiftbits = bits + map->bitstart;

    nbytes = ((shiftbits-1)/8)+1;

    if (nbytes == 1)
    {
        v = (edt_serial_dev_reg_read(edt_p, base_desc, device, offset) >> map->bitstart) & bitmasks[bits-1];
    }
    else
    {

        value = 0;

        for (i=0;i<nbytes;i++)
        {
            value = (value << 8) | edt_serial_dev_reg_read(edt_p, base_desc, device, offset + i);
        }

        value >>= map->bitstart;

        value &= bitmasks[bits-1];

        v = (u_int) value;
    }

    return v;

}



int
edt_set_two_wire_value(EdtDev *edt_p, EdtRegisterDescriptor *map, u_int base_desc, u_int device, u_int value)

{
    u_int v = value;
    u_int bits = map->nbits;
    u_int i = 0;
    u_int shiftbits;
    int nbytes;
    int offset = map->reg;


    shiftbits = bits + map->bitstart;

    nbytes = ((shiftbits-1)/8)+1;

    if (map->bitstart)
    {
        /* shifted */

        if (nbytes == 1)
        {
            edt_serial_dev_set_bits(edt_p,base_desc,device,offset,v,map->bitstart, bitmasks[bits-1]);
        }
        else
        {
            u_char c;
            c = ((v << map->bitstart) & 0xff) >> map->bitstart;
            edt_serial_dev_set_bits(edt_p,base_desc,device,offset + nbytes-1, c, map->bitstart, bitmasks[8 - map->bitstart-1]);
            v >>= (8-map->bitstart);

            for (i=nbytes-2;i>0;i--)
            {
                edt_serial_dev_reg_write(edt_p,base_desc,device, offset+i,v & 0xff);
                v >>= 8;
            }

            edt_serial_dev_set_bits(edt_p,base_desc,device, offset,v,0,bitmasks[(shiftbits % 8)-1]);
        }
    }
    else if (bits % 8)
    {
        for (i=nbytes-1;i>0;i--)
        {
            edt_serial_dev_reg_write(edt_p,base_desc,device, offset+i,v & 0xff);
            v >>= 8;
        }

        /* read high byte, or in high byte of value*/

        edt_serial_dev_set_bits(edt_p,base_desc,device, offset,v,0,bitmasks[(bits % 8)-1]);
    }
    else
    {
        for (i=nbytes-1;i>= 0;i--)
        {
            edt_serial_dev_reg_write(edt_p,base_desc,device, offset+i,v & 0xff);
            v >>= 8;
        }
    }

    return 0;
}

int
edt_two_wire_reg_dump_raw(EdtDev *edt_p, 
                          EdtRegisterDescriptor *map, 
                          u_int base_desc, 
                          u_int device)

{
    int mapindex;
    int shiftbits, nbytes,i;
    int address = -1;
    u_int v;
    u_char used[256];

    memset(used,0,sizeof(used));

    for (mapindex = 0; map[mapindex].name != NULL ; mapindex++)
    {
        shiftbits = map[mapindex].nbits + map[mapindex].bitstart;

        nbytes = ((shiftbits-1)/8)+1;
        for (i=0;i<nbytes;i++)
            used[map[mapindex].reg+i] = 1;
    }

    for (address=0;address<256;address++)
    {
        if (used[address])
        {
            v = edt_serial_dev_reg_read(edt_p, 
                base_desc, 
                device,
                address);

            printf("%-20d: %02x\n", address, v);

        }
    }
    return 0;
}

int
edt_two_wire_reg_dump(EdtDev *edt_p, 
                      EdtRegisterDescriptor *map, 
                      u_int base_desc, 
                      u_int device)

{
    int mapindex;

    u_int v;    

    for (mapindex = 0; map[mapindex].name != NULL ; mapindex++)
    {

        v = edt_get_two_wire_value(edt_p, 
            map + mapindex,
            base_desc, 
            device);

        printf("%-20s: %02x\n", map[mapindex].name, v);

    }

    return 0;
}

