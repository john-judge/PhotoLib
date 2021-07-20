/* #pragma ident "@(#)libedt.c  1.462 12/08/10 EDT" */

#include "edtinc.h"

#if defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include <stdlib.h>
#include <ctype.h>

#ifndef _NT_
#include <sys/ioctl.h>
#endif

#ifdef VXWORKS
#include "vmLib.h"
#include "cacheLib.h"
extern int sysGetVmPageSize();
/* #include <strings.h>*/
#include <string.h>
#endif

#ifdef _NT_

#include <process.h>

#else

#ifndef VXWORKS
#include <sys/errno.h>
#endif

#endif

#ifdef __sun
#include <thread.h>
#endif
#ifdef __sun
#include <sys/mman.h>
#endif
#ifdef __sun
#include <sys/priocntl.h>
#include <sys/rtpriocntl.h>
#include <sys/tspriocntl.h>
#endif

#if defined(_NT_)
#include <process.h>
#elif defined(__sun) || defined(__linux__) || defined(__APPLE__)
#include <sys/wait.h>
#endif


#ifndef PAGE_SHIFT
#define PAGE_SHIFT (12)
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE (1UL << PAGE_SHIFT)
#endif

#ifndef PAGE_MASK
#define PAGE_MASK (PAGE_SIZE-1)
#endif


/*
* EDT Library
*
* Copyright (c) 1998, 2005 by Engineering Design Team, Inc.
*
* DESCRIPTION Provides a 'C' language   interface to the EDT PCI DMA cards
* to simplify the       ring buffer method of reading data.
*
* All routines access a specific device, whose handle is created and returned
* by    the edt_open() routine.
*
*/


int dump_reg_access = 0;


static u_char *intfc_dump = 0;
static u_char *base_dump = 0;
static u_char *dma_reg_dump = 0;

static int get_xref_info_params(char *str, char *fpga, char *sn, char *mtype, char *moffs, char *mnum, char *desc, char *rsvd1, char *rsvd2);
static int isdigit_str(char *s);

int dump_ir_access = 0;

void edt_set_dump_reg_access(int on)

{
    if (intfc_dump == NULL)
    {
        intfc_dump = (u_char *) calloc(256,1);
        base_dump = (u_char *) calloc(256,1);
        dma_reg_dump = (u_char *) calloc(256,1);
    }

    dump_reg_access = on;

}

int edt_get_dump_reg_access()

{
    return dump_reg_access;
}

void edt_set_dump_ir_access(u_int on)

{
    dump_ir_access = on;
    if (on)
        edt_set_dump_reg_access(on);

}

void edt_set_dump_reg_address(u_int reglow, u_int n, u_int on)

{
    int type = (reglow >> 16) & 0xf;
    u_int i;
    u_int start = reglow & 0xff;
    u_int nmax;

    if (type == 3)
        nmax = 8;
    else
        nmax = 256;

    if (start + n > nmax-1)
        n = nmax-start;

    if (intfc_dump == NULL)
        edt_set_dump_reg_access(TRUE);

    switch(type)
    {

    case 0:
        for (i=start; i< start+n;i++)
            base_dump[i] = on;

        break;

    case 1:
        for (i=start; i< start+n;i++)
            intfc_dump[i] = on;

        break;

    case 3:
        if (n > 8)
            n = 8;

        for (i=start; i< start+n;i++)
            dma_reg_dump[i] = on;

        break;

    }

}

u_char
is_reg_dumpable(u_int regvalue)

{
    int type = EDT_REG_TYPE(regvalue);

    int offset = regvalue & 0xff;
    int class = EDT_REG_CLASS(regvalue);

    if (class == 1)
        return dump_ir_access;

    switch(type)
    {
    case 0:
        return base_dump[offset];
    case 1:
        return intfc_dump[offset];
    case 3:
        return dma_reg_dump[offset];

    }

    return 0;
}

void
edt_setenv_reg_dump(const char *s)

{
char test[128];
int low;
 int high;
size_t index;
int range=0;
int in_number=0;
int tindex=0;

for (index=0;index<strlen(s)+1;index++)
{
    switch(s[index]) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
            in_number=1;
            test[tindex++]=s[index];
        break;

        case ':':
        case ' ':
        case 0:

        if (in_number)
        {
            test[tindex] = 0;
            if (range)
                high=strtol(test,NULL,16);
            else
                low=strtol(test,NULL,16);
            if (range)
                edt_set_dump_reg_address(low | INTFC_BYTE,(high-low+1),1);
             else
                edt_set_dump_reg_address(low | INTFC_BYTE,1,1);
            range=0;
            in_number=0;
            tindex=0;
        }
        break;
        case '-':
        if (in_number)
        {
                test[tindex] = 0;
                low=strtol(test,NULL,16);
                range=1;
                in_number=0;
                tindex=0;
        }
        break;
    }
}
}

#ifndef _NT_

extern int errno;

static void
edt_set_errno(int val)

{
    errno = val;
}

#else

#define edt_set_errno(val)

#endif



int edt_clear_wait_status(EdtDev *edt_p);

#ifndef SECTOR_SIZE
#define SECTOR_SIZE 512
#endif

#ifndef PAGESIZE
#define PAGESIZE 4096
#endif

#define DEFAULT_BUFFER_GRANULARITY PAGESIZE
#define MINHEADERSIZE SECTOR_SIZE
/* shorthand debug level */
#define EDTDEBUG EDTLIB_MSG_INFO_2
#define EDTFATAL EDTLIB_MSG_FATAL
#define EDTWARN EDTLIB_MSG_WARNING

#if defined(__APPLE__)
void *edt_mac_open(char *classname, int unit, int channel);
void *edt_mac_ioctl(void *dataPort, int code, void *eis);
#endif

#ifdef _NT_
static u_int WINAPI
edt_wait_event_thread(void *);

int
edt_get_kernel_event(EdtDev *edt_p, int event_num);
static
void
edt_clear_event_func(EdtEventHandler * p);

#else
#ifndef VXWORKS
static void *
edt_wait_event_thread(void *);
#endif

#endif

#ifndef VXWORKS
static char *BaseEventNames[] =
{
    NULL,
    EDT_EODMA_EVENT_NAME,
    EDT_BUF_EVENT_NAME,
    EDT_STAT_EVENT_NAME,
    EDT_P16D_DINT_EVENT_NAME,
    EDT_P11W_ATTN_EVENT_NAME,
    EDT_P11W_CNT_EVENT_NAME,
    EDT_PDV_ACQUIRE_EVENT_NAME,
    EDT_EVENT_PCD_STAT1_NAME,
    EDT_EVENT_PCD_STAT2_NAME,
    EDT_EVENT_PCD_STAT3_NAME,
    EDT_EVENT_PCD_STAT4_NAME,
    EDT_PDV_STROBE_EVENT_NAME,
    EDT_EVENT_P53B_SRQ_NAME,
    EDT_EVENT_P53B_INTERVAL_NAME,
    EDT_EVENT_P53B_MODECODE_NAME,
    EDT_EVENT_P53B_DONE_NAME,
    EDT_PDV_EVENT_FVAL_NAME,
    EDT_PDV_EVENT_TRIGINT_NAME,
    EDT_EVENT_TEMP_NAME,
    NULL
};
#endif


int edt_parse_devname(EdtDev *edt_p, char *edt_devname, int unit, int channel);

EdtDev * edt_open_device_struct(EdtDev *edt_p,
                           const char *device_name,
                           int unit, int channel,
                           int verbose)

{

    static char *debug_env = NULL;
    u_int   dmy;
    int edt_debug;
    static char *debug_file = NULL;
    int level;

    if ((debug_env == NULL)
        && ((debug_env = (char *) getenv("EDTDEBUG")) != NULL)
        && *debug_env != '0')
    {
        edt_debug = atoi(debug_env);
        level = edt_msg_default_level();
        if (edt_debug > 0)
        {
            level |= EDTLIB_MSG_INFO_1;
            level |= EDTLIB_MSG_INFO_2;
        }
        edt_msg_set_level(edt_msg_default_handle(), level);

        if ((debug_file == NULL)
            && ((debug_file = (char *) getenv("EDTDEBUGFILE")) != NULL)
            && *debug_file != '0')
        {
            edt_msg_set_name(edt_msg_default_handle(), debug_file);
        }

        edt_msg(EDTDEBUG, "environment DEBUG set to %d: enabling debug in edtlib\n", edt_debug);
    }

    if(verbose)
        edt_msg(EDTDEBUG, "edt_open_device(%s, %d)\n", device_name, unit);

    if ((strncmp(device_name, "dmy", 3) == 0) || (strncmp(device_name, "DMY", 3) == 0))
        edt_p->devid = DMY_ID;


    if (edt_parse_devname(edt_p, (char *) device_name, unit, channel) != 0)
    {
        edt_msg(EDTFATAL, "Illegal EDT device name (edt_open_device):  %s\n", device_name);
        return NULL;
    }

    edt_p->unit_no = unit;
    edt_p->channel_no = channel;

    {

#ifdef _NT_
        edt_p->fd = CreateFile(edt_p->edt_devname,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            edt_p->devid == (DMY_ID) ? OPEN_ALWAYS
            : OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (edt_p->fd == INVALID_HANDLE_VALUE)
        {
            if (verbose)
                edt_msg(EDTWARN, "EDT %s open failed.\nCheck board installation and unit number, and try restarting the computer\n", device_name);

            return (NULL);
        }
#else
#if defined (__APPLE__)
        /* dataPort = edt_mac_open("pcd",0) ;*/
        if (verbose)
            edt_msg(EDTWARN, "edt_devname %s unit %d channel %d\n",edt_p->edt_devname,unit,channel) ;
        if (edt_p->devid != DMY_ID)
        {
            int nchannels = 0;
            int dummy = 0;

            /* open channel zero so we can check max channel val before
             * attempting to open the actual channel.
             */
            if ((edt_p->fd = edt_mac_open(edt_p->edt_devname, unit, 0)) <= 0)
                return NULL;

            edt_p->channel_no = 0;
            nchannels = edt_ioctl(edt_p, EDTG_MAXCHAN, &dummy);
            if (channel < nchannels)
            {
                edt_mac_close(edt_p->fd);
                edt_p->channel_no = channel;
                if ((edt_p->fd = edt_mac_open(edt_p->edt_devname, unit, channel)) <= 0 )
                {
                    if (verbose)
                        edt_msg(EDTWARN, "EDT %s open failed.\nCheck board installation and unit number, and try restarting the computer\n", edt_p->edt_devname);
                    return (NULL);
                }

                edt_ioctl(edt_p, EDTG_DEVID, &edt_p->devid);
                if (edt_is_pdv(edt_p))
                {
                    if (strncmp(device_name, "pdv", 3) != 0)
                    {
                        if (verbose)
                            printf("%s not match for pdv\n",edt_p->edt_devname) ;
                        close(edt_p->fd) ;
                        return(NULL);
                    }
                }
                else if (edt_is_pcd(edt_p))
                {
                    if (strncmp(device_name, "pcd", 3) != 0)
                    {
                        if (verbose)
                            printf("%s not match for pcd\n",edt_p->edt_devname) ;
                        close(edt_p->fd) ;
                       return(NULL);
                    }
                }
            }
            else
            {
                edt_msg(EDTWARN, "EDT %s open failed. Channel number %d is larger than max allowed %d\n", edt_p->edt_devname, channel, nchannels-1);
                return NULL;
            }
        }
        else
        {
            if (!edt_p->fd)
            {
                if ((edt_p->fd = open(edt_p->edt_devname, O_RDWR|O_CREAT, 0666)) < 0 )
                {
                    if (verbose)
                        edt_msg(EDTWARN, "EDT %s open failed.\nCheck board installation and unit number, and try restarting the computer\n", edt_p->edt_devname);
                    return (NULL);
                }
            }
            /* printf("skipping open with fd already open\n") ;*/
        }
#else

        if (edt_p->devid == DMY_ID)
          close(creat(edt_p->edt_devname, 0666));

        if ((edt_p->fd = open(edt_p->edt_devname, (O_RDWR | O_NONBLOCK), 0666)) < 0 )
        {
            if (verbose)
                edt_msg(EDTWARN, "EDT %s open failed.\nCheck board installation and unit number, and try restarting the computer\n", edt_p->edt_devname);
            return (NULL);
        }

        if (edt_p->devid == DMY_ID)
          unlink(edt_p->edt_devname);

        /* make sure fd not carried across exec */
#if defined( __linux__) || defined(__sun)
        fcntl(edt_p->fd, F_SETFD, FD_CLOEXEC);
#endif

#endif /* not apple */
#endif
    }



    edt_msg(EDTDEBUG, "edt_open for %s unit %d succeeded\n", device_name, unit);

    if (edt_p->devid != DMY_ID)
    {
        edt_ioctl(edt_p, EDTG_DEVID, &edt_p->devid);
        edt_ioctl(edt_p, EDTS_RESETCOUNT, &dmy);
        dmy = edt_p->channel_no ;
        /* edt_ioctl(edt_p, EDTS_RESETSERIAL, &dmy); */
        edt_p->DMA_channels = edtdev_channels_from_type(edt_p);

#ifdef _NT_
        edt_get_kernel_event(edt_p, EDT_EVENT_BUF);
        edt_get_kernel_event(edt_p, EDT_EVENT_STAT);
        if (edt_p->devid == P53B_ID)
        {
            edt_get_kernel_event(edt_p, EDT_EVENT_P53B_SRQ);
            edt_get_kernel_event(edt_p, EDT_EVENT_P53B_INTERVAL);
            edt_get_kernel_event(edt_p, EDT_EVENT_P53B_MODECODE);
            edt_get_kernel_event(edt_p, EDT_EVENT_P53B_DONE);
        }
#endif
    }

    edt_p->devtype = edt_devtype_from_id(edt_p->devid);
    edt_p->donecount = 0;
    edt_p->tmpbuf = 0;
    edt_p->dd_p = 0;
    edt_p->b_count = 0;

    edt_p->dmy_started = 0;
    edt_p->dmy_wait_for_buffers_callback = NULL;
    edt_p->dmy_reg_read_callback = NULL;
    edt_p->dmy_reg_write_callback = NULL;

    edt_p->buffer_granularity = DEFAULT_BUFFER_GRANULARITY;
    edt_p->mezz.id = MEZZ_ID_UNKNOWN;
    edt_p->reg_fifo_io    = NULL;
    edt_p->reg_fifo_cnt   = NULL;
    edt_p->reg_fifo_ctl   = NULL;
    edt_p->reg_intfc_off  = NULL;
    edt_p->reg_intfc_dat  = NULL;


    return edt_p;
}


/**
 *  Opens an EDT device. This call underlies the other edt_open* calls, which
 *  basically just map to different variations on calls to this one. For example,
 *  edt_open_quiet calls edt_open_device with <b>verbose</b> set to 0 (false).
 *  User applications should typically use the higher-level calls rather than
 *  calling this directly, although there's no real harm in doing so either.
 * @param device_name a string with the name of the EDT Product board;
 * for example, "pcd".  \c EDT_INTERFACE can also be used; it is defined as
 * the name of the board type in \c edtdef.h.
 * @param unit Unit number of the device (if multiple devices). The
 * first unit is always 0.
 * @param channel specifies DMA channel number (counting from zero).
 *  @param verbose when 0, produce no console output. When nonzero, outputs a message on successful open, or an error-specific message on failure
 *  @see edt_open, edt_open_channel, edt_open_quiet
 *
* @return A pointer to the EdtDev structure if successful. This data
 */
EdtDev *edt_open_device(const char *device_name, int unit, int channel, int verbose)

{
    EdtDev *edt_p;
    EdtDev *save_p;

    if ((edt_p = (EdtDev *) calloc(1, sizeof(EdtDev))) == NULL)
    {
        char errstr[128];
        sprintf(errstr, "edtlib: malloc (%x) in edt_open failed\n", (int)sizeof(EdtDev));
        edt_msg_perror(EDTWARN, errstr);
        return (NULL);
    }

    save_p = edt_p;
    edt_p = edt_open_device_struct(
        edt_p,
        device_name,
        unit, channel, verbose);

    if (edt_p == NULL)
    {
        free(save_p);
    }

    return edt_p;

}

/**
* Opens the specified EDT Product and sets up the device handle.
*
* Once opened, the device handle may be used to perform I/O using
* #edt_read, #edt_write, #edt_configure_ring_buffers, and other
* input-output library calls. When finished, use #edt_close to release
* any resources allocated during use.
*
* @param device_name a string with the name of the EDT Product board;
* for example, "pcd".  \c EDT_INTERFACE can also be used; it is defined as
* the name of the board type in \c edtdef.h.
* @param unit Unit number of the device (if multiple devices). The
* first unit is always 0.
* @see edt_open_channel, edt_open_quiet, edt_close
*
* @return A pointer to the EdtDev structure if successful. This data
* structure holds information about the device which is needed by
* library functions.  User applications should avoid accessing
* structure elements directly.
* NULL is returned if unsuccessful, and the global variable errno is
* set.  Use #edt_perror to print an error message.
*
*/

EdtDev *
edt_open(const char *device_name, int unit)
{
    return edt_open_device(device_name, unit, 0, 1);
}


/**
* Just a version of #edt_open that does so quietly, so we can try opening
* the device just to see if it's there without a lot of printfs coming out
*
* @param device_name a string with the name of the EDT Product board;
* for example, "pcd".  \c EDT_INTERFACE can also be used; it is defined as
* the name of the board type in \c edtdef.h.
* @param unit Unit number of the device (if multiple devices). The
* first unit is always 0.
* @see edt_open, edt_open_channel, edt_close
*
* @return Pointer to EdtDev struct, or NULL if error.
*/

EdtDev *
edt_open_quiet(const char *device_name, int unit)
{
    return edt_open_device(device_name, unit, 0, 0);
}

/**
* Opens a specific DMA channel on the specified EDT Product, when
* multiple channels are supported by the Xilinx firmware, and sets up
* the device handle. Use #edt_close to close the channel.
*
* To open a device with only one channel, just use #edt_open.
*
* Once opened, the device handle may be used to perform I/O using
* #edt_read, #edt_write, #edt_configure_ring_buffers, and other
* input-output library calls. When finished, use #edt_close to release
* any resources allocated during use.
*
* @param device_name a string with the name of the EDT Product board;
* for example, "pcd".  \c EDT_INTERFACE can also be used; it is defined as
* the name of the board type in \c edtdef.h.
* @param unit Unit number of the device (if multiple devices). The
* first unit is always 0.
* @param channel specifies DMA channel number (counting from zero).
*
* @return A pointer to the EdtDev structure if successful. This data
* structure holds information about the device which is needed by
* library functions.  User applications should avoid accessing
* structure elements directly.
* NULL is returned if unsuccessful, and the global variable errno is
* set.  Use #edt_perror to print an error message.
* @see edt_open, edt_open_quiet, edt_close
*/

EdtDev *
edt_open_channel(const char *device_name, int unit, int channel)
{
    return edt_open_device(device_name, unit, channel, 1);
}

/**
* edt_parse_devname
*
* parse the EDT device name
*
* @return 0 on success, -1 on failure
* @see edt_open
*/

int
edt_parse_devname(EdtDev *edt_p, char *device_name, int unit, int channel)
{

    char *format;

    char dev_string[512];

    if (!device_name)
        return -1;

    if (device_name[0] == '\\' || device_name[0] == '/')
    {
        strcpy(edt_p->edt_devname, device_name);
    }
    else if (edt_p->devid == DMY_ID)
    {
#ifdef _NT_
        format = ".\\%s%d" ;
#else
        format = "./%s%d" ;
#endif
        (void) sprintf(edt_p->edt_devname, format, device_name, unit) ;
    }
    else
    {
        int i;

        strncpy(dev_string, device_name,511);

        for (i=0;dev_string[i]; i++)
            dev_string[i] = tolower(dev_string[i]);

#ifdef _NT_
        dev_string[0] = toupper(dev_string[0]);
#endif

#if defined(__linux__)
        if (strcmp(dev_string, "pcd") == 0)
            strcpy(dev_string, "pcicd");
#endif


#ifdef _NT_

#ifdef WDF_DRIVER
        if (channel > 0)
        {
            format = "\\\\.\\%s%d\\%d_%d";
        }
        else
        {
            format = "\\\\.\\%s%d\\%d_%d";
        }
        (void) sprintf(edt_p->edt_devname, format, dev_string, unit, unit, channel);
#else
        if (channel > 0)
        {
            format = "\\\\.\\%s%d_%d";
        }
        else
        {
            format = "\\\\.\\%s%d";
        }
        (void) sprintf(edt_p->edt_devname, format, dev_string, unit, channel);


#endif

#else
        if (channel > 0)
        {
            format = "/dev/%s%d_%d";
        }
        else
        {
            format = "/dev/%s%d";
        }
        (void) sprintf(edt_p->edt_devname, format, dev_string, unit, channel);
#endif

    }
    /* edt_msg(EDTDEBUG, "parse open to %s\n",edt_p->edt_devname) ; */

    return 0;
}

/**
* Shuts down all pending I/O operations, closes the device or channel
* and frees all driver resources associated with the device handle.
*
* @param edt_p pointer to edt device structure returned by #edt_open
*
* @return 0 on success, -1 on failure. If an  error occurs, call
* #edt_perror to get the system error message.
* @see edt_open_device
*/

int
edt_close_device(EdtDev *edt_p)

{
    u_int   i;
#ifdef __sun
    EdtEventHandler *p;
#endif

    edt_msg(EDTDEBUG, "edt_close()\n");

    for (i = 0; i < EDT_MAX_KERNEL_EVENTS; i++)
    {


#ifdef __sun
        p = &edt_p->event_funcs[i];
        if (p->active)
        {
            p->active = 0;

            edt_ioctl(p->owner, EDTS_DEL_EVENT_FUNC, &i);
            edt_ioctl(p->owner, EDTS_CLR_EVENT, &i);
            sema_destroy(&p->sema_thread);
            thr_join(NULL, NULL, NULL);
        }
#else
#ifndef NO_PTHREAD

        edt_remove_event_func(edt_p, i);
#endif
#endif
    }

    if (edt_p->ring_buffers_configured)
        edt_disable_ring_buffers(edt_p);

    if (edt_p->fd)
    {
#ifdef _NT_
        CloseHandle(edt_p->fd);
#else
#if defined (__APPLE__)
        edt_mac_close(edt_p->fd) ;
#else
        close(edt_p->fd);
#endif
#endif
    }

    return 0;
}

int
edt_close(EdtDev *edt_p)

{
    edt_close_device(edt_p);
    free(edt_p);

    return 0;
}

#ifdef __sun

typedef struct
{
    uint_t  index;
    uint_t  writeflag;
    EdtDev *edt_p;
}       thr_buf_args;

static void *
aio_thread(   void   *arg)

{
    thr_buf_args *bufp = (thr_buf_args *) arg;
    int     ret;
    u_char *addr;
    uint_t  size;
    int     index;
    EdtDev *edt_p;
    extern int errno;

    edt_p = bufp->edt_p;
    index = bufp->index;
    addr = edt_p->ring_buffers[index];
    size = edt_p->rb_control[index].size;

    /*
    * Start a read which will block forever in this thread. aioread() does
    * the same thing but won't allow as much concurrency.  Lseek orders the
    * buffers inside the driver. also aioread has exit problems
    */
    edt_msg(EDTDEBUG, "start %s for %d addr %x\n",
        bufp->writeflag ? "write" : "read", index, (u_int) addr);
    edt_set_errno(0);;
    if (bufp->writeflag)
        ret = pwrite(edt_p->fd, addr, size, index * 512);
    else
        ret = pread(edt_p->fd, addr, size, index * 512);
    if (ret == -1)
    {
        char    errstr[128];

        if (errno)
            perror("pread");
        sprintf(errstr, "aio buffer %d at 0x%x", index, (u_int) addr);
        edt_msg_perror(EDTDEBUG, errstr);
        edt_msg_perror(EDTDEBUG, "aiothread\n");
    }
    edt_msg(EDTDEBUG, "end aiothread for %d\n", index);
    return (0);
}

#endif

/**
* solaris needed to use threads to lock user memory for
* dma before 2.8 -
* Call edt_get_umem_lock(edt_p, 1) to enable this on 2.8
* and not have to use threads just to hold memory down
*/

int
edt_use_umem_lock(EdtDev *edt_p, u_int use_lock)
{
    int ret = 0 ;
    if (edt_p->ring_buffers_configured)
    {
        printf("Can't change umem lock method when ring buffers active\n") ;
        return(1) ;
    }
    edt_msg(EDTDEBUG, "use umem lock %d ",use_lock) ;
    ret = edt_ioctl(edt_p, EDTS_UMEM_LOCK, &use_lock) ;
    edt_msg(EDTDEBUG, "returns %d\n",ret) ;
    if (ret)
    {
        edt_msg(EDTDEBUG,
            "Can't use Umem lock expect on solaris 2.8 or above\n") ;
    }
    else
    {
        edt_msg(EDTDEBUG,"Setting umem lock\n") ;
    }
    return(ret) ;
}

int
edt_get_umem_lock(EdtDev *edt_p)
{
    u_int using_lock ;
    edt_ioctl(edt_p, EDTG_UMEM_LOCK, &using_lock) ;
    edt_msg(EDTDEBUG,"Using lock returns %d\n",using_lock) ;
    return(using_lock) ;
}

/**
*
* edt_get_numbufs
*
* returns the number of buffers allocated even if
* by other process (for monitoring from a separate
* call to edt_open)
*
*/

int
edt_get_numbufs(EdtDev *edt_p)

{
    int nb = 0;
    edt_ioctl(edt_p, EDTG_NUMBUFS, &nb);
    return nb;
}

#ifdef __sun

void
edt_sun_lock_mem(EdtDev *edt_p, int index, int write_flag, EdtRingBuffer *pring)

{
    int  using_lock = 0 ;
    int     count;
    thr_buf_args bufargs;

    using_lock = edt_get_umem_lock(edt_p) ;

    if (!using_lock)
    {
        bufargs.edt_p = edt_p;
        bufargs.index = index;
        bufargs.writeflag = write_flag;

        edt_msg(EDTDEBUG, "%d buffer thread\n", index);
        if (thr_create(NULL, 0,
            aio_thread,
            (void *) &bufargs, THR_BOUND,
            &pring->ring_tid))
        {
            edt_msg_perror(EDTWARN, "thr_create");
        }
        count = index + 1;

        edt_msg(EDTDEBUG, "Before WAITN");
        edt_ioctl(edt_p, EDTS_WAITN, &count);
        edt_msg(EDTDEBUG, "After WAITN");
    }
}

#endif

/*
* edt_configure_ring_buffer
*
* configures a single PCI Bus Configurable DMA Interface ring buffer.
*
* @ref edt_configure_ring_buffers
*
* @param edt_p: device handle returned from edt_open
* @param index: index of the buffer which buffer
* @param bufsize: size of each buffer
* @param nbufs: number of buffers
* @param data_direction: indicates whether this connection is to be used for output or input
*  @param  bufarray: array of pointers to application-allocated buffers (<b>deprecated</b> -- see note in /ref edt_configure_ring_buffers.
*
* @return 0 on success; -1 on error
*/

static int
edt_configure_ring_buffer(EdtDev * edt_p,
                          int index,
                          int bufsize,
                          int write_flag,
                          unsigned char *pdata)

{
    buf_args sysargs;
    int allocated_size = bufsize;
    EdtRingBuffer *pring;
    int rc;

    if (index < 0 || index >= MAX_DMA_BUFFERS)
    {
        edt_set_errno(EINVAL);

        fprintf(stderr,
            "invalid buffer index %d < 0 or >= MAX_DMA_BUFFERS (%d)\n",
            index, MAX_DMA_BUFFERS);
        return -1;
    }

    pring = &edt_p->rb_control[index];



    pring->size = bufsize;
    pring->write_flag = write_flag;

    if (pdata || (edt_p->mmap_buffers))
    {
        pring->owned = FALSE;
        if (edt_p->fullbufsize)
            allocated_size = edt_p->fullbufsize;
    }
    else
    {
#ifdef _NT_
        /* round up to full page for fast file access */
        if (allocated_size & (SECTOR_SIZE-1))
            allocated_size = ((allocated_size / SECTOR_SIZE)+1)* SECTOR_SIZE;
#endif
        pdata =
            edt_alloc(allocated_size);
        pring->owned = TRUE;
    }

    pring->allocated_size = allocated_size;

    edt_p->ring_buffers[index] = pdata;

    sysargs.index = index;
    sysargs.writeflag = write_flag;
#ifdef WIN32
    sysargs.addr = (uint64_t) (pdata);
#else
    sysargs.addr = (uint64_t) ((unsigned long)pdata);
#endif

    sysargs.size = bufsize;

    if (edt_p->mmap_buffers)
    {
        if ((rc = edt_ioctl(edt_p, EDTS_BUF_MMAP, &sysargs)) == 0)
        {
#ifdef __sun
            edt_sun_lock_mem(edt_p, index, write_flag, pring);
#endif
            return 0;

        }
        else
        {
            edt_msg(EDT_MSG_FATAL, "Unable to configure ring buffer\n");
            edt_msg(EDT_MSG_FATAL, "rc = %d\n", rc);
            return rc;
        }
    }
    else if (edt_ioctl(edt_p, EDTS_BUF, &sysargs) == 0)
    {
#ifdef __sun
        edt_sun_lock_mem(edt_p, index, write_flag, pring);
#endif
        return 0;
    }
    return (-1);
}

/*
* Obsolete - was used as a workaround for edt_set_direction().
* Still used in legacy code, so keep for compatibility.
* TODO: needs to be made compatible with edt_p->mmap_buffers.
*/
int
edt_add_ring_buffer(EdtDev * edt_p,
                    unsigned int bufsize,
                    int write_flag,
                    void *pdata)
{

    int     index = -1;

    int     i;

    if (!edt_p->mmap_buffers)
    {

        /* allocate a slot for the data */

        for (i = 0; i < MAX_DMA_BUFFERS; i++)
            if (edt_p->ring_buffers[i] == NULL)
                break;

        if (i == MAX_DMA_BUFFERS)
            return -1;

        index = i;

        edt_p->ring_buffer_numbufs++;

        edt_ioctl(edt_p, EDTS_NUMBUFS, &edt_p->ring_buffer_numbufs);

        edt_configure_ring_buffer(edt_p, index, bufsize, write_flag, (u_char *) pdata);
    }

    return index;
}


/*
* Obsolete - was used as a workaround for edt_set_direction(), but
* no longer needed.  Still may show up in customer code, so keep.
*
* @return 0 on success, -1 on failure
*/
int
edt_configure_channel_ring_buffers(EdtDev *edt_p, int bufsize, int numbufs,
                                   int write_flag, unsigned char **bufarray)
{
    return edt_configure_ring_buffers(edt_p, bufsize, numbufs,
        write_flag, bufarray) ;
}

static
int edt_check_ring_buf_parms(EdtDev *edt_p, int numbufs, int bufsize,
                             unsigned char *pdata, unsigned char ** bufarray)

{

    int maxbufs;

    if (!edt_p)
    {
        edt_msg_perror(EDTFATAL, "Invalid edt handle\n");
        return -1;
    }


    /* Odd number of bytes is illegal.  Mark & Chet Oct'2000 */
    if (bufsize & 0x01)
    {
        edt_set_errno(EINVAL); ;

        edt_msg_perror(EDTFATAL, "edt_configure_ring_buffers: bufsize must be an even number of bytes\n") ;
        return -1 ;
    }
    maxbufs = edt_get_max_buffers(edt_p) ;
    if (numbufs > maxbufs)
    {

        edt_set_errno(EINVAL); ;

        edt_msg_perror(EDTFATAL,
            "edt_configure_ring_buffers: number of bufs exceeds maximum\n") ;
        edt_msg_perror(EDTFATAL,
            "use edt_set_max_buffers to increase max\n") ;
        return -1 ;
    }

    if (edt_p->devid != DMY_ID)
    {
      edt_p->mmap_buffers = edt_get_mmap_buffers(edt_p);

      if (edt_p->mmap_buffers)
      {
          if (edt_p->buffer_granularity < PAGE_SIZE)
              edt_p->buffer_granularity = PAGE_SIZE;

      }

      if (edt_p->mmap_buffers && ((pdata != NULL) || (bufarray != NULL)))
      {

          edt_set_errno(EINVAL); ;

          edt_msg_perror(EDTFATAL,
              "edt_configure_ring_buffers: can't pass in user pointer when using mmap kernel buffers\n") ;

          return -1;
      }
    }

    return 0;
}

caddr_t
edt_reserve_kernel_mem(EdtDev *edt_p, u_int totalsize)

{
    caddr_t p = NULL;
    buf_args sysargs;

    memset(&sysargs, 0, sizeof(sysargs));

    sysargs.size = totalsize;

    edt_ioctl(edt_p, EDTS_ALLOC_KBUFFER,&sysargs);

    p = (caddr_t) sysargs.addr;

    return p;

}


#define EDT_DMAMEM_OFFSET 0x10000000

int
edt_unmap_dmamem(EdtDev *edt_p)
{

#ifdef __linux__

    if (edt_p->base_buffer)
        munmap(edt_p->base_buffer, edt_p->totalsize);
#elif defined(WIN32)

    caddr_t p = NULL;
    buf_args sysargs;

    memset(&sysargs, 0, sizeof(sysargs));

    sysargs.size = 0;

    edt_ioctl(edt_p, EDTS_ALLOC_KBUFFER,&sysargs);

#endif

    return 0;
}

caddr_t
edt_map_dmamem(EdtDev *edt_p)
{

    caddr_t     ret = NULL;

#ifdef __linux__
    int pagen = EDT_DMAMEM_OFFSET;

    ret = (caddr_t) mmap((caddr_t)0, edt_p->totalsize, PROT_READ|PROT_WRITE,
        MAP_SHARED, edt_p->fd, pagen);

#elif defined(WIN32)

    /* This should already be allocated */
    ret = (caddr_t) edt_p->base_buffer;

#endif

    if (ret == ((caddr_t)-1)) {
        perror("mmap call");
        return(0) ;
    }

    return(ret) ;
}

static int
edt_allocate_ring_buffer_memory(EdtDev *edt_p, int bufsize, int numbufs, u_char *user_mem, u_char **buffers)

{

    unsigned char * bp;
    int i;

    edt_p->fullbufsize = edt_get_total_bufsize(edt_p, bufsize, edt_p->header_size);

    edt_p->totalsize = edt_p->fullbufsize * numbufs;

#ifdef __linux__
    if (edt_p->mmap_buffers)
    {
        /* don't allocate, we'll do that later */


        for (i=0;i<numbufs;i++)
            buffers[i] = NULL;

        return 0;
    }
#endif


    if (edt_p->base_buffer)
    {
        edt_free(edt_p->base_buffer);
        edt_p->base_buffer = NULL;
    }

    if (user_mem)
    {
        bp = user_mem;
    }
    else
    {


        if (edt_p->mmap_buffers)
        {
            /* call routine to reserve */
            edt_p->ring_buffers_allocated = TRUE;
            edt_p->base_buffer = (u_char *) edt_reserve_kernel_mem(edt_p, edt_p->totalsize);
        }
        else
        {
            edt_p->ring_buffers_allocated = TRUE;
            edt_p->base_buffer = edt_alloc(edt_p->totalsize);
        }

        if (!edt_p->base_buffer)
        {
            edt_msg_perror(EDTFATAL, "Unable to allocate buffer memory\n");
            return -1;
        }

        bp = edt_p->base_buffer;


    }

    if (edt_p->header_offset < 0)
        bp += ((int) edt_p->header_size);

    for (i=0;i<numbufs;i++)
    {
        buffers[i] = bp;
        bp += edt_p->fullbufsize;
    }


    return 0;

}


/**
* Sets up the EDT device ring buffers.
*
* Any previous configuration is replaced, and previously allocated buffers are released.
* Buffers are normally allocated and maintained within the EDT device library (bufarray = NULL).
*
* @note bufarray can alternately point to an array of user buffers which will be
* used instead of the internally allocated ones, however <b>it will fail</b>
* (possibly with a system crash) if the system has more than 4 GBytes of
* memory. Since > 4 GBytes is becoming ubiquitous,
* providing user buffers has effectively been deprecated. The argument remains in
* order to maintain code constency, nevertheless <b>EDT can not provide support
* for any applications that provide a non-NULL argument in bufarray.</b>
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @param bufsize size of each buffer, in bytes. For optimal efficiency,
* allocate a value approximating throughput divided by 20: that is, if
* transfer occurs at 20 MB per second, allocate 1 MB per buffer. Buffers
* significantly larger or smaller can overuse memory or lock the system
* up in processing interrupts at this speed.
*
* @param numbufs   number of buffers. Must be 1 or greater. Four is
* recommended for most applications.
*
* @param write_flag  Indicates whether this connection is to be used
* for input or output. Only one direction is possible per device or
* subdevice at any given time:
* - EDT_READ = 0
* - EDT_WRITE = 1
*
* @param bufarray If NULL, the library will allocate a set of
* page-aligned ring buffers. If not null <b>(Deprecated -- see note above)</b>
* this argument is an array of pointers to application-allocated
* and page aligned buffers (use #edt_alloc to allocate page alligned buffers);
* these buffers must match the size and number of buffers specified in this
* call and will be used as the ring buffers.
*
* @param pdata normally NULL
*
* @return 0 on success, -1 on error. If all buffers cannot be
* allocated, none are allocated and an error is returned.  Call
* #edt_perror to get the system error message.
*
*/
static int
edt_setup_ring_buffers(EdtDev *edt_p, int bufsize, int numbufs,
                       int write_flag, unsigned char *pdata, unsigned char **bufarray)
{
    int     i;
    int rc;

    u_char *localbuf[MAX_DMA_BUFFERS];
    u_char ** buffers;

    buffers = (bufarray != NULL)?bufarray:localbuf;

    if ((rc = edt_check_ring_buf_parms(edt_p, numbufs, bufsize,  pdata, bufarray)) != 0)
        return rc;

    if (edt_p->ring_buffers_configured)
        edt_disable_ring_buffers(edt_p);

    edt_p->nextwbuf = 0;
    edt_p->donecount = 0;
    edt_p->ring_buffer_bufsize = bufsize;
    edt_p->ring_buffer_numbufs = numbufs;

    if (write_flag)
        edt_set_direction(edt_p, EDT_WRITE);
    else
        edt_set_direction(edt_p, EDT_READ);

    edt_p->write_flag = write_flag;

    edt_ioctl(edt_p, EDTS_NUMBUFS, &numbufs);

    if (bufarray == NULL)
    {
        if (edt_allocate_ring_buffer_memory(edt_p, bufsize, numbufs, pdata, buffers) != 0)
            return -1;

        edt_p->ring_buffers_allocated = !edt_p->mmap_buffers && (pdata == NULL);
    }

    for (i = 0; i < numbufs; i++)
    {
        if (edt_configure_ring_buffer(edt_p, i,
            bufsize,
            write_flag,
            buffers[i]) == 0)
        {

        }
        else
        {
            return -1;
        }
    }

    if (edt_p->mmap_buffers)
    {
        u_char *bp;

        edt_p->base_buffer = (u_char *) edt_map_dmamem(edt_p);
        bp = edt_p->base_buffer;

        for (i=0;i<numbufs;i++)
        {
            edt_p->ring_buffers[i] = bp;
            bp += edt_p->fullbufsize;
        }

        /* go ahead and page it in */
        bp = edt_p->base_buffer;
        for (i=0; i<(int)edt_p->totalsize;i += PAGE_SIZE)
            bp[i] = 0x5a;


    }

    edt_p->ring_buffers_configured = 1;

    return 0;
}



void edt_set_buffer_granularity(EdtDev *edt_p, u_int granularity)

{
    edt_p->buffer_granularity = granularity;
}


/**
* edt_get_total_bufsize
*
* returns the total buffer size for block of buffers,
* in which the memory allocation size is rounded up so all buffers start on
* a page boundary. This is used to allocate a single contiguous block of
* DMA buffers.
*
**/

int
edt_get_total_bufsize(EdtDev *edt_p,
                      int bufsize,
                      int header_size)

{
    int fullbufsize;

    fullbufsize = header_size + bufsize;

    if (edt_p->buffer_granularity)
        if (fullbufsize % edt_p->buffer_granularity)
            fullbufsize = ((fullbufsize / edt_p->buffer_granularity)+1)* edt_p->buffer_granularity;

    return fullbufsize;
}

// Internal subroutine to support DMY_ID ring buffer operations.
static int
edt_alloc_dmy_buffers(EdtDev *edt_p, int bufsize, int numbufs, unsigned char **bufarray)
{
  int i;

  if (bufarray != NULL)
  {
    edt_perror("edt_configure_ring_buffers:  DMA_ID - Can not use preallocated user ring buffers");
    return -1;
  }

  bufarray = (u_char **) edt_alloc(sizeof(u_char *) * numbufs);
  if (bufarray == NULL)
  {
    edt_perror("edt_configure_ring_buffers:  DMA_ID - edt_alloc bufptrs failed");
    return -1;
  }


  for (i = 0; i < numbufs; ++i)
  {
    bufarray[i] = edt_alloc(bufsize);
    if (bufarray[i] == NULL)
    {
      edt_perror("edt_configure_ring_buffers:  DMA_ID - edt_alloc buffers failed!");
      return -1 ;
    }
    memset(bufarray[i], i, bufsize);
  }

  return 0;
}

/**
* Configures the EDT device ring buffers.
*
* Any previous configuration is replaced, and previously allocated buffers are released.
* Buffers are normally allocated and maintained within the EDT device library (bufarray = NULL).
*
* @note bufarray can alternately point to an array of user buffers which will be
* used instead of the internally allocated ones, however <b>it will fail</b>
* (possibly with a system crash) if the system has more than 4 GBytes of
* memory. Since > 4 GBytes is becoming ubiquitous,
* providing user buffers has effectively been deprecated. The argument remains in
* order to maintain code constency, nevertheless <b>EDT can not provide support
* for any applications that provide a non-NULL argument in bufarray.</b>
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @param bufsize size of each buffer, in bytes. For optimal efficiency,
* allocate a value approximating throughput divided by 20: that is, if
* transfer occurs at 20 MB per second, allocate 1 MB per buffer. Buffers
* significantly larger or smaller can overuse memory or lock the system
* up in processing interrupts at this speed.
*
* @param numbufs   number of buffers. Must be 1 or greater. Four is
* recommended for most applications.
*
* @param write_flag  Indicates whether this connection is to be used
* for input or output. Only one direction is possible per device or
* subdevice at any given time:
* - EDT_READ = 0
* - EDT_WRITE = 1
*
* @param bufarray If NULL, the library will allocate a set of
* page-aligned ring buffers. If not null <b>(Deprecated -- see note above)</b>
* this argument is an array of pointers to application-allocated
* and page aligned buffers (use #edt_alloc to allocate page alligned buffers);
* these buffers must match the size and number of buffers specified in this
* call and will be used as the ring buffers.
*
* @return 0 on success, -1 on error. If all buffers cannot be
* allocated, none are allocated and an error is returned.  Call
* #edt_perror to get the system error message.
*
*/

int
edt_configure_ring_buffers(EdtDev *edt_p, int bufsize, int numbufs,
                           int write_flag, unsigned char **bufarray)
{
    int retval = 0;

    if (edt_p->devid == DMY_ID)
      retval = edt_alloc_dmy_buffers(edt_p, bufsize, numbufs, bufarray);

    if (retval != 0)
      return retval;
    else
      return edt_setup_ring_buffers(edt_p, bufsize, numbufs, write_flag, NULL, bufarray);

}

/**
* Identical to #edt_configure_block_buffers, with the additional parameter
* \a user_mem, which allows the user to specify a block of
* pre-allocated memory to use (Note: this does not work on Linux).
*
* Users are encourage to use #edt_configure_block_buffers rather than
* this function, as that function handles allocation of memory and works
* on all systems.
*/

int
edt_configure_block_buffers_mem(EdtDev *edt_p,
                                int bufsize,
                                int numbufs,
                                int write_flag,
                                int header_size,
                                int header_before,
                                u_char *user_mem)

{

    /* check for valid arguments */
    if (header_before && header_size)
    {
        if (header_size & (SECTOR_SIZE-1))
            header_size = ((header_size / SECTOR_SIZE)+1)* SECTOR_SIZE;
    }


    edt_p->header_size = header_size;
    edt_p->header_offset = (header_before)? -header_size : bufsize;


    return edt_setup_ring_buffers(edt_p, bufsize, numbufs, write_flag, user_mem, NULL);

}


/**
* Similar to #edt_configure_ring_buffers, except that it allocates the
* ring buffers as a single large block, setting the ring buffer
* addresses from within that block. This allows reading or writing
* buffers from/to a file in single chunks larger than the buffer size,
* which is sometimes considerable more efficient.  Buffer sizes are
* rounded up by \c PAGE_SIZE so that DMA occurs on a page boundary.
*
*
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @param bufsize   size of individual buffers, in bytes
*
* @param numbufs   number of buffers to create
*
* @param write_flag:  1, if these buffers are set up to go out; 0 otherwise
*
* @param header_size  if non-zero, additional memory (\a header_size
* bytes) will be allocated for each buffer for Header data. The
* location of this header space is determined by the argument
* \a header_before.
*
* @param header_before if non-zero, the header space defined by
* \a header_size is placed before the DMA buffer; otherwise, it comes
* after the DMA buffer. The value returned by #edt_wait_for_buffers is
* always the DMA buffer.
*
*
* @return 0 on success, -1 on failure.
* @see edt_configure_ring_buffers
*/

/**
* Configures the EDT device ring buffers.
*
* Any previous configuration is replaced, and previously allocated buffers are released.
* Buffers are normally allocated and maintained within the EDT device library (bufarray = NULL).
*
* @note bufarray can alternately point to an array of user buffers which will be
* used instead of the internally allocated ones, however <b>it will fail</b>
* (possibly with a system crash) if the system has more than 4 GBytes of
* memory. Since > 4 GBytes is becoming ubiquitous,
* providing user buffers has effectively been deprecated. The argument remains in
* order to maintain code constency, nevertheless <b>EDT can not provide support
* for any applications that provide a non-NULL argument in bufarray.</b>
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @param bufsize size of each buffer, in bytes. For optimal efficiency,
* allocate a value approximating throughput divided by 20: that is, if
* transfer occurs at 20 MB per second, allocate 1 MB per buffer. Buffers
* significantly larger or smaller can overuse memory or lock the system
* up in processing interrupts at this speed.
*
* @param numbufs   number of buffers. Must be 1 or greater. Four is
* recommended for most applications.
*
* @param write_flag  Indicates whether this connection is to be used
* for input or output. Only one direction is possible per device or
* subdevice at any given time:
* - EDT_READ = 0
* - EDT_WRITE = 1
*
* @param bufarray If NULL, the library will allocate a set of
* page-aligned ring buffers. If not null <b>(Deprecated -- see note above)</b>
* this argument is an array of pointers to application-allocated
* and page aligned buffers (use #edt_alloc to allocate page alligned buffers);
* these buffers must match the size and number of buffers specified in this
* call and will be used as the ring buffers.
*
* @return 0 on success, -1 on error. If all buffers cannot be
* allocated, none are allocated and an error is returned.  Call
* #edt_perror to get the system error message.
*
*/
int
edt_configure_block_buffers(EdtDev *edt_p, int bufsize, int numbufs, int write_flag,
                            int header_size, int header_before)

{

    return edt_configure_block_buffers_mem(edt_p,
        bufsize,
        numbufs,
        write_flag,
        header_size,
        header_before,
        NULL);

}


/**
* @if edtinternal
* Disable a single ring buffer, freeing its resources.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @param whichone the index of the ring buffer to disable
*
* @return 0 on success, -1 on failure
* @endif
*/

int
edt_disable_ring_buffer(EdtDev *edt_p,
                        int whichone)

{
    if (edt_p->ring_buffers[whichone])
    {
        /* detach buffer from DMA resources */

        edt_ioctl(edt_p, EDTS_FREEBUF, &whichone);
        edt_msg(EDTDEBUG, "free buf %d\n", whichone);

#ifdef __sun
        {
            u_int using_lock ;
            using_lock = edt_get_umem_lock(edt_p) ;
            if (!using_lock)
            {
                edt_msg(EDTDEBUG, "joining user buf %d tid %x\n",
                    whichone, edt_p->rb_control[whichone].ring_tid);
                thr_join(edt_p->rb_control[whichone].ring_tid, NULL, NULL);

                edt_msg(EDTDEBUG, "join user buf %d done\n", whichone);
            }
        }
#endif
        /* free data pointer if we own it */
        if (edt_p->rb_control[whichone].owned)
        {
            edt_msg(EDTDEBUG, "free user buf %d\n", whichone);
            edt_free(edt_p->ring_buffers[whichone]);
        }

        edt_p->ring_buffers[whichone] = NULL;

    }
#if defined(__hpux)
    {
        int pret ;
        pret = plock(UNLOCK) ;
    }
#endif

    return 0;

}


/**
* Disables the EDT device ring buffers. Pending DMA is cancelled and
* all buffers are released.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @return 0 on success, -1 on error. If an error occurs, call
* #edt_perror to get the system error message.
*/

int
edt_disable_ring_buffers(EdtDev *edt_p)

{
    int   i;

    /* for (i = 0; i < edt_p->ring_buffer_numbufs; i++)*/
    for (i = edt_p->ring_buffer_numbufs - 1; i >= 0 ; i--)
    {
        edt_disable_ring_buffer(edt_p,i);
    }

    edt_p->ring_buffers_configured = 0;
    edt_p->ring_buffers_allocated = 0;
#if defined(__hpux)
    {
        int pret ;
        pret = plock(UNLOCK) ;
    }
#elif defined(sgi)
    munlockall();
#endif

    if (edt_p->base_buffer )
    {

        if (edt_p->mmap_buffers)
            edt_unmap_dmamem(edt_p);
        else
            edt_free(edt_p->base_buffer);
        edt_p->base_buffer = NULL;
    }
    /* Steve 1/11/01 to fix pdv_setsize bug */
    /* added NUMBUFS to fix dmaid issue 2/19/03 */
    {
        int one = 1;
        edt_p->ring_buffer_numbufs = 0;
        edt_ioctl(edt_p, EDTS_NUMBUFS, &one);
        edt_ioctl(edt_p, EDTS_CLEAR_DMAID, &one);
    }

    return 0;
}

/**
* Starts DMA to the specified number of buffers. If you supply a number
* greater than the number of buffers set up, DMA continues looping
* through the buffers until the total count has been satisfied.
*
* @param edt_p pointer to edt device structure returned by #edt_open
* @param count Number of buffers to release to the driver for
* transfer. An argument of 0 puts the driver in free running mode, and
* transfers run continuously until #edt_stop_buffers is called.
*
* @return 0 on success, -1 on error. If an error occurs, call
* #edt_perror to get the system error message.
*/

int
edt_start_buffers(EdtDev *edt_p, uint_t count)
{
    if (edt_p->devid == DMY_ID)
        edt_p->dmy_started += count;

    edt_msg(EDTDEBUG, "edt_start_buffers %d\n", count);
    return edt_ioctl(edt_p, EDTS_STARTBUF, &count);
}


int
edt_lockoff(EdtDev * edt_p)
{
    int count = 0;
    return edt_ioctl(edt_p, EDTS_STARTBUF, &count);
}

/**
* Gets the allocated size of the specified buffer.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param buffer the index of the buffer.
* @return The buffer size, in bytes, or 0 if the specified index is
* invalid. */

unsigned int
edt_allocated_size(EdtDev *edt_p, int buffer)

{
    if (buffer >= 0 && buffer < (int) edt_p->ring_buffer_numbufs)
    {
        return edt_p->rb_control[buffer].allocated_size;
    }
    return 0;
}

/**
* Sets which buffer should be started next. Usually done to recover
* after a timeout, interrupt, or error.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param bufnum the index of the buffer to start next.
*
* \Example
* @code
* u_int curdone;
* edt_stop_buffers(edt_p);
* curdone = edt_done_count(edt_p);
* edt_set_buffer(edt_p, curdone);
* @endcode
*
* @return 0 on success, -1 on failure.
*
* @see #edt_stop_buffers, #edt_done_count, #edt_get_todo
*/

int
edt_set_buffer(EdtDev *edt_p, uint_t bufnum)
{
    edt_ioctl(edt_p, EDTS_SETBUF, &bufnum);
    edt_p->donecount = bufnum;
    if (edt_p->ring_buffer_numbufs)
        edt_p->nextwbuf = bufnum % edt_p->ring_buffer_numbufs;
    return 0;
}

/**
* Performs a read on the EDT Product. For those on UNIX systems, the
* UNIX 2 GB file offset bug is avoided during large amounts of input or
* output, that is, reading past 2^31 bytes does not fail. This call is
* not multibuffering, and no transfer is active when it completes.
*
*
* @param edt_p pointer to edt device structure returned by #edt_open
* @param buf address of buffer to read into
* @param size size of read in bytes
* @return The return value from read, normally the number of bytes
* read; -1 is returned in case of error. Call #edt_perror to get the
* system error message.
*
* @note If using timeouts, call #edt_timeouts after edt_read returns to
* see if the number of timeouts has incremented. If it has incremented,
* call #edt_get_timeout_count to get the number of bytes transferred
* into the buffer.
*/

int
edt_read(EdtDev *edt_p, void   *buf, uint_t  size)
{
    u_char *thisbuf;

    if (size & 0x01) /* Odd no of bytes illegal.  Mark & Chet Oct'2000 */
        -- size;

    if (edt_p->last_direction != 1)
        edt_set_direction(edt_p, EDT_READ);

    if (edt_p->devid != DMY_ID)
    {
        if (edt_configure_ring_buffers(edt_p, size, 1, EDT_READ, NULL) != 0)
            return -1;

        edt_start_buffers(edt_p, 1) ;
        thisbuf = edt_wait_for_buffers(edt_p, 1);

        if (edt_timeouts(edt_p))
        {
            size = edt_get_timeout_count(edt_p);
            edt_msg(EDTDEBUG, "edt_read timeout with count %d\n", size);
        }

        memcpy(buf, thisbuf, size);
        edt_disable_ring_buffers(edt_p) ;
    }

    return (size);
}

/**
* Perform a write on the EDT Product. For those on UNIX systems, the
* UNIX 2 GB file offset bug is avoided during large amounts of input or
* output; that is, writing past 2^31 bytes does not fail. This call is
* not multibuffering, and no transfer is active when it completes.
*
*
* @param edt_p pointer to edt device structure returned by #edt_open
*
* @param buf address of buffer to write from
* @param size size of write in bytes
*
* @return The return value from write; -1 is returned in case of error.
* Call #edt_perror to get the system error message.
*
* @note If using timeouts, call #edt_timeouts after #edt_write returns
* to see if the number of timeouts has incremented. If it has
* incremented, call #edt_get_timeout_count to get the number of bytes
* transferred into the buffer. DMA does not automatically continue on
* to the next buffer, so you need to call #edt_start_buffers to move on
* to the next buffer in the ring.
*/

int
edt_write(EdtDev *edt_p, void   *buf, uint_t  size)
{
    u_char **bufs;

    if (size & 0x01) /* Odd no of bytes illegal.  Mark & Chet Oct'2000 */
        -- size;

    if (edt_p->last_direction != (EDT_WRITE+1))
        edt_set_direction(edt_p, EDT_WRITE);

    if (edt_p->devid != DMY_ID)
    {
        if (edt_configure_ring_buffers(edt_p, size, 1, EDT_WRITE, NULL) != 0)
            return -1;

        bufs = edt_buffer_addresses(edt_p);
        memcpy(bufs[0], buf, size);
        edt_start_buffers(edt_p, 1) ;
        edt_wait_for_buffers(edt_p, 1);

        if (edt_timeouts(edt_p))
        {
            size = edt_get_timeout_count(edt_p);
            edt_msg(EDTDEBUG, "edt_read timeout with count %d\n", size);
        }

        edt_disable_ring_buffers(edt_p) ;
    }

    return (size);
}

int
edt_write_pio(EdtDev *edt_p, u_char *buf, int size)

{
    edt_sized_buffer argbuf;

    argbuf.size = size;

    memcpy(argbuf.data, buf, size);

    return edt_ioctl(edt_p, EDTS_WRITE_PIO, &argbuf);

}


/**
* Returns a pointer to the next buffer scheduled for output DMA, in
* order to fill the buffer with data.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @return A pointer to the buffer, or NULL on failure.
*
* @see edt_next_writebuf_index
*/

unsigned char *
edt_next_writebuf(EdtDev *edt_p)

{
    unsigned char *buf_p;

    buf_p = edt_p->ring_buffers[edt_p->nextwbuf];
    edt_p->nextwbuf = ++edt_p->nextwbuf % edt_p->ring_buffer_numbufs;
    return buf_p;
}

/**
* Returns the index of the next buffer scheduled for output DMA, in
* order to fill the buffer with data. Increments the next buffer index,
* so subsequent calls to edt_next_writebuf will return subsequent buffers.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @return Index of the buffer, as returned by edt_buffer_addresses, or -1 on failure. If an error
* occurs, call #edt_perror to get the system error message.
*
* @return Index of the buffer, as returned by edt_buffer_addresses.
*/

uint_t
edt_next_writebuf_index(EdtDev *edt_p)

{
    uint_t ret = edt_p->nextwbuf;
    edt_p->nextwbuf = ++edt_p->nextwbuf % edt_p->ring_buffer_numbufs;
    return ret;
}

/**
* Returns an array containing the addresses of the ring buffers.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @return An array of pointers to the ring buffers allocated by the
* driver or the library. The array is indexed from zero to n-1 where n
* is the number of ring buffers set in #edt_configure_ring_buffers.
*
*/

unsigned char **
edt_buffer_addresses(EdtDev *edt_p)
{
    return (edt_p->ring_buffers);
}

/**
* Blocks until the specified number of buffers have completed with a
* pointer to the time the last buffer finished.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param count buffer number for which to block. Completed buffers are
* numbered cumulatively starting with 0 when the EDT Product is opened.
* @param timep pointer to an array of two unsigned integers. The first
* integer is seconds, the next integer is nanoseconds representing the
* system time at which the buffer completed.
*
* @return Address of last completed buffer on success; NULL on error.
* If an error occurs, call #edt_perror to get the system error message.
*
* @note If the ring buffer is in free-running mode and the application
* cannot process data as fast as it is acquired, DMA will wrap around
* and overwrite the referenced buffer . The application must ensure
* that the data in the buffer is processed or copied out in time to
* prevent overrun.
*/

unsigned char *
edt_wait_buffers_timed(EdtDev * edt_p, int count, u_int * timep)
{
    u_char *ret;

    edt_msg(EDTDEBUG, "edt_wait_buffers_timed()\n");

    ret = edt_wait_for_buffers(edt_p, count);
    edt_get_timestamp(edt_p, timep, edt_p->donecount - 1);

    edt_msg(EDTDEBUG, "buf %d done %s (%x %x)\n",
        edt_p->donecount,
        edt_timestring(timep),
        timep[0], timep[1]);

    return (ret);
}

/**
* Waits for the last buffer that has been transferred. This is useful
* if the application cannot keep up with buffer transfer. If this
* routine is called for a second time before another buffer has been
* transferred, it will block waiting for the next transfer to complete.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @return Address of the image.
* @see edt_wait_for_buffers, edt_last_buffer_timed
*/

unsigned char *
edt_last_buffer(EdtDev * edt_p)
{
    bufcnt_t     donecount;
    bufcnt_t     last_wait;
    int     delta;

    donecount = edt_done_count(edt_p);
    last_wait = edt_p->donecount;

    edt_msg(EDTDEBUG, "edt_last_buffer() last %d cur %d\n",
        last_wait, donecount);

    delta = donecount - last_wait;

    if (delta == 0)
        delta = 1;

    return edt_wait_for_buffers(edt_p, delta);
}

/**
* Like #edt_last_buffer but also returns the time at which the DMA was
* complete on this buffer. \a timep should point to an array of two
* unsigned integers which will be filled in with the seconds and
* nanoseconds of the time the buffer was finished being transferred.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param timep    pointer to an unsigned integer array
*
* \Example
* @code
* u_int timestamp [2];
* u_char *buf;
* buf = edt_last_buffer_timed(edt_p, timestamp);
* @endcode
*
* @return Address of the image.
* @see edt_wait_for_buffers, edt_last_buffer, edt_wait_buffers_timed
*/

unsigned char *
edt_last_buffer_timed(EdtDev * edt_p, u_int * timep)
{
    bufcnt_t     donecount;
    bufcnt_t     last_wait;
    int     delta;

    donecount = edt_done_count(edt_p);
    last_wait = edt_p->donecount;

    edt_msg(EDTDEBUG, "edt_last_buffer_timed() last %d cur %d\n",
        last_wait, donecount);

    delta = donecount - last_wait;

    if (delta == 0)
        delta = 1;

    return edt_wait_buffers_timed(edt_p, delta, timep);
}

char   *
edt_timestring(u_int * timep)
{
    static char timestr[100];
    struct tm *tm_p;
    time_t testtm ;

    /* timep is sec,nsec in two 32 bit ints */
    /* works when passed to localtime on 32 bit system, but not 64 */
    testtm = timep[0] ;
    tm_p = localtime(&testtm);
    timep++;
    sprintf(timestr, "%02d:%02d:%02d.%06d",
        tm_p->tm_hour,
        tm_p->tm_min,
        tm_p->tm_sec,
        *timep);

    return timestr;
}

/**
* Causes the driver to perform the same actions as it would on a
* timeout (causing partially filled fifos to be flushed and dma to be
* aborted). Used when the application has knowledge that no more data
* will be sent/accepted. Used when a common timeout cannot be known,
* such as when acquiring data from a telescope ccd array where the
* amount of data sent depends on unknown future celestial events. Also
* used by the library when the operating system can not otherwise wait
* for an interrupt and timeout at the same time.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @return 0 on success; -1 on failure
* @see ring buffer discussion
*/

int
edt_do_timeout(EdtDev *edt_p)

{
    int dummy;

    return edt_ioctl(edt_p,EDTS_DOTIMEOUT,&dummy);

}

/**
* Increments the driver's timeout count without performing any of the
* other actions associated with timeouts. Implemented specifically
* for use by framesync (e.g. #pdv_framesync_mode) when set for
* the EMULATE_TIMEOUTS.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @return 0 on success; -1 on failure
* @see ring buffer discussion
*/

int
edt_inc_timeout(EdtDev *edt_p)
{
    int dummy;

    return edt_ioctl(edt_p,EDTS_INCTIMEOUT,&dummy);

}

/**
* Blocks until the specified number of buffers have completed.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*        count:    how many buffers to block for
* @param count How many buffers to block for. Completed buffers are
* numbered relatively; start each call with 1.
*
*
* @return Address of last completed buffer on success; NULL on error.
* If an error occurs, call #edt_perror to get the system error
* message.
*
* @note If using timeouts, call #edt_timeouts after
* #edt_wait_for_buffers returns to see if the number of timeouts has
* incremented. If it has incremented, call #edt_get_timeout_count to
* get the number of bytes transferred into the buffer. DMA does not
* automatically continue on to the next buffer, so you need to call
* #edt_start_buffers to move on to the next buffer in the ring.
*
* @note If the ring buffer is in free-running mode and the application
* cannot process data as fast as it is acquired, DMA will wrap around
* and overwrite the referenced buffer. The application must ensure that
* the data in the buffer is processed or copied out in time to prevent
* overrun.
*/

unsigned char *
edt_wait_for_buffers(EdtDev * edt_p, int count)
{
    int     bufnum;
    int     ret = 0;
    bufcnt_t tmpcnt = count ;
#ifdef _NT_
    int timeoutval = edt_get_rtimeout(edt_p);
#endif

    edt_msg(EDTDEBUG, "edt_wait_for_buffers(%d)\n", count);

    if (edt_p->ring_buffer_numbufs == 0)
    {
        edt_msg(EDTDEBUG, "wait for buffers called with 0 buffers\n");
        return (0);
    }
    tmpcnt += edt_p->donecount;

#ifdef _NT_

    edt_p->last_wait_ret = EDT_WAIT_OK;

    if (edt_get_drivertype(edt_p) != EDT_WDM_DRIVER)

    {
        HANDLE event;
        int rc;


        if (!edt_p->event_funcs[EDT_EVENT_BUF].wait_event)
        {
            edt_get_kernel_event(edt_p, EDT_EVENT_BUF);
        }

        event = edt_p->event_funcs[EDT_EVENT_BUF].wait_event;

        while (edt_done_count(edt_p) < tmpcnt)
        {

            /* wait for event */

            if (timeoutval == 0)
                timeoutval = INFINITE;

            ret = EDT_WAIT_OK;
            edt_clear_wait_status(edt_p);

            rc = WaitForSingleObject(event, timeoutval);

            if (rc == WAIT_TIMEOUT)
            {
                if (edt_get_timeout_ok(edt_p))
                {
                    ret = EDT_WAIT_OK_TIMEOUT;
                }
                else
                {
                    edt_msg(EDTDEBUG, "timeout...\n");

                    /* deal with timeout */

                    edt_do_timeout(edt_p);
                    ResetEvent(event) ;
                    ret = EDT_WAIT_TIMEOUT;
                }
                break;
            }
            else if (edt_had_user_dma_wakeup(edt_p))
            {
                ret = EDT_WAIT_USER_WAKEUP;
                break;

            }
        }
    }
    else
    {
        ret = edt_ioctl(edt_p, EDTS_WAITBUF, &tmpcnt);
    }
#else

    ret = edt_ioctl(edt_p, EDTS_WAITBUF, &tmpcnt);
    ret = edt_get_wait_status(edt_p);

#endif


    edt_msg(EDTDEBUG, "edt_wait_for_buffers %d done ret = %d\n", count,
        ret);
    if (ret  == EDT_WAIT_OK || ret == EDT_WAIT_TIMEOUT)
        edt_p->donecount = tmpcnt;

    /* avoid error if close happened in another thread */

    if (edt_p->ring_buffer_numbufs)
        bufnum = (edt_p->donecount - 1) % edt_p->ring_buffer_numbufs;
    else
        return NULL;

#ifdef VXWORKS
    /* TODO - move this to dma done */
    /* ATTN - not right - should be bytecount? */
    CACHE_DMA_INVALIDATE(edt_p->ring_buffers[bufnum],
        edt_p->rb_control[bufnum].size);
#if defined(AV3_BOARD)
    printf("cache inv %x %x\n",
        edt_p->ring_buffers[bufnum], edt_p->rb_control[bufnum].size);
    bslPpcCacheInv(edt_p->ring_buffers[bufnum], edt_p->rb_control[bufnum].size);
#endif
#endif

    edt_p->last_wait_ret = ret;

    if (edt_p->devid == DMY_ID && edt_p->dmy_wait_for_buffers_callback != NULL)
    {
      edt_p->dmy_wait_for_buffers_callback(edt_p, edt_p->ring_buffers[bufnum]);
    }

    return (edt_p->ring_buffers[bufnum]);

}

/**
* Gets the seconds and nanoseconds timestamp of when dma was completed
* on the buffer specified by \a bufnum. \a bufnum is moduloed by the
* number of buffers in the ring buffer, so it can either be an index,
* or the number of buffers completed.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param timep     pointer to an unsigned integer array;
* @param bufnum    buffer index, or number of buffers completed
*
* \Example
* @code
* int timestamp[2];
* u_int bufnum=edt_done_count(edt_p);
* edt_get_timestamp(edt_p, timestamp, bufnum);
* @endcode
*
* @return 0 on success, -1 on failure.  Fills in timestamp pointed to
* by \a timep.
*
* @todo add code to print the timestamp to example.
*/

int
edt_get_timestamp(EdtDev * edt_p, u_int * timep, u_int bufnum)
{
    /* we return sec and nsec - change to usec */
    u_int   timevals[3];

    timevals[0] = bufnum;
    if (edt_p->devid == DMY_ID)
    {
        u_int inttime ;
        double testtime ;
        timevals[0] = bufnum ;
        testtime = edt_timestamp();
        inttime = (u_int)testtime ;
        testtime -= inttime ;
        *timep++ = inttime ;
        *timep++ = (u_int)(testtime * 1000000.0) ;
    }
    else
    {
        edt_ioctl(edt_p, EDTG_TMSTAMP, timevals);

        edt_msg(EDTDEBUG, "%x %x %x %x ", bufnum,
            timevals[0],
            timevals[1],
            timevals[2]);
        *timep++ = timevals[1];
        *timep++ = timevals[2];
    }

    return (timevals[0]);
}

/**
* Waits for the next buffer that finishes DMA. Depending on how often
* this routine is called, buffers that have already completed DMA might
* be skipped.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @return Returns a pointer to the buffer, or NULL on failure. If an
* error occurs, call #edt_perror to get the system error message.
*/

unsigned char *
edt_wait_for_next_buffer(EdtDev *edt_p)

{
    bufcnt_t count;
    unsigned int bufnum;
#ifdef _NT_
    int timeoutval = edt_get_rtimeout(edt_p);
#endif

    edt_ioctl(edt_p, EDTG_BUFDONE, &count);
    count++;
    if (edt_p->ring_buffer_numbufs == 0) {
        /* maybe they forgot edt_configure_ring_buffers */
        return NULL;
    }
    bufnum = (count - 1) % edt_p->ring_buffer_numbufs;

#ifdef _NT_
    if (edt_get_drivertype(edt_p) != EDT_WDM_DRIVER)

    {
        HANDLE event;
        int rc;

        if (!edt_p->event_funcs[EDT_EVENT_BUF].wait_event)
        {
            edt_get_kernel_event(edt_p, EDT_EVENT_BUF);
        }

        event = edt_p->event_funcs[EDT_EVENT_BUF].wait_event;

        while (edt_done_count(edt_p) < (bufcnt_t) count)
        {

            /* wait for event */

            if (timeoutval == 0)
                timeoutval = INFINITE;

            rc = WaitForSingleObject(event, timeoutval);

            if (rc == WAIT_TIMEOUT)
            {
                edt_msg(EDTDEBUG, "timeout...\n");

                /* deal with timeout */

                edt_do_timeout(edt_p);
                ResetEvent(event) ;

                break;
            }

        }

    }
    else
    {
        edt_ioctl(edt_p, EDTS_WAITBUF, &count);
    }
#else

    edt_ioctl(edt_p, EDTS_WAITBUF, &count);

#endif

    edt_msg(EDTDEBUG, "edt_wait_for_next_buffer %d done\n", count);
    edt_p->donecount = count;
    return (edt_p->ring_buffers[bufnum]);
}


/**
* edt_current_dma_buf
*
* Returns the address of the current active DMA buffer, for linescan
* cameras where the buffer is only partially filled. Note there is a
* possible error if this is called with normal DMA that doesn't time out,
* because the "current" buffer may change between a call to this function
* and the pointer's access.
*
* @param edt_p: device handle returned from edt_open
*
*
*/

unsigned char *
edt_get_current_dma_buf(EdtDev * edt_p)
{
    unsigned int count;
    unsigned int bufnum;
    unsigned int todo ;
    /* find out how many are done */

    edt_ioctl(edt_p, EDTG_BUFDONE, &count);

    todo = edt_get_todo(edt_p);

    /* if a dma is currently started
    * go to next buffer */

    if (todo == 0 || todo > count)
        count ++;

    bufnum = (count - 1) % edt_p->ring_buffer_numbufs;

    return (edt_p->ring_buffers[bufnum]);

}

/**
* Checks whether the specified number of buffers have completed without
* blocking.
*
*
* @param edt_p pointer to edt device structure returned by #edt_open
* @param count  number of buffers. Must be 1 or greater. Four is
* recommended.
*
* @return Returns the address of the ring buffer corresponding to count
* if it has completed DMA, or NULL if count buffers are not yet
* complete.
*
* @note If the ring buffer is in free-running mode and the application
* cannot process data as fast as it is acquired, DMA will wrap around
* and overwrite the referenced buffer. The application must ensure that
* the data in the buffer is processed or copied out in time to prevent
* overrun.
*/

unsigned char *
edt_check_for_buffers(EdtDev *edt_p, uint_t count)
{
    unsigned int driver_count;
    unsigned int bufnum;
    unsigned int target;

    target = count + edt_p->donecount;
    bufnum = (target - 1) % edt_p->ring_buffer_numbufs;
    edt_ioctl(edt_p, EDTG_BUFDONE, &driver_count);
    if (driver_count >= target)
    {
        return edt_wait_for_buffers(edt_p, count);
    }
    else
        return (NULL);
}

/**
* Returns the cumulative count of completed buffer transfers in ring
* buffer mode.
*
* @param edt_p pointer to edt device structure returned by #edt_open
*
* @return The number of completed buffer transfers. Completed buffers
* are numbered consecutively starting with 0 when
* #edt_configure_ring_buffers is invoked. The index of the ring buffer
* most recently completed by the driver equals the number returned
* modulo the number of ring buffers. -1 is returned if ring buffer mode
* is not configured. If an error occurs, call #edt_perror to get the
* system error message.
*/

bufcnt_t
edt_done_count(EdtDev * edt_p)
{
    bufcnt_t  donecount = 0;

    if (edt_p->devid == DMY_ID)
        donecount = edt_p->dmy_started;
    else
        edt_ioctl(edt_p, EDTG_BUFDONE, &donecount);
    return (donecount);
}

uint_t
edt_overflow(EdtDev * edt_p)
{
    uint_t  overflow;

    edt_ioctl(edt_p, EDTG_OVERFLOW, &overflow);
    return (overflow);
}


/**
* Formats and prints a system error
*
* @param errstr     Error string to include in printed error output.
*
* @see edt_errno for an example
*
* @internal
* \warning Obsolete -- use edt_msg_perror(...) inside edt library
* functions.
*/
void
edt_perror(char *errstr)
{
#ifdef _NT_
    LPVOID  lpMsgBuf;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),      /* Default language */
        (LPTSTR) & lpMsgBuf,
        0,
        NULL
        );

    edt_msg(EDTWARN, "%s: %s\n", errstr, lpMsgBuf);
#else
    edt_msg_perror(EDTWARN, errstr);
#endif
}

/**
* Returns an operating system-dependent error number.
*
* @return 32-bit integer representing the operating system-dependent
* error number generated by an error.
*
* \Example
* @code
* if ((edt_p = edt_open("pcd", 0)) == NULL)
* {
*     edt_perror("edt_open failed");
*     exit(edt_errno());
* }
* @endcode
*/

u_int
edt_errno(void)
{
#ifdef _NT_
    return GetLastError();
#else
    extern int errno;
    return errno;
#endif
}

/**
* Reads the specified register and returns its value.
* Use this routine instead of using ioctls.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param desc  The name of the register to read. Use the names provided
* in the register descriptions in Hardware Addendum for the card you
* are using (e.g. "PCI DV C-Link Hardware Addendum").
*
* @return The value of register.
*
* @todo add example of usage of this function and related macros.
* @todo add a link to edt.com/manuals
*/

uint_t
edt_reg_read(EdtDev * edt_p, uint_t desc)
{
    edt_buf buf = {0, 0};
    buf.desc = desc;

    if (edt_p->devid == DMY_ID && edt_p->dmy_reg_read_callback != NULL)
      buf.value = edt_p->dmy_reg_read_callback(edt_p, desc);
    else
    {
      int ret = edt_ioctl(edt_p, EDTG_REG, &buf);

      if (ret < 0)
            return ret;
    }

    if (dump_reg_access)
    {
      if (is_reg_dumpable(desc))
        printf("%d: reg read  %08x %08x\n", edt_p->channel_no, desc, (uint_t) buf.value);
    }

    return (u_int) buf.value;
}

/**
* Performs a bitwise logical OR of the value of the specified register
* and the value provided in the argument; the result becomes the new
* value of the register. Use this routine instead of using ioctls.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param desc  The name of the register to modify. Use the names
* provided in the register descriptions in Hardware Addendum for the
* card you are using (e.g. "PCI DV C-Link Hardware Addendum").
* @param mask The value to OR with the register.
*
* @return The new value of the register.
*/

uint_t
edt_reg_or(EdtDev * edt_p, uint_t desc, uint_t mask)
{
    int     ret;
    uint_t  val;
    edt_buf buf;

    buf.desc = desc;
    buf.value = mask;
    ret = edt_ioctl(edt_p, EDTS_REG_OR, &buf);
    if (ret < 0)
        edt_msg_perror(EDTFATAL, "edt_ioctl(EDT_REG_OR)");

    if (dump_reg_access)
    {
        if (is_reg_dumpable(desc))
        {
            printf("%d: reg or    %08x %08x\n", edt_p->channel_no,
                        desc, (uint_t) buf.value);
        }
    }
    val = (u_int) buf.value;
    return val;

}

/**
* Performs a bitwise logical AND of the value of the specified register
* and the value provided in the argument; the result becomes the new
* value of the register. Use this routine instead of using ioctls.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param desc  The name of the register to modify. Use the names
* provided in the register descriptions in Hardware Addendum for the
* card you are using (e.g. "PCI DV C-Link Hardware Addendum").
* @param mask The value to AND with the register.
*
* @return The new value of the register
*/

uint_t
edt_reg_and(EdtDev * edt_p, uint_t desc, uint_t mask)
{
    int     ret;
    uint_t  val;
    edt_buf buf;

    buf.desc = desc;
    buf.value = mask;
    ret = edt_ioctl(edt_p, EDTS_REG_AND, &buf);
    if (ret < 0)
        edt_msg_perror(EDTFATAL, "edt_ioctl(EDT_REG_AND)");

    if (dump_reg_access)
    {
        if (is_reg_dumpable(desc))
        {
            printf("%d: reg and   %08x %08x\n", edt_p->channel_no, desc, (uint_t) buf.value);
        }
    }

    val = (u_int) buf.value;
    return val;
}

/**
* Toggles the bits specified in the mask argument off then on in a single
* ioctl call.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param desc  The name of the register to modify. Use the names
* provided in the register descriptions in Hardware Addendum for the
* card you are using (e.g. "PCI DV C-Link Hardware Addendum").
* @param mask The value to XOR with the register.
*/

void
edt_reg_clearset(EdtDev * edt_p, uint_t desc, uint_t mask)
{
    int     ret;
    edt_buf buf;

    buf.desc = desc;
    buf.value = mask;
    ret = edt_ioctl(edt_p, EDTS_REG_BIT_CLEARSET, &buf);
    if (ret < 0)
        edt_msg_perror(EDTFATAL, "edt_ioctl(EDT_REG_BIT_CLEARSET)");

    return;
}

/**
* Toggles the bits specified in the mask argument on then off in a single
* ioctl call.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param desc  The name of the register to modify. Use the names
* provided in the register descriptions in Hardware Addendum for the
* card you are using (e.g. "PCI DV C-Link Hardware Addendum").
* @param mask The value to XOR with the register.
*/
void
edt_reg_setclear(EdtDev * edt_p, uint_t desc, uint_t mask)
{
    int     ret;
    edt_buf buf;

    buf.desc = desc;
    buf.value = mask;
    ret = edt_ioctl(edt_p, EDTS_REG_BIT_SETCLEAR, &buf);
    if (ret < 0)
        edt_msg_perror(EDTFATAL, "edt_ioctl(EDT_REG_BIT_SETCLEAR)");

    return;
}

/**
* Write the specified value to the specified register.
* Use this routine instead of using ioctls.
*
* @note Use this routine with care; it writes directly to the hardware.
* An incorrect value can crash your system, possibly causing loss of
* data.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param desc  The name of the register to write. Use the names
* provided in the register descriptions in Hardware Addendum for the
* card you are using (e.g. "PCI DV C-Link Hardware Addendum").
* @param value The desired value to write in register.
*/

void
edt_reg_write(EdtDev * edt_p, uint_t desc, uint_t value)
{
    if (edt_p->devid == DMY_ID && edt_p->dmy_reg_write_callback != NULL)
      edt_p->dmy_reg_write_callback(edt_p, desc, value);
    else
    {
      int     ret;
      edt_buf buf;

      buf.desc = desc;
      buf.value = value;

      ret = edt_ioctl(edt_p, EDTS_REG, &buf);
      if (ret < 0)
        edt_msg_perror(EDTFATAL, "write");
    }

    if (dump_reg_access)
    {
      if (is_reg_dumpable(desc))
        printf("%d: reg write %08x %08x\n", edt_p->channel_no, desc, (uint_t) value);
    }
}

/**
* Specifies when to perform the action at the start of a dma transfer
* as specified by #edt_startdma_reg. A common use of this is to write
* to a register which signals an external device that dma has started,
* to trigger the device to start sending. The default is no dma action.
* The PDV library uses this function to send a trigger to a camera at
* the start of dma. This function allows the register write to occur in
* a critical section with the start of dma and at the same time.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @param val  One of \c EDT_ACT_NEVER, \c EDT_ACT_ONCE, or \c EDT_ACT_ALWAYS
*
* \Example
* @code
* edt_startdma_action(edt_p, EDT_ACT_ALWAYS);
* edt_startdma_reg(edt_p, PDV_CMD, PDV_ENABLE_GRAB);
* @endcode
*
* @see edt_startdma_reg, edt_reg_write, edt_reg_read
*/
void
edt_startdma_action(EdtDev * edt_p, uint_t val)
{
    (void) edt_ioctl(edt_p, EDTS_STARTACT, &val);
}

/**
* Specifies when to perform the action at the end of a dma transfer as
* specified by #edt_enddma_reg. A common use of this is to write to a
* register which signals an external device that dma is complete, or to
* change the state of a signal which will be changed at the start of
* dma, so the external device can look for an edge. The default is no
* end of dma action. Most applications can set the output signal, if
* needed, from the application with #edt_reg_write. This routine is
* only needed if the action must happen within microseconds of the end
* of dma.
*
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @param val One of \c EDT_ACT_NEVER, \c EDT_ACT_ONCE, or \c EDT_ACT_ALWAYS
*
* \Example
* @code
* u_int fnct_value=0x1;
* edt_enddma_action(edt_p, EDT_ACT_ALWAYS);
* edt_enddma_reg(edt_p, PCD_FUNCT, fnct_value);
* @endcode
*
* @see #edt_startdma_action, #edt_startdma_reg, #edt_reg_write,
* #edt_reg_read
*
* @todo when would EDT_ACT_ONCE be used?
*/

void
edt_enddma_action(EdtDev * edt_p, uint_t val)
{
    (void) edt_ioctl(edt_p, EDTS_ENDACT, &val);
}

/**
* Sets the register and value to use at the start of dma, as set by
* #edt_startdma_action.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param desc     register description of which register to use as in
* \c edtreg.h.
* @param val      value to write
*
* \Example
* @code
* edt_startdma_action(edt_p, EDT_ACT_ALWAYS);
* edt_startdma_reg(edt_p, PDV_CMD, PDV_ENABLE_GRAB);
* @endcode
*
* @see edt_startdma_action
*/

void
edt_startdma_reg(EdtDev * edt_p, uint_t desc, uint_t val)
{
    int     ret;
    edt_buf buf;

    buf.desc = desc;
    buf.value = val;
    ret = edt_ioctl(edt_p, EDTS_STARTDMA, &buf);
    if (ret < 0)
        edt_msg_perror(EDTFATAL, "write");
}

/**
* Sets the register and value to use at the end of dma, as set by
* #edt_enddma_action.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param desc     register description of which register to use as in
* \c edtreg.h.
* @param val      value to write
*
* @see edt_enddma_action for example
*/
void
edt_enddma_reg(EdtDev * edt_p, uint_t desc, uint_t val)
{
    int     ret;
    edt_buf buf;

    buf.desc = desc;
    buf.value = val;
    ret = edt_ioctl(edt_p, EDTS_ENDDMA, &buf);
    if (ret < 0)
        edt_msg_perror(EDTFATAL, "write");
}



/**
* Enables an action where a specified register will be programmed with a
* specified value at the start of a dma read operation.  Enabled with
* EDT_ACT_ALWAYS and disabled with EDT_ACT_NEVER passed to the enable argument.
* A common use of this is to write to a register which signals an external
* device that dma has started, to trigger the device to start sending.
*
* This routine is intended to work with edt_read().  It will not work well
* ring buffers since sequential dma operations are pipelined in hardware
* in the EDT dma engine.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @param enable    EDT_ACT_ALWAYS to enable, EDT_ACT_NEVER to disable.
*
* @param reg_desc  Register access description code.
*
* @param set       Register bits to be set.
* @param clear     Register bits to be cleared.
* @param setclear  Register value to be toggled up then down.
* @param clearset  Register value to be toggled down then up.
*
* \Example
* @code
* edt_read_start_action(edt_p, EDT_ACT_ALWAYS, PCD_FUNCT, 0x8F, 0, 0x10, 0);
* edt_read_start_action(edt_p, EDT_ACT_NEVER, dummy, dummy, dummy, dummy);
* @endcode
*
* @see edt_read_end_action(), edt_write_start_action(), edt_write_end_action()
*/
void
edt_read_start_action(EdtDev * edt_p, u_int enable, u_int reg_desc,
                      u_char set, u_char clear, u_char setclear, u_char clearset,
                      int delay1, int delay2)
{
    edt_buf buf;
    u_int delays;

    delays = ((delay1 << 16) & 0xffff0000) | (delay2 & 0xffff);
    (void) edt_ioctl(edt_p, EDTS_READ_START_DELAYS, &delays);

    buf.flags = enable;
    buf.desc  = reg_desc;
    buf.value = set | (clear << 8) | (setclear << 16) | (clearset << 24);

    (void) edt_ioctl(edt_p, EDTS_READ_STARTACT, &buf);
}



/**
* Enables an action where a specified register will be programmed with a
* specified value at the end of a dma read operation.  Enabled with
* EDT_ACT_ALWAYS and disabled with EDT_ACT_NEVER passed to the enable argument.
* A common use of this is to write to a register which signals an external
* device that dma has ended to notify the device to stop sending.
*
* This routine is intended to work with edt_read().  It will not work well
* ring buffers since sequential dma operations are pipelined in hardware
* in the EDT dma engine.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @param enable    EDT_ACT_ALWAYS to enable, EDT_ACT_NEVER to disable.
*
* @param reg_desc  Register access description code.
*
* @param set       Register bits to be set.
* @param clear     Register bits to be cleared.
* @param setclear  Register value to be toggled up then down.
* @param clearset  Register value to be toggled down then up.
*
* \Example
* @code
* edt_read_end_action(edt_p, EDT_ACT_ALWAYS, PCD_FUNCT, 0x8F, 0, 0x10, 0);
* edt_read_end_action(edt_p, EDT_ACT_NEVER, dummy, dummy, dummy, dummy);
* @endcode
*
* @see edt_read_start_action(), edt_write_start_action(), edt_write_end_action()
*/
void
edt_read_end_action(EdtDev * edt_p, u_int enable, u_int reg_desc,
                    u_char set, u_char clear, u_char setclear, u_char clearset,
                    int delay1, int delay2)
{
    edt_buf buf;
    u_int delays;

    delays = ((delay1 << 16) & 0xffff0000) | (delay2 & 0xffff);
    (void) edt_ioctl(edt_p, EDTS_READ_END_DELAYS, &delays);

    buf.flags = enable;
    buf.desc  = reg_desc;
    buf.value = set | (clear << 8) | (setclear << 16) | (clearset << 24);

    (void) edt_ioctl(edt_p, EDTS_READ_ENDACT, &buf);
}



/**
* Enables an action where a specified register will be programmed with a
* specified value at the start of a dma write operation.  Enabled with
* EDT_ACT_ALWAYS and disabled with EDT_ACT_NEVER passed to the enable argument.
* A common use of this is to write to a register which signals an external
* device that dma has started, to trigger the device to start sending.
*
* This routine is intended to work with edt_write().  It will not work well
* ring buffers since sequential dma operations are pipelined in hardware
* in the EDT dma engine.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @param enable    EDT_ACT_ALWAYS to enable, EDT_ACT_NEVER to disable.
*
* @param reg_desc  Register access description code.
*
* @param set       Register bits to be set.
* @param clear     Register bits to be cleared.
* @param setclear  Register value to be toggled up then down.
* @param clearset  Register value to be toggled down then up.
*
* \Example
* @code
* edt_write_start_action(edt_p, EDT_ACT_ALWAYS, PCD_FUNCT, 0x8F, 0, 0x10, 0);
* edt_write_start_action(edt_p, EDT_ACT_NEVER, dummy, dummy, dummy, dummy);
* @endcode
*
* @see edt_write_end_action(), edt_read_start_action(), edt_read_end_action()
*/
void
edt_write_start_action(EdtDev * edt_p, u_int enable, u_int reg_desc,
                       u_char set, u_char clear, u_char setclear, u_char clearset,
                       int delay1, int delay2)
{
    edt_buf buf;
    u_int delays;

    delays = ((delay1 << 16) & 0xffff0000) | (delay2 & 0xffff);
    (void) edt_ioctl(edt_p, EDTS_WRITE_START_DELAYS, &delays);

    buf.flags = enable;
    buf.desc  = reg_desc;
    buf.value = set | (clear << 8) | (setclear << 16) | (clearset << 24);

    (void) edt_ioctl(edt_p, EDTS_WRITE_STARTACT, &buf);
}



/**
* Enables an action where a specified register will be programmed with a
* specified value at the end of a dma write operation.  Enabled with
* EDT_ACT_ALWAYS and disabled with EDT_ACT_NEVER passed to the enable argument.
* A common use of this is to write to a register which signals an external
* device that dma has ended to notify the device to stop sending.
*
* This routine is intended to work with edt_write().  It will not work well
* ring buffers since sequential dma operations are pipelined in hardware
* in the EDT dma engine.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @param enable    EDT_ACT_ALWAYS to enable, EDT_ACT_NEVER to disable.
*
* @param reg_desc  Register access description code.
*
* @param set       Register bits to be set.
* @param clear     Register bits to be cleared.
* @param setclear  Register value to be toggled up then down.
* @param clearset  Register value to be toggled down then up.
*
* \Example
* @code
* edt_write_end_action(edt_p, EDT_ACT_ALWAYS, PCD_FUNCT, 0x8F, 0, 0x10, 0);
* edt_write_end_action(edt_p, EDT_ACT_NEVER, dummy, dummy, dummy, dummy);
* @endcode
*
* @see edt_write_start_action(), edt_read_start_action(), edt_read_end_action()
*/
void
edt_write_end_action(EdtDev * edt_p, u_int enable, u_int reg_desc,
                     u_char set, u_char clear, u_char setclear, u_char clearset,
                     int delay1, int delay2)
{
    edt_buf buf;
    u_int delays;

    delays = ((delay1 << 16) & 0xffff0000) | (delay2 & 0xffff);
    (void) edt_ioctl(edt_p, EDTS_WRITE_END_DELAYS, &delays);

    buf.flags = enable;
    buf.desc  = reg_desc;
    buf.value = set | (clear << 8) | (setclear << 16) | (clearset << 24);

    (void) edt_ioctl(edt_p, EDTS_WRITE_ENDACT, &buf);
}



/**
* A convenience routine, partly for backward compatability, to access
* the user interface XILINX registers.  The register descriptors used by
* #edt_reg_read can also be used, since edt_intfc_read masks off the
* offset.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param offset    integer offset into user interface XILINX, or
* #edt_reg_read style register descriptor
*
* @return The value of the 8 bit register.
*
* \Example
* u_char func_reg = edt_intfc_read(edt_p, PCD_FUNC);
*
* @see edt_intfc_write, edt_reg_read, edt_intfc_read_short
* @todo: for backwords compat, or for convenience?
*/

u_char
edt_intfc_read(EdtDev * edt_p, uint_t offset)
{
    u_int   read;

    read = edt_reg_read(edt_p, INTFC_BYTE | (offset & 0xffff));
    return ((u_char) read & 0xff);
}

/**
* A convenience routine, partly for backward compatability, to access
* the user interface XILINX registers.  The register descriptors used by
* #edt_reg_write can also be used, since edt_intfc_write masks off the
* offset.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param offset    integer offset into user interface XILINX, or
* #edt_reg_write style register descriptor.
* @param data      unsigned character value to set
*
* \Example
* @code
* u_char fnct1=1;
* edt_intfc_write(edt_p, PCD_FUNCT, fnct1);
* @endcode
*
* @see edt_intfc_read, edt_reg_write, edt_intfc_write_short
*/

void
edt_intfc_write(EdtDev * edt_p, uint_t offset, u_char data)
{
    edt_reg_write(edt_p, INTFC_BYTE | (offset & 0xffff), data);
}

/**
* A convenience routine, partly for backward compatability, to access
* the user interface XILINX registers.  The register descriptors used
* by #edt_reg_read can also be used, since edt_intfc_read_short masks
* off the offset.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @param offset    integer offset into user interface XILINX, or
* #edt_reg_read style register descriptor
*
* @return          Value of the 16 bit register.
*
* \Example
* @code
* int i;
* puts("Directions for each channel of 16-channel card using
* user interface xilinx 'ssdio.bit':");
* u_short channel_direction_reg = edt_intfc_read_short(edt_p, SSD16_CHDIR);
* for (i = 0; i < 16; ++i) {
*     int dir = channel_dir_reg & (1 << i);
*          printf("Channel %d configured for: ", i);
*     if (dir == 0) {
*         printf("input\n");
*     } else if (dir == 1) {
*         printf("output");
*     }
* }
* @endcode
*
* @see edt_intfc_read, edt_reg_read
*/

u_short
edt_intfc_read_short(EdtDev * edt_p, uint_t offset)
{
    u_int   read;

    read = edt_reg_read(edt_p, INTFC_WORD | (offset & 0xffff));
    return (read & 0xffff);
}

/**
* A convenience routine, partly for backward compatability, to access
* the user interface XILINX registers.  The register descriptors used by
* edt_reg_write() can also be used, since edt_intfc_write_short masks
* off the offset.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param offset:     integer offset into user interface XILINX, or
* #edt_reg_write style register descriptor
* @param data        unsigned short integer value to set
*
* \Example
* @code
* puts("Enabling all 16 DMA channels on PCDa with 'ssdio.bit'
* loaded in user interface xilinx");
* edt_intfc_write_short(edt_p, SSD16_CHEN, 0xffff);
* @endcode
*
* @see edt_intfc_write, edt_reg_write
*/

void
edt_intfc_write_short(EdtDev * edt_p, uint_t offset, u_short data)
{
    edt_reg_write(edt_p, INTFC_WORD | (offset & 0xffff), data);
}

/**
* A convenience routine, partly for backward compatability, to access
* the user interface XILINX registers.  The register descriptors used be
* #edt_reg_read can also be used, since edt_intfc_read_32 masks off
* the offset.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param offset     integer offset into the user interface XILINX, or
* #edt_reg_read style register descriptor.
*
* @return The value of the 32 bit register.
*/

uint_t
edt_intfc_read_32(EdtDev * edt_p, uint_t offset)
{
    uint_t  read;

    read = edt_reg_read(edt_p, INTFC_32 | (offset & 0xffff));
    return (read);
}

/**
* A convenience routine, partly for backward compatability, to access
* the user interface XILINX registers.  The register descriptors used
* by #edt_reg_write can also be used, since edt_intfc_write_32 masks
* off the offset.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param offset    integer offset into user interface XILINX, or
* #edt_reg_write style register descriptor
* @param data      The 32 bit value to set the register to.
*
* @see edt_intfc_read32, edt_reg_write
*/

void
edt_intfc_write_32(EdtDev * edt_p, uint_t offset, uint_t data)
{
    edt_reg_write(edt_p, INTFC_32 | (offset & 0xffff), data);
}


int
edt_set_rci_chan(EdtDev *edt_p, int unit, int channel)
{
    int ret ;
    edt_buf buf;
    buf.desc = unit;
    buf.value = channel;
    edt_msg(EDTDEBUG, "set rci unit %d chan %d\n",unit,channel) ;
    ret = edt_ioctl(edt_p, EDTS_RCI_CHAN, &buf);
    return (ret) ;
}

int
edt_get_rci_chan(EdtDev *edt_p, int unit)
{
    int channel ;
    edt_buf buf;
    buf.desc = unit;
    edt_ioctl(edt_p, EDTG_RCI_CHAN, &buf);
    channel = (u_int) buf.value ;
    edt_msg(EDTDEBUG, "get rci unit %d chan %d\n",unit,channel) ;
    return (channel) ;
}

int
edt_set_rci_dma(EdtDev * edt_p, int unit, int channel)

{
    char msg[5];
    sprintf(msg,"D %d",channel);

    edt_send_msg(edt_p,unit,msg,3);
    edt_set_rci_chan(edt_p, unit, channel) ;

    return 0;
}

int
edt_get_rci_dma(EdtDev * edt_p, int unit)

{
    char msgbuf[5];
    int channel;
    msgbuf[1] = 0;

    edt_send_msg(edt_p, unit, "D", 1);
    edt_serial_wait(edt_p, 100, 0) ;
    edt_get_msg(edt_p, msgbuf, sizeof(msgbuf));

    channel = atoi(&msgbuf[2]);

    return channel;

}

/**
* Sets the burst enable flag, determining whether the DMA master
* transfers as many words as possible at once, or transfers them one at
* a time as soon as the data is acquired. Burst transfers are enabled
* by default to optimize use of the bus; however, you may wish to
* disable them if data latency is an issue, or for diagnosing DMA
* problems.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param onoff a value of 1 turns the flag on (the default); 0 turns it
* off.
*/

int
edt_set_burst_enable(EdtDev * edt_p, int onoff)
{
    return (edt_ioctl(edt_p, EDTS_BURST_EN, &onoff));
}


/**
* Returns the value of the burst enable flag, determining whether the
* DMA master transfers as many words as possible at once, or transfers
* them one at a time as soon as the data is acquired. Burst transfers
* are enabled by default to optimize use of the bus. For more
* information, see #edt_set_burst_enable.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @return A value of 1 if burst transfers are enabled; 0 otherwise.
*/

int
edt_get_burst_enable(EdtDev * edt_p)
{
    int     val;

    edt_ioctl(edt_p, EDTG_BURST_EN, &val);
    return (val);
}

int
edt_set_continuous(EdtDev * edt_p, int val)
{
#ifdef PDV
    u_int tmp ;
    tmp = edt_p->dd_p->datapath_reg ;
    /* tmp = edt_reg_read(edt_p, PDV_DATA_PATH) & ~PDV_CONTINUOUS ;*/
    edt_ioctl(edt_p, EDTS_PDVDPATH, &tmp);
    return (edt_ioctl(edt_p, EDTS_PDVCONT, &val));
#else
    return 0;
#endif
}

void
edt_reset_counts(EdtDev * edt_p)
{
    int     dmy;

    edt_ioctl(edt_p, EDTS_RESETCOUNT, &dmy);
}

void
edt_reset_serial(EdtDev * edt_p)
{
    int     dmy;

    dmy = edt_p->channel_no ;
    edt_ioctl(edt_p, EDTS_RESETSERIAL, &dmy);
}


void
edt_flush_channel(EdtDev *edt_p, int channel)

{
    u_int dma_cfg = edt_reg_read(edt_p, EDT_DMA_CFG) ;

    dma_cfg &= ~(EDT_EMPTY_CHAN | EDT_EMPTY_CHAN_FIFO) ;

    /*
    * Set the channel number and enable the rfifo and related logic.
    */
    dma_cfg |= ((channel << EDT_EMPTY_CHAN_SHIFT) | EDT_RFIFO_ENB) ;
    edt_reg_write(edt_p, EDT_DMA_CFG, dma_cfg) ;

    /*
    * Set then reset the channel fifo flush bit.
    */
    dma_cfg |= EDT_EMPTY_CHAN_FIFO ;
    edt_reg_write(edt_p, EDT_DMA_CFG, dma_cfg) ;

    dma_cfg &= ~EDT_EMPTY_CHAN_FIFO ;
    edt_reg_write(edt_p, EDT_DMA_CFG, dma_cfg) ;


}

#if defined( PCD )

u_char
pcd_get_option(EdtDev * edt_p)
{
    return (edt_intfc_read(edt_p, PCD_OPTION));
}

u_char
pcd_get_cmd(EdtDev * edt_p)
{
    return (edt_intfc_read(edt_p, PCD_CMD));
}

void
pcd_set_cmd(EdtDev * edt_p, u_char val)
{
    edt_intfc_write(edt_p, PCD_CMD, val);
}

u_char
pcd_get_funct(EdtDev * edt_p)
{
    return (edt_intfc_read(edt_p, PCD_FUNCT));
}

void
pcd_set_funct(EdtDev * edt_p, u_char val)
{
    edt_intfc_write(edt_p, PCD_FUNCT, val);
}

u_char
pcd_get_stat(EdtDev * edt_p)
{
    return ((u_char) edt_intfc_read(edt_p, PCD_STAT));
}

u_char
pcd_get_stat_polarity(EdtDev * edt_p)
{
    return (edt_intfc_read(edt_p, PCD_STAT_POLARITY));
}

void
pcd_set_stat_polarity(EdtDev * edt_p, u_char val)
{
    u_char polarity = edt_intfc_read(edt_p, PCD_STAT_POLARITY);

    polarity &= 0xF0;
    polarity |= val;

    edt_intfc_write(edt_p, PCD_STAT_POLARITY, polarity);
}

void
pcd_set_byteswap(EdtDev * edt_p, int val)
{
    u_char  tmp;

    tmp = edt_intfc_read(edt_p, PCD_CONFIG);
    if (val)
        tmp |= PCD_BYTESWAP;
    else
        tmp &= ~PCD_BYTESWAP;
    edt_intfc_write(edt_p, PCD_CONFIG, tmp);
}



void
pcd_flush_channel(EdtDev * edt_p, int channel)
{
    u_short ssd16_chen = edt_reg_read(edt_p, SSD16_CHEN) ;

    ssd16_chen &= ~(1 << channel);
    edt_reg_write(edt_p, SSD16_CHEN, ssd16_chen) ;

    edt_flush_channel(edt_p,channel);


    ssd16_chen |= (1 << channel);
    edt_reg_write(edt_p, SSD16_CHEN, ssd16_chen) ;

}

/*****************************************************************************
*
* pio routines:  programmed I/O for PCD DMA boards.
*
*****************************************************************************/

void
pcd_pio_init(EdtDev *edt_p)
{
    static u_char * mapaddr = 0 ;

    if (mapaddr == 0)
    {
        mapaddr = (u_char *) edt_mapmem(edt_p, 0, 256) ;
        edt_p->reg_fifo_io  = (u_int *)(mapaddr + (EDT_GP_OUTPUT & 0xff)) ;
        edt_p->reg_fifo_cnt  = (u_char *)(mapaddr + (EDT_DMA_CFG & 0xff) + 3) ;
        edt_p->reg_fifo_ctl = (u_char *)(mapaddr + (EDT_DMA_CFG & 0xff) + 1) ;
        edt_p->reg_intfc_off  = (volatile u_char *) mapaddr + (EDT_REMOTE_OFFSET & 0xff) ;
        edt_p->reg_intfc_dat = (volatile u_char *) mapaddr + (EDT_REMOTE_DATA & 0xff) ;
    }
    pcd_pio_intfc_write(edt_p, PCD_DIRB, 0xcc);
    pcd_pio_flush_fifo(edt_p) ;
}

void
pcd_pio_intfc_write(EdtDev *edt_p, u_int desc, u_char val)
{
    int dmy ;

    if (edt_p->reg_intfc_off == NULL)
        return;

    *edt_p->reg_intfc_off = desc & 0xff ;
    dmy = *edt_p->reg_intfc_off ;

    *edt_p->reg_intfc_dat = val ;
    dmy = *edt_p->reg_intfc_dat ;
}

u_char
pcd_pio_intfc_read(EdtDev *edt_p, u_int desc)
{
    int dmy ;

    if (edt_p->reg_intfc_off == NULL)
        return 0;

    *edt_p->reg_intfc_off = desc & 0xff ;
    dmy = *edt_p->reg_intfc_off ;

    return(*edt_p->reg_intfc_dat) ;
}



void
pcd_pio_set_direction(EdtDev *edt_p, int direction)
{
    u_char  cmd = pcd_pio_intfc_read(edt_p, PCD_CMD) ;

    if (direction)
    {
        pcd_pio_intfc_write(edt_p, PCD_DIRA, 0x0f);

        cmd &= ~PCD_DIR;
        cmd |= PCD_ENABLE;

    }
    else
    {
        pcd_pio_intfc_write(edt_p, PCD_DIRA, 0xf0);

        cmd |= PCD_DIR | PCD_ENABLE ;
    }

    pcd_pio_intfc_write(edt_p, PCD_CMD, cmd);
}

static void
pcd_pio_quick_flush(EdtDev *edt_p)
{
    u_char tmpc ;

    if (edt_p->reg_fifo_ctl == NULL)
        return;

    tmpc = *edt_p->reg_fifo_ctl ;
    tmpc &= ~1 ;
    *edt_p->reg_fifo_ctl = tmpc ;
    tmpc |= 1 ;
    *edt_p->reg_fifo_ctl = tmpc ;
}


void
pcd_pio_flush_fifo(EdtDev * edt_p)
{
    unsigned char cmd;
    unsigned int dmy;

    if (edt_p->reg_intfc_off == NULL)
        return;

    /* reset the interface fifos */
    *edt_p->reg_intfc_off = PCD_CMD & 0xff ;
    dmy = *edt_p->reg_intfc_off ;
    cmd = *edt_p->reg_intfc_dat ;
    cmd &= ~PCD_ENABLE;
    *edt_p->reg_intfc_dat = cmd ;
    dmy = *edt_p->reg_intfc_dat ;
    cmd |= PCD_ENABLE;
    *edt_p->reg_intfc_dat = cmd ;
    dmy = *edt_p->reg_intfc_dat ;

    pcd_pio_quick_flush(edt_p) ;

}

int
pcd_pio_write(EdtDev *edt_p, u_char *buf, int size)
{
    int i;
    u_int *tmpl = (u_int *) buf ;

    if (edt_p->reg_fifo_io == NULL)
        return -1;

    pcd_pio_set_direction(edt_p, EDT_WRITE) ;

    for (i = 0 ; i < size / 4 ; i++)
        *edt_p->reg_fifo_io = *tmpl++ ;

    return(size);
}

int
pcd_pio_read(EdtDev *edt_p, u_char *buf, int size)
{
    u_int *tmpl = (u_int *) buf ;
    int wordcnt = 0 ;
    int words_avail ;
    int words_requested = size / 4 ;

    if (edt_p->reg_fifo_io == NULL)
        return -1;

    pcd_pio_set_direction(edt_p, EDT_READ) ;

    while (wordcnt < words_requested)
    {
        words_avail = *edt_p->reg_fifo_cnt & 0xf ;

        while (words_avail && wordcnt < words_requested)
        {
            -- words_avail ;
            *tmpl = *edt_p->reg_fifo_io ;
            wordcnt++ ;
            tmpl++ ;
        }

        if ((wordcnt == words_requested - 1) && (*edt_p->reg_fifo_cnt & 0x1f) == 0x10)
        {
            *tmpl = *edt_p->reg_fifo_io ;
            wordcnt++ ;
        }
    }

    pcd_pio_quick_flush(edt_p);

    return(size) ;
}

void
pcd_set_abortdma_onintr(EdtDev *edt_p, int flag)
{
    if (flag == 0)
    {
        edt_reg_and(edt_p, PCD_CMD, ~PCD_STAT_INT_1) ;
    }

    edt_ioctl(edt_p, EDTS_ABORTDMA_ONINTR, &flag) ;

    if (flag)
    {
        edt_reg_or(edt_p, PCD_CMD, PCD_STAT_INT_1 | PCD_ENABLE) ;
        edt_reg_or(edt_p, PCD_STAT_POLARITY, PCD_STAT_INT_ENA) ;
        edt_set_remote_intr(edt_p, TRUE);
    }
}

#endif                          /* PCD */



/**
* Flushes the board's input and output FIFOs, to allow new data
* transfers to start from a known state.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*/

void
edt_flush_fifo(EdtDev * edt_p)
{
    uint_t  tmp;
    unsigned char cmd;
    unsigned char cfg;
    unsigned int dmy;

    edt_msg(EDTDEBUG, "edt_flush_fifo\n") ;
    /* Turn off the PCI fifo */
    tmp = edt_reg_read(edt_p, EDT_DMA_INTCFG);
    tmp &= (~EDT_RFIFO_ENB);
    edt_reg_write(edt_p, EDT_DMA_INTCFG, tmp);
    dmy = edt_reg_read(edt_p, EDT_DMA_INTCFG);

    if (edt_is_pcd(edt_p) || edt_p->devid == PDVAERO_ID)
    {
        /* reset the interface fifos */
        cmd = edt_intfc_read(edt_p, PCD_CMD);
        cmd &= ~PCD_ENABLE;
        edt_intfc_write(edt_p, PCD_CMD, cmd);
        dmy = edt_intfc_read(edt_p, PCD_CMD);
        cmd |= PCD_ENABLE;
        edt_intfc_write(edt_p, PCD_CMD, cmd);
        dmy = edt_intfc_read(edt_p, PCD_CMD);
    }
    else if (edt_is_pdv(edt_p))
    {
        int cont = edt_reg_read(edt_p, PDV_DATA_PATH) & PDV_CONTINUOUS;

        cfg = edt_intfc_read(edt_p, PDV_CFG);
        cfg &= ~PDV_FIFO_RESET;
        edt_intfc_write(edt_p, PDV_CFG, (u_char) (cfg | PDV_FIFO_RESET));
        edt_intfc_write(edt_p, PDV_CMD, PDV_RESET_INTFC);
        edt_intfc_write(edt_p, PDV_CFG, cfg);

        if (cont)
            edt_reg_or(edt_p,PDV_DATA_PATH, PDV_CONTINUOUS);
    }


    /* Turn     on the PCI fifos, which flushes them */
    tmp |= (EDT_RFIFO_ENB);
    edt_reg_write(edt_p, EDT_DMA_INTCFG, tmp);
    dmy = edt_reg_read(edt_p, EDT_DMA_INTCFG);

}

/**
* OBSOLETE: Use edt_get_bufbytecount(edt_p, &bufnum) instead.
*
* Obsoleted 04/2013 in favor of edt_get_buf_bytecount since it fails to
* identify offset and buffer atomically.
*
* Returns the number of bytes read so far into the current buffer.
* Can be used to monitor how much data has been read into the buffer
* during acquisition.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @return The number of bytes transferred, as described above.
*
*/

uint_t
edt_get_bytecount(EdtDev * edt_p)
{
    uint_t  tmp;

    if (edt_ioctl(edt_p, EDTG_BYTECOUNT, &tmp) < 0)
        edt_msg_perror(EDTFATAL, "edt_ioctl(EDTG_BYTECOUNT)");

    return (tmp);
}

/**
* Returns the number of bytes transferred at last timeout.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @return The number of bytes transferred at last timeout.
*/

uint_t
edt_get_timeout_count(EdtDev * edt_p)
{
    uint_t  tmp;

    if (edt_ioctl(edt_p, EDTG_TIMECOUNT, &tmp) < 0)
        edt_msg_perror(EDTFATAL, "edt_ioctl(EDTG_TIMECOUNT)");

    return (tmp);
}
/**
* Gets the value of the PCD_DIRA and PCD_DIRB registers.
* The value from PCD_DIRB is shifted up 8 bits.
*/
unsigned short
edt_get_direction(EdtDev * edt_p)
{
    unsigned short dirreg;

    /* Set the direction register for DMA output */
    dirreg = edt_intfc_read(edt_p, PCD_DIRA);
    dirreg |= edt_intfc_read(edt_p, PCD_DIRB) << 8;
    return (dirreg);
}

/**
* @if edtinternal
* Cancels DMA right now, without resetting any counters.
* User applications should use #edt_abort_dma instead.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @endif
*/
int
edt_cancel_current_dma(EdtDev * edt_p)
{
    unsigned int finish_current = 0;

    return edt_ioctl(edt_p, EDTS_STOPBUF, &finish_current);
}


/**
* On PCD cards, sets DMA direction to read or write.
*
* Most users will not need to use this function, but instead can just
* set the direction when calling #edt_configure_ring_buffers.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @param direction one of EDT_READ or EDT_WRITE
*/
void
edt_set_direction(EdtDev * edt_p, int direction)
{

    u_int tmp = edt_reg_read(edt_p, EDT_DMA_INTCFG);

    if (edt_is_pcd(edt_p) || edt_p->devid == PDVAERO_ID)
    {
        edt_ioctl(edt_p, EDTS_DIRECTION, &direction);
    }

    tmp |= (EDT_RFIFO_ENB);
    edt_reg_write(edt_p, EDT_DMA_INTCFG, tmp);

    edt_p->last_direction = (direction) ? 2: 1;

}

/*
* EDT IOCTL Interface Routines
*
* DESCRIPTION General   Purpose device control layer for EDT PCI and SBus
* interface products
*
* All routines access a specific device, whose handle is created and returned
* by    the <device>_open() routine.
*/


#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif


/**
* edt_ioctl
*
* DESCRIPTION initiate an action in Edt_Dev driver, without getting or setting
* anything
*
* ARGUMENTS: fd file descriptor from DEV_open() routine action  action to
* take -- mustbe XXG_ action, defined in edt_ioctl.h
*/
int
edt_ioctl(EdtDev * edt_p, int code, void *arg)
{
    int     get, set;
    int     size;
    edt_ioctl_struct eis;
#ifdef __APPLE__
    int     ret ;
#endif

#if 0 /* produces too much output for the normal case, but may still be useful...*/
    edt_msg(EDTDEBUG, "edt_ioctl(%04x, %04x)\n", code, arg);
#endif

    if (edt_p->devid == DMY_ID)
        return (0);
    memset(&eis,0, sizeof(eis));
    eis.device = edt_p->fd;
    eis.controlCode = EIO_DECODE_ACTION(code);
    size = EIO_DECODE_SIZE(code);
    set = EIO_DECODE_SET(code);
    get = EIO_DECODE_GET(code);
    eis.inSize = 0;
    eis.inBuffer = NULL;
    eis.outSize = 0;
    eis.outBuffer = NULL;
    eis.bytesReturned = 0 ;
#ifdef __APPLE__
    eis.channel = edt_p->channel_no ;
    eis.unit = edt_p->unit_no ;
#endif


    if (set)
    {
        eis.inSize = size;
        eis.inBuffer = arg;
    }

    if (get)
    {
        eis.outSize = size;
        eis.outBuffer = arg;
    }


#ifdef _NT_

    if (!DeviceIoControl(
        (HANDLE) edt_p->fd,
        EDT_MAKE_IOCTL(EDT_DEVICE_TYPE, eis.controlCode),
        eis.inBuffer,
        eis.inSize,
        eis.outBuffer,
        eis.outSize,
        (LPDWORD)&eis.bytesReturned,
        NULL))
    {
        return -1;
    }
    return 0;

#else
#ifdef VXWORKS
    return ioctl(edt_p->fd, EDT_NT_IOCTL, (int)&eis);
#else
#if defined(__APPLE__)

    eis.device = edt_p->channel_no ;
    ret = edt_mac_ioctl((u_int)edt_p->fd, EDT_NT_IOCTL, &eis) ;
    return(ret);
#else
    return ioctl(edt_p->fd, EDT_NT_IOCTL, &eis);
#endif /* apple */
#endif
#endif

}

int
edt_ioctl_nt(EdtDev * edt_p, int controlCode, void *inBuffer, int inSize,
             void *outBuffer, int outSize, int *bytesReturned)
{
    int     ret = 0;

    edt_ioctl_struct eis;

    eis.device = edt_p->fd;
    eis.controlCode = controlCode;
    eis.inBuffer = inBuffer;
    eis.inSize = inSize;
    eis.outBuffer = outBuffer;
    eis.outSize = outSize;
    eis.bytesReturned = 0;
#ifdef __APPLE__
    eis.channel = edt_p->channel_no ;
    eis.unit = edt_p->unit_no ;
#endif

#ifdef _NT_

    if (inSize == 0 && outSize == 0)
        return ret ;

    if (!DeviceIoControl(
        eis.device,
        eis.controlCode,
        eis.inBuffer,
        eis.inSize,
        eis.outBuffer,
        eis.outSize,
        (LPDWORD) &eis.bytesReturned,
        NULL))
    {

    }

#else
#ifdef VXWORKS
    if ((ret = ioctl(eis.device, EDT_NT_IOCTL, (int)&eis)) == 0)
#else /* vxworks */
#if defined(__APPLE__)
    if ((ret = edt_mac_ioctl((u_int)edt_p->fd, EDT_NT_IOCTL, &eis)) == 0)
#else
    if ((ret = ioctl(eis.device, EDT_NT_IOCTL, &eis)) == 0)
#endif /* apple */
#endif /* vxworks */
#endif /* nt */
    {
        *bytesReturned = eis.bytesReturned;
    }
    return ret;
}


uint_t
edt_debugval(EdtDev * edt_p)
{
    uint_t  debugval = 0;

    return (debugval);
}

/**
* Returns the number of read and write timeouts that have occurred
* since the last call of #edt_open.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @return The number of read and write timeouts that have occurred
* since the last call of #edt_open.
*/

int
edt_timeouts(EdtDev * edt_p)
{
    int     timeouts;

    if (edt_p->devid == DMY_ID)
        return (0);
    edt_ioctl(edt_p, EDTG_TIMEOUTS, &timeouts);
    return (timeouts);
}

int
edt_set_dependent(EdtDev * edt_p, void *addr)
{
    if (addr == NULL)
        return (-1);
    if (edt_p->devid == DMY_ID)
    {
#ifdef _NT_
        edt_p->fd = CreateFile(
            edt_p->edt_devname,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
#else
        if (edt_p->fd)
        {
            /* printf("fd already open\n") ;*/
        }
        else
        {
            if (edt_p->devid == DMY_ID)
              close(creat(edt_p->edt_devname, 0666));

            edt_p->fd = open(edt_p->edt_devname, O_RDWR, 0666);

            if (edt_p->devid == DMY_ID)
              unlink(edt_p->edt_devname);
        }
#endif
        edt_write(edt_p, addr, sizeof(Dependent));
#ifdef _NT_
        CloseHandle(edt_p->fd);
        edt_p->fd = NULL;
#else
        close(edt_p->fd);
        edt_p->fd = 0;
#endif
        return (0);
    }
    return (edt_ioctl(edt_p, EDTS_DEPENDENT, addr));
}

int
edt_get_dependent(EdtDev * edt_p, void *addr)
{
    if (addr == NULL)
        return (-1);
    if (edt_p->devid == DMY_ID)
    {
        int ret ;
#ifdef _NT_
        edt_p->fd = CreateFile(
            edt_p->edt_devname,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
#else
        if (edt_p->devid == DMY_ID)
          close(creat(edt_p->edt_devname, 0666));

        edt_p->fd = open(edt_p->edt_devname, O_RDWR, 0666);

        if (edt_p->devid == DMY_ID)
          unlink(edt_p->edt_devname);
#endif
        ret = edt_read(edt_p, addr, sizeof(Dependent));
#ifdef _NT_
        CloseHandle(edt_p->fd);
        edt_p->fd = NULL;
#else
        close(edt_p->fd);
        edt_p->fd = 0;
#endif
        if (ret != sizeof(Dependent))
            return (-1) ;
        return (0);
    }
    return (edt_ioctl(edt_p, EDTG_DEPENDENT, addr));
}

int
edt_dump_sglist( EdtDev * edt_p, int val)
{
    return (edt_ioctl(edt_p, EDTS_DUMP_SGLIST, &val));
}

int
edt_set_debug(EdtDev * edt_p, int val)
{
    return (edt_ioctl(edt_p, EDTS_DEBUG, &val));
}

int
edt_get_debug(EdtDev * edt_p)
{
    int     val;

    edt_ioctl(edt_p, EDTG_DEBUG, &val);
    return (val);
}

/**
* Sets the number of milliseconds for data read calls, such as
* #edt_read, to wait for DMA to complete before returning. A value of 0
* causes the I/O operation to wait forever--that is, to block on a
* read.  #edt_set_rtimeout affects #edt_wait_for_buffers and #edt_read.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param value   The number of milliseconds in the timeout period.
*
* @return 0 on success; -1 on error
*/

int
edt_set_rtimeout(EdtDev * edt_p, int value)
{
    edt_msg(EDTDEBUG, "edt_set_rtimeout(%d)\n", value);
    return edt_ioctl(edt_p, EDTS_RTIMEOUT, &value);
}

/**
* Sets the number of milliseconds for data write calls, such as
* #edt_write, to wait for DMA to complete before returning. A value of
* 0 causes the I/O operation to wait forever--that is, to block on a
* write.  \c edt_set_wtimeout affects #edt_wait_for_buffers and
* #edt_write.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param value      The number of milliseconds in the timeout period.
*
* @return 0 on success; -1 on error. If an error occurs, call
* #edt_perror to get the system error message.
*/

int
edt_set_wtimeout(EdtDev * edt_p, int value)
{
    edt_msg(EDTDEBUG, "edt_set_wtimeout(%d)\n", value);
    return edt_ioctl(edt_p, EDTS_WTIMEOUT, &value);
}

/**
* Gets the current read timeout value: the number of milliseconds to
* wait for DMA reads to complete before returning.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @return The number of milliseconds in the current read timeout
* period.
*/

int
edt_get_rtimeout(EdtDev * edt_p)
{
    int     value;

    edt_ioctl(edt_p, EDTG_RTIMEOUT, &value);
    return (value);
}

/**
* Gets the current write timeout value: the number of milliseconds to
* wait for DMA writes to complete before returning.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @return The number of milliseconds in the current write timeout
* period.
*/

int
edt_get_wtimeout(EdtDev * edt_p)
{
    int     value;

    edt_ioctl(edt_p, EDTG_WTIMEOUT, &value);
    return (value);
}

int
edt_get_tracebuf(EdtDev * edt_p, u_int * addr)
{
    if (addr == NULL)
        return (-1);
    return (edt_ioctl(edt_p, EDTG_TRACEBUF, addr));
}

/**
* send a message
*/
int
edt_send_msg(EdtDev * edt_p, int unit, const char *msg, int size)
{
    int dummy ;
    ser_buf ser ;
    ser.unit = unit ;
    ser.size = size ;
    ser.flags = EDT_SERIAL_SAVERESP ;

    if (size > sizeof(ser.buf))
    {
        edt_msg(EDTFATAL, "Error writing %d bytes to serial buffer\n", size);
        size = sizeof(ser.buf);
        return -1;
    }
    memcpy(ser.buf,msg,size) ;

    edt_ioctl_nt(edt_p, ES_SERIAL, &ser, size + EDT_SERBUF_OVRHD,
        NULL, 0, &dummy);
    return (0);
}

int
edt_get_msg_unit(EdtDev * edt_p, char *msgbuf, int maxsize, int unit)
{
    ser_buf ser ;
    int     bytes = 0;
    ser.unit = unit ;
    ser.size = maxsize ;

    edt_ioctl_nt(edt_p, EG_SERIAL, &ser, EDT_SERBUF_OVRHD,
        msgbuf, maxsize, &bytes);
    if (bytes < maxsize)
        msgbuf[bytes] = '\0';
    return (bytes);
}

/**
* Gets a message using the serial port of the current unit & channel.
*
* If the number of bytes read is less than \a maxsize, msgbuf will be
* NULL terminated.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param msgbuf the buffer to store the message into
* @param maxsize the maximum number of bytes to fill in. (Should not be
* greater than size of msgbuf.
* @return Number of bytes in message, 0 indicates error.
*/
int
edt_get_msg(EdtDev * edt_p, char *msgbuf, int maxsize)
{
    if (edt_is_dvcl(edt_p) || edt_is_dvfox(edt_p) || edt_p->devid == PDVAERO_ID)
        return(edt_get_msg_unit(edt_p, msgbuf, maxsize, edt_p->channel_no)) ;
    else return(edt_get_msg_unit(edt_p, msgbuf, maxsize, 0)) ;
}


int
edt_set_statsig(EdtDev * edt_p, int event, int sig)
{
    edt_buf buf;

    buf.desc = event;
    buf.value = sig;
    return (edt_ioctl(edt_p, EDTS_EVENT_SIG, &buf));
}

int
edt_set_eodma_int(EdtDev * edt_p, int sig)
{
    return (edt_ioctl(edt_p, EDTS_EODMA_SIG, &sig));
}

int
edt_set_autodir(EdtDev * edt_p, int val)
{
    return (edt_ioctl(edt_p, EDTS_AUTODIR, &val));
}

/**
* Tells whether and when to flush the FIFOs before DMA transfer. By
* default, the FIFOs are not flushed.  However, certain applications
* may require flushing before a given DMA transfer, or before each
* transfer.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param flag   Tells whether and when to flush the FIFOs. Valid values
* are:
* \arg \c EDT_ACT_NEVER  don't flush before DMA transfer (default)
* \arg \c EDT_ACT_ONCE   flush before the start of the next DMA
* transfer
* \arg \c EDT_ACT_ALWAYS flush before the start of every DMA transfer
*
* @return 0 on success; -1 on error. If an error occurs, call
* #edt_perror to get the system error message.
*/

int
edt_set_firstflush(EdtDev * edt_p, int flag)
{
    return (edt_ioctl(edt_p, EDTS_FIRSTFLUSH, &flag));
}


/**
* OBSOLETE.
*
* Returns the value set by edt_set_firstflush(). This is an obsolete
* function that was only used as a kludge to detect EDT_ACT_KBS (also
* obsolete).
*
* @param edt_p pointer to edt device structure returned by
* #edt_open or #edt_open_channel
*
* \Example
* @code
* int application_should_already_know_this;
* application_should_already_know_this=edt_get_firstflush(edt_p);
* @endcode
*
* @return The value set by edt_set_firstflush().
* @see edt_set_firstflush
*
* @todo better explaination of why app already knows this. Might they
* be trying to verify what the default is, in which case they aren't
* sure what the value is?
*/
int
edt_get_firstflush(EdtDev * edt_p)
{

    int val;

    edt_ioctl(edt_p, EDTG_FIRSTFLUSH, &val);

    return val;
}


u_char
edt_set_funct_bit(EdtDev * edt_p, u_char mask)
{
    unsigned char funct;

    funct = edt_reg_read(edt_p, PCD_FUNCT);
    funct |= mask;
    edt_reg_write(edt_p, PCD_FUNCT, funct);
    return (funct);
}

u_char
edt_clr_funct_bit(EdtDev * edt_p, u_char mask)
{
    u_char  funct;

    funct = edt_reg_read(edt_p, PCD_FUNCT);
    funct &= ~mask;
    edt_reg_write(edt_p, PCD_FUNCT, funct);
    return (funct);
}


/**
* assume the AV9110 is already selected assume the AV9110 clock is low and
* leave it low output the number of bits specified from the lsb until done
*/
static void
shft_av9110(EdtDev * edt_p, u_int data, u_int numbits)
{
    int use_pcd_method;

    if (ID_IS_PCD(edt_p->devid) || edt_p->devid == PDVAERO_ID)
        use_pcd_method = 1;
    else
        use_pcd_method = 0;

    while (numbits)
    {
        if (use_pcd_method)
        {
            if (data & 0x1)
                edt_set_funct_bit(edt_p, EDT_FUNCT_DATA);
            else
                edt_clr_funct_bit(edt_p, EDT_FUNCT_DATA);
            /* clock it in */
            edt_set_funct_bit(edt_p, EDT_FUNCT_CLK);
            edt_clr_funct_bit(edt_p, EDT_FUNCT_CLK);
        }
        else
        {
            if (data & 0x1)
                edt_set_pllct_bit(edt_p, EDT_FUNCT_DATA);
            else
                edt_clr_pllct_bit(edt_p, EDT_FUNCT_DATA);
            /* clock it in */
            edt_set_pllct_bit(edt_p, EDT_FUNCT_CLK);
            edt_clr_pllct_bit(edt_p, EDT_FUNCT_CLK);

        }
        data = data >> 1;
        numbits--;
    }
}

u_char
edt_set_pllct_bit(EdtDev * edt_p, u_char mask)
{
    unsigned char pll_ct;

    pll_ct = edt_reg_read(edt_p, PDV_PLL_CTL);
    pll_ct |= mask;
    edt_reg_write(edt_p, PDV_PLL_CTL, pll_ct);
    return (pll_ct);
}

u_char
edt_clr_pllct_bit(EdtDev * edt_p, u_char mask)
{
    u_char  pll_ct;

    pll_ct = edt_reg_read(edt_p, PDV_PLL_CTL);
    pll_ct &= ~mask;
    edt_reg_write(edt_p, PDV_PLL_CTL, pll_ct);
    return (pll_ct);
}

void
edt_set_out_clk(EdtDev * edt_p, edt_pll * clk_data)
{
    int use_pcd_method;

    unsigned char opt_e = 0;
    u_int   svfnct;

    if (ID_IS_PCD(edt_p->devid) || edt_p->devid == PDVAERO_ID)
        use_pcd_method = 1;
    else
        use_pcd_method = 0;

    switch (clk_data->h)
    {
    case 1:
        opt_e = EDT_FAST_DIV1;
        break;

    case 3:
        opt_e = EDT_FAST_DIV3;
        break;

    case 5:
        opt_e = EDT_FAST_DIV5;
        break;

    case 7:
        opt_e = EDT_FAST_DIV7;
        break;

    default:
        edt_msg(EDTDEBUG, "Illegal value %d for xilinx fast clk divide\n",
            clk_data->h);
        opt_e = EDT_FAST_DIV1;
        clk_data->h = 1;
        break;
    }
    opt_e |= ((clk_data->l - 1) << EDT_X_DIVN_SHFT);


    if (use_pcd_method)
    {
        edt_reg_write(edt_p, EDT_OUT_SCALE, opt_e);
        edt_reg_write(edt_p, EDT_REF_SCALE, clk_data->x - 1);
        svfnct = edt_reg_read(edt_p, PCD_FUNCT);
        edt_set_funct_bit(edt_p, EDT_FUNCT_SELAV);
        edt_clr_funct_bit(edt_p, EDT_FUNCT_CLK);


    }
    else
    {
        edt_reg_write(edt_p, PDV_OUT_SCALE, opt_e);
        edt_reg_write(edt_p, PDV_REF_SCALE, clk_data->x - 1);
        svfnct = edt_reg_read(edt_p, PDV_PLL_CTL);
        edt_set_pllct_bit(edt_p, EDT_FUNCT_SELAV);
        edt_clr_pllct_bit(edt_p, EDT_FUNCT_CLK);

    }


    svfnct &= ~EDT_FUNCT_SELAV;  /* Must turn this off when done - jerry */

    /* shift out data */
    shft_av9110(edt_p, clk_data->n, 7);
    shft_av9110(edt_p, clk_data->m, 7);
    /* set vco preescale */
    if (clk_data->v == 1)
        shft_av9110(edt_p, 0, 1);
    else
        shft_av9110(edt_p, 1, 1);
    /* clkx divide is not used (right now) so set to div 1 */
    switch (clk_data->r)
    {
    case 1:
        shft_av9110(edt_p, 0x170, 9);
        break;

    case 2:
        shft_av9110(edt_p, 0x174, 9);
        break;

    case 4:
        shft_av9110(edt_p, 0x178, 9);
        break;

    case 8:
        shft_av9110(edt_p, 0x17c, 9);
        break;

    default:
        edt_msg(EDTDEBUG, "illegal value %d for AV9110 aoutput divide\n",
            clk_data->r);
        shft_av9110(edt_p, 0x5c, 7);
        break;
    }
    /* restore fnct bits */

    if (use_pcd_method)
        edt_reg_write(edt_p, PCD_FUNCT, svfnct);
    else
        edt_reg_write(edt_p, PDV_PLL_CTL, svfnct);

}

u_int
edt_set_sglist(EdtDev *edt_p,
               uint_t bufnum,
               uint_t *log_list,
               uint_t log_entrys)
{
    int ret ;
    buf_args sg_args ;
    sg_args.index = bufnum ;
    sg_args.size = log_entrys ;
#ifdef WIN32
    sg_args.addr = (uint64_t) (log_list);
#else
    sg_args.addr = (uint64_t) ((unsigned long) log_list);
#endif
    ret = edt_ioctl(edt_p, EDTS_SGLIST, &sg_args);

    return ret;
}

u_int
edt_set_sgbuf(EdtDev * edt_p, u_int sgbuf, u_int bufsize, u_int bufdir, u_int verbose)
{
    edt_set_buffer_size(edt_p, sgbuf, bufsize, bufdir) ;
    return 0;
}

/**
* Sets the driver behavior on a timeout.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param action integer configures the any action taken on a timeout.
* Definitions:
* \arg \c EDT_TIMEOUT_NULL no extra action taken
* \arg \c EDT_TIMEOUT_BIT_STROBE flush any valid bits left in input
* circuits of SSDIO.
*
* @return 0 on success, -1 on failure.
*
* @todo review... is this just SSDIO?
*/

int
edt_set_timeout_action(EdtDev * edt_p, u_int action)

{

    if (!edt_p)
    {
        return -1;

    }

    edt_ioctl(edt_p, EDTS_TIMEOUT_ACTION, &action);

    return 0;
}

/**
* Returns the number of good bits in the last long word of a read
* buffer after the last timeout. This routine is called after a
* timeout, if the timeout action is set to \c EDT_TIMEOUT_BIT_STROBE. (See
* #edt_set_timeout_action.)
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @return Number 0-31 represents the number of good bits in the last
* 32-bit word of the read buffer associated with the last timeout.
*/

int
edt_get_timeout_goodbits(EdtDev * edt_p)

{

    u_int   nGoodBits;

    if (!edt_p)
    {
        return 0;
    }

    edt_ioctl(edt_p, EDTG_TIMEOUT_GOODBITS, &nGoodBits);

    return nGoodBits;
}

/**
* Returns the current number of good bits in the last long word of a
* read buffer (0 through 31).
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @return Number 0-31 representing the number of good bits in the
* last 32-bit word of the current read buffer.
*
* @bug Possible bug... shouldn't this return -1 if !edt_p?
*/

int
edt_get_goodbits(EdtDev * edt_p)

{

    u_int   nGoodBits;

    if (!edt_p)
    {
        return 0;
    }

    edt_ioctl(edt_p, EDTG_GOODBITS, &nGoodBits);

    return nGoodBits;
}

/**
* @fn int edt_set_event_func(EdtDev * edt_p, int event_type, EdtEventFunc func, void *data, int continuous)
*
* Defines a function to call when an event occurs. Use this routine to
* send an application-specific
* function when required; for example, when DMA completes, allowing the
* application to continue
* executing until the event of interest occurs.
*
* If you wish to receive notification of one event only, and then
* disable further event notification, send a
* final argument of 0 (see the continue parameter described below).
* This disables event notification at the
* time of the callback to your function.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @param event_type The event that causes the function to be called. Valid
* events are:
* <TABLE>
* <TR><TD>Event</TD><TD>Description</TD><TD>Board</TD></TR>
* <TR>
* <TD>\c EDT_PDV_EVENT_ACQUIRE</TD>
* <TD> Image has been acquired; shutter has closed;<br>subject can be
* moved if necessary; DMA will now restart </TD>
* <TD> PCI DV, PCI DVK, PCI FOI </TD>
* </TR>
* <TR>
* <TD>\c EDT_PDV_EVENT_FVAL</TD>
* <TD>Frame Valid line is set</TD>
* <TD>PCI DV, PCI DVK</TD>
* </TR>
* <TR>
* <TD>\c EDT_EVENT_P16D_DINT</TD>
* <TD>Device interrupt occurred</TD>
* <TD>PCI 16D</TD>
* </TR>
* <TR>
* <TD>\c EDT_EVENT_P11W_ATTN</TD>
* <TD>Attention interrupt occurred</TD>
* <TD>PCI 11W</TD>
* </TR>
* <TR>
* <TD>\c EDT_EVENT_P11W_CNT</TD>
* <TD>Count interrupt occurred</TD>
* <TD>PCI 11W</TD>
* </TR>
* <TR>
* <TD>\c EDT_EVENT_PCD_STAT1</TD>
* <TD>Interrupt occurred on Status 1 line</TD>
* <TD>PCI CD</TD>
* </TR>
* <TR>
* <TD>\c EDT_EVENT_PCD_STAT2</TD>
* <TD>Interrupt occurred on Status 2 line</TD>
* <TD>PCI CD</TD>
* </TR>
* <TR>
* <TD>\c EDT_EVENT_PCD_STAT3</TD>
* <TD>Interrupt occurred on Status 3 line</TD>
* <TD>PCI CD</TD>
* </TR>
* <TR>
* <TD>\c EDT_EVENT_PCD_STAT4</TD>
* <TD>Interrupt occurred on Status 4 line</TD>
* <TD>PCI CD</TD>
* </TR>
* <TR>
* <TD>\c EDT_EVENT_ENDDMA</TD>
* <TD>DMA has completed</TD>
* <TD>ALL</TD>
* </TR>
* </TABLE>
* @param func The function you've defined to call when the event
* occurs.
* @param data Pointer to data block (if any) to send to the function as
* an argument; usually \c edt_p.
* @param continuous Flag to enable or disable continued event
* notification. A value of 0 causes an implied #edt_remove_event_func as
* the event is triggered.
*
* @return 0 on success, -1 on failure. If an error occurs, call
* #edt_perror to get the system error message.
*
*/

/**
* @fn int edt_remove_event_func(EdtDev * edt_p, int event_type)
*
* Removes an event function previously set with edt_set_event_func.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param event_type The event that causes the function to be called. Valid
* events are as listed in #edt_set_event_func.
*
* @return 0 on success, -1 on failure. If an error occurs, call
* #edt_perror to get the system error message.
*/




#ifdef _NT_

int
edt_get_kernel_event(EdtDev *edt_p, int event_type)

{
    EdtEventHandler *p;
    char    fullname[128];
    char   *Name;
    char *edt_devname;

    if (edt_p == NULL
        || (event_type < 0)
        || (event_type >= EDT_MAX_KERNEL_EVENTS)
        || ((Name = BaseEventNames[event_type]) == NULL)
        || ((p = &edt_p->event_funcs[event_type]) == NULL))
    {
        /* out of range */
        edt_msg(EDTDEBUG, "Invalid argument to edt_get_kernel_event\n");
        return -1;
    }

    /* device ID -> drivertype name string */
    if (edt_p->devid == P11W_ID)
        edt_devname = "P11w";
    else if (edt_p->devid == P16D_ID)
        edt_devname = "P16d";
    else if (edt_is_pcd(edt_p))
        edt_devname = "Pcd";
    else if (edt_is_pdv(edt_p))
        edt_devname = "Pdv";
    else if (edt_is_1553(edt_p))
        edt_devname = "P53b";
    else
    {
        edt_devname = NULL;
        edt_msg(EDTDEBUG, "Unknown device type %d\n", edt_p->devid);
        return -1;
    }

    sprintf(fullname, "%s%s%d%d", edt_devname, Name, edt_p->unit_no, edt_p->channel_no);

    if ((p->wait_event =
        CreateEvent(NULL, FALSE, FALSE,fullname)) == NULL)
    {
        if ((p->wait_event =
            OpenEvent(SYNCHRONIZE, FALSE, fullname)) == NULL)
        {

            edt_msg(EDTDEBUG, "Unable to open event %s\n", fullname);
            edt_msg_perror(EDTFATAL, "event") ;

            return -1;

        }
        else
        {
            ResetEvent(p->wait_event) ;
            return(0) ;
        }

    }
    else
    {
        edt_buf argbuf ;
        argbuf.desc = event_type ;
        argbuf.value = (uint64_t)p->wait_event ;
        edt_ioctl(edt_p, EDTS_EVENT_HNDL, &argbuf);
        ResetEvent(p->wait_event) ;
    }

    return 0;

}

/********************************************************
* Close everything and zero memory
********************************************************/

static
void
edt_clear_event_func(EdtEventHandler * p)

{
    if (p->wait_thread)
        CloseHandle(p->wait_thread);
    if (p->wait_event)
        CloseHandle(p->wait_event);
    if (p->closing_event)
        CloseHandle(p->closing_event);

    memset(p, 0, sizeof(*p));

}

/********************************************************/
/* Thread function to wait on a particular driver event */
/********************************************************/

static u_int WINAPI
edt_wait_event_thread(void *pObj)
{
    EdtEventHandler *p = (EdtEventHandler *) pObj;
    int     rc = WAIT_OBJECT_0;
    HANDLE  events[2];
    u_int   tmp;


    if (p)
    {
        events[0] = p->wait_event;
        events[1] = p->closing_event;

        while (p->active)
        {

#ifndef NEW_WAY
            rc = WaitForMultipleObjects(2, events, FALSE, INFINITE);
#else

            edt_ioctl(p->owner, EDTS_WAIT_EVENT, &p->event_type);
#endif

            if (p->active && rc == WAIT_OBJECT_0)
            {
                edt_ioctl(p->owner, EDTS_CLR_EVENT, &p->event_type);

                if (p->callback)
                    p->callback(p->data);

            }

            if (p->continuous == 0)
            {
                edt_ioctl(p->owner, EDTS_DEL_EVENT_FUNC, &p->event_type);

                /* May need this later -- Mark 01/99 */
                tmp = p->thrdid;
                /*p->active = 0 ;*/
                edt_clear_event_func(p);
                p->thrdid = tmp;

                break;
            }
        }
    }
    return 0;
}

/* ****************************************************** */
/* Add an event function for an event_type               */
/* Name is derived from event type -                     */
/* only works for "base events" defined for all boards   */
/* ****************************************************** */

int
edt_set_event_func(EdtDev * edt_p, int event_type, EdtEventFunc func, void *data,
                   int continuous)
{
    EdtEventHandler *p;
    char   *Name;
    HANDLE event;

    edt_msg(EDTDEBUG, "edt_set_event_func(type %d cont %d)\n", event_type, continuous);

    if (edt_p == NULL || (func == NULL)
        || (event_type < 0)
        || (event_type >= EDT_MAX_KERNEL_EVENTS)
        || ((Name = BaseEventNames[event_type]) == NULL)
        || ((p = &edt_p->event_funcs[event_type]) == NULL))
    {
        /* out of range */
        return -1;
    }


    if (p->callback)
    {
        /* whoops - we're already going */

        /* if it's the same callback, no problem */

        if (p->callback == func && p->data == data)
            return 0;
        else
            return -1;
    }

    /* Set the function */
    p->callback = func;
    /* Set the data */
    p->data = data;

    p->owner = edt_p;
    p->active = TRUE;
    p->event_type = event_type;
    p->continuous = continuous;


    /* Unlock */

    /* Launch the thread */


    edt_get_kernel_event(edt_p, event_type) ;
    event = edt_p->event_funcs[event_type].wait_event;

    if (event == NULL)
    {
        edt_clear_event_func(p);
        return -1;

    }

    if ((p->closing_event = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL)
    {
        edt_clear_event_func(p);
        return -1;
    }
#ifdef __APPLE__
    if ((p->wait_thread = (mac_thread_t)
        _beginthreadex(NULL, 0, edt_wait_event_thread, p, CREATE_SUSPENDED,
        &p->thrdid)) == NULL)
#else
    if ((p->wait_thread = (thread_t)
        _beginthreadex(NULL, 0, edt_wait_event_thread, p, CREATE_SUSPENDED,
        &p->thrdid)) == NULL)
#endif
    {
        edt_clear_event_func(p);
        return -1;
    }

    /*
    * The continuous parameter now supports one of three modes:
    *
    *           EDT_EVENT_MODE_ONCE             (0)
    *           EDT_EVENT_MODE_CONTINUOUS       (1)
    *           EDT_EVENT_MODE_SERIALIZE        (2)
    *
    * The mode is passed to the driver in the high byte of event_type.
    */
    event_type |= ((continuous << EDT_EVENT_MODE_SHFT) & EDT_EVENT_MODE_MASK) ;
    edt_ioctl(edt_p, EDTS_ADD_EVENT_FUNC, &event_type);
    event_type &= ~EDT_EVENT_MODE_MASK ;

    ResumeThread(p->wait_thread);

    /*SetThreadPriority(p->wait_thread, THREAD_PRIORITY_HIGHEST);*/

    return 0;
}

int
edt_remove_event_func(EdtDev * edt_p, int event_type)

{
    EdtEventHandler *p;

    if (edt_p == NULL || event_type < 0 || event_type >= EDT_MAX_KERNEL_EVENTS)
    {
        /* out of range */
        return -1;
    }

    p = &edt_p->event_funcs[event_type];

    /*
    * Try and close the thread gracefully
    */

    if (p->wait_thread && p->wait_event)
    {
        p->active = FALSE;

        SetEvent(p->closing_event);

        edt_ioctl(edt_p, EDTS_DEL_EVENT_FUNC, &event_type);

        WaitForSingleObject(p->wait_thread, INFINITE);

        edt_clear_event_func(p);

    }else
        edt_clear_event_func(p);

    return 0;
}

#elif defined(__sun)            /* _NT_ */

/* *******************************************************/
/* Add an event function for an event_type               */
/* Name is derived from event type                       */
/* - only works for "base events" defined for all boards */
/* *******************************************************/

int
edt_set_event_func(EdtDev * edt_p, int event_type, EdtEventFunc f, void *data,
                   int continuous)
{
    EdtEventHandler *p;
    char    fullname[128];
    char   *Name;


    if (edt_p == NULL || (f == NULL)
        || (event_type < 0)
        || (event_type >= EDT_MAX_KERNEL_EVENTS)
        || ((Name = BaseEventNames[event_type]) == NULL)
        || ((p = &edt_p->event_funcs[event_type]) == NULL))
    {
        /* out of range */
        return -1;
    }

    if (p->active && p->callback)
    {
        /* whoops - we're already going */
        edt_remove_event_func(edt_p, event_type);
    }
    /*
    * Solaris:  if another thread has run for this event, wait for it.
    * Otherwise it stays alive and uses mmap() resources.  Mark  01/99
    */
    else if (p->thrdid)
        thr_join(p->thrdid, NULL, NULL);

    /* Set the function */
    p->callback = f;
    /* Set the data */
    p->data = data;

    p->owner = edt_p;
    p->active = TRUE;
    p->event_type = event_type;
    p->continuous = continuous;

    sema_init(&p->sema_thread, 0, USYNC_PROCESS, NULL);

    /*
    * The continuous parameter now supports one of three modes:
    *
    *           EDT_EVENT_MODE_ONCE             (0)
    *           EDT_EVENT_MODE_CONTINUOUS       (1)
    *           EDT_EVENT_MODE_SERIALIZE        (2)
    *
    * The mode is passed to the driver in the high byte of event_type.
    */
    event_type |= ((continuous << EDT_EVENT_MODE_SHFT) & EDT_EVENT_MODE_MASK) ;
    edt_ioctl(edt_p, EDTS_ADD_EVENT_FUNC, &event_type);
    event_type &= ~EDT_EVENT_MODE_MASK ;

    /* Launch the thread */

    sprintf(fullname, "%s%d%d", Name, edt_p->unit_no, edt_p->channel_no);
    if (thr_create(NULL, 0, edt_wait_event_thread, (void *) p,
        THR_NEW_LWP | THR_BOUND, &p->thrdid))
    {
        edt_msg_perror(EDTFATAL, "edt_set_event_func: thr_create");
        edt_msg(EDTWARN, "Unable to create eventthread %s\n", fullname);
        edt_msg(EDTWARN, "thr_create:  use \"-lthread\" compile option\n", stderr);
        return -1;
    }

    sema_wait(&p->sema_thread);
    thr_yield();

    return 0;
}

/********************************************************/
/* Close everything and zero memory                      */
/********************************************************/

static
void
edt_clear_event_func(EdtEventHandler * p)

{
    /* TODO - anything else need to be done here for solaris?  -- Mark 01/99 */
    memset(p, 0, sizeof(*p));
}


/********************************************************/
/* Thread function to wait on a particular driver event */
/********************************************************/

static void *
edt_wait_event_thread(void *pObj)

{
    EdtEventHandler *p = (EdtEventHandler *) pObj;
#ifdef __APPLE__
    mac_thread_t;
#else
    thread_t tmp;
#endif
    int     ret;

#ifdef __sun
    if (p->owner->use_RT_for_event_func)
        edt_set_RT(1) ;         /* This isn't a good idea unless
                            * you really can't live without it.  - Mark
                            */
#endif

    if (p)
    {
        while (p->active)
        {
            sema_post(&p->sema_thread);
            if (p->continuous == 0)
                edt_ioctl(p->owner, EDTS_WAIT_EVENT_ONCE, &p->event_type);
            else
                edt_ioctl(p->owner, EDTS_WAIT_EVENT, &p->event_type);

            if (p->active && p->callback)
                p->callback(p->data);

            if (p->continuous == 0)
            {
                edt_ioctl(p->owner, EDTS_DEL_EVENT_FUNC, &p->event_type);

                /* Need this later for thr_join() in set_event_func() */
                tmp = p->thrdid;
                edt_clear_event_func(p);
                p->thrdid = tmp;

                break;
            }
        }
    }

    thr_exit(NULL);

    return pObj;
}

/* *******************************************************/
/* Delete an event handler                               */
/* *******************************************************/

int
edt_remove_event_func(EdtDev * edt_p, int event_type)
{
    EdtEventHandler *p;

    if (edt_p == NULL || event_type < 0 || event_type >= EDT_MAX_KERNEL_EVENTS)
    {
        /* out of range */
        return -1;
    }

    p = &edt_p->event_funcs[event_type];

    /*
    * Try and close the thread gracefully
    */

    if (p->active)
    {
        p->active = FALSE;

        edt_ioctl(edt_p, EDTS_DEL_EVENT_FUNC, &event_type);

        thr_join(p->thrdid, NULL, NULL);

        edt_clear_event_func(p);
    }

    return 0;
}
#elif defined(sgi) || defined(__linux__) || defined(__APPLE__)

#ifndef NO_PTHREAD

#include <pthread.h>

/* *******************************************************/
/* Add an event function for an event_type               */
/* Name is derived from event type                       */
/* - only works for "base events" defined for all boards */
/* *******************************************************/

int
edt_set_event_func(EdtDev * edt_p, int event_type, EdtEventFunc f, void *data,
                   int continuous)
{
    EdtEventHandler *p;
    char    fullname[128];
    char   *Name;


    if (edt_p == NULL || (f == NULL)
        || (event_type < 0)
        || (event_type >= EDT_MAX_KERNEL_EVENTS)
        || ((Name = BaseEventNames[event_type]) == NULL)
        || ((p = &edt_p->event_funcs[event_type]) == NULL))
    {
        /* out of range */
        return -1;
    }

    if (p->active && p->callback)
    {
        /* whoops - we're already going */
        edt_remove_event_func(edt_p, event_type);
    }
    /*
    * pthreads:  if another thread has run for this event, wait for it.
    * Otherwise it stays alive and uses mmap() resources.  Mark  01/99
    */
    else if (p->thrdid)
        pthread_join(p->thrdid, NULL);

    /* Set the function */
    p->callback = f;
    /* Set the data */
    p->data = data;

    p->owner = edt_p;
    p->active = TRUE;
    p->event_type = event_type;
    p->continuous = continuous;


    /* sema_init(&p->sema_thread, 0, USYNC_PROCESS, NULL); */

    /*
    * The continuous parameter now supports one of three modes:
    *
    *           EDT_EVENT_MODE_ONCE             (0)
    *           EDT_EVENT_MODE_CONTINUOUS       (1)
    *           EDT_EVENT_MODE_SERIALIZE        (2)
    *
    * The mode is passed to the driver in the high byte of event_type.
    */
    event_type |= ((continuous << EDT_EVENT_MODE_SHFT) & EDT_EVENT_MODE_MASK) ;
    edt_ioctl(edt_p, EDTS_ADD_EVENT_FUNC, &event_type);
    event_type &= ~EDT_EVENT_MODE_MASK ;

    /* Launch the thread */

    sprintf(fullname, "%s%d%d", Name, edt_p->unit_no, edt_p->channel_no);
    if (pthread_create(&p->thrdid, 0, edt_wait_event_thread, (void *) p)
        != 0)
    {
        edt_msg_perror(EDTFATAL, "edt_set_event_func: thr_create");
        edt_msg(EDTWARN, "Unable to create eventthread %s\n", fullname);
        edt_msg(EDTWARN, "thr_create:  use \"-lthread\" compile option\n", stderr);
        return -1;
    }

    /* sema_wait(&p->sema_thread); */
    /* thr_yield(); */

    return 0;
}

/********************************************************/
/* Close everything and zero memory                      */
/********************************************************/

static
void
edt_clear_event_func(EdtEventHandler * p)

{
    /* TODO - anything else need to be done here for solaris?  -- Mark 01/99 */
    memset(p, 0, sizeof(*p));
}


/********************************************************/
/* Thread function to wait on a particular driver event */
/********************************************************/

static void *
edt_wait_event_thread(void *pObj)

{
    EdtEventHandler *p = (EdtEventHandler *) pObj;
    pthread_t tmp;

    if (p)
    {
        while (p->active)
        {
            /* sema_post(&p->sema_thread); */
            if (p->continuous == 0)
                edt_ioctl(p->owner, EDTS_WAIT_EVENT_ONCE, &p->event_type);
            else
                edt_ioctl(p->owner, EDTS_WAIT_EVENT, &p->event_type);

            if (p->active && p->callback)
                p->callback(p->data);

            if (p->continuous == 0)
            {
                edt_ioctl(p->owner, EDTS_DEL_EVENT_FUNC, &p->event_type);

                /* Need this later for thr_join() in set_event_func() */
                tmp = p->thrdid;
                edt_clear_event_func(p);
                p->thrdid = tmp;

                break;
            }
        }
    }

    pthread_exit(NULL);

    return pObj;
}

/* *******************************************************/
/* Delete an event handler                               */
/* *******************************************************/

int
edt_remove_event_func(EdtDev * edt_p, int event_type)
{
    EdtEventHandler *p;

    if (edt_p == NULL || event_type < 0 || event_type >= EDT_MAX_KERNEL_EVENTS)
    {
        /* out of range */
        return -1;
    }

    p = &edt_p->event_funcs[event_type];

    /*
    * Try and close the thread gracefully
    */

    if (p->active)
    {
        p->active = FALSE;

        edt_ioctl(edt_p, EDTS_DEL_EVENT_FUNC, &event_type);

        pthread_join(p->thrdid, NULL);

        edt_clear_event_func(p);
    }

    return 0;
}
#endif   /* NOT PTHREAD */

#elif defined(_AIX) || defined(__hpux)

/* *******************************************************
* Add an event function for an event_type
* Name is derived from event type - only works
* for "base events" defined for all boards
* *******************************************************/

int
edt_set_event_func(EdtDev * edt_p, int event_type, EdtEventFunc f, void *data,
                   int continuous)
{
    return -1;
}

/********************************************************
* Close everything and zero memory
********************************************************/

static
void
edt_clear_event_func(EdtEventHandler * p)

{

}


/********************************************************
* Thread function to wait on a particular driver event *
********************************************************/

static void *
edt_wait_event_thread(void *pObj)

{
    return NULL;
}

/* *******************************************************
* Delete an event handler                                                               *
* *******************************************************/

int
edt_remove_event_func(EdtDev * edt_p, int event_type)
{
    return -1;
}

#else

int
edt_remove_event_func(EdtDev * edt_p, int event_type)
{
    return -1;
}

#endif                          /* _NT_ */

int
edt_enable_event(EdtDev * edt_p, int event_type)
{
    edt_msg(EDTDEBUG, "edt_enable_event(type %d)\n", event_type) ;

    if (edt_p == NULL
        || (event_type < 0)
        || (event_type >= EDT_MAX_KERNEL_EVENTS))
    {
        /* out of range */
        return -1;
    }

#ifdef _NT_
    edt_get_kernel_event(edt_p, event_type) ;
#endif

    edt_ioctl(edt_p, EDTS_ADD_EVENT_FUNC, &event_type);

    return 0 ;
}

int
edt_wait_event(EdtDev * edt_p, int event_type, int timeoutval)
{
    int ret = 0 ;
    edt_msg(EDTDEBUG, "edt_wait_event(type %d)\n", event_type) ;
#ifdef _NT_
    {
        HANDLE event;
        int rc;

        if (!edt_p->event_funcs[event_type].wait_event)
        {
            edt_get_kernel_event(edt_p, event_type);
        }

        event = edt_p->event_funcs[event_type].wait_event;

        /* wait for event */

        if (timeoutval == 0)
            timeoutval = INFINITE;

        rc = WaitForSingleObject(event, timeoutval);

        if (rc == WAIT_TIMEOUT)
        {
            /* edt_msg(EDTDEBUG, "timeout...\n");*/

            /* deal with timeout */

            edt_do_timeout(edt_p);

            ret = 1 ;
        }
    }
#else
    edt_ioctl(edt_p, EDTS_WAIT_EVENT, &event_type);
#endif

    edt_msg(EDTDEBUG, "edt_wait_event(type %d)\n", event_type, ret) ;

    return(ret) ;
}


/**
* Added 09/24/99 Mark
*/

int
edt_reset_event_counter(EdtDev * edt_p, int event_type)
{
    return edt_ioctl(edt_p, EDTS_RESET_EVENT_COUNTER, &event_type);
}



/**
* Stops DMA transfer after the current buffer has completed. Ring
* buffer mode remains active, and transfers will be continued by
* calling #edt_start_buffers.
*
* @param edt_p pointer to edt device structure returned by #edt_open
*
* @return 0 on success, -1 on failure. If an error occurs, call
* #edt_perror to get the system error message.
*/

int
edt_stop_buffers(EdtDev * edt_p)

{
    unsigned int finish_current = 1;

    edt_msg(EDTDEBUG, "edt_stop_buffers\n");
    edt_ioctl(edt_p, EDTS_STOPBUF, &finish_current);
    return 0;
}

/**
* Stops any DMA currently in progress, then resets the ring buffer to
* start the next DMA at \a bufnum.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @param bufnum The index of the ring buffer at which to start the next
* DMA. A number larger than the number of buffers set up sets the
* current done count to the number supplied modulo the number of
* buffers.
*
*
* @return 0 on success; -1 on error. If an error occurs, call
* #edt_perror to get the system error message.
*/
int
edt_reset_ring_buffers(EdtDev * edt_p, uint_t bufnum)

{
    bufcnt_t     curdone;
    int     curBufIndex = -1;

    curdone = edt_done_count(edt_p);
    edt_cancel_current_dma(edt_p);
    if (edt_p->ring_buffer_numbufs)
    {
        curBufIndex = curdone % edt_p->ring_buffer_numbufs;
        edt_set_buffer(edt_p, bufnum);
    }
    edt_msg(EDTDEBUG, "edt_reset_ring_buffers buf %d curdone %d curBufIndex %d\n",
        bufnum, curdone, curBufIndex);
    return 0;
}

/**
* Stops any transfers currently in progress, resets the ring buffer
* pointers to restart on the current buffer.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @return 0 on sucess, -1 on error. If an error occurs, call
* #edt_perror to get the system error message.
*
* @see edt_abort_current_dma
*/

int
edt_abort_dma(EdtDev * edt_p)

{
    edt_msg(EDTDEBUG, "edt_abort_dma\n");
    edt_reset_ring_buffers(edt_p, edt_done_count(edt_p));
    return 0;
}

/**
* Stops the current transfers, resets the ring buffer pointers to the
* next buffer.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @return 0 on sucess, -1 on error. If an error occurs, call
* #edt_perror to get the system error message.
*
* @see edt_abort_dma
*/

int
edt_abort_current_dma(EdtDev * edt_p)

{
    edt_msg(EDTDEBUG, "edt_abort_current_dma\n");
    edt_reset_ring_buffers(edt_p, edt_done_count(edt_p) + 1);
    return 0;
}

/**
* Returns true (1) when DMA has wrapped around the ring buffer and
* overwritten the buffer which the application is about to access.
* Returns false (0) otherwise.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @return 1(true) when overrun has occurred, 0(false) otherwise.
*/
int
edt_ring_buffer_overrun(EdtDev *edt_p)

{
    bufcnt_t   dmacount;

    dmacount = edt_done_count(edt_p);

    if (dmacount >= edt_p->donecount + edt_p->ring_buffer_numbufs)
        return (1);
    else
        return (0);
}


#ifdef P16D

/**
* p16d_get_command
*
* gets value in the command register
*
* @param edt_p:    device handle returned from edt_open
*
* @return unsigned short containing the value currently in command register
*/

u_short
p16d_get_command(EdtDev * edt_p)
{
    return (edt_reg_read(edt_p, P16_COMMAND));
}

/**
* p16d_set_command
*
* sets value of command register
*
* @param edt_p:     device handle returned from edt_open
*         val:       value to which you wish to set the command register
*
* @return none
*/

void
p16d_set_command(EdtDev * edt_p, u_short val)
{
    edt_reg_write(edt_p, P16_COMMAND, val);
}

/**
* p16d_get_config
*
* gets value in configuration register
*
* @param edt_p:    device handle returned from edt_open
*
* @return unsigned short containing value currently in configuration register
*/

u_short
p16d_get_config(EdtDev * edt_p)
{
    return (edt_reg_read(edt_p, P16_CONFIG));
}

/**
* p16d_set_config
*
* sets value of configuration register
*
* @param edt_p:    device handle returned from edt_open
*         val:     value to which you wish to set configuration register
*
* @return none
*/

void
p16d_set_config(EdtDev * edt_p, u_short val)
{
    edt_reg_write(edt_p, P16_CONFIG, val);
}

/**
* p16d_get_stat
*
* gets value in status register
*
* @param edt_p:    device handle returned from edt_open
*
* @return unsigned short containing value currently in status register
*/

u_short
p16d_get_stat(EdtDev * edt_p)
{
    return (edt_reg_read(edt_p, P16_STATUS));
}

#endif                          /* P16D */


/**
* parse -u argument returning the device and unit.
*
* @return unit or -1 on failure (as well as device in dev, and channel
* in channel_ptr).
* @param instr The input string. The argument of the -u option (like
* "0" or "pcd0" for example).
* @param dev An array large enough to hold the device name, which is
* set by this function.
* @param default_dev The default device to copy to dev if instr doesn't
* specify device.  If NULL, EDT_INTERFACE will be used (which is "pcd"
* for pcd boards, "pdv" for dv boards, etc.).
* @param channel_ptr The channel specified in instr, or 0 (set by this function). If
*   channel_ptr is NULL or -1, it is ignored and unchanged.
*/
int
edt_parse_unit_channel(const char *instr,
                       char *dev,
                       const char *default_dev,
                       int *channel_ptr)
{
    int     unit = -1;
    int         channel = -1;
    size_t              last;

    char    retdev[256];
    char    str[256];


    edt_msg(EDTDEBUG, "edt_parse_unit_channel %s dev %s default %s: ",
        instr, dev, default_dev);

    if (default_dev)
        strcpy(retdev, default_dev);
    else
        strcpy(retdev, EDT_INTERFACE);

    strcpy(str,instr);

    if (strlen(str))
    {
        last = strlen(str)-1;

        /* find the channel or unit */

        if (isdigit(str[last]))
        {
            while (last && isdigit(str[last]))
            {
                last--;
            }

            if (!isdigit(str[last]))
            {
                last++;
            }

            channel = atoi(str+last);

            if (str[0] != '/' && str[0] != '\\')
                str[last] = 0;

            if (last>0)
                last--;

            if (last && str[last] == '_')
            {

                last--;

                if (isdigit(str[last]))
                {
                    size_t enddigit = last;
                    char checkstr[80];

                    while (last && isdigit(str[last]))
                    {
                        last--;
                    }

                    if (!isdigit(str[last]))
                    {
                        last++;
                    }

                    strncpy(checkstr,str+last,(enddigit - last)+1);

                    unit = atoi(checkstr);

                    if (str[0] != '/' && str[0] != '\\')
                        str[last] = 0;

                }
                else
                {
                    unit = channel;
                    channel = -1;
                }
            }
            else
            {
                unit = channel;
                channel = -1;
            }

        }
        else
            unit = 0;
    }
    else
        unit = 0;

    if (dev)
    {
        if (str[0])
            strcpy(dev, str);
        else
            strcpy(dev, retdev);
    }

    if (channel_ptr && (channel != -1))
        *channel_ptr = channel;

    return (unit);
}

/**
* Parses an EDT device name string. Fills in the name of the device,
* with the default_device if specified, or a default determined by the
* package, and returns a unit number.  Designed to facilitate a
* flexible device/unit command line argument scheme for application
* programs.  Most EDT example/utility programs use this susubroutine to
* allow users to specify either a unit number alone or a device/unit
* number concatenation.
*
* For example, if you are using a PCI CD, then either xtest -u 0 or
* xtest -u pcd0 could both be used, since xtest sends the argument to
* edt_parse_unit, and the subroutine parses the string and returns the
* device and unit number separately.
*
* @param str device name string from command line. Should be either a
* unit number ("0" - "8") or device/unit concantenation ("pcd0,"
* "pcd1," etc.)
* @param dev array to hold the device device string; filled in by ths routine.
* @param default_dev device name to use if none is given in the \a str
* argument. If NULL, will be filled in by the default device for the
* package in use. For example, if the code base is from a PCI CD
* package, the default_dev will be set to "pcd."
*
* @return Unit number, or -1 on error.  The first device is unit 0.
* @see example/utility programs xtest.c, initcam.c, simple_take.c.
*
*/

int
edt_parse_unit(const char *str, char *dev, const char *default_dev)
{
    int channel = 0;

    return edt_parse_unit_channel(str,dev,default_dev, &channel);


}
int
edt_serial_wait(EdtDev * edt_p, int msecs, int count)
{
    edt_buf tmp;
    int     ret;

    tmp.desc = msecs;
    tmp.value = count;
    edt_ioctl(edt_p, EDTS_SERIALWAIT, &tmp);
    ret = (u_int) tmp.value;

    edt_msg(EDTDEBUG, "edt_serial_wait(%d, %d) %d\n", msecs, count, ret);
    return (ret);
}

/**
* Causes application-defined events to show up in the same timeline
* as driver events when the event history is listed by running
* <code>setdebug -g</code>.
*
* This is useful for debugging.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param val  an arbitrary value meaningful to the application
*
* \Example
* @code
* #define BEFORE_WAIT 0x1212aaaa
* #define AFTER_WAIT 0x3434bbbb
* u_char *buf;
* edt_ref_tmstamp(edt_p, BEFORE_WAIT);
* buf=edt_wait_for_buffer(edt_p);
* edt_ref_tmstamp(edt_p, AFTER_WAIT);
*
* // now look at output of setdebug -g and you'll see something like:
*
* // 0: 0001ca0 REFTMSTAMP           : 1212aaaa      324.422071 (0.004189)
* // ... other events from edt_wait_for_buffer() shown, like START_BUF, SETUP_DMA, FLUSH, etc
* // 0: 0001d08 REFTMSTAMP           : 3434bbbb        324.518885 (0.000045)
*
* @endcode
*
* @return 0 on success, -1 on failure
* @see setdebug --help
*
*/

int
edt_ref_tmstamp(EdtDev *edt_p, u_int val)

{
    return edt_ioctl(edt_p,EDTS_REFTMSTAMP,&val);

}

/* check a bit against the capability register*/
int edt_board_capable(EdtDev *edt_p, u_int options)

{
  u_int mask;

  mask = edt_reg_read(edt_p, EDT_CAP_REGISTER);

  /* some old boards read all f's */
  if (mask == 0xffffffff)
    return 0;

  return ((mask & options) == options);

}

/**
*
* Atomically returns the number of bytes read so far into the current buffer
* along with the associated buffer number in the second argument.
* Can be used to monitor how much data has been read into the buffer
* during acquisition.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param cur_buffer        pointer to variable to receive applicable buffer number
*
* @return The number of bytes transferred, as described above, plus buffer number by reference.
*
*/
/* The counters are individual / channel */
#define EDT_BYTECOUNTER(channel) (EDT_PCI_FPGA_REGISTER + 0x100 + (4 * (channel)))

uint_t
edt_get_bufbytecount(EdtDev * edt_p, u_int *bufnum_p)
{
  uint64_t total;
  u_int ndone;
  u_int count;
  uint64_t wrapped;
  uint64_t totmod;
  u_int c;
  u_int bsize;
  uint_t args[2];

  /* Check for presence of bytecount reg */
  if (!edt_board_capable(edt_p, EDT_HAS_BYTECOUNT))
  {
    /* Old method */
    if (edt_ioctl(edt_p, EDTG_BUFBYTECOUNT, &args) < 0)
      edt_msg_perror(EDTLIB_MSG_FATAL, "edt_ioctl(EDTG_BUFBYTECOUNT)");

    if (bufnum_p)
      *bufnum_p = args[1];
    return (args[0]);
  }

  /* get an approximate location of DMA based on donecount
     to ddeal with 32-bit wraparound */
  bsize = edt_p->ring_buffer_bufsize / 4;
  ndone = edt_done_count(edt_p);
  total = (uint64_t)ndone * bsize;

  /* get high 32 bit word based on done */
  wrapped = total & 0xFFFFFFFF00000000;

  count = edt_reg_read(edt_p, EDT_BYTECOUNTER(edt_p->channel_no));
  wrapped += count;

  totmod = total & 0xFFFFFFFF;

  /* deal with wraparound special case */
  /* where total hasn't wrapped 32 bits yet but bytecount has */

  if (count < totmod)
  {
    /* increment upper 32 bits*/
    wrapped += 0x100000000;
  }

  /* calculate which buffer and buffer offset based on
     constant size buffers */
  *bufnum_p = (wrapped / bsize) % edt_p->ring_buffer_numbufs;
  c = wrapped % bsize;

  return c * 4;

}

void
edt_dmasync_fordev(EdtDev *edt, int bufnum, int offset, int bytecount)
{
    u_int args[3] ;

    args[0] = bufnum ;
    args[1] = offset ;
    args[2] = bytecount ;

    edt_ioctl(edt, EDTS_DMASYNC_FORDEV, args) ;
}

void
edt_dmasync_forcpu(EdtDev *edt, int bufnum, int offset, int bytecount)
{
    u_int args[3] ;

    args[0] = bufnum ;
    args[1] = offset ;
    args[2] = bytecount ;

    edt_ioctl(edt, EDTS_DMASYNC_FORCPU, args) ;
}

/* return true if machine is little_endian */
int
edt_little_endian()
{
    u_short test;
    u_char *byte_p;

    byte_p = (u_char *) & test;
    *byte_p++ = 0x11;
    *byte_p = 0x22;
    if (test == 0x1122)
    {
        edt_msg(EDTDEBUG, "edt_endian: BIG (SPARC, PowerPC)\n");
        return (0);
    }
    else
    {
        edt_msg(EDTDEBUG, "edt_endian: LITTLE (X86, AMD, SPARCv9)\n");
        return (1);
    }
}


//
// Internal EDT lab use only - set all ring buffer SgList entries to specified 32-bit physaddr
//
int
edt_set_buffer_physaddr(EdtDev * edt_p, uint_t index, uint64_t physaddr)
{

    buf_args sysargs;


    if (edt_p->ring_buffers[index])
    {

        sysargs.index = index;
        sysargs.addr = physaddr;
        sysargs.size = 0;

        return edt_ioctl(edt_p, EDTS_BUF, &sysargs);

    }
    else
    {

        edt_msg_perror(EDTFATAL,
                "edt_set_buffer_physaddr: Attempt to set physaddr on unallocated buffer\n");

        return -1;

    }

}


/**
* Used to change the size or direction of one of the ring buffers.
* Almost never used. Mixing directions requires detailed knowledge of
* the interface since pending preloaded DMA transfers need to be
* coordinated with the interface fifo direction. For example, a dma
* write will complete when the data is in the output fifo, but the dma
* read should not be started until the data is out to the external
* device.  Most applications requiring fast mixed reads/writes have
* worked out more cleanly using seperate, simultaneous, read and write
* dma transfers using different dma channels.
*
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @param index       index of ring buffer to change
* @param size        size to change it to
* @param write_flag  direction
*
* \Example
* @code
* u_int bufnum=3;
* u_int bsize=1024;
* u_int dirflag=EDT_WRITE;
* int ret;
* ret=edt_set_buffer_size(edt_p, bufnum, bsize, dirflag);
* @endcode
*
* @return 0 on success, -1 on failure
* @see edt_open_channel, \c rdpcd8.c, \c rd16.c, \c rdssdio.c, \c wrssdio.c
*/

int
edt_set_buffer_size(EdtDev * edt_p, uint_t index, uint_t size, uint_t write_flag)

{

    buf_args sysargs;

    if (edt_p->ring_buffers[index])
    {
        if (size > (u_int) edt_p->rb_control[index].allocated_size)
        {
            edt_msg_perror(EDTFATAL, "edt_set_buffer_size: Attempt to set size greater than allocated\n");
            return -1;
        }

        edt_p->rb_control[index].size = size;
        sysargs.index = index;
        sysargs.writeflag = write_flag;
        sysargs.addr = 0;
        sysargs.size = size;

        return edt_ioctl(edt_p, EDTS_BUF, &sysargs);

    }
    else
    {
        edt_msg_perror(EDTFATAL, "edt_set_buffer_size: Attempt to set size on unallocated buffer\n");

        return -1;
    }
}

/**
* Change the maximum number of ring buffers that can be allocated.
* The default number of buffers differs depending on the EDT device being accessed,
* and is set to a conservative limit meant to ensure that too many system resources
* are not consumed.  However, application and system architecture may indicate
* more buffers so this call is provided to up the number. The absolute maximum is 1024.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param newmax      new maximum, up to 1024
*
* @return 0 on success, -1 on failure
*
* @see edt_configure_ring_buffers, edt_get_max_buffers
*/

int
edt_set_max_buffers(EdtDev *edt_p, int newmax)

{
    edt_msg(EDTDEBUG, "edt_set_max_buffers\n");

    return
        edt_ioctl(edt_p, EDTS_MAX_BUFFERS, &newmax);
}

/**
* Get the maximum number of ring buffers that can be allocated.
* The default maximum number of ring buffers differs depending on the type
* of EDT device is in use; additionally the maximum can be changed by
* edt_set_max_buffers.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @return the maximum number of buffers that can be allocated, or -1 on failure
*
* @see edt_configure_ring_buffers, edt_set_max_buffers
*/
int
edt_get_max_buffers(EdtDev *edt_p)

{
    u_int val;

    if (edt_p->devid == DMY_ID) val = MAX_DMA_BUFFERS ;
    else edt_ioctl(edt_p, EDTG_MAX_BUFFERS, &val);

    return val;

}


void
edt_resume(EdtDev * edt_p)
{
    u_int dmy ;

    edt_msg(EDTDEBUG, "edt_resume\n") ;
    edt_ioctl(edt_p, EDTS_RESUME, &dmy);
}

/**
* Gets the number of buffers that the driver has been told to acquire.
* This allows an application to know the state of the ring buffers
* within an interrupt, timeout, or when cleaning up on close. It also
* allows the application to know how close it is getting behind the
* acquisition.  It is not normally needed.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @code
* int curdone;
* int curtodo;
* curdone = edt_done_count(edt_p);
* curtodo = edt_get_todo(edt_p);
* // curtodo - curdone is how close the DMA is to catching up with our
* // processing
* @endcode
*
* @return Number of buffers started via #edt_start_buffers.
* @see #edt_done_count, #edt_start_buffers, #edt_wait_for_buffers
* @ingroup dma_inout
*/

uint_t
edt_get_todo(EdtDev * edt_p)
{
    u_int todo ;
    if (edt_p->devid == DMY_ID)
        todo = edt_p->dmy_started ;
    else
        edt_ioctl(edt_p, EDTG_TODO, &todo);
    edt_msg(EDTDEBUG, "edt_get_todo: %d\n",todo) ;
    return todo;
}

uint_t
edt_get_drivertype(EdtDev * edt_p)
{
    u_int type ;
    edt_ioctl(edt_p, EDTG_DRIVER_TYPE, &type);
    edt_msg(EDTDEBUG, "edt_get_drivertype: %x\n",type) ;
    return type;
}

/* only for testing */
int
edt_set_drivertype(EdtDev *edt_p, u_int type)
{
    edt_msg(EDTDEBUG, "edt_set_drivertype\n");
    return edt_ioctl(edt_p, EDTS_DRIVER_TYPE, &type);
}

/**
* Gets the seconds and nanoseconds timestamp in the same format as the
* buffer_timed functions. Used for debugging and coordinating dma
* completion time with other events.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param timep     pointer to an unsigned integer array
*
* \Example
* @code
* int timestamp[2];
* edt_get_reftime(edt_p, timestamp);
* @endcode
*
* @return 0 on success, -1 on failure.  Fills in timestamp pointed to
* by timep.
*
* @see edt_timestamp, edt_done_count, edt_wait_buffers_timed
* @todo longer example showing more realistic use.
* @todo Better explanation of what this is returning. (what is "ref"?)
*/

int
edt_get_reftime(EdtDev * edt_p, u_int * timep)
{
    /* we return sec and nsec - change to usec */
    u_int   timevals[2];

    edt_ioctl(edt_p, EDTG_REFTIME, &timevals[0]);
    edt_msg(EDTDEBUG, "%x %x ",
        timevals[0],
        timevals[1]) ;
    timep[0] = timevals[0];
    timep[1] = timevals[1];
    return (timevals[1]) ;
}

void
edt_set_timetype(EdtDev * edt_p, u_int type)
{
    edt_ioctl(edt_p, EDTS_TIMETYPE, &type);
}

void
edt_set_abortintr(EdtDev * edt_p, u_int val)
{
    edt_ioctl(edt_p, EDTS_ABORTINTR, &val);
}


caddr_t
edt_mapmem(EdtDev *edt_p, u_int addr, int size)
{
    caddr_t     ret;
#if defined(_NT_)
    edt_buf tmp;

    tmp.value = size;
    tmp.desc = addr;

    edt_ioctl(edt_p, EDTS_MAPMEM, &tmp);

    ret = (caddr_t) tmp.value ;
    edt_msg(EDTDEBUG, "edt_mapmem(%x, %x) %x %x\n", addr, size, tmp.desc,tmp.value);
    return (ret);

#elif defined(__APPLE__)

    /* KLUDGE PCI device is already mapped to address 0
     * See EdtPciDriver.cpp start function
     */
    return (addr);


#else
    ret = (caddr_t) mmap((caddr_t)0, size, PROT_READ|PROT_WRITE,
        MAP_SHARED, edt_p->fd, addr);
    if (ret == ((caddr_t)-1)) {
        perror("mmap call");
        return(0) ;
    }
    return(ret) ;
#endif
}

u_int edt_get_mappable_size(EdtDev *edt_p, int bar /* Base Address Range, currently 0 or 1 */)

{
    u_int value = 0;

    if (bar == 1)
        edt_ioctl(edt_p, EDTG_MEM2SIZE, &value);
    else
        edt_ioctl(edt_p, EDTG_MEMSIZE, &value);

    return value;

}


#ifdef __sun

/**
* Return class ID and maximum priority. Input argument name is class name.
* Maximum priority is returned in *maxpri.
*/

static id_t
schedinfo(char *name, short *maxpri)
{
    pcinfo_t info;
    tsinfo_t *tsinfop;
    rtinfo_t *rtinfop;

    (void) strcpy(info.pc_clname, name);
    if (priocntl(0L, 0L, PC_GETCID, (char *) &info) == -1L)
    {
        return (-1);
    }
    if (strcmp(name, "TS") == 0)
    {
        tsinfop = (struct tsinfo *) info.pc_clinfo;
        *maxpri = tsinfop->ts_maxupri;
    }
    else if (strcmp(name, "RT") == 0)
    {
        rtinfop = (struct rtinfo *) info.pc_clinfo;
        *maxpri = rtinfop->rt_maxpri;
    }
    else
    {
        return (-1);
    }
    return (info.pc_cid);
}

/**
* edt_set_RT:  Input arg is a priority - 0 being highest and 9 being lowest.
* Current thread is put into real-time mode at this priority.
*/
void
edt_set_RT(u_int pri)
{
    pcparms_t pcparms;
    rtparms_t *rtparmsp;
    id_t    rtID;
    short   maxrtpri;

    /* Get highest valid RT priority. */
    if ((rtID = schedinfo("RT", &maxrtpri)) == -1)
    {
        edt_msg_perror(EDTFATAL, "schedinfo failed for RT");
        exit(2);
    }

    /* Change proc to RT, highest prio - 1, default time slice */
    pcparms.pc_cid = rtID;
    rtparmsp = (struct rtparms *) pcparms.pc_clparms;
    rtparmsp->rt_pri = maxrtpri - (1 + pri) ;
    rtparmsp->rt_tqnsecs = RT_TQDEF;

    if (priocntl(P_LWPID, P_MYID, PC_SETPARMS, (char *) &pcparms) == -1)
    {
        edt_msg_perror(EDTFATAL, "PC_SETPARMS failed");
        exit(3);
    }
}
#endif /* __sun */


#ifdef PCD
/*
* SSE (fast serial) support.
*/

/* These bits are in hi word of long, must split into two to do AND's, OR's */
#define XPLLBYP (0x1<<17)
#define XPLLCLK (0x1<<18)
#define XPLLDAT (0x1<<19)
#define XPLLSTB (0x1<<20)

#define XC_X16  (0x00)          /* PC Only: hi for SCLK=16MHz, not SCLK=CCLK */
#define XC_AVSEL (0x80)         /* GP Only: hi for SCLK=AVCLK, not SCLK=16MHz*/

#define XC_CCLK (0x01)          /* These codes interpreted by sse_wpp() */
#define XC_DIN  (0x02)          /*  Xilinx config data and clock */
#define XC_PROG (0x04)          /*  inverted in sse_wpp() when driving PROGL */
#define XC_INITL (0x04)         /* Actual bit positions in status register */
#define XC_DONE  (0x08)

/* Write encoded data to parallel port data register */
static void
sse_wpp(EdtDev *edt_p, int val)
{
    unsigned char bits;

    bits = edt_intfc_read(edt_p, PCD_FUNCT) & 0x08;     /* Preserve SWAPEND */

    if (val & XC_DIN)   bits |= 0x04;
    if (val & XC_PROG)  bits |= 0x01;
    if (val & XC_AVSEL) bits |= 0x80;

    edt_intfc_write(edt_p, PCD_FUNCT, bits);

    if (val & XC_CCLK)          /* strobe the clock hi then low */
    {
        edt_intfc_write(edt_p, PCD_FUNCT, (unsigned char) (bits | 0x02));
        edt_intfc_write(edt_p, PCD_FUNCT, bits);
    }

    edt_msg(EDTDEBUG, "sse_wpp(%02x)  ", bits);
}


static int
sse_spal(EdtDev * edt_p, int v)
{
    edt_msleep(1);
    edt_intfc_write(edt_p, PCD_PAL0, (unsigned char) (v >> 0));
    edt_intfc_write(edt_p, PCD_PAL1, (unsigned char) (v >> 8));
    edt_intfc_write(edt_p, PCD_PAL2, (unsigned char) (v >> 16));

    return (edt_intfc_read(edt_p, PCD_PAL3));
}

static void
sse_xosc(EdtDev * edt_p, int val)
{
    int     n, s;       /* First set up MC12430 lines, CLK=0, DAT=0, STB=0 */
    int   gspv = edt_intfc_read(edt_p, PCD_PAL0)
        | edt_intfc_read(edt_p, PCD_PAL1) << 8
        | edt_intfc_read(edt_p, PCD_PAL2) << 16;

    gspv = (gspv & ~XPLLBYP & ~XPLLCLK & ~XPLLDAT & ~XPLLSTB);
    sse_wpp(edt_p, 0);

    edt_msleep(1);              /* Shut down AVSEL */

    for (n = 13; n >= 0; n--)
    {                           /* Send 14 bits of data to MC12430 */
        s = val & (0x1 << n);   /* get the current data bit */

        if (s)
        {
            sse_spal(edt_p, gspv | XPLLDAT);    /* Set up data, then clock it */
            sse_spal(edt_p, gspv | XPLLDAT | XPLLCLK);
        }
        else
        {
            sse_spal(edt_p, gspv);      /* Strobe clock only, no data */
            sse_spal(edt_p, gspv | XPLLCLK);
        }

        edt_msg(EDTDEBUG, "sse_xosc:  n:%d  s:%x\n", n, (s != 0));
    }

    sse_spal(edt_p, gspv | XPLLSTB);    /* Strobe serial_load */
    sse_spal(edt_p, gspv);              /* Clear the strobe */


    if ((val >> 11) == 6)
    {
        gspv |= XPLLBYP;
        sse_spal(edt_p, gspv);

        sse_wpp(edt_p, XC_AVSEL);       /* On GP, drive SCLK from AV9110 */
    }
    else
        sse_wpp(edt_p, 0);


    if (XC_X16)
        sse_wpp(edt_p, XC_X16);         /* On PC, drive SCLK with 16 MHz ref */
}


double
sse_set_out_clk(EdtDev * edt_p, double fmhz)
{                               /* Set ECL clock to freq specified in MHz */
    int     m, n, t, hex, nn, fx;
    edt_pll avp;
    double  avf = 0.0; /* should this have a more reasonable value? --doug */

    if (fmhz < 0)
    {
        printf("sse_set_pll_freq: Invalid argument %f\n", fmhz);
        return 0;
    }

    fx = (int) fmhz;

    if ((fmhz > 800.0) || (fmhz < 0.000050))
    {
        printf("Error, %f MHz requested.  Min of 0.000050, max of 800Mhz\n",
            fmhz);
        return (0);
    }
    else if (fx > 400)
    {
        t = 0;
        n = 3;
        m = fx / 2;
        nn = 1;
    }                           /* Every 2 MHz */
    else if (fx > 200)
    {
        t = 0;
        n = 0;
        m = fx;
        nn = 2;
    }                           /* Every 1 MHz */
    else if (fx > 100)
    {
        t = 0;
        n = 1;
        m = fx * 2;
        nn = 4;
    }                           /* Every 500 KHz */
    else if (fx >= 50)
    {
        t = 0;
        n = 2;
        m = fx * 4;
        nn = 8;
    }                           /* Every 250 KHz */
    else
    {
        avf = edt_find_vco_frequency(edt_p, fmhz * 1E6, (double) 30E6, &avp, 0);
        if (avf != 0)
            edt_set_out_clk(edt_p, &avp);       /* Load AV9110 */
        t = 6;
        n = 3;
        m = 200;
        nn = 4;                 /* Put MC12430 in bypass, use AV9110 */
    }

    hex = (t << 11) | (n << 9) | m;

    sse_xosc(edt_p, hex);       /* Load MC12430 hw */

    if (t != 6)
    {
        edt_msg(EDTDEBUG,
            "sse_set_out_clk:  %f MHz  MC12430: t:%d  n:%d  m:%d  hex:0x%04x\n",
            (2.0 * m / nn), t, n, m, hex);
        return (2.0 * m / nn);  /* Freq of MC12430 in MHz */
    }
    else
    {
        edt_msg(EDTDEBUG,
            "sse_set_out_clk: %f MHz AV9110:  m:%d n:%d v:%d r:%d h:%d l:%d x:%d\n",
            (avf / 1E6), avp.m, avp.n, avp.v, avp.r, avp.h, avp.l, avp.x);
        return (avf / 1E6);     /* Freq of AV9110 in MHz */
    }
}




void
sse_shift(EdtDev *edt_p, int shift)
{
    int n ;
    unsigned char  old;

    if (shift)
    {
        old = edt_intfc_read(edt_p, PCD_PAL1) & ~0x02;
        edt_intfc_write(edt_p, PCD_PAL1, old);

        for (n = 0; n < shift; n++)
        {
            edt_msleep(1);
            edt_intfc_write(edt_p, PCD_PAL1, (unsigned char) (0x02 | old));
            edt_msleep(1);
            edt_intfc_write(edt_p, PCD_PAL1, old);
        }
    }
}
/*
To swallow a clock on channel 0, strobe bit 9,  so PCD_PAL1 0x02
To swallow a clock on channel 1, strobe bit 21, so PCD_PAL2 0x20
*/
void
sse_shift_chan(EdtDev *edt_p, int shift, int channel)
{
    int n ;
    unsigned char  old;

    if (shift) {
        if (channel==0) {
            old = edt_intfc_read(edt_p, PCD_PAL1) & ~0x02;
            edt_intfc_write(edt_p, PCD_PAL1, old);

            for (n = 0; n < shift; n++)
            {
                edt_msleep(1);
                edt_intfc_write(edt_p, PCD_PAL1, (unsigned char) (0x02 | old));
                edt_msleep(1);
                edt_intfc_write(edt_p, PCD_PAL1, old);
            }
        } else {
            old = edt_intfc_read(edt_p, PCD_PAL2) & ~0x20;
            edt_intfc_write(edt_p, PCD_PAL2, old);

            for (n = 0; n < shift; n++)
            {
                edt_msleep(1);
                edt_intfc_write(edt_p, PCD_PAL2, (unsigned char) (0x20 | old));
                edt_msleep(1);
                edt_intfc_write(edt_p, PCD_PAL2, old);
            }
        }
    }
}
#endif /* PCD */


#ifdef P11W

#include "p11w.h"

u_short
p11w_get_command(EdtDev * edt_p)
{
    return (edt_reg_read(edt_p, P11_COMMAND));
}

void
p11w_set_command(EdtDev * edt_p, u_short val)
{
    edt_reg_write(edt_p, P11_COMMAND, val);
}

u_short
p11w_get_config(EdtDev * edt_p)
{
    return (edt_reg_read(edt_p, P11_CONFIG));
}

void
p11w_set_config(EdtDev * edt_p, u_short val)
{
    edt_reg_write(edt_p, P11_CONFIG, val);
}

u_short
p11w_get_data(EdtDev * edt_p)
{
    return (edt_reg_read(edt_p, P11_DATA));
}

void
p11w_set_data(EdtDev * edt_p, u_short val)
{
    edt_reg_write(edt_p, P11_DATA, val);
}

u_short
p11w_get_stat(EdtDev * edt_p)
{
    return (edt_reg_read(edt_p, P11_STATUS));
}

u_int
p11w_get_count(EdtDev * edt_p)
{
    return (edt_reg_read(edt_p, P11_COUNT));
}

/*
* The following routine is deprecated.  Use the one following
* this, p11w_set_abortdma_onintr().     Mark  6/04
*/
void
p11w_abortdma_onattn(EdtDev *edt_p, int flag)
{
    int arg = 0x11 ;
    u_short readcmd ;


    if (flag)
    {
        edt_ioctl(edt_p, P11G_READ_COMMAND, &readcmd) ;
        readcmd |= P11W_EN_ATT ;
        edt_ioctl(edt_p, P11S_READ_COMMAND, &readcmd) ;

        edt_ioctl(edt_p, EDTS_CUSTOMER, &arg) ;
    }
    else
    {
        edt_ioctl(edt_p, P11G_READ_COMMAND, &readcmd) ;
        readcmd &= ~P11W_EN_ATT ;
        edt_ioctl(edt_p, P11S_READ_COMMAND, &readcmd) ;

        arg = 0 ;
        edt_ioctl(edt_p, EDTS_CUSTOMER, &arg) ;
    }
}

void
p11w_set_abortdma_onintr(EdtDev *edt_p, int flag)
{
    u_short readcmd, writecmd ;

    if (flag == 0)
    {
        edt_ioctl(edt_p, P11G_READ_COMMAND, &readcmd) ;
        readcmd &= ~P11W_EN_ATT ;
        edt_ioctl(edt_p, P11S_READ_COMMAND, &readcmd) ;

        edt_ioctl(edt_p, P11G_WRITE_COMMAND, &writecmd) ;
        writecmd &= ~P11W_EN_ATT ;
        edt_ioctl(edt_p, P11S_WRITE_COMMAND, &writecmd) ;
    }

    edt_ioctl(edt_p, EDTS_ABORTDMA_ONINTR, &flag) ;

    if (flag)
    {
        edt_ioctl(edt_p, P11G_READ_COMMAND, &readcmd) ;
        readcmd |= P11W_EN_ATT ;
        edt_ioctl(edt_p, P11S_READ_COMMAND, &readcmd) ;

        edt_ioctl(edt_p, P11G_WRITE_COMMAND, &writecmd) ;
        writecmd |= P11W_EN_ATT ;
        edt_ioctl(edt_p, P11S_WRITE_COMMAND, &writecmd) ;
    }
}

#endif                          /* P11W */

/**
* Gets the device ID of the specified device.
* The board ID can be compared to specific board IDs listed in
* \e edtreg.h. To check if the specific board is part of a broader
* type, like PCD or PDV, macros such as #edt_is_pcd and #edt_is_pdv can
* be used.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*/
int
edt_device_id(EdtDev *edt_p)
{
    if (edt_p)
        return edt_p->devid;
    return -1;
}

/**
* @brief Determines whether a board's PCI firmware is 4-channel or 1-channel
*
* @param edt_p pointer to device structure
*
* @return either 1 or 4 depending on number of channels.
*
* Since we can't tell from the PCI vendor id whether a PCI bitfile for the PCI SS
* or GS is the one-channel or four-channel type, this parses the name for a 1 or a
* 4.
*
*/


int
edt_check_1_vs_4(EdtDev *edt_p)
{
    char name[129];
    int pci_channels = 4;

    edt_flash_get_fname(edt_p, name);

    if (isdigit(name[strlen(name)-1]))
    {
        if (strlen(name) >= 2 && isdigit(name[strlen(name)-2]))
        {
            pci_channels = atoi(name + strlen(name)-2);
        }
        else
        {
            pci_channels = atoi(name + strlen(name)-1);
        }
    }

    return pci_channels;
}


/**
* @brief edtdev_channels_from_type
*
* @param edt_p pointer to device structure
*
* @return the number of channels for a board's devid (PCI minor number).
*
*
*/

int
edtdev_channels_from_type(EdtDev *edt_p)

{
    if (edt_is_4channel(edt_p)) return 4;
    if (edt_is_1or4channel(edt_p)) return edt_check_1_vs_4(edt_p);
    if (edt_is_2channel(edt_p)) return 2;
    if (edt_is_3channel(edt_p)) return 3;
    if (edt_is_1553(edt_p)) return 34;
    if (edt_is_16channel(edt_p)) return 16;
    if (edt_is_32channel(edt_p)) return 32;
    return 1;

}

/**
* Converts the board ID returned by #edt_device_id to a human readable
* form.
*/
int
edt_devtype_from_id(int id)
{
    if (id == P11W_ID) return p11w;
    if (id == P16D_ID) return p16d;
    if (ID_IS_PCD(id)) return pcd;
    if (ID_IS_PDV(id)) return pdv;
    if (ID_IS_DUMMY(id)) return dummy;
    return unknown;
}



/**
* Converts the board ID returned by #edt_device_id to a human readable
* form (new version, with promcode).  Supersedes edt_idstr which didn't take promcode
* now needed with new 'a' boards, some of which are detected via combination of ID and PROM code.
*
* @param id the board's hardware ID
* @param promcode the board's prom code, as defined in libedt.h
* @return The id string of this board, e.g. "pcie4 dva c-link"
*/
char *
edt_idstring(int id, int promcode)
{
    if (promcode == AMD_XC5VLX30T_A) {
        switch(id) {
            case PE4DVCL_ID:    return ("pcie4 dva c-link");
            case PE8DVCL_ID:    return ("pcie8 dva c-link");
            case PE8DVCLS_ID:   return ("pcie8 dva cls");
        }
    }
    return edt_idstr(id);
}

/**
* Converts the board ID returned by #edt_device_id to a human readable
* form (original version, sans promcode).  For new 'a' boards that used the
* same devid as older versions (i.e. PCIe8 DV C-Link, PCIe4 DVa c-link, PCIe8 DV CLS) this subroutine
* will return without the 'a' suffix; therefore this subroutine should no longer be called
* directly; instead use edt_idstring() to make sure those boards get properly IDd.
*
* @param id the board's hardware ID
* @return The id string of this board, with no check to see if it's an 'a board' (e.g. "pcie4 dv c-link")
*/
char *
edt_idstr(int id)
{
    switch(id)
    {

/* ALERT: SEE EDT_IDSTRING FOR THESE GUYS which need promcode to distinguish between 'a' and non 'a' versions
*    case PE4DVCL_ID:     return ("pcie4 dva c-link");
*    case PE8DVCL_ID:     return ("pcie8 dva c-link");
*    case PE8DVCLS_ID:    return ("pcie8 dva cls");
*/
    case PE1DVVL_ID:       return ("VisionLink F1");
    case PE4DVVL_ID:       return ("VisionLink F4");
    case PE8VLCLS_ID:      return ("VisionLink CLS");
    case PE4DVVLFOX_ID:    return ("VisionLink FOX4");

    case P11W_ID:          return("pci 11w");
    case P16D_ID:          return("pci 16d");
    case PDV_ID:           return("pci dv");
    case PDVA_ID:          return("pci dva");
    case PDVA16_ID:        return("pci dva16");
    case PDVK_ID:          return("pci dvk");
    case PDVRGB_ID:        return("pci dv-rgb");
    case PDV44_ID:         return("pci dv44");
    case PDVCL_ID:         return("pci dv c-link");
    case PDVCL2_ID:        return("pci dv cls");
    case PE4DVCL_ID:       return("pcie4 dv c-link");
    case PE8DVCL_ID:       return("pcie8 dv c-link");
    case PE8DVCLS_ID:      return("pcie8 dv cls");
    case PDVAERO_ID:       return("pcd dv aero serial");
    case PCD20_ID:         return("pci cd-20");
    case PCD40_ID:         return("pci cd-40");
    case PCD60_ID:         return("pci cd-60");
    case PCDA_ID:          return("pci cda");
    case PCDCL_ID:         return("pci cd cl");
    case PGP20_ID:         return("pci gp-20");
    case PGP40_ID:         return("pci gp-40");
    case PGP60_ID:         return("pci gp-60");

    case PGP_THARAS_ID:    return("pci gp-tharas");
    case PGP_ECL_ID:       return("pci gp-ecl");
    case PCD_16_ID:        return("pci cd-16");

    case PCDA16_ID:        return("pci cda16");
    case PE4CDA_ID:        return("pcie4 cda");
    case PE4CDA16_ID:      return("pcie4 cda16");

    case PDVFCI_AIAG_ID:   return("pci-fci aiag");
    case PDVFCI_USPS_ID:   return("pci-fci usps");
    case PCDFCI_SIM_ID:    return("pci-fci sim");
    case PCDFCI_PCD_ID:    return("pci-fci pcd");
    case PCDFOX_ID:        return("pcd fox");
    case PDVFOX_ID:        return("pci dv fox");
    case PE4DVFOX_ID:      return("pcie4 dva fox");
    case PE8DVFOX_ID:      return("pcie8 dva fox");
    case P53B_ID:          return("pci 53b");
    case PDVFOI_ID:        return("pdv foi");
    case PSS4_ID:          return("pci ss-4");
    case PSS16_ID:         return("pci ss-16");
    case PGS4_ID:          return("pci gs-4");
    case PGS16_ID:         return("pci gs-16");
    case PE8LX1_ID:        return("pcie8 lx-1");
    case PE8LX16_LS_ID:    return("pcie8 lx-16 (slow)");
    case PE8LX16_ID:       return("pcie8 lx-16");
    case PE8LX32_ID:       return("pcie8 lx-32");
    case PE4AMC16_ID:      return("pcie4 AMC16");
    case PE8G3S5_ID:       return("pcie8 g3s5");
    case PE8G3A5_ID:       return("pcie8 g3a5");
    case PE8G3KU_ID:       return("pcie8 g3ku");
    case PE8G2CML_ID:      return("pcie8 g2cml");
    case WSU1_ID:          return("wsu 1");
    case SNAP1_ID:         return("snap 1");
    case DMY_ID:           return("dummy");
    case DMYK_ID:          return("dummy pci dvk/44/foi");
    case PC104ICB_ID:      return("pc104 icb");
    case PE4BL_RADIO_ID:   return("pcie4 radio blade");
    case PE4BL_RXLFRADIO_ID: return("pcie4 rx/lf radio blade");
    case PE4BL_TXLFRADIO_ID: return("pcie4 tx/lf radio blade");
    case PE1BL_TIMING_ID:  return("pcie1 timing blade");
    case PE8BL_NIC_ID:     return("pcie8 nic blade");
    case LCRBOOT_ID:       return("lcr boot");
    case PE8G2V7_ID:       return("pcie8 g2v7");
    case PE8BL_WBDSP_ID:   return("pcie8 dsp wideband blade");
    case PE1BL_WBADC_ID:   return("pcie1 analog wideband blade");
    default:               return("unknown");
    }
}

/*
 * Return the PCI FPGA boot-sector header string.
 *
 * EDT PCI FPGA headers are embedded in the device's FPGA PROM and identify the specific FPGA
 * firmware running on the board. The specific format is hardware-dependent, but typically
 * consists of the name, build id, date, and time, with each field separated by whitespace.
 *
 * By convention, EDT PCI FPGA names are of the form name-XX.ext where XX is the rev number and
 * ext is the extension. The rev number will be present in the header string but is typically omitted
 * in the actual file name. Additionally, the extension is usually different -- in the header it will
 * likely be .ncd or .rpd, whereas the filename extension is virtually always .bit.  For example, the
 * PROM header name edt_firmware-02.rbt would most likely match a file named edt_firmware.bit in your
 * package (to find out whether a  given FPGA firmware file matches the one in the board's PROM,
 * use pciload verify.)
 *
 * @param edt_p pointer to edt device structure returned by #edt_open
 * @param name character string to store the device (must be of sufficient length to store the header
 * string; typically less than 80 characters but could be up to (and will be truncated to) 128 including
 * the trailing NULL.
 * @return
 */
char *
edt_get_flash_prom_header(EdtDev *edt_p, char *name)
{
    u_short stat;
    u_char jumpers, idbits, xidbits;
    int promcode;
    EdtPromData pdata;
    Edt_prominfo *ep;

    promcode = edt_flash_prom_detect(edt_p, &stat);
    ep = edt_get_prominfo(promcode);
    jumpers = stat & 0x3;
    idbits = (stat >>2) & 0x1f;
    xidbits = stat >> 8;

    ep = edt_get_prominfo(promcode);
    edt_read_prom_data(edt_p, promcode, ep->defaultseg, &pdata);

    if (promcode > edt_get_max_promcode())
        sprintf(name, "unknown [%02x %02x]", idbits, xidbits);
    else strncpy(name, pdata.id, 127);

    return name;
}

/**
 * Gets the hardware revision number, from the FLASH prom embedded info
 * @param edt_p pointer to edt device structure returned by #edt_open
 * @return the hardware revision, or 0 if no error / no revision
 */
int
edt_get_hw_rev(EdtDev *edt_p)
{
    int      promcode;
    u_short  stat;

    EdtPromData pdata;
    Edt_embinfo ei;
    Edt_prominfo *ep;

    promcode = edt_flash_prom_detect(edt_p, &stat);
    ep = edt_get_prominfo(promcode);
    edt_read_prom_data(edt_p, promcode, ep->defaultseg, &pdata);
    if (pdata.esn[0] && (edt_parse_devinfo(pdata.esn, &ei) == 0))
      return ei.rev;
    return 0;
}

/**
* Performs a UNIX-like system() call which passes the argument
* strings to a shell or command interpreter, then returns the exit status
* of the command or the shell so that errors can be detected.  In WINDOWS
* spawnl() must be used instead of system() for this purpose.
*/
#if defined(VXWORKS) || defined(TEST_VXWORKS)
/* writable command string for vxworks calling edt_system() */
char edt_vxw_cmdstr[512] = {0};

/*
* The following two arrays are used to register a function name with a
* function pointer for vxworks calling edt_system().  Only 32 unique
* program names are supported; most programs use less than 5.
*
* STRATEGY:  Preference is simplicity, but works with most applications.
*/
static char *program_func_name[32] = {NULL};
static void * edt_vx_system_func[32] = {NULL};

/*
* VxWorks programs calling edt_system() must first register the progam name
* and the program function.
*/
int
edt_vx_system_register_func(const char *funcstr, int (funcptr)(char *))
{
    int i;

    for (i = 0; i < 32; i++)
    {
        if (program_func_name[i] == NULL)
            break;
    }

    if (i == 32)
        return(-1);
    else
    {
        program_func_name[i] =  edt_alloc(strlen(funcstr) + 1);
        strcpy(program_func_name[i], funcstr);
        edt_vx_system_func[i] = funcptr;
        return 0;
    }
}
#endif

int
edt_system(const char *cmdstr)
{
#if defined(_NT_)

    int ret = -1;
    char newstr[512] = {'\0'};
    char *p = newstr;
    int nowait = 0;
    char *arg[32] = {NULL};
    int arg_cnt = 0;

    if (cmdstr == NULL || *cmdstr == '\0')
        return -1;

    strncpy(newstr, cmdstr, sizeof(newstr) - 1);
    for (arg_cnt = 0; (arg_cnt < 32) && (p && *p); arg_cnt++)
    {
        while (*p == ' ' || *p == '\t')
            *p++ = '\0';

        arg[arg_cnt] = p;

        while ((*p != ' ') && (*p != '\t') && (*p != '\0'))
            ++p;
    }

    /* Convert UNIX '&' for running process in background to NT "nowait".
     * Mark end of argument list with NULL pointer.
     */
    if (*arg[arg_cnt-1] == '&')
    {
        arg[arg_cnt-1] = NULL;
        nowait = 1;
    }

    ret = (int)spawnv((nowait) ? _P_NOWAIT : _P_WAIT, arg[0], arg);
    return ret;

#elif defined(VXWORKS) || defined(TEST_VXWORKS)
    /*
    * VxWorks programs calling edt_system() must first register the progam name
    * and the program function.  Here the string argument to edt_system() is
    * parsed into the command name and options.  The command name is looked up
    * and matched with a function pointer, then the function is called with
    * the option string.
    */
    {
        int i;
        char *cmd;
        char *opts;
        char *tmp;
        int (*fnp)(char *);

        strncpy(edt_vxw_cmdstr, cmdstr, 511);
        cmd = edt_vxw_cmdstr;
        opts = strchr(edt_vxw_cmdstr, ' ');

        if (opts)
            *opts++ = '\0';

        tmp = strrchr(edt_vxw_cmdstr, '/');
        if (tmp)
            cmd = tmp + 1;


        edt_msg(EDTDEBUG, "VXWORKS_test: edt_system(%s, %s)\n", cmd, opts);
#if defined(TEST_VXWORKS)
        printf("VXWORKS_test: edt_system(%s, %s)\n", cmd, opts);
#endif

        for (i = 0; i < 32; i++)
        {
            if (program_func_name[i] == NULL)
                return -1;
            else if (strncmp(program_func_name[i], cmd, 511) == 0)
            {
                fnp = (int (*)(char*)) edt_vx_system_func[i];
                edt_msg(EDTDEBUG, "VXWORKS_test: found name %s func %p\n", cmd, fnp);
                return fnp(opts);
            }
        }

        if (i == 32) /* Not found */
        {
            edt_msg(EDTDEBUG, "VXWORKS_test: name not found %s, must be registered with edt_vx_system_register_func()\n", cmd);
            return -1;
        }
    }
    return(0);
}
#else

    int                ret ;

    ret      = system(cmdstr) ;

#if defined(__sun) || defined(__linux__) || defined(__APPLE__)
    if (ret == -1 || WIFEXITED(ret) == 0)
        return -1 ;
    else
        return WEXITSTATUS(ret)      ;
#else
    return ret ;
#endif
#endif
}

#define MEZZ_PATH_FIELD_CHAR '|'

char *
edt_get_last_bitpath(EdtDev *edt_p)

{
    FILE *f;
    static char cache_name[MAXPATH];


    sprintf(cache_name, ".interface_bitfile.%d", edt_p->unit_no);

    if ((f = fopen(cache_name, "r")))
    {

        volatile char *throwaway = fgets(cache_name, MAXPATH, f);

        if (strlen(cache_name))
        {

            if (cache_name[strlen(cache_name)-1] == '\n')
                cache_name[strlen(cache_name)-1] = 0;
        }

        fclose(f);

        return cache_name;
    }

    return NULL;
}


void
edt_set_last_bitpath(EdtDev *edt_p, const char *fullpath)

{
    FILE *f;
    char cache_name[MAXPATH];

    sprintf(cache_name, ".interface_bitfile.%d", edt_p->unit_no);

    if ((f = fopen(cache_name, "w")))
    {

        fputs(fullpath, f);

        fclose(f);

    }
}


/**
* Sets pathname to the currently loaded mezzanine bitfile in the
* driver.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param bitpath  address of a character buffer of at most 128 bytes
* @param channel  which of two channels (0 or 1) this refers to
*
*
* @return 0 on success, -1 on failure.
* @see edt_get_mezz_bitpath, edt_set_bitpath
*/
int
edt_set_mezz_chan_bitpath(EdtDev *edt_p, const char *bitpath, int channel)
{
    edt_bitpath pathbuf, chan0, chan1;
    const char *s1;
    int sp;
    int rc;

    /* scan back for / in path - we may not have room for full path */

    if (strlen(bitpath))
    {
        sp = (int) strlen(bitpath) - 1;

        while (sp >= 0 && bitpath[sp] != '/' && bitpath[sp] != '\\')
            sp --;

        if (sp >= 0)
            sp++;
        else
            sp = 0;
    }
    else
        sp = 0;

    s1 = bitpath + sp;

    edt_get_mezz_chan_bitpath(edt_p,chan0, sizeof(chan0),0);
    edt_get_mezz_chan_bitpath(edt_p,chan1, sizeof(chan1),1);

    if (channel == 0)
        sprintf(pathbuf,"%s%c%s",s1,MEZZ_PATH_FIELD_CHAR,chan1);
    else
        sprintf(pathbuf,"%s%c%s",chan0, MEZZ_PATH_FIELD_CHAR, s1);

    rc = edt_ioctl(edt_p, EDTS_MEZZ_BITPATH, pathbuf);

    edt_get_mezz_chan_bitpath(edt_p, edt_p->bfd.mezz_name0,
        sizeof(edt_p->bfd.mezz_name0),0);
    edt_get_mezz_chan_bitpath(edt_p, edt_p->bfd.mezz_name1,
        sizeof(edt_p->bfd.mezz_name1),1);

    return rc;
}


/**
* Obtains pathname to the currently loaded mezzanine bitfile from the
* driver. The #edt_bitload sets this string in the driver when a
* mezzanine bitfile is successfully loaded.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param bitpath  address of a character buffer of at least 128 bytes
* @param size     number of bytes in the above character buffer
*
* @return 0 on success, -1 on failure.
* @see edt_get_bitpath
*/

int
edt_get_mezz_chan_bitpath(EdtDev *edt_p, char *bitpath, int size, int channel)
{
    int ret = 0 ;
    edt_bitpath pathbuf ;
    char *s2, *s3;
    int sp;
    /* split into channels */

    ret = edt_ioctl(edt_p, EDTG_MEZZ_BITPATH, pathbuf);

    s2 = pathbuf;
    sp = 0;
    while (sp < (int)strlen(pathbuf) && pathbuf[sp] != MEZZ_PATH_FIELD_CHAR)
        sp++;

    s3 = s2 + sp;
    if (s3[0] == MEZZ_PATH_FIELD_CHAR)
    {
        s3[0] = 0;
        s3++;
    }

    if (channel == 0)
        strncpy(bitpath, s2, size - 1) ;
    else
        strncpy(bitpath, s3, size - 1) ;

    return ret ;
}

/**
* Sets pathname to the currently loaded mezzanine bitfile in the
* driver.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param bitpath  address of a character buffer of at most 128 bytes
*
* @return 0 on success, -1 on failure.
* @see edt_get_mezz_bitpath, edt_set_bitpath
*/
int
edt_set_mezz_bitpath(EdtDev *edt_p, const char *bitpath)
{
    edt_bitpath pathbuf;
    int rc;
    int i = (int) strlen(bitpath)-1;

    while (i>= 0 && bitpath[i] != '\\' && bitpath[i] != '/')
        i--;

    if (i >= 0)
        i++;
    else
        i = 0;

    strncpy(pathbuf, bitpath+i, sizeof(edt_bitpath) - 1) ;
    rc =  edt_ioctl(edt_p, EDTS_MEZZ_BITPATH, pathbuf);

    return rc;
}



/**
* Obtains pathname to the currently loaded mezzanine bitfile from the
* driver. The #edt_bitload sets this string in the driver when a
* mezzanine bitfile is successfully loaded.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param bitpath  address of a character buffer of at least 128 bytes
* @param size     number of bytes in the above character buffer
*
* @return 0 on success, -1 on failure.
* @see edt_get_bitpath
*/
int
edt_get_mezz_bitpath(EdtDev *edt_p, char *bitpath, int size)
{
    int ret = 0 ;
    edt_bitpath pathbuf ;

    ret = edt_ioctl(edt_p, EDTG_MEZZ_BITPATH, pathbuf);
    strncpy(bitpath, pathbuf, size - 1) ;
    return ret ;
}


/**
* Sets pathname to the currently loaded user interface bitfile in the
* driver.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param bitpath  address of a character buffer of at most 128 bytes
*
* @return 0 on success, -1 on failure.
* @see edt_get_bitpath, edt_set_mezz_bitpath
*/
int
edt_set_bitpath(EdtDev *edt_p, const char *bitpath)
{
    edt_bitpath pathbuf ;
    int rc;

    strncpy(pathbuf, bitpath, sizeof(edt_bitpath) - 1) ;
    rc =  edt_ioctl(edt_p, EDTS_BITPATH, pathbuf);

    edt_get_bitname(edt_p, edt_p->bfd.bitfile_name,
        sizeof(edt_p->bfd.bitfile_name));

    edt_set_last_bitpath(edt_p, bitpath);

    return rc;
}


/**
* Obtains pathname to the currently loaded interface bitfile from the
* driver. The program \e bitload sets this string in the driver when an
* interface bitfile is successfully loaded.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param bitpath  address of a character buffer of at least 128 bytes
* @param size     number of bytes in the above character buffer
*
* @return 0 on success, -1 on failure.
* @see edt_set_bitpath
*/
int
edt_get_bitpath(EdtDev *edt_p, char *bitpath, int size)
{
    int ret = 0 ;
    edt_bitpath pathbuf ;
    pathbuf[0] = '\0';
    ret = edt_ioctl(edt_p, EDTG_BITPATH, pathbuf);

    strncpy(bitpath, pathbuf, size - 1) ;
    return ret ;
}

/**
* Obtains the name of the currently loaded interface bitfile from the
* driver. The program \e bitload sets this string in the driver when an
* interface bitfile is successfully loaded.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param bitpath  address of a character buffer of at least 128 bytes
* @param size     number of bytes in the above character buffer
*
* @return 0 on success, -1 on failure.
* @see edt_set_bitpath
*/
int
edt_get_bitname(EdtDev *edt_p, char *bitpath, int size)
{
    int ret = 0 ;
    char *name;
    edt_bitpath pathbuf ;

    pathbuf[0] = '\0';
    ret = edt_ioctl(edt_p, EDTG_BITPATH, pathbuf);

    /*  Remove the leading path from the bitfile name */
    if ((name = strrchr(pathbuf, '/')) || (name = strrchr(pathbuf, '\\')))
        ++ name;
    else
        name = pathbuf;

    strcpy(bitpath, name) ;

    return ret ;
}

/**
* Gets the version of the EDT driver.
* The version string is the same format as that returned by
* #edt_get_library_version.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param version a string large enough to hold the version information
* (64 bytes is sufficient).
* @param size the size, in bytes, of the user-allocated string
*/
int
edt_get_driver_version(EdtDev *edt_p, char *version, int size)
{
    int ret = 0 ;
    edt_version_string vbuf ;

    ret = edt_ioctl(edt_p, EDTG_VERSION, vbuf);

    strncpy(version, vbuf, size - 1) ;
    return ret ;
}

/**
* Gets the full build ID of the EDT library.
* The build ID string is the same format as that returned by
* #edt_get_library_buildid.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param build a string large enough to hold the build information
* (128 bytes is sufficient).
* @param size the size, in bytes, of the user-allocated string
* @see edt_get_library_buildid
*/
int
edt_get_driver_buildid(EdtDev *edt_p, char *build, int size)
{
    int ret = 0 ;
    edt_version_string vbuf ;

    ret = edt_ioctl(edt_p, EDTG_BUILDID, vbuf);

    strncpy(build, vbuf, size - 1) ;
    return ret ;
}

static char * edt_library_version = EDT_LIBRARY_VERSION;
static char * edt_library_buildid = FULLBUILD;

/**
* Gets the version (number and date) of the EDT library.
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param version a string large enough to hold the version information
* (64 bytes is sufficient).
* @param size the size, in bytes, of the user-allocated string
* @see edt_get_driver_version
*/
int
edt_get_library_version(EdtDev *edt_p, char *version, int size)
{
    strncpy(version, edt_library_version, size - 1) ;
    return 0 ;
}

u_int
edt_get_version_number()

{
    char id_n[64];

    u_int vi = 0;
    int starti;
    int i;

    strncpy(id_n, edt_library_version, 63);

    starti = 0;
    for (i=0;; i++)
    {
        if (!isdigit(id_n[i]))
        {

            vi = (vi << 8) | atoi(id_n+starti);

            starti = i+1;

            if (id_n[i] != '.')
                break;
        }


    }

    return vi;


}

/**
* Gets the full build ID of the EDT library.
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param build a string large enough to hold the build information
* (128 bytes is sufficient).
* @param size the size, in bytes, of the user-allocated string
* @see edt_get_driver_buildid
*/
int
edt_get_library_buildid(EdtDev *edt_p, char *build, int size)
{
    strncpy(build, edt_library_buildid, size - 1) ;
    return 0 ;
}


/** compares version strings between library and driver, returns 0 if they
aren't the same */

int
edt_check_version(EdtDev *edt_p)

{
    edt_version_string check;

    edt_get_driver_version(edt_p, check, sizeof(check));

    return (!strcmp(check,edt_library_version));

}

/**
* pci_reboot -- yanked from pdb reboot
*/
static u_int
epr_cfg(EdtDev *edt_p, int addr)
{
    int ret ;
    edt_buf buf ;
    buf.desc = addr ;
    if ((addr<0) || (addr>0x3c)) {
        edt_msg(EDTWARN, "epr_cfg: addr out of range\n");
        return(0);
    }
    ret = edt_ioctl(edt_p,EDTG_CONFIG,&buf) ;
    if (ret < 0)
    {
        edt_perror("EDTG_CONFIG") ;
    }
    return(u_int) (buf.value) ;
}

static void
epw_cfg(EdtDev *edt_p, int addr, int value)
{
    int ret ;
    edt_buf buf ;
    buf.desc = addr ;
    if ((addr<0) || (addr>0x3c)) {
        edt_msg(EDTWARN, "epw_cfg: addr out of range\n");
        return;
    }
    buf.value = value ;
    ret = edt_ioctl(edt_p,EDTS_CONFIG,&buf) ;
    if (ret < 0)
    {
        edt_perror("EDTS_CONFIG") ;
    }
}

/**
* reboot the PCI xilinx
*
* RETURNS: 0 on success, -1 on failure
*/
int
edt_pci_reboot(EdtDev *edt_p)
{
    int  addr, data;
    int  buf[0x40];
    int  rst, copy, new_one;
    int  ret = 0;
    FILE *fd2;
    char *tmpfname = "TMPfPciReboot.cfg";

    if ((fd2 = fopen(tmpfname, "wb")) == NULL)      {
        edt_msg(EDTFATAL,"edt_pci_reboot: Couldn't write to temp file %s\n", tmpfname);
        return -1;
    }
    for (addr=0; addr<=0x3c; addr+=4) {
        data  = epr_cfg(edt_p, addr);
        buf[addr] = data;
        putc(data, fd2);
        putc(data>>8, fd2);
        putc(data>>16, fd2);
        putc(data>>24, fd2);
        /* printf("%02x:  %08x\n", addr, data); */
    }
    fclose(fd2);
    edt_msg(EDTDEBUG, "Wrote config space state out to %s\n", tmpfname);

    edt_reg_write(edt_p, 0x01000085, 0x40) ;
    edt_msleep(2000) ;

    edt_msg(EDTDEBUG, "  copy     reset     new\n");
    for (addr=0; addr<=0x3c; addr+=4) {
        rst  = epr_cfg(edt_p, addr);
        copy  =  buf[addr];
        epw_cfg(edt_p, addr, copy) ;
        new_one  = epr_cfg(edt_p, addr);

        edt_msg(EDTDEBUG, "%02x:  %08x  %08x  %08x       ", addr, copy, rst, new_one);
        if      (copy != new_one)       edt_msg(EDTDEBUG, "ERROR\n");
        else if (rst != new_one)    edt_msg(EDTDEBUG, "changed\n");
        else                    edt_msg(EDTDEBUG, "\n");

        /* specifically check line cache reg to make sure it was reset by OS after cleared */
        if (addr == 0x0c)
        {
            if ((rst & 0xff) == (new_one & 0xff))
                ret = -1;
        }

    }
    return ret;
}

/**
* set merge params
*/
int
edt_set_merge(EdtDev * edt_p, u_int size, int span, u_int offset, u_int count)
{
    edt_merge_args tmp;
    int     ret;

    tmp.line_size = size ;
    tmp.line_span = span;
    tmp.line_offset = offset;
    tmp.line_count = count;
    ret = edt_ioctl(edt_p, EDTS_MERGEPARMS, &tmp);
    return (ret);
}

/* flip the bits inside a byte */
u_char
edt_flipbits(u_char val)
{
    int     i;
    u_char  ret = 0;

    for (i = 0; i < 8; i++)
    {
        if (val & (1 << i))
            ret |= 0x80 >> i;
    }
    return (ret);
}


/*
*
* edt_set_kernel_buffers
*
* Turn on or off the use of intermediate kernel buffers for DMA
* Use for instance in Linux > 4 Gb, where DMA fails for high memory
*
*/

int
edt_set_kernel_buffers(EdtDev * edt_p, int onoff)

{
    uint_t val = (uint_t) onoff;

    edt_ioctl(edt_p, EDTS_DRV_BUFFER, &val);

    return val;
}
/*
*
* edt_get_kernel_buffers
*
* Returns the current kbufs state
*
*/

int
edt_get_kernel_buffers(EdtDev * edt_p)

{
    uint_t val = (uint_t) -1;

    if (edt_p->devid == DMY_ID)
        val = 0 ;
    else
        edt_ioctl(edt_p, EDTS_DRV_BUFFER, &val);

    return val;
}

int
edt_set_mmap_buffers(EdtDev *edt_p, int state)

{

    uint_t val = edt_get_kernel_buffers(edt_p) & ~EDT_MMAP_KBUFS;

    val |= (state)?EDT_MMAP_KBUFS:0;

    return edt_ioctl(edt_p, EDTS_DRV_BUFFER, &val);


}

int
edt_get_mmap_buffers(EdtDev *edt_p)

{

    int rc = (edt_get_kernel_buffers(edt_p) & EDT_MMAP_KBUFS) != 0;

    return rc;
}

int
edt_get_persistent_buffers(EdtDev * edt_p)

{
    uint_t val = (uint_t) -1;

    edt_ioctl(edt_p, EDTS_DRV_BUFFER, &val);

    printf("val = %d\n", val);

    return (val & EDT_PERSISTENT_KBUFS) != 0;
}

int
edt_set_persistent_buffers(EdtDev * edt_p, int state)

{

    uint_t val = edt_get_kernel_buffers(edt_p) & ~EDT_PERSISTENT_KBUFS;

    val |= (state)?EDT_PERSISTENT_KBUFS:0;

    return edt_ioctl(edt_p, EDTS_DRV_BUFFER, &val);

}

/**
* Gets information about active dma. Use this function to determine whether
* this or another open process has enabled DMA or image acquisition on any
* channel of a specific board (unit)
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param dmainfo pointer to struct of type \c edt_dma_info (defined in
* \c libedt.h ), which has three fields containing hex values, with each bit
* in a field representing a channel that has been used, allocated or is
* currently active, as follows:
* @code
* typedef struct
* {
*     uint_t    used_dma ;   // which channels have started dma within current open/close
*     uint_t    alloc_dma ;  // which channels have has allocated > 1 ring buffer
*     uint_t    active_dma ; // which channels have dma active right now
* } edt_dma_info ;
* @endcode
*
* \Example
* @code
* // this code checks whether this or some other process has done or is
* // currently doing DMA on a given unit / channel, and prints out a warning
* // if there is a possibility of a conflict based on the results
*
* edt_dma_info tmpinfo ;
* EdtDev *edt_p = edt_open_channel(EDT_INTERFACE, unit, channel);
* u_int tmpmask = edt_get_dma_info(edt_p, &tmpinfo);
*
* if (tmpinfo.active_dma & (1 << channel))
* {
*     printf("Warning: DMA is currently active on unit %d ch. %d.\n", unit, channel);
*     printf("It is not safe to start another DMA on this unit/channel at this time.\n");
* }
* if (tmpinfo.used_dma & 1 << channel))
* {
*     printf("Warning: this or another process has already opened and done DMA on unit %d channel %d.\n", unit, channel);
*     printf("It may not be safe to start DMA on this unit/channel outside the currently opened process.\n");
* }
* @endcode
*
* @return mask of all of the above or'd together
* @see setdebug.c utility for example of use
*/
u_int
edt_get_dma_info(EdtDev * edt_p, edt_dma_info *dmainfo)
{
    edt_dma_info tmpinfo ;

    edt_ioctl(edt_p, EDTG_DMA_INFO, &tmpinfo);

    if (dmainfo) *dmainfo = tmpinfo ;

    return (tmpinfo.used_dma | tmpinfo.alloc_dma | tmpinfo.active_dma) ;
}

/**
* Reads the default part number->fpga cross-reference file edt_parts.xpn in the current directory, and provides the
* FPGA if a match is found.
*
* Equivalent to calling edt_find_get_xref_info with edt_parts.xpn as the filename.
* See /ref edt_find_xref_fpga for complete description.
*
* @param fpga is a character array into which the fpga type will be
* stored (e.g. 'xc2s100e' will be returned for the part_number '01901933').
* An array of 128 bytes will be more than enough for the foreseeable future.
*
* @return 1 if found 8 or 10 digit match, 0 if not
*/
int
edt_find_xpn(char *part_number, char *fpga)
{
    return edt_get_xref_info("./edt_parts.xpn", (const char *)part_number, fpga, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/**
 * Reads a part number->fpga cross-reference file and provides the fpga and base serial number if a match is found.
 *
 * Opens the file specified in the \a path argument (e.g. <em>edt_parts.xpn</em>) and
 * compares the entries with the provided part number. If a match is found, it will be copied
 * to the \a fpga argument. Will also copy a serial number if found. Format of the file is ASCII text,
 * one line per part number, as follows:
 *
 *  part_number fpga serial description (serial is optional and not present in earlier files)
 *
 * Anything after the third item is ignored, and can be blank but should typically be the
 * description (name of the device). Since files originally had only two fields and no serial
 * number, an attempt is made to determine if the 3rd field looks like a serial # and copies that if
 * so, otherwise sets the first character null.
 *
 * @param path const character array containing path of the xref fpga file (typ. edt_parts.xpn).
 * @param part_number character array in which to store the part number, should be 8 or 10 digits.
 * The last 2 digits of 10 digit part no. are the rev no. If a match with a 10-digit number is found,
 * returns with the info from that one. If no 10-digit match is found but an 8-digit is found, returns
 * with that info. That way we can have some numbers return a match regardless of rev, and others that
 * cover a specific rev that takes precedence.
 * @param fpga a character array (64-bytes is sufficient) into which the fpga will be stored (e.g. xc2s100e' will
 * be returned for the part_number '01901933'). If NULL this parameter will be ignored.
 * @param serial a character array into which the base serial number will be stored. An array of 64 bytes is sufficient, or NULL (ignored).
 * @param mac_type: a character array (8 bytes is sufficient) into which the mac address board type (as a character string) will be stored. If NULL this parameter will be ignored.
 * @param mac_offset: a character array (8 bytes is sufficient) into which the mac address offset will be stored. If NULL this parameter will be ignored.
 * @param nmacs: a character array (8 bytes is sufficient) into which the number of mac addresses (as a character string) for this board type will be stored. If NULL this parameter will be ignored.
 * @param rsvd1: reserved
 * @param rsvd2: reserved
 *
 * @return number of parameters successfully assigned, or 0 if none.
 */
int
edt_get_xref_info(const char *path, const char *pn, char *fpga, char *sn, char *mtype, char *moffs, char *mcount, char *desc, char *rsvd1, char *rsvd2)
{
    int ret = 0;
    FILE *xfp;
    char str[EDT_STRBUF_SIZE];
    char xf_pn[EDT_STRBUF_SIZE], xf_fpga[EDT_STRBUF_SIZE];

    if ((xfp = fopen(path, "r")) == NULL)
    {
        char errmsg[EDT_STRBUF_SIZE];
        sprintf(errmsg, "couldn't open '%s'\n", path);
        edt_msg(EDTWARN, errmsg);
        return 0;
    }

    while (fgets(str, EDT_STRBUF_SIZE, xfp) != NULL)
    {

        if ((strlen(str) > 10) && (*str >= '0') && (*str <= '9')) /* valid lines begin with a part number */
        {
            int n;

            /* check for part number match, make sure there's at least one more field (fpga) */
            if ((n = sscanf(str, "%s %s", xf_pn, xf_fpga)) == 2)
            {
                /* first check for 10-digit part number match; if found use this line and return */
                if ((strlen(xf_pn) == 10) && (strcasecmp(xf_pn, pn) == 0))
                {
                    ret = get_xref_info_params(str, fpga, sn, mtype, moffs, mcount, desc, rsvd1, rsvd2);

                    fclose(xfp);
                    return ret;
                }

                /* no 10-digit so far, check for 8-digit but don't return yet, */
                /* in case there's a 10-digit one still to be found */
                else if ((strlen(xf_pn) == 8) && (strncasecmp(xf_pn, pn, 8) == 0))
                {
                    ret = get_xref_info_params(str, fpga, sn, mtype, moffs, mcount, desc, rsvd1, rsvd2);
                }
            }
        }
    }

    fclose(xfp);

    return ret;
}

/**
 * EDT Internal: check & assign the remaining xref args
 */
static int
get_xref_info_params(char *str, char *fpga, char *sn, char *mtype, char *moffs, char *mcount, char *desc, char *rsvd1, char *rsvd2)
{
    int n, ret = 0;
    char xf_partnum[128], xf_fpga[128], xf_sn[128], xf_mtype[128], xf_moffs[128], xf_mcount[128], xf_desc[128], xf_rsvd1[128], xf_rsvd2[128];

    if (fpga) fpga[0] = '\0';
    if (sn) sn[0] = '\0';
    if (mtype) mtype[0] = '\0';
    if (moffs) moffs[0] = '\0';
    if (mcount) mcount[0] = '\0';
    if (desc) desc[0] = '\0';
    if (rsvd1) rsvd1[0] = '\0';
    if (rsvd2) rsvd2[0] = '\0';

    n = sscanf(str, "%s %s %s %s %s %s \"%64[^\"]s\" %s", xf_partnum, xf_fpga, xf_sn, xf_mtype, xf_moffs, xf_mcount, xf_desc, xf_rsvd1, xf_rsvd2);

    /* check if the xf_fpga arg looks like an fpga */
    if ((n > 1) && fpga && (strlen(xf_fpga) > 1) && (strlen(xf_fpga) <= 64))
    {
        strcpy(fpga, xf_fpga);
        ++ret;
    }

    /* check if the arg looks like a serial number (old format didn't have this) */
    if ((n > 2) && sn && (strlen(xf_sn) > 5) && (strlen(xf_sn) < 25))
    {
        int i;

        /* walk backward from the end to make sure there are at least 3 trailing digits */
        strcpy(sn, xf_sn);
        ++ret;
        for (i=strlen(xf_sn)-1; i > strlen(xf_sn) - 4; i--)
        {
            if (!isdigit(xf_sn[i]))
            {
                sn[0] = '\0';
                --ret;
                break;
            }
        }
    }

    /* check if the arg looks like a mac type -- s/b 1-3 digit numeric (old format didn't have this) */
    if ((n > 3) && mtype && (strlen(xf_mtype) > 0) && (strlen(xf_mtype) <= 3) && isdigit_str(xf_mtype))
    {
        strcpy(mtype, xf_mtype);
        ++ret;
    }

    /* check if the arg looks like a mac offset -- s/b 1-3 digit numeric (old format didn't have this) */
    if ((n > 4) && moffs && (strlen(xf_moffs) > 0) && (strlen(xf_moffs) <= 3) && isdigit_str(xf_moffs))
    {
        strcpy(moffs, xf_moffs);
        ++ret;
    }

    /* check if the arg looks like a mac count -- s/b 1-3 digit numeric (old format didn't have this) */
    if ((n > 5) && mcount && (strlen(xf_mcount) > 0) && (strlen(xf_mcount) <= 3) && isdigit_str(xf_mcount))
    {
        strcpy(mcount, xf_mcount);
        ++ret;
    }

    /* check if the arg looks like a description -- s/b 3-128 chars (old format didn't have this) */
    if ((n > 6) && desc && (strlen(xf_desc) > 3) && (strlen(xf_desc) <= 128))
    {
        strcpy(desc, xf_desc);
        ++ret;
    }

    /* N > 7: future */
    if (n > 7)
        strcpy(rsvd1, xf_rsvd1);

    if (n > 8)
        strcpy(rsvd2, xf_rsvd2);

    return ret;
}

static int
isdigit_str(char *s)
{
    u_int i;

    for (i=0; i<strlen(s); i++)
        if ((s[i]) < '0' || s[i] > '9')
            return 0;
    return 1;
}

int
edt_user_dma_wakeup(EdtDev *edt_p)
{
    u_int i = 1;

#ifdef _NT_
    HANDLE event;
    int rc;

    if (!edt_p->event_funcs[EDT_EVENT_BUF].wait_event)
    {
        edt_get_kernel_event(edt_p, EDT_EVENT_BUF);
    }

    event = edt_p->event_funcs[EDT_EVENT_BUF].wait_event;

    rc = SetEvent(event);
    rc = ResetEvent(event);

    edt_p->last_wait_ret = EDT_WAIT_USER_WAKEUP;
    return !rc;

#else

    return edt_ioctl(edt_p,EDTS_USER_DMA_WAKEUP,&i);

#endif

}

int
edt_had_user_dma_wakeup(EdtDev *edt_p)
{
    u_int i = 0;
#ifdef _NT_
    return edt_p->last_wait_ret == EDT_WAIT_USER_WAKEUP;
#endif
    edt_ioctl(edt_p,EDTG_USER_DMA_WAKEUP,&i);

    return i;
}

int
edt_get_wait_status(EdtDev *edt_p)

{
    u_int i = 0;

#ifdef _NT_
    return edt_p->last_wait_ret;
#else
    edt_ioctl(edt_p,EDTG_WAIT_STATUS,&i);
#endif
    return i;

}

int
edt_clear_wait_status(EdtDev *edt_p)

{
    u_int i = 0;

#ifdef _NT_
    edt_p->last_wait_ret = 0;
#else
    edt_ioctl(edt_p,EDTS_WAIT_STATUS,&i);
#endif
    return i;

}

int
edt_set_timeout_ok(EdtDev *edt_p, int val)

{
    return edt_ioctl(edt_p,EDTS_TIMEOUT_OK,&val);
}

int
edt_get_timeout_ok(EdtDev *edt_p)

{
    u_int i = 0;

    edt_ioctl(edt_p,EDTG_TIMEOUT_OK,&i);

    return i;
}

#if 0

DIR *opendir(char *dirName)
{
    printf("opendir") ;
}

STATUS closedir(DIR *pDir)
{
    printf("closedir") ;
}
struct dirent *readdir(DIR *pDir)
{
    printf("readdir") ;
}

#endif

u_int
edt_set_mezz_id(EdtDev *edt_p)

{
    int ret ;
    edt_buf mezz_args ;

    int chan;

#ifdef WIN32
    mezz_args.value = (uint64_t) (&edt_p->mezz);
#else
    mezz_args.value = (uint64_t) ((unsigned long) &edt_p->mezz);
#endif

    for (chan = 0;chan<edt_p->DMA_channels;chan++)
    {

        EdtDev *test_edt;

        if (chan != edt_p->channel_no)
        {
            test_edt = edt_open_channel(EDT_INTERFACE, edt_p->unit_no, chan);

            if (test_edt)
            {
                ret = edt_ioctl(test_edt, EDTS_MEZZ_ID, &mezz_args);

                edt_close(test_edt);
            }
        }
    }

    ret = edt_ioctl(edt_p, EDTS_MEZZ_ID, &mezz_args);

    return ret;
}

u_int
edt_get_mezz_id(EdtDev *edt_p)

{
    int ret ;
    edt_buf mezz_args ;
#ifdef WIN32
    mezz_args.value = (uint64_t) (&edt_p->mezz);
#else
    mezz_args.value = (uint64_t) ((unsigned long) &edt_p->mezz);
#endif

    ret = edt_ioctl(edt_p, EDTG_MEZZ_ID, &mezz_args);

    return ret;
}

/*
* This function returns the normal or extended board id information from a mezannine board.
*/

/*
* The lower 4 bits of the MEZZANIN_BD_ID register always read back the 4 bits of board ID
* from the mezzanine board. When all but one of the board ids where used EDT introduced
* a extended board ID. The last 4 bit board ID (0x2) is used on all subsequent mezzanine
* boards to indicate extended board id has been implemented.
* The extended board ID is implemented in a XC9572XL CPLD using a serial read scheme similar
* but not identical to I2C to read one or more 32 bit registers used for mezzanine id and revision.
* The first word is 12 bits - number of 32 bit words (usually 1)
*                   12 bits - extended board id
*                    8 bits - version number
* use of subsequent words is undefined
*
* The upper 4 bits of the MEZZANINE_BD_ID register controls the extended board id serial pins.
* They are defined as follows:
* bit 4 - Configuration file support for extended board id.
*         This bit is set to a 1 by any User FPGA configuration file that implements
*         the extend board ID functions described here. Previous configuration files
*         for mezzanines without extend board id will always return 0 in bit 4 (actually
*         bit4 to bit 7 as well)
* bit 5 - Board ID serial data.
*         When the FPGA is driving the bi-directional data line (bit 7 is low),
*         then bit 5 is set to the desired state of the data signal. When the FPGA
*         is tristate (bit 7 high) then bit reflects the state of the data signal.
*         The data signal is pulled up on the mezzanines with a 10k resistor. If
*         both the FPGA nad the CPLD are driving the data bus this bit will reflect
*         the value desired by the FPGA not necessarily the actual state of the bus.
* bit 6 - Board ID clock.
*         The serial clock is always driven by the FPGA. When this bit is high the clock is
*         high. When low the clock is low.
* bit 7 - FPGA Serial Data Tristate.
*         See bit 5 description.
*
* The protocol to read the CPLD register is as follows:
* 1) Verify CPLD data is tristate by sending CLK_CPLD clocks.
*    Even if the CPLD is left in the middle of a read this number
*    of clocks should complete any possible read and tristate the data
*    signal.
* 2) Reset the CPLD. With the data enabled and low set the clock high,
*    then set the data high and low to set a reset state. Toggle the
*    clock at least 2 times (we use 5 here) to complete the reset.
* 3) Read the data. With the data high set the clock hi, then set the data
*    low. This sets the start state. Tristate the FPGA data and toggle the clock
*    while reading the serial data from the CPLD on bit 5.
* 4) After the data is read set the clock low, enable the FPGA data with the
*    data low. Set the clock high and then the data high. This will set stop
*    and reset the CPLD.
*/


#define CLK_CPLD 500
/* register masks */
#define EDT_EXTBDID_IMPL        0x10
#define EDT_EXTBDID_DATA        0x20
#define EDT_EXTBDID_CLK         0x40
#define EDT_EXTBDID_TS          0x80

/*
* read a 32 bit word from the cpld.
* the start state has already been set
*/
static u_int
edt_read_extbdid_val(EdtDev *edt_p)

{
    u_int ext_info = 0;
    int idcnt;          /* counter for reading bits from cpld */
    int bdidreg;

    for(idcnt = 31; idcnt >= 0; --idcnt)
    {
        /* toggle clock */
        edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_TS | EDT_EXTBDID_CLK);
        edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_TS);
        /*
        edt_msleep(1);
        */
        bdidreg = edt_intfc_read(edt_p, MEZZANINE_BD_ID);

        if ((bdidreg & EDT_EXTBDID_DATA) != 0)
            ext_info |= 1 << idcnt;
    }

    return ext_info;

    /* clock is low, data is tristate at FPGA */

}

void edt_extbdid_stop(EdtDev *edt_p)
{
    int init;

    /* one more clock to tristate CPLD data */
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_TS | EDT_EXTBDID_CLK);
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_TS);

    /* set stop condition */
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, 0); /* FPGA enabled and low, clock stays low */
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_CLK); /*clock hi */
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_CLK | EDT_EXTBDID_DATA); /* data hi sets stop */
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_DATA); /* clock low */
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_TS | EDT_EXTBDID_DATA); /* tristate data while high */

    /* a few extra clocks to complete reset */

    for(init = 2; init > 0; --init)
    {
        edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_TS | EDT_EXTBDID_CLK);
        edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_TS);
    }

    edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_TS | EDT_EXTBDID_CLK | EDT_EXTBDID_DATA);
    /* CPLD should be idle, clock high and data tristate (pulled up high) just like we should start */

}

/*
* reset the CPLD from a completely unknown state
* and set start state to begin read
*/
void edt_extbdid_reset(EdtDev *edt_p)
{
    int init;
    int bdidreg;
    /* clock cpld to make sure CPLD is tristate */
    /* force a start and then assume a start to get early rev cplds
    * to definitly be tristate. Requires pciss16test with 24 ma drive
    * on the data (to overpower a CPLD sstuck in tristate)
    */

    edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_TS | EDT_EXTBDID_DATA); /* make data hi before enable*/
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_DATA); /*  enable FPGS on data bus */
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_CLK | EDT_EXTBDID_DATA); /* set clock hi */
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_CLK ); /* data low while clock high sets start */
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, 0); /* clock low */
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_DATA); /*  data hi in prep for ts */
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_TS | EDT_EXTBDID_DATA); /* ts data */

    for(init = CLK_CPLD; init > 0; --init)
    {
        /* toggle clock - disregard data */
        edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_TS | EDT_EXTBDID_CLK);
        edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_TS);
    }

    bdidreg = edt_intfc_read(edt_p, MEZZANINE_BD_ID);

    if ((bdidreg & EDT_EXTBDID_DATA) == 0)
        printf("After %d loops BDID databus is still low\n", init);

    /* set both start and stop condition to reset */
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, 0); /* FPGA enabled and low, clock stays low */
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_CLK); /*clock hi */
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_CLK | EDT_EXTBDID_DATA); /* data hi sets stop */
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_CLK ); /* data low sets start (now reset) */
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, 0); /* clock low */
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_DATA); /* drive data high in prep for ts */
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_TS | EDT_EXTBDID_DATA); /* tristate while data hi*/

    /* a few extra clocks to complete reset */

    for(init = 2; init > 0; --init)
    {
        edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_TS | EDT_EXTBDID_CLK);
        edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_TS);
    }
    /* CPLD should be idle, clock low and data tristate (pulled up high) */
    /* force another start */

    edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_TS | EDT_EXTBDID_DATA); /* make data hi before enable*/
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_DATA); /*  enable FPGS on data bus */
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_CLK | EDT_EXTBDID_DATA); /* set clock hi */
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_CLK ); /* data low while clock high sets start */
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, 0); /* clock low */
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_DATA); /*  data hi in prep for ts */
    edt_intfc_write(edt_p, MEZZANINE_BD_ID, EDT_EXTBDID_TS | EDT_EXTBDID_DATA); /* ts data */

}

/**
* @brief Gets the mezzanine id including extended data
*
* @param edt_p
*
* @param extended_n pointer to int to receive the number of extended data elements
*
* @param rev_id pointer to int to fill in with the mezzanine rev_id
*
* @param extended_data pointer to array to fill in with extended data elements
*
* @return mezzanine id
*
* This function works on SS and GS boards to read the mezzanine board ids. If the
* id is an "extended" id, it reads the eeprom on the mezzanine including the extended
* data array.
*
*
* The following values could be returned instead of the mezzanine id,
* if the mezzanine id couldn't be determined:
* <table>
* <tr><td> MEZZ_ERR_NO_BITFILE </td>
*     <td> Indicates that no UI bitfile is loaded, so the mezzanine id
*     couldn't be determined.  </td></tr>
*
* <tr><td> MEZZ_ERR_BAD_BITSTREAM </td>
*     <td> Indicates an error while looking up the extended board info.
*          Before EDT ticket #95 is fixed, this could also result when
*          the ui bitfile is pciss4test and the mezz. board is
*          3X3G.</td></tr>
*
* <tr><td> MEZZ_ERR_NO_REGISTER </td>
*     <td> Indicates that a bitfile has been loaded into the UI which
*     doesn't support the extended board id register.  All EDT UI bitfiles
*     should support this, so contact EDT if this occurs. </td> </tr>
*
* <tr><td> MEZZ_UNKN_EXTBDID </td>
*     <td> Indicates that the board id is extended but the UI bitfile
*     doesn't support this functionality.  This is also unlikely -
*     contact EDT if you see this.  </td></tr>
* </table>
*
* If any of those values are returned, load a bitfile which supports
* the extended board id register, such as pciss1test, pciss4test, or
* pciss16test (depending on channels), or 3x3g.bit for the 3X3G board.
*/
u_int
edt_get_full_board_id(EdtDev *edt_p, int *extended_n, int *rev_id, u_int *extended_data)
{
    u_int mezz_id;              /* board id from mezannine */
    int board_id = 0;           /* board id read back from cpld. initialized to 0 */
    int num_blocks = 0;         /* number of 32 bit blocks to read from cpld. initialized to 0 */
    int ext_info = 0;
    int ext_data_index = 0;
    int tries = 2;
    int store = 1;
    int try_3x3g = 1;

    /* once id is set, go ahead and return */
    if (edt_p->mezz.id == MEZZ_ID_UNKNOWN)
    {

        /* check whether it's set in the driver */

        edt_get_mezz_id(edt_p);

        edt_msleep(1);

        if (edt_p->mezz.id == MEZZ_ID_UNKNOWN)
        {


            /* Read the Board ID register and determine if it is extended */

            mezz_id = edt_intfc_read(edt_p, MEZZANINE_BD_ID);
            if ((mezz_id & 0x1f) == 0x12)
            {
                /* for 3x3g case, force to use "standard" extbdid pins. */
                edt_intfc_write(edt_p, (INTFC_BYTE | 0x84), 0x00);
                printf("edt_get_full_board_id: First time since driver load\n");
                board_id = -1;


                while (tries && board_id == -1)
                {

                    /* resetthe CPLD and start a read */
                    /* data is tristate and clock is low when finished */

                    edt_extbdid_reset(edt_p);

                    /* clock the cpld 32 times to get the first 32 bits of data. Data is valid on falling edge */
                    /* The first 12 bits are the number of 32 bit words, the next 12 are the board id, and the  */
                    /* last 8 is the board revision id. */

                    ext_info = edt_read_extbdid_val(edt_p);

                    /* calculate the number of 32 bit blocks */

                    num_blocks = ext_info >> 20;
                    if(num_blocks >= 1 && num_blocks <= MAX_EXTENDED_WORDS)
                    {
                        board_id = (ext_info >> 8) & 0xfff;

                        if (rev_id)
                            *rev_id = ext_info & 0xff;
                        edt_p->mezz.extended_rev = ext_info & 0xff;
                    }
                    else
                    {

                        tries--;
                        edt_extbdid_stop(edt_p);
                        num_blocks = 0;
                        edt_msg(EDTWARN, "Warning: bitstream error during extended board id lookup?\n");
                        /* for 3x3g case: */
                        if (tries == 0 && try_3x3g == 1){
                            edt_msg(EDTWARN, "Warning: attempting to detect 3x3g.\n");
                            /* force to use "3x3g" extbdid pins. */
                            edt_intfc_write(edt_p, (INTFC_BYTE | 0x84), 0x01);
                            /* retry and flag as having tried 3x3g. */
                            tries = 2;
                            try_3x3g = 0;
                        }
                    }

                }

                /* read all the additional 32 bit blocks back from the cpld  */
                for (ext_data_index = 0; ext_data_index < num_blocks - 1;
                    ext_data_index ++)
                {
                    ext_info = edt_read_extbdid_val(edt_p);

                    if (extended_data)
                        extended_data[ext_data_index] = ext_info;
                    edt_p->mezz.extended_data[ext_data_index] = ext_info;

                }

                edt_extbdid_stop(edt_p);

            }

            /* board id is extended, but UI bitfile does not support this functionality. Need to load  */
            /* the correct bitfile */
            else if(mezz_id == 0x02)
            {
                /* TODO:  error handling */
                board_id = (mezz_id);
                store = 0;
            }

            /* No bitfile loaded. Need to load a bitfile supporting extended
            * board id. */
            else if (mezz_id == 0xcc)
            {
                /* TODO:  error handling */
                board_id = (mezz_id);
                store = 0;
            }

            /* board ID is the lower 4 bits of MEZZANINE_BD_ID register. No extended information is available. */
            else if (mezz_id != 0xff)
            {
                board_id = (mezz_id & 0x0f);
            }
            else
            {
                store = 0;
            }


            if (store)
            {
                edt_p->mezz.id = board_id;
                edt_p->mezz.n_extended_words = num_blocks-1;
                edt_set_mezz_id(edt_p);
            }
        }
    }

    /* fill in arguments */
    /* redundant now that info is store in edt_p */

    if (extended_n)
        *extended_n = edt_p->mezz.n_extended_words;
    if (rev_id)
        *rev_id = edt_p->mezz.extended_rev;
    if (extended_data)
        memcpy(extended_data, edt_p->mezz.extended_data, sizeof(edt_p->mezz.extended_data));

    return edt_p->mezz.id;

}

/**
* @brief Gets the mezzanine id
*
* @param edt_p
*
* @return mezzanine id
*
* This function works on SS and GS boards to read the mezzanine board ids. It actually
* calls edt_get_full_board_id and ignores the extended data and rev_id returned from
* that function.
*
*
*/

u_int
edt_get_board_id(EdtDev *edt_p)
{
    return edt_get_full_board_id(edt_p, NULL, NULL, NULL);
}


int
edt_set_ignore_signals(EdtDev *edt_p, int ignore)
{
    return edt_ioctl(edt_p, EDTS_IGNORE_SIGNALS, &ignore);
}

/**
* Enable or disable tracing a single reg access
*
* If reg_def ends in ffff, trace all register accesses of that
* type.
*
*/

void
edt_set_trace_regs(EdtDev *edt_p, u_int reg_def, u_int state)

{
    /* we pass the trace state in the upper bit of reg_def */
    u_int reg_def_mask = (state) ? reg_def | 0x80000000 : reg_def;

    edt_ioctl(edt_p, EDTS_TRACE_REG, &reg_def_mask);

}

/* Turn on global flag for tracing register accesses.
* Individual registers or register types must still be enabled
* using edt_set_trace_regs */

void
edt_trace_regs_enable(EdtDev *edt_p, u_int state)

{
    u_int reg_def_mask = (state) ? 1 : 0 ;

    edt_ioctl(edt_p, EDTS_TRACE_REG, &reg_def_mask);

}

void
edt_set_intr_mask(EdtDev *edt_p, u_int state)

{
    edt_ioctl(edt_p, EDTS_INTR_MASK, &state);
}

u_int
edt_get_intr_mask(EdtDev *edt_p)

{
    u_int state = 0;
    edt_ioctl(edt_p, EDTG_INTR_MASK, &state);
    return state;
}

void
edt_set_remote_intr(EdtDev *edt_p, u_int onoff)

{
    u_int current;
    current = edt_get_intr_mask(edt_p);

    current = (current & ~EDT_ICFG_RMT_EN_INTR) ;
    current |= (onoff)?EDT_ICFG_RMT_EN_INTR | EDT_ICFG_PCI_EN_INTR:0;

    edt_set_intr_mask(edt_p, current);
}

u_int
edt_get_remote_intr(EdtDev *edt_p)

{
    return (edt_get_intr_mask(edt_p) & EDT_ICFG_RMT_EN_INTR) != 0;
}


/**
* For large buffers on systems with bounce buffers (i.e. solaris x86_64)
* set the size of DMA which will cause an interrupt to call
* the sync function, rather than waiting until the end.
* For example a 40 MB buffer with interval 4 MB will call
* sync 10 times as DMA is happening.
*/

int edt_set_sync_interval(EdtDev *edt_p, u_int interval)

{
    edt_ioctl(edt_p, EDTS_SYNC_INTERVAL, &interval);
    return 0;
}


/**
* @brief Sets specified mezzanine channel enable bits
*
* @param edt_p
*
* @param channel
*
* @return 0 on success, -1 on failure
*
* This function enables DMA channels specified by the bitmask in second argument.
*
*
*/
int
edt_enable_channels(EdtDev *edt_p, u_int mask)
{

    switch (edt_p->devid)
    {
        case PE8LX32_ID:
        case PE8G2V7_ID:
            edt_reg_or(edt_p, SSD16_CHEN32, mask);
            break;

        default:
            edt_reg_or(edt_p, SSD16_CHEN, mask);
            break;
    }

    return 0;
}


/**
* @brief Clears specified mezzanine channel enable bits
*
* @param edt_p
*
* @param channel
*
* @return 0 on success, -1 on failure
*
* This function disables DMA channels specified by the bitmask in second argument.
*
*
*/
int
edt_disable_channels(EdtDev *edt_p, u_int mask)
{

    switch (edt_p->devid)
    {
        case PE8LX32_ID:
        case PE8G2V7_ID:
            edt_reg_and(edt_p, SSD16_CHEN32, ~mask);
            break;

        default:
            edt_reg_and(edt_p, SSD16_CHEN, ~mask);
            break;
    }


    return 0;
}

/**
* @brief Sets a specified mezzanine channel enable bit
*
* @param edt_p
*
* @param channel
*
* @return 0 on success, -1 on failure
*
* This function enables a DMA channel specified by the second argument.
*
*
*/
int
edt_enable_channel(EdtDev *edt_p, u_int channel)
{

    u_int enable_bit = 1 << channel;

    switch (edt_p->devid)
    {
        case PE8LX32_ID:
        case PE8G2V7_ID:
            edt_reg_or(edt_p, SSD16_CHEN32, enable_bit);
            break;

        default:
            edt_reg_or(edt_p, SSD16_CHEN, enable_bit);
            break;
    }

    return 0;
}



/**
* @brief Clears a specified mezzanine channel enable bit
*
* @param edt_p
*
* @param channel
*
* @return 0 on success, -1 on failure
*
* This function disables a DMA channel specified by the second argument.
*
*
*/
int
edt_disable_channel(EdtDev *edt_p, u_int channel)
{

    u_int enable_bit = 1 << channel;

    switch (edt_p->devid)
    {
        case PE8LX32_ID:
        case PE8G2V7_ID:
            edt_reg_and(edt_p, SSD16_CHEN32, ~enable_bit);
            break;

        default:
            edt_reg_and(edt_p, SSD16_CHEN, ~enable_bit);
            break;
    }

    return 0;
}

/**
* A convenience routine to access the EDT BAR1 registers.  Passed the BAR1 byte
* address for a 32-bit word; note that the LS two bits of the address are ignored.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param offset    integer byte offset into EDT BAR1 register memory, addressing
* a 32-bit value.  Note that the LS two bits of the address are ignored.
*
* @return The value of the 32-bit register.
*
* \Example
* u_int reg24 = edt_bar1_read(edt_p, 0x24);
*
* @see edt_bar1_write, edt_reg_read.
*/

u_int
edt_bar1_read(EdtDev * edt_p, u_int offset)
{
    u_int   val;

    val = edt_reg_read(edt_p, EDT_BAR1_REGISTER | (offset & 0x0ffffffc));

    return val;
}

/**
* A convenience routine to access the EDT BAR1 registers.  Passed the BAR1 byte
* address for a 32-bit word; note that the LS two bits of the address are ignored.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param offset    integer byte offset into EDT BAR1 register memory, addressing
* a 32-bit value.  Note that the LS two bits of the address are ignored.
* @param data      32-bit value to set register with.
*
* \Example
* @code
* u_int reg24 = 0xb01d_bee;
* edt_bar1_write(edt_p, 0x24, reg24);
* @endcode
*
* @see edt_bar1_read, edt_reg_write.
*/

void
edt_bar1_write(EdtDev * edt_p, u_int offset, u_int data)
{
    edt_reg_write(edt_p, EDT_BAR1_REGISTER | (offset & 0x0ffffffc), data);
}


/**
* A convenience routine to access the EDT LCR registers.  Passed the Block
* and offset for a 32-bit word.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param block     integer register block selector
* @param offset    integer block offset
*
* @return The value of the 32-bit register.
*
* \Example
* u_int regVal = edt_lcr_read(edt_p, 2, 1);
*
* @see edt_lcr_write, edt_reg_read.
*/

unsigned int
edt_lcr_read(EdtDev *edt_p, unsigned int regBlock, unsigned int regOffset)
{

    unsigned int regAddr = LCR_DDC_REG_SPACE | ((regBlock & 0x7F) << 12) | (regOffset & 0xFFF);

    return edt_bar1_read(edt_p, regAddr);

}



/**
* A convenience routine to access the EDT LCR registers.  Passed the Block
* and offset for a 32-bit word.
*
* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param block     integer register block selector
* @param offset    integer block offset
* @param data      32-bit value to set register with.
*
* \Example
* @code
* u_int regVal = 0xb01d_bee;
* edt_lcr_write(edt_p, 2, 1, regVal);
* @endcode
*
* @see edt_demod_read, edt_reg_write.
*/

void
edt_lcr_write(EdtDev *edt_p, unsigned int regBlock, unsigned int regOffset, unsigned int regVal)
{

    unsigned int regAddr = LCR_DDC_REG_SPACE | ((regBlock & 0x7F) << 12) | (regOffset & 0xFFF);

    edt_bar1_write(edt_p, regAddr, regVal);

}


/**
* Routine to set the "port" number, as distinct from the dma channel
*/

void
edt_set_port(EdtDev *edt_p, int port)

{
    edt_p->port_no = port;
}
/**
* Routine to get the "port" number, as distinct from the dma channel
*/
int
edt_get_port(EdtDev *edt_p)

{
    return edt_p->port_no;
}


/**
* Reads the specified indirect method 2 register and returns its value.

* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param desc  The name of the register to read. Use the names provided
* in the register descriptions in Hardware Addendum for the card you
* are using (e.g. "PCI DV C-Link Hardware Addendum").
*
* @return The value of register.
*
*/

uint_t
edt_ind_2_read(EdtDev * edt_p, uint_t desc, u_int *width)
{
    int     ret;
    edt_buf buf = {0, 0};

    buf.desc = desc;
    if (width)
        buf.flags = *width;

    ret = edt_ioctl(edt_p, EDTG_IND_2_REG, &buf);
    if (ret < 0)
        return ret;

    if (width)
        *width = buf.flags;

    return (u_int) buf.value;
}

/**
* Reads the specified indirect method 2 register and returns its value.

* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
* @param desc  The name of the register to read. Use the names provided
* in the register descriptions in Hardware Addendum for the card you
* are using (e.g. "PCI DV C-Link Hardware Addendum").
*
* @return The value of register.
*
*/

uint_t
edt_ind_2_write(EdtDev * edt_p, uint_t desc, u_int value, u_int *width)
{
    int     ret;
    edt_buf buf = {0, 0};

    buf.desc = desc;
    buf.value = value;

    if (width)
        buf.flags = *width;

    ret = edt_ioctl(edt_p, EDTS_IND_2_REG, &buf);
    if (ret < 0)
        return -1;

    if (width)
        *width = buf.flags;

    return (uint_t) buf.value;

}


/**
 * Get homedir for EDT device based on EDT environment variable for the driver type, if set.
 * Driver type depends on the version of EDT package / SDK in use. Drivertype / environment
 * variable associations follow:
 *
 * "pcd":    $PCDHOME
 * "pdv":    $PDVHOME
 * "p11w":   $P11wHOME
 * "p16d":   $P16dHOME
 * "p53b":   $P53bHOME
 *
 * By default, the installation script installs packages in /opt/EDT<em>dev<\em> (Linux) or C:\EDT\<em>dev</em> (Windows),
 * so for example with a pcd package and default install directory, this subroutine would return "/opt/EDTpcd" or "c:\EDT\pcd".
 *
 * @param edt_p pointer to edt device structure returned by #edt_open or #edt_open_channel
 * @return value of the appropriate $XXXHOME environment variable, or CWD (".") if not set.
 *
 */
#ifdef WIN32
#include <direct.h>
#endif

static char pwd_home[MAXPATH];

const char *edt_home_dir(EdtDev *edt_p)
{

    const char *s = edt_envvar_from_devtype(edt_p->devtype);

    if (s)
        return s;

    s = getcwd(pwd_home, MAXPATH-1);
    if (!s)
        s = ".";
    return s;

}

const char *edt_envvar_from_devstr(const char *devstr)
{
    const char *s;
    if (strcmp(devstr,"pcd") == 0)
        if (s = getenv("PCDHOME"))
            return s;
    if (strcmp(devstr,"pdv") == 0)
        if (s = getenv("PDVHOME"))
            return s;
    if (strcmp(devstr,"p11w") == 0)
        if (s = getenv("P11wHOME"))
            return s;
    if (strcmp(devstr,"p16d") == 0)
        if (s = getenv("P16dHOME"))
            return s;
    if (strcmp(devstr,"p53b") == 0)
        if (s = getenv("P53bHOME"))
            return s;

    return NULL;
}

const char *edt_envvar_from_devtype(const int devtype)
{
    const char *s;
    const char *ret;

    switch (devtype)
    {
        case pcd:
            if (s = getenv("PCDHOME"))
                ret = s;
            else ret = NULL;
            break;
        case pdv:
            if (s = getenv("PDVHOME"))
                ret = s;
            else ret = NULL;
            break;
        case p11w:
            if (s = getenv("P11wHOME"))
                ret = s;
            else ret = NULL;
            break;
        case p16d:
            if (s = getenv("P16dHOME"))
                ret = s;
            else ret = NULL;
            break;
        case p53b:
            if (s = getenv("P53bHOME"))
                ret = s;
            else ret = NULL;
            break;
        default:
            ret = NULL;
    }
    return ret;
}

/**
* When "dmy" or "DMY" is passed as the first argument to edt_open*(),
* edt_p->devid is set to DMY_ID and all attempted interaction with
* EDT hardware is ignored.
*
* This function registers a callback function which is invoked when
* edt_wait_for_buffers() is called to populate the ring buffer with
* data.
*
* Example:  See the rd16_dmy_dma.c sample program.

* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @param callBack callback function invoked when edt_p->devid is DMY_ID
* and edt_wait_for_buffers() is called.
*
* synopsys:  void (*callBack)(struct edt_device *edt_p, u_char *buf)
*
*/
void
edt_set_dmy_wait_for_buffers_callback(EdtDev *edt_p, void (*callBack)(struct edt_device *edt_p, u_char *buf))
{
  edt_p->dmy_wait_for_buffers_callback = callBack;
}

/**
* When "dmy" or "DMY" is passed to edt_open*(), edt_p->devid is set to
* DMY_ID and all attempted interaction with EDT hardware is ignored.
*
* This function registers a callback function which is invoked when
* edt_reg_read() is called to return a simulated register read value.
*
* Example:  See the rd16_dmy_register.c sample program.

* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @param callBack callback function invoked when edt_p->devid is DMY_ID
* and edt_reg_read() is called.
*
* synopsys:  u_int (*callBack)(struct edt_device *edt_p, u_int reg_desc)
*
*/
void
edt_set_dmy_reg_read_callback(EdtDev *edt_p, u_int (*callBack)(struct edt_device *edt_p, u_int reg_desc))
{
  edt_p->dmy_reg_read_callback = callBack;
}


/**
* When "dmy" or "DMY" is passed as the first argument to edt_open*(),
* edt_p->devid is set to DMY_ID and all attempted interaction with EDT
* hardware is ignored.
*
* This function registers a callback function which is invoked when
* edt_reg_write() is called to set a simulated register write value.
*
* Example:  See the wr16_dmy_register.c sample program.

* @param edt_p pointer to edt device structure returned by #edt_open or
* #edt_open_channel
*
* @param callBack callback function invoked when edt_p->devid is DMY_ID
* and edt_reg_write() is called.
*
* synopsys:  u_int (*callBack)(struct edt_device *edt_p, u_int reg_desc)
*
*/
void
edt_set_dmy_reg_write_callback(EdtDev *edt_p, void (*callBack)(struct edt_device *edt_p, u_int reg_desc, u_int reg_value))
{
  edt_p->dmy_reg_write_callback = callBack;
}


