/**
 * @file
 * Camera Link Simulator Library, for use with PCI DV CLS and PCIe DV C-Link with
 * Simulator FPGA loaded.
 */
#include "edtinc.h"
#include "clsim_lib.h"
#include "edt_si570.h"
#include "edt_si5338.h"

void pdv_cls_set_clock(PdvDev *pdv_p, double freq);
void pe8dvcls_set_clock(PdvDev *pdv_p, double target);
void pe8vlcls_set_clock(PdvDev *pdv_p, double freqMHz);

/**
 * Set the width and height of the simulator frame.
 *
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param taps number of clocks per line
 * @param depth in bits of data
 * @param width width of active data
 * @param height number of lines of active data
 * @param hblank horizontal blanking between lines
 * @param totalwidth total width including horizontal blanking if hblank is zero
 * @param vblank horizontal blanking between lines
 * @param totalwidth total number of lines including vertical blanking if vblank is zero
 *
 * There are two ways to set the total width and height including blanking:
 * If hblank is non-zero, the total line width is width + hblank 
 * otherwise it is the value passed in in totalwidth.
 * Likewise, if vblank is non-zero, the number of lines between frame valids is
 * height + vblank, otherwise it's the value passed in in totalheight
 *
 * @return void
 */
void pdv_cls_set_size(PdvDev *pdv_p,
        int taps,
        int depth ,
        int width,
        int height,
        int hblank,
        int totalwidth,
        int vblank,
        int totalheight)

{
    PdvDependent *dd_p = pdv_p->dd_p;
    int clocks;


    if (taps == 0)
        taps = 1;

    clocks = width / taps;

    if (hblank)
    {
        totalwidth = clocks + hblank;
        pdv_p->dd_p->cls.hblank = hblank;
    }

    if (vblank)
    {
        totalheight = height + vblank;
        pdv_p->dd_p->cls.vblank = vblank;
    }

    /* hold off sim start */

    edt_reg_write(pdv_p, PDV_CLSIM_HCNTMAX, totalwidth-1);
    edt_reg_write(pdv_p, PDV_CLSIM_VACTV, height-1);
    edt_reg_write(pdv_p, PDV_CLSIM_VCNTMAX, totalheight-1);

    edt_reg_write(pdv_p, PDV_CL_DATA_PATH, dd_p->cl_data_path);

    edt_reg_write(pdv_p, PDV_CFG, 0);

    dd_p->width = width;
    dd_p->height = height;
    dd_p->depth = depth;
    dd_p->imagesize = dd_p->height * pdv_get_pitch(pdv_p);
    dd_p->cls.taps = taps;
}

/**
 * Set the values for frame valid (FVAL), line valid (LVAL), and read valid (RVAL) timing.
 * In each case, if the end value is 0, the number of clocks required for width is added to 
 * the start value (default 0). So if start and end are 0, defaults are start = 0 and end = width/taps.
 *
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param taps number of clocks per line
 * @param width active pixels per line
 * @param Hfvstart
 * @param Hfvend
 * @param Hlvstart
 * @param Hlvend
 * @param Hrvstart
 * @param Hrvend
 *
 * @return void
 */
    void
pdv_cls_set_line_timing(PdvDev *pdv_p,
        int width,
        int taps,
        int Hfvstart,
        int Hfvend,
        int Hlvstart,
        int Hlvend,
        int Hrvstart,
        int Hrvend)

{
    ClSimControl *cls = &pdv_p->dd_p->cls;
    int clocks;

    if (taps == 0)
        taps = 1;

    clocks = width / taps;

    cls->Hfvstart = Hfvstart;
    cls->Hfvend = (Hfvend)?Hfvend:Hfvstart+clocks;

    cls->Hlvstart = Hlvstart;
    cls->Hlvend = (Hlvend)?Hlvend:Hlvstart+clocks;

    cls->Hrvstart = Hrvstart;
    cls->Hrvend = (Hrvend)?Hrvend:Hrvstart+clocks;

    edt_reg_write(pdv_p, PDV_CLSIM_HFVSTART, cls->Hfvstart);
    edt_reg_write(pdv_p, PDV_CLSIM_HFVEND, cls->Hfvend);

    edt_reg_write(pdv_p, PDV_CLSIM_HLVSTART, cls->Hlvstart);
    edt_reg_write(pdv_p, PDV_CLSIM_HLVEND, cls->Hlvend);

    edt_reg_write(pdv_p, PDV_CLSIM_HRVSTART, cls->Hrvstart);
    edt_reg_write(pdv_p, PDV_CLSIM_HRVEND, cls->Hrvend);

    pdv_p->dd_p->width = width;
    pdv_p->dd_p->cls.taps = taps;

}


/**
 * Computes the horizontal gap value based on the difference between
 * active clocks (hblank) and the total clocks.
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @return horizontal gap value
 */
    int
pdv_cls_get_hgap(PdvDev *pdv_p)

{
    int totalcnt;
    int clocks;
    int taps;
    int rgb;

    rgb = pdv_p->dd_p->cl_cfg & PDV_CL_CFG_RGB;

    totalcnt = edt_reg_read(pdv_p, PDV_CLSIM_HCNTMAX) + 1;

    if (rgb)
        taps = 1;
    else if (pdv_get_depth(pdv_p) > 8)
        taps = 1;
    else
        taps = (edt_reg_read(pdv_p, PDV_CL_DATA_PATH) >> 4) + 1;

    clocks = pdv_get_width(pdv_p) / taps;

    return totalcnt - clocks;

}

/**
 * Computes the vertical gap value based on the difference between
 * active lines(vblank) and the total lines
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @return vertical gap value
 */
    int
pdv_cls_get_vgap(PdvDev *pdv_p)

{
    int totalcnt;
    int vactv;

    totalcnt = edt_reg_read(pdv_p, PDV_CLSIM_VCNTMAX) + 1;

    vactv = edt_reg_read(pdv_p, PDV_CLSIM_VACTV) + 1;

    return totalcnt - vactv;

}

/**
 * Re-intializes and enables the serial communication.
 * 
 * Rarely used since the serial gets initialized at device open.
 * 
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @return void
 */
    void
pdv_cls_init_serial(PdvDev *pdv_p)

{
    pdv_reset_serial(pdv_p);

    edt_reg_write(pdv_p, PDV_CMD, 1);

    edt_reg_write(pdv_p, PDV_SERIAL_DATA_CNTL, 3);
    edt_reg_write(pdv_p, PDV_BYTESWAP, 0);

    pdv_serial_read_enable(pdv_p);
}

/**
 * General bit setting routine, primarily for internal use by other library routines.
 *
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param reg the register to set (see PDV_CLSIM* registers in edtreg.h)
 * @param mask mask for specific bits to set, leaving others unchanged
 * @param state state to set specific bit(s)
 * @return void
 */
void pdv_cls_set_cfg_bit(PdvDev *pdv_p,
        int reg,
        int mask,
        int state)

{
    int cfg = edt_reg_read(pdv_p, reg);

    cfg = (cfg & ~mask) | ((state)? mask : 0);

    edt_reg_write(pdv_p, reg, cfg);
}

/** 
 * When set, once the start-of-frame conditions are met, the simulator
 * runs forever, emulating a linescan camera (as if the total vertical active and
 * total vertical count maximum  were set to infinity.)
 *
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param state 1 enables linescan, 0 disables linescan
 *
 * @return void
 */
    void
pdv_cls_set_linescan(PdvDev *pdv_p, int state)

{
    pdv_cls_set_cfg_bit(pdv_p, PDV_CLSIM_CFGA, 0x80, state);
}

/** 
 * Enables / disables line valid timing during vertical blanking.
 *
 * When set, line valid is asserted continuously with it's normal timing,
 * even during the vertical blanking interval between frames. When celar,
 * line valid remains low during vertical blanking. Default is unset (0).
 *
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param state 1 enables continuous line valid , 0 disables it
 *
 * @return void
 */
    void
pdv_cls_set_lvcont(PdvDev *pdv_p, int state)

{
    pdv_cls_set_cfg_bit(pdv_p, PDV_CLSIM_CFGA, 0x40, state);
}

/** Enables or disables ReadValid Enable (RVEN).
 *
 * Read valid is special functionality (not in the Camera Link specification)
 * that allows for outputting an image that's wider than the image data provided.
 * The data outside the image data margins is filled with dummy data values.
 *
 * When  RVEN is set, then the start and end margins of each raster are filled
 * with the values from the FillA and FillB registers respectively, the
 * positions of the margins are determined by HrvStart and HrvEnd.
 * When RVEN is cleared, the entire raster is filled with valid data.
 * HrvStart and HrvEnd can be set with #pdv_cls_set_readvalid().
 *
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param enable true to enable ReadValid so data in margins comes from Fill values.
 * @return void
 */
    void
pdv_cls_set_rven(PdvDev *pdv_p, int state)

{
    pdv_cls_set_cfg_bit(pdv_p, PDV_CLSIM_CFGA, 0x20, state);
}

/** 
 * Enables or disables UART looping (echo) of serial data.
 *
 * When set, serial data emitted by the framegrabber is echoed back
 * unchanged, allowing testing of the framegrabber's serial port.
 *
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param state 1 enables uart looping, 0 disables it.
 *
 * @return void
 */
    void
pdv_cls_set_uartloop(PdvDev *pdv_p, int state)

{
    pdv_cls_set_cfg_bit(pdv_p, PDV_CLSIM_CFGA, 0x10, state);
}

/** 
 * Sets simulator FIFO for small (less than 16KB) images.
 * 
 * When set, simulator starts DMA when 1KB of data is in the FIFO, allowuing the simulator
 * to handle images smaller than 16 KB. When clear, simulator waits until 16 KB of data is
 * in the FIFO before starting DMA. Default for this state is 0 (clear).
 *
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param state 1 enables the state, 0 disables it
 *
 * @return void
 */
    void
pdv_cls_set_smallok(PdvDev *pdv_p, int state)

{
    pdv_cls_set_cfg_bit(pdv_p, PDV_CLSIM_CFGA, 8, state);
}

/** Enables or disables four-tap interleaving.
 *
 * When set, enables four-tap interleaving -- the four-tap reordering of 8-bit pixel values.
 *
 * See the CLS Users Guide, Appendix A for a complete description of how data is
 * interleaved. For example, 0x60-61 Tap 0 Start through 0xE-6F Tap 3 Delta. Image data destined
 * for the framegrabber is first passed through an interleaving mechanism to duplicate the data
 * ordering that some cameras exhibit. WHen interleaving is enabled, rasters are restricted to a
 * maximum of 4096 eight-bit pixels of active image data (DMA plus fill).
 * 
 * When clear (default), interleaving is disabled. 
 *
 * To use interleave, first set up the interleave scheme using
 * #pdv_cls_setup_interleave.
 *
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param enable true to turn on interleave, false to disable it.
 * @see pdv_cls_setup_interleave 
 * @return void
 */
    void
pdv_cls_set_intlven(PdvDev *pdv_p, int state)

{
    pdv_cls_set_cfg_bit(pdv_p, PDV_CLSIM_CFGA, 4, state);
}

/** 
 * Enables / disables frame count in the first word of each frame.
 *
 * When set, the first word of the frame is the frame count: a 16-bit flag of
 * 0x3333 in the MSbs and a 16-bit framecount in the LSbs. It replaces the first
 * 32-bit word of DMA or internally generated data, after any interleaving.
 * When clear, the forst word is the DMA data or generated data per #pdv_cls_set_firstfc.
 *
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param state 1 enables the first word frame count, 0 disables it.
 *
 * @return void
 */
    void
pdv_cls_set_firstfc(PdvDev *pdv_p, int state)

{
    pdv_cls_set_cfg_bit(pdv_p, PDV_CLSIM_CFGA, 2, state);
}

/** 
 * Enables / disables internal image data generation.
 * 
 * When enabled, image data comes from the counters instead of the DMA stream.
 *
 * The simulated 32-bit data generated has a 16-bit count in the LSbs; the 16 MSbs
 * are an inverted version of the LSBs.  The count is cleared to zero at the start
 * of each frame. Thus the first 32-bit word of each frame is 0xffff0000, the second
 * is fffe0001, and so on. The CLS treats this data as little-endian, so the foruth
 * 8-bit pixel fo the frame has a value of 0x01. When set, also setting SMALLOK (#pdv_cls_set_smallok)
 * stops the simulator at the start of the next frame, to enable getting a single
 * frame of counter data.
 * 
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param state 1 outputs internally-generated data. When disabled, outputs data from the host via DMA.
 * @see pdv_cls_set_smallok
 *
 * @return void
 */
    void 
pdv_cls_set_datacnt(PdvDev *pdv_p, int state)

{
    pdv_cls_set_cfg_bit(pdv_p, PDV_CLSIM_CFGA, 1, state);
}

/** Controls state of the board's green LED.
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param power_state true (non-zero) to turn on LED, false to turn it off.
 * @return void
 */
    void
pdv_cls_set_led(PdvDev *pdv_p, int state)

{
    pdv_cls_set_cfg_bit(pdv_p, PDV_CLSIM_CFGC, 0x80,state);
}

/** 
 * Selects which input pins to look at for external trigger.
 * When set, selects camera control line 2 as trigger source. When clear,
 * selects camera control line 1.
 *
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param select 1 enables input trigger on CC2, when clear uses CC1
 * @see pdv_cls_set_trigpol, pdv_cls_set_trigframe, pdv_cls_set_trigline
 *
 * @return void
 */
    void
pdv_cls_set_trigsrc(PdvDev *pdv_p, int select)

{
    pdv_cls_set_cfg_bit(pdv_p, PDV_CLSIM_CFGC, 0x08, select);
}

/** 
 * Sets the trigger polariry.
 * A value of 1 sets the trigger polarity to positive TRUE (the default). A value of
 * 0 sets it to negative TRUE.
 *
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param state trigger polarity
 * @see pdv_cls_set_trigsrc, pdv_cls_set_trigframe, pdv_cls_set_trigline
 *
 * @return void
 */
    void
pdv_cls_set_trigpol(PdvDev *pdv_p, int polarity)

{
    pdv_cls_set_cfg_bit(pdv_p, PDV_CLSIM_CFGC, 0x04, polarity);
}

/** 
 * Set to enable frame-valid triggering. Simulator waits at the start of each frame
 * until a trigger is detected.
 *
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param state 1 enables, 0 disables
 * @see pdv_cls_set_trigsrc, pdv_cls_set_trigpol, pdv_cls_set_trigline
 *
 * @return void
 */
    void
pdv_cls_set_trigframe(PdvDev *pdv_p, int state)

{
    pdv_cls_set_cfg_bit(pdv_p, PDV_CLSIM_CFGC, 0x02, state);
}

/** 
 * Set to enable line-valid triggering. Simulator waits at the start of each raster
 * until a trigger is detected. A Dalsa linescan camera starts the next raster when it
 * detects a rising edge on the CC1 line.
 *
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param state 1 enables, 0 disables
 * @see pdv_cls_set_trigsrc, pdv_cls_set_trigpol, pdv_cls_set_trigframe
 *
 * @return void
 */
    void
pdv_cls_set_trigline(PdvDev *pdv_p, int state)

{
    pdv_cls_set_cfg_bit(pdv_p, PDV_CLSIM_CFGC, 1,state);
}

/** 
 * Clears the CFG register including the FIFO_RESET bit (bit 3, 0x08) which clears the fifo
 * and starts the simulator.
 *
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 *
 * @return void
 */
    void
pdv_cls_sim_start(PdvDev *pdv_p)

{
    edt_reg_write(pdv_p, PDV_CFG, 0);
}

/** 
 * Sets the CFG register FIFO_RESET bit (bit 3, 0x08) which stops the simulator.
 *
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 *
 * @return void
 */
    void
pdv_cls_sim_stop(PdvDev *pdv_p)

{

    edt_reg_write(pdv_p, PDV_CFG, 8);
}

/** Set the width of outgoing lines, as well as the number of clocks (hgap) between lines.
 * Make sure depth / taps are set correctly first by calling #pdv_set_depth (or use #pdv_cls_set_size
 * instead of this routine), otherwise the registers won't be set correctly.
 * Also note that this overwrites the horizontal line valid start values with new values
 * based on the width  & blanking, and sets readvalid to the full width. Follow this with a call
 * to /ref pdv_cls_set_line_timing if you want to set specific values for those.
 *
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param width number of pixels per line
 * @param hgap number of clocks between lines (horizontal gap)
 * @see pdv_cls_set_height, pdv_cls_set_line_timing
 * @return void
 */
    void
pdv_cls_set_width(PdvDev *pdv_p, int width, int hblank)

{
   Dependent *dd_p = pdv_p->dd_p;
   int totalwidth;
   int taps = edt_reg_read(pdv_p, PDV_CL_DATA_PATH);
   int rgb = pdv_p->dd_p->cl_cfg & PDV_CL_CFG_RGB;
   int clocks;

    taps =  (taps >> 4) + 1;

    if ((taps == 0) || rgb)
        taps = 1;
        
    clocks = width / taps;

    dd_p->width = width;
    dd_p->cls.taps = taps;

    if (hblank)
    {
        totalwidth = clocks + hblank;
        pdv_p->dd_p->cls.hblank = hblank;
    }
    else totalwidth = width / taps;

    dd_p->imagesize = dd_p->height * pdv_get_pitch(pdv_p);

    /* hold off sim start */

    edt_reg_write(pdv_p, PDV_CLSIM_HCNTMAX, totalwidth-1);  

    /* reset line timing; note that this overwrites hlvstart/hlvend and hrvstart/hrvend */
    pdv_cls_set_line_timing(pdv_p,
            width,
            taps,
            dd_p->cls.Hfvstart, dd_p->cls.Hfvend,
            0,0,
            0,0);

    dd_p->imagesize = dd_p->height * pdv_get_pitch(pdv_p);
    edt_set_dependent(pdv_p, dd_p);
}

/** Set the width of outgoing lines, as well as the number of clocks (hgap) between lines and start and end of
 * line valid and read valid.
 * Same as pdv_cls_set_width but includes lval and readval start and end.
 * Make sure depth / taps are set correctly first by calling #pdv_set_depth (or use #pdv_cls_set_size
 * instead of this routine), otherwise the registers won't be set correctly.
 *
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param width number of pixels per line
 * @param hgap number of clocks between lines (horizontal gap)
 * @param hlvstart number of clocks between lines (horizontal gap)
 * @param hlvend number of clocks between lines (horizontal gap)
 * @see pdv_cls_set_width
 * @return void
 */
    void
pdv_cls_set_width_lval_rval(PdvDev *pdv_p, int width, int hblank, int hlvstart, int hlvend, int hrvstart, int hrvend)

{
   Dependent *dd_p = pdv_p->dd_p;
   int totalwidth;
   int taps = edt_reg_read(pdv_p, PDV_CL_DATA_PATH);
   int rgb = pdv_p->dd_p->cl_cfg & PDV_CL_CFG_RGB;
   int clocks;

    taps =  (taps >> 4) + 1;

    if ((taps == 0) || rgb)
        taps = 1;
        
    clocks = width / taps;

    dd_p->width = width;
    dd_p->cls.taps = taps;

    if (hblank)
    {
        totalwidth = clocks + hblank;
        pdv_p->dd_p->cls.hblank = hblank;
    }
    else totalwidth = width / taps;

    dd_p->imagesize = dd_p->height * pdv_get_pitch(pdv_p);

    /* hold off sim start */

    edt_reg_write(pdv_p, PDV_CLSIM_HCNTMAX, totalwidth-1);  

    pdv_cls_set_line_timing(pdv_p,
            width,
            taps,
            dd_p->cls.Hfvstart, dd_p->cls.Hfvend,
            hlvstart,hlvend,
            hrvstart, hrvend);

    dd_p->imagesize = dd_p->height * pdv_get_pitch(pdv_p);
    edt_set_dependent(pdv_p, dd_p);
}


/** Set the height of outgoing frames, as well as the number of lines (vgap) between lines.
 *
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param height number of pixels per line
 * @param vgap number of clocks between lines (vertical gap)
 * @return void
 */
    void
pdv_cls_set_height(PdvDev *pdv_p, int height, int vblank)
{
   Dependent *dd_p = pdv_p->dd_p;

    dd_p->height = height;
    dd_p->cls.vblank = vblank;

    edt_reg_write(pdv_p, PDV_CLSIM_VACTV, height-1);
    edt_reg_write(pdv_p, PDV_CLSIM_VCNTMAX, (height + vblank) - 1);

    dd_p->imagesize = dd_p->height * pdv_get_pitch(pdv_p);
    edt_set_dependent(pdv_p, dd_p);

}

    void
pdv_cls_set_depth(PdvDev * pdv_p, int value)
{
    Dependent *dd_p = pdv_p->dd_p;

    dd_p->depth = value;

    /* set cameralink data path register, if appropriate -- note that
     * odball values are generally  handled differently (e.g.
     * pdvcamlk_pir which has hard-coded depth)
     */
    if (pdv_is_cameralink(pdv_p))
    {
        int reg;

        if ((value >= 8) && (value <= 16))
            reg = value-1;
        else if (value == 24 || value == 32)
            reg = 0x7;
        else if (value == 30)
            reg = 0x9;
        else reg = 0;

        if (reg)
        {
            if (dd_p->dual_channel) /* obsolete, but still... */
                reg |= 0x10;
            dd_p->dual_channel = reg;
            edt_reg_write(pdv_p, PDV_CL_DATA_PATH, dd_p->cl_data_path);
        }
    }

    dd_p->imagesize = dd_p->height * pdv_get_pitch(pdv_p);
    edt_set_dependent(pdv_p, dd_p);

}

/**
 * Set the clock frequency (MHz).
 *
 * On PCI boards, this sets the MPC9230 PLL on PCI CD-CLSIM to 3.5 times the requested pixclk freq.
 * On PCIe DVa boards, sets the SI570 PLL to 1.25x the requeted freq. On  PCIe DV boards,
 * sets the SI570 PLL to 1x the  requested freq.
 * Valid range is 19.9-85.1. A warning is produced for frequencies outside this range
 * @param freq pixel clock frequency (MHz)
 * @return void
 */
    void
pdv_cls_set_clock(PdvDev *pdv_p, double freq) 

{	/* Pixel clock frequency in MHz */
    int hex, 
        bit, 
        dbit, 
        m, 
        n, 
        n2, 
        b, 
        m_best, 
        n_best;
    double ref=14.0;  /* 40.0/3.0; */
    double targ35, 
           best35, 
           current35;    /* PixClock*3.5 in MHz */
    double err_current35, err_best35, err1x;
    int mmin = (int) (0.99 + 800*4/ref);    /* highest acceptable m value */
    int mmax = (int) (1800*4/ref);	/* lowest acceptable m value */

    /* zero is special case, reserved for testing/internal 40 */
    if (freq == 0.0)
    {
        edt_reg_write(pdv_p, EDT_SS_PLL_CTL, 0);
        pdv_p->dd_p->cls.pixel_clock = (float)freq;
        return;
    }
    else if (freq<19.9 || freq>85.1)
        printf("WARNING: %3.4f is not in range of 20 to 85 MHz\n", freq);

    pdv_p->dd_p->cls.pixel_clock = (float)freq;
    pdv_p->dd_p->pclock_speed = (int)(pdv_p->dd_p->cls.pixel_clock + 0.5);
       /* pclock_speed is redundant w.r.t. CLS but still they should both agree */

    if (pdv_p->devid == PE8DVCLS_ID)
    {
        if (pdv_p->channel_no == 0)
            pe8dvcls_set_clock(pdv_p, (double)freq);
    }
    else if (pdv_p->devid == PE8VLCLS_ID)
    {
	// Take the board out of reset
	edt_bar1_write(pdv_p, 0, 1);

	// Program the SI5338 clock chip
	pe8vlcls_set_clock(pdv_p, (double)freq);
    }
    else
    {
	if (mmax > 511)   
	    mmax=511;

	targ35 = freq*3.5;
	best35 = 0.0;  
	m_best=0; 
	n_best=0;  
	err_best35=100.0;

	for (m=mmin; m<=mmax; m++) 
	{
	    for (n=1; n<=8; n*=2) 
	    {	/* for n values of 1,2,4,8 */
		current35 = ref/8 * m / n;
		err_current35 = targ35 - current35;	  /* abs() not for doubles */
		if (err_current35 < 0)    
		    err_current35 = -err_current35;
		if (err_current35 < err_best35) 
		{
		    best35=current35; 
		    m_best=m; 
		    n_best=n;  
		    err_best35=err_current35;
		}
	    }
	}
	if (n_best==1) 
	    n2=3;
	else if (n_best==2) 
	    n2=0;
	else if (n_best==4) 
	    n2=1;
	else if (n_best==8) 
	    n2=2;
	hex = n2<<9 | m_best;

	err1x = (best35-targ35)/3.5;
	if (err1x < 0.0)   
	    err1x = - err1x;
	if (err1x > 0.00090) 
	{
	    printf("req:%3.4f got:%3.4f err:%3.4f ecl:%3.4f vco:%3.4f m:%d n:%d h:%04x\n",
		    freq,
		    best35/3.5,
		    (best35-targ35)/3.5,
		    best35,best35*2*n_best,
		    m_best,n_best,hex);
	}

	edt_reg_write(pdv_p, EDT_SS_PLL_CTL, 0);
	for (bit=13; bit>=0; bit--) 
	{	/* send 14 data bits to ECL PLL */
	    b = hex & (1<<bit);	 /* rising edge of *CLK shifts data in*/
	    if (b)   
		dbit=EDT_SS_PLL_DATA; 
	    else 
		dbit=0; 
	    edt_reg_write(pdv_p, EDT_SS_PLL_CTL, dbit) ;    
	    edt_reg_write(pdv_p, EDT_SS_PLL_CTL, dbit | EDT_SS_PLL_CLK);
	}	     /* latches to m,n transparent while *STROBE0 is high */
	edt_reg_write(pdv_p, EDT_SS_PLL_CTL, EDT_SS_PLL_STROBE0);
	edt_reg_write(pdv_p, EDT_SS_PLL_CTL, 0);
	edt_set_dependent(pdv_p, pdv_p->dd_p);
    }
}

/**
 * Set si570 clock speed in microseconds.
 * Not to be called directly -- use #pdv_cls_set_clock()
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param target clock speed in microseconds (ALERT CHECK THIS)
 * @return void
 */
    void
pe8dvcls_set_clock(PdvDev *pdv_p, double target)
{
    u_int base_desc = 0x70; 
    u_int device  = 0;
    EdtSI570 clock_parms;
    int do_reset = 0;

    if (target < 100)
        target *= 1000000.0;

    if (pdv_p->dd_p->cls.si570_nominal == 0)
    {
        u_short stat;
        if (edt_flash_prom_detect(pdv_p, &stat) == AMD_XC5VLX30T_A)
            pdv_p->dd_p->cls.si570_nominal = 125000000.0;
        else pdv_p->dd_p->cls.si570_nominal = 100000000.0;
    }
    else if (pdv_p->dd_p->cls.si570_nominal < 2000.0)
        pdv_p->dd_p->cls.si570_nominal *= 1000000.0;

    edt_si570_set_clock(pdv_p, base_desc, device, pdv_p->dd_p->cls.si570_nominal, target, &clock_parms);

    edt_set_dependent(pdv_p, pdv_p->dd_p);

}

/**
 * Set si5338 clock speed in microseconds.
 * Not to be called directly -- use #pdv_cls_set_clock()
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param target clock speed in microseconds (ALERT CHECK THIS)
 * @return void
 */
void
pe8vlcls_set_clock(PdvDev *pdv_p, double freqMHz)
{
  int clk = (pdv_p->channel_no == 0) ? 2 : 3;

  edt_si5338_setFreq(pdv_p, clk, (uint32_t)(freqMHz*1000000.0), 0);
}



/**
 * Set the values for Data Valid (DVAL), timing.
 *
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param skip how many clocks to skip (ALERT CHECK THIS)
 * @param mode mode on or off (ALERT CHECK THIS)
 * @return void
 */
void pdv_cls_set_dvalid(PdvDev *pdv_p, u_char skip, u_char mode)
{
    edt_reg_write(pdv_p, PDV_CLSIM_CFGB, (skip << 4) | (mode & 3));  
}

/** Sets the left and right fill values when READVAL is set.
 *
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param left the 8 bit left fill value (FillA in CLSIM docs)
 * @param right the 8 bit right fill value (FillB in CLSIM docs)
 * @see pdv_cls_set_rven
 * @return void
 */
void pdv_cls_set_fill(PdvDev *pdv_p, u_char left, u_char right)
{
    /* left= FILLA, right=FILLB. */
    edt_reg_write(pdv_p, PDV_CLSIM_FILLA, left);
    edt_reg_write(pdv_p, PDV_CLSIM_FILLB, right); 
}

/** Sets the horizontal start and end positions of the ReadValid signal.
 * Note that these values have no effect unless RVEN is true.
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param HrvStart start postion
 * @param HrvEnd end position
 * @see pdv_cls_set_rven.
 */
void pdv_cls_set_readvalid(PdvDev *pdv_p, 
        u_short HrvStart, u_short HrvEnd)
{
    edt_reg_write(pdv_p, PDV_CLSIM_HRVSTART, HrvStart);
    edt_reg_write(pdv_p, PDV_CLSIM_HRVEND, HrvEnd);
    pdv_p->dd_p->cls.Hrvstart = HrvStart;
    pdv_p->dd_p->cls.Hrvend = HrvEnd;
}


/** Sets the start address and delta for each tap.
 * The start address is the 12-bit address of an 8-bit pixel within
 * the 4096 pixel raster.  The delta is the amount added to the pixel
 * address with each pixel clock. 
 * 
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @param tap0start the start address for tap 0
 * @param tap0delta the delta for tap 0
 * @param tap1start the start address for tap 1 
 * @param tap1delta the delta for tap 1
 * @param tap2start the start address for tap 2
 * @param tap2delta the delta for tap 2
 * @param tap3start the start address for tap 3
 * @param tap3delta the delta for tap 3
 * @return void
 */
void pdv_cls_setup_interleave(PdvDev *pdv_p,
        short tap0start, short tap0delta, 
        short tap1start, short tap1delta,
        short tap2start, short tap2delta,
        short tap3start, short tap3delta) 

{
    edt_reg_write(pdv_p, PDV_CLSIM_TAP0START, tap0start);
    edt_reg_write(pdv_p, PDV_CLSIM_TAP0DELTA, tap0delta);

    edt_reg_write(pdv_p, PDV_CLSIM_TAP1START, tap1start);
    edt_reg_write(pdv_p, PDV_CLSIM_TAP1DELTA, tap1delta);

    edt_reg_write(pdv_p, PDV_CLSIM_TAP2START, tap2start);
    edt_reg_write(pdv_p, PDV_CLSIM_TAP2DELTA, tap2delta);

    edt_reg_write(pdv_p, PDV_CLSIM_TAP3START, tap3start);
    edt_reg_write(pdv_p, PDV_CLSIM_TAP3DELTA, tap3delta);

}


/**
 * Initializes simulator values based on PdvDependent structure in \c pdv_p.
 * The structure is normally filled in by clsiminit. Assumes bitfile already loaded.
 *
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 */
    int
pdv_cls_set_dep(PdvDev *pdv_p)

{
    PdvDependent *dd_p = (PdvDependent *) pdv_p->dd_p;
    ClSimControl *cls = &dd_p->cls;
    int rgb = pdv_p->dd_p->cl_cfg & PDV_CL_CFG_RGB;

    edt_startdma_action(pdv_p, EDT_ACT_NEVER);

    pdv_cls_sim_stop(pdv_p);

    /* set clock  0 means use onboard clock*/
    if ((cls->pixel_clock != 0) && ((cls->pixel_clock < 19.9) || (cls->pixel_clock > 85.1)))
        printf("WARNING: %3.4f is not in range of 20 to 85 MHz\n", cls->pixel_clock);

    /* set size params */
    pdv_cls_set_depth(pdv_p, dd_p->depth);
    pdv_cls_set_width_lval_rval(pdv_p, dd_p->width, cls->hblank, cls->Hlvstart, cls->Hlvend, cls->Hrvstart, cls->Hrvend);
    pdv_cls_set_height(pdv_p, dd_p->height, cls->vblank);
    
    pdv_set_cam_width(pdv_p, dd_p->width);
    pdv_set_cam_height(pdv_p, dd_p->height);

    pdv_check_fpga_rev(pdv_p);
    pdv_set_baud(pdv_p, dd_p->serial_baud);

/* MODIFIED 9/10/2012, set taps to 1 for RGB based on depth/extdepth now, as opposed to
 * before when we did it based on the RGB bit. And leave datapath as-is most other cases.
 * If we want to fake RGB using 4 taps we now have to set cl_data_path* explicitly to 0x37
 * in cfg whereas before it did it here any time we saw RGB.  So... make sure users get
 * updated FW!!! Otherwise for RGB 24-bit they'll need to have a different cfg for FG vs SIM
 */
    if ((pdv_p->dd_p->depth == 24) && (pdv_p->dd_p->extdepth == 24))
        cls->taps = 1;
    else if (cls->taps == 0)
        cls->taps = (dd_p->cl_data_path >> 4) + 1;
    else if (dd_p->cl_data_path == 0)
        dd_p->cl_data_path = ((cls->taps - 1) << 4) + 7;

    if (cls->hblank == 0 && cls->Hcntmax == 0)
        cls->hblank = PDV_CLS_DEFAULT_HGAP;

    if (cls->vblank == 0 && cls->Vcntmax == 0)
        cls->vblank = PDV_CLS_DEFAULT_VGAP;

    pdv_cls_set_size(pdv_p,
            cls->taps,
            dd_p->depth,
            dd_p->width,
            dd_p->height,
            cls->hblank, cls->Hcntmax,
            cls->vblank, cls->Vcntmax);

    pdv_cls_set_line_timing(pdv_p,
            dd_p->width,
            cls->taps,
            cls->Hfvstart,
            cls->Hfvend,
            cls->Hlvstart,
            cls->Hlvend,
            cls->Hrvstart,
            cls->Hrvend);

    /* special case with rvstart/end superseding width */

    if ((cls->Hrvend - cls->Hrvstart) > dd_p->width)
    {
        int rvwidth = cls->Hrvend - cls->Hrvstart;

        if (rvwidth > cls->Hlvend - cls->Hlvstart)
        {
            cls->Hlvstart = cls->Hrvstart;
            cls->Hlvend = cls->Hrvend;
        }
        pdv_cls_set_width_lval_rval(pdv_p, rvwidth, cls->hblank, cls->Hlvstart, cls->Hlvend, cls->Hrvstart, cls->Hrvend);
    }

    /* set cfg values */

    pdv_cls_set_linescan(pdv_p, cls->flags.Cfg.linescan);
    pdv_cls_set_lvcont(pdv_p, cls->flags.Cfg.lvcont);
    pdv_cls_set_rven(pdv_p, cls->flags.Cfg.rven);
    pdv_cls_set_uartloop(pdv_p, cls->flags.Cfg.uartloop);
    pdv_cls_set_smallok(pdv_p, cls->flags.Cfg.smallok);
    pdv_cls_set_intlven(pdv_p, cls->flags.Cfg.intlven);
    pdv_cls_set_firstfc(pdv_p, cls->flags.Cfg.firstfc);
    pdv_cls_set_datacnt(pdv_p, cls->flags.Cfg.datacnt);

    pdv_cls_set_dvalid(pdv_p, cls->flags.Cfg.dvskip, cls->flags.Cfg.dvmode);

    pdv_cls_set_led(pdv_p, cls->flags.Cfg.led);
    pdv_cls_set_trigsrc(pdv_p, cls->flags.Cfg.trigsrc);
    pdv_cls_set_trigpol(pdv_p, cls->flags.Cfg.trigpol);
    pdv_cls_set_trigframe(pdv_p, cls->flags.Cfg.trigframe);
    pdv_cls_set_trigline(pdv_p, cls->flags.Cfg.trigline);

    pdv_cls_set_fill(pdv_p,cls->FillA, cls->FillB);

    edt_reg_write(pdv_p, PDV_CLSIM_EXSYNCDLY, cls->Exsyncdly);

    if (dd_p->n_intlv_taps == 4)
    {
        pdv_cls_setup_interleave(pdv_p,
                dd_p->intlv_taps[0].startx,
                dd_p->intlv_taps[0].dx,
                dd_p->intlv_taps[1].startx,
                dd_p->intlv_taps[1].dx,
                dd_p->intlv_taps[2].startx,
                dd_p->intlv_taps[2].dx,
                dd_p->intlv_taps[3].startx,
                dd_p->intlv_taps[3].dx);
    } 
    else
    {
        pdv_cls_set_intlven(pdv_p, 0);
    }

    pdv_cls_sim_start(pdv_p);

    pdv_cls_set_clock(pdv_p, cls->pixel_clock);

    pdv_do_xregwrites(pdv_p, dd_p);

    return 0;

}

/**
 * Checks for inconsistencies in the configuration (stub). 
 *
 * Currently this is a stub. In the future it will return a nonzero error code if a
 * problem is found.
 *
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @return 0 if ok, otherwise error code
 *
 * @see pdv_cls_set_dep
 */
    int
pdv_cls_dep_sanity_check(PdvDev *pdv_p)

{
    return 0;
}

/**
 * Computes and returns the frame time in milliseconds.
 * 
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @return the computed frame time
 */
    double
pdv_cls_frame_time(PdvDev *pdv_p)

{
    double clock = pdv_p->dd_p->cls.pixel_clock;

    if (clock != 0.0)
    {
        double msecs; /* msecs per clock */
        double frameclocks, lineclocks;

        msecs = 1.0 / (clock * 1000.0);

        lineclocks = edt_reg_read(pdv_p, PDV_CLSIM_HCNTMAX) * msecs;

        frameclocks = edt_reg_read(pdv_p, PDV_CLSIM_VCNTMAX) * lineclocks;

        return frameclocks;
    }

    return 0.0;
}

/** Prints the board state to stdout. 
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @return void
 */
    void
pdv_cls_dump_state(PdvDev *pdv_p)

{
    int val;
    double clock;
    val = edt_reg_read(pdv_p, PDV_CL_DATA_PATH);

    printf("Taps:   %d\n", (val >> 4) + 1);

    val = edt_reg_read(pdv_p, PDV_CLSIM_CFGA);
    printf("CfgA:\n");
    printf("	linescan:   %s\n", (val & 0x80)? "enabled":"off");
    printf("	lvcont  :   %s\n", (val & 0x40)? "enabled":"off");
    printf("	rven    :   %s\n", (val & 0x20)? "enabled":"off");
    printf("	uartloop:   %s\n", (val & 0x10)? "enabled":"off");
    printf("	smallok :   %s\n", (val & 8)? "enabled":"off");
    printf("	intlven :   %s\n", (val & 4)? "enabled":"off");
    printf("	firstfc :   %s\n", (val & 2)? "enabled":"off");
    printf("	datacnt :   %s\n", (val & 1)? "enabled":"off");
    val = edt_reg_read(pdv_p, PDV_CLSIM_CFGB);
    printf("CfgB:\n");
    printf("	dvskip  :   %d\n", (val >> 4));
    printf("	dvmode  :   %d\n", (val & 3));
    val = edt_reg_read(pdv_p, PDV_CLSIM_CFGC);
    printf("CfgC:\n");
    printf("	led     :   %s\n", (val & 0x80)? "enabled":"off");
    printf("	trigsrc :   %s\n", (val & 8)? "enabled":"off");
    printf("	trigpol :   %s\n", (val & 4)? "enabled":"off");
    printf("	trigfram:   %s\n", (val & 2)? "enabled":"off");
    printf("	trigline:   %s\n", (val & 1)? "enabled":"off");

    printf("\nFillA   :  0x%x\n", edt_reg_read(pdv_p, PDV_CLSIM_FILLA));
    printf("FillB   :  0x%x\n\n", edt_reg_read(pdv_p, PDV_CLSIM_FILLB));


    val = edt_reg_read(pdv_p, PDV_CLSIM_HCNTMAX);
    printf("Hcntmax :  %d clocks\n", val+1);

    val = edt_reg_read(pdv_p, PDV_CLSIM_VACTV);
    printf("Vactv   :  %d lines\n", val+1);

    val = edt_reg_read(pdv_p, PDV_CLSIM_VCNTMAX);
    printf("Vcntmax :  %d lines\n", val+1);

    printf("Hfvstart:  %d\n", edt_reg_read(pdv_p, PDV_CLSIM_HFVSTART));
    printf("Hfvend  :  %d\n", edt_reg_read(pdv_p, PDV_CLSIM_HFVEND));
    printf("Hlvstart:  %d\n", edt_reg_read(pdv_p, PDV_CLSIM_HLVSTART));
    printf("Hlvend  :  %d\n", edt_reg_read(pdv_p, PDV_CLSIM_HLVEND));
    printf("Hrvstart:  %d\n", edt_reg_read(pdv_p, PDV_CLSIM_HRVSTART));
    printf("Hrvend  :  %d\n", edt_reg_read(pdv_p, PDV_CLSIM_HRVEND));

    printf("hblank    :  %d\n", pdv_cls_get_hgap(pdv_p));
    printf("vblank    :  %d\n", pdv_cls_get_vgap(pdv_p));

    if (edt_reg_read(pdv_p, PDV_CLSIM_CFGA) & 4)
    {
        printf("\nInterleave\n");

        printf("Tap 0 : Start %4d Delta %4d\n", edt_reg_read(pdv_p, PDV_CLSIM_TAP0START),
                edt_reg_read(pdv_p, PDV_CLSIM_TAP0DELTA));
        printf("Tap 1 : Start %4d Delta %4d\n", edt_reg_read(pdv_p, PDV_CLSIM_TAP1START),
                edt_reg_read(pdv_p, PDV_CLSIM_TAP1DELTA));
        printf("Tap 2 : Start %4d Delta %4d\n", edt_reg_read(pdv_p, PDV_CLSIM_TAP2START),
                edt_reg_read(pdv_p, PDV_CLSIM_TAP2DELTA));
        printf("Tap 3 : Start %4d Delta %4d\n", edt_reg_read(pdv_p, PDV_CLSIM_TAP3START),
                edt_reg_read(pdv_p, PDV_CLSIM_TAP3DELTA));
    }

    clock = pdv_p->dd_p->cls.pixel_clock;

    if (clock != 0.0)
    {
        double msecs; /* msecs per clock */
        double frameclocks, lineclocks;

        printf("\nPixel clock       : %10.4f MHz\n", pdv_p->dd_p->cls.pixel_clock);

        msecs = 1.0 / (clock * 1000.0);

        lineclocks = edt_reg_read(pdv_p, PDV_CLSIM_HCNTMAX) * msecs;
        printf("Line time         : %10.4f msecs\n", lineclocks);

        frameclocks = edt_reg_read(pdv_p, PDV_CLSIM_VACTV) * lineclocks;
        printf("Active frame time : %10.4f msecs\n", frameclocks);

        frameclocks = edt_reg_read(pdv_p, PDV_CLSIM_VCNTMAX) * lineclocks;
        printf("Frame time        : %10.4f msecs\n", frameclocks);

        if (frameclocks)
            printf("Frame rate        : %10.4f fps\n",1000.0 / frameclocks);

    }
}


/**
 * Prints board geometry only to stdout.
 *
 * @param pdv_p	pointer to pdv device structure returned by #pdv_open
 * @return void
 */
    void
pdv_cls_dump_geometry(PdvDev *pdv_p)
{
    int val;
    val = edt_reg_read(pdv_p, PDV_CL_DATA_PATH);

    printf("\nFillA   :  0x%x\n", edt_reg_read(pdv_p, PDV_CLSIM_FILLA));
    printf("FillB   :  0x%x\n\n", edt_reg_read(pdv_p, PDV_CLSIM_FILLB));


    val = edt_reg_read(pdv_p, PDV_CLSIM_HCNTMAX);
    printf("Hcntmax :  %d clocks\n", val+1);

    val = edt_reg_read(pdv_p, PDV_CLSIM_VACTV);
    printf("Vactv   :  %d lines\n", val+1);

    val = edt_reg_read(pdv_p, PDV_CLSIM_VCNTMAX);
    printf("Vcntmax :  %d lines\n", val+1);

    printf("Hfvstart:  %d\n", edt_reg_read(pdv_p, PDV_CLSIM_HFVSTART));
    printf("Hfvend  :  %d\n", edt_reg_read(pdv_p, PDV_CLSIM_HFVEND));
    printf("Hlvstart:  %d\n", edt_reg_read(pdv_p, PDV_CLSIM_HLVSTART));
    printf("Hlvend  :  %d\n", edt_reg_read(pdv_p, PDV_CLSIM_HLVEND));
    printf("Hrvstart:  %d\n", edt_reg_read(pdv_p, PDV_CLSIM_HRVSTART));
    printf("Hrvend  :  %d\n", edt_reg_read(pdv_p, PDV_CLSIM_HRVEND));

    printf("hblank    :  %d\n", pdv_cls_get_hgap(pdv_p));
    printf("vblank    :  %d\n", pdv_cls_get_vgap(pdv_p));

}


