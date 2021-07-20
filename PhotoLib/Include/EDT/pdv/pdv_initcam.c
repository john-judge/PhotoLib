/**
* @file
* Functions to initialize the device driver, EDT digital video board, and 
* camera device.
* 
* (C) 1997-2007 Engineering Design Team, Inc.
*/
#include "edtinc.h"

#include "libedt_timing.h"
#include "pdv_irig.h"

/* shorthand debug level */
#define INITCAM_MSG_ALWAYS EDT_MSG_FATAL

/*
* static prototypes
*/
int     is_hex_byte_command(char *str);
void    check_terminator(char *str);
void    dir_from_path(char *dirstr, const char *pathstr);
void    fname_from_path(char *fname, char *pathstr);
int	async_bitfile(EdtDev *edt_p);
char   *grepit(char *buf, char *pat);
int     is_hex_byte_command(char *str);
int     findid(char *str, char *idstr);
char   *strip_crlf(char *str);
void    propeller_sleep(int n);
char   *search_for_bitfile(char *rbtfile, const char *cfgfname, char *bitpath);
int     init_serial(EdtDev * edt_p, Dependent * dd_p, Edtinfo *ei_p);
int     init_serial_basler_binary(EdtDev * edt_p, Dependent * dd_p, int delay, char *serial_init);
int     init_serial_duncan_binary(EdtDev * edt_p, Dependent * dd_p, int delay, char *serial_init);
int     init_serial_binary(EdtDev * edt_p, Dependent * dd_p, int delay, char *serial_init);
int     init_serial_ascii(EdtDev * edt_p, Dependent * dd_p, int delay, char *serial_init);
int     send_xilinx_commands(EdtDev * edt_p, char *str);
int     check_roi(EdtDev * edt_p, Dependent * dd_p, int ena);
void    check_camera_values(EdtDev * ed, Dependent * dd_p);
int     kodak_query_serial(EdtDev * edt_p, char *cmd, int *val);
int     specinst_download(EdtDev * edt_p, char *fname);
int     specinst_setparams(EdtDev * edt_p, char *fname);
void	setup_cl2_simulator(EdtDev *edt_p, Dependent *dd_p);
char   *serial_tx_rx(PdvDev * pdv_p, char *command, int hexin);
static int set_rci = 0;
static int rci_set_unit = 0;
static long    isascii_str(u_char * buf, int n);

void
pdv_initcam_set_rci(EdtDev * edt_p, int rci_unit)
{
    rci_set_unit = rci_unit;
    set_rci = 1;
}

/*
* initcam internal function -- assumes dependent struct has already been loaded from
* pdv_readcfg() or equivalent.
*/

int
pdv_channel_initialized(int unit, int channel)

{
    PdvDev *pdv_p;

    pdv_p = pdv_open_channel(EDT_INTERFACE,unit, channel);

    if (pdv_p && pdv_get_width(pdv_p) && pdv_get_height(pdv_p))
    {
        return 1;
    }

    return 0;
}

/**
* Initializes the framegrabber board and camera. This is the "guts" of the
* \e inticam program that gets executed to initialize when you choose a
* camera. The library subroutine is provided for programmers who wish
* to incorporate the initialization procedure into their own
* applications.
* 
* @note unlike other pdv library calls, pdv_inticam requires an edt device pointer returned from
* #edt_open or #edt_open_channel. After initializing, close the device with #edt_close before reopening
* with #pdv_open_channel or #pdv_open for futher use.
*
* @note pdv_initcam is designed to initialize EDT framegrabber (input) boards only. For simulator
* boards, (e.g. the PCIe8 DVa CLS) see the clsiminit.c example/utility application.
*
* @param pdv_p   pointer to edt device structure returned by #edt_open 
* @param dd_p    pointer to a previously allocated (via
* #pdv_alloc_dependent) and initialized (through #pdv_readcfg)
* dependent structure. The library uses this until it is either freed
* by #edt_close, or no longer used by later calls to this function
* (which means that if you call pdv_initcam again, you should free()
* pdv_p->dd_p first to avoid memory leaks).
* @param unit    unit number of the device. The first unit is 0.
* @param edtinfo miscellaneous variable information structure, defined
* in initcam.h, initialized via #pdv_readcfg.
* @param cfgfname path name of configuration file.
* @param bitdir  directory path name for .bit (FPGA) files. If NULL,
* pdv_initcam will search for bitfiles under ".", then
* "./camera_config/bitfiles".
* @param pdv_debug should be set to 0 (but is ignored currently).
*
* @return 0 on success, -1 on failure
*
* @Example
* @note The following is simplified example code. Normally, we would
* check the return values and handle error conditions. See \e initcam.c
* for a complete example of reading the configuration file and
* configuring the pdv device driver and camera.
*
* @code 
* Dependent *dd_p;
* Edtinfo ei_p;
* EdtDev *edt_p;
* int unit, channel;
* char* unitstr = argv[1];
*
* dep = pdv_alloc_dependent();
* pdv_readcfg(cfgfname, dd_p, &edtinfo);
* unit = edt_parse_unit_channel(unitstr, edt_devname, "pdv", &channel);
* edt_p = edt_open_channel(edt_devname, unit, channel);
* pdv_initcam(edt_p, dd_p, unit, &ei_p, cfgfname, bitdir, 0);
* edt_close(edt_p);
* free(dd_p);
* @endcode
*
* @see pdv_readcfg, initcam.c source code
*
* @ingroup init
*/
int
pdv_initcam(EdtDev * pdv_p, Dependent * dd_p, int unit, Edtinfo * ei_p,
            const char *cfgfname, char *bitdir, int pdv_debug)
{

    char errmsg[256];

    /* make sure the device is open and valid */
    if ((pdv_p == NULL) || (dd_p == NULL) || (pdv_p->fd == 0))
    {
        edt_msg(EDT_MSG_FATAL, "ERROR: invalid dependent struct pointer\n");
        return -1;
    }

#if 0 /* this doesn't work since is_simulator includes PCIe8 and PCIe4 even if they're not configured as a simulator -- need a way to detect that!!! */
    if (edt_is_simulator(pdv_p))
    {
        edt_msg(EDT_MSG_FATAL, "ERROR: this is a simulator (output) board -- use clsimit to initialize\n");
        return -1;
    }
#endif

    /*
    * reality check on some critical input variables
    */
    if ((dd_p->camera_class[0] == '\0')
        || (dd_p->width == 0)
        || (dd_p->height == 0)
        || (dd_p->depth < 8)
        || (dd_p->depth > 32)
        || (dd_p->extdepth < 8)
        || (dd_p->extdepth > 32)
        || (dd_p->depth & 1)
        || (dd_p->extdepth & 1))
    {
        edt_msg(PDVLIB_MSG_WARNING, "WARNING: one or more critical config values appear invalid\n");
        sprintf(errmsg, "check readcfg/dependent struct and camera_class, width, height, depth, extdepth\n");
        edt_msg(DEBUG2, errmsg);
    }

    /*
    * pdv_p->dd_p is not necessarily used here, and is normally set by
    * pdv_open, but initcam is a special case
    */
    if (pdv_p->dd_p == 0) /* must've been opened with edt_open (or separately freed?) */
        pdv_p->dd_p = dd_p;
    else if (pdv_p->dd_p != dd_p)
        memcpy(pdv_p->dd_p, dd_p, sizeof(Dependent));

    /* IMPORTANT: use pdv_p->dd_p not just dd_p from here on out in this function */

    if (pdv_p->devid == PDVFOI_ID)
    {
        edt_msg(EDT_MSG_FATAL, "FOI not supported after v4.1.5.9\n");
    }
    else
    {
        edt_reset_serial(pdv_p);

        if (pdv_p->dd_p->rbtfile[0])
        {
            /* no bitload if a single FPGA */
            if (edt_has_combined_fpga(pdv_p) )
            {
                edt_msg(DEBUG1, "Combined FPGA, skipping device FPGA load\n");
            }
            else if (strcmp(pdv_p->dd_p->rbtfile, "_SKIPPED_") == 0)
                edt_msg(INITCAM_MSG_ALWAYS, "skipping bitload\n");
            else if (edt_is_dvfox(pdv_p) && pdv_p->channel_no > 0)
            {
                edt_msg(DEBUG1, "DV FOX channel %d skipping bit load\n",
                    pdv_p->channel_no);

                if (!pdv_channel_initialized(pdv_p->unit_no, 0))
                {
                    edt_msg(EDT_MSG_FATAL, "You must initialize channel 0 before initializing any other channels on the DV-FOX\n");
                }
            }
            else if (pdv_initcam_load_bitfile(pdv_p, pdv_p->dd_p, unit, bitdir, cfgfname))
            {
                edt_msg(EDT_MSG_FATAL, "ERROR: Failed bitload\n");
                return -1;
            }
        }
        else edt_msg(DEBUG1, "No bitfile specified, skipping device FPGA load\n");

        if (pdv_initcam_reset_camera(pdv_p, pdv_p->dd_p, ei_p))
        {
            edt_msg(EDT_MSG_FATAL, "ERROR: Failed camera reset\n");
            return -1;
        }
    }

    return 0;
}



/** Allocates a dependent structure, for use by #pdv_readcfg and #pdv_initcam,
* and checks for and reports error conditions as a result of the alloc.
* The structure can be deallocated with free() later.  
* @return pointer to a Dependent structure (defined in camera.h).
* @see #pdv_initcam, initcam.c and camera.h source files.
* @ingroup init 
*/
Dependent *
pdv_alloc_dependent()
{
    int     dsize = sizeof(Dependent);

    /*
    * check dependent size, not over limit allocated for it
    */
    if (dsize > EDT_DEPSIZE)
    {
        edt_msg(EDT_MSG_FATAL, "libpdv internal error: bad dependent struct size (%d) s/b %d\n", dsize, EDT_DEPSIZE);
        return NULL;
    }

    return (Dependent *) calloc(EDT_DEPSIZE, 1);
}


static void
dep_wait(EdtDev * edt_p)
{
}

/*
* isapromaddress -- must be XXXXX.mmmm-YYYYY.mmmmo, where
* XXXXX and YYYYY are 5-digit (or more) hex addresses, and mmmm and nnnnn
* are the respective sizes in decimal  to download
*/
int
get_prom_addrs(char *str, u_int *addr1, int *size1, u_int *addr2, int *size2)
{

    if (str == NULL)
        return 0;

    if (sscanf(str, "%x.%d-%x.%d", addr1, size1, addr2, size2) != 4)
    {
        *addr1 = 0;
        *addr2 = 0;
        *size1 = 0;
        *size2 = 0;
        return -1;
    }
    return 0;
}

int
pdv_initcam_load_bitfile(EdtDev * edt_p,
                         Dependent * dd_p,
                         int unit,
                         char *bitdir,
                         const char *cfgfname)
{
    char    dir_arg[256];
    char    cfgdir[256];
    int     emb = 0;
    int     ret, flags=0;
#ifdef NO_FS
    char bitname[128];
    int  len;
#endif

    cfgdir[0] = '\0';

    edt_msg(DEBUG2, "pdv_initcam_load_bitfile('%s')\n", dd_p->rbtfile);

    if (strcmp(bitdir, "_NOFS_") == 0)
    {
        emb = 1;
        flags = BITLOAD_FLAGS_NOFS;
    }


    if (edt_p->devid == DMY_ID)
    {
#if 0
        edt_flush_fifo(edt_p);
#endif
        dd_p->serial_respcnt = 4;
    }
    else
    {
        u_int addr1, addr2;
        int size1, size2;

        dd_p->serial_respcnt = 2;
        /*
        * if -b flag specified, send "-b dirname". if not, strip off leading
        * dir from filename and if that exists, send -d dirname otheriwise
        * just send "-d camera_config" and hope there's a
        * camera_config/bitfiles
        */

        if (get_prom_addrs(dd_p->rbtfile, &addr1, &size1, &addr2, &size2) == 0)
        {

            edt_msg(DEBUG1, "loading device FPGA from PROM @ %x %d / %x %d", addr1, size1, addr2, size2);
            ret = edt_bitload_from_prom(edt_p, addr1, size1, addr2, size2, flags);
            printf("\n");
        }
        else
        {
#ifdef NO_FS
            if (emb)
            {
                /* strip off .bit from name if nofs (embedded) FPGA */
                strcpy(bitname, dd_p->rbtfile);
                len = strlen(bitname);
                if ((len >= 4) && (strcasecmp(&bitname[len-4], ".bit") == 0))
                    bitname[len-4] = '\0';
                edt_msg(EDT_MSG_INFO_1, "loading embedded camera fpga %s....\n", bitname);
                ret = edt_bitload(edt_p, bitdir, bitname, 1, 0);
                if (edt_is_dvfox(edt_p))
                    edt_msleep(500);
                edt_flush_fifo(edt_p);
                return ret;
            }
#endif
            if (!emb)
            {
                if (*bitdir)
                    strcpy(dir_arg, bitdir);
                else
                {
                    dir_from_path(cfgdir, cfgfname);
                    sprintf(bitdir, "%s/bitfiles", cfgdir);

                    if ((!(*cfgdir)) || (pdv_access(bitdir, 0) != 0))
                        strcpy(cfgdir, "camera_config");

                    strcpy(dir_arg, cfgdir);
                }
            }

            edt_msg(DEBUG1, "loading camera fpga %s....\n", dd_p->rbtfile);

            if ((ret = edt_bitload(edt_p, dir_arg, dd_p->rbtfile, 0, 0)) != 0)
                return ret;
        }

        if (edt_is_dvfox(edt_p))
            edt_msleep(500);

        edt_flush_fifo(edt_p);
    }

    return 0;
}



/**
 * @internal
 * Set up registers on the board and send camera setup serial if indicated
 */
int
pdv_initcam_reset_camera(EdtDev * edt_p, Dependent * dd_p, Edtinfo * ei_p)
{
    int     ret = 0;
    int     data_path = 0;

    if (dd_p->xilinx_rev == NOT_SET)
    {
        pdv_check_fpga_rev(edt_p);

        /*
        * need to let the driver know the rev in dep_set/get since register
        * offsets changed in rev 2
        */
        edt_set_dependent(edt_p, dd_p);
    }

    pdv_set_baud(edt_p, dd_p->serial_baud);

    /*
    * infer dual channel from other flags
    */
    if (pdv_is_cameralink(edt_p)
        && (dd_p->cl_data_path & 0x10)
        && (!(dd_p->cl_cfg & PDV_CL_CFG_RGB)))
        dd_p->dual_channel = 1;

    if (!(edt_is_simulator(edt_p) && dd_p->sim_enable))
    {
        int roi_enabled = (dd_p->cl_cfg & PDV_CL_CFG_ROIDIS)?0:1;

        /*
        * set width/height.  Width and height as set in config file should be
        * actual camera width and height, before any ROI changes. ROI changes
        * will change dd_p->width/height but not dd_p->cam_width/height.
        */
        edt_msg(DEBUG1, "setting device defaults....\n");
        pdv_set_cam_width(edt_p, dd_p->width);
        pdv_set_cam_height(edt_p, dd_p->height);
        pdv_set_depth_extdepth_dpath(edt_p, dd_p->depth, dd_p->extdepth, dd_p->cl_data_path);

        /* since setsize and set width/height also set ROI now... */
        if (roi_enabled)
        {
            if (dd_p->hactv > 0)
                dd_p->width = dd_p->hactv;
            if (dd_p->vactv > 0)
                dd_p->height = dd_p->vactv;
        }

        pdv_set_width(edt_p, dd_p->width);
        pdv_set_height(edt_p, dd_p->height);

        check_roi(edt_p, dd_p, roi_enabled);

        if (dd_p->sim_enable) /* old simulator */
        {
            u_int   roictl;

            roictl = edt_reg_read(edt_p, PDV_ROICTL);
            /* default to simulated not from camera */
            if (!dd_p->sim_ctl)
                roictl |= (PDV_ROICTL_SIM_DAT | PDV_ROICTL_SIM_SYNC);
            else
                roictl |= dd_p->sim_ctl | PDV_ROICTL_SIM_DAT;
            edt_msg(DEBUG1, "setting simulator bits in roictl %x\n", roictl);
            edt_reg_write(edt_p, PDV_ROICTL, roictl);
        }
    }

    edt_set_dependent(edt_p, dd_p);

    edt_msg(DEBUG1, "setting registers....\n");
    if (dd_p->genericsim)
    {
        edt_msg(DEBUG1, "setting up for simulator....\n");
        edt_reg_write(edt_p, SIM_LDELAY, dd_p->line_delay);
        edt_reg_write(edt_p, SIM_FDELAY, dd_p->frame_delay);
        edt_reg_write(edt_p, SIM_WIDTH, dd_p->sim_width - 1);
        edt_reg_write(edt_p, SIM_HEIGHT, dd_p->sim_height - 1);
        edt_reg_write(edt_p, SIM_CFG, dd_p->genericsim);


        edt_msg(DEBUG2, "SIM_CFG %x\n", dd_p->genericsim);
        switch (dd_p->sim_speed)
        {
        case 0:
            edt_msg(DEBUG2, "starting pixel clock at 5Mhz\n");
            edt_reg_write(edt_p, SIM_SPEED, 0);
            break;
        case 1:
            edt_msg(DEBUG2, "starting pixel clock at 10Mhz\n");
            edt_reg_write(edt_p, SIM_SPEED, 1);
            break;
        case 2:
            edt_msg(DEBUG2, "starting pixel clock at 20Mhz\n");
            edt_reg_write(edt_p, SIM_SPEED, 2);
            break;
        }
    }


    /* Configuration register settings (except CL2 simulator */
    if (!(edt_is_simulator(edt_p) && dd_p->sim_enable))
    {
        int     configuration = 0;

        if (dd_p->inv_shutter)
            configuration |= PDV_INV_SHUTTER;
        else
            configuration &= ~PDV_INV_SHUTTER;

        /*
        * if MODE_CNTL_NORM is not set, set default based on value
        * of camera_shutter timing
        */
        if (dd_p->mode_cntl_norm == NOT_SET)
        {
            if ((dd_p->camera_shutter_timing == AIA_MCL)
                || (dd_p->camera_shutter_timing == AIA_MCL_100US)
                || (dd_p->camera_shutter_timing == AIA_TRIG))
                dd_p->mode_cntl_norm = 0x10;
            else dd_p->mode_cntl_norm = 0;
        }

        /*
        * set default values for shutter_speed_min/max if using board shutter
        * timer and no min/max specified
        */
        if ((dd_p->camera_shutter_timing == AIA_MCL)
            || (dd_p->camera_shutter_timing == AIA_MCL_100US))
        {
            int exp;

            if ((dd_p->shutter_speed_min == 0) && (dd_p->shutter_speed_max == 0))
            {
                dd_p->shutter_speed_min = 0;
                dd_p->shutter_speed_max = 25500;
            }

            exp = pdv_get_exposure(edt_p);
            if (exp < dd_p->shutter_speed_min || exp > dd_p->shutter_speed_max)
                pdv_set_exposure(edt_p, dd_p->shutter_speed_min);
        }

        if (dd_p->camera_shutter_timing == AIA_SERIAL)
            dd_p->trig_pulse = 1;

        if (dd_p->trig_pulse || dd_p->camera_shutter_timing == AIA_SERIAL)
            configuration |= PDV_TRIG;
        else
            configuration &= ~PDV_TRIG;

        if (dd_p->dis_shutter)
            configuration |= PDV_DIS_SHUTTER;
        else
            configuration &= ~PDV_DIS_SHUTTER;

        if (dd_p->enable_dalsa)
            configuration |= PDV_EN_DALSA;
        else
            configuration &= ~PDV_EN_DALSA;

        edt_msg(DEBUG2, "CONFIG %x\n", configuration);
        dep_wait(edt_p);
        edt_reg_write(edt_p, PDV_CFG, configuration);
    }

    /* util3 register settings */
    {
        int     util3 = 0;

        if (dd_p->inv_ptrig)
            util3 |= PDV_LV_INVERT;
        else
            util3 &= ~PDV_LV_INVERT;

        if (dd_p->inv_fvalid)
            util3 |= PDV_FV_INVERT;
        else
            util3 &= ~PDV_FV_INVERT;

        /*
        * set dvfox mode16 automatically (ON if 8 bits single or dual channel
        * or 8-16 bits single channel, otherwise off), unless specifically set
        * in the config file
        */
        if (edt_is_dvfox(edt_p))
        {
            if (dd_p->mode16 == NOT_SET)
            {
                if ((!(dd_p->cl_cfg & PDV_CL_CFG_RGB))
                    && (((dd_p->cl_data_path & 0x0f) == 0x07)
                    || (dd_p->cl_data_path <= 0x0f)))
                    util3 |= PDV_MODE16;
            }
            else if (dd_p->mode16)
                util3 |= PDV_MODE16;
        }


        edt_msg(DEBUG2, "UTIL3 %x\n", util3);
        dep_wait(edt_p);

        edt_reg_write(edt_p, PDV_UTIL3, util3);
    }

    pdv_set_fval_done(edt_p, dd_p->fval_done);

    /* old initcam for irc was this */
    if (dd_p->camera_download == IRC_160)
    {
        int     tmp;

        edt_msg(DEBUG1, "doing old initcam method for IRC160");
#if 0
        tmp = edt_reg_read(edt_p, PDV_CFG);	/* inverse shutter, not
                                            * trigger */
        tmp &= ~(PDV_INV_SHUTTER | PDV_TRIG);
#endif
        tmp = 0;
        if (dd_p->inv_shutter)
            tmp |= PDV_INV_SHUTTER;
        if (dd_p->camera_shutter_timing != AIA_MCL)
            tmp |= PDV_TRIG;
        dep_wait(edt_p);
        edt_reg_write(edt_p, PDV_CFG, tmp);
        edt_msg(DEBUG2, "CONFIG %x", tmp);
    }

    /* Data Path register settings (and camera link config/cntl) */
    if (pdv_is_cameralink(edt_p))
    {
        edt_msg(DEBUG2, "camera link cfg register %x\n", dd_p->cl_cfg);
        edt_reg_write(edt_p, PDV_CL_CFG, dd_p->cl_cfg);

        edt_msg(DEBUG2, "camera link cfg2 register %x\n", dd_p->cl_cfg2);
        edt_reg_write(edt_p, PDV_CL_CFG2, dd_p->cl_cfg2);

        edt_msg(DEBUG2, "camera link data_path register %x\n", dd_p->cl_data_path);
        edt_reg_write(edt_p, PDV_CL_DATA_PATH, dd_p->cl_data_path);

        if (dd_p->cl_hmax != 0x0)
        {
            edt_msg(DEBUG2, "camera link hmax register %x\n", dd_p->cl_hmax);
            edt_reg_write(edt_p, PDV_CL_HMAX, dd_p->cl_hmax);
        }

        if (dd_p->swinterlace == PDV_BGGR
            || dd_p->swinterlace == PDV_ILLUNIS_BGGR)
        {
            /* kludge for now */
        }
        else if (dd_p->depth > 8)
            data_path |= PDV_EXT_DEPTH;

        if (dd_p->camera_shutter_timing == AIA_MCL_100US)
            data_path |= PDV_MULTIPLIER_100US;

        if (dd_p->fv_once)
            data_path |= PDV_CONTINUOUS;

        if (dd_p->trigdiv)
            edt_reg_write(edt_p, PDV_TRIGDIV, dd_p->trigdiv);

        /* used hwpad to set 30-bit RGB in PDV and maybe PCIe4 medium mode FW but
         * not PCIe8 or any future boards (better not!) */
        if (dd_p->rgb30)
            if (edt_p->devid < PE8DVCL_ID)
                dd_p->hwpad = dd_p->rgb30;

        dep_wait(edt_p);
        edt_reg_write(edt_p, PDV_DATA_PATH, data_path);
        dd_p->datapath_reg = data_path;

        /* set up framesync */
        pdv_enable_framesync(edt_p, edt_p->dd_p->framesync_mode);

        if (ei_p->cl_mgtspeed != NOT_SET)
            edt_reg_write(edt_p, PDV_CL_MGTSPEED, ei_p->cl_mgtspeed);

        edt_set_dependent(edt_p, dd_p);
    }
    else
    {
        if (dd_p->camera_shutter_timing == DALSA_CONTINUOUS)
            dd_p->continuous = 1;
#if 0
        /* don't turn on until in driver */
        if (dd_p->continuous)
            data_path |= PDV_CONTINUOUS;
        else
            data_path &= ~PDV_CONTINUOUS;
#endif

        if (dd_p->fv_once)
            data_path |= PDV_CONTINUOUS;

        if (dd_p->interlace)
            data_path |= PDV_INTERLACED;
        else
            data_path &= ~PDV_INTERLACED;

        edt_msg(DEBUG2, "data_path register %x\n", data_path);

        /*
        * if async we set the board generated pixel clock speed in
        * roictl register (added async check so we can use pclock_speed
        * for auto timeout and other stuff 08/06 rwh)
        */
        if (async_bitfile(edt_p))
        {
            u_int bits = 0;
            u_int roictl = edt_reg_read(edt_p, PDV_ROICTL);

            if (dd_p->pclock_speed < 5)
                dd_p->pclock_speed = 5;

            switch(dd_p->pclock_speed)
            {
            case 20: bits = 0x04; break;
            case 10: bits = 0x05; break;
            case 5: bits = 0x06; break;
            default: edt_msg(PDVLIB_MSG_WARNING, "invalid pclock_speed value for async\n");
            }

            if (bits)
                edt_reg_write(edt_p, PDV_ROICTL, roictl | bits);

        }
        else if (dd_p->pclock_speed < 1)
        {
            if (pdv_is_cameralink(edt_p))
                dd_p->pclock_speed = 20;
            else dd_p->pclock_speed = 5;
        }

        if (dd_p->double_rate)
        {
            u_int   roictl = edt_reg_read(edt_p, PDV_ROICTL) | PDV_RIOCTL_PCLKSEL_DBL_CAM;

            edt_reg_write(edt_p, PDV_ROICTL,
                roictl);

        }

        if (dd_p->camera_shutter_timing == PDV_DALSA_LS)
        {
            u_int   roictl = edt_reg_read(edt_p, PDV_ROICTL);

            roictl |= PDV_ROICTL_DALSA_LS;

            edt_reg_write(edt_p, PDV_ROICTL,
                roictl);

        }

        /*
        * 'different' dual channel
        */
        if (dd_p->dual_channel)
        {
            edt_msg(DEBUG1, "setting dual channel\n");
            data_path &= ~PDV_RES0;
            data_path |= PDV_RES1;
#if 0				/* wha...? this seems to BREAK Dalsa
            * (ca-d4-1024A anyway), so skip it! */
            if (dd_p->enable_dalsa)
            {
                if (dd_p->extdepth > 8)
                    data_path |= PDV_EXT_DEPTH;
            }
            else
#endif
                data_path |= PDV_EXT_DEPTH;
        }
        else if (dd_p->swinterlace == PDV_BGGR)
        {
            /* set for 10 bit ext - 8 bit first pass kludge */
            data_path |= PDV_RES0;
            data_path &= ~PDV_RES1;
        }
        else if (dd_p->swinterlace == PDV_BGGR_WORD
            || dd_p->swinterlace == PDV_BGGR_DUAL)
        {
            /* set for 10 bit ext */
            data_path |= PDV_EXT_DEPTH;
            data_path |= PDV_RES0;
            data_path &= ~PDV_RES1;
        }
        else
        {
            if (dd_p->depth > 8)
                data_path |= PDV_EXT_DEPTH;
            switch (dd_p->extdepth)
            {
            case 8:
                data_path &= ~PDV_EXT_DEPTH;
                break;
            case 10:
                data_path |= PDV_RES0;
                data_path &= ~PDV_RES1;
                break;
            case 12:
                data_path &= ~PDV_RES0;
                data_path &= ~PDV_RES1;
                break;
            case 14:
                data_path |= PDV_RES0;
                data_path |= PDV_RES1;
                break;
            case 16:
                data_path &= ~PDV_RES0;
                data_path |= PDV_RES1;
                break;
            }
        }

        if (dd_p->camera_shutter_timing == AIA_MCL_100US)
            data_path |= PDV_MULTIPLIER_100US;

        edt_msg(DEBUG2, "DATA_PATH %x\n", data_path);
        dep_wait(edt_p);
        edt_reg_write(edt_p, PDV_DATA_PATH, data_path);
        dd_p->datapath_reg = data_path;
        edt_set_dependent(edt_p, dd_p);
    }

    /* new aia */
    if (dd_p->xilinx_rev >= 1 && dd_p->xilinx_rev <= 32)
    {
        int util2 = dd_p->util2;

        /*
        * set shift/mask (except n/a on cameralink)
        */
        if (!pdv_is_cameralink(edt_p))
        {
            if (dd_p->shift == NOT_SET)
            {
                int     tmpdepth;

                if (dd_p->depth == 24)
                    tmpdepth = 8;
                else
                    tmpdepth = dd_p->depth;

                /*
                * shift gets a bit weird for DVC -- the DVK and DV44 cables are
                * wired differently, and the older DVK is yet a third case --
                * punt on that one and try to handle the first two as best we we
                * can. Worst case will be that someone will need to use explicit
                * shift in the in the config file if they have a specific cable.
                */
                if (pdv_is_dvc(edt_p))
                {
                    edt_msg(DEBUG1, "auto-setting shift for dvc\n");

                    if (edt_p->devid == PDV44_ID)
                        dd_p->shift = (16 - dd_p->extdepth) | PDV_AIA_SWAP;
                    else if (dd_p->extdepth == 12)	/* dvk 12-bit cable */
                    {
                        dd_p->shift = 2;
                    }
                    else
                        dd_p->shift = 0;
                }
                /*
                * not set in config so calculate default for non-cameralink
                */
                else
                {
                    dd_p->shift = 16 - dd_p->extdepth;
                    dd_p->shift |= PDV_AIA_SWAP;
                }

                /*
                * since shift was not set, check mask as well to see if it
                * appears not to have been set either (0xffff) and set it too
                */
                if (dd_p->mask == 0xffff)
                {
                    dd_p->mask = (u_int) (0x0000ffff >> (16 - tmpdepth));
                    if (pdv_is_dvc(edt_p))
                        edt_msg(DEBUG1, "auto-setting mask for dvc\n");
                    else
                        edt_msg(DEBUG1, "auto-set shift/mask to %02x/%02x (AIA/swap)\n", dd_p->shift, dd_p->mask);
                }
                else
                    edt_msg(DEBUG1, "auto-set shift to %02x (AIA/swap)\n", dd_p->shift);
            }

            edt_msg(DEBUG2, "PDV_SHIFT %x\n", dd_p->shift);
            dep_wait(edt_p);
            edt_reg_write(edt_p, PDV_SHIFT, dd_p->shift);
            edt_msg(DEBUG2, "PDV_MASK %x\n", dd_p->mask);
            dep_wait(edt_p);
            edt_reg_write(edt_p, PDV_MASK, dd_p->mask);
        }
#if 0
        edt_reg_write(edt_p, PDV_LINERATE, dd_p->linerate);
        /* Note dd_p->prin has been retired from dependent struct */
        edt_reg_write(edt_p, PDV_PRIN, dd_p->prin);
#endif
        if (dd_p->photo_trig)
            util2 |= PDV_PHOTO_TRIGGER;
        if (dd_p->fieldid_trig)
            util2 |= PDV_FLDID_TRIGGER;
        if (dd_p->acquire_mult)
            util2 |= PDV_AQUIRE_MULTIPLE;

        /*
        * RS-232 bit -- overloaded on MC4, only applicable to PCI DVa or DVFox
        */
        if ((dd_p->serial_mode == PDV_SERIAL_RS232)
            && ((edt_p->devid == PDVA_ID) || (edt_is_dvfox(edt_p))))
            util2 |= PDV_RX232;

        else if (dd_p->sel_mc4)
            util2 |= PDV_SEL_MC4;

        /*
        * mc4 is EXPEXSYNC on camera link, so do it whether or not (the nearly obsolete)
        * SEL_MC4 is specified
        */
        if (dd_p->mc4) /* alias EXPEXSYNC on pcidv c-link */
            util2 |= PDV_MC4;

        if (dd_p->dbl_trig)
            util2 |= PDV_DBL_TRIG;
        if (dd_p->pulnix)
            util2 |= PDV_PULNIX;

        dd_p->util2 = util2;
        edt_msg(DEBUG2, "PDV_UTIL2 %x\n", util2);
        dep_wait(edt_p);
        edt_reg_write(edt_p, PDV_UTIL2, util2);
    }

    if (dd_p->byteswap == NOT_SET)
    {
        if (edt_little_endian())
            dd_p->byteswap = 0;
        else
            dd_p->byteswap = 1;
    }
    else edt_msg(DEBUG2, "byteswap %d, forcing %s\n",
        dd_p->byteswap? "ON":"OFF");

    /* set Byteswap/ Hwpad */
    {
        int     padword = 0;

        padword = dd_p->hwpad << 1;

        if (dd_p->byteswap)
            padword |= PDV_BSWAP;
        if (dd_p->shortswap)
            padword |= PDV_SSWAP;
        if (dd_p->disable_mdout)
            padword |= PDV_DISABLEMD;
        if (dd_p->gendata)
            padword |= PDV_GENDATA;
        if (dd_p->skip)
            padword |= PDV_SKIP;

        edt_msg(DEBUG2, "PAD_SWAP %x\n", padword);
        dep_wait(edt_p);
        edt_reg_write(edt_p, PDV_BYTESWAP, padword);
    }


    /* xybion only (for now) uses fixedlen */
    if (dd_p->fixedlen)
    {
        /* #define SWAPEM 1 */
#if SWAPEM
        unsigned short val = dd_p->fixedlen;
        unsigned short tmp = ((val & 0xff) << 8) | ((val & 0xff00) >> 8);

        edt_msg(DEBUG2, "FIXEDLEN %x\n", tmp);
        dep_wait(edt_p);
        edt_reg_write(edt_p, PDV_FIXEDLEN, tmp);
#else

        edt_msg(DEBUG2, "FIXEDLEN %x\n", dd_p->fixedlen);
        dep_wait(edt_p);
        edt_reg_write(edt_p, PDV_FIXEDLEN, dd_p->fixedlen);
#endif
    }


    edt_msg(DEBUG2, "MODE_CNTL %x\n", dd_p->mode_cntl_norm);
    dep_wait(edt_p);
    edt_reg_write(edt_p, PDV_MODE_CNTL, dd_p->mode_cntl_norm);
    dep_wait(edt_p);


    edt_reset_serial(edt_p);

    /* set waitc */
    if (dd_p->serial_waitc == NOT_SET)
        pdv_set_waitchar(edt_p, 0, 0);
    else
        pdv_set_waitchar(edt_p, 1, (u_char)dd_p->serial_waitc);

    if (dd_p->user_timeout == NOT_SET)
    {
        dd_p->user_timeout = 0;	/* user_set will be the real flag */
        pdv_auto_set_timeout(edt_p);
    }
    else
        pdv_set_timeout(edt_p, dd_p->user_timeout);

    if (dd_p->timeout != NOT_SET)
        edt_set_rtimeout(edt_p, dd_p->timeout);

    pdv_set_defaults(edt_p);
    check_camera_values(edt_p, dd_p);

    pdv_do_xregwrites(edt_p, dd_p);

    /*
    * final stuff (none applicable to CL2 sim) 
    */
    if (!(edt_is_simulator(edt_p) && dd_p->sim_enable))
    {
        check_terminator(dd_p->serial_term);

        if ((dd_p->camera_shutter_timing == SPECINST_SERIAL)
            || (dd_p->camera_shutter_speed == SPECINST_SERIAL)
            || (dd_p->camera_download == SPECINST_SERIAL))
            dd_p->force_single = 1;

        if (dd_p->camera_download == SPECINST_SERIAL)
        {
            if ((DD_P_CAMERA_DOWNLOAD_FILE[0])
                && ((ret = specinst_download(edt_p, DD_P_CAMERA_DOWNLOAD_FILE)) == 0))
                if (DD_P_CAMERA_COMMAND_FILE[0])
                    ret = specinst_setparams(edt_p, DD_P_CAMERA_COMMAND_FILE);
            if (ret != 0)
                return ret;
        }

        if (ei_p->serial_init)
            init_serial(edt_p, dd_p, ei_p);

        if (strlen(dd_p->xilinx_init) > 0)
            send_xilinx_commands(edt_p, dd_p->xilinx_init);

        if (dd_p->frame_timing != 0)
            pdv_set_frame_period(edt_p, dd_p->frame_period, dd_p->frame_timing);

        if (ei_p->startdma != NOT_SET)
            edt_startdma_action(edt_p, ei_p->startdma);
        else
            edt_startdma_action(edt_p, EDT_ACT_ALWAYS);
        if (ei_p->enddma != NOT_SET)
            edt_enddma_action(edt_p, ei_p->enddma);
        if (ei_p->flushdma != NOT_SET)
            edt_set_firstflush(edt_p, ei_p->flushdma);
        else
            edt_set_firstflush(edt_p, EDT_ACT_ONCE);
    }

    if (dd_p->header_type)
        pdv_set_header_type(edt_p, dd_p->header_type, dd_p->irig_slave, dd_p->irig_offset, dd_p->irig_raw);

    return 0;
}



#ifdef _NT_
#define popen(cmd, mode) _popen(cmd, mode)
#define pclose(cmd) _pclose(cmd)
#define BITLOAD "bitload"
#else
#define BITLOAD "./bitload"
#endif

/* find id string, starting with "id:". If found return 1, else 0 */
int
findid(char *buf, char *idstr)
{
    if (strncmp(buf, "id:", 3) == 0)
    {
#if 0
        char   *dp = idstr;
        char   *p = strchr(buf, '"');
        char   *pp = strrchr(p, '"');

        for (; p != pp; p++)
            *dp++ = *p;
#endif
        return (sscanf(&buf[4], " \"%[^\"]\"", idstr));
    }
    return 0;
}

void
dir_from_path(char *dirstr, const char *pathstr)
{
    char   *bsp = strrchr(pathstr, '\\');
    char   *fsp = strrchr(pathstr, '/');
    char   *sp;

    if (bsp > fsp)
        sp = bsp;
    else
        sp = fsp;

    if (sp != NULL)
    {
        strncpy(dirstr, pathstr, sp - pathstr);
        dirstr[sp - pathstr] = '\0';
    }
    else
        *dirstr = '\0';
}

/*
* return 1 if async bitfile (from name); so that we set roictl
* from pclock speed ONLY if async bitfile
*/
int
async_bitfile(EdtDev *edt_p)
{
    edt_bitpath  pathbuf;

    edt_get_bitname(edt_p, pathbuf, sizeof(edt_bitpath)) ;
    if (pathbuf[0] && (strstr(pathbuf, "async") != NULL))
        return 1;
    return 0;
}

void
fname_from_path(char *fname, char *pathstr)
{
    char   *bsp = strrchr(pathstr, '\\');
    char   *fsp = strrchr(pathstr, '/');
    char   *sp;

    if (bsp > fsp)
        sp = bsp;
    else
        sp = fsp;

    if (sp == NULL)
        strcpy(fname, pathstr);
    else
        strcpy(fname, sp + 1);
}

/*
* check serial_exe to see if it's  Kodak 'i' type, if found then flush the
* serial with a TRM command, and get the idn. Then send serial init string,
* one command at a time.
*/
int
init_serial(EdtDev * edt_p, Dependent * dd_p, Edtinfo *ei_p)
{
    SerialInitNode *lp = ei_p->serial_init;

    edt_msg(DEBUG1, "sending serial init....\n");


    while (lp)
    {
        if (lp->tag == serial_tag_binary)
            init_serial_binary(edt_p, dd_p, ei_p->serial_init_delay, lp->data);

        else if (lp->tag == serial_tag_baslerf)
            init_serial_basler_binary(edt_p, dd_p, ei_p->serial_init_delay, lp->data);

        else if (lp->tag == serial_tag_duncanf)
            init_serial_duncan_binary(edt_p, dd_p, ei_p->serial_init_delay, lp->data);

        else init_serial_ascii(edt_p, dd_p, ei_p->serial_init_delay, lp->data);

        lp = lp->next;
    }
    return 0;

}

int
init_serial_ascii(EdtDev * edt_p, Dependent * dd_p, int delay, char *serial_init)
{
    char    resp[257];
    int     skip_init = 0;
    int     hamamatsu = 0;
    int     ret = 0;

    if (grepit(dd_p->cameratype, "Hamamatsu") != NULL)
        hamamatsu = 1;

    /* first flush any pending/garbage serial */
    resp[0] = '\0';
    ret = pdv_serial_wait(edt_p, dd_p->serial_timeout, 256);
    pdv_serial_read(edt_p, resp, 256);

    if (edt_p->dd_p->startup_delay)
        edt_msleep(edt_p->dd_p->startup_delay);

    edt_msg(DEBUG1, "sending serial init commands to camera....\n");

    /*
    * if kodak 'i' camera, send a couple of "TRM?"s just to flush out the
    * serial, and send the IDN string to get the camera firmware ID
    */
    if (pdv_is_kodak_i(edt_p))
    {
        /* first send a couple of TRM?s to flush and sync */
        pdv_serial_command(edt_p, "TRM?");
        pdv_serial_wait(edt_p, 500, 40);
        pdv_serial_command(edt_p, "TRM?");
        pdv_serial_wait(edt_p, 500, 40);
        ret = pdv_serial_read(edt_p, resp, 256);

        pdv_serial_command(edt_p, "IDN?");
        edt_msg(DEBUG1, "IDN? ");
        pdv_serial_wait(edt_p, 1000, 60);
        ret = pdv_serial_read(edt_p, resp, 256);
        if (ret > 20)
            edt_msg(DEBUG1, "%s\n", resp);
        else if (ret > 0)
            edt_msg(DEBUG1, "%s (unexpected response!)\n", resp);
        else
        {
            edt_msg(DEBUG1, "<no response from camera -- check cables and connections>\n");
            skip_init = 1;
        }
    }

    if (!skip_init)
    {
        int    i = 0, ms;
        char   *lastp = NULL;
        char   *p = serial_init;
        char    cmdstr[32];

        if (delay == NOT_SET)
            ms = 500;
        else ms = delay;

        /*
        * send serial init string
        */
        cmdstr[0] = '\0';
        while (*p)
        {
            if (i > 31)
            {
                edt_msg(EDT_MSG_FATAL, "ERROR: serial command too long\n");
                return -1;
            }
            if (*p == ':' && lastp && *lastp != '\\')
            {
                cmdstr[i] = '\0';
                i = 0;

                memset(resp, '\0', 257);

                if (cmdstr[0])
                {
                    if (is_hex_byte_command(cmdstr))
                    {
                        if (delay == NOT_SET)
                            ms = 10;
                        edt_msg(DEBUG2, "%s\n", cmdstr);
                        pdv_serial_command_hex(edt_p, cmdstr, 1);
                        ret = pdv_serial_wait(edt_p, dd_p->serial_timeout, 128);
                        pdv_serial_read(edt_p, resp, ret);
                    }
                    else if (hamamatsu)
                    {
                        char   *resp_str;

                        edt_msg(DEBUG2, "%s", strip_crlf(cmdstr));
                        fflush(stdout);
                        resp_str = serial_tx_rx(edt_p, cmdstr, 0);
                        edt_msg(DEBUG2, " <%s>\n", strip_crlf(resp_str));
                    }
                    else			/* FUTURE: expand and test serial_tx_rx and replace
                                     * pdv_serial_command/wait/read with that for all (?)
                                     */
                    {
                        /* edt_msg(DEBUG1, ".", cmdstr); */
                        edt_msg(DEBUG2, "%s ", cmdstr);
                        pdv_serial_command(edt_p, cmdstr);
                        ret = pdv_serial_wait(edt_p, dd_p->serial_timeout, 16);
                        pdv_serial_read(edt_p, resp, 256);
                        edt_msg(DEBUG2, " <%s>", strip_crlf(resp));
                        edt_msg(DEBUG2, "\n");
                    }
                    /* edt_msg(DEBUG1, ".", cmdstr); */
                    edt_msleep(ms);
                }
            }
            else if (*p != '\\')
                cmdstr[i++] = *p;
            lastp = p;
            ++p;
        }
    }

    return 0;
}

u_char
atoxdigit(char ch)
{
    if (ch >= '0' && ch <= '9')
        return (ch - '0');

    if (ch >= 'a' && ch <= 'f')
        return (ch - 'a') + 10;

    if (ch >= 'A' && ch <= 'F')
        return (ch - 'A') + 10;

    return 0;
}

/*
* binary initialization: serial_binit specified, interpret as string of
* hex bytes only
* update 7/10/07: now sends adjacent bytes as a single command (string of
* bytes); does a * wait/read on whitespace only
*/
int
init_serial_binary(EdtDev * edt_p, Dependent * dd_p, int delay, char *serial_init)
{
    int ret;
    u_int i;
    size_t buflen = strlen(serial_init);
    int cmdlen = 0, nibble_index = 0;
    u_char hbuf[1024];
    char    resp[1025];
    u_char ch;


    /* flush junk */
    ret = pdv_serial_wait(edt_p, dd_p->serial_timeout, 1024);

    hbuf[0] = 0;
    nibble_index = 0;
    for (i = 0; i < buflen+1; i++)
    {
        if (i < buflen)
            ch = serial_init[i];
        else ch = '\0';

        if ((i == buflen) || (ch == '\t' || ch == ' '))
        {
            cmdlen = (nibble_index+1) / 2;

            if (cmdlen)
            {
                /* send command, wait for response */
                pdv_serial_binary_command(edt_p, (char *) hbuf, cmdlen);
                if ((ret = pdv_serial_wait(edt_p, dd_p->serial_timeout, 1024)))
                    pdv_serial_read(edt_p, resp, ret);
            }

            nibble_index = 0;
            hbuf[0] = 0;
        }
        else if (isxdigit(ch))
        {
            hbuf[nibble_index/2] = (hbuf[nibble_index/2] << 4) | atoxdigit(ch);
            if ((nibble_index++) % 2)
                hbuf[nibble_index/2] = 0;
        }
        else
        {
            edt_msg(PDVLIB_MSG_WARNING, "serial_binit: hex string format error on buf[%d] ('%c')\n", i, serial_init[i]);
            return -1;
        }
    }

    edt_set_dependent(edt_p, dd_p);

    return 0;
}

/*
* basler A202k binary only initialization -- expects serial_init to contain
* colon-separated strings of hex bytes WITHOUT framing information; add
* basler A202k style framing and send the commands to the camera
*/
int
init_serial_basler_binary(EdtDev * edt_p, Dependent * dd_p, int delay, char *serial_init)
{
    u_int  i, j;
    char   *p;
    int    ms = 50;
    char   *nextp;
    size_t len;
    int     ret;
    char    cmdstr[32];
    char    bytestr[3];
    char    resp[1024];

    if (delay != NOT_SET)
        ms = delay;

    /* first flush any pending/garbage serial */
    resp[0] = '\0';
    ret = pdv_serial_wait(edt_p, dd_p->serial_timeout, 1023);
    pdv_serial_read(edt_p, resp, 1023);

    edt_msg(DEBUG1, "sending Basler A202k framed commands to camera:\n");

    /*
     * send serial init string (first stick on a trailing ':' for parser)
     */
    len = strlen(serial_init);
    if (serial_init[len-1] != ':')
    {
        serial_init[len] = ':';    /* safe because data is */
        serial_init[len+1] = '\0'; /* malloc'd to arg len + 2 */
    }

    p = serial_init;

    while ((nextp = strchr(p, ':')))
    {
        int     ms = 50;
        u_char bytebuf[32];
        u_char *bp = bytebuf;

        len = (size_t)(nextp - p);
        if (len > 31)
        {
            edt_msg(EDT_MSG_FATAL, "ERROR: serial command too long\n");
            return -1;
        }

        strncpy(cmdstr, p, len);
        cmdstr[len] = 0;
        memset(resp, '\0', 1023);

        if (len % 2)
        {
            edt_msg(EDT_MSG_FATAL, "serial_binary format string error: odd nibble count\n");
            return -1;
        }

        for (i=0; i<len; i++)
        {
            if (!isxdigit(cmdstr[i]))
            {
                edt_msg(EDT_MSG_FATAL, "serial_binrary format string error: odd nibble count\n");
                return -1;
            }
        }

        for (i=0, j=0; i<len; i+=2, j++)
        {
            u_int bint;
            bytestr[0] = cmdstr[i];
            bytestr[1] = cmdstr[i+1];
            bytestr[2] = '\0';
            sscanf(bytestr, "%x", &bint);
            *(bp++) = (u_char)(bint & 0xff);
        }
        pdv_send_basler_frame(edt_p, bytebuf, (int)len/2);

        ret = pdv_serial_wait(edt_p, dd_p->serial_timeout, 1023);
        pdv_serial_read(edt_p, resp, ret);

        p = nextp + 1;
        edt_msleep(ms);
    }

    edt_set_dependent(edt_p, dd_p);

    return 0;
}

/*
* duncantech MS2100, 2150, 3100, 4100 binary only initialization -- expects
* serial_init to contain colon-separated strings of hex bytes WITHOUT
* framing information; add duncan framing and send the commands to the
* camera
*/
int
init_serial_duncan_binary(EdtDev * edt_p, Dependent * dd_p, int delay, char *serial_init)
{
    int    i, j;
    char   *p;
    int    ms = 50;
    char   *nextp;
    int     len;
    int     ret;
    char    cmdstr[32];
    char    bytestr[3];
    char    resp[1024];

    if (delay != NOT_SET)
        ms = delay;

    /* first flush any pending/garbage serial */
    resp[0] = '\0';
    ret = pdv_serial_wait(edt_p, dd_p->serial_timeout, 1023);
    pdv_serial_read(edt_p, resp, 1023);

    edt_msg(DEBUG1, "sending DuncanTech framed commands to camera:\n");

    /*
     * send serial init string (first stick on a trailing ':' for parser)
     */
    len = strlen(serial_init);
    if (serial_init[len-1] != ':')
    {
        serial_init[len] = ':';    /* safe because data is */
        serial_init[len+1] = '\0'; /* malloc'd to arg len + 2 */
    }

    p = serial_init;

    while ((nextp = strchr(p, ':')))
    {
        int     ms = 50;
        u_char bytebuf[32];
        u_char *bp = bytebuf;

        len = (int)(nextp - p);
        if (len > 31)
        {
            edt_msg(EDT_MSG_FATAL, "ERROR: serial command too long\n");
            return -1;
        }

        strncpy(cmdstr, p, len);
        cmdstr[len] = 0;
        memset(resp, '\0', 1024);

        if (len % 2)
        {
            edt_msg(EDT_MSG_FATAL, "serial_binrary format string error: odd nibble count\n");
            return -1;
        }

        for (i=0; i<len; i++)
        {
            if (!isxdigit(cmdstr[i]))
            {
                edt_msg(EDT_MSG_FATAL, "serial_binrary format string error: odd nibble count\n");
                return -1;
            }
        }

        for (i=0, j=0; i<len; i+=2, j++)
        {
            u_int bint;
            bytestr[0] = cmdstr[i];
            bytestr[1] = cmdstr[i+1];
            bytestr[2] = '\0';
            sscanf(bytestr, "%x", &bint);
            *(bp++) = (u_char)(bint & 0xff);
        }
        pdv_send_duncan_frame(edt_p, bytebuf, len/2);

        /* flush out junk */
        ret = pdv_serial_wait(edt_p, dd_p->serial_timeout, 1023);
        pdv_serial_read(edt_p, resp, ret);

        p = nextp + 1;
        edt_msleep(ms);
    }


    edt_set_dependent(edt_p, dd_p);

    return 0;
}

/*
* multi-purpose ROI check/set. Cases include:
* 
* - xilinx rev doesn't support ROI: return without setting
* - dalsa line scan: check hskip/hactv for valid values (no need to enable tho)
* - hactv/vactv set: set and enable ROI
* - camera link board: auto set to width (or 4 byte aligned), full height - otherwise disable
*/
int
check_roi(EdtDev * edt_p, Dependent * dd_p, int ena)
{
    int     ret = 0;

    if ((dd_p->xilinx_rev < 2) || (dd_p->xilinx_rev > 32))
        return 0;

    /*
    * NEW 10/30/2008: htaps / vtaps now get set automatically (htaps = #taps as set by
    * CL_DATA_PATH_NORM left nibble) unless set explicitly via htaps / vtaps directives
    */
    if (dd_p->htaps == NOT_SET)
    {
        if (dd_p->vtaps == NOT_SET)
        {
            if ((dd_p->cl_data_path == 0)               /* special case, datapath is 00 */
                || ((dd_p->cl_data_path & 0xf0) == 0x20))  /* special case RGB */
                dd_p->htaps = 1;
            else dd_p->htaps = ((dd_p->cl_data_path & 0xf0) >> 4) + 1;
            dd_p->vtaps = 1;
        }
        else dd_p->htaps = 1;
    }
    else if (dd_p->vtaps == NOT_SET)
        dd_p->vtaps = 1;

    /* set ROI */
    if (dd_p->hactv && dd_p->camera_shutter_timing == PDV_DALSA_LS)
    {
        if (pdv_dalsa_ls_set_expose(edt_p, dd_p->hskip, dd_p->hactv) != 0)
        {
            edt_msg(EDT_MSG_FATAL, "Error setting DALSA LS parameters!\n");
            ret = -1;
        }
    }
    else if (dd_p->hactv || dd_p->vactv)
    {
        if (dd_p->hactv == 0)	/* only vactv given; set hactv to a mult. of
                                * 4 */
                                dd_p->hactv = ((pdv_get_width(edt_p) / 4) * 4);
        else if (dd_p->vactv == 0)	/* only hactv given; set hactv to
                                    * height */
                                    dd_p->vactv = pdv_get_height(edt_p);

        if ((ret = (pdv_set_roi(edt_p, dd_p->hskip, dd_p->hactv, dd_p->vskip, dd_p->vactv) == 0)))
            ret = pdv_enable_roi(edt_p, ena);

        if (ret != 0)
            edt_msg(EDT_MSG_FATAL, "Error setting or enabling ROI!\n");
    }
    else if (pdv_is_cameralink(edt_p) && ena)
        ret = pdv_auto_set_roi(edt_p);	/* also enables */
    else
        ret = pdv_enable_roi(edt_p, 0);	/* DISable */


    /* new 6/20/06 -- set up pingpong stuff in initcam */
    if (edt_p->dd_p->pingpong_varsize)
    {
        int util3;

        pdv_enable_roi(edt_p, 0); /* disable ROI regardless */

        /* 
        * set frame valid done flag and enable ping pong bit (0x02) in
        * utility 3 register (NOTE: special FW required, might want to
        * check for that here)
        */
        pdv_set_fval_done(edt_p, 0);
        util3 = edt_reg_read(edt_p, PDV_UTIL3);
        edt_reg_write(edt_p, PDV_UTIL3, util3 & ~PDV_PPENB);
        edt_reg_write(edt_p, PDV_UTIL3, util3 | PDV_PPENB);

        /* reset pingpong to channel 0 */
        util3 = edt_reg_read(edt_p, PDV_UTIL3);
        edt_reg_write(edt_p, PDV_UTIL3, util3 | PDV_PPRST);
        edt_reg_write(edt_p, PDV_UTIL3, util3 & ~PDV_PPRST);
        pdv_set_fval_done(edt_p, 1);
    }

    return ret;
}


void
check_camera_values(EdtDev * ed, Dependent * dd_p)
{
    if ((dd_p->shutter_speed_min != NOT_SET)
        && ((dd_p->shutter_speed < dd_p->shutter_speed_min)
        || (dd_p->shutter_speed > dd_p->shutter_speed_max)))
        dd_p->shutter_speed = dd_p->shutter_speed_min;

    if ((dd_p->gain_min != NOT_SET)
        && ((dd_p->gain < dd_p->gain_min)
        || (dd_p->gain > dd_p->gain_max)))
        dd_p->gain = dd_p->gain_min;

    if ((dd_p->offset_min != NOT_SET)
        && ((dd_p->level < dd_p->offset_min)
        || (dd_p->level > dd_p->offset_max)))
        dd_p->level = dd_p->offset_min;

    edt_set_dependent(ed, dd_p);
}


int
kodak_query_serial(EdtDev * ed, char *cmd, int *val)
{
    char    cmdstr[32];
    char    resp[256];
    char   *p, *pp;
    int     ret;

    if (!(*cmd))
        return 0;

    sprintf(cmdstr, "%s?", cmd);
    pdv_serial_command(ed, cmdstr);
    ret = pdv_read_response(ed, resp);

    if ((ret < 5) || (ret > 15))
        return 0;

    p = strchr(resp, ' ');
    pp = p + 1;

    if ((p == NULL) || (*pp == '\0') || ((*pp != '-') && !isdigit(*pp)))
        return 0;

    /* check for Kodak ES (and ES 4.0) format */
    if (strchr(pp, '.') != NULL)
    {
        /*
        * ES 4.0 -- uses fractions for both in and out, we punt and stick
        * with positive integers only
        */
        if (ed->dd_p->camera_shutter_timing == AIA_SERIAL_ES40)
        {
            *val = (int) (atof(pp) + 0.5);
            if (*val < 1)
                *val = 1;
        }
        /* ES 1.0 -- uses fractions out, ints in */
        else
            *val = (int) ((atof(pp) / .0636) + 0.5);
    }
    else
        *val = atoi(pp);
    return (ret);
}


/*
* looking for "0xNN". If so, return 1, else return 0
*/
int
is_hex_byte_command(char *str)
{
    int     i;

    if (strlen(str) != 4)
        return 0;

    if ((strncmp(str, "0x", 2) != 0)
        && (strncmp(str, "0X", 2) != 0))
        return 0;

    for (i = 2; i < (int)strlen(str); i++)
    {
        if ((str[i] < '0') && (str[i] > '9')
            && (str[i] < 'a') && (str[i] > 'z')
            && (str[i] < 'A') && (str[i] > 'Z'))
            return 0;
    }
    return 1;
}


/*
* takes a colon separated string of FPGA commands, parses them into
* individual commands, and sends one at a time to the interface xilinx
*/
int
send_fpga_commands(EdtDev * edt_p, char *str)
{
    char   *p;
    char   *nextp;
    int     len;
    char    cmdstr[32];
    char    cmd[32];
    u_int   addr;
    unsigned long lvalue;
    u_char  value;

    edt_msg(DEBUG1, "sending FPGA commands....\n");

    p = str;

    while ((nextp = strchr(p, ':')))
    {
        len = (int)(nextp - p);
        if (len > 31)
        {
            edt_msg(EDT_MSG_FATAL, "ERROR: FPGA cmd too long\n");
            return -1;
        }

        strncpy(cmdstr, p, len);
        cmdstr[len] = 0;

        sscanf(cmdstr, "%s %x %lx", cmd, &addr, &lvalue);
        if (addr < 0xffff)
            addr |= 0x01010000;
        value = (unsigned char) (lvalue & 0xff);
        edt_msg(DEBUG1, "%s %08x %02x\n", cmd, addr, value);

        if (strcmp(cmd, "w") == 0)
        {
#if 0
            edt_intfc_write(edt_p, addr, value);
#else
            edt_msg(PDVLIB_MSG_WARNING, "ALERT: edt_intfc_write commented out for testing only\n");
#endif
        }
        /* else other commands here */
        else
        {
            edt_msg(PDVLIB_MSG_WARNING, "unknown FPGA command %s\n", cmd);
        }

        p = nextp + 1;
    }
    return 1;
}

/*
* search for a pattern in a char buffer, return pointer to next char if
* found, NULL if not
*/
char   *
grepit(char *buf, char *pat)
{
    int     i;

    for (i = 0; i < (int)strlen(buf); i++)
    {
        if (buf[i] == pat[0])
        {
            if (strncmp(&buf[i], pat, strlen(pat)) == 0)
                return &buf[i + strlen(pat)];
        }
    }
    return NULL;
}

/*
* config file takes "\r" and "\n" which need to be converted from strings to
* chars
*/
void
check_terminator(char *str)
{
    int     i, j = 0;
    char    tmpstr[MAXSER];

    for (i = 0; i < (int) strlen(str); i += 2)
    {
        if (str[i] == '\\')
        {
            if (str[i + 1] == 'r')
                tmpstr[j++] = '\r';
            else if (str[i + 1] == 'n')
                tmpstr[j++] = '\n';
            else if (str[i + 1] == '0')
                tmpstr[j++] = '\0';
        }
    }
    tmpstr[j] = '\0';
    if (*tmpstr)
        strcpy(str, tmpstr);
}

int
specinst_download(EdtDev * edt_p, char *fname)
{
    int     i, extras = 0, ena;
    size_t  n, nb = 0;
    char    dmy[32];
    int     ret, resp = 0;
    u_char  buf[1024];
    u_char  savechar;
    FILE   *fd;

    edt_msg(DEBUG1, "SpecInst download <%s>", fname);
    fflush(stdout);

    if (pdv_access(fname, 0) != 0)
    {
        edt_msg(EDT_MSG_FATAL, "ERROR: Failed camera download (%s)\n", fname);
        return -1;
    }

    if ((fd = fopen(fname, "rb")) == NULL)
    {
        edt_perror(fname);
        edt_msg(EDT_MSG_FATAL, "\nERROR: couldn't open camera download file <%s>", fname);
        return -1;
    }

    pdv_send_break(edt_p);
    edt_msleep(500);

    /* flush any pending/garbage serial */
    ret = pdv_serial_wait_next(edt_p, edt_p->dd_p->serial_timeout, 256);
    if (ret == 0)
        ret = 3;
    if (ret)
        pdv_serial_read(edt_p, (char *) buf, ret);

    ena = pdv_get_waitchar(edt_p, &savechar);
    while ((n = (int)fread(buf, 1, 1, fd)))
    {
        buf[n] = '\0';
        nb += n;
        if (!(nb % 200))
        {
            edt_msg(DEBUG1, ".");
            fflush(stdout);
        }
        pdv_set_waitchar(edt_p, 1, buf[0]);
        pdv_serial_binary_command(edt_p, (char *) buf, (int)n);

        if ((ret = pdv_serial_wait_next(edt_p, 50, (int)n)))
        {
            pdv_serial_read(edt_p, dmy, ret);
            /* DEBUG */ dmy[ret] = '\0';
            if (ret > 1)
            {
                edt_msg(PDVLIB_MSG_WARNING, "specinst_download wrote %x read %x ret %d! \n", buf[0], dmy[0], ret);
            }
        }

        if (ret > 1)
        {
            ++extras;

            edt_msg(PDVLIB_MSG_WARNING, "specinst_download: ret %d s/b 1, read <", ret);
            for (i = 0; i < ret; i++);
            edt_msg(PDVLIB_MSG_WARNING, "%c", dmy[i]);
            edt_msg(PDVLIB_MSG_WARNING, ">\n");
            edt_msleep(2000);
        }
        resp += ret;
    }
    edt_msg(DEBUG1, "done\n");

    /*
    * restore old waitchar (if any)
    */
    pdv_set_waitchar(edt_p, ena, savechar);

    if (extras)
        edt_msg(DEBUG1, "Spectral Instruments program download got extra bytes...???\n");
    else if (nb > (size_t)resp)
    {
        edt_msg(EDT_MSG_FATAL, "Spectral Instruments program download apparently FAILED\n");
        edt_msg(EDT_MSG_FATAL, "Wrote %d bytes, got %d responses (continuing anyway)\n", nb, resp);
    }

    fclose(fd);
    edt_msleep(500);
    return 0;
}

int
specinst_setparams(EdtDev * edt_p, char *fname)
{
    char    cmd;
    char    resp[256];
    u_char  savechar;
    int     ret, ena;
    char    buf[1024];
    u_long  offset;
    u_long  param;
    u_char  cmdbuf[8];
    u_char  offsetbuf[8];
    u_char  parambuf[8];
    FILE   *fd;
    int     si_wait = 150;


    edt_msg(DEBUG1, "SpecInst setparams <%s>", fname);
    fflush(stdout);

    pdv_serial_wait_next(edt_p, si_wait, 2);

    ret = pdv_serial_read(edt_p, buf, 64);


    resp[0] = resp[1] = resp[2] = 0;

    if (pdv_access(fname, 0) != 0)
    {
        edt_msg(EDT_MSG_FATAL, "\nERROR: Failed camera setparams (%s) - aborting\n", fname);
        return -1;
    }

    if ((fd = fopen(fname, "rb")) == NULL)
    {
        edt_perror(fname);
        edt_msg(EDT_MSG_FATAL, "\ncouldn't open camera parameter file <%s> - aborting", fname);
        return -1;
    }

    pdv_serial_wait_next(edt_p, 1000, 64);
    ret = pdv_serial_read(edt_p, buf, 64);

    ena = pdv_get_waitchar(edt_p, &savechar);
    while (fgets(buf, 1024, fd))
    {
        if ((buf[0] == '#') || (strlen(buf) < 1))
            continue;

        if (sscanf(buf, "%c %lu %lu", &cmd, &offset, &param) != 3)
        {
            edt_msg(PDVLIB_MSG_WARNING, "\ninvalid format in parameter file <%s> '%s' -- ignored\n", fname, buf);
            return -1;
        }

        edt_msg(DEBUG1, ".");
        fflush(stdout);

        /* edt_msg(DEBUG1, "cmd %c %04x %04x\n",cmd,offset,param) ; */
        dvu_long_to_charbuf(offset, offsetbuf);
        dvu_long_to_charbuf(param, parambuf);


        /*
        * specinst echoes the the command
        */
        cmdbuf[0] = cmd;
        pdv_set_waitchar(edt_p, 1, cmd);

        pdv_serial_binary_command(edt_p, (char *) cmdbuf, 1);
        /* pdv_serial_binary_command(edt_p, &cmd, 1); */
        pdv_serial_wait_next(edt_p, 500, 2);
        ret = pdv_serial_read(edt_p, resp, 2);
        if ((ret != 1) || (resp[0] != cmd))
        {
            edt_msg(EDT_MSG_FATAL, "specinst_setparams: invalid or missing serial response (sent %c, ret %d resp %02x), aborting\n", cmd, ret, (ret > 0) ? cmd : 0);
            return -1;
        }

        pdv_set_waitchar(edt_p, 1, 'Y');

        pdv_serial_binary_command(edt_p, (char *) offsetbuf, 4);
#if 0
        pdv_serial_wait_next(edt_p, si_wait, 2);
        ret = pdv_serial_read(edt_p, buf, 2);
#endif
        pdv_serial_binary_command(edt_p, (char *) parambuf, 4);
        pdv_serial_wait_next(edt_p, si_wait, 2);
        ret = pdv_serial_read(edt_p, resp, 2);

        if ((ret != 1) || (resp[0] != 'Y'))
        {
            edt_msg(EDT_MSG_FATAL, "invalid or missing serial response (sent %04x %04x ret %d resp %02x), aborting\n", offset, param, ret, (ret > 0) ? cmd : 0);
            return -1;
        }
    }

    /*
    * restore old waitchar (if any)
    */
    pdv_set_waitchar(edt_p, ena, savechar);

    edt_msg(DEBUG1, "done\n");
    fclose(fd);
    return 0;
}


/*
* serial send AND recieve -- send a command and wait for the response
*/
static char stRetstr[256];
char   *
serial_tx_rx(PdvDev * pdv_p, char *getbuf, int hexin)
{
    int     i;
    int     ret;
    int     nbytes;
    int     length;
    u_char  hbuf[2], waitc, lastbyte;
    char    tmpbuf[256];
    char   *buf_p, *ibuf_p;
    char    buf[2048];
    char    bs[32][3];
    long    stx_etx_str(u_char * buf, int n);

    ibuf_p = getbuf;
    stRetstr[0] = '\0';

    /* flush any junk */
    (void) pdv_serial_read(pdv_p, buf, 2048);

    if (hexin)
    {
        nbytes = sscanf(ibuf_p, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
            bs[0], bs[1], bs[2], bs[3], bs[4], bs[5], bs[6], bs[7],
            bs[8], bs[9], bs[10], bs[11], bs[12], bs[13], bs[14], bs[15]);

        /*
        * change 5/28/99 one serial_binary_command for the whole thing --
        * before it did one write per byte which was dumb 
        */
        for (i = 0; i < nbytes; i++)
        {
            if (strlen(bs[i]) > 2)
            {
                edt_msg(EDT_MSG_FATAL, "hex string format error\n");
                break;
            }
            hbuf[i] = (u_char) (strtoul(bs[i], NULL, 16) & 0xff);

        }

        /*
         * using pdv_serial_binary_command instead of pdv_serial_write
         * because it prepends a 'c' if FOI [but FOI is gone anyhow...]
         */
        pdv_serial_binary_command(pdv_p, (char *) hbuf, nbytes);
        /* edt_msleep(10000); */
    }
    else
    {
        sprintf(tmpbuf, "%s\r", ibuf_p);
        edt_msg(DEBUG2, "writing <%s>\n", ibuf_p);
        pdv_serial_command(pdv_p, tmpbuf);
    }

    /*
    * serial_timeout comes from the config file (or -t override flag in this
    * app), or if not present defaults to 500
    */
    pdv_serial_wait(pdv_p, pdv_p->dd_p->serial_timeout, 64);

    /*
    * get the return string. How it gets printed depends on whether (1
    * ASCII, 2) HEX, or 3) Pulnix STX/ETX format
    */
    buf_p = buf;
    length = 0;
    do
    {
        ret = pdv_serial_read(pdv_p, buf_p, 2048 - length);
        edt_msg(DEBUG2, "\nread returned %d\n", ret);

        if (*buf_p)
            lastbyte = (u_char)buf_p[strlen(buf_p)-1];

        if (ret != 0)
        {
            buf_p[ret + 1] = 0;
            if (!hexin && (stx_etx_str((u_char *) buf, ret)))
                /* PULNIX TM-1010 fmt */
            {
                for (i = 0; i < ret; i++)
                {
                    switch (buf_p[i])
                    {
                    case 0x02:
                        sprintf(&stRetstr[strlen(stRetstr)], "[STX]");
                        break;

                    case 0x03:
                        sprintf(&stRetstr[strlen(stRetstr)], "[ETX]");
                        break;

                    case 0x06:
                        sprintf(&stRetstr[strlen(stRetstr)], "[ACK]");
                        break;
                    case 0x15:
                        sprintf(&stRetstr[strlen(stRetstr)], "[NAK]");
                        break;
                    default:
                        sprintf(&stRetstr[strlen(stRetstr)], "%c", buf_p[i]);
                    }
                    edt_msg(DEBUG2, "");
                }
            }
            /* Hex (or other non-ASCII */
            else if (hexin || (!isascii_str((u_char *) buf_p, ret)))
            {
                int     i;

                if (ret)
                {
                    for (i = 0; i < ret; i++)
                        sprintf(&stRetstr[strlen(stRetstr)], "%s%02x", i ? " " : "", (u_char) buf_p[i]);
                }
            }
            else		/* simple ASCII */
            {
                sprintf(&stRetstr[strlen(stRetstr)], "%s", strip_crlf(buf_p));
            }
            buf_p += ret;
            length += ret;
        }
        if (pdv_get_waitchar(pdv_p, &waitc) && (lastbyte == waitc))
            ret = 0; /* jump out if waitchar is enabled/received */
        else pdv_serial_wait(pdv_p, pdv_p->dd_p->serial_timeout, 64);
    } while (ret > 0);

    return stRetstr;
}

static long
isascii_str(u_char * buf, int n)
{
    int     i;

    for (i = 0; i < n; i++)
        if ((buf[i] < ' ' || buf[i] > '~')
            && (buf[i] != '\n')
            && (buf[i] != '\r'))
            return 0;
    return 1;
}

long
stx_etx_str(u_char * buf, int n)
{
    int     i;

    if ((buf[0] != 0x02) || (buf[n - 1] != 0x03))
        return 0;

    for (i = 1; i < n - 1; i++)
        if ((buf[i] < ' ' || buf[i] > '~')	/* any ASCII */
            && (buf[i] != 0x6)	/* ACK */
            && (buf[i] != 0x15))/* NAK */
            return 0;
    return 1;
}

void
propeller_sleep(int n)
{
    int     i;
    char    prop_position[5] = "-\\|/";

    for (i = 0; i < n * 2; i++)
    {
        edt_msg(DEBUG1, "%c\r", prop_position[i % 4]);
        fflush(stdout);
        edt_msleep(500);
    }
}

char    scRetStr[256];
char   *
strip_crlf(char *str)
{
    char   *p = str;

    scRetStr[0] = '\0';

    while (*p)
    {
        if (*p == '\r')
            sprintf(&scRetStr[strlen(scRetStr)], "\\r");
        else if (*p == '\n')
            sprintf(&scRetStr[strlen(scRetStr)], "\\n");
        else
            sprintf(&scRetStr[strlen(scRetStr)], "%c", *p);
        ++p;
    }

    return scRetStr;
}


/*
* special setup for CL2 Camera Link simulator
*/
void
setup_cl2_simulator(EdtDev *edt_p, Dependent *dd_p)
{
    edt_msg(DEBUG1, "STUB setting up camera link simulator...\n");
}

/*
* takes a colon separated string of xilinx commands, parses them into
* individual commands, and sends one at a time to the interface xilinx
*/
int
send_xilinx_commands(EdtDev * edt_p, char *str)
{
    char   *p;
    char   *nextp;
    int     len;
    char    cmdstr[32];
    char    cmd[32];
    u_int   addr;
    unsigned long lvalue;
    u_char  value;

    edt_msg(DEBUG1, "sending xilinx commands....\n");

    p = str;

    while ((nextp = strchr(p, ':')))
    {
        len = (int)(nextp - p);
        if (len > 31)
        {
            edt_msg(EDT_MSG_FATAL, "ERROR: xilinx cmd too long\n");
            return -1;
        }

        strncpy(cmdstr, p, len);
        cmdstr[len] = 0;

        sscanf(cmdstr, "%s %x %lx", cmd, &addr, &lvalue);
        if (addr < 0xffff)
            addr |= 0x01010000;
        value = (unsigned char) (lvalue & 0xff);
        edt_msg(DEBUG1, "%s %08x %02x\n", cmd, addr, value);

        if (strcmp(cmd, "w") == 0)
        {
#if 0
            edt_intfc_write(edt_p, addr, value);
#else
            edt_msg(PDVLIB_MSG_WARNING, "ALERT: edt_intfc_write commented out for testing only\n");
#endif
        }
        /* else other commands here */
        else
        {
            edt_msg(PDVLIB_MSG_WARNING, "unknown xilinx command %s\n", cmd);
        }

        p = nextp + 1;
    }
    return 1;
}
