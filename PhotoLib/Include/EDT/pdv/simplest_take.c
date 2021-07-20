/**
 * @file
 * An example program to show usage of EDT PCI DV library to acquire and
 * a single image from devices connected to EDT high speed digital imaging
 * interface such as the VisionLink, PCIe4/8 DVa C-Link, PCI DV C-Link, etc.
 * 
 * This is the simplest example for acquiring single images, and is
 * provided as a starting point incorporating digital image acquisition
 * in a user application.  It doesn't provide for any arguments to specify
 * board or channel number, number of buffers, number of loops, etc.,
 * optimization such as ring buffering or pipelining or error checking, or any
 * processing (display, save, etc.) -- -- just the basics to do a simple image
 * grab. For an example that includes those operations, see simple_take.c and
 * other simple_*.c programs.  For even more more complex operations, including
 * error detection, diagnostics, changing camera exposure times, and tuning the
 * acquisition in various ways, see the take.c utility. For serial, see
 * serial_cmd.c. For full-blown GUI application and library source code, see
 * the vlviewer application.
 * 
 * (C) 2005-2014 Engineering Design Team, Inc.
 */
#include "edtinc.h"

/*
 * main
 */
main(int argc, char **argv)
{
    int     unit = 0, channel = 0;
    char    edt_devname[128];
    char    errstr[64];
    u_char *image_p;
    PdvDev *pdv_p;
    char osn[32];

    if (argc > 3)
    {
        printf("usage: %s -u [unit]\n", argv[0]);
        exit(1);
    }

    if ((argc > 1) && ((strcmp(argv[1], "-u") == 0)))
    {
        unit = atoi(argv[2]);
    }

    /*
     * open the interface
     * 
     * EDT_INTERFACE is defined in edtdef.h (included via edtinc.h).
     * With one EDT PCI DV board in the chassis it's uddressed as
     * unit 0; the first channel (bottom connector) on multi-channel
     * boards such as the PCI DV C-Link, is channel 0
     *
     */
    if ((pdv_p = pdv_open_channel(EDT_INTERFACE, unit, channel)) == NULL)
    {
        sprintf(errstr, "pdv_open_channel(%s%d_%d)", edt_devname, unit, channel);
        pdv_perror(errstr);
        return (1);
    }

    /*
     * acquire an image
     */
    image_p = pdv_image(pdv_p);

    /* 
     * optional (but recommended) -- check for timeouts (missing data)
     */
    if (pdv_timeouts(pdv_p) > 0)
        printf("got timeout, check camera and cables\n");
    else printf("got one image\n");

    /*
     * add your code to process the image (save, display, etc) here.
     * image_p points to the image data. Data format is determined by the
     * config file in use; use calls such as pdv_get_width, pdv_get_depth
     * to determine image characteristics.
     */

    pdv_close(pdv_p);

    exit(0);
}


