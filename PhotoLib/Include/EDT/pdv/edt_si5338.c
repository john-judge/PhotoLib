
#include "si5338_register_map.h"
#include "edt_si5338.h"

/**
 * Initialize the SI5338 chip.
 * @param edt_p	(or pdv_p) pointer to edt device structure returned by #edt_open or #pdv_open
 * @param verbose enable verbose output from library
 */
void
edt_si5338_init(EdtDev *edt_p, int verbose)
{
  Reg_Data curr;

  int      channel  = 0;
  int      counter  = 0;
  u_int    base     = 0x64; // 0x64 for CLS
  u_int    device   = 0;    // Device 0 for SI5338 on CLS equiates to i2c ADDR 0x70
  u_int    combined = 0;
  u_int    reg      = 0;
  int i;

  // Funky READ/MODIFY/WRITE variables
  u_int    curr_chip_val  = 0;
  u_int    clear_curr_val = 0;
  u_int    clear_new_val  = 0;

  if (verbose)
    printf("%s\n", "Initalizing...");

  //----------------------------------------------------------------
  // See Si5338 datasheet Figure 9 for more details on this procedure
  // delay added to wait for Si5338 to be ready to communicate after 
  // turning on. Not required but left here to remind anyone porting
  // this code to VHDL that a power-up delay must be provided.

  edt_msleep(12);

  if (base < 0x100) base |= 0x01010000;

  edt_serial_dev_reg_write(edt_p, base, device, 0xE6, 0x10); //OEB_ALL = 1
  edt_serial_dev_reg_write(edt_p, base, device, 0xF1, 0xE5); //DIS_LOL = 1

  //for all the register values in the Reg_Store array
  //get each value and mask and apply it to the Si5338
  for(counter=0; counter<NUM_REGS_MAX; counter++){
    curr = Reg_Store[counter];
    if(curr.Reg_Mask != 0x00) {
      if(curr.Reg_Mask == 0xFF) {
        // do a write transaction only
        // since the mask is all ones
        edt_serial_dev_reg_write(edt_p, base, device, curr.Reg_Addr, curr.Reg_Val);
    } else {
      //do a read-modify-write
      curr_chip_val = edt_serial_dev_reg_read(edt_p, base, device, curr.Reg_Addr);
      clear_curr_val = curr_chip_val & ~curr.Reg_Mask;
      clear_new_val = curr.Reg_Val & curr.Reg_Mask;
      combined = clear_new_val | clear_curr_val;
      edt_serial_dev_reg_write(edt_p, base, device, curr.Reg_Addr, combined);
      }
    }
  }

  // check LOS alarm for the xtal input
  // on IN1 and IN2 (and IN3 if necessary) -
  // change this mask if using inputs on IN4, IN5, IN6
  reg = edt_serial_dev_reg_read(edt_p, base, device, 0xDA) & LOS_MASK;

  while(reg != 0){
    reg = edt_serial_dev_reg_read(edt_p, base, device, 0xDA) & LOS_MASK;
  }

  reg = edt_serial_dev_reg_read(edt_p, base, device, 0x31);
  edt_serial_dev_reg_write(edt_p, base, device, 0x31, reg & 0x7F); //FCAL_OVRD_EN = 0
  edt_serial_dev_reg_write(edt_p, base, device, 0xF6, 0x2);        //soft reset
  edt_serial_dev_reg_write(edt_p, base, device, 0xF1, 0x65);       //DIS_LOL = 0

  // wait for Si5338 to be ready after calibration (ie, soft reset)
  edt_msleep(24);

  //make sure the device locked by checking PLL_LOL and SYS_CAL
  reg = edt_serial_dev_reg_read(edt_p, base, device, 0xDA) & LOCK_MASK;
  while(reg != 0){
    reg = edt_serial_dev_reg_read(edt_p, base, device, 0xDA) & LOCK_MASK;
  }

  //copy FCAL values
  reg = edt_serial_dev_reg_read(edt_p, base, device, 0xEB);
  edt_serial_dev_reg_write(edt_p, base, device, 0x2D, reg);
  reg = edt_serial_dev_reg_read(edt_p, base, device, 0xEC);
  edt_serial_dev_reg_write(edt_p, base, device, 0x2E, reg);
  // clear bits 0 and 1 from 47 and
  // combine with bits 0 and 1 from 237
  reg = (edt_serial_dev_reg_read(edt_p, base, device, 0x2F) & 0xFC) | 
        (edt_serial_dev_reg_read(edt_p, base, device, 0xED) & 0x3);
  edt_serial_dev_reg_write(edt_p, base, device, 0x2F, reg);
  reg = edt_serial_dev_reg_read(edt_p, base, device, 0x31);
  edt_serial_dev_reg_write(edt_p, base, device, 0x31, reg | 0x80); // FCAL_OVRD_EN = 1
  edt_serial_dev_reg_write(edt_p, base, device, 0xE6, 0x00);       // OEB_ALL = 0

  if (verbose)
    printf("%s\n", "Done.");
}

/**
 * Initialize the SI5338 chip, when the PLLs are not programmed, then program the
 * selected clock to the selected frequency.
 * @param edt_p	(or pdv_p) pointer to edt device structure returned by #edt_open or #pdv_open
 * @param clk the selected clock
 * @param freq the selected frequency in Hz
 * @param verbose enable verbose output from library
 */
void
edt_si5338_setFreq(EdtDev *edt_p, u_char clk, u_int freq, int verbose)
{
  Reg_Data curr;

  int      channel  = 0;
  int      counter  = 0;
  u_int    base     = 0x64; // 0x64 for CLS 
  u_int    device   = 0;    // Device 0 for SI5338 on CLS equiates to i2c ADDR 0x70

  u_int msa, msb, msc, msxp1, msxp2, msxp3p2top, vco;
  int i;

  edt_msleep(12);

  if (base < 0x100) base |= 0x01010000;

  if ((edt_serial_dev_reg_read(edt_p, base, device, 50) & 0xC0) == 0x00)
  {
    edt_si5338_init(edt_p, verbose); //if PLL is disabled, si5338 needs to be initialized
  }

  vco = SI5338_BX_XTAL * SI5338_MSn;

  msa = vco/freq; //integer
  msb = vco%freq; //numberatir
  if(msb == 0)
    msc = 1; //denoninator
  else
    msc = freq;
  
  while (msc & 0xfff00000) {msb=msb>>1; msc=msc>>1;}
  msxp1 = (128*msa + 128*msb/msc - 512);
  msxp2 = (msb*128)%msc;
  msxp3p2top = msc;
  msxp2 = msxp2 << 2;

  {
    u_char vals[10] = {BB0(msxp1), BB1(msxp1), //breaks 32 bit values into bytes for reg writing
			BB0(msxp2), BB1(msxp2), BB2(msxp2), 0x00,
			BB0(msxp3p2top), BB1(msxp3p2top), BB2(msxp3p2top), 0x00};

    for(i=0;i<10;i++) {
      curr = Reg_Store[11*(5+clk)+i-2]; //gets and sets MSx registers
      curr.Reg_Val = vals[i];
      edt_serial_dev_reg_write(edt_p, base, device, curr.Reg_Addr, curr.Reg_Val);

      if (verbose && i == 0)
	printf("%s%i-", "Writing to registers ", curr.Reg_Addr);
      if (verbose)
	printf("edt_serial_dev_reg_write edt_p %p base %x device %x Reg_Addr %x Reg_Val %x\n",
					 edt_p,   base,   device, curr.Reg_Addr, curr.Reg_Val);
      if (verbose && i == 9)
	printf("%i%s\n%s\n", curr.Reg_Addr, "...", "Done.");
    }
  }
}
