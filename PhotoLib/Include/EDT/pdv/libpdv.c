/* #pragma ident "@(#)libpdv.c	1.398 03/15/07 EDT" */

/**
* @file 
* PDV Library, most subroutines.
*
* @defgroup dv EDT Digital Imaging Library
*
* The PDV digital imaging library (pdvlib) provides a C language interface to the PDV
* device driver, including routines for image capture, save, and device control.
*
* The library is designed for use with all EDT Digital Imaging boards, including the VisionLink series,
* PCIe and PCI DV, DVa and DV series Camera Link and legacy AIA interfaces, and PMC and Compact PCI
* variants. The library also has components to support the Camera Link simulator boards including 
* the PCI DV CLS, PCIe8 DV CLS and PCIe8 DVa CLS.
*
* The PDV library sits on top of the
* lower-level \ref dma (edtlib). Library functions from both libraries operate on the same device handle,
* and routines from both libraries can be used in the same application. However pdvlib (<code>pdv_</code>)
* subroutines are designed to handle the extra bookeeping, error-recovery, triggering and
* timing functionality that is present on EDT Digital Imaging boards. Therefore direct calls to edtlib 
* (<code>edt_</code>) subroutines should only be made when they provide functionality that is not present
* in an equivalent or similar pdvlib call. Most notable are the DMA image capture subroutines --
* pdvlib DMA should always be used (e.g. #pdv_multibuf, #pdv_start_images, #pdv_wait_images),
* rather than calling the lower-level edtlib DMA subroutines directly (e.g. #edt_configure_ring_buffers,
* #edt_start_buffers, #edt_wait_for_buffers.) However this restriction does not apply to the \ref msg.
*
<a href="http://www.edt.com/api/EDTapi.pdf">Complete EDT API reference in PDF format</a>
*
* @note Applications that access EDT boards must be linked with appropriate library (32 or 64-bit) for the
* platform in use. Applications linked with 32-bit EDT libraries will not run correctly on 64-bit
* systems, or vice-versa.
*
* All routines access a specific device whose handle is created and returned by
* the #pdv_open or #pdv_open_channel routine. PDV applications typically include the
* following elements:
*  -# The preprocessor statement:
*  @code
*  #include "edtinc.h"
*  @endcode
*  -# A call to #pdv_open or #pdv_open_channel, such as:
*  @code
* PdvDev *pdv_p = pdv_open_channel(EDT_INTERFACE, 0, 0);
*  @endcode
*   (EDT_INTERFACE is defined as \c "pdv" in edtdef.h.)
*  -# Device control or status calls, such as #pdv_get_height, as in:
*  @code
* int height = pdv_get_height(pdv_p);
*  @endcode
*  -# Ring buffer initialization code, such as:
*  @code
* pdv_multibuf(pdv_p, 4) ;
*  @endcode
*  -# Data acquisition calls, such as #pdv_image (which acquires an image and returns a pointer to it),
* as in:
* @code
* unsigned char *image = pdv_image(pdv_p) ;
* @endcode
*  -# A check for timeouts (to flag a problem in the case of an unplugged camera, misconfiguration, or other reason for data loss), as in:
* @code
* int t = pdv_timeouts() ;
* @endcode
* followed by appropriate action if new timeouts are detected, such as error output & timeout recovery code per simple_take.c
*  -# A call to #pdv_close to close the device before ending the program, as in:
*  @code
* pdv_close(pdv_p) ;
*  @endcode
*  -# Appropriate settings in your makefile or C workspace to compile and link
*  the library files.
*  -# On Linux systems, the \e -lpdv and \e -ledt option to the compiler, to
*  link the library file \e libpdv.so with your program.
*  -# On Linux systems, the \e -L and \e -R options to specify where to find the
*  dynamic libraries at link- and runtime respectively. (See the makefile
*  provided for examples.)
*  -# On Linux systems, the \e -mt option to the compiler (because the library
*  uses multithreading), and the \e -lm option to the compiler (because it uses
*  the math library).
*
* To compile the library as a shared (.so) library on Linux, the following steps are necessary:
* -# run
* @code
* make clean
* @endcode
* -# add
* @code
* -fPIC
* @endcode
* to the ALL_CFLAGS macro in the makefile
* -# switch the library macro in the makefile:
* @code
* LIBRARY=$(PDVLIB)
* @endcode
*  
* See the \e makefile and example programs provided in the install directory for examples of compiling code using the digital
* imaging library routines. Windows packages include a Visual Studio (8) solution and project files in <code><em>install_dir</em>\\projects.vs2008</code>.
*
* Suggested starting points for acquisition are the simple_take.c, simplest_take.c and other
* <b>simple_*.c</b> example programs. For serial communication, see serial_cmd.c, a command line
* serial utility. Other <b>simple_*.c</b> example programs are provided to show specialized
* functionality.
*
* The  #PdvDev device status structure is defined in the file libpdv.h. It
* includes the #PdvDependent substructure, and other structure elements that describe
* the state of the board and camera, as initialized by the current camera configuration file
* (see the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</a>)
* or modified by any subsequent API setup calls.  These structure elements include values for
* things such as the current pixel re-order or color interpolation method, size and depth
* of the image, number and size of currently allocated buffers.  To ensure compatibility with future
* versions of the library, programmers should always use the library calls for getting / setting any
* library values, and refrain from referencing the structure elements directly. Additionally, anything
* that can be queried via the subroutine calls such as currently set image width, height and depth should be
* done via subroutine calls rather than hard-coding specific values.
*
* The PDV library source files are included in the installation. Most but not
* all routines are documented here.  Undocumented routines include internals, custom,
* special purpose and obsoletes.  Feel free, however, to look through the library
* source code and use (with caution) any routines that are appropriate.  Email
* tech@edt.com if you have questions about specific routines.
*
* @note When acquiring images in multithreaded applications, all routines that
* deal with starting, waiting for, or aborting images or buffers should be in
* the same thread.
*
* Routines are divided into the following modules. You can use the Search button
* at the top of this page (in the HTML version of this doc) to search for specific
* library routines.
*/

/** 
* @defgroup updown Startup / Shutdown
* To open and close the EDT digital imaging device.
* #pdv_open and #pdv_open_channel differ only in the \e channel argument.
* Since many applications are written for single channel boards (for example, the VisionLink F1) 
* pdv_open will often suffice for opening a handle to the device. However it is just as easy to
* use #pdv_open_channel with zero-assigned variable in the \e channel argument, providing for
* future possible expansion to multiple channel boards.
*
* @ingroup dv
*/

/** 
* @defgroup settings Settings
* @ingroup dv
* Get and set EDT interface board (register) values as well as device
* driver and camera settings.
*
* Most values get initialized from the config file, via the initcam program or the
* the camera configuration dialog (see pdv_initcam, initcam.c and the
* <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>).
* In many cases the "get" routines are all that are used from an application, but
* sometimes it is useful for an application to make changes as well. These subroutines
* can be used to do both.
*/

/** 
* @defgroup init Initialization
* @ingroup dv
* Read configuration files and initialize the board and camera.
* Typically the external utility program \e initcam handles these tasks
* (possibly invoked by an EDT application such as \e pdvshow or \e camconfig.)
* \e initcam calls these subroutines to do the work, and they are avaiable as well
* for programmers who wish to invoke them directly from a user application. See the
* initcam.c source code for an example of how how to use these subroutines to read
* configration files and initialize the board from within an application.
*/

/** 
* @defgroup acquisition Acquisition
* @ingroup dv
* Image acquisition subroutines.
* The simplest way to acquire an image from an EDT digital imaging board is to use
* #pdv_image (see simplest_take.c for an example).
*
* Using #pdv_start_image / #pdv_wait_image splits image acquisition into \e queue and
* \e retrieve phases, allowing programmers to parallelize image acquisition and
* processing (see simple_take.c).
*
* Using #pdv_start_images with #pdv_wait_images (or #pdv_wait_image) adds prestart /
* queuing for further optimization.  Other subroutines are provided for more
* specialized uses (see other <b><em>simple_*.c</em></b> example programs).
*
* Image acquisition subroutines such as #pdv_wait_image return the data as a
* pointer to the image buffer. Images are not framed in any way, the buffer only
* contains the pixel data. Application programs should use query routines such
* as #pdv_get_width, #pdv_get_height and #pdv_get_depth to find out the data line
* or frame size and number of bits per pixel.
*
* The bitwise format of the pixel data will depend on the number of bits per
* pixel as defined by the camera and configuration file, as well as any
* data deinterleave or demosaicing method (e.g. bayer interpolation) that may be
* enabled via the config file's \b method_interlace directive (exception: \b _raw
* subroutines bypass data re-ordering).  Pixel data for typical formats and
* re-ordering methods are as follows:
*
* <table>
* <tr><td><b>Camera Output</b></td><td><b>Config Attributes (also see the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration guide</A>)</b></td><td><b>Buffer data</b></td></tr>
* <tr><td>Monochrome 8 bits</td><td>depth: 8<br>extdepth: 8<br>cl_data_path_norm: 07 (single ch.) or 17 (dual ch.)</td><td>1 byte/pixel<br></td></tr>
* <tr><td>Monochrome 10-16 bits</td><td>depth: 10, 12, 14 or 16<br>extdepth: same as depth<br>CL_DATA_PATH_NORM: 09, 0b, 0d or 0f (single ch.), 19, 1b, 1d or 1f (dual ch.)</td><td>2 bytes/pixel, msb-justified</td></tr>
* <tr><td>Bayer color 8 bits</td><td>depth: 24<br>extdepth: 8<br>CL_DATA_PATH_NORM: 07 (single ch.), 17 (dual ch.)<br>method_interlace: BGGR</td><td>3 bytes/pixel, B G R<br></td></tr>
* <tr><td>Bayer color 10-16 bits</td><td>depth: 24<br>extdepth: 10, 12, 14 or 16<br>CL_DATA_PATH_NORM: 09, 0b, 0d or 0f (single ch.), 19, 1b, 1d or 1f (dual ch.)<br>method_interlace: BGGR_WORD</td><td>3 bytes/pixel, B G R<br></td></tr>
* <tr><td>RGB color 24 bits</td><td>depth: 24<br>extdepth: 24<br>CL_CFG_NORM: 01</td><td>3 bytes/pixel, B G R<br></td></tr>
* <tr><td>RGB color 30 bits</td><td>depth: 32<br>extdepth: 32<br>rgb30: 1 or 3<br>CL_CFG_NORM: 01<br>(note: PCI DV C-Link and PCIe4/8 DVa C-Link boards must be flashed with medium mode FPGA [see the users guide])</td><td>4 bytes/pixel, 8B 8G 8R 2B 2G 2R 2x<br></td></tr>
* </table>
*/

/** 
* @defgroup serial Communications/Control 
* @ingroup dv
*
* Serial communications and camera control subroutines.
* Subroutines in this section of the library fall into three general
* categories: 1) low level serial communications and control, 2) framing
* commands for cameras that have sophisticated command framing protocols,
* and 3) high level convenience routines for specific operations on selected
* cameras.
*
* These subroutines are used to communicate with cameras that have a serial
* command set.  Since there is (to date) no standard command set, programmers
* who wish to embed camera control commands within applications will need to
* write code that is specific to the camera(s) in use.
*
* Serial control typically consists a command/response sequence, and looks like
* the following:
*
* @code
* pdv_serial_command(pdv_p, command_string); // ASCII; for binary use pdv_serial_binary_command
* n = pdv_serial_wait(pdv_p, timeout, nchars);
* pdv_serial_read(pdv_p, response_string, n); 
* @endcode
*
* The above is the most general purpose method, but it can be slow since
* pdv_serial_wait will only return after the timeout period expires, in order to ensure
* that all of the response characters have come in. If the last character of a response
* is known and can be assured to always be unique within that response, then the use of
* a <em>serial wait</em> character can be used. When set, it causes #pdv_serial_wait to
* return immediately when the character is seen, without waiting for the full timeout period
* to expire:
*
* @code
* pdv_set_waitchar(pdv_p, '\n');    // only needs to be set once
* pdv_serial_command(pdv_p, command_string);
* n = pdv_serial_wait(pdv_p, timeout, nchars); // still use max timeout in case of failure
* pdv_serial_read(pdv_p, response_string, n);
* @endcode
*
* @note When this library was originally developed, there were a relatively small
* number of cameras and camera command sets to deal with. Consequently, subroutines
* written to directly control specific camera parameters such as #pdv_set_exposure,
* #pdv_set_gain and #pdv_set_blacklevel were coded to handle those tasks for the
* majority of cameras that had such functionality. With the proliferation of cameras
* and command sets over the years, these "convenience routines" have become
* less useful. They remain helpful for cameras whose command sets conform to the
* relatively narrow format defined by the \b serial_exposure, \b serial_gain and
* \b serial_offset config directives, but even then such control is limited, so for
* full control of cameras it is usually necessary for programmers to code such control with the lower-level subroutines
* #pdv_serial_command, #pdv_serial_binary_command, #pdv_serial_wait and
* #pdv_serial_read (or specialized framing commands such as
* #pdv_send_basler_frame).  One important exception is if the camera and board
* are to be set up for <em>pulse-width</em>, aka <em>level trigger</em> acquisition
* control (where the length of the board's shutter timer is used to control the length
* of the EXPOSE pulse and consequently the camera's integration time). When using that mode (ebabled via the \b method_camera_shutter_timing configuration directive),
* #pdv_set_exposure <em>should</em> be used, since it also controls the board's
* expose timer.
*/

/** 
* @defgroup utility Utility 
* @ingroup dv
* Various utility subroutines.
* Most PDV utility routines have a \c dvu_ prefix. \c dvu_ subroutines are not necessarily
* specific to the EDT digital imaging hardware. For example, dvu_write_rasfile could concievably
* be used to write a raster file from any source, not just one captured by an EDT framegrabber.
* As such, \c dvu_ subroutines do not operate on an #PdvDev device handle in their
* parameter lists.
*
* There are a few utility subroutines that don't take a #PdvDev device handle but do have
* a \c pdv_ prefix, and may or may not have some PDV specificity.
* 
* The remaining \c pdv_ subroutines that do take a #PdvDev device handle are tagged
* as utility subroutines because they do not fit any other category.
*/

/** 
* @defgroup debug Debug
* @ingroup dv
* Get and set flags that determine debug output from the library.
* For more information, see the \ref msg.
*
*/

/**
* libpdv.c
* 
* PCIbus Digital Imaging Capture Library Main Module 
* 
* Copyright (c) 1993-2006, Engineering Design Team, Inc.
* 
* Provides a 'C' language interface to the EDT PCI bus Digital Imaging
* Camera/Device Capture family of products
* 
* All routines access a specific device, whose handle is created and returned
* by the pdv_open() routine.
*/


#include "edtinc.h"

#include <math.h>
/* #define NDEBUG  here or with make -DNDEBUG to turn off assert */
#include <assert.h>

#include "libedt_timing.h"

#ifdef _NT_
#define strncasecmp strnicmp
#endif

/* shorthand debug level */
#define PDVWARN PDVLIB_MSG_WARNING
#define PDVFATAL PDVLIB_MSG_FATAL
#define DBG1 PDVLIB_MSG_INFO_1
#define DBG2 PDVLIB_MSG_INFO_2

int Pdv_debug = 0;
int     Smd_type = NOT_SET;
int     Smd_rate = NOT_SET;

/*
* library routines
*/


#define PDV_DEPENDENT(pdv_p) ((pdv_p)->dd_p)

int check_register_wrap(EdtDev * edt_p);
static void debug_print_serial_command(char *cmd);
static void send_serial_binary_cmd(PdvDev * pdv_p, char *hexstr, int value);
static void pdv_trigger_specinst(PdvDev * pdv_p);
static void pdv_posttrigger_specinst(PdvDev * pdv_p);
static int pdv_specinst_setparam(PdvDev * pdv_p, char cmd, u_long offset, u_long value);

/** @addtogroup settings
* @{
*/
static int pdv_set_exposure_specinst(PdvDev * pdv_p, int value);
static int pdv_set_gain_specinst(PdvDev * pdv_p, int value);
static int pdv_set_exposure_adimec(PdvDev * pdv_p, int value);
static int pdv_set_exposure_su320(PdvDev * pdv_p, int value);
static int pdv_set_gain_adimec(PdvDev * pdv_p, int value);
static int pdv_set_blacklevel_adimec(PdvDev * pdv_p, int value);
static int pdv_set_exposure_smd(PdvDev * pdv_p, int value);
static int pdv_set_exposure_timc1001pf(PdvDev * pdv_p, int value);
static int pdv_set_exposure_ptm6710_1020(PdvDev * pdv_p, int value);
static int pdv_set_gain_ptm6710_1020(PdvDev * pdv_p, int value);
static int pdv_set_binning_generic(PdvDev * pdv_p, int value);
static int pdv_set_gain_smd(PdvDev * pdv_p, int value);
static int pdv_set_blacklevel_smd(PdvDev * pdv_p, int value);
static int pdv_set_gain_hc8484(PdvDev * pdv_p, int value);
static int pdv_set_exposure_toshiba(PdvDev * pdv_p, int value);
static int pdv_set_gain_toshiba(PdvDev * pdv_p, int value);
static int pdv_set_exposure_cohu(PdvDev * pdv_p, int value);
static int pdv_set_gain_cohu(PdvDev * pdv_p, int value);
static int pdv_set_blacklevel_cohu(PdvDev * pdv_p, int value);
static int pdv_total_block_size(PdvDev *pdv_p, int numbufs);
static void pdv_allocate_output_buffers(PdvDev *pdv_p);
static void pdv_free_output_buffers(PdvDev *pdv_p);
static int pdv_multibuf_block(PdvDev *pdv_p, int numbufs, u_char *block, int blocksize);
static int pdv_multibuf_separate(PdvDev *pdv_p, int numbufs, u_char **buffers);
static int pdv_set_roi_internal(PdvDev *pdv_p, int hskip, int hactv, int vskip, int vactv, int call_setsize) ;
/** @} */

static int pdv_specinst_serial_triggered(PdvDev * pdv_p);
static void CheckSumMessage(unsigned char *msg);
int     pdv_auto_set_timeout(PdvDev * pdv_p);
static int isafloat(char *str);
static int isdigits(char *str);
static int isxdigits(char *str);
static int update_int_from_serial(char **stat, int nstat, char *str, int *value);
static int update_string_from_serial(char **stat, int nstat, char *str, char *value, int maxlen);
static void update_hex_from_serial(char **stat, int nstat, char *str, int *value);
static void update_2dig_from_serial(char **stat, int nstat, char *str, int *val1, int *val2);
int     edt_get_rtimeout(PdvDev * pdv_p);
static int pdv_update_from_kodak_i(PdvDev * pdv_p);

static int pdv_update_from_atmel(PdvDev * pdv_p);
static int pdv_update_from_hamamatsu(PdvDev * pdv_p);
#ifdef IS_UNUSED
static int pdv_set_mode_atmel(PdvDev * pdv_p, char *mode);
static int pdv_set_mode_hamamatsu(PdvDev * pdv_p, char *mode);
#endif

static int pdv_set_exposure_timc1001pf(PdvDev * pdv_p, int value);

int     pdv_query_serial(PdvDev * pdv_p, char *cmd, char **resp);

/* from pdv_interlace.c */

void    pdv_dmy_data(void *buf, int width, int height, int depth);
void    pdv_alloc_tmpbuf(PdvDev * pdv_p);
extern  int pdv_process_inplace(PdvDev *pdv_p);
int 	pdv_update_size(PdvDev * pdv_p);

static char *hex_to_str(char *resp, int n);

#ifdef DOXYGEN_SHOW_UNDOC
/**
* Unless otherwise noted, everthing is just in the 'dv' group.
* @weakgroup dv
* @{
*/
#endif

/**
* Opens an EDT Framegrabber channel for application access.
* Opens the device, which is the first step in accessing the hardware. 
* Allocates the memory for the device struct, as defined in libpdv.h
* (included through edtinc.h), and host memory required to store a
* captured image.
*
* If you only want to use channel 0 on a multi-channel board, you can use
* #pdv_open, but using pdv_open_channel with 0 in the \b channel argument
* is preferred.
* 
* pdv_open_channel provides for multiple cameras on separate channels,
* on boards that have multiple channels.  Calling pdv_open_channel
* with using a specified board and channel returns a pointer to a software
* structure representing the connection to a specific camera -- channel
* 0 for the camera on the connector closest to the PCI bus, channel 1
* for the next connector up, and so on. Each call to pdv_open_channel
* with a unique channel number returns a discrete pointer, which is
* handled separately from any others, just as if each camera were
* connected to separate boards. 
*
* @Example
* @code
* // Example of opening and acquiring images from two cameras connected
* // to separate channels 0 and 1 of a single VisionLink F4, PCIe8 DVa C-Link,
* // or other multi-channel EDT Digital Imaging board
*
* PdvDev *pdv_p0 = pdv_open_channel(EDT_INTERFACE, 0, 0);
* PdvDev *pdv_p1 = pdv_open_channel(EDT_INTERFACE, 0, 1);
*
* unsigned char *image_p0 = pdv_image(pdv_p0);
* unsigned char *image_p1 = pdv_image(pdv_p1);
* @endcode
*
* @note
* Acquiring data from multiple channels at the same time (or from multiple boards on the
* same bus) increases amount of data going across the bus. Unless the aggregate data is within
* the available bus bandiwdth, bus saturation (in the form of dropped data, broken images,
* overruns, or timeouts) is likely to occur. For more on bandwidth requirements, see EDT's
* <a href="http://www.edt.com/requirements.html"> System Requirements web page</a>.
*
* @param dev_name	The name of the device, which for all EDT Digital
* Imaging boards is \c "pdv". If dev_name is NULL, \c "pdv" will be assumed.
* EDT_INTERFACE can also be used (recommended); it's defined as
* \c "pdv" in edtdef.h. 
*
* @param unit Unit number of the device (board). The first device is 0.
* @param channel The channel of the specified unit to open. The first channel is 0.
* 
* @return A pointer to the #PdvDev data structure, if successful. This
* data structure holds information about the device which is needed
* by library functions.  User applications should avoid accessing
* structure elements directly.
* NULL is returned if unsuccessful.
* @see pdv_open
* 
* @ingroup updown
*/
PdvDev *
pdv_open_channel(const char *dev_name, int unit, int channel)
{
    return pdv_open_device(dev_name, unit, channel, 1);
}

/** Open the pdv device, with option to suppress non-existent device console output.
 *
 * #pdv_open_channel calls this subroutine with the verbose argument set to 1, so
 * this subroutine can be directly instead with verbose set to 0 if you want to suppress the
 * console warning message that gets output if attempting to open a non-existent device.
 * Since that's not a bad thing to know (and usually doesn't show up in Windows non-console
 * apps anyway), we recommend using #pdv_open_channel unless you specifically want to suppress
 * those messages.
 *
 * Other than the verbose argument, this is the same as #pdv_open_channel see that subroutine
 * for a full description.

 * @param verbose verbose output (default 1 for pdv_open_channel) or not (0)
 * @param param see #pdv_open_channel for other parameters
 * @return see #pdv_open_channel
 */
PdvDev *
pdv_open_device(const char *dev_name, int unit, int channel, int verbose)
{
    PdvDev *pdv_p;
    char    tmpname[64];
    Dependent *dd_p;
    static char *debug_env = NULL;
    int     level;

    if ((debug_env == NULL)
        && ((debug_env = (char *) getenv("PDVDEBUG")) != NULL)
        && *debug_env != '0')
    {
        Pdv_debug = atoi(debug_env);
        level = edt_msg_default_level();
        if (Pdv_debug > 0)
        {
            level |= DBG1;
            level |= DBG2;
        }
        edt_msg_set_level(edt_msg_default_handle(), level);

        edt_msg(DBG2, "environment DEBUG set to %d: enabling debug in pdvlib\n", Pdv_debug);
    }

    edt_msg(DBG2, "pdv_open_channel('%s', %d, %d)\n", dev_name ? dev_name : "NULL",
        unit, channel);

    if (dev_name == NULL)
        strcpy(tmpname, EDT_INTERFACE);
    else
        strcpy(tmpname, dev_name);
    if ((pdv_p = edt_open_device(tmpname, unit, channel, verbose)) == NULL)
        return NULL;


    /*
    * alloc and get the PDV dependent methods struct, which should have
    * been initialized in the driver by initcam.
    */
    if (sizeof(Dependent) > EDT_DEPSIZE)
    {
        edt_msg(PDVWARN, "pdv_open_channel: sizeof Dependent %d > DEPSIZE %d\n",
            sizeof(Dependent), EDT_DEPSIZE);
    }
    if ((dd_p = (Dependent *) malloc(EDT_DEPSIZE)) == NULL)
    {
        pdv_close(pdv_p);
        return NULL;
    }
    pdv_p->dd_p = dd_p;

    if (edt_get_dependent(pdv_p, dd_p) < 0)
    {
        free(dd_p);
        dd_p = 0;
        pdv_p->dd_p = NULL;
        pdv_close(pdv_p);
        return NULL;
    }

    pdv_p->tmpbufsize = 0;

    pdv_p->dd_p->xilinx_rev = 2;

    if (pdv_p->dd_p->swinterlace ||
        pdv_p->dd_p->interlace_module[0])
        pdv_setup_postproc(pdv_p, pdv_p->dd_p, NULL);

    return pdv_p;

}



/**
* Opens channel 0 of an EDT Framegrabber for application access.
* Opens the device, which is the first step in accessing the hardware. 
* Allocates the memory for the device struct, as defined in libpdv.h
* (included through edtinc.h), and host memory required to store a
* captured image.
*
* To open a specific channel on multi-channel device, see pdv_open_channel.
*
* @param dev_name	The name of the device, which for all EDT Digital
* Imaging boards is \c "pdv". If dev_name is NULL, \c "pdv" will be assumed.
* \c EDT_INTERFACE can also be used (recommended); it's defined as
* \c "pdv" in edtdef.h. 
*
* @param unit Unit number of the device (board). The first device is 0.
* 
* @return A pointer to the #PdvDev data structure, if successful. This
* data structure holds information about the device which is needed
* by library functions.  User applications should avoid accessing
* structure elements directly.
* NULL is returned if unsuccessful.
* @see pdv_open_channel
* 
* @ingroup updown
*/

PdvDev *
pdv_open(char *dev_name, int unit)
{
    return pdv_open_channel(dev_name, unit, 0);
}


/**
* Sets up device for DMA. 
* Generally only for internal use.
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return void
* @ingroup acquisition 
*/
void
pdv_setup_dma(PdvDev * pdv_p)

{
    /* This looks like bad stuff to put in the open function... */
    /* Since we might not always want to affect */

    edt_set_continuous(pdv_p, 0);
    pdv_p->dd_p->started_continuous = 0;

    edt_startdma_reg(pdv_p, PDV_CMD, PDV_ENABLE_GRAB);
}


/**
* Closes the specified device and frees the device struct and 
* image memory.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return 0 if successful, -1 if unsuccessful
* @ingroup updown
*/
int
pdv_close(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_close()\n");

    if (!pdv_p)
        return -1;
    if (pdv_p->dd_p)
    {
        free(pdv_p->dd_p);
        pdv_p->dd_p = 0;
    }

    pdv_free_output_buffers(pdv_p);

    return edt_close(pdv_p);
}

/**
* Returns bytes per line based on width and bit depth, including depth
* < 8.
* @param width pixels per line
* @param depth bits per pixel
* @return bytes per line
* @ingroup utility 
*/

int
pdv_bytes_per_line(int width, int depth)

{
    if (depth == 1)
        return width >> 3;
    else if (depth == 2)
        return width >> 2;
    else if (depth == 4)
        return width >> 1;
    else if (depth == 80)
        return width + (width/4);
    else
        return width * bits2bytes(depth);
}

/**
* Gets the number of bytes per image, based on the set width, height,
* and depth. Functionally equivalent to pdv_get_imagesize.
* @param pdv_p pointer to pdv device structure returned by #pdv_open
* @ingroup settings 
*/
int
pdv_get_bytes_per_image(PdvDev *pdv_p)

{
    return pdv_p->dd_p->height * 
        pdv_bytes_per_line(pdv_p->dd_p->width, pdv_p->dd_p->depth);
}


/** @addtogroup settings 
* @{
*/

/**
* Gets the width of the image (number of pixels per line), based on the
* camera in use.
*
* If the width has been changed by setting a region of interest, the
* modified values are returned; use #pdv_get_cam_width to get the unchanged width.
*
* @param pdv_p  device struct returned by pdv_open
* @return Width in pixels of images returned from an aquire.
*/
int
pdv_get_width(PdvDev * pdv_p)
{
    if (!pdv_p->dd_p)
        return (0);

    edt_msg(DBG2, "pdv_get_width() %d\n", pdv_p->dd_p->width);


    return pdv_p->dd_p->width;
}

/**
* Gets the number of bytes per line (pitch). Functionally equivalent to pdv_get_width.
*
* @param pdv_p  device struct returned by pdv_open
* @return width in pixels of images returned from an aquire.
* @see pdv_get_width
*/

int
pdv_get_pitch(PdvDev * pdv_p)
{
    int pitch;

    if (!pdv_p->dd_p)
        return (0);

    pitch = pdv_bytes_per_line(pdv_p->dd_p->width, 
        pdv_p->dd_p->depth);

    edt_msg(DBG2, "pdv_get_pitch() %d\n", pitch);


    return pitch ;
}

/**
* Returns the camera image width, in pixels, as set by the
* configuration file directive \b width.
*
* Not to be confused with #pdv_get_width; this subroutine gets the
* pdv_p->dd_p->cam_width value which only exists as a place to record
* the camera's (presumably) full width, as set by the config file
* 'width' directive and unaffected by any subsequent region of interest
* or pdv_setsize changes.
* Generally only useful to provide a hint to applications
* that want to know the original camera size since the value returned doesn't
* necessarily reflect the actual size of the buffers, frame passed in as
* modified by padding, headers or region of interest.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open
*
* @return Image width in pixels.
*
* @see pdv_get_dmasize, pdv_image_size, \b width directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
*/
int
pdv_get_cam_width(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_get_cam_width() %d\n", pdv_p->dd_p->cam_width);

    return pdv_p->dd_p->cam_width;
}

/**
* Returns the actual amount of image data for DMA.
* Normally DMA is the same as the size of the sensor output (width x height x depth in bytes),
* so for example a 1K x 1k 8 bits per pixel camera would be 1024x1024x1 = 1048576 bytes, and
* a 1K x 1k 10 bits per pixel camera would be 1024x1024x2 = 2097152. However it can be different
* in a number of cases:
* <ul>
* <li>If DMA header data is enabled (for IRIGB timestamp input for example), dmasize will be imagesize plus the size
* of the header</li>
* <li>If the sensor is a bayer or other interpolated image with one of the interleave options enabled (via the method_interlace: BGGR_WORD directive in the config file for example), imagesize will be at least 3x dmasize.</li>
* <li>If the data is packed (e.g. 10-bit 8-tap mode), dmasize will be the exact size of the data coming in in bits, but imagesize will be the unpacked data size</li>
* </ul>
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open
* @return DMA size in bytes -- that is, the actual number of bytes acquired plus any added DMA if header data WITHIN the data is specified -- see #pdv_get_header_position, #pdv_extra_headersize
*
* @see pdv_image_size
*/
int
pdv_get_dmasize(PdvDev * pdv_p)
{
    Dependent *dd_p = PDV_DEPENDENT(pdv_p);

    int     size;
    int     extra_dma = 0;

    /* NEW header_within allocates extra buffer AND DMA the size of header_size */
    if (pdv_p->dd_p->header_position == HeaderBegin ||
        pdv_p->dd_p->header_position == HeaderMiddle ||
        pdv_p->dd_p->header_position == HeaderEnd)
        extra_dma = pdv_p->dd_p->header_size;

    if (dd_p->swinterlace == PDV_INV_RT_INTLV_24_12)
        size = dd_p->width * dd_p->height * 3 / 2;
    else if (dd_p->swinterlace == PDV_INTLV_10BIT_8TAP_PACKED
          || dd_p->swinterlace == PDV_INTLV_10BIT_8TAP_TO_8BIT)
        size = dd_p->height * pdv_bytes_per_line(pdv_p->dd_p->width, 80);
    else if (dd_p->depth > dd_p->extdepth) /* 24 bit bayer filter  or 1 bit or 80-bit data */
        size = dd_p->height * pdv_bytes_per_line(pdv_p->dd_p->width, pdv_p->dd_p->extdepth);
    else size = dd_p->height * pdv_bytes_per_line(pdv_p->dd_p->width, pdv_p->dd_p->depth);

    return size + extra_dma;
}

/**
* Sets the width and height of the image. Tells the driver what width
* and height (in pixels) to expect from the camera. This call is
* ordinarily unnecessary in an application program, because the width
* and height are set automatically when \e initcam runs. Exceptions can
* occur, however; for example, if the camera's output size can be
* changed while running, or if the application performs setup that
* supersedes initcam.  This routine is provided for these special
* cases.
* 
* 5/17/2012: added call to pdv_set_roi to specified width and height, 
* eliminating the need to call it separately, given that ROI is usually
* enabled by default.
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open
* @param width width of the image in pixels.
* @param height height of the image in pixels.
*
* @return 0 on success, -1 on failure. 
*/
int
pdv_setsize(PdvDev * pdv_p, int width, int height)
{
    Dependent *dd_p = pdv_p->dd_p;
    int wuz_enabled = dd_p->roi_enabled;

    edt_msg(DBG2, "pdv_setsize(%d, %d)\n", width, height);

    dd_p->width = width;
    dd_p->height = height;
    pdv_set_roi_internal(pdv_p, dd_p->hskip, width, dd_p->vskip, height, 0);

    return pdv_update_size(pdv_p);

}

/**
* Sets placeholder for original full camera frame width, unaffected by
* ROI changes and usually only called by pdv_initcam.
*
* Not to be confused with pdv_set_width; this subroutine sets the
* pdv_p->dd_p->cam_width value, which only exists as a place to record
* the camera's (presumably) full width, normally set by the config file
* 'width' directive and unaffected by any subsequent region of interest
* or pdv_setsize changes. 
* Generally only useful to provide a hint to applications
* a way to change that value, though it normally only gets called by
* pdv_initcam.  Doesn't change the buffer sizes or region of interest --
* for that, use pdv_set_roi or pdv_setsize.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open
* @param value width of the camera's sensor in pixels
*
* @return 0 on success, -1 on failure. 
*/
int
pdv_set_cam_width(PdvDev * pdv_p, int value)
{
    int     ret;
    Dependent *dd_p = pdv_p->dd_p;

    edt_msg(DBG2, "pdv_set_cam_width(%d)\n", value);

    dd_p->cam_width = value;

    ret = edt_set_dependent(pdv_p, dd_p);

    return ret;
}

/**
* Returns the size of the image, absent any padding or header data.
* Since padding and header data are usually absent, the value returned
* from this is usually the same as that returned by #pdv_image_size.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open
* @returns image size, in bytes.
* @see pdv_image_size
*
*/
int
pdv_get_imagesize(PdvDev * pdv_p)
{

    edt_msg(DBG2, "pdv_get_imagesize() %d\n", pdv_p->dd_p->imagesize);

    return pdv_p->dd_p->imagesize;
}

/**
* Returns the allocated size of the image, including any header and
* pad for page alignment. 
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open
* @returns allocated size, in bytes.
* @see pdv_image_size, pdv_get_header_dma
*/
int
pdv_get_allocated_size(PdvDev * pdv_p)
{
    Dependent *dd_p = pdv_p->dd_p;
    int     total = 0;

    dd_p->imagesize = dd_p->width * dd_p->height * bits2bytes(dd_p->depth);

    total = pdv_p->dd_p->imagesize + pdv_p->dd_p->slop + pdv_p->dd_p->header_size;

#ifdef _NT_

    if (total % PAGESIZE)
    {
        total = ((total / PAGESIZE) + 1) * PAGESIZE;
    }
#endif

    edt_msg(DBG2, "pdv_get_allocated_size() %d\n", total);

    return total;

}

/**
* Sets the length of time to wait for data on acquisition before timing out.
*
* The default timeout value is set at initcam time and recalculated / updated
* whenever pdv_set_exposure is called (see #pdv_auto_set_timeout). Calling this
* routine with a value of 0 or greater overrides the automatic value. If called with
* a value of 0, acquisition routines such as #pdv_image and #pdv_wait_image will wait
* forever for (block) the amount of data requested. A value between 0 and 65535
* sets the timeout to a fixed time (millisecond units). A value of -1 tells the
* driver to revert to the automatically calculated value.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param value the number of milliseconds to wait for timeout, or 0 to block
* waiting for data, or -1 to revert to automatic timeouts
*
* @return 0 on success, -1 on failure. 
* @see pdv_auto_set_timeout, \b user_timeout directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
* @ingroup acquisition 
*/
int
pdv_set_timeout(PdvDev * pdv_p, int value)
{
    Dependent *dd_p = pdv_p->dd_p;

    if (value < 0)
    {
        edt_msg(DBG2, "pdv_set_timeout(%d) (< 0, going back to auto)\n", value);

        pdv_p->dd_p->user_timeout_set = 0;
        edt_set_dependent(pdv_p, dd_p);
        return pdv_auto_set_timeout(pdv_p);
    }
    else
    {
        edt_msg(DBG2, "pdv_set_timeout(%d) (user set, overriding auto)\n", value);

        pdv_p->dd_p->user_timeout_set = 1;
        pdv_p->dd_p->user_timeout = value;
        edt_set_dependent(pdv_p, dd_p);
        return edt_set_rtimeout(pdv_p, value);
    }
}

/**
* Gets the length of time to wait for data on acquisition before timing out.
*
* A default time value for this is calculated based on the size of the image
* produced by the camera in use and set by #pdv_open.  If this value is 0,
* acquisition routines such as #pdv_image and #pdv_wait_image will wait forever
* for (block) the amount of data requested.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open
*
* @return Timeout value, in milliseconds.
*
* @ingroup acquisition 
*/
int
pdv_get_timeout(PdvDev * pdv_p)
{

    edt_msg(DBG2, "pdv_get_timeout()\n");

    return edt_get_rtimeout(pdv_p);
}

/**
* Sets the length of time to wait for data on acquisition before timing out.
*
* This function is only here for backwards compatibility. You should use 
* pdv_set_timeout() instead.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param value the number of milliseconds to wait for timeout, or 0 to block
* waiting for data
*
* @returns 0 if successful, nonzero on failure.
*/
int
pdv_picture_timeout(PdvDev * pdv_p, int value)
{
    return pdv_set_timeout(pdv_p, value);
}

/** @} */ /* end settings */


/** @addtogroup acquisition
* @{
*/



/**
* Returns the number of times the device timed out (frame didn't transfer 
* completely or at all) since the device was opened.
*
* Timeouts occur when some or all of the image failed to transfer. Reasons for this
* range from an unplugged cable to misconfiguration to system bandwidth saturation.
* If broken images, slow frame rates or blank images are encountered, it will usually
* be associated with an image timeout.
*
* Frequent timeouts can be a result of the board being in a non-optimal bus slot
* or other system-related issues. This is especially true with legacy PCI devices
* such as the PCI DV C-Link. For more information on optimizing your configuration
* (and system requirements in general) see EDT's
* <a href="http://www.edt.com/requirements.html"> System Requirements web page</a>.
*
* A robust application will check to see whether the timeout counter has increased
* after every new acquire, and take appropriate action. Since timeouts are often
* associated with data overruns or underruns, they frequently indicate an out-of-synch
* condtion. So for continuous captures, applications should perform a reset and
* restart following detection of a timeout, by calling #pdv_timeout_restart.
*
* Various factors can prevent timeouts from being reported when data is dropped.
* With some versions of board firmware, if a small amount of data is lost on a line,
* the board's region-of-interest (ROI) logic will fill in the missing data using
* blanking between lines, preventing a timeout from occurring (but still resulting in
* an out-of-synch frame.) This situation can usually be rectified by updating the board
* firmware, since the new versions (e.g. PCIe board FW versions 14 and later) have the
* blanking feature disabled by default.
*
* Hardware continuous mode (enabled via #pdv_start_hardware_continuous or the fv_once
* or hardware_continuous config file directives) can be problmatic for timeouts.
* Since these modes cause the board to ignore all FVAL (frame start) signals beyond
* the first one in a continuous sequence, losses of relatively small amounts of
* data won't trigger a timeout, resulting in a persistently out-of-synch condition.
* The framesync footer logic shown in the \b simple_irig2.c example
* application was designed as a workaround for this, and more recently (e.g. driver/library
* versions 5.3.9.4 and later) the framesync logic was incorporated into pdv_timeouts,
* providing a convenient and transparent way to ensure detection of and recovery from 
* an out-of-synch condition without the need to change any code. See  #pdv_enable_framesync
* and the the \b method_framesync directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
* for more on this.
*
*
* @see pdv_timeout_restart, pdv_enable_framesync, \b user_timeout and \b method_framesync directives in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>.
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
*
* @Example
* @code
* int t, last_timeouts = 0;
*
* t = pdv_timeouts(pdv_p);
* if (t > last_timeouts)
* {
*   printf("got timeout\n");
*
*   // (add recovery code here -- see simple_take.c for example)
*
*   last_timeouts = t;
* }
* @endcode
*
* @return The number of timeouts since the device was opened.
* @see pdv_set_timeout, pdv_get_timeout, pdv_auto_set_timeout
*/
int
pdv_timeouts(PdvDev * pdv_p)
{
    int     ret;

    ret = edt_timeouts(pdv_p);
    edt_msg(DBG2, "pdv_timeouts(%d)\n", ret);

    return ret;
}


/**
* Cleans up after a timeout, particularly when you've prestarted multiple
* buffers or if you've forced a timeout with #edt_do_timeout. Superseded by
* #pdv_timeout_restart with newer boards such as the VisionLink and PCIe series.
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
*
* @return # of buffers left undone; normally will be used as argument to
* pdv_start_images() if calling routine wants to restart pipeline as if
* nothing happened (see take.c for example of use)
* @see pdv_timeout_restart
*/
int
pdv_timeout_cleanup(PdvDev * pdv_p)
{
    int     curdone, curtodo;

    curdone = edt_done_count(pdv_p);
    curtodo = edt_get_todo(pdv_p);
    pdv_stop_continuous(pdv_p);
    edt_msleep(500);
    edt_set_buffer(pdv_p, curdone);
    edt_reg_write(pdv_p, PDV_CMD, PDV_RESET_INTFC);
    pdv_setup_continuous(pdv_p);
    return curtodo - curdone;
}

/** @} */ /* end acquisition */


/** @addtogroup settings
* @{
*/


/**
* Gets the height of the image (number of lines), based on the camera
* in use.
*
* If the heigth has been changed by setting a region of interest, the
* new values are returned; use #pdv_get_cam_height to get the unchanged
* height.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return Height in pixels of images returned from an aquire.
* @see pdv_get_cam_height, \b height directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>.
*/
int
pdv_get_height(PdvDev * pdv_p)
{
    if (!pdv_p->dd_p)
        return (0);

    edt_msg(DBG2, "pdv_get_height() %d\n", pdv_p->dd_p->height);

    return pdv_p->dd_p->height;
}

/**
* Returns the camera image height, in pixels, as set by the
* configuration file directive \b height, unaffected by changes made by
* setting a region of interest. See #pdv_set_roi for more
* information.
* 
* Not to be confused with pdv_get_height; this subroutine gets the
* pdv_p->dd_p->cam_height value which only exists as a place to record
* the camera's (presumably) full height, as set by the config file
* 'height' directive and unaffected by any subsequent region of interest
* or pdv_setsize changes. This subroutine is just here to give
* applications a way to remember what that is.  Doesn't change the buffer
* sizes or region of interest -- for that, use pdv_set_roi or pdv_setsize.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open
* @return Image height in pixels.
*
* @see pdv_get_height, pdv_get_imagesize, \b width directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>.
*/
int
pdv_get_cam_height(PdvDev * pdv_p)
{

    edt_msg(DBG2, "pdv_get_cam_height() %d\n", pdv_p->dd_p->cam_height);

    return pdv_p->dd_p->cam_height;
}


/**
* Gets the camera image height.
*
* The camera image height is in pixels, as set by the configuration file
* directive \b height, and is unaffected by changes made by setting the region
* of interest.   Typically the value is the same as that returned by #pdv_get_height
* unless the frame_height directive is specified in the config file and is
* different than height. This may occur in some cases where special handling
* of image data by an application is used such as multiple frames per image.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 

* @return the camera image height in pixels

* @see pdv_set_roi, pdv_debug, pdv_get_imagesize
*/
int
pdv_get_frame_height(PdvDev * pdv_p)
{

    edt_msg(DBG2, "pdv_get_cam_height() %d\n", pdv_p->dd_p->cam_height);

    return pdv_p->dd_p->frame_height;
}

/** @} */ /* end settings */

/**
* @if edtinternal
* Updates image size information and reallocates buffers if necessary.
* @internal 
* User applications should not need to call this -- it will
* be called as needed by library functions like #pdv_set_width.
* @ingroup utility 
* @endif
*/ 
int
pdv_update_size(PdvDev *pdv_p)

{
    Dependent *dd_p = pdv_p->dd_p;
    int ret;

    edt_msg(DBG2, "update_size\n");

    dd_p->imagesize = dd_p->height * pdv_get_pitch(pdv_p);

    ret = edt_set_dependent(pdv_p, dd_p);

    if (pdv_p->ring_buffer_numbufs > 0)
        pdv_multibuf(pdv_p, pdv_p->ring_buffer_numbufs);

    if (dd_p->swinterlace)
    {
        pdv_alloc_tmpbuf(pdv_p);
    }

    return ret;
}

/** @addtogroup settings
* @{
*/

/**
* Sets width and reallocates buffers accordingly. Since we rarely ever set
* width and not height, you should normally just use #pdv_setsize to set both.
*
* 5/17/2012: added call to pdv_set_roi to set specified width,
* avoids having to reset ROI separately when the width is changed
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param value the new width.
*/
int
pdv_set_width(PdvDev * pdv_p, int value)
{
    Dependent *dd_p = pdv_p->dd_p;

    edt_msg(DBG2, "pdv_set_width(%d)\n", value);

    dd_p->width = value;

    pdv_set_roi(pdv_p, dd_p->hskip, dd_p->width, dd_p->vskip, dd_p->height);
    return pdv_update_size(pdv_p);

}

/**
* Sets height and reallocates buffers accordingly. Since we rarely ever set height
* and not width, you should normally just use #pdv_setsize to set both at once.
* 
* 5/17/2012: added call to pdv_set_roi to specified height,
* avoids having to reset ROI separately when the height is changed
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param value the new height.
*/
int
pdv_set_height(PdvDev * pdv_p, int value)
{
    Dependent *dd_p = pdv_p->dd_p;

    edt_msg(DBG2, "pdv_set_height(%d)\n", value);

    dd_p->height = value;

    pdv_set_roi(pdv_p, dd_p->hskip, dd_p->width, dd_p->vskip, dd_p->height);

    return pdv_update_size(pdv_p);

}

/**
* Sets placeholder for original full camera frame height, unaffected by
* ROI changes and usually only called by pdv_initcam.
*
* Not to be confused with pdv_set_height; this subroutine sets the
* pdv_p->dd_p->cam_height value, which only exists as a place to record
* the camera's (presumably) full height, normally set by the config file
* 'height' directive and unaffected by any subsequent region of interest
* or pdv_setsize changes. This subroutine is just here to give applications
* a way to change that value, though it normally only gets called by
* pdv_initcam.  Doesn't change the buffer sizes or region of interest --
* for that, use pdv_set_roi or pdv_setsize.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open
* @param value height of the camera's sensor in pixels
* @return 0 on success, -1 on failure. 
*/
int
pdv_set_cam_height(PdvDev * pdv_p, int value)
{
    int     ret;
    Dependent *dd_p = pdv_p->dd_p;

    edt_msg(DBG2, "pdv_set_cam_height(%d)\n", value);

    dd_p->cam_height = value;

    ret = edt_set_dependent(pdv_p, dd_p);
    return ret;
}


/**
* Gets the depth of the image (number of bits per pixel), as set in the
* configuration file for the camera in use.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return Number of bits per pixel in the image.
* @see pdv_set_depth, pdv_get_extdepth, \b depth directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>.
*/
int
pdv_get_depth(PdvDev * pdv_p)
{
    if (!pdv_p->dd_p)
        return (0);

    edt_msg(DBG2, "pdv_get_depth() %d\n", pdv_p->dd_p->depth);

    return pdv_p->dd_p->depth;
}

/**
* Gets the extended depth of the camera.
*
* The extended depth is the number of valid bits per pixel that the
* camera outputs, as set by \e initcam from the configuration file
* \b edtdepth directive.
* Note that if \b depth is set differently than \b extdepth, the actual
* number of bits per pixel passed through by the EDT framegrabber board will be
* different.  For example, if \b extdepth is 10 but \b depth is 8, the
* board will only pass one byte per pixel, even though the camera is
* outputting two bytes per pixel.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open.
* @return The extended depth (an integer).
* 
* @see pdv_get_depth, \b extdepth directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>.
*/
int
pdv_get_extdepth(PdvDev * pdv_p)
{

    edt_msg(DBG2, "pdv_get_extdepth() %d\n", pdv_p->dd_p->extdepth);

    return pdv_p->dd_p->extdepth;
}

/**
* Deprecated -- instead use the combined #pdv_set_depth_extdepth_dpath.
*
* Sets the bit depth and extended depth.
* Depth is the number of valid bits per pixel that the
* board will transfer across the bus.  Extended depth (extdepth) is usally the same but
* not always, for example if we want to pass only the upper 8 bits of data from
* a 12 bit camera, depth will be 8 and extdepth will 12. Bayer color cameras are another
* special case -- for example a 24-bit  RGB camera should have depth set to 24 and extdepth
* to 8.
*
* Normally depth and extended depth
* are  initialized during \e initcam via the configuration file \b depth and \b extdepth directives.
* Therefore, the only time this subroutine should be needed is if the depth changes,
* for example via a post-initialization command to the camera.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param depth the new depth value 
* @param extdepth the new extended depth value 
* @return 0 on success, -1 on failure
* 
* @see pdv_get_depth, pdv_get_extdepth, \b depth, \b extdepth directives in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
*/
int
pdv_set_depth_extdepth(PdvDev * pdv_p, int depth, int extdepth)
{
    /* just call the internal */
    return pdv_set_depth_extdepth_dpath(pdv_p, depth, extdepth, 0);
}

/**
* Sets the bit depth, extended depth, and camera link data path.
*
* Depth is the number of valid bits per pixel that the
* board will transfer across the bus.  Extended depth (extdepth) is usally the same but
* not always, for example if we want to pass only the upper 8 bits of data from
* a 12 bit camera, depth will be 8 and extdepth will 12. Bayer color cameras are another
* special case -- for example a 24-bit  RGB camera should have depth set to 24 and extdepth
* to 8.
*
* This subroutine also allows you to set the camera link data path register for the specific number
* of taps and bits per pixel. Specific value (hex) is as follows:
*
* Left (MS) nibble: number of taps minus 1
*
* Right (LS) nibble: number of bits per pixel minus 1
*
* For example for a 2-tap, 8-bit camera, dpath should be 0x17.
* The correct data path value can usually be inferred automatically from the depth.  If you specify
* a dpath value of 0, pdv_set_depth_extdepth_dpath will automatically set the register to the most likely value.
*
* Normally depth, extended depth and dpath
* are  initialized during \e initcam via the configuration file \b depth and \b extdepth and \b CL_DATA_PATH_NORM directives.
* Therefore, the only time this subroutine should be needed is if the depth changes,
* for example via a post-initialization command to the camera.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param depth the new depth value 
* @param extdepth the new extended depth value 
* @param dpath the new camera link data path value 
* @return 0 on success, -1 on failure
* 

* @see pdv_cl_set_base_channel, spdv_get_depth, pdv_get_extdepth, \b depth, \b extdepth, \b CL_DATA_PATH_NORM directives in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
*/
int
pdv_set_depth_extdepth_dpath(PdvDev * pdv_p, int depth, int extdepth, u_int dpath)
{
    Dependent *dd_p = pdv_p->dd_p;

    dd_p->depth = depth;
    dd_p->extdepth = extdepth;

    /* set cameralink data path register, if appropriate -- note that
    * odball values are generally  handled differently (e.g.
    * pdvcamlk_pir which has hard-coded depth)
    */
    if (pdv_is_cameralink(pdv_p))
    {
        if (dpath)
        {
            edt_reg_write(pdv_p, PDV_CL_DATA_PATH, dpath);
            dd_p->cl_data_path = dpath;
        }
        else
        {
            int reg;

            if ((depth == 10) && (extdepth == 80))
                reg = 0x79; /* 8-tap 10-bit packed */

            else if (depth == 24)
            {
                reg = 0x7;
                if (extdepth == 24) /* true RGB not bayer */
                    reg = 0x27;

            }

            else if (depth == 32)
                reg = 0x07; /* 4-tap RGBI */

            else if (depth == 30) /* is this anywhere? */
                reg = 0x09;

            else if ((depth >= 8) && (depth <= 16)) /* typical 10-16 bit */
                reg = depth-1;

            else reg = 0;

            if (reg)
            {
                if ((dd_p->dual_channel) && ((reg & 0xf0) == 0))
                    reg |= 0x10;
                dd_p->cl_data_path = reg;
                edt_reg_write(pdv_p, PDV_CL_DATA_PATH, dd_p->cl_data_path);
            }
        }
    }

    edt_msg(DBG2, "pdv_set_depth_extdepth_dpath(%d, %d, %02x (%02x))\n", depth, extdepth, dpath, dd_p->cl_data_path);

    return pdv_update_size(pdv_p);

}


/**
* Deprecated -- instead use the combined #pdv_set_depth_extdepth_dpath.
*
* The bit depth is the number of valid bits per pixel that the
* board will transfer across the bus.  Normally depth is initialized
* during \e initcam via the configuration file \b depth directive, and
* the only time this subroutine should be needed is if the depth changes,
* via a post-initialization command to the camera for example. 
*
* Note that if \b depth is set differently than \b extdepth, the actual
* number of bits per pixel passed through by the EDT framegrabber board will be
* different from that received from the camera.  For example, if \b
* extdepth is 10 (matching a camera output of 10 bits) but \b depth is
* 8, the board will only pass one byte per pixel, even though the camera
* is outputting two bytes per pixel. There are also special cases
* including 24-bit depth / 8-bit extdepth (Bayer), and 10-bit depth /
* 80-bit extdepth (8-tap, 10-bit packed).
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param value the new depth value 
* @return The extended depth (an integer).
* 
* @see pdv_set_depth_extdepth, pdv_set_depth_extdepth_dpath, pdv_get_depth, pdv_get_extdepth, \b extdepth directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
*/
int
pdv_set_depth(PdvDev * pdv_p, int value)
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
            if ((dd_p->dual_channel) && ((reg & 0xf0) == 0))
                reg |= 0x10;
            dd_p->cl_data_path = reg;
            edt_reg_write(pdv_p, PDV_CL_DATA_PATH, dd_p->cl_data_path);
        }
    }

    edt_msg(DBG2, "pdv_set_depth(%d)\n", value);
    return pdv_update_size(pdv_p);

}

/**
* Deprecated -- instead use the combined #pdv_set_depth_extdepth_dpath.
*
* Sets the bit depth coming from the camera.
* Normally only called by #pdv_initcam; user applications should avoid calling
* this subroutine directly.
*
* Extdepth must match the number of valid bits per pixel coming from
* the camera. Normally this is initialized during \e initcam
* via the configuration file \b extdepth directive. The only time this
* subroutine should be needed is if the camera's depth changes,
* via a post-initialization command sent to the camera for example. 
*
* Note that if \b depth is set differently than \b extdepth, the actual
* number of bits per pixel passed through by the EDT framegrabber board will be
* different.  For example, if \b extdepth is 10 but \b depth is 8, the
* board will only pass one byte per pixel, even though the camera is
* outputting two bytes per pixel.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param value the extended depth, in bits per pixel
* @return The extended depth (an integer).
* 
* @see pdv_get_extdepth, pdv_set_depth, \b extdepth directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
*/
int
pdv_set_extdepth(PdvDev * pdv_p, int value)
{
    int     ret;
    Dependent *dd_p = pdv_p->dd_p;

    dd_p->extdepth = value;

    edt_msg(DBG2, "pdv_set_extdepth(%d)\n", value);

    ret = edt_set_dependent(pdv_p, dd_p);

    return ret;
}

/**
* Sets the camera's  type (model) string in the dependent structure.
* typically the camera model is set via initcam using the \b camera_model
* configuration file directive. This subroutine is provided in case
* there is a need for an application program to modify the string.
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param model camera model (31 characters max).
* @return 0 on success, -1 on failure.
* @see pdv_get_cameratype, \b cameratype directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
*/
int
pdv_set_cameratype(PdvDev * pdv_p, char *model)
{
    Dependent *dd_p = pdv_p->dd_p;

    edt_msg(DBG2, "pdv_set_cameratype(%s)\n", model);

    strcpy(dd_p->cameratype, model);

    return edt_set_dependent(pdv_p, dd_p);
}


/**
* Gets the type of the camera, as set by \e initcam from the camera
* configuration file's camera description directives. This is a
* concatenation of \b camera_class, \b camera_model, and \b camera_info,
* directives.
* 
* @note the camera class, model and info are for application/GUI information
* only, and are not used in any other way by the driver or library. They are
* provided for the convenience of applications such as PdvShow which uses them
* to help the user choose a specific camera configuration in the camera setup
* dialog.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return String representing the camera type.
* @see pdv_get_camera_class, pdv_get_camera_model, pdv_get_camera_info, \b camera_class, \b camera_model, \b camera_info directives in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
*/
char   *
pdv_get_cameratype(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_get_cameratype()\n");

    return pdv_p->dd_p->cameratype;
}


/**
* Gets the class of the camera (usually the manufacturer name),
* as set by \e initcam from the camera_config file \b camera_class directive.
* 
* @note the camera class is for application/GUI information only, and is not
* used by the driver or library. It is provided for the convenience of
* applications; for example the PdvShow and other camera configuration dialogs
* get and display the camera class, model and info strings to help the user
* to choose a spcific configuration.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return String representing the camera class.
*
* @see pdv_get_cameratype, \b camera_class directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
* @ingroup settings
*/
char   *
pdv_get_camera_class(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_get_camera_class()\n");

    return pdv_p->dd_p->camera_class;
}


/**
* Gets the model of the camera, as set by initcam from the camera_config
* file \b camera_model directive.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return String representing the camera model.
* @see pdv_set_camera_model, \b camera_model directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
* @ingroup settings
*/
char   *
pdv_get_camera_model(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_get_camera_model()\n");

    return pdv_p->dd_p->camera_model;
}

/**
* Gets the string set by the \b camera_info configuration file directive.
* see #pdv_get_cameratype for more information on camera strings.
*  
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return String representing the camera info.
* @see pdv_set_camera_info, \b camera_info directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
* @ingroup settings
*
*/
char   *
pdv_get_camera_info(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_get_camera_info()\n");

    return pdv_p->dd_p->camera_info;
}

/**
* Alias of pdv_get_cameratype 
*
* This is the same as #pdv_get_cameratype (but diff name) and exists 
* for backward compatability. 
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return the camera type 
* @see pdv_get_cameratype
* @ingroup settings
*/
char   *
pdv_camera_type(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_camera_type()\n");

    return pdv_p->dd_p->cameratype;
}

/** @} */ /* end settings */

static int
smd_read_reg(PdvDev * pdv_p, int reg)
{
    u_char  buf[128];
    int     ret;

    /* flush out any junk */
    pdv_serial_read(pdv_p, (char *) buf, 64);

    /* read SMD frame rate register */
    buf[0] = (u_char) reg;
    pdv_serial_binary_command(pdv_p, (char *) buf, 1);

    ret = pdv_serial_wait(pdv_p, pdv_p->dd_p->serial_timeout, pdv_p->dd_p->serial_respcnt);
    if (ret == 0)
        return -1;
    pdv_serial_read(pdv_p, (char *) buf, ret);
    return (int) buf[0];
}

/*
* for serial_format new as of 4.2.3.2 -- allow serial_exposure/gain/blacklevel
* to be a %x or %d or some such, e.g. EXP %d or SM=%02x. Note: Up to 4 % expressions
* can be present but the same value will be entered for each
* ONE % expression. Allow \% to go through 
* RETURNS:  number of % expressions, or -1 if invalid format
*/
static int
check_valid_formatstr(char *str)
{
    int i, len = (int)strlen(str);
    int ret = 0;

    for (i=0; i<len; i++)
    {
        if (str[i] == '%')
        {
            if ((i > 0) && (str[i-1] != '\\')) /* escaped so never mind */
                ++ret;
            else if (i == len-1) /* % can't be the last character unless escaped */
            {
                ret = -1;
                break;
            }
            /* ALERT should be more checks here, for valid formatting */
        }
    }
    return ret;
}

/**
* Sets the exposure time, using the method defined by the directives in the camera configuration file, if set.
*
* pdv_set_exposure will set the exposure (or not) on the camera depending on how
* the related directives are set in the camera configuration file. Specifically, the
* \b method_camera_shutter_timing directive (or #pdv_set_shutter_method) defines whether timing is to be
* controlled via camera serial commands, or by the board via Camera Control (CC) lines.
*
* If \b method_camera_shutter_timing is \b AIA_MCL or \b AIA_MCL_100US and
* something other than 0 is in the left nibble of \b MODE_CNTL_NORM, the board will
* use its internal shutter timer and send out an expose pulse on the specified CC line with
* a TRUE period of the number in milliseconds (AIA_MCL) or tenths of milliseconds (AIA_MCL_100US)
* specified by the \b value parameter. The valid range in either case is 0-25500.
*
* If \b method_camera_shutter_timing is \b AIA_SERIAL (the default), and then this subroutine
* sends the appropriate serial commands based on the \b method_serial_format directive, which
* defines which serial format is to be used. The default format is SERIAL_ASCII, in which case 
* the subroutine will set the exposure by sending the command specified by the \b serial_exposure directive, if present.
* If \b method_serial_format is \b SERIAL_ASCII but there is no \b serial_exposure directive, this subroutine
* is a no-op.
*
* In the case of \b method_serial_format: \b SERIAL_ASCII or any other serial mode, the range is camera
* dependent. Other methods are available that are specific to specific cameras -- see the Camera
* Configuration guide for details.
*
* @note Using this subroutine for other than \b AIA_MCL or \b AIA_100US camera shutter timing modes (that is, any method
* that uses serial) is no longer recommended. Back in the AIA (pre-Camera Link) days, there was a manageable set of
* serial methods, so it made sense to have one subroutine that could control exposure time for all the available methods.
* But the sheer number of different schemes has outgrown this library's ability to keep up, so for any camera command
* sets other than those that use straight ASCII serial with an integer argument, it's more reliable to instead
* send any camera-specific serial commands using #pdv_serial_command, #pdv_serial_binary_command, or #pdv_serial_write.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param value Exposure time. For AIA_MCL or AIA_MCL_100US, the valid range is 0-25500. For other methods, valid range and increments are camera-dependent. 
* @return 0 if successful, -1 if unsuccessful.
* @see pdv_set_shutter_method, pdv_get_shutter_method, pdv_set_exposure_mcl
* @see <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration</A> directives \b MODE_CNTL_NORM, \b serial_exposure & \b method_camera_shutter_timing 
* @ingroup settings
*/
int
pdv_set_exposure(PdvDev * pdv_p, int value)
{
    int     ret = -1;
    Dependent *dd_p = pdv_p->dd_p;
    char    cmdstr[64];
    int     n;

    cmdstr[0] = '\0';

    edt_msg(DBG2, "pdv_set_exposure(%d)\n", value);

    dd_p->shutter_speed = value;

    if (edt_set_dependent(pdv_p, dd_p) < 0)
    {
        edt_msg(DBG2, "pdv_set_exposure ret %d\n", ret);
        return -1;
    }

    pdv_auto_set_timeout(pdv_p);

    if ((dd_p->camera_shutter_timing == AIA_MCL)
        && (dd_p->mode_cntl_norm & 0xf0)
        && (!dd_p->trig_pulse))
    {
        ret = pdv_set_exposure_mcl(pdv_p, value);
    }
    else if (dd_p->camera_shutter_timing == AIA_MCL_100US)
    {
        /* in this case do mcl even if trig_pulse */
        ret = pdv_set_exposure_mcl(pdv_p, value);
    }
    else if ((strlen(dd_p->serial_exposure) >= 0) && (dd_p->serial_format == SERIAL_BINARY))
    {
        send_serial_binary_cmd(pdv_p, dd_p->serial_exposure, value);
    }
    else if ((strlen(dd_p->serial_exposure) >= 0)
        && ((dd_p->serial_format == SERIAL_ASCII)
        || (dd_p->serial_format == SERIAL_ASCII_HEX)
        || (dd_p->serial_format == SERIAL_PULNIX_1010)))
    {
        /* special serial cmd for ham 4880 */
        /* ALERT: get rid of this, s/b camera_shutter_timing here and */
        /* in config files and everywhere else....! */
        if (dd_p->camera_shutter_timing == HAM_4880_SER)
        {
            int     minutes;
            int     seconds;
            int     useconds;

            minutes = value / 60000;
            value -= minutes * 60000;

            seconds = value / 1000;
            value -= seconds * 1000;

            useconds = value;

            sprintf(cmdstr, "%s %04d:%02d.%03d",
                dd_p->serial_exposure, minutes, seconds, useconds);
        }

        else switch(dd_p->serial_format)
        {

            case SERIAL_ASCII_HEX:
                sprintf(cmdstr, "%s %02x", dd_p->serial_exposure, value);
                break;

            case SERIAL_PULNIX_1010:
                sprintf(cmdstr, "%s%d", dd_p->serial_exposure, value);
                break;

            case SERIAL_ASCII:
            default:
                {
                    int n = check_valid_formatstr(dd_p->serial_exposure); 

                    cmdstr[0] = '\0';

                    if (n == 0) /* no % formatting, use old method */
                    {
                        if (dd_p->serial_exposure[0])
                            sprintf(cmdstr, "%s %d", dd_p->serial_exposure, value);
                    }
                    else if ((n > 0) && (n < 5))
                        sprintf(cmdstr, dd_p->serial_exposure, value, value, value, value);

                    if (cmdstr[0])
                    {
                        ret = pdv_serial_command_flagged(pdv_p, cmdstr, SCFLAG_NORESP);

                        pdv_serial_wait(pdv_p, pdv_p->dd_p->serial_timeout, 64);
                        if (*pdv_p->dd_p->serial_response)
                            pdv_serial_read(pdv_p, cmdstr, 63);
                    }
                }

                if (check_valid_formatstr(dd_p->serial_exposure) >= 1)
                    sprintf(cmdstr, dd_p->serial_exposure, value, value, value, value);
                else sprintf(cmdstr, "%s %d", dd_p->serial_exposure, value);
                break;
        }

        if (cmdstr[0])
        {
            ret = pdv_serial_command_flagged(pdv_p, cmdstr, SCFLAG_NORESP);
            n = pdv_serial_wait(pdv_p, pdv_p->dd_p->serial_timeout, pdv_p->dd_p->serial_respcnt);
            if (*pdv_p->dd_p->serial_response)
                if (n)
                    pdv_serial_read(pdv_p, cmdstr, n);
        }
    }
    else if ((dd_p->camera_shutter_timing == SPECINST_SERIAL)
        || (dd_p->camera_shutter_speed == SPECINST_SERIAL))
    {
        ret = pdv_set_exposure_specinst(pdv_p, value);
    }
    else if (dd_p->camera_shutter_timing == SMD_SERIAL)
    {
        ret = pdv_set_exposure_smd(pdv_p, value);
    }
    else if (dd_p->camera_shutter_timing == PTM6710_SERIAL)
    {
        ret = pdv_set_exposure_ptm6710_1020(pdv_p, value);
    }
    else if (dd_p->camera_shutter_timing == TOSHIBA_SERIAL)
    {
        ret = pdv_set_exposure_toshiba(pdv_p, value);
    }
    else if (dd_p->camera_shutter_timing == COHU_SERIAL)
    {
        ret = pdv_set_exposure_cohu(pdv_p, value);
    }
    else if (dd_p->camera_shutter_timing == PTM1020_SERIAL)
    {
        ret = pdv_set_exposure_ptm6710_1020(pdv_p, value);
    }
    else if (dd_p->camera_shutter_timing == TIMC1001_SERIAL)
    {
        ret = pdv_set_exposure_timc1001pf(pdv_p, value);
    }
    else if (dd_p->camera_shutter_timing == ADIMEC_SERIAL)
    {
        ret = pdv_set_exposure_adimec(pdv_p, value);
    }
    else if (dd_p->camera_shutter_timing == BASLER202K_SERIAL)
    {
        ret = pdv_set_exposure_basler202k(pdv_p, value);
    }
    else if (dd_p->camera_shutter_timing == SU320_SERIAL)
    {
        ret = pdv_set_exposure_su320(pdv_p, value);
    }
    else if (dd_p->camera_shutter_timing == HAM_4880_SER)
    {
        /* controlled by serial; do nothing */
    }
    else if (dd_p->camera_shutter_timing == AIA_SERIAL)
    {
        /* controlled by serial; do nothing */
    }
    else if (dd_p->camera_shutter_timing == AIA_SERIAL_ES40)
    {
        /* controlled by serial; do nothing */
    }
    else if (dd_p->camera_shutter_timing == AIA_TRIG)
    {
        ret = pdv_set_exposure_mcl(pdv_p, value);
    }
    else if (!dd_p->trig_pulse)
    {
        /* default to aia mcl if not trig_pulse */
        ret = pdv_set_exposure_mcl(pdv_p, value);
    }

    edt_msg(DBG2, "pdv_set_exposure returns %d\n", ret);
    return (ret);
}

/**
* Set the exposure when in pulse-width mode (also known as level trigger mode).
* Sets data Path register decade bits as appropriate for value input.
* Called by #pdv_set_exposure if dd_p->camera_shutter_timing is set to
* \b AIA_MCL or \b AIA_MCL_100US (typically set by config file directive
* <b>method_camera_shutter_timing: AIA_MCL</b>; (<b>MODE_CNTL_NORM: 10</b>
* should typically also be set). If AIA_MCL, units are milliseconds. If
* AIA_MCL_100US, units are in microseconds. Sets the actual exposure time to value + 1.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param value Exposure time, range 0-65535
*
* @return 0 if successful, -1 if unsuccessful.
* @see pdv_set_exposure, \b MODE_CNTL_NORM & \b method_camera_shutter_timing directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
* @ingroup settings 
*/
int
pdv_set_exposure_mcl(PdvDev * pdv_p, int value)
{
    u_int   data_path;

    if (value < 0)
        value = 0;
    if (value > 25500)
        value = 25500;

    pdv_p->dd_p->shutter_speed = value;

    data_path = pdv_p->dd_p->datapath_reg;
    data_path &= ~PDV_MULTIPLIER_MASK;


    /*
    * special case microsecond shutter timer
    */
    if (pdv_p->dd_p->camera_shutter_timing == AIA_MCL_100US)
    {
        edt_msg(DBG2, "pdv_set_exposure_mcl(%d) (100US)\n", value);

        if (value < 256)
        {
            data_path |= PDV_MULTIPLIER_100US;
        }
        else if (value < 2560)
        {
            /* already turned off multiplier */
            value = (value + 5) / 10;
        }
        else if (value < 25600)
        {
            data_path |= PDV_MULTIPLIER_10MS;
            value = (value + 50) / 100;
        }
        else if (value < 256000)
        {
            data_path |= PDV_MULTIPLIER_100MS;
            value = (value + 50) / 1000;
        }
    }
    else
    {
        edt_msg(DBG2, "pdv_set_exposure_mcl(%d)\n", value);

        if (value < 256)
        {
            /* already turned off multiplier */
        }
        else if (value < 2560)
        {
            data_path |= PDV_MULTIPLIER_10MS;
            value = (value + 5) / 10;
        }
        else
        {
            data_path |= PDV_MULTIPLIER_100MS;
            value = (value + 50) / 100;
        }
    }

    pdv_p->dd_p->datapath_reg = data_path;
    edt_reg_write(pdv_p, PDV_SHUTTER, value);
    edt_reg_write(pdv_p, PDV_DATA_PATH, data_path);

    return 0;
}

/**
* set exposure, SMD specific
*/
static int
pdv_set_exposure_smd(PdvDev * pdv_p, int value)
{
    int     fp = 0;
    u_char  buf[128];
    int     n;
    int     ret = 0;
    int     smd_reg1, smd_reg3;


    if (Smd_type == NOT_SET)
    {
        Smd_type = smd_read_reg(pdv_p, SMD_READ_CAMTYPE);
        if ((Smd_type & 0xfff) == 0xfff)	/* sometimes there's garbage
                                            * at first; if so try again */
                                            Smd_type = smd_read_reg(pdv_p, SMD_READ_CAMTYPE);
    }

    /* send SMD write integration reg. command and arg */
    switch (Smd_type)
    {
    case SMD_TYPE_4M4:
        /*
        * set time to send, then write that based on msecs (value is int so
        * can't do 0.5msec, just use 0 for that -- $#^&*@ screwy way to do
        * it if you ask me.....) buf[0] = (u_char) SMD_4M4_WRITE_R2;
        */
        if (value == 0)		/* 500us */
            buf[1] = 0x7d;
        else if (value == 1)	/* 1ms */
            buf[1] = 0x7b;
        else if (value == 2)	/* 2ms */
            buf[1] = 0x77;
        else if (value <= 4)	/* 4ms */
            buf[1] = 0x6f;
        else if (value <= 8)	/* 8ms */
            buf[1] = 0x5f;
        else if (value <= 16)	/* 16ms */
            buf[1] = 0x3f;
        else			/* 32ms (default) */
            buf[1] = 0x00;
        /*
        * send write r2 (integration reg) command
        */
        pdv_serial_binary_command(pdv_p, (char *) buf, 2);
        break;

    case SMD_TYPE_BT25:
        /*
        * adjust timeout multiplier according to frame rate
        */
        buf[0] = (u_char) SMD_BT25_WRITE_R2;
        if (Smd_rate == NOT_SET)
        {
            if ((Smd_rate = smd_read_reg(pdv_p, SMD_BT25_READ_FRAMERATE)) == -1)
                edt_msg(PDVWARN, "libpdv: no response from SMD camera rate reg read\n");
            if (pdv_p->dd_p->timeout_multiplier < Smd_rate)
            {
                pdv_p->dd_p->timeout_multiplier = Smd_rate;
                pdv_auto_set_timeout(pdv_p);
            }
        }
        buf[1] = (u_char) value;
        /*
        * send write r2 (integration reg) command
        */
        pdv_serial_binary_command(pdv_p, (char *) buf, 2);

        /* SMD doesn't resp but do a dummy wait to make sure its done */
        pdv_serial_wait(pdv_p, 100, 64);
        break;

    case SMD_TYPE_1M30P:
    case SMD_TYPE_6M3P:
        buf[0] = (u_char) SMD_1M30P_REG_W_INTEG0;
        buf[1] = value & 0xff;
        pdv_serial_binary_command(pdv_p, (char *) buf, 2);

        buf[0] = (u_char) SMD_1M30P_REG_W_INTEG1;
        buf[1] = (value & 0xff00) >> 8;
        pdv_serial_binary_command(pdv_p, (char *) buf, 2);

        buf[0] = (u_char) SMD_1M30P_REG_W_INTEG2;
        buf[1] = (value & 0xff0000) >> 16;
        pdv_serial_binary_command(pdv_p, (char *) buf, 2);

        /*
        * SMD 1M30p  shutter speeds are in uSecs, so fake out
        * pdv_auto_set_timeout, then switch speed back to actual value
        */
        /*
        * ALERT: should take frame rate (& binning?) into account here too
        */
        pdv_p->dd_p->shutter_speed = value / 1000;
        pdv_auto_set_timeout(pdv_p);
        pdv_p->dd_p->shutter_speed = value;
        break;

    case SMD_TYPE_1M15P:
        if ((smd_reg1 = smd_read_reg(pdv_p, SMD_1M15P_READ_R1)) == -1)
        {
            edt_msg(PDVWARN, "libpdv: no response from SMD R1 reg read\n");
            return -1;
        }


        buf[0] = (u_char) SMD_1M15P_WRITE_R1;

        /*
        * valid values for this camera are 0, 1, 2, 4 and 8, but allow values
        * in between and just round down 
        */
        if (value == 0)
        {
            buf[1] = (smd_reg1 &~ SMD_1M15P_R1_INTMSK) | 0x0; /* NONE */
            fp = 65000;
        }
        else if (value == 1)
        {
            buf[1] = (smd_reg1 &~ SMD_1M15P_R1_INTMSK) | 0x7; /* 1 ms */
            fp = 65000;
        }
        else if (value < 4)
        {
            buf[1] = (smd_reg1 &~ SMD_1M15P_R1_INTMSK) | 0x6; /* 2 ms */
            fp = 64000;
        }
        else if (value < 7)
        {
            buf[1] = (smd_reg1 &~ SMD_1M15P_R1_INTMSK) | 0x5; /* 4 ms */
            fp = 62000;
        }
        else if (value == 8)
        {
            buf[1] = (smd_reg1 &~ SMD_1M15P_R1_INTMSK) | 0x4; /* 8 ms */
            fp = 58000;
        }
        else return -1;

        if (smd_reg1 & SMD_1M15P_R1_TRIGMODE)
        {
            /* if binned and triggered, use half the frame delay */ 
            smd_reg3 = smd_read_reg(pdv_p, SMD_1M15P_READ_R3);
            if (smd_reg3 & 0x40) /* 2x2 bin mode bit */
                fp /= 2;
            pdv_set_frame_period(pdv_p, fp, PDV_FVAL_ADJUST);
        }

        pdv_p->dd_p->shutter_speed = value;
        pdv_serial_binary_command(pdv_p, (char *) buf, 2);
        break;

    default:
        ret = -1;
        edt_msg(PDVWARN, "libpdv: unknown SMD camera type %02x\n", Smd_type);
        break;
    }

    if (!ret)
    {
        /* SMD doesn't respond but do a dummy wait to make sure its done */
        n = pdv_serial_wait(pdv_p, 100, 64);
        if (n > 127)
            n = 127;
        if (n)
            pdv_serial_read(pdv_p, (char *) buf, n);
    }
    return ret;
}

/**
* set exposure, TI MC-1001PF specific.
* 
* Shutter SW bits correspond to Mode Control bits 1, 2 and 3:
* 
* 0x00 0.063msec 0x01 0.127msec 0x02 0.254msec 0x04 0.508msec 0x08 1.016msec
* 0x16 2.032msec 0x32 4.064msec 0x64 8.128msec
*/
static int
pdv_set_exposure_timc1001pf(PdvDev * pdv_p, int value)
{
    Dependent *dd_p = pdv_p->dd_p;
    u_int   mcl = edt_reg_read(pdv_p, PDV_MODE_CNTL) & 0xf1;

    if ((value >= 0) && (value <= 7))
    {
        dd_p->shutter_speed = value;
        mcl |= (value << 1);
        edt_reg_write(pdv_p, PDV_MODE_CNTL, mcl);
        return 0;
    }
    return -1;
}

/**
* set exposure, Toshiba CS3960DCL (and similar) specific
*/
static int
pdv_set_exposure_toshiba(PdvDev * pdv_p, int value)
{
    char    cmdbuf[128];
    int n;

    if ((value < 0) || (value > 0xff))
        return -1;

    cmdbuf[0] = 0x02;
    sprintf(cmdbuf+1, "68%02X", value);
    cmdbuf[5] = 0x03;
    pdv_serial_binary_command(pdv_p, (char *)cmdbuf, 6);

    /* response should be <STX><ACK><ETX> */
    if (n = pdv_serial_wait(pdv_p, 200, 3))
        pdv_serial_read(pdv_p, cmdbuf, n);

    cmdbuf[0] = 0x02;
    sprintf(cmdbuf+1, "6E01");
    cmdbuf[5] = 0x03;
    pdv_serial_binary_command(pdv_p, (char *)cmdbuf, 6);

    /* response should be <STX><ACK><ETX> */
    if (n = pdv_serial_wait(pdv_p, 200, 3))
        pdv_serial_read(pdv_p, cmdbuf, n);

    return 0;
}

/**
* set gain, Toshiba CS3960DCL (and similar) specific
*/
static int
pdv_set_gain_toshiba(PdvDev * pdv_p, int value)
{
    char    cmdbuf[128];
    int n;

    if ((value < 0) || (value > 0xff))
        return -1;

    cmdbuf[0] = 0x02;
    sprintf(cmdbuf+1, "64%02X", value);
    cmdbuf[5] = 0x03;
    pdv_serial_binary_command(pdv_p, (char *)cmdbuf, 6);

    /* response should be <STX><ACK><ETX> */
    if (n = pdv_serial_wait(pdv_p, 200, 3))
        pdv_serial_read(pdv_p, cmdbuf, n);

    cmdbuf[0] = 0x02;
    sprintf(cmdbuf+1, "6E01");
    cmdbuf[5] = 0x03;
    pdv_serial_binary_command(pdv_p, (char *)cmdbuf, 6);

    /* response should be <STX><ACK><ETX> */
    if (n = pdv_serial_wait(pdv_p, 200, 3))
        pdv_serial_read(pdv_p, cmdbuf, n);

    return 0;
}

/**
* set exposure, Cohu 7700 (and similar) specific:
*/
static int
pdv_set_exposure_cohu(PdvDev * pdv_p, int value)
{
    char    cmdbuf[128];
    int n, len;

    if ((value < 0) || (value > 10))
        return -1;

    /* send STX ID 'c' 'E' value 81 ETX (value is ASCII int, 81 is dummy checksum) */
    cmdbuf[0] = 0x02; /* STX */
    cmdbuf[1] = 0x01; /* Camera ID 1 */
    sprintf(cmdbuf+2, "cE2,%d", value);
    len = (int)strlen(cmdbuf);
    cmdbuf[len++] = (char)0x81;
    cmdbuf[len++] = (char)0x03;
    cmdbuf[len] = 0;
    pdv_serial_binary_command(pdv_p, (char *)cmdbuf, len);

    /* ALERT: check for specific response [ STX ID 'c' 'C' '0' CS ETX ] here? */
    if (n = pdv_serial_wait(pdv_p, 200, 7))
        pdv_serial_read(pdv_p, cmdbuf, n);

    /* send STX 'c' 'T' '0' 81 ETX (required to "make it so") */
    cmdbuf[0] = (char)0x02; /* STX */
    cmdbuf[1] = (char)0x01; /* Camera ID 1 */
    cmdbuf[2] = 'c';
    cmdbuf[3] = 'T';
    cmdbuf[4] = '0';
    cmdbuf[5] = (char)0x81;
    cmdbuf[6] = (char)0x03;
    pdv_serial_binary_command(pdv_p, (char *)cmdbuf, 7);

    /* ALERT: check for specific response [ STX ID 'c' 'C' '0' CS ETX ] here? */
    if (n = pdv_serial_wait(pdv_p, 200, 7))
        pdv_serial_read(pdv_p, cmdbuf, n);

    return 0;
}

/**
* set gain, Cohu 7700 (and similar) specific
*/
static int
pdv_set_gain_cohu(PdvDev * pdv_p, int value)
{
    char    cmdbuf[128];
    int n, len;

    if ((value < 0) || (value > 320))
        return -1;

    /* send STX ID 'c' 'E' value 81 ETX (value is ASCII int, 81 is dummy checksum) */
    cmdbuf[0] = (char)0x02; /* STX */
    cmdbuf[1] = (char)0x01; /* Camera ID 1 */
    sprintf(cmdbuf+2, "cB1,%d", value);
    len = (int)strlen(cmdbuf);
    cmdbuf[len++] = (char)0x81;
    cmdbuf[len++] = (char)0x03;
    cmdbuf[len] = 0;
    pdv_serial_binary_command(pdv_p, (char *)cmdbuf, len);

    /* ALERT: check for specific response [ STX ID 'c' 'C' '0' CS ETX ] here? */
    if (n = pdv_serial_wait(pdv_p, 200, 7))
        pdv_serial_read(pdv_p, cmdbuf, n);

    /* send STX 'c' 'T' '0' 81 ETX (required to "make it so") */
    cmdbuf[0] = (char)0x02; /* STX */
    cmdbuf[1] = (char)0x01; /* Camera ID 1 */
    cmdbuf[2] = 'c';
    cmdbuf[3] = 'T';
    cmdbuf[4] = '0';
    cmdbuf[5] = (char)0x81;
    cmdbuf[6] = (char)0x03;
    pdv_serial_binary_command(pdv_p, (char *)cmdbuf, 7);

    /* ALERT: check for specific response [ STX ID 'c' 'C' '0' CS ETX ] here? */
    if (n = pdv_serial_wait(pdv_p, 200, 7))
        pdv_serial_read(pdv_p, cmdbuf, n);

    return 0;
}

/**
* set level, Cohu 7700 (and similar) specific
*/
static int
pdv_set_blacklevel_cohu(PdvDev * pdv_p, int value)
{
    char    cmdbuf[128];
    int n, len;

    if ((value < 0) || (value > 1023))
        return -1;

    /* send STX ID 'c' 'E' value 81 ETX (value is ASCII int, 81 is dummy checksum) */
    cmdbuf[0] = 0x02; /* STX */
    cmdbuf[1] = 0x01; /* Camera ID 1 */
    sprintf(cmdbuf+2, "cB0,%d", value);
    len = (int)strlen(cmdbuf);
    cmdbuf[len++] = (char)0x81;
    cmdbuf[len++] = (char)0x03;
    cmdbuf[len] = 0;
    pdv_serial_binary_command(pdv_p, (char *)cmdbuf, len);

    /* ALERT: check for specific response [ STX ID 'c' 'C' '0' CS ETX ] here? */
    if (n = pdv_serial_wait(pdv_p, 200, 7))
        pdv_serial_read(pdv_p, cmdbuf, n);

    /* send STX 'c' 'T' '0' 81 ETX (required to "make it so") */
    cmdbuf[0] = (char)0x02; /* STX */
    cmdbuf[1] = (char)0x01; /* Camera ID 1 */
    cmdbuf[2] = 'c';
    cmdbuf[3] = 'T';
    cmdbuf[4] = '0';
    cmdbuf[5] = (char)0x81;
    cmdbuf[6] = (char)0x03;
    pdv_serial_binary_command(pdv_p, (char *)cmdbuf, 7);

    /* ALERT: check for specific response [ STX ID 'c' 'C' '0' CS ETX ] here? */
    if (n = pdv_serial_wait(pdv_p, 200, 7))
        pdv_serial_read(pdv_p, cmdbuf, n);

    return 0;
}

/**
* set exposure, Pulnix TM6710/1020 specific
*/
static int
pdv_set_exposure_ptm6710_1020(PdvDev * pdv_p, int value)
{
    char    buf[128];

    if ((value < 0) || (value > 9))
        return -1;

    sprintf(buf, ":SM%d", value);
    pdv_serial_command_flagged(pdv_p, buf, SCFLAG_NORESP);

    return 0;
}

/**
* set gain, Pulnix TM6710/1020 specific
*/
static int
pdv_set_gain_ptm6710_1020(PdvDev * pdv_p, int value)
{
    char    buf[128];

    if ((value < 0) || (value > 0xff))
        return -1;

    sprintf(buf, ":GM%02X", value);
    pdv_serial_command_flagged(pdv_p, buf, SCFLAG_NORESP);

    return 0;
}

/**
* set gain, Pulnix Hamamatsu C8484 specific -- L(ow) or H(igh) -- take 0 or
* 1 and translate
*/
static int
pdv_set_gain_hc8484(PdvDev * pdv_p, int value)
{
    char    buf[128];

    if ((value != 0) && (value != 1))
        return -1;
    sprintf(buf, "CEG %c", value ? 'H' : 'L');
    pdv_serial_command_flagged(pdv_p, buf, SCFLAG_NORESP);

    return 0;
}

/**
* set exposure (,gain, blacklevel) on adimec 1000m and similar cameras ref.
* Adimec-1000M Operation and Technical Manual p/n 149200
*/
static int
pdv_set_exposure_adimec(PdvDev * pdv_p, int value)
{
    char    cmdbuf[32];

    sprintf(cmdbuf, "@IT%d", value);
    pdv_serial_command(pdv_p, cmdbuf);
    return 0;
}

static int
pdv_set_gain_adimec(PdvDev * pdv_p, int value)
{
    char    cmdbuf[32];

    sprintf(cmdbuf, "@GA%d", value);
    pdv_serial_command(pdv_p, cmdbuf);
    return 0;
}

static int
pdv_set_blacklevel_adimec(PdvDev * pdv_p, int value)
{
    char    cmdbuf[32];

    sprintf(cmdbuf, "@BL%d;%d", value, value);
    pdv_serial_command(pdv_p, cmdbuf);
    return 0;
}


/**
* set exposure for spectral instruments camera see spectral instruments
* specification p/n 1870A for details on serial command set
*/
static int
pdv_set_exposure_specinst(PdvDev * pdv_p, int value)
{
    edt_msg(DBG2, "pdv_set_exposure_specinst(%d)\n", value);

    if (pdv_specinst_setparam(pdv_p, 'G', 8, value) != 0)
    {
        edt_msg(DBG2, "pdv_set_exposure_specinst() apparently FAILED\n");
        return -1;
    }
    return 0;
}

/**
* set exposure for spectral instruments camera see spectral instruments
* specification p/n 1870A for details on serial command set
*/
static int
pdv_set_gain_specinst(PdvDev * pdv_p, int value)
{
    edt_msg(DBG2, "pdv_set_gain_specinst(%d)\n", value);

    if (pdv_specinst_setparam(pdv_p, 'G', 11, value) != 0)
    {
        edt_msg(DBG2, "pdv_set_gain_specinst() apparently FAILED\n");
        return -1;
    }
    return 0;
}

/**
* Set exposure (,gain, blacklevel) on Sensors Unlimited SU320M-1.7RT
* and similar cameras. Ref. SU320M-1.7RT Indium Gallium Arsenide MiniCamera
* Operation Manual, P/N 4200-0029.
*/
static int
pdv_set_exposure_su320(PdvDev * pdv_p, int value)
{
    char    cmdbuf[32];
    char resp[1024];
    int n;

    sprintf(cmdbuf, "INT%d", value);
    pdv_serial_command(pdv_p, cmdbuf);
    pdv_serial_wait(pdv_p, 100, 20);
    if ((n = pdv_serial_read(pdv_p, resp, 20)) != 20)
        return -1;
    return 0;
}


static int
pdv_specinst_setparam(PdvDev * pdv_p, char cmd, u_long offset, u_long value)
{
    int     ret1, ret2, ret3;
    char    resp1[32];
    char    resp2[32];
    char    buf[32];
    u_char  cmdbuf[5];
    u_char  offsetbuf[5];
    u_char  parambuf[5];
    int     si_wait = 200;

    edt_msg(DBG2, "pdv_specinst_setparam(%c %04x %04x)\n", cmd, offset, value);

    dvu_long_to_charbuf(offset, offsetbuf);
    dvu_long_to_charbuf(value, parambuf);

    cmdbuf[0] = cmd;

    pdv_serial_binary_command(pdv_p, (char *) cmdbuf, 1);
    pdv_serial_wait_next(pdv_p, si_wait, 31);
    ret1 = pdv_serial_read(pdv_p, resp1, 31);

    pdv_serial_binary_command(pdv_p, (char *) offsetbuf, 4);
    pdv_serial_wait_next(pdv_p, si_wait, 31);
    ret2 = pdv_serial_read(pdv_p, buf, 31);

    pdv_serial_binary_command(pdv_p, (char *) parambuf, 4);
    pdv_serial_wait_next(pdv_p, si_wait, 31);
    ret3 = pdv_serial_read(pdv_p, resp2, 31);

    if ((ret1 != 1) || (ret3 != 1) || (resp1[0] != 'G') || (resp2[0] != 'Y'))
    {
        edt_msg(DBG1, "invalid or missing serial response from specinst\n");
        return -1;
    }
    return 0;
}

/**
* Send a basler binary command -- do the framing and BCC.
* ref. BASLER A202K Camera Manual Doc. ID number DA044003.
*
* @param pdv_p   pointer to pdv device structure returned by #pdv_open
* @param cmd     basler command
* @param rwflag  read/write flag -- 1 if read, 0 if write
* @param len     data length
* @param data    the data (if any)
*
* @return 0 on success, -1 on failure
* @ingroup serial 
*/
int
pdv_send_basler_command(PdvDev * pdv_p, int cmd, int rwflag, int len, int data)
{
    int       i;
    u_char    frame[32];
    u_char    rwbit = (rwflag & 0x1) << 7;

    frame[0] = cmd;			      
    frame[1] = ((u_char)len & 0xef) | rwbit;
    for (i=0; i<len; i++)
        frame[i+2] = (data >> (8 * i)) & 0xff;

    return pdv_send_basler_frame(pdv_p, frame, len+2);
}

/**
* set exposure (,gain, blacklevel) on basler A202K -- ref
* BASLER A202K Camera Manual Document ID number DA044003.
* @ingroup settings 
*/
int
pdv_set_exposure_basler202k(PdvDev * pdv_p, int value)
{
    u_char    rframe[8];

    memset(rframe, 0, 8);
    pdv_send_basler_command(pdv_p, 0xa6, 0, 3, value);
    pdv_read_basler_frame(pdv_p, rframe, 1);
    if (rframe[0] != 0x6) /* ACK */
        return -1;
    return 0;
}

int
pdv_set_gain_basler202k(PdvDev * pdv_p, int valuea, int valueb)
{
    u_char    rframe[8];

    memset(rframe, 0, 8);
    pdv_send_basler_command(pdv_p, 0x80, 0, 2, valuea);
    pdv_read_basler_frame(pdv_p, rframe, 1);
    if (rframe[0] != 0x6) /* ACK */
        return -1;

    memset(rframe, 0, 8);
    pdv_send_basler_command(pdv_p, 0x82, 0, 2, valueb);
    pdv_read_basler_frame(pdv_p, rframe, 1);
    if (rframe[0] != 0x6) /* ACK */
        return -1;

    return 0;
}

int
pdv_set_offset_basler202k(PdvDev * pdv_p, int valuea, int valueb)
{
    u_char    rframe[8];

    memset(rframe, 0, 8);
    pdv_send_basler_command(pdv_p, 0x81, 0, 2, valuea);
    pdv_read_basler_frame(pdv_p, rframe, 1);
    if (rframe[0] != 0x6) /* ACK */
        return -1;

    memset(rframe, 0, 8);
    pdv_send_basler_command(pdv_p, 0x83, 0, 2, valueb);
    pdv_read_basler_frame(pdv_p, rframe, 1);
    if (rframe[0] != 0x6) /* ACK */
        return -1;
    return 0;
}

/**
* Set exposure for Redlake (formerly Duncantech) DT and MS series cameras.
* ref. DuncanTech User Manual Doc # 9000-0001-05
*
* @note Convenience routine, for Duncantech (Redlake) DT/MS series
* cameras only. Intended as a starting point for programmers wishing to
* use EDT serial commands with Duncantech cameras.  These subroutines can
* be used as a template for controlling camera parameters beyond simple
* exposure and gain.
*
* @ingroup settings 
* @param pdv_p   pointer to pdv device structure returned by #pdv_open
* @param value     expsosure value
* @param ch  camera channel
* @see pdv_send_duncan_frame, pdv_read_duncan_frame
*/
int
pdv_set_exposure_duncan_ch(PdvDev * pdv_p, int value, int ch)
{
    u_char    msg[8];
    u_char    rmsg[16];

    msg[0] = 0x04;				/* LSB size */
    msg[1] = 0x00;				/* MSB size */
    msg[2] = 0x14;				/* command byte */
    msg[3] = (u_char)(ch & 0xff);		/* channel*/
    msg[4] = (u_char)(value & 0xff);		/* value lsb */
    msg[5] = (u_char)((value >> 8) & 0xff);	/* value msb */
    pdv_send_duncan_frame(pdv_p, msg, 6);
    pdv_read_duncan_frame(pdv_p, rmsg);
    return 0;
}

/**
* Set gain for Redlake (formerly Duncantech) DT and MS series cameras.
* ref. DuncanTech User Manual Doc # 9000-0001-05
*
* @note Convenience routine, for Duncantech (Redlake) DT/MS series
* cameras only. Intended as a starting point for programmers wishing to
* use EDT serial commands with Duncantech cameras.  These subroutines can
* be used as a template for controlling camera parameters beyond simple
* exposure and gain.
*
* @ingroup settings 
* @param pdv_p   pointer to pdv device structure returned by #pdv_open
* @param value     gain value
* @param ch  camera channel
* @see pdv_send_duncan_frame, pdv_read_duncan_frame
*/
int
pdv_set_gain_duncan_ch(PdvDev * pdv_p, int value, int ch)
{
    u_char    msg[8];
    u_char    rmsg[16];

    msg[0] = 0x04;				/* LSB size */
    msg[1] = 0x00;				/* MSB size */
    msg[2] = 0x02;				/* command byte */
    msg[3] = (u_char)(ch & 0xff);		/* channel*/
    msg[4] = (u_char)(value & 0xff);		/* value lsb */
    msg[5] = (u_char)((value >> 8) & 0xff);	/* value msb */
    pdv_send_duncan_frame(pdv_p, msg, 6);
    pdv_read_duncan_frame(pdv_p, rmsg);
    return 0;
}


/**
* parse serial hex string -- assume max of 16 all instances of "??" will be
* replaced with value, all others assumed to be valid hex digits
*/
static void
send_serial_binary_cmd(PdvDev * pdv_p, char *hexstr, int value)
{
    int     nb;
    u_char  hbuf[2];
    char    resp[128];
    char    bs[16][3];
    int     ret;

    edt_msg(DBG2, "send_serial_binary_cmd(\"%s\", %d)\n", hexstr, value);

    nb = sscanf(hexstr, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
        bs[0], bs[1], bs[2], bs[3], bs[4], bs[5], bs[6], bs[7],
        bs[8], bs[9], bs[10], bs[11], bs[12], bs[13], bs[14], bs[15]);

    pdv_serial_binary_command(pdv_p, (char *) hbuf, nb);
    pdv_serial_wait(pdv_p, 100, 3);

    if (*pdv_p->dd_p->serial_response)
        ret = pdv_serial_read(pdv_p, resp, 50);

    {

        edt_msg(DBG2, "serial response <%s> (%d)\n", hex_to_str(resp, ret), ret);
    }
}

/**
* @ingroup init 
*
* Sets a reasonable image timeout value based on image size and exposure
* time (if set) and pixel clock speed (also if set).
*
* \note This subroutine is called by #pdv_initcam so it generally isn't
* necessary to call it from a user application. Nevertheless it can
* be useful to know how initcam sets the default timeout value (and how
* to override it); hence this description.
* \note
* #pdv_initcam calls this subroutine after reading in the various camera
* parameters from the config file. Since most configs don't (presently)
* have a \b pclock_speed directive specified, it assumes a conservative 5
* Mhz pixel clock speed, which can make for a long timeout value. As a
* result, for faster cameras in general, and large format ones specifically,
* if data loss occurs for whatever reason, the \e pdv_wait acquisition routines
* may block for an excessively long time if data loss occurs. To get around
* this, either add a \b pclock_speed directive to the config file (preferred),
* or set your own fixed timeout override with the \b user_timeout directive
* or #pdv_set_timeout.
* 
* @see pdv_initcam,  pdv_set_timeout, pdv_set_exposure, \b pclock_speed & \b user_timeout directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
*
* @return 0 on success, -1 on failure. 
*/
int
pdv_auto_set_timeout(PdvDev * pdv_p)
{
    Dependent *dd_p = pdv_p->dd_p;
    int     user_timeout = dd_p->user_timeout;
    int     user_set = dd_p->user_timeout_set;
    int     tmult = dd_p->timeout_multiplier;
    int     cur_timeout = edt_get_rtimeout(pdv_p);
    int     timeout;
    int     exposure;
    int     pdiv = 1;
    int     ret = 0;
    int     xfersize;

    /*
    * tmult is for cameras with non-standard integration times
    */
    if (tmult < 1)
        tmult = 1;

    /* pixel clock speed less than 5? Hope not! */
    if (dd_p->pclock_speed < 5)
        dd_p->pclock_speed = 5;
    pdiv = dd_p->pclock_speed / 5;

    if (((exposure = dd_p->shutter_speed)) < 500)
        exposure = 500; /* conservative enough? hope so */

    xfersize = dd_p->cam_width * dd_p->cam_height * bits2bytes(dd_p->depth);
    timeout = (((xfersize * tmult) / 4000) / pdiv) + exposure;

    edt_msg(DBG2, "pdv_auto_set_timeout(): current %d new %d exposure %d pclock %d pdiv %d mult %d user %d\n",
        cur_timeout, timeout, exposure, dd_p->pclock_speed, pdiv,
        tmult, user_timeout);

    /*
    * sticking an 0.5 sec min in here because sometimes small cameras are
    * problematic
    */
    if (timeout < 500)
        timeout = 500;

    if (user_set)
    {
        edt_msg(DBG2, "  user set to %d - overriding auto\n", user_timeout);

        if (timeout > user_timeout && user_timeout != 0)
        {
            edt_msg(DBG2, "  Warning: exposure %d msecs user specified timeout %d msecs\n", dd_p->shutter_speed, user_timeout);
            edt_msg(DBG2, "  not automatically increased since user specified\n");
        }
    }
    else
    {
        int     targ;

        edt_msg(DBG2, "  setting picture timeout from %d to %d\n", cur_timeout, timeout);
        targ = timeout;
        ret = edt_set_rtimeout(pdv_p, targ);
    }
    return ret;
}

/**
* Sets the gain on the input device.
* Applies only to cameras for which extended control capabilities have
* been added to the library (see the source code), or that have a serial
* command protocol that has been configured using the \b serial_gain
* configuration directive. Unless you know that one of the above has
* been implemented for your camera, it is usually safest to just send
* the specific serial commands via #pdv_serial_command or #pdv_serial_write.
*
* @Example
* @code
* pdv_set_gain(pdv_p, 0); // neutral gain 
* @endcode
*
* @return 0 on success, -1 on failure. 
*
* @ingroup settings 
* @see pdv_get_gain, \b serial_gain configuration file directive.
*/
int
pdv_set_gain(PdvDev * pdv_p, int value)
{
    int     ret;
    Dependent *dd_p = pdv_p->dd_p;

    edt_msg(DBG2, "pdv_set_gain(%d)\n", value);

    dd_p->gain = value;
    ret = edt_set_dependent(pdv_p, dd_p);

    if ((strlen(dd_p->serial_gain) > 0) && (dd_p->serial_format == SERIAL_BINARY))
        send_serial_binary_cmd(pdv_p, dd_p->serial_gain, value);

    else if (dd_p->serial_format == SERIAL_ASCII)
    {
        char cmdstr[64] = {'\0'};
        int n = check_valid_formatstr(dd_p->serial_gain); 

        if (n == 0) /* no % formatting, use old method */
        {
            if (dd_p->serial_gain[0])
                sprintf(cmdstr, "%s %d", dd_p->serial_gain, value);
        }
        else if ((n > 0) && (n < 5))
            sprintf(cmdstr, dd_p->serial_gain, value, value, value, value);

        if (cmdstr[0])
        {
            ret = pdv_serial_command_flagged(pdv_p, cmdstr, SCFLAG_NORESP);

            pdv_serial_wait(pdv_p, pdv_p->dd_p->serial_timeout, 64);
            if (*pdv_p->dd_p->serial_response)
                pdv_serial_read(pdv_p, cmdstr, 63);
        }
    }

    else if ((strncasecmp(dd_p->camera_class, "Hamamatsu", 9) == 0)
        && ((strncasecmp(dd_p->camera_model, "C8484", 5) == 0)
        || (strncasecmp(dd_p->camera_model, "8484", 4) == 0)))
    {
        ret = pdv_set_gain_hc8484(pdv_p, value);
    }

    else if ((dd_p->camera_shutter_timing == SPECINST_SERIAL)
        || (dd_p->camera_shutter_speed == SPECINST_SERIAL))
    {
        ret = pdv_set_gain_specinst(pdv_p, value);
    }
    else if (dd_p->camera_shutter_timing == TOSHIBA_SERIAL)
    {
        ret = pdv_set_gain_toshiba(pdv_p, value);
    }
    else if (dd_p->camera_shutter_timing == COHU_SERIAL)
    {
        ret = pdv_set_gain_cohu(pdv_p, value);
    }
    else if (dd_p->set_gain == SMD_SERIAL)	/* smd binary serial method */
    {
        ret = pdv_set_gain_smd(pdv_p, value);
    }
    else if ((strncasecmp(dd_p->camera_class, "Adimec", 6) == 0))
    {
        ret = pdv_set_gain_adimec(pdv_p, value);
    }

    else if ((strncasecmp(dd_p->camera_class, "PULNiX", 6) == 0)
        && ((strncasecmp(dd_p->camera_model, "TM-6710", 7) == 0)
        || (strncasecmp(dd_p->camera_model, "TM-1020", 7) == 0)))
    {
        ret = pdv_set_gain_ptm6710_1020(pdv_p, value);
    }

    else if ((dd_p->set_gain == AIA_MC4)	/* mode cntl 4 method */
        && (dd_p->xilinx_rev >= 1 && dd_p->xilinx_rev <= 32))
    {
        u_int   util2 = edt_reg_read(pdv_p, PDV_UTIL2);
        u_int   mcl = edt_reg_read(pdv_p, PDV_MODE_CNTL);

        edt_reg_write(pdv_p, PDV_UTIL2, util2 & ~PDV_MC4);
        edt_reg_write(pdv_p, PDV_MODE_CNTL, dd_p->gain);
        edt_reg_write(pdv_p, PDV_UTIL2, util2 | PDV_MC4);
        edt_reg_write(pdv_p, PDV_MODE_CNTL, mcl);
    }

    edt_msg(DBG2, "pdv_set_gain returning %d\n", ret);
    return (ret);
}

/** 
* @ingroup serial
*/
static int
pdv_set_gain_smd(PdvDev * pdv_p, int value)
{
    char    buf[128];
    char    smd_config;
    int     smd_reg1;
    int     ret = 0;

    if (Smd_type == NOT_SET)
    {
        Smd_type = smd_read_reg(pdv_p, SMD_READ_CAMTYPE);
        if ((Smd_type & 0xfff) == 0xfff)	/* sometimes there's garbage
                                            * at first; if so try again */
                                            Smd_type = smd_read_reg(pdv_p, SMD_READ_CAMTYPE);
    }

    switch (Smd_type)
    {
    case SMD_TYPE_4M4:
        buf[0] = (char) SMD_4M4_READ_R1;
        pdv_serial_binary_command(pdv_p, buf, 1);
        pdv_serial_wait(pdv_p, pdv_p->dd_p->serial_timeout,
            pdv_p->dd_p->serial_respcnt);
        if (pdv_serial_read(pdv_p, buf, 63) == 1)
        {
            smd_config = buf[0];

            buf[0] = (char) SMD_4M4_WRITE_R1;
            pdv_serial_binary_command(pdv_p, buf, 1);

            if (value == 0)
                smd_config &= ~SMD_4M4_R1_GAIN;
            else
                smd_config |= SMD_4M4_R1_GAIN;
            buf[0] = smd_config;
            pdv_serial_binary_command(pdv_p, buf, 1);
            /* SMD doesn't resp but do a dummy wait to make sure its done */
            pdv_serial_wait(pdv_p, 100, 64);
        }
        else
            ret = -1;
        break;

    case SMD_TYPE_1M30P:
        buf[0] = (u_char) SMD_1M30P_REG_W_LS_GAIN;
        buf[1] = value & 0xff;
        pdv_serial_binary_command(pdv_p, (char *) buf, 2);

        buf[0] = (u_char) SMD_1M30P_REG_W_MS_GAIN;
        buf[1] = (value & 0xff00) >> 8;
        pdv_serial_binary_command(pdv_p, (char *) buf, 2);

        /* SMD doesn't respond but do a dmy wait to make sure its done */
        pdv_serial_wait(pdv_p, 100, 64);
        break;

    case SMD_TYPE_1M15P:
    case SMD_TYPE_6M3P:
        if ((smd_reg1 = smd_read_reg(pdv_p, SMD_1M15P_READ_R1)) == -1)
        {
            edt_msg(PDVWARN, "libpdv: no response from SMD R1 reg read\n");
            return -1;
        }

        buf[0] = (u_char) SMD_1M15P_WRITE_R1; /* same for 6M3P */

        /*
        * valid values for this camera are 0 or 1
        */
        if (value == 0)
            buf[1] = smd_reg1 &~ SMD_1M15P_R1_GAIN;
        else if (value == 1)
            buf[1] = smd_reg1 | SMD_1M15P_R1_GAIN;
        else return -1;

        pdv_serial_binary_command(pdv_p, (char *) buf, 2);
        pdv_p->dd_p->gain = value;

        /* SMD doesn't respond but do a dmy wait to make sure its done */
        pdv_serial_wait(pdv_p, 100, 64);
        break;
    }

    return ret;
}

/**
* This method is obsolete and should not be used.
* The current implementation creates a warning message and returns
* -1 signifying failure.
*
* @return -1 for failure, always.
* @ingroup edt_undoc 
*/
int
pdv_set_gain_ch(PdvDev * pdv_p, int value, int chan)
{
    int     ret = -1;

    /* ALERT -- no methods implemented since ddcam went away... */
    edt_msg(DBG2, "pdv_set_gain_ch(%d, [%c]) ret %d\n",
        value, (chan == 1) ? 'A' : (chan == 2) ? 'B' : '?', ret);
    return (ret);
}

/**
* Sets the black level (offset) on the input device.
* Applies only to cameras for which extended control capabilities have
* been added to the library (see the source code), or that have a serial
* command protocol that has been configured using the \b serial_offset
* configuration directive. Unless you know that one of the above has
* been implemented for your camera, it is usually safest to just send
* the specific serial commands via #pdv_serial_command or #pdv_serial_write.
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param value Black level value. The valid range is camera-dependent.
*
* @return 0 on success, -1 on failure. 
*
* @see pdv_get_offset, \b serial_offset configuration file directive.
* @ingroup settings 
*/
int
pdv_set_blacklevel(PdvDev * pdv_p, int value)
{
    Dependent *dd_p = pdv_p->dd_p;
    int     ret;

    edt_msg(DBG2, "pdv_set_blacklevel(%d)\n", value);

    dd_p->level = value;
    ret = edt_set_dependent(pdv_p, dd_p);

    if (dd_p->set_offset == SMD_SERIAL)	/* smd binary serial method */
        ret = pdv_set_blacklevel_smd(pdv_p, value);

    else if (dd_p->camera_shutter_timing == COHU_SERIAL)
    {
        ret = pdv_set_blacklevel_cohu(pdv_p, value);
    }

    else if ((strncasecmp(dd_p->camera_class, "Adimec", 6) == 0))
    {
        ret = pdv_set_blacklevel_adimec(pdv_p, value);
    }

    else if ((strlen(dd_p->serial_offset) > 0) && (dd_p->serial_format == SERIAL_BINARY))
    {
        send_serial_binary_cmd(pdv_p, dd_p->serial_offset, value);
    }

    else if (dd_p->serial_format == SERIAL_ASCII)
    {
        char cmdstr[64] = {'\0'};
        int n = check_valid_formatstr(dd_p->serial_offset); 

        if (n == 0) /* no % formatting, use old method */
        {
            if (dd_p->serial_offset[0])
            sprintf(cmdstr, "%s %d", dd_p->serial_offset, value);
        }
        else if ((n > 0) && (n < 5))
            sprintf(cmdstr, dd_p->serial_offset, value, value, value, value);

        if (cmdstr[0])
        {
            ret = pdv_serial_command_flagged(pdv_p, cmdstr, SCFLAG_NORESP);

            pdv_serial_wait(pdv_p, pdv_p->dd_p->serial_timeout, 64);
            if (*pdv_p->dd_p->serial_response)
                pdv_serial_read(pdv_p, cmdstr, 63);
        }
    }

    edt_msg(DBG2, "pdv_set_blacklevel() %d\n", ret);
    return (ret);
}

static int
pdv_set_blacklevel_smd(PdvDev * pdv_p, int value)
{
    char    buf[128];
    int     ret = 0;
    int     tmpval = value & 0xfff;
    int     smd_reg2, smd_reg3;

    if (Smd_type == NOT_SET)
    {
        Smd_type = smd_read_reg(pdv_p, SMD_READ_CAMTYPE);
        if ((Smd_type & 0xfff) == 0xfff)	/* sometimes there's garbage
                                            * at first; if so try again */
                                            Smd_type = smd_read_reg(pdv_p, SMD_READ_CAMTYPE);
    }

    switch (Smd_type)
    {
    case SMD_TYPE_4M4:
    case SMD_TYPE_BT25:
        break;

    case SMD_TYPE_1M30P:
        buf[0] = (u_char) SMD_1M30P_REG_W_LS_OFFSET;
        buf[1] = value & 0xff;
        pdv_serial_binary_command(pdv_p, (char *) buf, 2);

        buf[0] = (u_char) SMD_1M30P_REG_W_MS_OFFSET;
        buf[1] = (value & 0xff00) >> 8;
        pdv_serial_binary_command(pdv_p, (char *) buf, 2);

        /* SMD doesn't respond but do a dmy wait to make sure its done */
        pdv_serial_wait(pdv_p, 100, 64);
        break;

    case SMD_TYPE_1M15P:
        if ((smd_reg3 = smd_read_reg(pdv_p, SMD_1M15P_READ_R3)) == -1)
        {
            edt_msg(PDVWARN, "libpdv: no response from Dalstar R3 reg read\n");
            ret = -1;
        }

        smd_reg2 =  tmpval >> 4;
        smd_reg3 = (smd_reg3 & ~0x0f) | (tmpval & 0x0f);

        /* write R3 */
        buf[0] = (u_char) SMD_1M15P_WRITE_R3;
        buf[1] = (u_char) smd_reg3;
        pdv_serial_binary_command(pdv_p, (char *) buf, 2);

        /* must write R2 last */
        buf[0] = (u_char) SMD_1M15P_WRITE_R2;
        buf[1] = (u_char) smd_reg2;
        pdv_serial_binary_command(pdv_p, (char *) buf, 2);

        /* SMD doesn't respond but do a dmy wait to make sure its done */
        pdv_serial_wait(pdv_p, 100, 64);
        break;

    default:
        ret = -1;
    }

    return ret;
}

/**
* This method is obsolete and should not be used.
* The current implementation creates a warning message and returns
* -1 signifying failure.
*
* @return -1 for failure, always.
* @ingroup obsolete 
*/
int
pdv_set_aperture(PdvDev * pdv_p, int value)
{
    edt_msg(PDVWARN, "pdv_set_aperture is OBSOLETE\n");
    return (-1);
}

/**
* Set binning on the camera to the specified values, and recalculate the
* values that will be returned by #pdv_get_width, #pdv_get_height, and
* #pdv_get_imagesize.
*
* Only applicable to cameras for which binning logic has been implemented in
* the library -- specifically DVC cameras that use the \e BIN \e xval \e yval,
* Atmel cameras that use \e B= val (where val= 0, 1 or 2), or in conjunction with
* the \b serial_binning camera configuration directive for any camera that
* uses an ASCII \e CMD \e VALUE pair to set binning.
*
* This subroutine was an attempt to provide a way to set binning in a
* generic way, handling a few specific cameras via special code and others
* using an assumed serial format. As it turned out, the "assumed" format is not
* all that standard, therefore this subroutine is of limited usefulness.

* If your camera is one that takes a single ASCII command / argument to set a
* binning mode, then this subroutine may still be handy since it can be a single-
* call method for setting the camera and the board in a given binning mode.
* 
* To use this method, simply set the \b serial_binning camera configuration
* directive to the command that sets binning. Then when called, this subroutine
* will send the command and reset the board's camera size. 
* 
* If your camera does not fit any of the above formats (or if you would rather
* not depend on this flakey logic), simply use #pdv_serial_command or
* #pdv_serial_binary_command to send the command to put the camera into binned
* mode, then call #pdv_setsize to reset the board to the new frame size.
*
* If the PDV library does not know how to set binning on the camera in use,
* a -1 will be returned and the width/height/imagesize will remain unchanged.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param xval x binning value. Usually 1, 2, 4 or 8. Default is 1.
* @param yval y binning value. Usually 1, 2, 4 or 8. Default is 1.
*
* @return 0 on success, -1 on failure. 
*
* @see \b serial_binning directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
* @ingroup settings 
*/
int
pdv_set_binning(PdvDev * pdv_p, int xval, int yval)
{
    int     ret = -1;
    int     newvskip, newhskip, newwidth, newheight;
    Dependent *dd_p = pdv_p->dd_p;

    if (((xval != 1) && (xval % 2)) || ((yval != 1) && (yval % 2)))
    {
        edt_msg(PDVWARN, "pdv_set_binning(%d, %d) -- invalid value\n", xval, yval);
        return -1;
    }

    edt_msg(DBG2, "pdv_set_binning(%d, %d)\n", xval, yval);

    if (strcmp(dd_p->serial_binning, "BIN") == 0)	/* DVC */
    {
        ret = pdv_set_binning_dvc(pdv_p, xval, yval);
    }
    else if (strcmp(dd_p->serial_binning, "B=") == 0)	/* ATMEL */
    {
        ret = pdv_set_binning_generic(pdv_p, xval - 1);
    }
    else if (*dd_p->serial_binning)	/* Generic */
    {
        /* wing it */
        ret = pdv_set_binning_generic(pdv_p, xval);
    }
    else
        return -1;

    if (ret)
        return ret;

    if (dd_p->binx < 1)
        dd_p->binx = 1;
    if (dd_p->biny < 1)
        dd_p->biny = 1;

    newwidth = (((pdv_get_width(pdv_p) * dd_p->binx) / xval) / 4) * 4;
    newheight = (((pdv_get_height(pdv_p) * dd_p->biny) / yval)) ;

    if (dd_p->roi_enabled)
    {
        newhskip = (dd_p->hskip * dd_p->binx) / xval;
        newvskip = (dd_p->vskip * dd_p->biny) / yval;
        ret = pdv_set_roi(pdv_p, newhskip, newwidth, newvskip, newheight);
    }

    if (!ret)
    {
        dd_p->binx = xval;
        dd_p->biny = yval;
    }

    return (ret);
}

/**
* generic binning, assumes a command and integer argument.
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @ingroup settings 
*/
static int
pdv_set_binning_generic(PdvDev * pdv_p, int value)
{
    char    cmdstr[64];
    Dependent *dd_p = pdv_p->dd_p;

    if (dd_p->serial_binning[strlen(dd_p->serial_binning) - 1] == '=')	/* atmel */
        sprintf(cmdstr, "%s%d", dd_p->serial_binning, value);
    else
        sprintf(cmdstr, "%s %d", dd_p->serial_binning, value);
    pdv_serial_command_flagged(pdv_p, cmdstr, SCFLAG_NORESP);

    return (0);
}


/**
* Gets the exposure time on the digital imaging device. Applies only when
* using board-controlled shutter timing (with a few cameras) for which
* shutter timing methods have been programmed into the library.  The
* valid range is camera-dependent. See \b method_camera_shutter_timing
* configuration directive for more information.
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return Exposure time, in milliseconds.
* @see pdv_set_exposure, \b method_camera_shutter_timing directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
* @ingroup settings 
*/
int
pdv_get_exposure(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_get_exposure() %d\n", pdv_p->dd_p->shutter_speed);

    return (pdv_p->dd_p->shutter_speed);
}

void
pdv_set_defaults(PdvDev * pdv_p)
{
    Dependent *dd_p = pdv_p->dd_p;

    if (dd_p->default_shutter_speed != NOT_SET)
        pdv_set_exposure(pdv_p, dd_p->default_shutter_speed);
    if (dd_p->default_gain != NOT_SET)
        pdv_set_gain(pdv_p, dd_p->default_gain);
    if (dd_p->default_offset != NOT_SET)
        pdv_set_blacklevel(pdv_p, dd_p->default_offset);
}

/**
* Read serial response, wait for timeout (or serial_term if specified),
* max is 2048 (arbitrary).
* This subroutine has limited usefulness. While it is convenient in that it combines the
* wait/read sequence, optimized command/response is usually better accomplished with
* separate #pdv_serial_command / #pdv_serial_wait / #pdv_serial_read sequences
*
* @return number of characters read
* @see pdv_serial_read, pdv_serial_wait
* @ingroup settings
*/
int
pdv_read_response(PdvDev * pdv_p, char *buf)
{
    int     len;

    /*
    * serial_timeout comes from the config file, or default of 1 sec
    */
    len = pdv_serial_wait(pdv_p, pdv_p->dd_p->serial_timeout, 2048);
    if (len)
        pdv_serial_read(pdv_p, buf, len);
    return len;
}


/**
* Gets the gain on the device.
*
* Applies only to cameras for which extended control capabilities have
* been written into the library, such as the Kodak Megaplus i series.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
*
* @return Gain value.  The valid range is -128 to 128.  The actual range is 
* camera-dependent.
*
* @ingroup settings 
*/
int
pdv_get_gain(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_get_gain() %d\n", pdv_p->dd_p->gain);

    return (pdv_p->dd_p->gain);
}


/**
* Gets the black level (offset) on the imaging device. Applies only to cameras
* for which extended control capabilities have been written into the libaray,
* such as the Kodak Megaplus i series.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return Black level value
* @ingroup settings 
*/
int
pdv_get_blacklevel(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_get_blacklevel() %d\n", pdv_p->dd_p->level);

    return (pdv_p->dd_p->level);
}

/**
* This method is obsolete and should not be used.
* The current implementation creates a warning message and returns
* -1 signifying failure.
*
* @return -1 for failure, always.
* @ingroup obsolete 
*/
int
pdv_get_aperture(PdvDev * pdv_p)
{
    edt_msg(PDVWARN, "pdv_get_aperture is OBSOLETE\n");
    return (-1);
}

/**
* Tell the EDT framergrabber hardware to invert each pixel before transferring it
* to the host computer's memory.  This is a hardware operation that is
* implemented in the board's firmware and has no impact on performance.
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param val   1=invert, 0=normal
* @return void
* 
* @ingroup settings 
*/
void
pdv_invert(PdvDev * pdv_p, int val)
{
    u_int   data_path;

    /* data_path = edt_reg_read(pdv_p, PDV_DATA_PATH); */
    data_path = pdv_p->dd_p->datapath_reg;

    edt_msg(DBG2, "pdv_invert(%d)\n", val);

    if (val)
        data_path |= PDV_INVERT;
    else
        data_path &= ~PDV_INVERT;

    edt_reg_write(pdv_p, PDV_DATA_PATH, data_path);
    pdv_p->dd_p->datapath_reg = data_path;
}

/**
* Get the state of the hardware invert register enable bit.
* See /ref pdv_invert for details on this feature.
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return state of the enable bit for this feature:  1=enabled, 0=disabled
* 
* @ingroup settings 
*/
int
pdv_get_invert(PdvDev * pdv_p)
{
    u_int   data_path;

    return (int)((edt_reg_read(pdv_p, PDV_DATA_PATH) & PDV_INVERT) != 0);
}


/**
* Enable hardware overwrite of first two bytes of the frame with a counter.
* Counter increments by one for every frame received by the framegrabber.
* Disabling this also resets the counter to zero, unless framesync mode is
* also enabled (see pdv_enable_framesync).
*
* Only available on  PCIe8 DVa C-Link, Visionlink, and going forward.
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param val   1=enable, 0=disable
* @return void
* 
* @ingroup settings 
*/
void
pdv_set_firstpixel_counter(PdvDev * pdv_p, int ena)
{
    if (pdv_p->devid >= PE8DVCL_ID)
    {
      u_int reg = edt_reg_read(pdv_p, PDV_CL_CFG2);
      if (ena)
          edt_reg_write(pdv_p, PDV_CL_CFG2, reg | PDV_CL_CFG2_PE8_FRMTAG2);
      else edt_reg_write(pdv_p, PDV_CL_CFG2, reg & ~PDV_CL_CFG2_PE8_FRMTAG2);
    }
}

/**
* Query state of the hardware first pixel counter register enable bit.
* See /ref pdv_set_firstpixel_counter for details on this feature.
*
* Only available on  PCIe8 DVa C-Link, Visionlink, and going forward.
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return state of the enable bit for this feature:  1=enabled, 0=disabled
* 
* @ingroup settings 
*/
int
pdv_get_firstpixel_counter(PdvDev * pdv_p)
{
    if (pdv_p->devid >= PE8DVCL_ID)
    {
      int reg = edt_reg_read(pdv_p, PDV_CL_CFG2);

      return (int)((edt_reg_read(pdv_p, PDV_CL_CFG2) & PDV_CL_CFG2_PE8_FRMTAG2) != 0);
    }

    return 0;
}
        

/**
* Obsolete, included for backwards compatability only -- see
* #pdv_interlace_method.
* @ingroup obsolete 
*/
u_short
pdv_get_interlaced(PdvDev * pdv_p)

{
    edt_msg(DBG2, "pdv_get_interlaced() %u\n", pdv_p->dd_p->interlace);

    return (pdv_p->dd_p->interlace);
}

/**
* Returns the value of the \b force_single flag. This flag is 0 by default, and
* is set by the \b force_single directive in the config file (see Camera
* Configuration Guide). This flag is generally set in cases where the camera
* uses a trigger method that will violate the pipelining of multiple ring
* buffers. Most cameras are either continuous, or triggered from the frame
* grabber, or triggered externally through a trigger line, and won't have this
* flag set. But a very few cameras use a serial command or similar to trigger
* the camera, and possibly require a response to be read, in which case the
* parallel scheme won't work. It is for such cases that this variable is meant
* to be used. In these cases, the application should allocate only a single
* buffer (pdv_multibuf(pdv_p, 1)), and should never pre-start more than one
* buffer before waiting for it.  
*
* The take.c program has an example of use of this routine.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return Value of the \b force_single flag.
* @see \b force_single <a href="http://edt.com/downloads/camera-configuration-guide.pdf">camera
* configuration</A> directive, pdv_multibuf
* @ingroup acquisition 
*/
int
pdv_force_single(PdvDev * pdv_p)
{
    return pdv_p->dd_p->force_single;
}

/**
* Obsolete.
* Is variable_size set ("variable_size: 1" in the config file)?
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @see \b variable_size <a href="http://edt.com/downloads/camera-configuration-guide.pdf">camera configuration</A> directive
* @ingroup edt_undoc
*/
int
pdv_variable_size(PdvDev * pdv_p)
{
    return pdv_p->dd_p->variable_size;
}



/** @addtogroup serial
* @{ 
*/

/**
* 
* Preforms a serial read over the RS-422 or RS-232 lines if EDT has provided
* a special cable to accommodate RS-422 or RS-232 serial control.
*
* The buffer passed in will be NULL-terminated if nullterm is true.
*
* @param pdv_p    device struct returned from pdv_open
* @param buf      pointer to data buffer--must be preallocated to at least \b count bytes
* @param size number of bytes to be read, which must be at most one less than the size of the buf (so there is room for the NULL terminator). 
* @param nullterm true to null terminate the buffer read in, false to disable that.
*
* @return The number of bytes read into buf. 
*/

int
pdv_serial_read_nullterm(PdvDev * pdv_p, char *buf, int size, int nullterm)
{
#define PKT_OVERHEAD 2
#define PKT_SIZE_MAX 15

    int     bytesReturned;
    Dependent *dd_p;

    if (pdv_p == NULL || pdv_p->dd_p == NULL)
        return 0;

    pdv_serial_check_enabled(pdv_p);

    dd_p = pdv_p->dd_p;

    if (buf == NULL || pdv_p == NULL || size == 0)
        return (-1);

    bytesReturned = edt_get_msg(pdv_p, buf, size);
    assert(bytesReturned <= size);

    if (nullterm)
        buf[bytesReturned] = 0;

    if (Pdv_debug)
    {
        int     i, num = bytesReturned;
        int is_ascii = 1;

        if (num > 32)		/* limit number of bytes printed to 16 */
            num = 32;

        for (i=0;i<num; i++)
        {
            if (!isascii(buf[i]))
                is_ascii = 0;
        }
        edt_msg(DBG2, "pdv_serial_read(<");
        for (i = 0; i < num; i++)
        {
            if (is_ascii)
                edt_msg(DBG2, "%c", (char) buf[i]);
            else edt_msg(DBG2, "%02x%s", (u_char) buf[i], i < num-1?", ":"");
        }
        edt_msg(DBG2, ">, %d)\n", size);
    }
    return bytesReturned;
}

/**
* Performs a serial read over the serial control lines.
* 
* The serial data read will be stored in a user supplied buffer. That buffer
* will be NULL-terminated. Use pdv_serial_read_nullterm(pdv_p, FALSE) if you
* don't want that behavior.
* 
* @Example
* @code
* int count = 64;  // wait for 64 bytes, or timeout, whichever comes first.
* int got = pdv_serial_wait(pdv_p, 0, count); 
* char buf[count+1];
*
* // read the data we waited for
* pdv_serial_read(pdv_p, buf, got);
* if (got < count)
*   printf("timeout occurred while waiting for serial data\n");
* if (got != 0)
*   printf("data read over serial: %s\n", buf);
* @endcode
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param buf   pointer to data buffer--must be preallocated to at least \b
* count + 1 bytes (\b count bytes of data plus a one byte NULL terminator).
* @param count  Number of bytes to be read.
* @return the number of bytes read into the buffer
* @see pdv_serial_wait
*/

int
pdv_serial_read(PdvDev * pdv_p, char *buf, int count)
{

    return pdv_serial_read_nullterm(pdv_p,buf,count,TRUE);
}

/**
* wrapper for edt_send_msg, but added pause between bytes if
* indicated by pause_for_serial (done initially for imperx cam)
* @return 0 on success, -1 on failure. If an error occurs, call
* #pdv_perror to get the system error message. 
*/
int
pdv_send_msg(PdvDev *ed, int chan, const char *buf, int size)
{
    int i, ret = 0;
    int pause = ed->dd_p->pause_for_serial;
    char bbuf[32];

    pdv_serial_check_enabled(ed);

    /* sleep between bytes if indicated */
    if (pause)
    {
        for (i=0; i<size; i++)
        {
            bbuf[0] = buf[i];
            edt_msleep(ed->dd_p->pause_for_serial);
            if ((ret = edt_send_msg(ed, chan, bbuf, 1)) != 0)
                return ret;
        }
        return ret;
    }
    else return edt_send_msg(ed, chan, buf, size);
}

/**
* Writes a serial command buffer to a serial aia (Kodak type) device.
* 
* Note: applications should pretty much ALWAYS use pdv_serial_command or
* pdv_serial_binary_command instead of calling pdv_serial_write directly
* since, when a FOI is detected, those two calls prepend the required that
* is needed to pass the command on to the camera.
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param buf string to send to the device 
* @param size	number of bytes to write
* 
* @return 0 on success, -1 on failure (and errno is set). 
* If an error occurs, call #pdv_perror to get the system error message. 
*/
int
pdv_serial_write_single_block(PdvDev * pdv_p, const char *buf, int size)
{
    int     ret;
    Dependent *dd_p;

    if (size == 0)
        return (-1);

    if (Pdv_debug)
    {
        int     i, num = size;

        if (num > 16)		/* limit number of bytes printed to 16 */
            num = 16;
        edt_msg(DBG2, "pdv_serial_write_single_block(<");
        for (i = 0; i < num; i++)
        {
            edt_msg(DBG2, "%02x", (u_char) buf[i]);
            if (i + 1 < num)
                edt_msg(DBG2, ", ");
            else
                break;
        }
        edt_msg(DBG2, ">, %d)\n", size);
    }

    if (buf == NULL || pdv_p == NULL || pdv_p->dd_p == NULL)
        return (-1);
    dd_p = pdv_p->dd_p;

    ret = pdv_send_msg(pdv_p, pdv_p->channel_no, buf, size);
    return (ret);
}

/**
* pdv_serial_write_avail
* Get the number of bytes available in the driver's serial write buffer.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 

* @return the number of bytes available in the driver's write buffer 
**/
int
pdv_serial_write_available(PdvDev *pdv_p)

{

    int avail;

    edt_ioctl(pdv_p, EDTG_SERIAL_WRITE_AVAIL, &avail);

    return avail;

}

static int pdv_serial_block_size = 1024;

/**
* Sets the block size for serial writes if the default of 512 is not adequate.
*
* @param newsize the new serial block size
* @return void
*/
void pdv_set_serial_block_size(int newsize)
{
    pdv_serial_block_size = newsize;
}

/**
* Returns the block size for serial writes.
*
* @return the serial block size
* @see pdv_get_serial_block_size
*/
int
pdv_get_serial_block_size()
{
    return pdv_serial_block_size;
}

/**
* @ingroup edt_undoc 
*/

#define SERIAL_ENABLED_FLAGS (PDV_EN_TX | PDV_EN_RX | PDV_EN_RX_INT | PDV_EN_DEV_INT)

int
pdv_serial_read_enable(PdvDev *pdv_p)

{

    edt_reg_or(pdv_p, PDV_SERIAL_DATA_CNTL, SERIAL_ENABLED_FLAGS);

    edt_set_remote_intr(pdv_p, TRUE);

    pdv_p->is_serial_enabled = 1;

    return 0;
}


int  
pdv_serial_read_disable(PdvDev *pdv_p)

{
    edt_set_remote_intr(pdv_p, FALSE);
    edt_reg_and(pdv_p, PDV_SERIAL_DATA_CNTL, ~SERIAL_ENABLED_FLAGS);
    pdv_p->is_serial_enabled = 0;

    return 0;
}

int  
pdv_serial_check_enabled(PdvDev *pdv_p)

{
    if (!pdv_p->is_serial_enabled)
        return pdv_serial_read_enable(pdv_p);

    return 0;
}



/**
* Performs a serial write over the serial lines. This command applies only to
* cameras that use a serial control method.
*
* This function is mainly for sending binary data over the serial lines to the
* camera. It can be used for ASCII commands, but #pdv_serial_command is generally easier.
* 
* For a detailed example of serial communications, see the serial_cmd.c example
* program.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open
* @param buf buffer containing serial command(s)
* @param size number of bytes to send
* @return 0 on success, -1 on failure. 
* @see pdv_serial_command
*/
int
pdv_serial_write(PdvDev *pdv_p, const char *buf, int size)

{
    int avail;
    int left = size;
    int ret = 0;
    int offset = 0;
    int speed = pdv_get_baud(pdv_p);
    int sleepval;
    int chunk = pdv_serial_block_size;

    pdv_serial_check_enabled(pdv_p);

    if (speed != 0)
        sleepval = speed/10;
    else
        sleepval = 10;

    sleepval = (chunk * 1000) / sleepval; /* time to send chunk chars */

    avail = pdv_serial_write_available(pdv_p);
    if (avail > size)
    {
        return pdv_serial_write_single_block(pdv_p, buf, size);
    }
    else
    {
        left -= avail;
        offset += avail;

#ifdef DBG_SERIAL_IO
        printf("Writing %d chars\n", avail);
#endif

        ret = pdv_serial_write_single_block(pdv_p, buf, avail);

        if (ret != 0)
            return ret;
        while (left > 0)
        {
            edt_msleep(sleepval);
            avail = pdv_serial_write_available(pdv_p);
            if (avail > 0)
            {
                if ( avail > left)
                {
                    avail = left;
                    left = 0;
                }
                else
                {
                    left -= avail;
                }

#ifdef DBG_SERIAL_IO
                printf("Writing %d chars\n", avail);
#endif
                ret = pdv_serial_write_single_block(pdv_p, buf+offset, avail);
                if (ret != 0)
                    return ret;
                offset += avail;

            }
        }
    }
    return 0;
}

/**
* Performs a serial read over the serial control lines, blocks until all
* requested serial is read.
* 
* Similar to pdv_serial_read but blocks until all requested serial bytes
* have been received.  The serial data read will be stored in a user
* supplied buffer. 
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param buf   pointer to data buffer--must be preallocated to at least \b
* count + 1 bytes (\b count bytes of data plus a one byte NULL terminator).
* @param count  Number of bytes to be read.
* @return the number of bytes read in
* @see pdv_serial_wait, pdv_serial_read
*/

int
pdv_serial_read_blocking(PdvDev *pdv_p, char *buf, int size)

{
    int left = size;
    int ret = 0;
    int offset = 0;
    int total = 0;


    int avail = 1024;
    int speed = pdv_get_baud(pdv_p);
    int sleepval;
    int chunk = pdv_serial_block_size;


    if (speed != 0)
        sleepval = speed/10;
    else
        sleepval = 10;

    sleepval = (chunk * 1000) / sleepval; /* time to send chunk chars */


    while (left > 0)
    {
        avail = pdv_serial_wait(pdv_p, sleepval, chunk);
        if (avail)
        {
            if ( avail > left)
            {
                avail = left;
                left = 0;
            }
            else
            {
                left -= avail;
            }

#ifdef DBG_SERIAL_IO
            printf("Reading %d chars total = %d\n", avail, offset + avail);
#endif
            ret = pdv_serial_read(pdv_p, buf+offset, avail);
            offset += avail;

            total += ret;

        }


    }



    return total;

}


/**
* Sends an ASCII serial command to the camera, with ASCII camera command
* formatting. Applies only to cameras that use a serial control method for
* camera-computer communications.
*
* Appends the required serial terminator onto the string before sending.  The
* default serial terminator is the `\\r' (carriage return) character, which
* is the most common serial terminator character for cameras with use ASCII
* serial command sets.   If the \b serial_term directive is present in the config file
* in use, it will use the terminator specified by that instead. For example, if
* the camera requires a CR/LF (carriage return/line feed) to terminate instead
* of just a single carriage return, make sure the following command is in the
* config file in use:
* @code
* serial_term: "\r\n"
* @endcode
*
* Also available but much less common is the serial prefix, which can also be
* added to any command via the \b serial_prefix camera configuration directive.
* By default there is no serial prefix.
*
* For a detailed example of serial communications, see the serial_cmd.c example
* program.
*
* Consult your camera manufacturer's users guide for information on serial
* command format reqirements.
*
* @Example
* @code
* pdv_serial_command(pdv_p, "DEF_ON"); // set defect correction on
* @endcode
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param cmd command -- must be a valid serial command for the camera
* in use, as as defined in the camera manufacturer's user's manual
* 
* @return 0 on success, -1 on failure
*
* @todo add note about method_serial_format PULNIX_1010 prepending special
* characters, and review @see part of this comment.
*
* @see pdv_serial_term, pdv_serial_prefix, pdv_set_serial_delimiters, pdv_serial_write
*/
int
pdv_serial_command(PdvDev * pdv_p, const char *cmd)
{
    /* older comment:
    * Sends a serial AIA command convenience wrapper for pdv_serial_write()
    * -- takes the command string and adds the '\r' and deals with the
    * string length issue... Prepends a 'c if FOI. Because of the FOI
    * issue, applications should ALWAYS use this or
    * pdv_serial_binary_command instead of calling pdv_serial_write
    * directly.
    * 
    * UNLESS -- serial_format is SERIAL_PULNIX_1010, in which case it prepends an
    * STX (0x2) and appends an ETX (0x3)
    */
    return pdv_serial_command_flagged(pdv_p, cmd, 0);
}

/**
* Bottom level serial_command that takes a flag for different options.
*
* Primarily for internal use; applications should avoid calling directly and
* instead use pdv_serial_command.
* 
* The only flag is the SCFLAG_NORESP flag, which says whether to wait for
* response on FOI. Normal case is no, but internally (when called from
* pdv_set_exposure, for example) the flag is set to 1 so it doesn't slow
* down the data stream.
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open
* @param cmd command to send
* @param flag flag whether to wait for response on FOI
* @return 0 on success, -1 on failure
*/
int
pdv_serial_command_flagged(PdvDev * pdv_p, const char *cmd, u_int flag)
{
    char    *buf;
    int     ret;
    int     i;
    int     len=0;
    int     bufsize=8; /* arbitrary but should handle whatever extra is tacked on below */
    const char    *p = cmd;

    if (pdv_p == NULL || pdv_p->dd_p == NULL)
        return -1;

    /* find the size (new, was fixed @ 256, now dynamic) */
    while (*p++)
        ++bufsize;
    buf = (char *)malloc(bufsize+16);
    /* 16 is arbitrary, to allow for serial_prefix/serial_term */


    /* new:  serial_prefix -- prepend if nonzero */
    if (*pdv_p->dd_p->serial_prefix)
    {
        strcpy(&(buf[len]), pdv_p->dd_p->serial_prefix);
        len += (int)strlen(pdv_p->dd_p->serial_prefix);
    }

    /* strip off  CR or LF, then add prepend/append STX/ETX */
    if (pdv_p->dd_p->serial_format == SERIAL_PULNIX_1010)
    {
        buf[len++] = 0x02;	/* prepend STX */
        for (i = 0; i < bufsize; i++)
            if (cmd[i] == '\r' || cmd[i] == '\n' || cmd[i] == 0)
                break;
            else
                buf[len++] = cmd[i];
        buf[len++] = 0x03;	/* append ETX */
    }
    else
    {
        /* strip off  CR or LF, then add serial_term */

        for (i = 0; i < bufsize; i++)
        {
            if (cmd[i] == '\r' || cmd[i] == '\n' || cmd[i] == 0)
                break;
            else
                buf[len++] = cmd[i];
        }
        /* ALERT! need to change for FOI if \\n! */
        sprintf(&(buf[len]), "%s", pdv_serial_term(pdv_p));
        len += (int)strlen(pdv_serial_term(pdv_p));
    }

    if (Pdv_debug)
        debug_print_serial_command(buf);
    ret = pdv_serial_write(pdv_p, buf, len);

    free(buf);
    return (ret);
}

/**
* Get the serial terminator.
* See pdv_serial_command for more about the serial terminator.
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open
* @return a character string containing any serial terminator character(s)
* @see pdv_serial_command
*/
char   *
pdv_serial_term(PdvDev * pdv_p)
{
    return pdv_p->dd_p->serial_term;
}

/**
* Get the serial prefix.
* See pdv_serial_command for more about the serial prefix.
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open
* @return a character string containing any serial prefix character(s)
* @see pdv_serial_command
*/
char   *
pdv_serial_prefix(PdvDev * pdv_p)
{
    return pdv_p->dd_p->serial_prefix;
}

/**
* Get the serial prefix.
*
* The serial prefix (if any) is typically set through the config file, which is that
* is the preferred way to set up any serial delimiters; calling this subroutine
* directly should be avoided.
*
* See #pdv_serial_command for more about the serial delimiters.
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open
* @param prefix - see #pdv_serial_command
* @param term - see #pdv_serial_command
* @see pdv_serial_command
* @return void
*/
void
pdv_set_serial_delimiters(PdvDev *pdv_p, char *prefix, char *term)

{
    if (prefix)
        strncpy(pdv_p->dd_p->serial_prefix,prefix, sizeof(pdv_p->dd_p->serial_prefix)-1);
    else
        pdv_p->dd_p->serial_prefix[0] = 0;

    if (term)
        strncpy(pdv_p->dd_p->serial_term,term, sizeof(pdv_p->dd_p->serial_term)-1);
    else
        pdv_p->dd_p->serial_term[0] = 0;

    edt_set_dependent(pdv_p,pdv_p->dd_p);
}


/* debug print ASCII serial command string, from pdv_serial_command */
static void
debug_print_serial_command(char *cmd)
{
    char    tmpbuf[256];
    char   *p = cmd;
    char   *pp = tmpbuf;
    int    len=0;

    while (*p != '\0')
    {
        if (*p == '\r')
        {
            sprintf(pp, "\\r");
            pp += 2;
            len += 2;
            ++p;
        }
        else if (*p == '\n')
        {
            sprintf(pp, "\\n");
            pp += 2;
            len += 2;
            ++p;
        }
        else if (!isprint(*p))
        {
            sprintf(pp, "<%02x>", (*p) & 0xff);
            pp += 4;
            len += 4;
            ++p;
        }
        else
        {
            *(pp++) = *(p++);
            ++len ;
        }
        if (len > 250) /* arbitrary max, just bail out so we don't crash */
            break;
    }
    *pp = '\0';
    edt_msg(DBG2, "pdv_serial_command(\"%s\")\n", tmpbuf);
}

/**
* Sends binary serial command(s) to the camera. Applicable only to cameras that
* use RS-232 or RS-422 binary serial for camera-computer communications.
* Similar to #pdv_serial_command, but for binary instead of ASCII commands, it
* uses a count instead of a terminating NULL to indicate the end of the data.
* Also, it doesn't add on any terminating CR or LF characters.
*
* Consult your camera manufacturer user's guide for information on serial
* command format requirements. 
*
* For a detailed example of serial communications, see the serial_cmd.c
* example program.
*
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param cmd buffer containing serial command(s)
* @param len number of bytes to send
*
* @return 0 on success, -1 on failure. 
*
* @see pdv_serial_command, pdv_serial_read, pdv_serial_wait
* 
* @todo review this comment (i already took out "don't use write() 'cuz of
* FOI..")
*
* @internal
* pdv_serial_binary_command -- backwards compat, use the FLAGGED version if
* you want to use flags such as SCFLAG_NORESP (don't wait for response
* before returning)
*/
int
pdv_serial_binary_command(PdvDev * pdv_p, const char *cmd, int len)
{
    return pdv_serial_binary_command_flagged(pdv_p, cmd, len, 0);
}


/**
* Send a Basler formatted serial frame. Adds the framing and BCC,
* ref. BASLER A202K Camera Manual Doc. ID number DA044003
*
* RETURNS 0 on success, -1 on failure
*/
int
pdv_send_basler_frame(PdvDev * pdv_p, u_char *cmd, int len)
{
    int i;
    u_char frame[128];
    u_char *p = frame;
    u_char bcc = 0;

    *p++ = 0x02;
    for (i=0; i<len; i++)
    {
        bcc ^= cmd[i];
        *p++ = cmd[i];
    }
    *p++ = bcc;
    *p++ = 0x03;

    return pdv_serial_binary_command_flagged(pdv_p, (char *)frame, len+3, 0);
}

/**
* Read a Basler binary frame command.  Check the framing and BCC
* -- ref. BASLER A202K Camera Manual Doc. ID number DA044003
*
* RETURNS number of characters read back, or 0 if none or failure
*/
int
pdv_read_basler_frame(PdvDev * pdv_p, u_char *frame, int len)
{
    int n, nn;
    char tmpbuf[128];
    char *p;

    n = pdv_serial_wait(pdv_p, 500, len);
    if (n < 1)
        return 0;

    nn = pdv_serial_read(pdv_p, tmpbuf, n);

    if (tmpbuf[0] == 0x06) /* ACK */
    {
        frame[0] = 0x06;
        return 1;
    }

    if (tmpbuf[0] == 0x15) /* ACK */
    {
        frame[0] = 0x06;
        return 1;
    }

    if (tmpbuf[0] == 0x02)
    {
        p = &tmpbuf[nn];
        n = pdv_serial_wait(pdv_p, 50, len);
        pdv_serial_read(pdv_p, p, n);
        /* ALERT -- FINISH this! */
    }
    return n;
}

/**
* Send a Duncantech MS / DT series camera frame -- adds the framing and
* checksum, then sends the command. Ref. DuncanTech User Manual Doc
* # 9000-0001-05.
*
* cmdbuf: command buf: typically includes command, 2 size bytes, and
*         size-1 message butes
* size:   number of message bytes plus command byte
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param cmdbuf buffer containing the command, minus framing information
* @param size number of bytes in the cmdbuf
* @return 0 on success, -1 on failure
* @see pdv_read_duncan_frame
*/
int
pdv_send_duncan_frame(PdvDev * pdv_p, u_char *cmdbuf, int size)
{
    int i;
    u_char frame[128];
    u_char *p = frame;

    *p++ = 0x02;
    for (i=0; i<size; i++)
        *p++ = cmdbuf[i];

    CheckSumMessage(frame);

    return pdv_serial_binary_command_flagged(pdv_p, (char *)frame, size+2, 0);
}

static void
CheckSumMessage(unsigned char *msg)
{
    unsigned short length; 
    unsigned char csum = 0;

    msg++;         
    length = *msg++;
    length += *msg++ << 8;
    if (length > 0)
    {
        for (; length > 0; length--)
            csum += *msg++;
        *msg = -csum;
    }
}


/**
* Read response (binary serial) from a Duncantech MS and DT series
* camera -- checks for STX and size, then waits for size+1 more bytes.
* Ref. DuncanTech User Manual Doc # 9000-0001-05.
* 
* Convenience routine for Duncantech (Redlake) DT/MS series cameras only.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param frame buffer containing the frame read back from the camera
*
* @see pdv_send_duncan_frame
*/
int
pdv_read_duncan_frame(PdvDev * pdv_p, u_char *frame)
{
    int n, nn;
    u_short length;

    /* read the STX and 2-byte length */
    n = pdv_serial_wait(pdv_p, 500, 3);
    if (n < 3)
        return 0;

    nn = pdv_serial_read(pdv_p, (char *)frame, 3);

    length = (u_short)frame[1] + (u_short)(frame[2] << 8);

    if (length)
        n = pdv_serial_wait(pdv_p, 1000, length+1);

    if (n)
        pdv_serial_read(pdv_p, (char *)(&frame[3]), n);

    return n+nn;
}

/**
* Sends a binary serial command.
* 
* convenience wrapper for pdv_serial_write() -- takes the command string and
* prepends the 'c' to it if FOI, then calls pdv_serial_write(). Because of
* the FOI issue, applications should ALWAYS use this or one of the other pdv
* serial command calls (pdv_serial_binary_command,
* pdv_serial_command_flagged, etc.) instead of calling pdv_serial_write
* directly
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open
* 
* @param cmd command -- must be a valid serial command for the camera in use, as
* defined in camera manufacturer's user's manual
* 
* @param len number of bytes of cmd to write
* 
* @param flag flag bits -- so far only SCFLAG_NORESP is defined -- tells the driver
* not to wait for a response before returning
* 
* @return 0 on success, -1 on failure
* @ingroup serial
*/
int
pdv_serial_binary_command_flagged(PdvDev * pdv_p, const char *cmd, int len, u_int flag)
{
    char    *buf;
    int     ret;
    int     i;
    int     tmplen = 0;

    edt_msg(DBG2, "pdv_serial_binary_command()\n");

    if (pdv_p == NULL || pdv_p->dd_p == NULL)
        return -1;

    if ((buf = (char *)malloc(len)) == NULL)
        return -1;

    /* don't include CR or LF */
    for (i = 0; i < len; i++)
    {
        buf[tmplen++] = cmd[i];
    }
    ret = pdv_serial_write(pdv_p, buf, tmplen);

    free(buf);
    return (ret);
}

/** @} */ /* end serial */

/** @addtogroup acquisition
* @{
*/

/**
* Reads image data from the EDT framegrabber board.  This is the 
* lowest-level method for aquiring an image. pdv_read is not supported on all platforms;
* and is included mainly for historical reasons; we recommend instead setting up ring
* buffers using #pdv_multibuf and ring buffer subroutines such as #pdv_start_image to do the
* acquire.  pdv_read should never be used when ring buffering is in effect (after
* calling #pdv_multibuf), or be mixed with ring buffer acquisition commands.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param buf      data buffer.
* @param size     size, in bytes, of the data buffer; ordinarily, width * height * bytes per pixel
*
* @Example
* @code
* int size = pdv_get_dmasize(pdv_p) ;
*                                 
* unsigned char *buf = malloc(size);
* int bytes_returned;
*
* bytes_returned = pdv_read(pdv_p, buf, size;
* @endcode
*
* @return The number of bytes read.
*
*/
int
pdv_read(PdvDev * pdv_p, unsigned char *buf, unsigned long size)
{
    Dependent *dd_p;
    unsigned long newsize;
    int     readsize;
    unsigned char *tbuf;
    u_int fc;
    int last_timeouts;

    if (pdv_p == NULL || pdv_p->dd_p == NULL)
        return 0; 

    edt_msg(DBG2, "pdv_read(%d)\n", size);

    pdv_setup_dma(pdv_p);

    last_timeouts = edt_timeouts(pdv_p);

    if (pdv_p->dd_p->start_delay)
        edt_msleep(pdv_p->dd_p->start_delay);


    if (pdv_specinst_serial_triggered(pdv_p))
    {
        edt_msg(PDVWARN, "libpdv invalid combination: SPECINST_SERIAL/pdv_read/i/f trigger");
        edt_msg(PDVWARN, "should use pdv_start_image() or external trigger");
    }

    if (pdv_p == NULL || pdv_p->dd_p == NULL)
        return -1;

    dd_p = pdv_p->dd_p;

    if (pdv_p->devid == DMY_ID)
    {
        pdv_dmy_data(buf, dd_p->width, dd_p->height, dd_p->depth);
        if (dd_p->markras)
            pdv_mark_ras(buf, dd_p->rascnt,
            dd_p->width, dd_p->height, dd_p->markrasx, dd_p->markrasy);
        if (dd_p->markbin)
            pdv_mark_bin(buf, dd_p->rascnt, dd_p->width, dd_p->height, 4, 0);
        if (dd_p->markbin || dd_p->markras) dd_p->rascnt++ ;
        printf("pdv_read rascnt %d\n",dd_p->rascnt) ;
        return (size);
    }

    readsize = pdv_get_dmasize(pdv_p);

    if (dd_p->swinterlace)
        pdv_alloc_tmpbuf(pdv_p);

    /*
    * specify register and value to set after starting dma
    */
    edt_startdma_reg(pdv_p, PDV_CMD, PDV_ENABLE_GRAB);
    if (edt_get_firstflush(pdv_p) != EDT_ACT_KBS)
        edt_set_firstflush(pdv_p, EDT_ACT_ONCE);


    if (dd_p->slop)
    {
        edt_msg(DBG2, "adjusting readsize %x by slop %x to %x\n",
            readsize, dd_p->slop, readsize - dd_p->slop);
        readsize -= dd_p->slop;
    }

    if (dd_p->swinterlace)
    {
        if (pdv_process_inplace(pdv_p))
            tbuf = buf;
        else
            tbuf = pdv_p->tmpbuf;

        newsize = edt_read(pdv_p, tbuf, readsize);
        dd_p->last_raw = tbuf;
        dd_p->last_image = buf;
    }
    else
    {
        tbuf = buf;
        newsize = edt_read(pdv_p, buf, readsize);
        dd_p->last_raw = buf;
        dd_p->last_image = buf;
    }

    dd_p->rascnt = 1;
    if (dd_p->markras)
        pdv_mark_ras(buf, dd_p->rascnt++, dd_p->width, dd_p->height,
        dd_p->markrasx, dd_p->markrasy);

    size = newsize;

    /* edt_startdma_reg(pdv_p, 0, 0) ; */

    edt_msg(DBG2, "swinterlace %d\n", dd_p->swinterlace);

    pdv_deinterlace(pdv_p, dd_p, tbuf, buf);

    if ((pdv_framesync_mode(pdv_p) == PDV_FRAMESYNC_EMULATE_TIMEOUT)
     && (pdv_check_framesync(pdv_p, buf, &fc) > 0)
     && (last_timeouts == edt_timeouts(pdv_p)))
        edt_do_timeout(pdv_p);

    return (size);
}


/**
* Start image acquisition if not already started, then wait for and return
* the address of the next available image.  This routine is the same as doing
* a #pdv_start_image followed by #pdv_wait_image. It is the simplest way to acquire
* an image, and in single shot applications may be all that is needed.
* For continuous sequential transfers with fast cameras, particularly when there
* is processing involved, (including displaying or saving), the separate start / wait
* calls will usually be necessary in order to avoid skipping images.
*
* The format of the returned data depends on the number of bits per pixel and any post-capture
* reordering that is enabled via the config file. For detailed information on data formats,
* see the \ref acquisition section.
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return Address of the next available image buffer that has been
* acquired.
* @see pdv_start_image, pdv_wait_image
*/
unsigned char *
pdv_image(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_image()\n");

    pdv_start_images(pdv_p, 1);
    return pdv_wait_images(pdv_p, 1);
}


/**
* Start image acquisition if not already started, then wait for and return
* the address of the next available image (unprocessed). This routine is the
* same as pdv_image but skips the deinterleave step (if enabled via the
* \b method_interlace config file directive).
*
* @param pdv_p    device struct returned from pdv_open
* @return Address of the next available image buffer that has been acquired
* @see pdv_image
*/
unsigned char *
pdv_image_raw(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_image()\n");

    pdv_start_images(pdv_p, 1);
    return pdv_wait_images_raw(pdv_p, 1);
}

/**
* Starts acquisition of a single image.  Returns without waiting for 
* acquisition to complete.  Used with #pdv_wait_image, which waits for the
* image to complete and returns a pointer to it. pdv_start_image(pdv_p)
* is equivalent to pdv_start_images(pdv_p, 1).
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return void
*/
void
pdv_start_image(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_start_image()\n");

    pdv_start_images(pdv_p, 1);
}


/**
* Starts multiple image acquisition.
*
* Queues multiple image acquisitions. Recommended to be used with ring
* buffering (see pdv_multibuf).  Returns without waiting for acquisition to 
* complete.  Use #pdv_wait_image, #pdv_wait_images, or 
* #pdv_buffer_addresses to get the address(es) of the acquired image(s).
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param count    number of images to start.  A value of 0 starts
* freerun. To stop freerun, call #pdv_start_images again with a \a count of 1.
* @return void
*/
void
pdv_start_images(PdvDev * pdv_p, int count)
{

    edt_msg(DBG2, "pdv_start_images(%d) %d\n", count, pdv_p->dd_p->started_continuous);

    if (pdv_p->dd_p->start_delay)
        edt_msleep(pdv_p->dd_p->start_delay);

    /* automatically alloc 1 buffer if none allocated */
    if (pdv_p->ring_buffer_numbufs < 1)
        pdv_multibuf(pdv_p, 1);

    /* 
    if (pdv_p->devid == DMY_ID)
    {
    dmystarted += count ;
    return;
    }
    */
    if (!pdv_p->dd_p->started_continuous)
    {
        pdv_setup_continuous(pdv_p);
    }
    if (pdv_force_single(pdv_p) && (count > 1))
    {
        edt_msg(PDVWARN,
            "pdv_start_images(): %d buffers requested; should only\n", count);
        edt_msg(PDVWARN,
            "start one at a time when 'force_single' is set in config file\n");
        count = 1;
    }


    edt_start_buffers(pdv_p, count);

    if (pdv_specinst_serial_triggered(pdv_p))
        pdv_trigger_specinst(pdv_p);
    else if (strlen(pdv_p->dd_p->serial_trigger) > 0)
        pdv_serial_command(pdv_p, pdv_p->dd_p->serial_trigger);
}


/**
* Wait for the image started by #pdv_start_image, or for the next image
* started by #pdv_start_images.  Returns immediately if the image started
* by the last call to #pdv_start_image is already complete. 
*
* Use #pdv_start_image to start image acquisition, and #pdv_wait_image to 
* wait for it to complete.  #pdv_wait_image returns the address of the image.
* You can start a second image while processing the first if you've used 
* #pdv_multibuf to allocate two or more separate image buffers.
*
* @note \c pdv_wait_ subroutines wait for all of the image data (as determined by the
* configured width, height and depth) to be read in before returning. If data loss
* occurs during the transfer or there is no incoming camera data, the subroutines
* return (with partial or no data in the buffer) after the image timeout period
* has expired - see #pdv_timeouts, #pdv_set_timeout, #pdv_get_timeout, and #pdv_auto_set_timeout. 
*
* The format of the returned data depends on the number of bits per pixel and any post-capture
* reordering that is enabled via the config file. For detailed information on data formats,
* see the \ref acquisition section.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
*
* @Example
* @code
* //see also simple_take.c and simplest_take.c example program.
* pdv_multibuf(pdv_p, 4);
* pdv_start_image(pdv_p);
* while(1)
* {
*   unsigned char *image;
*
*   image = pdv_wait_image(pdv_p); //returns the latest image
*   pdv_start_image(pdv_p);        //start acquisition of next image
*
*   // (process and/or display image previously acquired here)
*
*   printf("got image\n");
* }
* @endcode
*
* @return Address of the image.
* @see pdv_start_image, pdv_wait_images, pd pdv_wait_image_raw, pdv_wait_image_timed
*/
unsigned char *
pdv_wait_image(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_wait_image()\n");

    return pdv_wait_images(pdv_p, 1);
}

/**
* Identical to #pdv_wait_image, except image data is returned directly
* from DMA, bypassing any post-processing that may be in effect.
*
* Post-processing is enabled by the method_interlace configuration file directive.
* When no method_interlace directive is present in the camera configuration
* file, this subroutine is equivalent to #pdv_wait_image.
*
* For information about camera configuration directives, see the
* <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open
*
* @Example
* @code
* pdv_multibuf(pdv_p, 4);
* pdv_start_image(pdv_p);
* while(1)
* {
*   unsigned char *image;
*
*   image = pdv_wait_image_raw(pdv_p); //returns the latest image
*   pdv_start_image(pdv_p);            //start acquisition of next image
*
*   // (process and/or display image previously acquired here)
*   printf("got raw image\n");
* }
* @endcode
*
* @return Address of the image.
* @see pdv_wait_image
*/
unsigned char *
pdv_wait_image_raw(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_wait_image()\n");

    return pdv_wait_images_raw(pdv_p, 1);
}


/**
 * Identical to #pdv_wait_image but also provides the time at which the
 * DMA was complete on this image. The argument \a timep should point
 * to an array of two unsigned integers which will be filled in with the
 * seconds and microseconds (respectively) of the time the image finished being
 * acquired. #pdv_decode_timestamp can be used to decode the value into a double float.
 *
 * Timestamp comes from the system clock (gettimeofday) at the time the image
 * transfer from the camera to the host memory (DMA) is finished. Latency between
 * the end of DMA and when the timestamp is made will be on the order of a few
 * microseconds. There are other delays in the chain -- the camera may have a
 * lag between the end of frame valid and the end of sending out the data, and system
 * interrupt response time may also be a factor. For more precise
 * timestamping using an external time input, see the
 * <a href="http://www.edt.com/manuals/PDV/pcie8dvcl_irig_appnote.pdf">PCIe8 DV C-Link Application Note: Using the Timestamp function for IrigB input</a>.
 *
 * @param pdv_p pointer to pdv device structure returned by #pdv_open
 * @param timep a pointer to an array of at least two unsigned integers.
 * @see pdv_decode_timestamp
 * 
 * @Example
 * @code
 * u_int timestamp[2];
 *
 * pdv_multibuf(pdv_p, 4); // set up multiple buffers for optimal pipelining
 * pdv_start_image(pdv_p); // start the first image
 *
 * while(1)
 * {
 *  unsigned char *image;
 *
 *  // get the latest image
 *  image = pdv_wait_image_timed(pdv_p, timestamp); 
 *
 *  // start the next one (pipelined)
 *  pdv_start_image(pdv_p); 
 *
 *  // (process and/or display image previously acquired here)
 *
 *  // print the timestamp (double float)
 *  printf("got image at time %lf\n", pdv_decode_timestamp(timestamp));
 * }
 * @endcode
 *
 * @return Address of the image.
 * @see pdv_wait_image, pdv_start_image, pdv_wait_image_raw, pdv_wait_image_timed_raw, pdv_decode_timestamp
 */
unsigned char *
pdv_wait_image_timed(PdvDev * pdv_p, u_int * timep)
{
    return pdv_wait_image_timed_raw(pdv_p, timep, FALSE);
}

/**
* Identical to #pdv_wait_image_timed, except the new argument \a doRaw
* specifies whether or not to perform the deinterleave. If the doRaw
* option is 0, the deinterleave conversion will be performed; if the
* doRaw option is 1, the deinterleave conversion will not be performed.
*
* The argument \a timep should * point to an array of at least two unsigned integers
* which will be filled in with the seconds and microseconds of the time the last
* image was finished being acquired. See #pdv_wait_image_timed for a more detailed
* explanation of the timecode description and handling.
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param timep a pointer to an array of at least two unsigned integers.
* @param doRaw specifies raw (if 1) or interleaved (if 0) image data.
*
* @Example
* @code
* u_int timestamp[2];
*
* pdv_multibuf(pdv_p, 4);
* pdv_start_image(pdv_p);
* while(1)
* {
*   unsigned char *image;
*
*   // get the most recent image
*   image = pdv_wait_image_timed_raw(pdv_p, timestamp, 1); 
*
*   //start acquisition of next image
*   pdv_start_image(pdv_p);
*
*   //process and/or display image previously acquired here
*   printf("got raw image, at time %lf\n", pdv_decode_timestamp(timestamp));
* }
* @endcode
*
* @return Address of the image.
* @see pdv_wait_image, pdv_wait_image_raw, pdv_start_image, pdv_wait_image_timed
*/

unsigned char *
pdv_wait_image_timed_raw(PdvDev * pdv_p, u_int * timep, int doRaw)
{
    u_char *ret;

    edt_msg(DBG2, "pdv_wait_image()\n");

    if (doRaw)
        ret = pdv_wait_images_raw(pdv_p, 1);
    else
        ret = pdv_wait_images(pdv_p, 1);

    edt_get_timestamp(pdv_p, timep, pdv_p->donecount - 1);
    return (ret);
}

/**
* Identical to #pdv_wait_images_timed, except the new argument \a doRaw
* specifies whether or not to perform the deinterleave. If the doRaw
* option is 0, the deinterleave conversion will be performed; if the
* doRaw option is 1, the deinterleave conversion will not be performed.
*
* The argument \a timep should * point to an array of at least two unsigned integers
* which will be filled in with the seconds and microseconds of the time the last
* image was finished being acquired. See #pdv_wait_image_timed for a more detailed
* explanation of the timecode description and handling.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param count number of images to wait for before returning. If
*              \a count is greater than the number of buffers set by
*              #pdv_multibuf, only the last \a count images will be
*              available when this function returns.
* @param timep a pointer to an array of at least two unsigned integers.
* @param doRaw specifies raw (if 1) or interleaved (if 0) image data.
*
* @Example 
* @code 
* unsigned char **bufs;
* unsigned int timestamp[2];
* int doRaw = 1; // true 
* int number_of_images = 4;
*
* pdv_multibuf(pdv_p, num_images);
* pdv_start_images(pdv_p, num_images);
* pdv_wait_images_timed_raw(pdv_p, num_images, timestamp, doRaw);
* bufs = pdv_buffer_addresses(pdv_p);
* printf("got all images, last one at time: %lf\n", pdv_decode_timestamp(pdv_p, timestamp));
*
* for (i=0; i<4; i++)
*   process_image(bufs[i]);   // your processing routine 
* @endcode
* 
* @return The address of the last image. 

* @see pdv_start_image, pdv_wait_images, pdv_wait_images_raw, pdv_wait_images_timed, pdv_decode_timestamp
*/
unsigned char *
pdv_wait_images_timed_raw(PdvDev * pdv_p, int count, u_int * timep, int doRaw)
{
    u_char *ret;

    edt_msg(DBG2, "pdv_wait_images_timed_raw(count=%d, doRaw=%d)\n", count, doRaw);
    if (doRaw)
        ret = pdv_wait_images_raw(pdv_p, count);
    else
        ret = pdv_wait_images(pdv_p, count);

    edt_get_timestamp(pdv_p, timep, pdv_p->donecount - 1);
    return (ret);
}

/**
* Identical to #pdv_wait_images but also returns the time at which the
* DMA was complete on the last image.  The argument \a timep should
* point to an array of at least two unsigned integers which will be
* filled in with the seconds and microseconds of the time the last
* image was finished being acquired.
*
* The argument \a timep should * point to an array of at least two unsigned integers
* which will be filled in with the seconds and microseconds of the time the last
* image was finished being acquired. See #pdv_wait_image_timed for a more detailed
* explanation of the timecode description and handling.
*
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param count number of images to wait for before returning. If
*              \a count is greater than the number of buffers set by
*              #pdv_multibuf, only the last \a count images will be
*              available when this function returns.
*
* @param timep a pointer to an array of at least two unsigned integers.

* @return The address of the last image. 

* @see pdv_start_image, pdv_wait_images, pdv_wait_images_timed_raw, pdv_decode_timestamp
*/
unsigned char *
pdv_wait_images_timed(PdvDev * pdv_p, int count, u_int * timep)
{
    return pdv_wait_images_timed_raw(pdv_p, count, timep, FALSE);
}

/**
* Identical to #pdv_wait_last_image_timed_raw; included for
* backwards compatability only.
*/
unsigned char *
pdv_last_image_timed_raw(PdvDev * pdv_p, u_int * timep, int doRaw)
{
    return pdv_wait_last_image_timed_raw(pdv_p, timep, doRaw);

}

/**
* Identical to #pdv_wait_last_image_raw but also returns the time at which
* the DMA was complete on the last image.
* The argument \a timep should * point to an array of at least two unsigned integers
* which will be filled in with the seconds and microseconds of the time the last
* image was finished being acquired. See #pdv_wait_image_timed for a more detailed
* explanation of the timecode description and handling.
*
* If reordering is not enabled, the data buffer will be the same whether
* doRaw is 0 or 1. For more on data reordering, see the \b method_interlace
* directive in the 
* <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param timep a pointer to an array of at least two unsigned integers.
* @param doRaw specifies raw (if 1) or interleaved (if 0).
* @return The address of the last image. 
* @see pdv_start_images, pdv_wait_images, pdv_wait_last_image_raw
*/
unsigned char *
pdv_wait_last_image_timed_raw(PdvDev * pdv_p, u_int * timep, int doRaw)
{
    u_char *ret;
    int     donecount;
    int     last_wait;
    int     delta;

    donecount = edt_done_count(pdv_p);
    last_wait = pdv_p->donecount;

    edt_msg(DBG2, "pdv_wait_last_image_timed() last %d cur %d\n",
        last_wait, donecount);

    delta = donecount - last_wait;

    if (delta == 0)
        delta = 1;

    if (doRaw)
        ret = pdv_wait_images_raw(pdv_p, delta);
    else
        ret = pdv_wait_images(pdv_p, delta);

    edt_get_timestamp(pdv_p, timep, pdv_p->donecount - 1);
    return (ret);
}

/** 
* Identical to #pdv_wait_last_image, but also returns the time at which
* the DMA was complete on the last image.  The argument \a timep should
* point to an array of at least two unsigned integers which will be
* filled in with the seconds and microseconds of the time the last
* image was finished being acquired.
*
* The argument \a timep should * point to an array of at least two unsigned integers
* which will be filled in with the seconds and microseconds of the time the last
* image was finished being acquired. See #pdv_wait_image_timed for a more detailed
* explanation of the timecode description and handling.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open
* @param timep a pointer to an array of at least two unsigned integers.
*
* @return Address of the image.
* 
* @see pdv_start_images, pdv_wait_image, pdv_wait_image_raw
*/

unsigned char *
pdv_wait_last_image_timed(PdvDev * pdv_p, u_int * timep)
{
    return pdv_wait_last_image_timed_raw(pdv_p, timep, FALSE);
}

/**
* Identical to pdv_wait_last_image_timed; included for
* backwards compatability only.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open
* @param timep a pointer to an array of at least two unsigned integers.
*/
unsigned char *
pdv_last_image_timed(PdvDev * pdv_p, u_int * timep)
{

    return pdv_last_image_timed_raw(pdv_p, timep, FALSE);
}

/**
 * Decode the timestamp from #pdv_get_image_timed and related subroutines.
 *
 * Converts the u_int array returned by the _timed subroutines to a double float, representing
 * the value in nanoseconds.
 *
 * @param timestamp as an array of two unsigned integers, as provided by #pdv_wait_image_timed or similar call
 * @return the timestamp as a double float
 * @see pdv_wait_image_timed, pdv_wait_images_timed, and other pdv_wait_*_timed subroutines
 */
double
pdv_decode_timestamp(PdvDev *edt_p, u_int timestamp[])
{
    double curtime = 0.0;

   /* NOTE: this only handles the default case of timetype being set to EDT_TM_SEC_NSEC.
    * If that was changed (via edt_set_timetype), all bets are off since that was
    * experimental / unsupported code.
    */

    /* secs, nsec */
    curtime = (double) timestamp[0] * 1000000000L + timestamp[1];
    curtime /= 1000000000.0;
    return curtime;
}


/**
* Identical to the pdv_wait_last_image, except that it provides a way to
* determine whether to include or bypass any image deinterleave that is
* enabled.
*
* If data reordering is not enabled, the data buffer will be the same whether
* doRaw is 0 or 1. For more on data reordering, see the \b method_interlace
* directive in the 
* <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>.
*
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open
* @param nSkipped pointer to an integer which will be filled in with
* number of images skipped, if any. 
* @param doRaw specifies raw (if 1) or interleaved (if 0) image data.
*
* @return Address of the image.
*
* @see pdv_start_images, pdv_wait_image, pdv_wait_image_raw
*/
unsigned char *
pdv_wait_last_image_raw(PdvDev * pdv_p, int *nSkipped, int doRaw)
{
    u_char *ret;
    int     donecount;
    int     last_wait;
    int     delta;

    donecount = edt_done_count(pdv_p);
    last_wait = pdv_p->donecount;

    edt_msg(DBG2, "pdv_wait_last_image() last %d cur %d\n",
        last_wait, donecount);

    delta = donecount - last_wait;

    if (nSkipped)
        *nSkipped = (delta) ? delta - 1 : 0;

    if (delta == 0)
        delta = 1;

    if (doRaw)
        ret = pdv_wait_images_raw(pdv_p, delta);
    else
        ret = pdv_wait_images(pdv_p, delta);

    return (ret);
}

/** 
* Waits for the last image that has been acquired. This is useful if
* the display cannot keep up with acquisition and it is not necessary
* to store all images. If this routine is called for a second time
* before another image has been acquired, it will block waiting for the
* next image to complete.
*
* The format of the returned data depends on the number of bits per pixel and any post-capture
* reordering that is enabled via the config file. For detailed information on data formats,
* see the \ref acquisition section.
* 
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open
* @param nSkipped pointer to an integer which will be filled in with
* number of images skipped, if any. 
*
* @return Address of the image.
*
* @Example
* @code
* int skipped_frames;
* u_char *imagebuf;
*
* imagebuf=pdv_wait_last_image(pdv_p &skipped_frames);
* @endcode
* 
* @see pdv_start_images, pdv_wait_image, pdv_wait_image_raw
*/

unsigned char *
pdv_wait_last_image(PdvDev * pdv_p, int *nSkipped)
{
    return pdv_wait_last_image_raw(pdv_p, nSkipped, FALSE);
}

/**
* Identical to the pdv_wait_next_image, except that it provides a way to
* include or bypass any image deinterleave method defined by the \b method_interlace
* config file directive. 
*
* If data reordering is not enabled, the data buffer will be the same whether
* doRaw is 0 or 1. For more on data reordering, see the \b method_interlace
* directive in the 
* <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param nSkipped pointer to an integer which will be filled in with
* number of images skipped, if any. 
* @param doRaw specifies raw (if 1) or interleaved (if 0) image data.
*
* @return Address of the image.
*
* @see pdv_start_images, pdv_wait_image, pdv_wait_next_image
*/

unsigned char *
pdv_wait_next_image_raw(PdvDev * pdv_p, int *nSkipped, int doRaw)
{
    u_char *ret;
    int     donecount;
    int     last_wait;
    int     delta;

    donecount = edt_done_count(pdv_p);
    last_wait = pdv_p->donecount;

    edt_msg(DBG2, "pdv_wait_last_image() last %d cur %d\n",
        last_wait, donecount);

    delta = donecount - last_wait;

    if (*nSkipped)
        *nSkipped = (delta) ? delta - 1 : 0;


    if (delta == 0)
        delta = 1;

    delta++;

    if (doRaw)
        ret = pdv_wait_images_raw(pdv_p, delta);
    else
        ret = pdv_wait_images(pdv_p, delta);

    return (ret);
}

/**
* Waits for the next image, skipping any previously started images.
*
* The format of the returned data depends on the number of bits per pixel and any post-capture
* reordering that is enabled via the config file. For detailed information on data formats,
* see the \ref acquisition section.
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param nSkipped pointer to an integer which will be filled in with
* number of images skipped, if any. 
*
* @return Address of the image.
*
* @see pdv_start_images, pdv_wait_image, pdv_wait_next_image_raw
*/
unsigned char *
pdv_wait_next_image(PdvDev * pdv_p, int *nSkipped)
{

    return pdv_wait_next_image_raw(pdv_p, nSkipped, FALSE);
}


/**
* Gets the status of the continuous flag.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open
*
* @return 1 if continuous, 0 if not.
*
* @see pdv_setup_continuous, pdv_stop_continuous
*/
int
pdv_in_continuous(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_in_continuous() %x\n",
        pdv_p->dd_p->continuous);
    return pdv_p->dd_p->continuous;
}

/** @} */ /* end acquisition */

void
pdv_check(PdvDev * pdv_p)
{
    int     stat;
    int     overrun;

    stat = edt_reg_read(pdv_p, PDV_STAT);
    overrun = edt_ring_buffer_overrun(pdv_p);
    edt_msg(DBG2, "pdv_check() stat %x overrun %x\n",
        stat, overrun);
    if ((stat & PDV_OVERRUN) || overrun)
    {
        if (pdv_p->dd_p->continuous)
            pdv_stop_hardware_continuous(pdv_p);

        pdv_flush_fifo(pdv_p);

        pdv_multibuf(pdv_p, pdv_p->ring_buffer_numbufs);

        if (pdv_p->dd_p->continuous)
            pdv_start_hardware_continuous(pdv_p);
    }
}

void
pdv_checkfrm(PdvDev * pdv_p, u_short * imagebuf, u_int imagesize, int verbose)
{
    u_short *tmpp;

    for (tmpp = (u_short *) imagebuf;
        tmpp < (u_short *) (&imagebuf[imagesize]); tmpp++)
    {
        if (*tmpp & 0xf000)
        {
            edt_msg(DBG2, "found start of image %x at %x %d\n",
                *tmpp >> 12,
                tmpp - (u_short *) imagebuf,
                tmpp - (u_short *) imagebuf);
            if (tmpp != imagebuf)
            {
                int     curdone;
                int     curtodo;

                edt_reg_write(pdv_p, PDV_CMD, PDV_RESET_INTFC);
                pdv_flush_fifo(pdv_p);
                curdone = edt_done_count(pdv_p);
                curtodo = edt_get_todo(pdv_p);
                edt_msg(DBG2, "done %d todo %d\n", curdone, curtodo);
                {
                    pdv_stop_continuous(pdv_p);
                    edt_set_buffer(pdv_p, curdone);
                    pdv_setup_continuous(pdv_p);
                    pdv_start_images(pdv_p, pdv_p->ring_buffer_numbufs -
                        (curtodo - curdone));
                }
            }
            break;
        }
    }
}

u_char *
pdv_get_interleave_data(PdvDev *pdv_p, u_char * buf, int bufnum)

{
    if (!pdv_process_inplace(pdv_p))
    {
        return  pdv_p->output_buffers[bufnum];
    }
    else
        return buf;
}


/**
* Identical to the #pdv_wait_images, except that it skips any
* image deinterleave method defined by the \b method_interlace
* config file directive. 
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param count    number of images to wait for before returning.  If \a count
* is greater than the number of buffers set by #pdv_multibuf, only the last
* \a count images will be available when this function returns.
*
* @Example 
* @code 
* // see also simple_take.c example program 
* unsigned char **bufs;
*
* pdv_multibuf(pdv_p, 4);
* pdv_start_images(pdv_p, 4);
* pdv_wait_images_raw(pdv_p, 4);
* bufs = pdv_buffer_addresses(pdv_p);
*
* for (i=0; i<4; i++)
*   process_image(bufs[i]);   // your processing routine 
* @endcode
*
* @return Address of the last image.
* @ingroup acquisition 
*/
unsigned char *
pdv_wait_images_raw(PdvDev * pdv_p, int count)
{
    u_char *buf;
    Dependent *dd_p;
    u_int fc;
    int last_timeouts;

    edt_msg(DBG2, "pdv_wait_images_raw(%d)\n", count);

    if (pdv_p == NULL || pdv_p->dd_p == NULL)
        return NULL;

    dd_p = pdv_p->dd_p;

    last_timeouts = edt_timeouts(pdv_p);

    if (pdv_specinst_serial_triggered(pdv_p))
        pdv_posttrigger_specinst(pdv_p);

    if (pdv_p->devid == DMY_ID)
    {
        u_char *buf;
        u_int  *tmpp;

        buf = edt_next_writebuf(pdv_p);
        tmpp = (u_int *) buf;
        if (*tmpp != 0xaabbccdd)
        {
            pdv_dmy_data(buf, dd_p->width, dd_p->height, dd_p->depth);
        }
        *tmpp = 0xaabbccdd;
        dd_p->last_raw = buf;
        dd_p->last_image = buf;

        if (dd_p->markras)
            pdv_mark_ras(buf, dd_p->rascnt, dd_p->width, dd_p->height,
            dd_p->markrasx, dd_p->markrasy);
        if (dd_p->markbin)
            pdv_mark_bin(buf, dd_p->rascnt, dd_p->width, dd_p->height, 4, 0);

        if (dd_p->markbin || dd_p->markras) dd_p->rascnt++ ;
        return (buf);
    }
    
    buf = edt_wait_for_buffers(pdv_p, count);

    dd_p->last_raw = buf;
    dd_p->last_image = buf;

    if ((pdv_framesync_mode(pdv_p) == PDV_FRAMESYNC_EMULATE_TIMEOUT)
     && (pdv_check_framesync(pdv_p, buf, &fc) > 0)
     && (last_timeouts == edt_timeouts(pdv_p)))
        edt_do_timeout(pdv_p);

    return buf;
}


/** 
* Waits for the images started by #pdv_start_images. Returns immediately
* if all of the images started by the last call to #pdv_start_images are
* complete.  
* 
* Use #pdv_start_images to start image acquisition of a specificed
* number of images and #pdv_wait_images to wait for some or all of them
* to complete.  #pdv_wait_images returns the address of the last image.
* If you've used #pdv_multibuf to allocate two or more
* separate image buffers, you can start up to the number of buffers
* specified by #pdv_multibuf, wait for some or all of them to
* complete, then use #pdv_buffer_addresses to get the addresses of the
* images.
*
* @note \c pdv_wait_ subroutines wait for all of the image data (as determined by the
* configured width, height and depth) to be read in before returning. If data loss
* occurs during the transfer or there is no incoming camera data, the subroutines
* return (with partial or no data in the buffer) after the image timeout period
* has expired - see #pdv_timeouts, #pdv_set_timeout, #pdv_get_timeout, and #pdv_auto_set_timeout. 
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param count number of images to wait for before returning. If
* \a count is greater than the number of buffers set by
* #pdv_multibuf, only the last \a count images will be available when
* #pdv_wait_images returns.
*
* @Example 
* @code 
* // see also simple_take.c example program 
* unsigned char **bufs;
*
* pdv_multibuf(pdv_p, 4);
* pdv_start_images(pdv_p, 4);
* pdv_wait_images(pdv_p, 4);
* bufs = pdv_buffer_addresses(pdv_p);
*
* for (i=0; i<4; i++)
*   process_image(bufs[i]);   // your processing routine 
* @endcode
* 
* @return The address of the last image. 
*
* @see pdv_wait_images_raw
* @ingroup acquisition 
*/

u_char *
pdv_wait_images(PdvDev *pdv_p, int count)

{
    u_char *buf;
    u_char *retval;
    Dependent *dd_p;
    u_int fc;
    int last_timeouts;

    edt_msg(DBG2, "pdv_wait_images(%d)\n", count);

    if (pdv_p == NULL || pdv_p->dd_p == NULL)
        return NULL;

    dd_p = pdv_p->dd_p;

    last_timeouts = edt_timeouts(pdv_p);

    if (dd_p->swinterlace)
    {
        pdv_alloc_tmpbuf(pdv_p);
    }

    buf = pdv_wait_images_raw(pdv_p, count);


    if (dd_p->swinterlace)
    {

        if (!pdv_process_inplace(pdv_p))
        {

            int buf_index = pdv_p->donecount % pdv_p->ring_buffer_numbufs;

            /* INTERLACE FIX */
            retval = pdv_p->output_buffers[buf_index];

            /* retval = buf + pdv_get_dmasize(pdv_p); */
        }
        else
            retval = buf;

        pdv_deinterlace(pdv_p, dd_p, buf, retval);

        dd_p->last_raw = buf;
        dd_p->last_image = retval;
    }
    else
    {
        retval = buf;
        dd_p->last_raw = buf;
        dd_p->last_image = buf;
    }


    /* problem is that pdv_flshow calling images_raw directly - talk to steve */
    if (pdv_p->devid != DMY_ID)
    {
        if (dd_p->markras)
            pdv_mark_ras(buf, dd_p->rascnt, dd_p->width, dd_p->height,
            dd_p->markrasx, dd_p->markrasy);
        if (dd_p->markbin)
            pdv_mark_bin(buf, dd_p->rascnt, dd_p->width, dd_p->height, 4, 0);

        if (dd_p->markbin || dd_p->markras) dd_p->rascnt++ ;
    }

#if 0 /* NO! pdv_wait_images_raw does this */
    if ((pdv_framesync_mode(pdv_p) == PDV_FRAMESYNC_EMULATE_TIMEOUT)
     && (pdv_check_framesync(pdv_p, buf, &fc) > 0)
     && (last_timeouts == edt_timeouts(pdv_p)))
        edt_do_timeout(pdv_p);
#endif

    return (retval);
}

int
pdv_slop(PdvDev * pdv_p)
{
    return (pdv_p->dd_p->slop);
}


int
pdv_set_slop(PdvDev * pdv_p, int slop)
{
    edt_msg(DBG2, "pdv_set_slop()\n");

    pdv_p->dd_p->slop = slop;
    return (0);
}


/** @addtogroup settings
* @{
*/

/**
* Sets the header (or footer) type. 
*
* Enables header (or footer) functionality including position, size, dma, and associated
* registers for tagging data with magic number, count, and timestamp data.
* 
* Currently only one type, HDR_TYPE_IRIG2 is defined. For more about the
* IRIG functionality on the PCIe8 DV C-Link, see the Timestamping appendinx in the User's Guide.
*
* This subroutine and the associated camera config directive
* \b method_header_type encapsulate setting the header logic for a specific
* method in a single operation. Header functionality can also be implemented by setting
* the header directives directly, via #pdv_set_header_size,
* #pdv_set_header_dma, #pdv_set_header_offset, etc.
*
* The subroutine will return a fail code if the EDT device is one that does not
* support this feature. Currently the PCIe8 DV C-link, PCIe4 DVa C-Link and PCIe4 DVa
* C-link boards support the IRIGB footer (any newer boards are expected to do so as
* well.)  Note that only the device type, not the firmware rev, is checked, and PCIe8
* firmware revs earlier than 4/22/2010 did not support HDR_TYPE_IRIG2. So programmers
* should make sure their board firmware is up-to-date with 4/22/2010 or later firmware
* via pciload. Applications can check edt_get_board_id
*
* header type may be alternately set at init time via the configuration 
* file directive \b method_header_type: \b IRIG2
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param header_type header type, as described above
* @param irig_slave set to 1 if IRIGB time source is from a different device (or not present), 0 otherwise
* @param irig_offset timecode offset, set to 2 typically  (ignored if irig_slave is not set)
* @param irig_raw enables irig timecode (ignored if irig_slave is not set)

* @return 0 in success, -1 on failure

* @see pdv_set_header_size, \b method_header_type directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>,  and the Timestamp appendix in the Users guide.</a>.
*/
pdv_set_header_type(PdvDev *pdv_p, int header_type, int irig_slave, int irig_offset, int irig_raw)
{

    switch(header_type)
    {
        case HDR_TYPE_NONE:
            if (edt_has_irigb(pdv_p))
            {
                edt_reg_and(pdv_p, PDV_UTILITY, ~(PDV_ENFRMCNT|PDVCL_IRIG2));
                pdv_p->dd_p->header_position = HeaderNone;
                pdv_p->dd_p->header_size = 0;
                pdv_p->dd_p->header_dma = 0;
                pdv_p->dd_p->header_type = 0;
            }
            break;
        case HDR_TYPE_IRIG2:
            if (!edt_has_irigb(pdv_p))
                return -1;

            edt_reg_and(pdv_p, PDV_UTILITY, ~(PDV_ENFRMCNT|PDVCL_IRIG2));
            pdv_p->dd_p->header_position = HeaderEnd;
            pdv_p->dd_p->header_size = 32;
            pdv_p->dd_p->header_dma = 1;
            edt_reg_or(pdv_p, PDV_UTILITY, PDVCL_IRIG2); /* just toggled off then back on resets */
            pdv_p->spi_reg_base = PDV_IRIG_SPI_BASE;
            if (pdv_p->channel_no == 1)
                pdv_p->spi_reg_base -= 0x40;

            pdv_irig_set_slave(pdv_p, irig_slave);
            if (!irig_slave)
            {
                pdv_irig_set_offset(pdv_p, irig_offset);
                pdv_irig_set_bcd(pdv_p, irig_raw);
            }
            pdv_p->dd_p->header_type = header_type;

            break;

        default:
            return -1;
    }

    edt_set_dependent(pdv_p, pdv_p->dd_p);

    return 0;
}

/**
* Returns the currently defined header or footer size.  
*
* This is usually set in the configuration file with the directive 
* \b header_size.  It can also be set by calling #pdv_set_header_size.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 

* @return Current header size.

* @see pdv_set_header_size, \b header_size directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
*/
int
pdv_get_header_size(PdvDev * pdv_p)
{
    return (pdv_p->dd_p->header_size);
}

/**
* Returns the header or fotter position value. 
*
* The header position value can by on of the following HdrPosition enumerated values:
*
* - HeaderNone
* - HeaderBefore  
* - HeaderBegin
* - HeaderMiddle
* - HeaderEnd
* - HeaderAfter
* - HeaderSeparate
*
* These values can be set in the configuration file with the 
* \b method_header_position directive.  The values in the configuration file
* should be the same as the definitions above without the leading \b pdv_.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
*
* @return Current header position.
*
* @see pdv_get_header_offset, \b header_offset directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
*/
HdrPosition
pdv_get_header_position(PdvDev * pdv_p)
{
    return (HdrPosition) (pdv_p->dd_p->header_position);
}

/**
* Returns the byte offset of the header in the buffer.  
*
* The byte offset is determined by the header position value.  If header_position
* is PDV_HEADER_BEFORE, the offset is 0; if header_position is PDV_HEADER_AFTER
* (i.e. not really a header but a footer), the offset is the image size.  If
* header_position is PDV_HEADER_WITHIN, the header offset can be set using the
* \b header_offset directive in the camera_configuration file, or by calling 
* #pdv_set_header_offset.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 

* @return A byte offset from the beginning of the buffer.
*
* @see pdv_get_header_position, pdv_set_header_offset
*/
int
pdv_get_header_offset(PdvDev * pdv_p)
{
    switch (pdv_p->dd_p->header_position)
    {
    case HeaderBefore:
        pdv_p->dd_p->header_offset = - (int) pdv_p->dd_p->header_size;
        break;
    case HeaderBegin:
        pdv_p->dd_p->header_offset = 0;
        break;
    case HeaderEnd:
        pdv_p->dd_p->header_offset = pdv_get_dmasize(pdv_p) - pdv_p->dd_p->header_size ;
        break;
    case HeaderAfter:
        pdv_p->dd_p->header_offset = pdv_p->dd_p->imagesize;
        break;

    }

    return (pdv_p->dd_p->header_offset);
}

/**
* Returns the current setting for flag which determines whether the header
* (or footer) size is to be added to the DMA size.  This is true if the camera/device
* returns header information at the beginning or end of its transfer.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return 1 true or 0 false.
*/
int
pdv_get_header_dma(PdvDev * pdv_p)
{
    return (pdv_p->dd_p->header_dma);
}

/**
* Tells if there is a header and it is within the data, and not extra data
* that gets added to the image DMA.
* Returns 1 if header_position is any of the enumerated values HeaderBegin,
* HeaderMiddle, or HeaderEnd. Otherwise it returns 0.
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return 1 true or 0 false.
*/
int
pdv_get_header_within(PdvDev *pdv_p)

{
    return (pdv_p->dd_p->header_position == HeaderBegin ||
        pdv_p->dd_p->header_position == HeaderMiddle ||
        pdv_p->dd_p->header_position == HeaderEnd);
}

/**
* Return the header space allocated but not used for DMA.
* 
* Typically set via the
* \b header_dma and \b header_size directives in the configuration file. 
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @see pdv_get_header_dma, pdv_set_header_size
*/
int
pdv_extra_headersize(PdvDev * pdv_p)

{
    if (pdv_p->dd_p->header_size && (!pdv_p->dd_p->header_dma) &&
        (pdv_p->dd_p->header_position ==  HeaderBefore || 
        pdv_p->dd_p->header_position == HeaderAfter))
        return pdv_p->dd_p->header_size;

    return 0;
}

/**
* Sets the header (or footer) size, in bytes, for the device.  This can also be done by
* using the \b header_size directive in the camera configuration file.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param header_size new value for header size.
*
* @see pdv_get_header_size, \b header_size directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
* @return void
*/
void
pdv_set_header_size(PdvDev * pdv_p, int header_size)
{
    edt_msg(DBG2, "pdv_set_header_size()\n");

    pdv_p->dd_p->header_size = header_size;

}

/**
* Sets the header (or footer) position.
*
* Originally one of PDV_HEADER_BEFORE, PDV_HEADER_WITHIN, PDV_HEADER_AFTER, later changed to
* the HdrPosition enumerated values:
*
* - HeaderNone,
* - HeaderBefore,
* - HeaderBegin,
* - HeaderMiddle,
* - HeaderEnd,
* - HeaderAfter,
* - HeaderSeparate
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param header_position the astarting point for the header position
*
* @see pdv_get_header_offset, pdv_set_header_offset
* @return void
*/
void
pdv_set_header_position(PdvDev * pdv_p, HdrPosition header_position)

{
    edt_msg(DBG2, "pdv_set_header_position(%d)\n", header_position);

    pdv_p->dd_p->header_position = header_position;
}

/**
* Sets the boolean value for whether the image header is included in the DMA
* from the camera.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param header_dma  new value (0 or 1) for the header_dma attribute.
*
* @see pdv_get_header_dma
* @return void
*/
void
pdv_set_header_dma(PdvDev * pdv_p, int header_dma)
{
    edt_msg(DBG2, "pdv_set_header_dma(%d,%d)\n", header_dma);

    pdv_p->dd_p->header_dma = header_dma;
}


/**
* Sets the byte offset of the header data in the allocated buffer.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param header_offset  new value for the header offset.
* @return void
*/
void
pdv_set_header_offset(PdvDev * pdv_p, int header_offset)

{
    edt_msg(DBG2, "pdv_set_header_offset(%d,%d)\n", header_offset);

    pdv_p->dd_p->header_offset = header_offset;
}

/**
* Checks for frame sync and frame count.
*
* Framesync is hardware-enabled frame tagging via extra footer data on every
* frame. With framesync enabled, there are 16 bytes of extra footer data added
* to the frame DMA, with a magic number and frame count. If the magic number is
* not correct, framesync will return an error, allowing the calling function to
* handle the error. Typically this means stopping any continuous capture loop,
* resetting the DMA via #pdv_timeout_restart, and re-starting continuous capture
* Or aborting altogether if repeated failures are detected (e.g.  misconfiguration,
* cable unplugged, hardware failure.) The framecount argument allows users to ensure
* all frames are captured. It is not unusual for frames to be skipped but remain in
* sync; for example if blanking is very short between frames, or if the OS takes an
* extra long snooze to go do something else. Subroutine will return -1 if framesync
* is unsupported or not enabled, 0 if successful, or 1 if an out of sync condition
* is detected. If return code is 0, framecount will be updated with the current
* frame count, otherwise framecount will be 0.
* 
* Framesync functionality is available in PCIe Camera Link framegrabbers except
* the PCIe4 DV C-Link. This subroutine will return -1 if the device does not
* support this feature.
*
* @see pdv_enable_framesync, pdv_framesync_mode;
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param image_p pointer to previously acquired image (via e.g. #pdv_wait_image) for which you want the framesync to be checked.
* @param framecnt pointer to location to put frame counter from this frame.
* @return result code (see description)
*/
int 
pdv_check_framesync(PdvDev *pdv_p, u_char *image_p, u_int *framecnt)
{
    Irig2Record *irp; /* from pdv_irig.h */

    if ((image_p == NULL) || (pdv_framesync_mode(pdv_p) == 0))
        return -1;

    irp = (Irig2Record *)(image_p + pdv_get_header_offset(pdv_p));
    edt_msg(DBG2, "pdv_framesync(): magic %s framecnt %d\n", irp->magic == IRIG2_MAGIC? "OK ": "BAD", irp->framecnt);
    if (irp->magic == IRIG2_MAGIC)
    {
        *framecnt = irp->framecnt;
        return 0; /* success */
    }
    else *framecnt = 0;

    return 1; /* out-of-sync */
}

/**
* Enables frame sync footer and frame out-of-synch detection.
*
* With framesync enabled, extra footer data is added to the frame DMA,
* enabling you to check for an out-of-synch condition using #pdv_check_framesync
* or #pdv_timeouts, and respond accordingly.
* The mode argument should be one of:
*
*  - PDV_FRAMESYNC_OFF: Framesync functionality disabled.
*  - PDV_FRAMESYNC_ON:  Framesync functionality enabled, call #pdv_check_framesync to check for out-of-synch data on a given frame.
*  - PDV_FRAMESYNC_EMULATE_TIMEOUT: Framesync functionality enabled, framesync errors will be reflected as timeouts (see #pdv_timeouts).
*
* Framesync functionality is available in PCIe Camera Link framegrabbers except
* the PCIe4 (no 'a') DV C-Link. No PCI devices support this feature.
*
* @see pdv_framesync, pdv_framesync_mode, pdv_timeouts;
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param mode framesync mode (see above)
* @return 0 on success, -1 if not supported by the device in use.
*/
int
pdv_enable_framesync(PdvDev *pdv_p, int mode)
{
    int ret = 0;


    pdv_p->dd_p->framesync_mode = 0;

    if (!edt_has_irigb(pdv_p))
    {
        ret = -1;
    }
    else
    {
        if (mode)
        {
            if (pdv_set_header_type(pdv_p, HDR_TYPE_IRIG2, 1, 2, 0) < 0)
            {
                edt_msg(DBG2, "pdv_enable_framesync() FAIL\n");
                ret = -1;
            }
            else
            {
                pdv_p->dd_p->framesync_mode = mode;
                edt_msg(DBG2, "pdv_enable_framesync() OK\n");
            }
        }
        else
        {
            if (pdv_p->dd_p->header_type == HDR_TYPE_NONE)
                pdv_set_header_type(pdv_p, HDR_TYPE_NONE, 0, 0, 0);
        }
    }

    return ret;
}

/**
* Returns the framesync mode.
* Can be one of:
*
*  - PDV_FRAMESYNC_OFF: Framesync functionality disabled.
*  - PDV_FRAMESYNC_ON:  Framesync functionality enabled.
*  - PDV_FRAMESYNC_EMULATE_TIMEOUT Framesync functionality enabled, and framesync errors will be reflected as timeouts.
*
* @see pdv_framesync, pdv_check_framesync;
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return 1 if enabled, 0 if not enabled;
*/
int
pdv_framesync_mode(PdvDev *pdv_p)
{
    return pdv_p->dd_p->framesync_mode;
}


/** @} */ /* end settings */

/**
* Return shutter (expose) timing method and mode control (CC) state. 
*
* See \ref pdv_set_shutter_method for an explanation of the return value
* (shutter method) and mcl parameter;
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param mcl mode control (CC line) state
* @return the shutter (expose) timing method
* @see pdv_set_shutter_method
* @ingroup settings 
*/
int
pdv_get_shutter_method(PdvDev * pdv_p, u_int *mcl)
{
    edt_msg(DBG2, "pdv_get_shutter_method()\n");

    *mcl = edt_reg_read(pdv_p, PDV_MODE_CNTL);

    return pdv_p->dd_p->camera_shutter_timing;
}

/**
* Return shutter (expose) timing method. 
*
* This subroutine returns only the timing method, not the mode control (CC lines) state.
* Generally you'll want both so it's recommended to use the newer \ref pdv_get_shutter_method()
* call. See the description for #pdv_set_shutter_method() for explanation of the return values.
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return the shutter (expose) timing method
* @see pdv_set_shutter_method, pdv_get_shutter_method
* @ingroup settings 
*/
int
pdv_shutter_method(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_shutter_method()\n");

    return pdv_p->dd_p->camera_shutter_timing;
}

/**
 * Set the device's exposure method and CC line state.
 * 
 * Typically the exposure method is set in the config file via the \b method_camera_shutter_timing
 * and \b MODE_CNTL_NORM directives. This subroutine provides a programatic way to do the same thing,
 * post-configuration.
 *
 * The most common values for \b method (defined in pdv_dependent.h) are:
 *
 * - \b AIA_SERIAL:    Default. Expose timing is controlled via serial or other (camera-dependent)
 *                     method and the board's hardware is not involved in timing the shutter.
 * - \b AIA_MCL:       CC pulse-width timing, millisecond granularity. Each image capture
 *                     request (e.g. #pdv_start_image) will cause the board to set the EXPOSE (CC) line
 *                     or lines (as set via the \b mcl parameter's left nibble) TRUE for the current expose
 *                     time in milliseconds, as set by #pdv_set_exposure.
 * - \b AIA_MCL_100US: CC pulse-width timing, 100 microsecond granularity. Each image capture
 *                     request (e.g. #pdv_start_image) will cause the board to set the EXPOSE (CC) line
 *                     or lines (as set via the \b mcl parameter's left nibble) TRUE for the current expose
 *                     time in 100 microsecond increments, as set by #pdv_set_exposure.
 * 
 * Several other methods are defined, but most are specific to legacy AIA cameras / framegrabbers and
 * are not applicable to Camera Link. For more information on all available methods see
 * the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>.
 * 
 * The \b mcl parameter sets the state of the four camera control (CC) lines, as an 8-bit hexidecimal
 * number. The right nibble sets the steady state of the CC lines, and the left nibble selects which
 * of these lines, if any, the framegrabber hardware use to send out a trigger or expose pulse on each capture request.
 * Most commonly, this value will be \c 0x00 when the camera generates images continuously or is triggered
 * via an external source, or \c 0x10 if the board should send out a trigger pulse (1 millisecond, if \b method
 * equals \c AIA_SERIAL) or timed pulse (as set via #pdv_set_exposure if \b method equals \c AIA_MCL or
 * \c AIA_MCL_100US) on the CC1 line on each image capture request. See the 
 * the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
 * for information on the less common values.
 *
 * @note The AIA Camera Link specification doesn't define how the four CC lines should be used, if at all. 
 * However in our experience, virtually all Camera Link cameras that have CC-driven trigger or expose modes
 * use CC1, which corresponds to an \b mcl value of \c 0x10. For more details see your camera's
 * documentation, and the description of <b>0x07 Mode Control register</b> in the
 * <a href="http://edt.com/downloads/dv-c-link-firmware-reference.pdf">Firmware Guide for Camera Link</a>.
 * 
 * @param pdv_p pointer to pdv device structure returned by #pdv_open 
 * @param method method (see above)
 * @param mcl mode control (CC line) state (see above)
 * @see pdv_get_shutter_method
 * @return 0 on success, -1 on failure
 * @ingroup settings
 */
int
pdv_set_shutter_method(PdvDev *pdv_p, int method, unsigned int mcl)
{
    if (!pdv_p)
        return -1;

    edt_msg(DBG2, "pdv_set_shutter_method (%d, %02x)\n", method);

    pdv_p->dd_p->camera_shutter_timing = method;
    pdv_p->dd_p->mode_cntl_norm = mcl;

    edt_reg_write(pdv_p, PDV_MODE_CNTL, mcl);

    return 0;
}

/**
* Set the interlace flag. Flag is no longer used so it's obsolete.
* Currently de-interleaving is iset via the config file and the
* dd_p flag is switnerlace.
* @return void
*
* @ingroup edt_undoc 
*/
void
pdv_set_interlace(PdvDev * pdv_p, int interlace)
{
    edt_msg(DBG2, "pdv_set_interlace()\n");

    pdv_p->dd_p->swinterlace = interlace;

    pdv_setup_postproc(pdv_p, pdv_p->dd_p, NULL);

}

/**
* Returns the interlace method, as set from the \b method_interlace directive in the
* configuration file [from #pdv_initcam]. This method is used to determine how
* the image data will be rearranged (if at all) before being returned from #pdv_wait_images
* or #pdv_read.
* 
* For more on deinterleave methods, see the
* <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>.
*
* @note the \c _raw acquisition routines bypass the deinterleave
* logic.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open
*
* @return One of the following interlace methods, defined in pdv_dependent.h
* \arg \b PDV_BGGR
* 8-bit Bayer encoded data
* \arg \b PDV_BGGR_DUAL
* 8-bit Bayer encoded data, from dual channel camera
* \arg \b PDV_BGGR_WORD
* 10-12 bit Bayer encoded data
* \arg \b PDV_BYTE_INTLV 
*      Data is byte interleaved (odd/even pixels are from odd/even lines, 8
*      bits per pixel). 
* \arg \b PDV_WORD_INTLV    
*      Data is word interleaved (odd/even pixels are from odd/even lines, 16
*      bits per pixel).
* \arg \b DALSA_2CH_INTLV
*      Byte data per 2 channel dalsa "A" model sensor format -- see Dalsa D4/D7 camera manual
* \arg \b DALSA_4CH_INTLV
*      Byte data with 4 channel Dalsa formatting -- see Dalsa D4/D7 camera manual
* \arg \b EVEN_RIGHT_INTLV
*      8-bit data, pixels in pairs with 1st pixel from left half, 2nd pixel from right half of screen
* \arg \b PDV_FIELD_INTLC
*      Data is byte interleaved (odd/even pixels are from odd/even lines, 8
* \arg \b PDV_FIELD_INTLC   
*      Data is field interlaced (odd/even lines are from top/bottom half of image).
* \arg \b PDV_ILLUNIS_BGGR
*      BBGR for Illunis cameras (?)
* \arg \b PDV_ILLUNIS_INTLV
*      Byte interleave from Illunis cameras (?)
* \arg \b PDV_INVERT_RIGHT_INTLV
*      Byte data, even pixels are right half, inverted
* \arg \b PDV_PIRANHA_4CH_INTLV
*      Piranha 4 channel line scan format (see Dalsa Piranha camera manual)
* \arg \b PDV_SPECINST_4PORT_INTLV
*      Spectral instruments format (see Spectral Instruments camera manual)
* \arg \b PDV_WORD_INTLV
*      Deinterlaced, word format
* \arg \b PDV_WORD_INTLV_HILO
*      Deinterlaced, 2-bytes per pixel, even first
* \arg \b PDV_WORD_INTLV_ODD
*      Deinterlaced, 2-bytes per pixel, odd first
*
* @see \b method_interlace directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
* @ingroup acquisition 
*/
int
pdv_interlace_method(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_interlace_method()\n");

    return pdv_p->dd_p->swinterlace;
}


/**
* Sets the debug level of the PDV library. This results in debug output
* being written to the screen by PDV library calls.  The same thing can be 
* accomplished by setting the PDVDEBUG environment variable to 1.  See also 
* the program setdebug.c for information on using the device driver debug 
* flags.
*
* To control the output of messages from the DV library, see the \ref
* msg. 
*
* @param flag flags debug output on (nonzero) or off (zero).
*
* @return previous debug level
*
* @ingroup debug
*/
int
pdv_debug(int flag)
{
    int     oldval = Pdv_debug;

    edt_msg(DBG2, "pdv_debug()\n");

    Pdv_debug = flag;
    return (oldval);
}

/**
* Gets the debug level, as set by pdv_debug or outside environment variables.
* For values, see the \ref msg.
*
* @return debug level
*
* @ingroup debug 
*/
int
pdv_debug_level()
{
    return Pdv_debug;
}


/**
* Determines whether data overran on the last aquire.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @returns Number of bytes of data remaining from last aquire.  0 indicates
* no overrun.
* 
* @todo futher explanation... data in fifo, possible prev. timeout, etc.
*
* @ingroup acquisition
*/
int
pdv_overrun(PdvDev * pdv_p)
{
    int     overrun;

    edt_msg(DBG2, "pdv_overrun()\n");

    if (pdv_p->devid == DMY_ID)
        return (0);
    edt_ioctl(pdv_p, EDTG_OVERFLOW, &overrun);
    return (overrun);
}

/**
* Waits for response from the camera as a result of a #pdv_serial_write or
* #pdv_serial_command. After calling this function, use #pdv_serial_read to get
* the data. For a detailed example of serial communications, see the
* serial_cmd.c example program.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param msecs   number of milliseconds to wait before timing out.  If this
* parameter is 0, the defualt timeout value is used, as specified by the 
* \b serial_timeout directive in the current configuration file.  If no default
* timout value was specified, the default is 1000 milliseconds (1 second).
* @param count    Maximum number of bytes to wait for before returning.
*
* @return Number of bytes of serial data returned from the camera.
*
* @see pdv_serial_read for simple example.
* @ingroup serial 
*/
int
pdv_serial_wait(PdvDev * pdv_p, int msecs, int count)
{
    edt_buf tmp;
    int     ret;

    pdv_serial_check_enabled(pdv_p);

    if (msecs == 0)
        msecs = pdv_p->dd_p->serial_timeout;

    tmp.desc = msecs;
    tmp.value = count;
    edt_ioctl(pdv_p, EDTS_SERIALWAIT, &tmp);
    ret = (int) tmp.value;
    edt_msg(DBG2, "pdv_serial_wait(%d, %d) ret %d\n", msecs, count, ret);
    return (ret);
}

/**
* Returns the number of bytes of unread data in the serial response buffer.
* Similar to pdv_serial_wait but doesn't wait for any timeout period,
* nor does it have any minimum count parameter.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param count    Maximum number of bytes to wait for before returning.
*
* @return Number of bytes of unread data in the serial response buffer
*
* @ingroup serial 
*/
int
pdv_serial_get_numbytes(PdvDev * pdv_p)
{
    edt_buf tmp;
    int     ret;

    tmp.desc = 0;
    tmp.value = 0;
    edt_ioctl(pdv_p, EDTS_SERIALWAIT, &tmp);
    ret = (int) tmp.value;
    edt_msg(DBG2, "pdv_serial_get_numbytes %d\n", ret);
    return (ret);
}

/**
* Wait for next serial to come in -- ignore any previous if 0, just wait for
* the next thing, however many it is.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param msecs number of milliseconds to wait before timing out
* @param count number maximum number to wait for 

* @return number of characters seen, can be passed to pdv_serial_read
* @see pdv_serial_wait, pdv_serial_read
*
* @ingroup serial 
*/
int
pdv_serial_wait_next(EdtDev * pdv_p, int msecs, int count)
{
    int     ret;
    int     newcount = count;

    ret = pdv_serial_wait(pdv_p, msecs, count);
    return (ret);
}

/**
* Set serial wait character.
* Normally pdv_serial_wait will wait until the serial_timeout period expires before
* returning (unless the max number of characters is seen). This is the most general
* purpose and robust method since there's no other way of knowing all different camera
* response formats. However if the camera formats are known, and specifically a if
* each response can be expected to be 1 line terminated by the same character
* (such as a newline) every time, then setting the serial_waitchar to that character
* can greatly shorten the time it takes for a pdv_serial_wait call to return.
*
* This character can also be initialized in the
* <a href="http://edt.com/downloads/camera-configuration-guide.pdf">camera configuration</A> directive
* \b serial_waitchar.
*
* @return 0 in success, -1 on failure
* @ingroup serial 
*/
int
pdv_set_waitchar(PdvDev * pdv_p, int enable, u_char wchar)
{
    edt_buf tmp;
    int     ret;


    tmp.desc = enable;
    tmp.value = wchar;
    ret = edt_ioctl(pdv_p, EDTS_WAITCHAR, &tmp);
    pdv_p->dd_p->serial_waitc = wchar;
    if (!enable) /* top bit is flag for disabled (internally) */
        pdv_p->dd_p->serial_waitc |= 0x100;
    edt_msg(DBG2, "pdv_set_waitchar(%d, %02x) returns %d\n", enable, wchar, ret);
    return (ret);
}

/**
* Get serial wait character, or byte. This value, if set, is what pdv_serial_wait
* will return immediately after it comes in instead of waiting for the serial timeout
* period to expire.
*
* @param pdv_p  same as it ever was
* @param waitc  character (byte) to wait for
* 
* @return 1 if waitchar enabled, 0 if disabled
* @see pdv_set_waitchar and \b serial_waitchar directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
* @ingroup serial 
*/
int
pdv_get_waitchar(PdvDev * pdv_p, u_char *waitc)
{
    int ret;

    if (pdv_p->dd_p->serial_waitc & 0x100)
    {
        *waitc = pdv_p->dd_p->serial_waitc & 0xff;
        ret = 0;
    }
    else
    {
        *waitc = pdv_p->dd_p->serial_waitc;
        ret = 1;
    }
    return ret;
}


int
pdv_get_fulldma_size(PdvDev *pdv_p, int *extrasizep)

{
    int     size = pdv_get_imagesize(pdv_p);

    int     slop = pdv_slop(pdv_p);
    int     extrasize;
    Dependent *dd_p = pdv_p->dd_p;

    extrasize = slop + pdv_extra_headersize(pdv_p);

    if (pdv_get_header_dma(pdv_p) && (pdv_get_header_within(pdv_p)))
        size += pdv_get_header_size(pdv_p);

    if (dd_p->swinterlace || size != pdv_get_dmasize(pdv_p))
    {
        /* If extra buffer needed, put it after the data */

        if (!pdv_process_inplace(pdv_p) || size != pdv_get_dmasize(pdv_p))
        {
            int     newsize = pdv_get_dmasize(pdv_p);

            extrasize += size; /* use second part of buffer for interlace result */
            size = newsize;
        }
    }

    if (extrasizep)
        *extrasizep = extrasize;

    return size;
}

int
pdv_total_block_size(PdvDev *pdv_p, int numbufs)

{
    int extrasize = 0;
    int size;

    size = pdv_get_fulldma_size(pdv_p, &extrasize);

    /* round up */
    size = edt_get_total_bufsize(pdv_p, size,/* INTERLACE FIX extrasize */ 0) * numbufs;

    return size;

}


void
pdv_free_output_buffers(PdvDev *pdv_p)

{
    if (pdv_p->output_buffers)
    {
        edt_free((uchar_t *)pdv_p->output_buffers);

        if (pdv_p->output_base)
            edt_free(pdv_p->output_base);

        pdv_p->output_base = NULL;
        pdv_p->output_buffers = NULL;
    }
}


void
pdv_allocate_output_buffers(PdvDev *pdv_p)

{
    /* determine if new allocation needed */

    if (pdv_p->output_buffers)
    {
        pdv_free_output_buffers(pdv_p);
    }

    if (pdv_p->dd_p->swinterlace)
    {
        int size = pdv_get_imagesize(pdv_p);

        u_int totalsize = size * pdv_p->ring_buffer_numbufs;

        if (totalsize > 0)
        {
            pdv_p->output_base = edt_alloc(totalsize);

            pdv_p->output_buffers = (unsigned char **)edt_alloc(sizeof(u_char *) * pdv_p->ring_buffer_numbufs);

            if (pdv_p->output_base && pdv_p->output_buffers)
            {
                int i;
                for (i=0;i<(int)pdv_p->ring_buffer_numbufs;i++)
                    pdv_p->output_buffers[i] = pdv_p->output_base + (size * i);
            }
        }
    }

}

int
pdv_multibuf_block(PdvDev *pdv_p, int numbufs, u_char *block, int blocksize)

{
    int size, extrasize = 0;
    int ret;
    int header_before = (pdv_p->dd_p->header_position == HeaderBefore);

    edt_msg(DBG2, "pdv_multibuf_block(%d, %d, %x)\n", numbufs, block, blocksize);

    /* FIXED INTERLACE */

    size = pdv_get_fulldma_size(pdv_p, &extrasize);


    pdv_setup_dma(pdv_p);

    ret = edt_configure_block_buffers_mem(pdv_p, 
        size, 
        numbufs, 
        EDT_READ, 
        /* INTERLACE FIX extrasize */ 0, 
        header_before,
        block) ;

    pdv_allocate_output_buffers(pdv_p);

    return ret;

}

int
pdv_multibuf_separate(PdvDev *pdv_p, int numbufs, u_char **buffers)

{
    int     size = pdv_get_imagesize(pdv_p);
    int ret;

    Dependent *dd_p = pdv_p->dd_p;

    edt_msg(DBG2, "pdv_multibuf_separate(%d, %x)\n", numbufs, buffers);

    if (dd_p->swinterlace || size != pdv_get_dmasize(pdv_p))
    {
        /* If extra buffer needed, put it after the data */

        if (!pdv_process_inplace(pdv_p) || size != pdv_get_dmasize(pdv_p))
        {
            edt_msg_perror(PDVFATAL,"Interlace not inplace fails for multibuf_separate\n");
            return -1;
        }
    }

    size = pdv_get_fulldma_size(pdv_p, NULL);

    pdv_setup_dma(pdv_p);

    ret = edt_configure_ring_buffers(pdv_p, size, numbufs, EDT_READ, buffers) ;

    return ret;

}

/** 
* Sets the number of multiple buffers to use in ring buffer continuous mode,
* and allocates them. This routine allocates the buffers itself, in kernel or
* low memory as required by the EDT device driver for optimal DMA.
*
* pdv_multibuf need only be called once after a #pdv_open or #pdv_open_channel,
* and before any calls to acquisition subroutines such as #pdv_start_images / #pdv_wait_images.
* If image size changes, call #pdv_multibuf again to re-allocate buffers with the new image size.
*
* The number of buffers is limited only by the amount of host memory available, up to
* approximately 3.5GBytes (or less, depending on other OS use of the low 3.5 GB of memory).
* Each buffer has a certain amount of overhead, so setting a large number,
* even if the images are small, is not recommended. Four is the recommended number:
* at any time, one buffer is being read in, one buffer is being read out, one is
* being set up for DMA, and one is in reserve in case of overlap.  Additional buffers
* may be necessary with very fast cameras; 32 will almost always smooth out any problems with
* really fast cameras, and if the system can't keep up with 64 buffers allocated, there
* may be other problems.
*
* @note The ring buffer scheme is designed for one primary purpose: optimal acquisition speed.
* Programmers should resist the temptation to increase the number of buffers and use them for
* storage or for processing. Instead use memcpy or equivalent to copy each buffer out after
* the image has been acquired, and do any processing etc. on the copy.
*
* @return 0 on success, -1 on failure. 
*
* @see pdv_buffer_addresses
*
* @ingroup acquisition  
*/

int
pdv_multibuf(PdvDev * pdv_p, int numbufs)
{
    /* set number of buffers, library allocated. */

    /* define if not enough memory for contiguous allocation */
    /* but beware of interlace since currently not supported in separate */
    /* #ifdef RADSTONE*/
#if defined(XCALIBURCOMMON) || defined(VMIC)
    return pdv_multibuf_separate(pdv_p, numbufs,NULL);
#else
    return pdv_multibuf_block(pdv_p, numbufs, NULL, 0);
#endif

}

/**
* Used to set up user-allocated buffers to be used in ring buffer mode, cannot be used
* on systems that have more than 3.5GB/memory (ie the subroutine has been depricated
* for all practical purposes, instead use #pdv_multibuf).
*
* @note  Due to PCI and EDT 32-bit driver architecture limitations, we recommend
* avoiding this subroutine, as it will not work on most systems that have more than 3.5 MB of
* memory. Instead, use #pdv_multibuf to set up ring buffers, and #pdv_buffer_addresses to retrieve
* the list of buffer pointers generated by pdv_multibuf, and copy out to your local buffers if needed.
*
* This function takes an argument that is an array of buffers allocated by the
* user. The memory pointed to by the array must be in the lower 3.5 GB.  Buffers should be page-aligned.
* We recommend using #pdv_alloc which does this in a system-independent way.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open
* @param numbufs number of buffers. Must be 1 or greater. Four is recommended
* for most applications.
* @param bufarray If NULL, the library allocates a set of page-aligned ring
* buffers. If not NULL, this argument is an array of pointers to
* application-allocated buffers; these buffers must match the size returned by
* #pdv_image_size and number of buffers specified in this call and will be used
* as the ring buffers.
*
* @Example
* @code
* int size = pdv_image_size(pdv_p);
* unsigned char *bufarray[4];
*
* for (i=0; i < 4; i++)
*   bufarray[i] = pdv_alloc(size);
*    
* pdv_set_buffers(pdv_p, 4, bufarray);
* @endcode
*
* @return 0 on success, -1 on failure. 
*
* @see pdv_multibuf, pdv_buffer_addresses
*
* @ingroup acquisition 
*/
int
pdv_set_buffers(PdvDev * pdv_p, int numbufs, unsigned char **bufarray)
{
    /* set number of buffers, user allocated. */
    int     size = pdv_get_dmasize(pdv_p);

    edt_msg(DBG2, "pdv_set_buffers(%d %x) (size %d)\n", numbufs, bufarray, size);

    pdv_setup_dma(pdv_p);

    return (edt_configure_ring_buffers(pdv_p, size, numbufs, EDT_READ, bufarray));
}

/* plus size - so far just used for merge */
int
pdv_set_buffers_x(PdvDev * pdv_p, int numbufs, int size, unsigned char **bufs)
{

    edt_msg(DBG2, "pdv_set_buffers(%d %x) (size %d)\n", numbufs, bufs, size);

    pdv_setup_dma(pdv_p);

    return (edt_configure_ring_buffers(pdv_p, size, numbufs, EDT_READ, bufs));
}


/**
* Returns the size of the image buffer in bytes, based on its width,
* height, and depth. Enabling a region of interest changes this value.
* The size returned includes allowance for buffer headers. To obtain
* the actual size of the image data without any optional header or other
* padding, see #pdv_get_dmasize.  
*
* @param pdv_p    device struct returned from pdv_open
* @return Total number of bytes in the image, including buffer header
* overhead.
*
* @see pdv_set_roi
* @ingroup settings
*/
int
pdv_image_size(PdvDev * pdv_p)
{
    int     size;

    size = pdv_p->dd_p->imagesize;

    if (pdv_p->dd_p->slop)
    {

        edt_msg(DBG2, "pdv_image_size: adjusting size %x by slop %x to %x\n",
            size, pdv_p->dd_p->slop, size - pdv_p->dd_p->slop);

        size -= pdv_p->dd_p->slop;

    }

    if (pdv_p->dd_p->header_dma)
        size += pdv_p->dd_p->header_size;

    edt_msg(DBG2, "pdv_image_size() %d\n", size);

    return size;

}


/** 
* Returns a pointer to the last image that was acquired (non-blocking).
* It will return a pointer to the same buffer if called a second time
* with no new images acquired.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open
* 
* @return Address of the last image acquired.
*
* @see pdv_wait_last_image, pdv_wait_last_image_raw, pdv_wait_image, edt_done_count
*
* @ingroup acquisition 
*/
u_char *
pdv_get_last_image(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_get_last_image()\n");

    return (pdv_p->dd_p->last_image);
}

/** get last raw image.
*
* Identical to the #pdv_get_last_image, except that it skips any
* image deinterleave method defined by the \b method_interlace
* config file directive. 
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open
*
* @return Address of the last image acquired.
*
* @see pdv_get_last_image
*
* @ingroup acquisition
*/
u_char *
pdv_get_last_raw(PdvDev * pdv_p)
{
    edt_msg(DBG2, "pdv_get_last_raw()\n");

    return (pdv_p->dd_p->last_raw);
}

/**
* Returns the addresses of the buffers allocated by the last call to
* #pdv_multibuf or #pdv_set_buffers. See #pdv_wait_images for a description and
* example of use.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open
*
* @return An array of pointers to image buffers. The size of the array is equal
* to the number of buffers allocated.
*
* @see pdv_multibuf, pdv_set_buffers
*
* @ingroup acquisition
*/
u_char **
pdv_buffer_addresses(PdvDev * pdv_p)
{
    /* (same as edt_buffer_addresses) */
    return (pdv_p->ring_buffers);
}

/**
* Starts hardware continuous mode.
*
* When hardware continuous mode is enabled, the hardware waits until
* the first acquisition request, and starts reading data when it sees
* the camera's first FRAME VALID signal going TRUE. Subsequent frames
* are read in without regard to the state of FRAME VALID, and LINE VALID
* (and DATA VALID) are depended upon to gate the data.
*
* This functionality is necessary in some cases where the interframe
* gap is too small for the OS/device driver to be able to respond
* for images, typically with very high frame rate cameras.
* The downside to this is that if data is ever dropped as a result of
* bandwidth saturation or an unplugged cable for example, frame synch will be
* forever lost, and cannot be regained without either operator intervention
* or some intelligent image recognition software. Therefore this mode
* should only be used when it is certain that it is needed.
*
* @note Hardware continuous mode can be enabled at init time via the 
* directive \b continuous: \b 1 camera configuration directive. 
* 
* @param pdv_p    device struct returned from pdv_open
* @see pdv_stop_hardware_continuous
* @return void
* @ingroup acquisition
*/
void
pdv_start_hardware_continuous(PdvDev * pdv_p /* , int frames */ )
{
    int     data_path;

    /* data_path = edt_reg_read(pdv_p, PDV_DATA_PATH); */
    data_path = pdv_p->dd_p->datapath_reg & ~PDV_CONTINUOUS;
    /* data_path = 0x5 ; */
    edt_msg(DBG2, "pdv_start_hardware_continuous()\n");


    edt_reg_write(pdv_p, PDV_DATA_PATH, data_path | PDV_CONTINUOUS);
    pdv_p->dd_p->datapath_reg = data_path;
}


/**
* Stops hardware continous mode.
*
* See #pdv_start_hardware_continuous for further description.
*
* @param pdv_p    device struct returned from pdv_open
* @see pdv_start_hardware_continuous
* @return void
* @ingroup acquisition
*/
void
pdv_stop_hardware_continuous(PdvDev * pdv_p)
{
    int     data_path;

    /* data_path = edt_reg_read(pdv_p, PDV_DATA_PATH); */
    data_path = pdv_p->dd_p->datapath_reg;
    edt_msg(DBG2, "pdv_stop_hardware_continuous()\n");
    data_path &= ~PDV_CONTINUOUS;
    edt_reg_write(pdv_p, PDV_DATA_PATH, data_path);
    pdv_p->dd_p->datapath_reg = data_path;

}

/**
* Sets parity to even, odd, or none.
*
* @param parity the desired partity. Should be 'e', 'o', or 'n' for
* even, odd, or none (respectively).
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* 
* @return 0 on success, -1 on error
* @ingroup serial 
*/
int
pdv_set_serial_parity(PdvDev * pdv_p, char parity)
{
    if (pdv_p->devid == PDVAERO_ID)
    {
        int val = 0;
        switch(parity)
        {

        case 'e':
            val = 1;
            break;
        case 'o':
            val = 3;
            break;
        case 'n':
            val = 0;
            break;
        default:
            edt_msg(DBG1, "parity must be e, o, or n");
            return -1;
        }

        edt_reg_write(pdv_p,PDV_SERIAL_PARITY,val);
        return 0;
    }
    else
    {
        edt_msg(DBG1,"parity not an option for this board");
    }
    return -1;
}

/**
* Sets the baud rate on the serial lines; applies only to cameras with serial
* control. Valid values are 9600, 19200, 38500, 57500, and 115200.
*
* @note The baud rate is ordinarily initialized using the value of the \b serial_baud
* directive in the configuration file, and defaults to 9600 if the directive is not
* present. Under most circumstances, applications do not need to set the baud
* rate explicitly.
*
* @param baud the desired baud rate.
* @param pdv_p pointer to pdv device structure returned by #pdv_open

* @return 0 on success, -1 on error
* @ingroup serial 
*/
int
pdv_set_baud(PdvDev * pdv_p, int baud)
{
    Dependent *dd_p = pdv_p->dd_p;
    u_int   baudbits = 0;
    int     id=pdv_p->devid;
    u_int   new, baudreg;
    u_int   cntl;
    int     donew = 0;
    int     ret = 0;

    if (edt_is_dvcl(pdv_p) || edt_is_dvfox(pdv_p) || (id == PDVA_ID) || edt_is_simulator(pdv_p))
        donew = 1;

    switch (baud)
    {
    case 9600:
        baudbits = 0;
        baudreg = 0x80;
        break;

    case 19200:
        baudbits = PDV_BAUD0;
        baudreg = 0x3f;
        break;

    case 38400:
        baudbits = PDV_BAUD1;
        baudreg = 0x1f;
        break;

    case 115200:
        baudbits = PDV_BAUD0 | PDV_BAUD1;
        baudreg = 0x09;
        break;

    case 57600:
        baudbits = PDV_BAUD0 | PDV_BAUD1; /* ALERT (funky old DV or DVK only) */
        donew = 1;
        baudreg = 0x014;
        break;

    default:
        donew = 1;
        baudreg = (unsigned int)(((20000000.0 / (16.0 * (double)baud)) - 2.0) + 0.5) ; 
        edt_msg(DBG2, "pdv_set_baud(%d) using new method, reg %02x\n", baud, baudreg);
        break;
    }

    if (donew && (!dd_p->register_wrap))
    {
        if (baudreg > 0xff)
        {
            edt_msg(DBG2, "pdv_set_baud(%d) (baudreg %x) BAD VALUE not set\n", baud, baudreg);
            ret = -1; 
        }
        else 
        {
            edt_msg(DBG2, "pdv_set_baud(%d) (baudreg %x) using NEW baud reg\n", baud, baudreg);
            edt_reg_write(pdv_p, PDV_BRATE, baudreg);
            new = edt_reg_read(pdv_p, PDV_BRATE);
            if (new != baudreg)
            {
                edt_msg(DBG2, "pdv_set_baud(%d) wrote %x read %x) readback ERROR\n", baud, baudreg, new);
                ret = -1;
            }
        }
    }

    else /* old way */
    {
        edt_msg(DBG2, "pdv_set_baud(%d) (baudbits %x) using OLD baud bits\n", baud, baudbits);
        cntl = edt_reg_read(pdv_p, PDV_SERIAL_DATA_CNTL);
        cntl &= ~PDV_BAUD_MASK;
        cntl |= baudbits;
        edt_reg_write(pdv_p, PDV_SERIAL_DATA_CNTL, cntl);
    }

    if (ret == 0)
    {
        pdv_p->dd_p->serial_baud = baud;
        edt_set_dependent(pdv_p, pdv_p->dd_p);
    }

    return ret;
}

/**
* Get the baud rate, typically initialized by the \b serial_baud directive in
* the config file (default 9600).
* 
* @return baud rate in bits/sec, or 0 on error
* @see \b serial_baud directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
* @ingroup serial 
*/
int
pdv_get_baud(PdvDev * pdv_p)
{
    if (pdv_p->dd_p->serial_baud == NOT_SET)
        return 9600;
    return pdv_p->dd_p->serial_baud;
}

/** 
* Formats and prints a system error.
*
* @if edtinternal
* @warning Obsolete -- EDT library functions should use
* edt_msg_perror(&pdv_err, ...) instead of this function. 
* @endif
*
* Convenience function to format and print a system error. In Linux
* implementations, the routine just turns around and makes a perror system
* call, with the errstr argument. NT implementations format and print the last
* error using GetLastErrorString.
* @return void
*
* @ingroup utility
*/
void
pdv_perror(char *err)
{
    edt_perror(err);
}

void
pdv_setdebug(PdvDev * pdv_p, int debug)
{
    Pdv_debug = debug;
}





/**
* Resets the serial interface.
*
* This is mostly used during initialization (\e initcam) to make sure any outstanding
* reads and writes from previous interrupted applications are cleaned up and to put
* the serial state machine in a known idle state. Applications typically do not
* need to call this subroutine.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return void
*
* @ingroup serial 
*/
void
pdv_reset_serial(PdvDev * pdv_p)
{
    edt_reset_serial(pdv_p);
}


/**
* Convenience routine to allocate memory in a system-independent way. 
*
* The buffer returned is page aligned. Page alignment is required for
* some EDT image routines and always preferred.  This function uses
* VirtualAlloc on Windows NT/2000/XP systems, or valloc on
* Linux/Unix systems.
*
* @Example
* @code
* unsigned char *buf = pdv_alloc(pdv_image_size(pdv_p));
* @endcode
*
* @param size   the number of bytes of memory to allocate
* @return The address of the allocated memory, or NULL on error.  If NULL, use
* #pdv_perror to print the error.
* 
* @see pdv_free
* @ingroup utility 
*/
uchar_t *
pdv_alloc(int size)
{
    return edt_alloc(size);
}


/**
* Convenience routine to free the memory allocated with #pdv_alloc.
*
* @param ptr  Address of memory buffer to free.
* @return void
* @ingroup utility 
*/
void
pdv_free(uchar_t * ptr)
{
    edt_free(ptr);
}



static u_short zero[] = {
    0x0F80,
    0x3FE0,
    0x38E0,
    0x7070,
    0x6030,
    0xE038,
    0xE038,
    0xE038,
    0xE038,
    0xE038,
    0xE038,
    0xE038,
    0x6030,
    0x7070,
    0x38E0,
    0x3FE0,
    0x0F80,
};
static u_short one[] = {
    0xFC00,
    0xFC00,
    0x1C00,
    0x1C00,
    0x1C00,
    0x1C00,
    0x1C00,
    0x1C00,
    0x1C00,
    0x1C00,
    0x1C00,
    0x1C00,
    0x1C00,
    0x1C00,
    0x1C00,
    0xFF80,
    0xFF80,
};
static u_short two[] = {
    0x7E00,
    0x7F80,
    0x4380,
    0x01C0,
    0x01C0,
    0x01C0,
    0x01C0,
    0x0380,
    0x0700,
    0x0E00,
    0x1C00,
    0x3800,
    0x3800,
    0x7000,
    0xE000,
    0xFFC0,
    0xFFC0,
};
static u_short three[] = {
    0x7E00,
    0xFF00,
    0x8780,
    0x0380,
    0x0380,
    0x0380,
    0x0F00,
    0x7C00,
    0x7E00,
    0x0F00,
    0x0380,
    0x0380,
    0x0380,
    0x0380,
    0x8700,
    0xFF00,
    0xFC00,
};
static u_short four[] = {
    0x0380,
    0x0780,
    0x0F80,
    0x0F80,
    0x1F80,
    0x1B80,
    0x3380,
    0x3380,
    0x6380,
    0xE380,
    0xFFE0,
    0xFFE0,
    0xFFE0,
    0x0380,
    0x0380,
    0x0380,
    0x0380,
};
static u_short five[] = {
    0xFF80,
    0xFF80,
    0xE000,
    0xE000,
    0xE000,
    0xC000,
    0xF800,
    0xFE00,
    0x0F00,
    0x0780,
    0x0380,
    0x0380,
    0x0380,
    0x0380,
    0x8700,
    0xFE00,
    0xFC00,
};
static u_short six[] = {
    0x07E0,
    0x1FE0,
    0x3C20,
    0x7000,
    0x7000,
    0xE000,
    0xE780,
    0xFFE0,
    0xF0E0,
    0xE070,
    0xE070,
    0xE070,
    0xE070,
    0x7070,
    0x70E0,
    0x3FC0,
    0x0F80,
};
static u_short seven[] = {
    0xFFC0,
    0xFFC0,
    0x01C0,
    0x0180,
    0x0380,
    0x0700,
    0x0700,
    0x0E00,
    0x0C00,
    0x1C00,
    0x1800,
    0x3800,
    0x3800,
    0x3000,
    0x7000,
    0x7000,
    0x7000,
};
static u_short eight[] = {
    0x0F80,
    0x3FC0,
    0x71E0,
    0x70E0,
    0x70E0,
    0x7DE0,
    0x3FC0,
    0x1F00,
    0x1FC0,
    0x7FE0,
    0xF1F0,
    0xE0F0,
    0xE070,
    0xE070,
    0xF0E0,
    0x7FE0,
    0x1F80,
};
static u_short nine[] = {
    0x1F00,
    0x3FC0,
    0x70E0,
    0xE0E0,
    0xE070,
    0xE070,
    0xE070,
    0xE070,
    0x70F0,
    0x7FF0,
    0x1E70,
    0x0070,
    0x00E0,
    0x00E0,
    0x43C0,
    0x7F80,
    0x7E00,
};

static u_short *digits[] = {
    zero,
    one,
    two,
    three,
    four,
    five,
    six,
    seven,
    eight,
    nine,
};


static void
set_square_32(int sx, int sy, u_short * buf, u_int * addr, int stride, int fg)
{
    /*
    * colors
    */

#define BG16	0

    /*
    * get offset to square
    */
    register u_int *svptr;
    register u_int *ptr;
    int     i;
    int     bit;
    u_short tmp;
    int     val;

    svptr = addr + (stride * sy) + (sx);

    for (i = 0; i < 17; i++)
    {
        ptr = svptr;
        tmp = *buf++;
        for (bit = 15; bit >= 0; bit--)
        {
            if (tmp & (1 << bit))
            {
                val = fg;
            }
            else
            {
                val = BG16;
            }
            *ptr++ = val;
        }
        svptr += stride;
    }
}


static void
set_square_16(int sx, int sy, u_short * buf, u_short * addr, int stride, int fg)
{
    /*
    * colors
    */

#define BG16	0

    /*
    * get offset to square
    */
    register u_short *svptr;
    register u_short *ptr;
    int     i;
    int     bit;
    u_short tmp;
    int     val;

    svptr = addr + (stride * sy) + (sx);

    for (i = 0; i < 17; i++)
    {
        ptr = svptr;
        tmp = *buf++;
        for (bit = 15; bit >= 0; bit--)
        {
            if (tmp & (1 << bit))
            {
                val = fg;
            }
            else
            {
                val = BG16;
            }
            *ptr++ = val;
        }
        svptr += stride;
    }
}

static void
set_square(int sx, int sy, u_short * buf, u_char * addr, int stride, int fg)
{
    /*
    * colors
    */

#define BG	0

    /*
    * get offset to square
    */
    register u_char *svptr;
    register u_char *ptr;
    int     i;
    int     bit;
    u_short tmp;
    int     val;

    svptr = addr + (stride * sy) + (sx);

    for (i = 0; i < 17; i++)
    {
        ptr = svptr;
        tmp = *buf++;
        for (bit = 15; bit >= 0; bit--)
        {
            if (tmp & (1 << bit))
            {
                val = fg;
            }
            else
            {
                val = BG;
            }
            *ptr++ = val;
        }
        svptr += stride;
    }
}

void
pdv_mark_digit_16(u_short * addr, int n, int width, int height, int x, int y,
                  int mask, int fg)

{
    int dig = n % 10;

    set_square_16(x, y, digits[dig], addr, width, fg);

}

void
pdv_mark_digit_32(u_int * addr, int n, int width, int height, int x, int y,
                  int mask, int fg)

{
    int dig = n % 10;

    set_square_32(x, y, digits[dig], addr, width, fg);

}

void
pdv_mark_digit_24(u_char * addr, int n, int width, int height, int x, int y,
                  int mask, int fg)

{
    int dig = n % 10;

    set_square(x, y, digits[dig], addr, width, fg);

}

void
pdv_mark_digit_8(u_char * addr, int n, int width, int height, int x, int y,
                 int mask, int fg)

{
    int dig = n % 10;

    set_square(x, y, digits[dig], addr, width, fg);

}

void
pdv_mark_ras_depth(void * addr, int n, int width, int height, int x, int y, int depth, int fg)
{
#define GAP	16
    int div = 1000000;

    int i;

    for (i=0;div>=1;i++)
    {

        if (n > div)
        {
            int val = (n / div) % 10;

            switch(depth)
            {
            case 8:
                pdv_mark_digit_8((u_char *) addr, val, width, height, x + (GAP * i), y, 1, fg);
                break;
            case 16:
                pdv_mark_digit_16((u_short *) addr, val, width, height, x + (GAP * i), y, 1, fg);
                break;
            case 24:
                pdv_mark_digit_24((u_char *) addr, val, width, height, x + (GAP * i), y, 1, fg);
                break;
            case 32:
                pdv_mark_digit_32((u_int *) addr, val, width, height, x + (GAP * i), y, 1, fg);
                break;
            }
        }

        div /= 10;
    }
}

void
pdv_mark_bin_16(u_short * addr, int n, int width, int height, int x, int y)
{
    u_int   val = n;
    u_char *svptr;

    svptr = (u_char *) addr + (width * y) + x;
    *svptr++ = (val >> 24) & 0xff;
    *svptr++ = (val >> 16) & 0xff;
    *svptr++ = (val >> 8) & 0xff;
    *svptr++ = val & 0xff;
}


void
pdv_mark_ras(u_char * addr, int n, int width, int height, int x, int y)
{
#define GAP	16
    int div = 1000000;
    int i;

    for (i=0;div;i++)
    {
        pdv_mark_digit_8(addr, n, width, height, x + (GAP * i), y, div, 200);
        div /= 10;
    }
}

void
pdv_mark_bin(u_char * addr, int n, int width, int height, int x, int y)
{
    u_int   val = n;
    u_char *svptr;

    svptr = (u_char *) addr + (width * y) + x;
    *svptr++ = (val >> 24) & 0xff;
    *svptr++ = (val >> 16) & 0xff;
    *svptr++ = (val >> 8) & 0xff;
    *svptr++ = val & 0xff;
}

/**
* Send hex byte command (formatted ascii "0xNN") as binary. Assumes the format
* has already been checked.
*
* Not all that useful for user applications, mainly it's here for special use by
* #pdv_initcam.
* 
* @attention length is unused -- here only for future use if/when we want to send more
* than one byte at a time. For now only one byte at a time (and only used by
* initcam really...).
*
* @param pdv_p  pointer to pdv device structure returned by #pdv_open 
* @param str  ASCII command string containing "0x%s" hex formatted string
* @param length  reserved, for future use
* 
* @ingroup serial
*/
int
pdv_serial_command_hex(PdvDev * pdv_p, const char *str, int length)
{
    char    buf[2];
    const char   *p = &str[2];
    u_int   lval;

    sscanf(p, "%x", &lval);
    buf[0] = (char) (lval & 0xff);
    edt_msg(DBG2, "pdv_serial_command_hex(%s),0x%x\n", str, (u_char) (buf[0]));

    return pdv_serial_binary_command(pdv_p, buf, 1);
}

/** Sets a rectangular region of interest, supporting cropping.
* Sets the coordinates of a rectangular region of interest within the
* image. Checks the camera \b width and \b height directives in the
* configuration file and returns an error if the coordinates provided
* are out of range. Use this with #pdv_enable_roi, which enables the
* region of interest.
* 
* Note that hactv + hskip should always be less than or equal to the actual
* output width of the camera, and vact + vskip should be less than or equal to
* the number of output lines.
*
* An initial region of interest can be set from the config file with the
* hactv, hskip, vactv, and vskip directives.
*
* @note Region of Interest may not work with some very old cameras
* which required special bitfiles.  It will work with most DV, DVK, and
* all Camera Link boards (inlcluding DVFOX with RCX C-LINK). 
*
* @param pdv_p  pointer to pdv device structure returned by #pdv_open 
* @param hskip  the X coordinate of the upper left corner of the region of interest.
* @param hactv  the width (number of pixels per line) of the region of interest.
* @param vskip  the Y coordinate of the upper left corner of the region of interest.
* @param vactv  the height (number of lines per frame) of the region of interest.
*
* @Example
* @code
* //use the region of interest calls to cut off a 10 pixel wide
* //border around the image.
* int cam_w = pdv_get_cam_width(pdv_p);
* int cam_h = pdv_get_cam_height(pdv_p);
* int hactv = cam_w - 20
* int vactv = cam_h - 20
* int hskip = 10;
* int vskip = 10;
*
* pdv_set_roi(pdv_p, hskip, hactv, vskip, vactv);
* pdv_enable_roi(pdv_p, 1); 
* @endcode
*
* @return 0 on success, -1 on failure. 
* @see pdv_enable_roi, \b vskip, \b vactv, \b hskip, \b hactv directives in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
* @ingroup settings 
*/

int
pdv_set_roi(PdvDev * pdv_p, int hskip, int hactv, int vskip, int vactv)
{
    return pdv_set_roi_internal(pdv_p, hskip, hactv, vskip, vactv, 1);
}


/**
* @if edtinternal
* Sets ROI with extra intrnal flag to call setsize -- this lets us call it from subroutines
* that call #pdv_setsize and avoid recursion
* @internal 
* User applications should not need to call this -- it will
* be called as needed by library functions like #pdv_setsize.
* @ingroup utility 
* @endif
*/ 
int
pdv_set_roi_internal(PdvDev * pdv_p, int hskip, int hactv, int vskip, int vactv, int call_setsize)
{
    Dependent *dd_p = pdv_p->dd_p;
    int     cam_w, cam_h;	/* camera actual w/h */

    edt_msg(DBG2, "pdv_set_roi(hskip %d hactv %d vskip %d vactv %d)\n",
        hskip, hactv, vskip, vactv);

    cam_w = pdv_get_cam_width(pdv_p);
    cam_h = pdv_get_cam_height(pdv_p);

    /* check width/height for out of range, unless camera link or FOI */
    if (!pdv_is_cameralink(pdv_p))
    {
        if ((cam_w && ((hskip + hactv) > cam_w)) || ((hskip + hactv) <= 0))
        {
            edt_msg(DBG2, "ROI: horiz. skip/actv out of range error\n");
            return -1;
        }
        if ((cam_h && ((vskip + vactv) > cam_h)) || ((vskip + vactv) <= 0))
        {
            edt_msg(DBG2, "ROI: vert. skip/actv out of range error\n");
            return -1;
        }
    }

    /* vert must be even, and regs set to value/taps  */
    if (pdv_is_cameralink(pdv_p))
    {
        if (dd_p->htaps > 0)
        {
            hactv = (hactv / dd_p->htaps) * dd_p->htaps;
            hskip = (hskip / dd_p->htaps) * dd_p->htaps;

            edt_reg_write(pdv_p, PDV_HSKIP, (hskip / dd_p->htaps));
            edt_reg_write(pdv_p, PDV_HACTV, (hactv / dd_p->htaps) - 1);
        }

        if (dd_p->vtaps > 0)
        {
            vactv = (vactv / dd_p->vtaps) * dd_p->vtaps;
            vskip = (vskip / dd_p->vtaps) * dd_p->vtaps;

            edt_reg_write(pdv_p, PDV_VSKIP, (vskip / dd_p->vtaps));
            edt_reg_write(pdv_p, PDV_VACTV, (vactv / dd_p->vtaps) - 1);
        }
    }
    else if (dd_p->dual_channel)
    {
        vactv = (vactv / 2) * 2;
        vskip = (vskip / 2) * 2;
        edt_reg_write(pdv_p, PDV_VSKIP, (vskip / 2));
        edt_reg_write(pdv_p, PDV_HSKIP, hskip);
        edt_reg_write(pdv_p, PDV_VACTV, (vactv / 2) - 1);
        edt_reg_write(pdv_p, PDV_HACTV, hactv - 1);
    }
    else
    {
        edt_reg_write(pdv_p, PDV_HSKIP, hskip);
        edt_reg_write(pdv_p, PDV_VSKIP, vskip);
        edt_reg_write(pdv_p, PDV_HACTV, hactv - 1);
        edt_reg_write(pdv_p, PDV_VACTV, vactv - 1);
    }

    dd_p->hactv = hactv;
    dd_p->hskip = hskip;
    dd_p->vactv = vactv;
    dd_p->vskip = vskip;

    if (call_setsize && dd_p->roi_enabled)
        pdv_setsize(pdv_p, dd_p->hactv, dd_p->vactv);

    edt_set_dependent(pdv_p, dd_p);

    return 0; /* ALERT: need to return error from above if
                        * any */
}


/**
 * Set the number of channels (taps) and horizontal and vertical alignment of the taps.
 * Will set the number of Camera Link taps (channels) in the hardware by setting
 * the left nibble of the PDV_CL_DATA_PATH register, and the htaps and vtaps
 * PdvDev->dd_p structure elements.
 *
 * For single-tap modes, htaps and vtaps should both be 1. For dual or 4-tap modes, most
 * cameras output the data horizontally so htaps would be 2 or 4, and vtaps would remain 1.
 * For RGB cameras (except bayer), htaps is usually 3 and vtaps 1.
 *
 * Typically these are set via initcam or pdv_initcam; look at the various config files' htaps
 * and vtaps directives. If a camera's output tap configuration is changed after after
 * initialization, (usually via a serial command) this command can be used to update
 * the framegrabber's registers to match.
 *
 * @param pdv_p pointer to pdv device structure returned by #pdv_ope
 * @param htaps number of horizontal taps
 * @param htaps number of vertical taps
 * @return void
 * @see pdv_set_depth_extdepth_dpath, \b hskip, \b vskip and \b CL_DATA_PATH_NORM directives in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
 * 
 * @ingroup settings 
 */
void
pdv_cl_set_base_channels(PdvDev *pdv_p, int htaps, int vtaps)

{
    Dependent *dd_p = pdv_p->dd_p;
    int taps;

    if ((htaps > 1) && (vtaps > 1))
        taps = htaps + vtaps;
    else if (htaps > 1)
        taps = htaps;
    else if (vtaps > 1)
        taps = vtaps;
    else taps = 1;

    dd_p->cl_data_path = (taps - 1) << 4 | (dd_p->depth - 1);

    dd_p->htaps = htaps;
    dd_p->vtaps = vtaps;

    edt_reg_write(pdv_p, PDV_CL_DATA_PATH, dd_p->cl_data_path);

    edt_set_dependent(pdv_p, dd_p);

    pdv_set_roi(pdv_p, dd_p->hskip, dd_p->hactv, dd_p->vskip, dd_p->vactv);
}



/*****************************************************/
/* Set the Dalsa line scan using AIAG - hskip and hactv */
/* control the exposure time and the linerate */
/*****************************************************/

int
pdv_dalsa_ls_set_expose(PdvDev * pdv_p, int hskip, int hactv)
{
    Dependent *dd_p = pdv_p->dd_p;

    edt_msg(DBG2, "pdv_dalsa_ls_set_expose(hskip %d hactv %d)\n", hskip, hactv);

    edt_reg_write(pdv_p, PDV_HSKIP, hskip);
    edt_reg_write(pdv_p, PDV_HACTV, hactv - 1);

    dd_p->hactv = hactv;
    dd_p->hskip = hskip;

    edt_set_dependent(pdv_p, dd_p);
    pdv_enable_roi(pdv_p, 1);

    return 0;	/* ALERT: need to return error from above if any */
}


/**
* set ROI to camera width/height; adjust ROI width to be a multiple of 4,
* and enable ROI
* 
* mainly for use starting up with PCI DV C-Link which we want to use ROI in by
* default. But can be used for other stuff.
* @ingroup settings 
*/
int
pdv_auto_set_roi(PdvDev * pdv_p)
{
    int     w = pdv_p->dd_p->width;
    int     h = pdv_p->dd_p->height;
    int     ret;

    edt_msg(DBG2, "pdv_auto_set_roi()\n");

    if ((ret = pdv_set_roi(pdv_p, 0, w, 0, h)) == 0)
    {
        if (pdv_is_cameralink(pdv_p))
            pdv_setsize(pdv_p, pdv_p->dd_p->hactv, pdv_p->dd_p->vactv);
        ret = pdv_enable_roi(pdv_p, 1);
    }

    return ret;
}

/**
* Enables on-board region of interest. The rectangular region of interest
* parameters are set using #pdv_set_roi; this subroutine is used to
* enable/disballe that region. Also calls pdv_setsize so subsequent
* calls to #pdv_get_width or #pdv_get_height return the values after
* region of interest is applied.  Also resizes and reallocates any
* buffers allocated as a result of calling #pdv_multibuf. Returns an
* error if the region of interest values are out of range.
*
* The initial state of the region of interest can be controlled with directives
* in the configuration file.  Most config files provided by EDT have ROI enabled
* by default.  See the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
* for more information.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open
* @param flag nonzero to enable region of interest; 0 to disable it.
* 
* @return 0 on success, -1 on failure. 
* 
* @see pdv_set_roi for an example.
* @ingroup settings 
*/
int
pdv_enable_roi(PdvDev * pdv_p, int flag)
{
    u_int   roictl;
    Dependent *dd_p = pdv_p->dd_p;

    /* NEW 6/20 -- CL NOT always enabled; modified accordingly */
    if (pdv_is_cameralink(pdv_p))
    {
        if (flag) /* enable */
        {
            edt_reg_and(pdv_p, PDV_CL_CFG, ~PDV_CL_CFG_ROIDIS);
            pdv_setsize(pdv_p, dd_p->hactv, dd_p->vactv);
        }
        else /* disable */
        {
            edt_reg_or(pdv_p, PDV_CL_CFG, PDV_CL_CFG_ROIDIS);
            pdv_setsize(pdv_p, dd_p->cam_width, dd_p->cam_height);
        }

        dd_p->roi_enabled = flag;

        return 0;
    }

    edt_msg(DBG2, "pdv_enable_roi(%d): %sabling\n", flag, flag ? "EN" : "DIS");

    /* edt_msleep(100); */

    roictl = edt_reg_read(pdv_p, PDV_ROICTL);


    if (flag)			/* enable ROI */
    {
        roictl |= PDV_ROICTL_ROI_EN;
        edt_reg_write(pdv_p, PDV_ROICTL, roictl);
        pdv_setsize(pdv_p, dd_p->hactv, dd_p->vactv);
    }
    else			/* disable ROI */
    {
        /* ALERT -- not a R/W reg so can't and/or bits */
        roictl &= ~PDV_ROICTL_ROI_EN;
        edt_reg_write(pdv_p, PDV_ROICTL, roictl);
        pdv_setsize(pdv_p, dd_p->cam_width, dd_p->cam_height);
    }

    dd_p->roi_enabled = flag;

    return 0;
}

int
pdv_get_roi_enabled(PdvDev *pdv_p)

{
    Dependent *dd_p = pdv_p->dd_p;

    if (pdv_is_cameralink(pdv_p))
    {
        dd_p->roi_enabled = (edt_reg_read(pdv_p, PDV_CL_CFG) & 0x08) != 0;
    }
    else
    {
        dd_p->roi_enabled = (edt_reg_read(pdv_p, PDV_ROICTL) & PDV_ROICTL_ROI_EN) != 0;
    }

    return dd_p->roi_enabled;
}

/**
* Determines file access independent of operating system. This a convenience
* routine that maps to \c access() on Unix/Linux systems, and \c _access on Windows
* systems.
*
* @param fname path name of the file to check access of.
* @param perm  permission flag(s) to test for. See \c access() (Unix/Linux) or
* \c _access (Windows) for valid arguments.
*
* @Example
* @code 
* if (pdv_access("file.ras", F_OK))
*   print("Warning: about to overwrite file %s/n", "file.ras");
* @endcode
*
* @return 0 on success, -1 on failure. 
*
* @ingroup utility
*/
int
pdv_access(char *fname, int perm)
{
    return edt_access(fname, perm);
}


/**
* Fires the strobe. This is a specialized routine that only works with a camera
* that has a strobe cable connected to an EG&G or Perkin_Elmer strobe with EDT
* strobe trigger circuitry including DAC level, and custom aia_strobe.bit
* XILINX downloaded.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param count    number of strobe pulses. 
* @param delay    number of msecs between pulses, as well as before the first
* and after the last pulse.  Actual delay between flashes is 100us * (delay +
* 1). High 4 bits of count is ignored so maximum count is 4095.
* 
* @Example
* @code
* // fire the strobe 10 times, with a
* // 101 millisecond delay between pulses
* pdv_strobe(pdv_p, 10, 100);      
* @endcode
*
* @return 0 on success, -1 on failure
*
* @ingroup edt_undoc 
*/
int
pdv_strobe(PdvDev * pdv_p, int count, int delay)
{
    if (pdv_strobe_method(pdv_p) != PDV_LHS_METHOD1)
        return -1;

    edt_msg(DBG2, "pdv_strobe(%d %d)\n", count, delay);

    edt_reg_write(pdv_p, PDV_SHUTTER, delay);

    /*
    * write to LSB loads the low byte, write to MSB loads high byte and also
    * fires the strobe
    */
    edt_reg_write(pdv_p, PDV_FIXEDLEN, count);

    return 0;
}

/**
* pdv_set_strobe_counters.
* 
* NEW method (method2)  -- so far only for c-link but will probably be
* folded back into pdv/pdvk eventually. Only works with new strobe
* xilinx. check pdv_strobe_method for PDV_LHS_METHOD2.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param count   the number of strobe pulses. range 0-4095
* @param delay   the # of msecs before the first and after the last pulse
*                actual interval before the first pulse will be delay+period
*                range 0-255
* @param period  delay (msecs) between pulses. range 0-255
* 
* 
* Returns 0 on success, -1 on failure
*
* @see pdv_enable_strobe, pdv_set_strobe_dac, pdv_strobe_method
* @ingroup edt_undoc 
*/
int
pdv_set_strobe_counters(PdvDev * pdv_p, int count, int delay, int period)
{
    if (pdv_strobe_method(pdv_p) != PDV_LHS_METHOD2)
        return -1;

    if (count > 0xfff)
    {
        edt_msg(DBG1, "pdv_set_strobe_counters() ERROR -- count out of range\n");
        return -1;
    }

    if (delay > 0xff)
    {
        edt_msg(PDVWARN, "pdv_set_strobe_counters() ERROR -- delay out of range\n");
        return -1;
    }

    if (period > 0xff)
    {
        edt_msg(PDVWARN, "pdv_set_strobe_counters() ERROR -- period out of range\n");
        return -1;
    }

    edt_msg(DBG2, "pdv_set_strobe_counters(%d %d %d)\n", count, delay, period);

    edt_reg_write(pdv_p, PDV_LHS_DELAY, delay);

    edt_reg_write(pdv_p, PDV_LHS_PERIOD, period);

    /*
    * write to the count (2 bytes, combined)
    */
    edt_reg_write(pdv_p, PDV_LHS_COUNT, count);

    return 0;
}

/**
* enable/disable lh strobe.
*
* if method 2, enable or disable as specified
* if method 1, return ok if enable specified, -1 if disable,
* since we can't do that with method 1
*
* counters and DAC value will persist whether strobe is enabled or not
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param ena  1 to enable, 0 to disable
*/
int
pdv_enable_strobe(PdvDev * pdv_p, int ena)
{
    int method = pdv_strobe_method(pdv_p);

    if (method == 0)
        return -1;

    if (method == PDV_LHS_METHOD1)
    {
        if (ena == 0)
            return -1;
        return 0;
    }

    if (ena)
        edt_reg_write(pdv_p, PDV_LHS_CONTROL, PDV_LHS_ENABLE);
    else edt_reg_write(pdv_p, PDV_LHS_CONTROL, 0);

    return 0;
}

/**
* check if the strobe is even valid for this FPGA, and which
* method is used.
*
* @return  
*     0 (not implemenented)
*     PDV_LHS_METHOD1 (original method)
*     PDV_LHS_METHOD2 (new method)
* @ingroup edt_undoc 
*/
int
pdv_strobe_method(PdvDev *pdv_p)
{
    Dependent *dd_p = pdv_p->dd_p;
    int status;

    if (dd_p->strobe_enabled == NOT_SET)
    {
        int reg ;

        /* default to not enabled */
        dd_p->strobe_enabled = 0;


        /* ALERT: figure out somehow whether LH strobe flash is loaded
        * but for now just return lhs2 */
        if (pdv_p->devid == PDVCL_ID)
        {
            dd_p->strobe_enabled = PDV_LHS_METHOD2;
            return dd_p->strobe_enabled;
        }

        /* old xilinx revs didn't have strobe, so just bail out here */
        if (dd_p->xilinx_rev < 2 || dd_p->xilinx_rev > 32)
            return 0;

        /*
        * see if new LHS registers are there (can be written/read)
        */
        if (!dd_p->register_wrap)
        {
            reg = edt_reg_read(pdv_p, PDV_LHS_COUNT_HI);
            edt_reg_write(pdv_p, PDV_LHS_COUNT_HI, 0x50);
            if (edt_reg_read(pdv_p, PDV_LHS_COUNT_HI) == 0x50)
            {
                edt_reg_write(pdv_p, PDV_LHS_COUNT_HI, reg);
                dd_p->strobe_enabled = PDV_LHS_METHOD2;
                return dd_p->strobe_enabled;
            }
        }

        /* still here? okay check for (infer) OLD strobe method */

        status = edt_reg_read(pdv_p, PDV_SERIAL_DATA_STAT);

        if (status & LHS_DONE)
            dd_p->strobe_enabled = PDV_LHS_METHOD1;

    }

    return dd_p->strobe_enabled;
}


/**
* Sets the strobe DAC level. This is a specialized routine that only works with
* a camera that has a strobe cable connected to an EG&G strobe with EDT strobe
* trigger circuitry including DAC level, and custom aia_strobe.bit XILINX
* downloaded.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param value  DAC voltage level. Valid values are 0-4095.
* 
* @return 0 on success, -1 on failure. 
*
* @see pdv_strobe, strobe.c example program
* @ingroup edt_undoc 
*
* @todo remove old code?
* set strobe DAC by clocking in the pixels. 
* Only applies if XILINX with strobe is loaded (aiags.bit)
* or special LH c-link strobe.
*/
int
pdv_set_strobe_dac(PdvDev * pdv_p, u_int value)
{
    int     i;
    int     reg;
    int     method;
    u_int   mcl, mask;
    u_char  data;

    if ((method = pdv_strobe_method(pdv_p)) == 0)
        return -1;

    if (method == PDV_LHS_METHOD2)
        reg = PDV_LHS_CONTROL;
    else reg = PDV_MODE_CNTL;

    edt_msg(DBG2, "pdv_strobe(%d)\n", value);

    mcl = edt_reg_read(pdv_p, reg) & 0x0f;	/* preserve low bits */

    /*
    * dac is high nibble of register with strobe xilinx first we clear, then
    * set low bit
    */
    edt_reg_write(pdv_p, reg, mcl | PDV_LHS_DAC_LOAD);

    /*
    * shift in 16 bits
    */
    for (i = 0; i < 16; i++)
    {
        mask = (value & (1 << (15 - i)));

        if (mask)
            data = PDV_LHS_DAC_DATA;
        else
            data = 0;

        edt_reg_write(pdv_p, reg, mcl | PDV_LHS_DAC_LOAD | data);
        edt_reg_write(pdv_p, reg, mcl | PDV_LHS_DAC_LOAD | PDV_LHS_DAC_CLOCK | data);
        edt_reg_write(pdv_p, reg, mcl | PDV_LHS_DAC_LOAD | data);
        edt_reg_write(pdv_p, reg, mcl | PDV_LHS_DAC_LOAD);
    }

    edt_reg_write(pdv_p, reg, mcl &~ PDV_LHS_DAC_LOAD);
    return 0;
}

/** @addtogroup acquisition
* @{
*/

/** 
* OBSOLETE: just use pdv_flush_fifo(pdv_p) now.
*/
void
pdv_flush_channel_fifo(PdvDev * pdv_p)
{

    /* now obsolete channel reset is fixed for FOX 
    jsc 3/6/06 */

    pdv_flush_fifo(pdv_p);
}

/**
* Flushes the board's input FIFOs, to allow new data transfers to start
* from a known state. 
*
* This subroutine effectively resets the board, so calling it after every
* image will degrade performance and is not recommended. Additionally, resetting
* after a timeout, involves more than just flushing the FIFOs -- therefore we
* recommend using #pdv_timeout_restart to reset (which calls this, among
* other things).
*
* @param pdv_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
 * @return void
*/
void
pdv_flush_fifo(PdvDev * pdv_p)
{
    /* previously in comment: @see edt_set_firstflush */
    if (pdv_p->devid == PDV_ID ||
        pdv_p->devid == PDVK_ID ||
        pdv_p->devid == PDVFOI_ID ||
        pdv_p->devid == PDVA_ID)
    {
        edt_msg(DBG2, "pdv_flush_fifo() [PCI]\n");
        edt_flush_fifo(pdv_p);
        return;
    }

    if (pdv_p->devid == PDVCL_ID   || 
        pdv_p->devid == PE1DVVL_ID ||
        pdv_p->devid == PE4DVVL_ID ||
        pdv_p->devid == PE4DVCL_ID ||
        pdv_p->devid == PE8DVCL_ID ||
        pdv_p->devid == PE8VLCLS_ID ||
        pdv_p->devid == PE8DVCLS_ID)
    {
        /* RFIFO doesnt exist on camera link */
        /* doing the reset intfc here may cause problems */
        /* possibly when it happens in same frame as grab - TODO - check */
        u_int cfg ;

        edt_msg(DBG2, "pdv_flush_fifo() [cameralink]\n");
        cfg = edt_intfc_read(pdv_p, PDV_CFG);
        cfg &= ~PDV_FIFO_RESET;
        edt_intfc_write(pdv_p, PDV_CMD, PDV_RESET_INTFC);
        edt_intfc_write(pdv_p, PDV_CFG, (u_char) (cfg | PDV_FIFO_RESET));
        edt_intfc_write(pdv_p, PDV_CFG, cfg);
        edt_flush_channel(pdv_p, pdv_p->channel_no);

    }
    else
    {
        /* DV-FOX, PCIe C-Link */

        u_int cfg ;

        edt_msg(DBG2, "pdv_flush_fifo() [DVFOX, PCIe4/8]");
        cfg = edt_intfc_read(pdv_p, PDV_CFG);
        cfg &= ~PDV_FIFO_RESET;
        edt_intfc_write(pdv_p, PDV_CFG, (u_char) (cfg | PDV_FIFO_RESET));
        edt_flush_channel(pdv_p, pdv_p->channel_no);
        edt_intfc_write(pdv_p, PDV_CFG, cfg);
    }
}

/**
*  Obsolete. See #pdv_setup_continuous.
*/
void
pdv_setup_continuous_channel(PdvDev * pdv_p)

{
    /* now obsolete channel reset is fixed for FOX 
    jsc 3/6/06 */
    pdv_setup_continuous(pdv_p);
}

/**
* Performs setup for continuous transfers.
*
* Shouldn't need to be called by user apps since #pdv_start_images, etc.
* call it already. But it is in some EDT example applications from before
* this was the case.
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @see pdv_stop_continuous
* @return void
*
*/
void
pdv_setup_continuous(PdvDev * pdv_p)
{
    pdv_flush_fifo(pdv_p);

    if (pdv_p->devid == PDV_ID ||
        pdv_p->devid == PDVK_ID ||
        pdv_p->devid == PDVA_ID)
    {

        if (edt_get_firstflush(pdv_p) != EDT_ACT_KBS)
            edt_set_firstflush(pdv_p, EDT_ACT_ONCE);
    }
    else
    {
        edt_set_firstflush(pdv_p,EDT_ACT_NEVER) ;
        edt_set_autodir(pdv_p, 0) ;

    }

    edt_startdma_reg(pdv_p, PDV_CMD, PDV_ENABLE_GRAB);

    if (pdv_in_continuous(pdv_p))
    {
        /* pdv_start_hardware_continuous(pdv_p) ; */
        edt_set_continuous(pdv_p, 1);
    }
    else if (pdv_p->dd_p->fv_once)
    {
        pdv_start_hardware_continuous(pdv_p);
    }
    else
        edt_set_continuous(pdv_p, 0);

    pdv_p->dd_p->started_continuous = 1;

}

/**
* Performs un-setup for continuous transfers.
*
* Shouldn't need to be called by user apps since other subroutines 
* (e.g. #pdv_timeout_cleanup) now call it as needed. But it is still in
* some EDT example applications from before this was the case.
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @see pdv_setup_continuous
* @return void
*/
void
pdv_stop_continuous(PdvDev * pdv_p)
{

    if (pdv_in_continuous(pdv_p))
    {
        edt_set_continuous(pdv_p, 0);
    }
    else if (pdv_p->dd_p->fv_once)
        pdv_stop_hardware_continuous(pdv_p); 

    pdv_p->dd_p->started_continuous = 0;


}

/**
* Cleans up after a timeout, particularly when you've prestarted multiple
* buffers or if you've forced a timeout with #edt_do_timeout. The example
* programs take.c and simple_take.c have examples of use; note that these
* examples call #pdv_timeout_restart twice, which may be overkill for some
* applications/cameras.  If the system is configured properly (and all cables/
* cameras have robust connections), timeouts should not be a factor. Even so,
* a robust app will handle timeouts gracefully so it is a good idea to experiment
* with various timeout recovery methods to make sure you have something that
* works for your situation.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param restart whether to immediately restart acquiring.
*
* @return # of buffers left undone; normally will be used as argument to
* #pdv_start_images if calling routine wants to restart pipeline as if
* nothing happened (see take.c and simple_take.c for examples of use).
* @see pdv_timeouts
*/

int
pdv_timeout_restart(PdvDev * pdv_p, int restart)
{
    int     curdone, curtodo;

    curdone = edt_done_count(pdv_p);
    curtodo = edt_get_todo(pdv_p);

    edt_abort_dma(pdv_p);
    pdv_stop_continuous(pdv_p);

    edt_set_buffer(pdv_p, curdone);
    edt_reg_write(pdv_p, PDV_CMD, PDV_RESET_INTFC);
    pdv_setup_continuous(pdv_p);

    if (restart && 
        (curtodo - curdone))
        pdv_start_images(pdv_p, curtodo - curdone);

    return curtodo - curdone;
}

/** @} */ /* end acquisition */

/* for debug printfs only */
static char hs[128];
static char *
hex_to_str(char *resp, int n)
{

    int     i;
    char   *p = hs;

    for (i = 0; i < n; i++)
    {
        sprintf(p, "%02x ", resp[i]);
        p += 3;
    }
    *p = '\0';
    return hs;
}


/**
* Gets the minimum allowable exposure value for this camera, as set by
* \e initcam from the camera_config file \b shutter_speed_min directive.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
*
* @return Minimum exposure value.
* @see \b shutter_speed_min directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
* @ingroup settings 
*/
int
pdv_get_min_shutter(PdvDev * pdv_p)

{
    return pdv_p->dd_p->shutter_speed_min;
}


/**
* Gets the maximum allowable exposure value for this camera, as set by
* \e initcam from the camera_config file \b shutter_speed_max directive.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return Maximum exposure value.
* @ingroup settings 
*/
int
pdv_get_max_shutter(PdvDev * pdv_p)

{
    return pdv_p->dd_p->shutter_speed_max;
}


/**
* Gets the minimum allowable gain value for this camera, as set by \e initcam
* from the camera configuration file \b gain_min directive.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return Minimum gain value.
* @see \b gain directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
* @ingroup settings 
*/
int
pdv_get_min_gain(PdvDev * pdv_p)

{
    return pdv_p->dd_p->gain_min;
}


/**
* Gets the maximum allowable gain value for this camera, as set by \e initcam
* from the camera configuration file \b gain_max directive.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return Maximum gain value.
* @see \b gain directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
* @ingroup settings 
*/
int
pdv_get_max_gain(PdvDev * pdv_p)

{
    return pdv_p->dd_p->gain_max;
}


/**
* Gets the minimum allowable offset (black level) value for this camera, as set
* by \e initcam from the camera configuration file \b offset_min directive.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return Minimum offset value.
* @see \b offset directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
* @ingroup settings 
*/
int
pdv_get_min_offset(PdvDev * pdv_p)

{
    return pdv_p->dd_p->offset_min;
}


/**
* Gets the maximum allowable offset (black level) value for this camera, as set
* by \e initcam from the camera configuration file \b offset_max directive.
*
* @param pdv_p    device struct returned from pdv_open
* @return maximum offset value
* @see \b offset directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
* @ingroup settings 
*/
int
pdv_get_max_offset(PdvDev * pdv_p)

{
    return pdv_p->dd_p->offset_max;
}

/**
* Convenience routine to enable/disable shutter lock on/off on certain cameras.
* Obsolete routine, if camera can lock the shutter (currently only a few
* old Kodak Megaplus cameras) then just do it with #pdv_serial_command.
* @ingroup settings 
*/
int
pdv_enable_lock(PdvDev * pdv_p, int flag)
{
    Dependent *dd_p = pdv_p->dd_p;
    int     ret;

    switch (dd_p->lock_shutter)
    {
    case KODAK_AIA_MCL:
        {
            int     mcl = edt_reg_read(pdv_p, PDV_MODE_CNTL);

            if (flag)
                mcl |= PDV_AIA_MC3;
            else
                mcl &= ~PDV_AIA_MC3;

            edt_reg_write(pdv_p, PDV_MODE_CNTL, mcl);
            ret = 0;
            break;
        }
    case KODAK_AIA_SER:
        {
            if (flag)
                ret = pdv_serial_command(pdv_p, "SHE OF");
            else
                ret = pdv_serial_command(pdv_p, "SHE ON");

            break;
        }
    case KODAK_SER_14I:
        {
            if (flag)
                ret = pdv_serial_command(pdv_p, "SHE FO");
            else
                ret = pdv_serial_command(pdv_p, "SHE ON");
            break;
        }
    case HAM_4880:
        {
            if (flag)
                ret = pdv_serial_command(pdv_p, "ASH O");
            else
                ret = pdv_serial_command(pdv_p, "ASH A");
            break;
        }
    }
    return (ret);
}


/**
* Send serial break (only Camera Link, and aiag and related FPGA files).
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return void
* @ingroup serial  
*/
void
pdv_send_break(PdvDev * pdv_p)
{
    u_int   reg;

    edt_msg(DBG2, "pdv_send_break()");

    reg = edt_reg_read(pdv_p, PDV_UTIL2);
    edt_reg_write(pdv_p, PDV_UTIL2, reg & ~PDV_MC4);
    edt_reg_write(pdv_p, PDV_UTIL2, (reg | PDV_SEL_MC4) & ~PDV_MC4);
    edt_msleep(500);
    edt_reg_write(pdv_p, PDV_UTIL2, reg & ~PDV_SEL_MC4 & ~PDV_MC4);
    edt_reg_write(pdv_p, PDV_UTIL2, reg);
}

static void
pdv_trigger_specinst(PdvDev * pdv_p)
{
    char    cmd = pdv_p->dd_p->serial_trigger[0];

    edt_msg(DBG2, "pdv_trigger_specinst('%c')\n", cmd);

    pdv_serial_binary_command(pdv_p, &cmd, 1);

}

/**
* wait for response from spectral instruments camera trigger
*/
static void
pdv_posttrigger_specinst(PdvDev * pdv_p)
{
    char    cmd = pdv_p->dd_p->serial_trigger[0];
    char    resp[32];
    int     ret;
    int     waitcnt = 2;

    resp[0] = resp[1] = resp[2] = 0;

#ifdef SPECINST_WAS
    if (pdv_p->devid == PDVFOI_ID)
        waitcnt += 2;
    if ((ret = pdv_serial_wait(pdv_p, 1500, waitcnt)) == waitcnt)
        pdv_serial_read(pdv_p, resp, ret);
#else
    if (pdv_p->devid == PDVFOI_ID)
        pdv_serial_wait_next(pdv_p, 2000, 0);
    else
        pdv_serial_wait_next(pdv_p, 2000, 1);
    ret = pdv_serial_read(pdv_p, resp, 2);
#endif

    if ((ret != waitcnt)
        || (resp[0] != pdv_p->dd_p->serial_trigger[0])
        || (resp[1] != pdv_p->dd_p->serial_response[0]))
    {
        edt_msg(DBG2, "\npdv_posttrigger_specinst: invalid/missing serial?\n");
        edt_msg(DBG2, "response (sent %c, ret %d s/b %d, resp <%s>)\n", cmd, ret, waitcnt, (ret > 0) ? resp : "");
        return;
    }
}

static int
pdv_specinst_serial_triggered(PdvDev * pdv_p)
{
    if (((pdv_p->dd_p->camera_shutter_timing == SPECINST_SERIAL)
        || (pdv_p->dd_p->camera_shutter_speed == SPECINST_SERIAL))
        && (pdv_p->dd_p->serial_trigger[0]))
        return 1;
    return 0;
}

int
pdv_pause_for_serial(PdvDev * pdv_p)
{
    return pdv_p->dd_p->pause_for_serial;
}


static int
isafloat(char *str)
{
    unsigned int i;
    int     numdots = 0;
    int     numchars = 0;

    for (i = 0; i < strlen(str); i++)
    {
        if (str[i] == '.')
            ++numdots;
        else if (isdigit(str[i]))
            ++numchars;
        else
            return 0;
    }

    if (numdots == 1 && numchars > 0)
        return 1;
    return 0;
}

static int
isdigits(char *str)
{
    unsigned int i;
    int     numchars = 0;

    for (i = 0; i < strlen(str); i++)
    {
        if (isdigit(str[i]))
            ++numchars;
        else if ((str[i] == '-') && (i == 0))
            ;
        else
            return 0;
    }

    if (numchars > 0)
        return 1;
    return 0;
}

static int
isxdigits(char *str)
{
    unsigned int i;
    int     numchars = 0;

    for (i = 0; i < strlen(str); i++)
    {
        if (isxdigit(str[i]))
            ++numchars;
        else
            return 0;
    }

    if (numchars > 0)
        return 1;
    return 0;
}

/**
* Infer if it's a Redlake (formerly Roper, formerly Kodak) 'i' camera from
* the serial settings. Since serial commands have changed quite a bit over
* the years, this subroutine should not be depended on and is only included
* for backwards compatability.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return 1 if pdv_p appears setup for Redlake, 0 otherwise.
* @ingroup utility 
*/
int
pdv_is_kodak_i(PdvDev * pdv_p)
{
    Dependent *dd_p = pdv_p->dd_p;

    if ((strcmp(dd_p->serial_exposure, "EXE") == 0)
        || (strcmp(dd_p->serial_gain, "DGN") == 0)
        || (strcmp(dd_p->serial_offset, "GAE") == 0)
        || (strcmp(dd_p->serial_offset, "BKE") == 0))
        return 1;
    return 0;
}


/**
* Infers that this device is connected to is an Atmel camera,
* based on the camera_class directive.
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return 1 if device pdv_p has been setup for Atmel camera, 0
* otherwise.
*
* @see pdv_get_camera_class
* @ingroup utility 
*/
int
pdv_is_atmel(PdvDev * pdv_p)
{
    Dependent *dd_p = pdv_p->dd_p;

    if ((strncasecmp(dd_p->camera_class, "Atmel", 5) == 0)
        || (strncmp(dd_p->serial_exposure, "I=", 2) == 0))
        return 1;
    return 0;
}

/**
* Infers that this device is connected to is a Hamamatsu camera based on the
* camera class string.
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return 1 if device pdv_p has been setup for Hamamatsu camera, else 0.
* @ingroup utility 
*/
int
pdv_is_hamamatsu(PdvDev * pdv_p)
{
    Dependent *dd_p = pdv_p->dd_p;

    if ((strncasecmp(dd_p->camera_class, "Hamamatsu", 9) == 0))
        return 1;
    return 0;
}


/**
* Deprecated -- Queries certain specific cameras via serial, and sets library variables for gain,
* black level, exposure time and binning to values based on the results of the query.
* Included for backwards compatability only. The cameras supported are all older (pre-2000) cameras
* made by Kodak Megaplus 'i', Hamamatsu, DVC, and Atmel.
* 
* This subroutine will be removed in a future release and should not be used.
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return 0 on success, -1 on error (including if the camera is not one that is supported by this subroutine)
* @ingroup utility 
*/
int
pdv_update_values_from_camera(PdvDev * pdv_p)
{
    int     ret = 0;

    if (pdv_is_kodak_i(pdv_p))
        ret = pdv_update_from_kodak_i(pdv_p);
    else if (pdv_is_dvc(pdv_p))
        ret = pdv_update_from_dvc(pdv_p);
    else if (pdv_is_atmel(pdv_p))
        ret = pdv_update_from_atmel(pdv_p);
    else if (pdv_is_hamamatsu(pdv_p))
        ret = pdv_update_from_hamamatsu(pdv_p);
    /* add more here */
    else
        ret = -1;

    return ret;
}

/**
* Deprecated -- Queries Kodak 'i' model cameras via serial, and sets library variables for gain,
* black level, exposure time and binning based on the results of the query.
* Included for backwards compatability only.  This subroutine will be removed in a future release
* and should not be used.
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return 0 on success, -1 on error
* @ingroup utility 
*/
static int
pdv_update_from_kodak_i(PdvDev * pdv_p)
{
    int     i, n, ret = 0;
    char   *stat[64];
    char  **stat_p = stat;
    Dependent *dd_p = pdv_p->dd_p;

    for (i = 0; i < 64; i++)
    {
        *stat_p = (char *) malloc(64 * sizeof(char));
        **stat_p = '\0';
        ++stat_p;
    }

    if ((n = pdv_query_serial(pdv_p, "STS?", stat)) < 1)
        ret = -1;
    else
    {
        update_int_from_serial(stat, n, "BKE", &dd_p->level);
        update_int_from_serial(stat, n, "GAE", &dd_p->gain);
        update_int_from_serial(stat, n, "DGN", &dd_p->gain);
        update_int_from_serial(stat, n, "EXE", &dd_p->shutter_speed);
        update_int_from_serial(stat, n, "BNS", &dd_p->binx);
        update_int_from_serial(stat, n, "BNS", &dd_p->biny);
    }

    for (i = 0; i < 64; i++)
        free(stat[i]);

    return ret;
}

/**
* Deprecated -- Queries some Hamamatsu cameras via serial, and sets library variables for gain,
* black level, and exposure time based on the results of the query.
* Included for backwards compatability only.  This subroutine will be removed in a future release
* and should not be used.
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return 0 on success, -1 on error
* @ingroup internal 
*/
static int
pdv_update_from_hamamatsu(PdvDev * pdv_p)
{
    int     i, n, ret = 0;
    char   *stat[64];
    char  **stat_p = stat;
    Dependent *dd_p = pdv_p->dd_p;

    for (i = 0; i < 64; i++)
    {
        *stat_p = (char *) malloc(64 * sizeof(char));
        **stat_p = '\0';
        ++stat_p;
    }

    if ((n = pdv_query_serial(pdv_p, "?CEG", stat)) < 1)
        ret = -1;
    else
        update_int_from_serial(stat, n, "CEG", &dd_p->gain);

    if ((n = pdv_query_serial(pdv_p, "?CEO", stat)) < 1)
        ret = -1;
    else
        update_int_from_serial(stat, n, "CEO", &dd_p->level);

    if ((n = pdv_query_serial(pdv_p, "?SHT", stat)) < 1)
        ret = -1;
    else
        update_int_from_serial(stat, n, "SHT", &dd_p->shutter_speed);

    for (i = 0; i < 64; i++)
        free(stat[i]);

    return ret;
}


/**
* Deprecated -- Queries some Atmel cameras via serial, and sets library variables for gain,
* exposure time and binning based on the results of the query.
* Included for backwards compatability only.  This subroutine will be removed in a future release
* and should not be used.
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return 0 on success, -1 on error
* @ingroup internal 
*/
static int
pdv_update_from_atmel(PdvDev * pdv_p)
{
    int     i, n, ret = 0;
    int     tmpval;
    char   *stat[64];
    char  **stat_p = stat;
    Dependent *dd_p = pdv_p->dd_p;

    for (i = 0; i < 64; i++)
    {
        *stat_p = (char *) malloc(64 * sizeof(char));
        **stat_p = '\0';
        ++stat_p;
    }

    if ((n = pdv_query_serial(pdv_p, "!=3", stat)) < 1)
        ret = -1;
    else
    {
        update_int_from_serial(stat, n, "G", &dd_p->gain);
        update_int_from_serial(stat, n, "I", &dd_p->shutter_speed);
        if (update_int_from_serial(stat, n, "B", &tmpval) == 0)
            dd_p->binx = dd_p->biny = tmpval + 1;
    }

    for (i = 0; i < 64; i++)
        free(stat[i]);

    return ret;
}

/**
* Send a serial command, get the response in a multiline string, one line per
* string pointer. 
* @return the number of strings found. Max return string length is 2048
* @ingroup serial
*/
int
pdv_query_serial(PdvDev * pdv_p, char *cmd, char **resp)
{

    char   *buf_p;
    char    buf[2048];
    int     length;
    int     ret;
    int     i, j, l;
    int     nfound = 0;

    {
        char    *tmp_storage;
        if ((tmp_storage = (char *)malloc(strlen(cmd)+1)) == NULL)
            return 0;
        sprintf(tmp_storage, "%s\r", cmd);
        edt_msg(DBG2, "pdv_query_serial: writing <%s>\n", cmd);
        pdv_serial_command(pdv_p, tmp_storage);
        free(tmp_storage);
    }
    /*
    * serial_timeout comes from the config file (or -t override flag in this
    * app), or if not present defaults to 500
    */
    pdv_serial_wait(pdv_p, pdv_p->dd_p->serial_timeout, 64);

    /*
    * get the return string. How its printed out depends on whether its 1)
    * ASCII, 2) HEX, or 3) Pulnix STX/ETX format
    */
    buf_p = buf;
    length = 0;
    do
    {
        ret = pdv_serial_read(pdv_p, buf_p, 2048 - length);
        edt_msg(DBG2, "read returned %d\n", ret);

        if (ret != 0)
        {
            buf_p[ret + 1] = 0;
            buf_p += ret;
            length += ret;
        }
        pdv_serial_wait(pdv_p, 500, 64);
    } while (ret > 0);

    /*
    * copy the buffer to the string list
    */
    i = 0;
    j = 0;
    buf_p = buf;
    for (l = 0; l < length; l++)
    {
        if ((*buf_p == '\n') || (*buf_p == '\r'))
        {
            if (j != 0)
            {
                resp[i++][j] = '\0';
                j = 0;
            }
        }
        else
        {
            if (j == 0)
                ++nfound;
            resp[i][j++] = *buf_p;
            resp[i][j] = '\0';
        }

        ++buf_p;
    }


    return nfound;
}

/**
* update an int or float value based on a serial string.
* @ingroup utility 
*/
static int
update_int_from_serial(char **stat, int nstats, char *cmd, int *value)
{
    int     i, ret = -1;
    int     len = (int) strlen(cmd);
    double  tmpf;
    char   *arg;

    /*
    * expected format: "CMD NN or CMD -NN or CMD F.F or CMD=N"
    */
    for (i = 0; i < nstats; i++)
    {
        arg = &stat[i][len + 1];
        if ((strncmp(stat[i], cmd, len) == 0)
            && ((stat[i][len] == ' ') || (stat[i][len] == '=')))
        {
            if (isdigits(arg))
            {
                *value = atoi(arg);
                ret = 0;
            }
            /* special case kodak 'i' decimal EXE */
            else if ((isafloat(arg) && strncmp(cmd, "EXE", 3) == 0))
            {
                tmpf = atof(arg);
                *value = (int) (tmpf / 0.063);
                ret = 0;
            }
        }
    }
    return ret;
}

/**
* update an int or float value based on a serial string.
* @ingroup utility 
*/
static int
update_string_from_serial(char **stat, int nstats, char *cmd, char *value, int maxlen)
{
    int     i, ret = -1;
    int     len = (int) strlen(cmd);
    char   *arg;

    /*
    * expected format: "CMD NN or CMD -NN or CMD F.F or CMD=N"
    */
    for (i = 0; i < nstats; i++)
    {
        arg = &stat[i][len + 1];
        if ((strncmp(stat[i], cmd, len) == 0)
            && ((stat[i][len] == ' ') || (stat[i][len] == '=')))
        {
            strncpy(value, stat[i] + len + 1, maxlen);
        }
    }
    return ret;
}

/**
* update a NxN bin value based on a serial string.
* @ingroup utility 
*/
static void
update_2dig_from_serial(char **stat, int nstats, char *cmd, int *val1, int *val2)
{
    int     i;
    int     len = (int) strlen(cmd);
    char   *arg;

    /*
    * expected format: "CMD XY"
    */
    for (i = 0; i < nstats; i++)
    {
        arg = &stat[i][len + 1];
        if ((strncmp(stat[i], cmd, len) == 0)
            && (stat[i][len] == ' '))
        {
            if (isdigits(arg) && strlen(arg) == 2)
            {
                *val1 = arg[0] - '0';
                *val2 = arg[1] - '0';
            }
        }
    }
}

/**
* update a hex value based on a serial string.
* @ingroup utility 
*/
static void
update_hex_from_serial(char **stat, int nstats, char *cmd, int *value)
{
    int     i;
    size_t     len = strlen(cmd);
    char   *arg;

    /*
    * expected format: "CMD XX"
    */
    for (i = 0; i < nstats; i++)
    {
        arg = &stat[i][len + 1];
        if ((strncmp(stat[i], cmd, len) == 0)
            && (stat[i][len] == ' '))
        {
            if (isxdigits(arg))
            {
                *value = strtol(arg, NULL, 16);
            }
        }
    }
}


/** @addtogroup edt_undoc
* @{
*/

/**
* This method is obsolete and should not be used.
* The current implementation creates a warning message and returns
* -1 signifying failure.
*
* @return -1 for failure, always.
*/
int 
pdv_set_mode(PdvDev * pdv_p, char *mode, int mcl)
{
    edt_msg(PDVWARN, "pdv_set_aperture is OBSOLETE\n");
    return -1;
}


int 
pdv_set_mode_atmel(PdvDev * pdv_p, char *mode)
{
    Dependent *dd_p = pdv_p->dd_p;
    int     cfg = 0;

    if ((strcmp(mode, "T=0") == 0)	/* CONTINUOUS */
        || (strcmp(mode, "T=1") == 0))
    {
        if (strcmp(mode, "T=0") == 0)
        {
            dd_p->shutter_speed_min = 0;
            dd_p->shutter_speed_max = 0;
        }
        else
        {
            dd_p->shutter_speed_min = 1;
            dd_p->shutter_speed_max = 2000;
        }
        dd_p->camera_shutter_timing = AIA_SERIAL;
        dd_p->trig_pulse = 1;	/* probably N/A but its in the cfg so
                                * whatever */
        dd_p->mode_cntl_norm = 0x0;
        edt_reg_write(pdv_p, PDV_MODE_CNTL, dd_p->mode_cntl_norm);
        cfg = edt_reg_read(pdv_p, PDV_CFG) | PDV_TRIG | PDV_INV_SHUTTER;
        edt_reg_write(pdv_p, PDV_CFG, cfg);
        pdv_serial_command(pdv_p, mode);
        strcpy(dd_p->serial_exposure, "I=");
    }
    else if (strcmp(mode, "T=2") == 0)	/* TRIGGERED */
    {
        dd_p->timeout_multiplier = 1;
        dd_p->shutter_speed_min = 1;
        dd_p->shutter_speed_max = 0x414;
        dd_p->camera_shutter_timing = AIA_SERIAL;
        dd_p->trig_pulse = 1;
        dd_p->mode_cntl_norm = 0x10;
        edt_reg_write(pdv_p, PDV_MODE_CNTL, dd_p->mode_cntl_norm);
        cfg = edt_reg_read(pdv_p, PDV_CFG) | PDV_TRIG;
        edt_reg_write(pdv_p, PDV_CFG, cfg);
        pdv_serial_command(pdv_p, mode);
        strcpy(dd_p->serial_exposure, "I=");
    }
    else if (strcmp(mode, "T=3") == 0)	/* CONTOLLED (camelia linescan only?) */
    {
        dd_p->timeout_multiplier = 1;
        dd_p->shutter_speed_min = 1;
        dd_p->shutter_speed_max = 25500;
        dd_p->camera_shutter_timing = AIA_MCL;
        dd_p->trig_pulse = 0;
        dd_p->mode_cntl_norm = 0x10;
        edt_reg_write(pdv_p, PDV_MODE_CNTL, dd_p->mode_cntl_norm);
        cfg = edt_reg_read(pdv_p, PDV_CFG) & ~PDV_TRIG;
        edt_reg_write(pdv_p, PDV_CFG, cfg);
        pdv_serial_command(pdv_p, mode);
        dd_p->serial_exposure[0] = '\0';
    }
    else			/* UNKNOWN */
    {
        edt_msg(PDVWARN, "WARNING: unsupported ATMEL mode: %s\n", mode);
        return -1;
    }
    return 0;
}

/**
* Obsolete
*/
int 
pdv_set_mode_kodak(PdvDev * pdv_p, char *mode)
{
    Dependent *dd_p = pdv_p->dd_p;
    char    cmd[32];
    int     cfg = 0;

    if (strcmp(mode, "CS") == 0)/* CONTINUOUS */
    {
        dd_p->shutter_speed_min = 1;
        dd_p->shutter_speed_max = 512;
        dd_p->camera_shutter_timing = AIA_SERIAL;
        dd_p->trig_pulse = 1;	/* probably N/A but its in the cfg so
                                * whatever */
        dd_p->mode_cntl_norm = 0x0;
        edt_reg_write(pdv_p, PDV_MODE_CNTL, dd_p->mode_cntl_norm);
        cfg = edt_reg_read(pdv_p, PDV_CFG) | PDV_TRIG | PDV_INV_SHUTTER;
        edt_reg_write(pdv_p, PDV_CFG, cfg);
        sprintf(cmd, "MDE %s", mode);
        pdv_serial_command(pdv_p, cmd);
        strcpy(dd_p->serial_exposure, "EXE");
    }
    else if (strcmp(mode, "TR") == 0)	/* TRIGGERED */
    {
        dd_p->shutter_speed_min = 1;
        dd_p->shutter_speed_max = 255;
        dd_p->camera_shutter_timing = AIA_SERIAL;
        dd_p->trig_pulse = 1;
        dd_p->mode_cntl_norm = 0x10;
        edt_reg_write(pdv_p, PDV_MODE_CNTL, dd_p->mode_cntl_norm);
        cfg = edt_reg_read(pdv_p, PDV_CFG) | PDV_TRIG;
        edt_reg_write(pdv_p, PDV_CFG, cfg);
        sprintf(cmd, "MDE %s", mode);
        pdv_serial_command(pdv_p, cmd);
        strcpy(dd_p->serial_exposure, "EXE");
    }
    else if (strcmp(mode, "CD") == 0)	/* CONTROLLED */
    {
        dd_p->shutter_speed_min = 1;
        dd_p->shutter_speed_max = 25500;
        dd_p->camera_shutter_timing = AIA_MCL;
        dd_p->trig_pulse = 0;
        dd_p->mode_cntl_norm = 0x10;
        edt_reg_write(pdv_p, PDV_MODE_CNTL, dd_p->mode_cntl_norm);
        cfg = edt_reg_read(pdv_p, PDV_CFG) & ~PDV_TRIG;
        edt_reg_write(pdv_p, PDV_CFG, cfg);
        sprintf(cmd, "MDE %s", mode);
        pdv_serial_command(pdv_p, cmd);
        dd_p->serial_exposure[0] = '\0';
    }
    else			/* PDI, PDP...? */
    {
        edt_msg(PDVWARN, "WARNING: unsupported DVC mode: %s\n", mode);
        return -1;
    }
    return 0;
}

int 
pdv_set_mode_hamamatsu(PdvDev * pdv_p, char *mode)
{
#if 0
    Dependent *dd_p = pdv_p->dd_p;
    int     cfg = 0;
    int     cont = 0;
#endif

    /* stub */

    return 0;

}

/** @} */ /* end settings */


/*************************************************************
* DVC Camera Functions
*************************************************************/

/**
* Infers that this device is connected to is a DVC camera,
* from settings from the loaded camera config file. 
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @return 1 if device pdv_p has been setup for DVC camera, else 0.
* @ingroup utility 
*/
int
pdv_is_dvc(PdvDev * pdv_p)
{
    Dependent *dd_p = pdv_p->dd_p;

    if ((strncasecmp(dd_p->camera_class, "DVC", 3) == 0)
        || (strncmp(dd_p->serial_exposure, "EXP", 3) == 0))
        return 1;
    return 0;
}

/**
* DVC 1312 binning.
* 
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param xval horizontal binning value
* @param yval vertical binning value
* @ingroup settings 
*/
int
pdv_set_binning_dvc(PdvDev * pdv_p, int xval, int yval)
{
    int     ret = 0;
    char    cmdstr[64];
    Dependent *dd_p = pdv_p->dd_p;

    sprintf(cmdstr, "%s %d%d", dd_p->serial_binning, yval, xval);

    pdv_serial_command(pdv_p, cmdstr);

    return (ret);
}

int
pdv_update_from_dvc(PdvDev * pdv_p)
{
    int     i, n, ret = 0;
    char   *stat[64];
    char  **stat_p = stat;
    Dependent *dd_p = pdv_p->dd_p;

    for (i = 0; i < 64; i++)
    {
        *stat_p = (char *) malloc(64 * sizeof(char));
        **stat_p = '\0';
        ++stat_p;
    }

    if ((n = pdv_query_serial(pdv_p, "STA", stat)) < 1)
        ret = -1;
    else
    {
        update_hex_from_serial(stat, n, "OFS", &dd_p->level);
        update_hex_from_serial(stat, n, "GAI", &dd_p->gain);
        update_hex_from_serial(stat, n, "EXP", &dd_p->shutter_speed);
        update_2dig_from_serial(stat, n, "BIN", &dd_p->binx, &dd_p->biny);
    }

    for (i = 0; i < 64; i++)
        free(stat[i]);

    return ret;
}


int 
pdv_set_mode_dvc(PdvDev * pdv_p, char *mode)
{
    Dependent *dd_p = pdv_p->dd_p;
    char    cmd[32];
    int     cfg = 0;

    if ((strcmp(mode, "NOR") == 0)	/* CONTINUOUS */
        || (strcmp(mode, "ULT") == 0)
        || (strcmp(mode, "HNL") == 0)
        || (strcmp(mode, "HDL") == 0)
        || (strcmp(mode, "NFR") == 0)
        || (strcmp(mode, "NRR") == 0))
    {
        if (mode[0] == 'H')	/* one of the high-speed modes */
        {
            dd_p->shutter_speed_min = 1;
            dd_p->shutter_speed_max = 0x414;
        }
        else
        {
            if (strcmp(mode, "ULT") == 0)	/* ultra long term */
                dd_p->timeout_multiplier = 10;
            else if (strcmp(mode, "NFR") == 0)	/* frame time (87 msec) */
                dd_p->timeout_multiplier = 5;
            else
                dd_p->timeout_multiplier = 1;

            dd_p->shutter_speed_min = 1;
            dd_p->shutter_speed_max = 0x7ff;
        }
        dd_p->camera_shutter_timing = AIA_SERIAL;
        dd_p->trig_pulse = 1;	/* probably N/A but its in the cfg so
                                * whatever */
        dd_p->mode_cntl_norm = 0x0;
        edt_reg_write(pdv_p, PDV_MODE_CNTL, dd_p->mode_cntl_norm);
        cfg = edt_reg_read(pdv_p, PDV_CFG) | PDV_TRIG | PDV_INV_SHUTTER;
        edt_reg_write(pdv_p, PDV_CFG, cfg);
        sprintf(cmd, "MDE %s", mode);
        pdv_serial_command(pdv_p, cmd);
        strcpy(dd_p->serial_exposure, "EXP");
    }
    else if (strcmp(mode, "HDO") == 0)	/* TRIGGERED */
    {
        dd_p->timeout_multiplier = 1;
        dd_p->shutter_speed_min = 1;
        dd_p->shutter_speed_max = 0x414;
        dd_p->camera_shutter_timing = AIA_SERIAL;
        dd_p->trig_pulse = 1;
        dd_p->mode_cntl_norm = 0x10;
        edt_reg_write(pdv_p, PDV_MODE_CNTL, dd_p->mode_cntl_norm);
        cfg = edt_reg_read(pdv_p, PDV_CFG) | PDV_TRIG;
        edt_reg_write(pdv_p, PDV_CFG, cfg);
        sprintf(cmd, "MDE %s", mode);
        pdv_serial_command(pdv_p, cmd);
        strcpy(dd_p->serial_exposure, "EXP");
    }
    else if (strcmp(mode, "PDX") == 0)	/* CONTROLLED */
    {
        dd_p->timeout_multiplier = 1;
        dd_p->shutter_speed_min = 1;
        dd_p->shutter_speed_max = 25500;
        dd_p->camera_shutter_timing = AIA_MCL;
        dd_p->trig_pulse = 0;
        dd_p->mode_cntl_norm = 0x10;
        edt_reg_write(pdv_p, PDV_MODE_CNTL, dd_p->mode_cntl_norm);
        cfg = edt_reg_read(pdv_p, PDV_CFG) & ~PDV_TRIG;
        edt_reg_write(pdv_p, PDV_CFG, cfg);
        sprintf(cmd, "MDE %s", mode);
        pdv_serial_command(pdv_p, cmd);
        dd_p->serial_exposure[0] = '\0';
    }
    else			/* PDI, PDP...? */
    {
        edt_msg(PDVWARN, "WARNING: unsupported DVC mode: %s\n", mode);
        return -1;
    }
    return 0;
}

int
pdv_get_dvc_state(PdvDev * pdv_p, DVCState * pState)

{
    int     i, n, ret = 0;
    char   *stat[64];
    char  **stat_p = stat;
    Dependent *dd_p = pdv_p->dd_p;

    for (i = 0; i < 64; i++)
    {
        *stat_p = (char *) malloc(64 * sizeof(char));
        **stat_p = '\0';
        ++stat_p;
    }

    if ((n = pdv_query_serial(pdv_p, "STA", stat)) < 1)
        ret = -1;
    else
    {
        update_hex_from_serial(stat, n, "OFS", &dd_p->level);
        update_hex_from_serial(stat, n, "GAI", &dd_p->gain);
        update_hex_from_serial(stat, n, "EXP", &dd_p->shutter_speed);
        update_2dig_from_serial(stat, n, "BIN", &dd_p->binx, &dd_p->biny);
    }

    pState->binx = dd_p->binx;
    pState->biny = dd_p->biny;
    pState->blackoffset = dd_p->level;
    pState->exposure = dd_p->shutter_speed;
    pState->gain = dd_p->gain;

    update_string_from_serial(stat, n, "MDE", pState->mode, 3);

    for (i = 0; i < 64; i++)
        free(stat[i]);

    return ret;


}


/**
* Enables external triggering.
*
* One of several methods for external triggering. Calling this subroutine will
* enable the board's external trigger logic. When enabled via this subroutine,
* the hardware will queue any acquisition request made via #pdv_start_image or
* similar subroutine, but will not service the request (that is, trigger the
* camera) until it sees a transition on the external trigger line coming in to
* the optical trigger pins (TTL level) on the board.  If the camera is in
* freerun mode this of course won't have any effect.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param flag	one of --  
* \arg 0 = turn off trigger
* \arg 1 = turn on photo trigger
* \arg 2 = turn on field ID trigger (through camera or cable). Does not apply
* to PCI C-Link.
* @return void
* @ingroup settings 
*/
void
pdv_enable_external_trigger(PdvDev * pdv_p, int flag)
{
    int     mask = PDV_PHOTO_TRIGGER | PDV_FLDID_TRIGGER;
    int     util2 = edt_reg_read(pdv_p, PDV_UTIL2);
    int     bit = 0;

    if (flag & 1)
        bit &= PDV_PHOTO_TRIGGER;
    if (flag & 2)
        bit &= PDV_FLDID_TRIGGER;

    if (flag)
        edt_reg_write(pdv_p, PDV_UTIL2, util2 | flag);
    else
        edt_reg_write(pdv_p, PDV_UTIL2, util2 & ~mask);
}



/** Start expose independent of grab - only works in continuous mode 
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @ingroup acquisition 
* @return void
*/

void 
pdv_start_expose(PdvDev * pdv_p)

{
    edt_reg_write(pdv_p, PDV_CMD, PDV_EXP_STROBE);
}


/**
* Set the Frame Valid interrupt to occur on the rising instead of
* falling edge of frame valid.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @ingroup settings 
* @return void
*/
void 
pdv_invert_fval_interrupt(PdvDev * pdv_p)

{

    edt_reg_or(pdv_p, PDV_UTIL3, PDV_FV_INT_INVERT);
}

/**
* Set the frame period counter and enable/disable frame timing.
*
* Enables either continuous frame pulses at a specified interval, or
* extending the frame valid signal by the specified amount, to in-
* effect extend the amount of time after a frame comes in from the
* camera before the next trigger is issued. This can be used 
* to hold off on issuing subsequent triggers for cameras that require
* an extra delay between triggers, or to set a specific trigger interval.
* Only applies when the camera is in triggered or pulse-width mode
* and the board is controlling the timing.
*
* The camera config file directives \b frame_period and \b method_frame_timing
* (which pretty much always go together) are typically used to initialize
* these values at initcam time for cameras that need a fixed frame delay
* for reliable operation in a given mode (very rare). Frame timing functionality
* is disabled by default.
* 
* @note See the Triggering section in your EDT framegrabber's Users Guide, and also the
* <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
* for more on camera triggering methods.
*
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @param period	frame period in microseconds-2, range 0-16777215
* @param method  one of:
*          \arg  0                 -- disable frame counter
*          \arg  PDV_FMRATE_ENABLE -- continuous frame counter
*          \arg  PDV_FVAL_ADJUST   -- frame counter extends every frame valid
*                                    by 'period' microseconds
* @return -1 on error, 0 on success
* @ingroup settings 
* @see pdv_get_frame_period
*/
int
pdv_set_frame_period(PdvDev *pdv_p, int period, int method)
{
    u_int util3 = edt_reg_read(pdv_p, PDV_UTIL3);

    if (pdv_p->dd_p->register_wrap)
        return -1;

    edt_reg_write(pdv_p, PDV_FRAME_PERIOD0, period & 0xff);
    edt_reg_write(pdv_p, PDV_FRAME_PERIOD1, (period >> 8) & 0xff);
    edt_reg_write(pdv_p, PDV_FRAME_PERIOD2, (period >> 16) & 0xff);

    if (method == 0)
        edt_reg_write(pdv_p, PDV_UTIL3, util3 & ~(PDV_FRENA | PDV_FVADJ));
    else if (method == PDV_FMRATE_ENABLE)
        edt_reg_write(pdv_p, PDV_UTIL3, (util3 | PDV_FRENA) & ~PDV_FVADJ);
    else if (method == PDV_FVAL_ADJUST)
        edt_reg_write(pdv_p, PDV_UTIL3, (util3 | PDV_FVADJ) & ~PDV_FRENA);

    return 0;
}

/**
* Get the frame period
*
* Returns the frame period, for boards that support the frame delay
* / frame period functionality. Frame_period is typically initialized via
* the \b frame_period configuration file directive (which pretty much
* always goes along with the \b method_frame_timing directive). \b frame_period
* is an integer value that determines either the number of microseconds between
* the start of one frame and the next, or the continuous frame trigger interval,
* depending on the state of the \b frame_timing. A more complete description of
* frame interval and frame timing can be found in #pdv_set_frame_period.
*
* @param pdv_p    device handle returned by pdv_open
* @return period  the frame period (microsecond units)
* @see pdv_set_frame_period, \b frame_period directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
*
* @ingroup settings 
*/
int
pdv_get_frame_period(PdvDev *pdv_p)
{
    int period;

    period = edt_reg_read(pdv_p, PDV_FRAME_PERIOD0);
    period |= (edt_reg_read(pdv_p, PDV_FRAME_PERIOD1) & 0xff) << 8;
    period |= (edt_reg_read(pdv_p, PDV_FRAME_PERIOD2) & 0xff) << 16;

    return period;
}

/**
* Serial send AND recieve -- send a command and wait for the response.
*
* Takes both expected receive count and char on which to terminate the
* receive -- if both are specified will return on first one -- that is
* if there's a count of 4 but the 3rd char back is the one specified in
* wchar, then will return after 3. 
*
* @param pdv_p    device handle returned by pdv_open
* @param txbuf    buffer to send out
* @param txcount  number of characters to send out
* @param rxbuf    buffer to hold response
* @param rxcount  number of characters expected back
* @param timeout  number of milliseconds to wait for expected response
* @param wchar    pointer to terminating char (NULL if none)
* @return void
*
* @ingroup serial 
*/
void
pdv_serial_txrx(PdvDev * pdv_p, char *txbuf, int txcount, char *rxbuf, int rxcount, int timeout, u_char *wchar)
{
    int     n, ena;
    u_char  savec;
    int     respflag = 0; /* default to response */
    char    dmybuf[1024];

    if (rxcount == 0)
        respflag = SCFLAG_NORESP;

    ena = pdv_get_waitchar(pdv_p, &savec);

    /*
    * flush any pending junk
    */
    if ((n = pdv_serial_wait(pdv_p, 1, 1024)) != 0)
        pdv_serial_read(pdv_p, dmybuf, n);

    if (*wchar)	
        pdv_set_waitchar(pdv_p, 1, *wchar);

    pdv_serial_binary_command_flagged(pdv_p, txbuf, txcount, respflag);

    if ((n = pdv_serial_wait(pdv_p, timeout, rxcount)) != 0)
        pdv_serial_read(pdv_p, rxbuf, n);

    /* set waitchar back to what it was */
    pdv_set_waitchar(pdv_p, ena, savec);
}


/**
* Infers that this device is connected to is a Camera Link camera
* (as opposed to RS-422 or LVDS parallel), based on settings
* from the loaded camera config file.  Generally useful only for
* applications that may use both Camera Link and the (older)
* AIA cameras, and that need to differentiate between the two.
* Specifically for framegrabbers, will return false (0) for simulators.
*
* @param pdv_p    device handle returned by pdv_open
*
* @return 1 if Camera Link framegrabber, 0 otherwise
*
* @ingroup utility 
*/
int
pdv_is_cameralink(PdvDev *pdv_p)
{
    if (edt_is_dvcl(pdv_p))
        return 1;

    /* Other camera link boards have the a PCI xilinx
    * ID that edt_is_dvcl() recognizes, but FOX doesn't.
    * The FOX units can be either camera link or not 
    * (depending on what they're hooked up to). So
    * when initcam sees camera link directives in the camera
    * .cfg file, it will set the pdv->dd_p->cameralink flag 
    * so we know this is connected to camera link:
    */
    if (pdv_p->dd_p->cameralink)
        return 1;

    return 0;
}

/**
* Infers that this device is a simulator -- either a PCI DV CLS
* board, or a PCIe DV C-Link with simulator FPGA loaded.
*
* @param pdv_p    device handle returned by pdv_open
*
* @return 1 if a simulator, 0 otherwise
*
* @ingroup utility 
*/
int
pdv_is_simulator(PdvDev *pdv_p)
{
    char xilinx_name[256];

    edt_flash_get_fname_auto(pdv_p, xilinx_name);

    /* CL2 (CLS) */
    if (edt_is_simulator(pdv_p))
        return 1;

    /* Legacy: old PCIe4 used the same ID whether it was loaded with sim FPGA or
    * not, so we check the xilinx name." Not any more, and  but nevertheless
    * keep this and the ID thing will win out if that's what we use.
    */
    if (pdv_p->devid == PE4DVCL_ID)
    {
        if ((strncmp(xilinx_name, "pedvclsim", 9) == 0)
            || (strncmp(xilinx_name, "pe4dvcls", 8) == 0)) /* never called it that but who knows... */
            return 1;
    }

    return 0;
}


/**
* Enables frame valid done functionality on the board.
* 
* In most area scan and many line scan applications, the number of
* lines in the image is known beforehand. EDT boards start reading
* pixels in when FRAME VALID signal goes TRUE, but as an optimization
* measure, they ignore the frame valid FALSE and instead return when
* the expected number of pixels have been read in.
*
* However when the number of lines is not known beforehand (for example
* in a mail scanner with a sensor that detects the start/end of packages)
* it becomes necessary to enable image termination on the Frame Valid.
* This subroutine enables that functionality.
*
* When using this, the number of lines (\b height directive in the
* configuration file) should be equal to or greater than the largest
* possible number of lines that will be read in and the ring buffers
* should be big enough to accomodate the largest possible image.
* Otherwise, frames will be split across separate buffers.
* 
* use #pdv_get_lines_xferred after the acquisition returns to find out
* how many lines actually transferred.
*
* @note If the <b>fval_done: 1</b>  directive is present in the
* configuration file (preferred), this subroutine to be called with enable=1
* during initialization and it will not be necessary to call it from
* an application.
*
* @return void
* @ingroup acquisition 
* @see pdv_get_lines_xferred, \b fval_done directive in the <a href="http://edt.com/downloads/camera-configuration-guide.pdf">Camera Configuration Guide</A>
*/
void
pdv_set_fval_done(PdvDev *pdv_p, int enable)

{
    u_int i = (enable != 0);

    edt_ioctl(pdv_p, EDTS_FVAL_DONE, &i);

    if (enable != PDV_TRIGINT)
    {
        edt_reg_and(pdv_p, PDV_UTIL3, ~PDV_TRIGINT);
    }

    pdv_p->dd_p->fval_done = enable;
    edt_set_dependent(pdv_p, pdv_p->dd_p);

}

int
pdv_get_fval_done(PdvDev *pdv_p)

{
    return pdv_p->dd_p->fval_done;

}


/**
* Gets the number of lines transferred during the last acquire.
* Typically only used in line scan applications where the actual number
* of lines transferred into a given buffer is unknown at the time of
* the acquire (see also \b fval_done config file directive.)
* an interrupt (such as from an external sensor) that tells
* the device to stop acquiring before a full buffer has been
* read in. Note that if acquires are continuously being
* queued (as in \c pdv_start_images(pdv_p, n) where \e n is
* greater than 1), the number of lines tranferred may not reflect
* the last finished acquire.
*
* @return number of lines transferred on the last acquire
* @ingroup acquisition 
* @see pdv_get_width_xferred
*/
int
pdv_get_lines_xferred(PdvDev *pdv_p)

{
    u_int rasterlines = 0;

    if (pdv_p->ring_buffer_numbufs)
        rasterlines  = (pdv_p->donecount-1) % pdv_p->ring_buffer_numbufs;

    edt_ioctl(pdv_p, EDTG_LINES_XFERRED, &rasterlines);

    return (rasterlines & 0xffff);

}

/**
* Gets the number of pixels transferred during the last line transferred.
* Typically only used in line scan applications where the actual number
* of pixels transferred per line may not be known
* (see also \b fval_done config file directive.)
* an interrupt (such as from an external sensor) that tells
* the device to stop acquiring before a full buffer has been
* read in. Note that if lines are continuously being
* transferred (the normal case), the number of pixels tranferred may
* not reflect the last finished line.
*
* @return number of pixels transferred on the last line
* @ingroup acquisition 
* @see pdv_get_lines_xferred
*/
int
pdv_get_width_xferred(PdvDev *pdv_p)

{
    u_int rasterlines = (pdv_p->donecount-1) % pdv_p->ring_buffer_numbufs;

    edt_ioctl(pdv_p, EDTG_LINES_XFERRED, &rasterlines);

    return ((rasterlines >> 16) & 0xffff);

}

/**
* Gets the number of frame valid transitions that have been seen by
* the board since the last time the board/channel was initialized or
* the last time #pdv_cl_reset_fv_counter was called.
* The number returned is NOT the number of frames read in; the counter
* on the board counts all frame ticks seen whether images are being read
* in or not. As such this subroutine can be useful in determining whether
* a camera is connected (among other things), assuming that the camera
* is a freerun camera or has a continuous external trigger.
*
* @note This subroutine only works on EDT Camera Link boards.
*
* @return number of frame valids seen
* @ingroup acquisition 
* @see pdv_reset_fv_counter
*/
int
pdv_cl_get_fv_counter(PdvDev *pdv_p)

{
    return edt_reg_read(pdv_p, PDV_CL_FRMCNT);
}


/**
* Resets the frame valid counter to zero.
*
* @note This subroutine only works on EDT Camera Link boards.
* @param pdv_p pointer to pdv device structure returned by #pdv_open 
* @see pdv_get_fv_counter
* @return void
*
* @ingroup acquisition 
*/
void
pdv_cl_reset_fv_counter(PdvDev *pdv_p)

{
    edt_reg_write(pdv_p, PDV_CL_FRMCNT_RESET, 1);
    edt_reg_write(pdv_p, PDV_CL_FRMCNT_RESET, 0);
}

/**
* Checks whether a camera is connected and turned on.
*
* Looks for an active (changing) pixel clock from the camera,
* and returns 1 if detected.
*
* @note This subroutine only works on EDT Camera Link boards, and
* only those that have base mode firmware (pdvcamlk or pdvcamlk2
* 11/02/2006 (rev 34) or later.
*
* @return 1 if active pixel clock is detected, 0 if not detected
* OR not supported (not camera link). Will also return 0 if
* firmware does not support this feature (see above).
*
* @ingroup utility 
*/
int
pdv_cl_camera_connected(PdvDev *pdv_p)

{
    int i, reg, ppl=0, changed=0;

    if (!pdv_is_cameralink(pdv_p))
        return 0;

    reg = edt_reg_read(pdv_p, PDV_UTILITY);
    /* WAS: edt_reg_or(pdv_p, PDV_UTILITY, PDVCL_PCLVFREE); */
    /* but that bit was overloaded as of some iteration of PCIe8 rev08, so as of rev09.... */
    edt_reg_or(pdv_p, PDV_CL_CFG2, PDV_CL_CFG2_PCLVFREE);
    for (i=0; i<5; i++)
    {
        int p;

        if ((p = edt_reg_read(pdv_p, PDV_CL_PIXELSPERLINE)) != ppl)
            ++changed;
        ppl = p;
        edt_msleep(1);
    }

    edt_reg_write(pdv_p, PDV_UTILITY, reg); /* restore */

    if (changed < 2)
        return 0;
    return 1;
}

/** @internal */
void
pdv_new_debug(int val)
{
    int level ;
    Pdv_debug = val ;
    level = edt_msg_default_level();
    if (Pdv_debug > 0)
    {
        level |= DBG1;
        level |= DBG2;
    }
    edt_msg_set_level(edt_msg_default_handle(), level);
}

#ifdef DOXYGEN_SHOW_UNDOC
/* @} */ /* end weak dv group */
#endif


int
pdv_get_rawio_size(PdvDev *pdv_p)

{
    int size = pdv_get_dmasize(pdv_p);

    if (size % 512)
        size = (((size/512)+1)*512);

    return size;

}

/*
 * for #pdv_initcam and pdv_cls_set_dep only
 */
static void
dep_wait(pdv_p)
{
}

void
pdv_check_fpga_rev(PdvDev *pdv_p)
{
    PdvDependent *dd_p = pdv_p->dd_p;

    if (dd_p->xilinx_rev == NOT_SET)
    {
        int     rev;

        edt_msg(DEBUG2, "checking for new rev xilinx\n");
        dep_wait(pdv_p);

        pdv_flush_fifo(pdv_p); /* ADDED  3/14/2012 RWH */

        edt_reg_write(pdv_p, PDV_STAT, 0xff);
        rev = edt_reg_read(pdv_p, PDV_REV);
        if (rev >= 1 && rev <= 32)
        {
            edt_msg(DEBUG2, "xilinx rev set to %d (0x%x)\n", rev, rev);
            dd_p->xilinx_rev = rev;
        }
        else
        {
            dd_p->xilinx_rev = 0;
            edt_msg(DEBUG2, "no xilinx rev from IOCTL, setting to 0\n");
        }

#ifdef NOT_DONE
        /* xilinx rev 2 we got option flag register */
        if ((dd_p->xilinx_rev >= 2) && (dd_p->xilinx_rev != 0xff))
            dd_p->xilinx_opt = edt_reg_read(pdv_p, PDV_XILINX_OPT);
#endif
        dd_p->register_wrap = check_register_wrap(pdv_p);
    }
}


/*
* this code checks for register wrap. Since newer xilinxs have larger
* register space, there's a possibility that we can read/write registers
* but they're actually wrapping by 32. So read/write the low register
* and check if it wraps to high. Uses MASK register and writes back
* when done
*
* RETURNS 1 if wrap, 0 if not
*/
int
check_register_wrap(EdtDev *pdv_p)
{
    int wrapped = 0;
    int r;
    int mask_lo, mask_lo_wrap;

    /* definately not in and OLD xilinx.... */
    if (pdv_p->dd_p->xilinx_rev < 2 || pdv_p->dd_p->xilinx_rev > 32)
        return 1;

    /* made it this far; check for wrap */
    mask_lo = edt_reg_read(pdv_p, PDV_MASK_LO);
    mask_lo_wrap = edt_reg_read(pdv_p, PDV_MASK_LO+32) ;
    edt_reg_write(pdv_p, PDV_MASK_LO+32, 0);
    edt_reg_write(pdv_p, PDV_MASK_LO, 0xa5);
    if ((r = edt_reg_read(pdv_p, PDV_MASK_LO+32)) == 0xa5)
        wrapped = 1;

    /* restore the register */
    edt_reg_write(pdv_p, PDV_MASK_LO, mask_lo);
    if (!wrapped)
        edt_reg_write(pdv_p, PDV_MASK_LO, mask_lo_wrap);

    edt_msg(DEBUG2, "registers %s\n", wrapped? "WRAPPING":"not wrapping");
    return wrapped;
}


/**
 * Set board FPGA registers from the a list of FPGA register address / value pairs in
 * the device handle's dependent struct.
 * Primarily for internal use by pdv_initcam (initcam) and clsim_lib (clsiminit).
 * @param pdv_p   device handle returned by pdv_open
 * @param dd_p    dd_p Dependent device structure, including xilinx_flag element
 * @return void
 */
void
pdv_do_xregwrites(EdtDev *edt_p, Dependent *dd_p)
{
    int i;

    /*
    * set any registers specifically called out with xregwrite
    * xilinx_flag (fpga) used to be just a flag 0 or 1 to write, now 
    * holds the actual address of the register (unless 0xff)
    */
    for (i = 0; i < 32; i++)
    {
        if (dd_p->xilinx_flag[i] == 0xff)
            break;

        edt_intfc_write(edt_p, dd_p->xilinx_flag[i], dd_p->xilinx_value[i]);
        edt_msg(DEBUG2, "xregwrite_%d writing reg 0x%02x value 0x%02x\n",
            dd_p->xilinx_flag[i], dd_p->xilinx_flag[i],
            dd_p->xilinx_value[i]);
        dep_wait(edt_p);
    }
}
