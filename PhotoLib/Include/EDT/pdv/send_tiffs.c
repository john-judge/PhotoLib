/*
 * A utility / example program to send multiple TIFF format images through the
 * EDT PCI DV CLS simulator or equivalent to a Camera Link receiver.
 * This is the original example app that has hooks for esoteric things like 
 * variable sized images, margin fills and formatted image lists. If you don't
 * need any of those, we recommend starting with the much simpler simple_clsend.c
 * application.
 *
 * see README.send_tiffs for more complete information on this application.
 * Run send_tiffs --help for arguments and usage.
 */

#include "edtinc.h"
#include "clsim_lib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tiffio.h>

#define DEFAULT_FILLA 0xff
#define DEFAULT_FILLB 0xff
#define DEFAULT_VGAP 400
#define DEFAULT_HSTART (-1)

#define DEFAULT_LISTFILE "imagelist.txt"

/* use four buffers for max efficiency. */
#define NUMBUFS 4 

/* depth of images to send (bits/pixel) */
/* depth is whatever camera has been configured as.  If user wants to do something
 * silly, like have a bunch of 16 bit images sent as 8 bit images, they can 
 * use another program (like ImageMagic's convert) to convert it beforehand. */
/* int depth = -1; */

EdtMsgHandler logger;
EdtMsgHandler *lp = &logger;
/* Message logger masks */
#define MINVRB	 1    /* print only the minimum */
#define CHECKDMA 2
#define CHECKIMG 4
#define CHECK_FV 8
#define CHECK_EVENT 16
#define CHECK_IMG_READ 32
#define MAXVRB	 0x80 /* print everything */



/* information about each image stored in imagelist is 
 * stored in this image_info_t struct. */
typedef struct {
    char *filename;
    int width;
    int height;
    short depth;
    int size;
    short bands; /* image dimensions */
    int hStart; /* horizontal start postion of image */
    int fillA, fillB; /* left & right fill values, respectively */
    int vgap; /* desired amount of virtical blanking in # of rasters */
    double rate;
    int image_size;
    unsigned char *data;
} image_info_t;


/* event_data_t structure holds all data needed by the 
 * EdtEvent function (setup_clsim_event) that configures
 * the simulator for each image as framevalid goes high. */
typedef struct {
    PdvDev *pdv_p;
    image_info_t *image_details;
    int sending;
    int next_setup;
    int next_load;
} event_data_t; 


int is_valid_tiff(TIFF *image) ;
int Rgb_4tap = 0;


/* set up configuration registers on simulator */
void setup_clsim(PdvDev *pdv_p, image_info_t image);

/* write the image found in img_deatils->filename to the specified buffer.
 * the img_details->width and img_details->height will be filled in. */
int fill_buffer(image_info_t *img_details, u_char *buffer, int max_size);

/* returns true if frame valid is low. */
int frame_valid_low(PdvDev *pdv_p) {
    return ! (edt_reg_read(pdv_p, PDV_STAT) & 2);
}




int fv_seen = 0;

/* an EdtEvent function which will configure the clsim card */
int setup_clsim_event (void *p) 
{ 

    event_data_t *data = (event_data_t *)p;
    static int count = 0;

    static double last_time = 0.0;
    double t = edt_timestamp();
    double dt = t - last_time;
    last_time = t;

    edt_msg_output(lp,CHECK_EVENT, "Event %4d : setting up %4d width = %5d height = %5d dt = %10.9f \n", 
            count,
            data->next_setup, 
            data->image_details[data->next_setup].width,
            data->image_details[data->next_setup].height,
            dt);

    count++;
    if (data->next_setup >= 0)
        setup_clsim(data->pdv_p, 
                data->image_details[data->next_setup]);
    else
        edt_msg_output(lp,CHECK_EVENT, "Not setting up at end of list\n");

    fv_seen = 1;

    return 0;
}



/*
 * parse an image listing.
 * returns an image_info_t containing the info in the specified line,
 *  or the default values (those in the defaults argument) if not specified.
 * The following parameters are accepted. This is case insensitive.
 * filename, hstart, vgap, fillA, fillB
 * Parameters can be in any order as long as the filename is first.
 * Parameter-value pairs are specified as "parameter:value".
 *
 */

image_info_t process(const char *line, image_info_t defaults) {
    /* filename.tif filla:value fillb:value hstart:value vgap:value #comment*/
    image_info_t img_info = defaults;
    char *line_copy = strdup(line);
    char *tok = line_copy;

    tok = strtok(tok, " ");
    if (tok != NULL) {
        size_t str_length;
        char *filename;
        filename = img_info.filename = strdup(tok);
        str_length = strlen(filename);
        if (filename[str_length - 1] == '\n') {
            filename[str_length - 1] = '\0';
        }
        while ((tok = strtok(NULL, " ")) != NULL) {
            if (strncasecmp(tok, "fillA:", 6) == 0) {
                img_info.fillA = (int) strtod(tok+6, NULL);
            }
            else
                if (strncasecmp(tok, "fillB:", 6) == 0) {
                    img_info.fillB = (int) strtod(tok+6, NULL);
                }
                else if (strncasecmp(tok, "hStart:", 7) == 0) {
                    img_info.hStart = (int) strtod(tok+7, NULL);
                }
                else if (strncasecmp(tok, "vgap:", 5) == 0) {
                    img_info.vgap = (int) strtod(tok+5, NULL);
                }
                else if (tok[0] == '#') {
                    break; /* done processing if we see #comment */
                }
        }
    }
    else {
        /* we can't return an image_info_t if we can't find the file.
         * perhaps we could return one that is empty (and null filename),
         * and let caller handle it.  but for now we'll just bail. */
        edt_msg_output(lp,MINVRB, "ERROR: line without filename: \"%s\"\n", line);
        exit(1);
    }
    free(line_copy);
    return img_info;
}

    int
edt_get_tif_info(image_info_t *img_details)

{
    TIFF *image;

    uint32 width = 0, height = 0, depth = 0;
    int stripMax = 0, stripCount = 0;
    unsigned long image_size; /* in bytes */

    edt_msg_output(lp,MAXVRB, "edt_get_tif_info(): processing image: [%s]\n", img_details->filename);

    /* Open the TIFF image */
    if((image = TIFFOpen(img_details->filename, "r")) == NULL)
    {
        edt_msg_output(lp,MINVRB, "ERROR: Could not open image %s\n", img_details->filename);
        exit(42);
    }

    /* Check that it is of a type that we support */
    if (!is_valid_tiff(image)) 
    {
        edt_msg_output(lp,MINVRB, "ERROR: Invalid image; exiting.\n");
        exit(42);
    } 

    TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &img_details->width);
    TIFFGetField(image, TIFFTAG_IMAGELENGTH, &img_details->height);


    TIFFGetField(image, TIFFTAG_BITSPERSAMPLE, &img_details->depth);
    TIFFGetField(image, TIFFTAG_SAMPLESPERPIXEL, &img_details->bands);

    if (img_details->width == 0 || img_details->height == 0 || img_details->depth == 0 || img_details->bands == 0)
    {
        printf("Error: invalid size parameter(s) -- has clsiminit been run?\n");
        exit(1);
    }

    if (img_details->bands == 3)
        image_size = width * height * bits2bytes(depth);

    image_size = width * height * bits2bytes(depth);

    TIFFClose(image);

    return 0;

}


/**
 * Load image # current_image into the appropriate 
 * ring-buffer
 *
 * @param total_images: number of image_info_t's in 
 *  image_details
 *
 * If current_image is larger than total_images,
 *  it wraps around (current_image = current_image % total_images)
 */
    int
load_next_image(PdvDev *pdv_p,
        int current_image, 
        image_info_t *image_details, 
        int total_images,
        u_char **buffers, 
        int nbuffers, 
        int buffer_size)
{
    int image_size;
    double t2;
    int current_buffer = current_image % nbuffers;
    current_image = current_image % total_images;

    edt_msg_output(lp,CHECKIMG, "Filling buffer %d with image %d of %d (%s)\n",
            current_buffer, current_image + 1, total_images,
            image_details[current_image].filename);

    edt_dtime();

    if (image_details[current_image].size)
    {
        memcpy(buffers[current_buffer], image_details[current_image].data, image_details[current_image].size);
        image_size = image_details[current_image].size;
    }
    else
    {
        image_size = fill_buffer(&image_details[current_image],
                buffers[current_buffer], buffer_size);
    }

    t2 = edt_dtime();

    image_details[current_image].rate = ((double) buffer_size)/t2;

    /* resize buffer to correct size according to image. */
    edt_msg_output(lp, CHECKIMG, "load_next_image(): Setting buffer %d to size %d\n",
            current_buffer, image_size);

    edt_set_buffer_size(pdv_p, current_buffer, image_size, EDT_WRITE);

    return image_size; /* success */

}

/**
 * Allocate for and load image # current_image into local memory buffer (not ring buffer)
 *
 * @param total_images: number of image_info_t's in  image_details
 */
    int
load_next_imagefile(PdvDev *pdv_p,
        int current_image, 
        image_info_t *image_details, 
        int total_images,
        int buffer_size)
{
    edt_msg_output(lp,CHECKIMG, "Loading image #%d of %d (%s)\n",
            current_image + 1, total_images,
            image_details[current_image].filename);

    
    if ((image_details[current_image].data = (unsigned char *)edt_alloc(buffer_size)) == NULL)
    {
        printf("Error: could not allocate buffer (possible memory overflow)\n");
        exit(1);
    }

    image_details[current_image].size = fill_buffer(&image_details[current_image],
            image_details[current_image].data, buffer_size);

    return image_details[current_image].size; /* success */
}

/*
 * check string for comment or blank: return 1 (true) if all blanks or first
 * nonblank is '#' 
 */
    int
comment_or_blank(char *str)
{
    int i;

    for (i=0; i<strlen(str); i++)
    {
        if (str[i] == '#')
            return 1;

        if ((str[i] != ' ') && (str[i] != '\t') && (str[i] != '\r') && (str[i] != '\n'))
            return 0;
    }
    return 1;
}


/* Gets list of images and their parameters from file with specified name.
 *
 * @param details  An array describing each image in the imagelist.
 * This function allocates the array.  Caller is responsible for
 * free()ing it. 
 *
 * @param filename the name of the image list file.
 *
 * @param defaults  The default values to use for the first image in
 * the image list (if it doesn't already specify all possible values). 
 *
 * Returns The number of elements in the allocated array.  
 */
int get_list(image_info_t **details, char *filename, image_info_t defaults)
{

    char line[80];
    int capacity = 100; /* starting capacity */
    int size = 0; /* size actually used */
    FILE *list_file;
    image_info_t *img_details;

    img_details = calloc(capacity, sizeof(image_info_t));
    if (img_details == NULL) {
        edt_msg_output(lp, EDTAPP_MSG_FATAL, "ERROR: calloc() failed in get_list()\n");
        exit(1);
    }

    if (strcmp(filename, "-") == 0) { 
        list_file = stdin;
    }
    else {
        list_file = fopen(filename, "r");
        if (list_file == NULL) {
            edt_msg_output_printf_perror(lp, EDTAPP_MSG_FATAL, "Error opening file '%s'", filename);
            return 0;
        }
    }


    while (fgets(line, sizeof(line), list_file) != NULL) {
        if (size == capacity) {
            capacity *= 2;
            img_details = realloc(img_details, 
                    capacity * sizeof(image_info_t));
            if (img_details == NULL) {
                edt_msg_output(lp,MINVRB, "ERROR: realloc() failed in get_list()\n");
                exit(1);
            } 
        }
        if (!comment_or_blank(line)) {
            defaults = process(line, defaults);
            /* Note that if fillA or fillB is negative, the left
             * or right pixel value from the first line of the image
             * will be used (which is done in the function that opens the image) */
            if (defaults.hStart < 0) 
                defaults.hStart = DEFAULT_HSTART;
            if (defaults.vgap < 0)
                defaults.vgap = DEFAULT_VGAP;
            defaults.data = NULL;
            defaults.size = 0;

            img_details[size] = defaults;

            edt_get_tif_info(img_details + size);

            ++size;
        }
    }

    if (list_file != stdin) {
        fclose(list_file);
    }

    *details = img_details;
    return size;
}

    int
check_list_same_size(image_info_t *list, int nimages)

{
    int i;
    int w,h,d;

    if (nimages == 0)
        return 0;

    w = list->width;
    h = list->height;
    d = list->depth;

    for (i=0;i<nimages;i++)
    {
        if (list[i].width != w ||
                list[i].height != h ||
                list[i].depth != d)
            return 0;
    }

    return 1;

}



/* print usage text */
void usage(const char *progname, const char *errstr)
{
    if (errstr && *errstr)
        puts(errstr);
    printf("usage: %s [options]\n", progname);
    printf("Options:\n");
    printf("-u <unit>            unit (board) number (default 0)\n");
    printf("-c <channel>         channel number (0 [default] or 1)\n");
    printf("-i <listfile>        input file with list of images and parameters.\n");
    printf("                      (Default file is '%s'). Use '-i -' to specify stdin.\n",
            DEFAULT_LISTFILE);
    printf("-l LOOPS             loop through image set LOOPS times\n");
    printf("-rgb4                emulate RGB with 4-taps for RGB (R+G+B+dummy, needed for PCI\n");
    printf("                      boards, and PCIe boards with pre-10/2012 FW)\n");
    printf("-m                   load all images into memory up front (CAUTION: must have\n");
    printf("                     adequate memory for the #images in the list!)\n");
    printf("-t <#images>         number of images to send (should be the same or\n");
    printf("                      less than the number found in image list)\n");
    /* TODO: User s/b able to override depth, so images not matching won't be sent (or included
     * in internal image list)
     * puts("-d <depth>           bits per pixel (default: depth of first image)"); */
    printf(
            "-A <fillA, hex>      left fill value if image width < cam width (default 0x%x)\n",
            DEFAULT_FILLA);
    printf(
            "-B <fillB, hex>      right fill value if image width < cam width (default 0x%x)\n",
            DEFAULT_FILLB);            
    printf("-h                   print this help message\n");
    printf("-N <#buffers>        number of ring buffers to use (default %d)\n",
            NUMBUFS);
    printf("-v <mask>            set verbosity to debug output mask ... \n\
                      Verbosity can be mask of: \n\
                      MINVRB         1 (print only the minimum) \n\
                      CHECKDMA       2 \n\
                      CHECKIMG       4 \n\
                      CHECK_FV       8 \n\
                      CHECK_EVENT    16 \n\
                      CHECK_IMG_READ 32 \n\
                      MAXVRB         0x80 (print everything)\n");

    return;
}




/* open a number of TIFF images and send them though the simulator.
 * The number of images sent will be the number found in the file
 * specified by the -i option, unless the user sets the number of images 
 * to less than that using the -t option. */

int main(int argc, char *argv[])
{
    int unit = 0, channel = 0; /* default simulator device */

    char *list_file = DEFAULT_LISTFILE; /* file with list of images to send */

    u_int loops = 1;  /* of times to loop through and send image list */
    int loop_indefinitely = 0; /* Set true if loops arg is 0.  Causes program to loop through image list indefinitely. */
    u_int num_buffers = NUMBUFS; /* # of ring buffers to use */

    int num_images = -1; /* number of images to xfer, is # of images found in list_file, unless user sets less than that with -t. */
    int actual_images; /* number of actual images in list */

    PdvDev *pdv_p; /* pointer to simulator device structure */

    u_char **buffers; /* array of buffers used for ring buffer */
    image_info_t *image_details; /* list of images & parameters */
    u_int images_configured; /* # of images specified in image_list */

    int buffer_size; /* size of each buffer.  width * height. */
    u_int current_image;
    u_int current_buffer;

    event_data_t cbinfo; /* call back info for the setup clsim event function */

    int cam_width = 0, cam_height = 0, cam_depth = 0;

    int verbosity = MINVRB;

    int variable_size = FALSE;
    /* -1 specifies not-set, and will be changed to DEFAULT_* later */
    image_info_t defaults = { 
        "FAKE_FILE",	/* filename */
        0,		/* width */
        0,		/* height */
        -1,		/* depth */
        -1,		/* size */
        -1,		/* bands */
        -1,		/* hstart */
        -1, -1,		/* fillA, fillB */
        0,		/* vgap */
        20.0,		/* rate */
        0,		/* image_size */
        NULL 		/* data */
    };	

    double wait_time, total_time;
    int started = 0;
    int usemem = 0;
    char *progname = argv[0];

    /* ************************************************************ */

    /* set up output printing with message levels */
    /* level is set after checking args */

    edt_msg_init(&logger);

    /* parse user arguments. */
    --argc;
    ++argv;
    while (argc && argv[0][0] == '-')
    {
        switch(argv[0][1])
        {
            case 'u':		/* device unit number */
                ++argv;
                --argc;
                if (argc < 1) 
                {
                    usage(progname, "Error: option 'u' requires an argument\n");
                    exit(1);
                }
                if  ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
                    unit = atoi(argv[0]);
                break;

            case 'c':		/* device channel number */
                ++argv;
                --argc;
                if (argc < 1) 
                {
                    usage(progname, "Error: option 'c' requires an argument\n");
                    exit(1);
                }
                if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
                {
                    channel = atoi(argv[0]);
                }
                else 
                {
                    usage(progname, "Error: option 'c' requires an argument\n");
                    exit(1);
                }
                break;

            case 'N': /* number of buffers to use */
                ++argv;
                --argc;
                if (argc < 1) {
                    usage(progname, "ERROR: option 'N' requires argument\n");
                    exit(1);
                }
                num_buffers = atoi(argv[0]);
                break;

            case 'i': /* input file to read list of images from */
                ++argv;
                --argc;
                if (argc < 1) {
                    usage(progname, "ERROR: option 'i' requires argument\n");
                    exit(1);
                }
                list_file = argv[0];
                break;

            case 'l': /* loops */
                ++argv;
                --argc;
                if (argc < 1) {
                    usage(progname, "ERROR: option 'l' requires argument\n");
                    exit(1);
                }
                loops = atoi(argv[0]);
                break;

            case 't': /* transfer only this many images */
                ++argv;
                --argc;
                if (argc < 1) {
                    usage(progname, "ERROR: option 't' requires argument\n");
                    exit(1);
                }
                num_images = atoi(argv[0]);
                break;

                /*
                   case 'd': // image depth 
                   ++argv;
                   --argc;
                   if (argc < 1) {
                   usage(progname, "ERROR: option 'd' requires argument\n");
                   exit(1);
                   }
                   depth = atoi(argv[0]);
                   break;
                 */

            case 'r': /* fake RGB with 4-tap (R+G+B+dummy) */
                if (strcmp(argv[0], "-rgb4") == 0)
                    Rgb_4tap = 1;
                break;

            case 'm': /* load all images into memory up front */
                usemem = 1;
                break;

            case 'h': /* help message */
                usage(progname, NULL);
                exit(0);
                break;

            case 'v': /* verbosity */
                ++argv;
                --argc;
                if (argc < 1) {
                    usage(progname, "ERROR: option 'v' requires argument\n");
                    exit(1);
                }
                verbosity |= strtol(argv[0],0,0);
                break;

            case 'A': /* fillA (left fill value) */
                ++argv;
                --argc;
                if (argc < 1) {
                    usage(progname, "ERROR: option 'A' requires argument\n");
                    exit(1);
                }
                defaults.fillA = strtol(argv[0],0,0);
                break;

            case 'B': /* fillB (right fill value) */
                ++argv;
                --argc;
                if (argc < 1) {
                    usage(progname, "ERROR: option 'B' requires argument\n");
                    exit(1);
                }
                defaults.fillB = strtol(argv[0],0,0);
                break;

            case '-':
                if (strcmp(argv[0], "--help") == 0) {
                    usage(progname, NULL);
                    return 0;
                }
                else {
                    usage(progname, "ERROR: unknown option\n");
                    exit(1);
                }
                break;


            default:
                usage(progname, NULL);
                exit(1);
        }
        argv++;
        argc--;
    }

    /* turn off tiff warnings */
    TIFFSetWarningHandler(NULL);

    edt_msg_set_level(&logger, verbosity);

    pdv_p = pdv_open_channel("pdv", unit, channel);
    if (pdv_p == NULL) {
        pdv_perror("ERROR: Could not open EDT device");
        exit(1);
    }

    edt_set_wtimeout(pdv_p, 0);
    edt_set_rtimeout(pdv_p, 0);

    /* error if not CLSIM */
    if (!pdv_is_simulator(pdv_p))
    {
        edt_msg_output(lp,MINVRB, "ERROR: unit %d is not a Camera Link Simulator!\n",
                unit);
        exit(1);
    }

    edt_flush_fifo(pdv_p);

    cam_width = pdv_get_width(pdv_p);
    cam_height = pdv_get_height(pdv_p);


    cam_depth = pdv_get_depth(pdv_p);

    /* emulate 24 bit rgb with 32 bit; last 8 bits are throwaway */
    if ((cam_depth == 24) && Rgb_4tap)
        cam_depth = 32;

    /* create ring buffers */
    buffer_size = cam_width * pdv_bytes_per_line(cam_height, cam_depth);

    if (buffer_size == 0)
    {
        edt_msg_output(lp,MINVRB, "ERROR: Simulator not initialized\n");
        exit(1);
    }

    defaults.vgap = pdv_cls_get_vgap(pdv_p);

    edt_msg_output(lp,MAXVRB,"Vgap = %d\n", defaults.vgap);

    images_configured = get_list(&image_details, list_file, defaults);

    variable_size = ! check_list_same_size(image_details,images_configured);

    edt_msg_output(lp,MINVRB, "%s images configured: %d\n", (variable_size)?"variable size":"constant size",
            images_configured);

    /* turn off data counters */

    pdv_cls_set_datacnt(pdv_p, 0);

    if (num_images < 0 || num_images > (int)images_configured) {
        num_images = images_configured;
    }
    if (num_images < 1) {
        edt_msg_output(lp,MINVRB, "ERROR: no images specified for sending.\n");
        exit(1);
    }

    actual_images = num_images;

    /* allow the set of images to be sent multiple times. any code dealing 
     * with cur_image or num_images should also use images_configured as 
     * the actual maximum number of different images (and size of image list)
     */
    if (loops != 0) /* Run thru imagelist 'loops' times */
        num_images *= loops;
    else if (loops == 0) /* Loop forever */
        loop_indefinitely = 1;

    edt_msg_output(lp,MAXVRB, 
            "width: %d height: %d buffer_size: 0x%x bytes\n",
            cam_width, cam_height, buffer_size); 

    edt_configure_ring_buffers(pdv_p, 
            buffer_size, (int)num_buffers, 
            EDT_WRITE, NULL);

    buffers = edt_buffer_addresses(pdv_p);

    /*
     * if specified, read all images into memory first
     */
    if (usemem)
    {
        int i;

        for (i=0; i<actual_images; i++)
        {
            load_next_imagefile(pdv_p, i,
                    image_details,
                    images_configured,
                    buffer_size);
        }
    }

    buffers = edt_buffer_addresses(pdv_p);


    /*
     * fill all but the last ring buffer, and get the details about
     * the images stored in those buffers.
     */
    for (current_image = 0; current_image < num_buffers - 1 &&
            (int) current_image < num_images; ++current_image) {

        load_next_image(pdv_p,current_image,
                image_details,
                images_configured,
                buffers,
                (int)num_buffers,
                buffer_size);
    }


    cbinfo.pdv_p = pdv_p;
    cbinfo.image_details = image_details;

    /* config clsim for first image */
    setup_clsim(pdv_p, image_details[0]); 

    if (variable_size)
        edt_set_event_func(pdv_p, EDT_PDV_EVENT_FVAL, (EdtEventFunc) setup_clsim_event, &cbinfo, 1);


    ((int)current_image == edt_min((int)num_buffers - 1, (int)num_images));

    edt_start_buffers(pdv_p, current_image);

    started = current_image;

    for (current_buffer = 0; (int)current_buffer < (int)num_images || loop_indefinitely; 
            ++current_buffer, ++current_image) 
    {

        double size = 0;
        int t1, t2;
        int load_detail;
        if (verbosity != MINVRB)
            edt_msg_output(lp, 0xfe, "----------------------------------------------\n");

        total_time = edt_timestamp();

        cbinfo.sending = current_buffer;
        if ((int)current_buffer + 1 >= (int)num_images && !loop_indefinitely)
            cbinfo.next_setup = -1;
        else
            cbinfo.next_setup = current_buffer + 1;
        cbinfo.next_load = current_image;

        /* start sending out current buffer */
        edt_ref_tmstamp(pdv_p, 0xabcd0001);

        /* reset so that event will set it */
        fv_seen = 0;

        edt_msg_output(lp, CHECKDMA, "Sending %d\n", current_buffer);

        /* start current_buffer */

        if (started < (int)num_images || loop_indefinitely) 
        {
            edt_start_buffers(pdv_p,1);
            started++;
        }

        if ((int)current_image < (int)num_images || loop_indefinitely) 
        {
            size = load_next_image(pdv_p,current_image,
                    image_details,
                    images_configured,
                    buffers,
                    num_buffers,
                    buffer_size);
        }

        load_detail = current_image % images_configured;

        t1 = edt_timeouts(pdv_p);

        edt_wait_for_buffers(pdv_p, 1);

        t2 = edt_timeouts(pdv_p);

        if (t1 != t2)
        {
            edt_msg_output(lp, MINVRB, "Apparent timeout\n");
        }

        wait_time = edt_dtime();

        total_time = edt_timestamp() - total_time;

        edt_msg_output(lp, MINVRB,     "\rSent %6d %-20s Loaded %6d %-20s Rate %10f", 
                current_buffer, 
                image_details[current_buffer % images_configured].filename, load_detail, 
                image_details[load_detail].filename, image_details[load_detail].rate/1000000.0);

    }


    free(image_details);
    pdv_close(pdv_p);
    return 0;
}



/* set up configuration registers on simulator */
void setup_clsim(PdvDev *pdv_p, image_info_t image)
{
    /* The following will be set for each image:
     * FillA: left fill
     * FillB: right fill
     * vgap: # rasterlines of Vertical blanking (VcntMax - Vactv)
     * hStart: horizontal start position of image within raster
     *  note that if hStart is -1, image will be centered between even
     *  margins of FillA and FillB values.
     * height of image
     */
    int rvStart, rvEnd;
    int max_width;

    int clocks;

    int taps = (edt_reg_read(pdv_p, PDV_CL_DATA_PATH) >> 4) + 1;
    int num_bytes = ((edt_reg_read(pdv_p, PDV_CL_DATA_PATH) & 15)+7)/8;

    int hlvstart = edt_reg_read(pdv_p, PDV_CLSIM_HLVSTART);
    int hlvend = edt_reg_read(pdv_p, PDV_CLSIM_HLVEND);

    int depth_bytes = bits2bytes(pdv_get_depth(pdv_p));

    /* emulate 3 tap RGB with 4 tap; last byte is throwaway */
    if (taps == 3)
        taps = 4;

    /* 
     * 1 tap = 1 byte / clock. 
     * width is in pixels
     * depth_bytes is in bytes / pixel
     *
     * So, (width * depth_bytes) / taps  
     *
     *   = (width * depth_bytes) * 1/taps 
     *
     *               bytes     clocks 
     *   =  pixels * -----  *  ------  = clocks
     *               pixel     byte 
     */

    if (depth_bytes == 3)
        depth_bytes = 4;

    clocks = (image.width * depth_bytes) / (taps * num_bytes);

    max_width = hlvend - hlvstart;

    edt_msg_output(lp,CHECKIMG, "setup_clsim(): file: %s ", image.filename);
    edt_msg_output(lp,CHECKIMG, "height: %d width: %d ", image.height, image.width);
    edt_msg_output(lp,CHECKIMG, "vgap: %d ", image.vgap);
    edt_msg_output(lp,CHECKIMG, "filla: %d fillb: %d ", image.fillA, image.fillB);
    edt_msg_output(lp,CHECKIMG, "hStart: %d\n", image.hStart);

    edt_msg_output(lp,CHECKIMG, "setup_clsim(): max_width: %d\n", max_width);

    if (image.hStart == -1) 
    {
        /* Adjust margins so that image is centered between fill values. */
        rvStart = (max_width / 2) - (clocks / 2) + hlvstart;
        rvEnd = rvStart + clocks;
    }
    else 
    {
        /* use supplied settings */
        rvStart = image.hStart;
        rvEnd = rvStart + clocks;
    }

    edt_msg_output(lp,CHECKIMG, "setup_clsim(): rvStart: %d rvEnd: %d\n",
            rvStart, rvEnd);

    pdv_cls_set_fill(pdv_p, image.fillA, image.fillB);
    pdv_cls_set_height(pdv_p, image.height, image.vgap);
    pdv_cls_set_rven(pdv_p, 1);
    pdv_cls_set_readvalid(pdv_p, rvStart, rvEnd);

    return;
}



/* Validate the TIFF image to ensure it meets requirements.
 * Currently image must be <= 16 bits/pixel and 1 sample/pixel.
 * Image must also define width and height.
 * Returns:
 *  True if valid image, false otherwise. */
int is_valid_tiff(TIFF *image) 

{
    int value = 0;

    if(TIFFGetField(image, TIFFTAG_BITSPERSAMPLE, &value) == 0) 
    {
        edt_msg_output(lp,MINVRB, "ERROR: undefined number of bits per sample\n");
        return 0;

    }
    else if (value > 16)
    {  
        edt_msg_output(lp,MINVRB, "ERROR: unsupported number of bits per sample: %d\n", value);
        return 0;
    }

    if(TIFFGetField(image, TIFFTAG_SAMPLESPERPIXEL, &value) == 0) 
    {
        edt_msg_output(lp,MINVRB, "ERROR: undefined number of samples per pixel\n");
        return 0;

    }

    else if ((value != 1)  && (value != 3))
    {
        /* TODO: support 3 sample images (rgb) as long as depth isn't > 8  */
        edt_msg_output(lp,MINVRB, "ERROR: unsupported number of samples per pixel: %d\n", value);
        return 0;
    }

    if(TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &value) == 0) 
    {
        edt_msg_output(lp,MINVRB, "ERROR: Image does not define its width\n");
        return 0;
    }

    if(TIFFGetField(image, TIFFTAG_IMAGELENGTH, &value) == 0) 
    {
        edt_msg_output(lp,MINVRB, "ERROR: Image does not define its height\n");
        return 0;
    }

    return 1;
}


/**
 * Stores the image found in img_details->filename to the specified buffer.
 * @param img_details Contains filename from which to get image.
 * @return the size in bytes of image loaded into buffer
 * @param buffer a pointer to a buffer in which to store the image.
 * @param max_size the maximum size (width*height*samples*depth_bytes) which can
 * be stored in the buffer.  (The size of the buffer in bytes.)
 *
 * Information about the image (width, height) will be
 * added to the img_details, and buffer will be filled in 
 *
 * If the fill values fillA and fillB for the image are set to -1, 
 * fill_buffer() replaces them with the left and right value of the first image line.
 */
int fill_buffer(image_info_t *img_details, u_char *buffer, int max_size)
{

    TIFF *image;

    uint32 width = 0, height = 0, depth = 0, bands = 0;
    unsigned long imageOffset, result;
    int stripMax = 0, stripCount = 0;
    int stripSize = 0;

    unsigned long image_size; /* in bytes */

    edt_msg_output(lp,MAXVRB, "fill_buffer(): processing image: [%s]\n", img_details->filename);

    /* Open the TIFF image */
    if((image = TIFFOpen(img_details->filename, "r")) == NULL)
    {
        edt_msg_output(lp,MINVRB, "ERROR: Could not open image %s\n", img_details->filename);
        exit(42);
    }

    /* Check that it is of a type that we support */
    if (!is_valid_tiff(image)) 
    {
        edt_msg_output(lp,MINVRB, "ERROR: Invalid image; exiting.\n");
        exit(42);
    } 

    TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(image, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetField(image, TIFFTAG_BITSPERSAMPLE, &depth);
    TIFFGetField(image, TIFFTAG_SAMPLESPERPIXEL, &bands);

    /* is_valid_tiff checks for width, height, depth, otherwise we'd do
     * better error checking here. */
    if (width == 0 || height == 0 || depth == 0 || bands == 0)
    {
        printf("Error: invalid size parameter(s) -- has clsiminit been run?\n");
        exit(1);
    }

    /* Print out info about image for the heck of it.  */
    edt_msg_output(lp,MAXVRB, "fill_buffer(): Image: %s Width: %d Height: %d Depth: %d Bands %d\n", 
            img_details->filename, width, height, depth, bands);


    /* CLS doesn't do RGB per se' so we instead send 4 channels with 1 throwaway */
    if ((bands == 3) && Rgb_4tap)
        image_size = width * height * bits2bytes(depth) * (bands + 1) ;
    else image_size = width * height * bits2bytes(depth) * bands ;

    edt_msg_output(lp,MAXVRB, "fill_buffer(): Width*Height*depth_bytes*channels=%d (0x%x)\n", 
            image_size, image_size);

    /* Read in the possibly multiple strips */
    stripMax = TIFFNumberOfStrips (image);
    stripSize = TIFFStripSize(image);

    imageOffset = 0;

    if (image_size > (uint32)max_size) 
    {
        edt_msg_output(lp,MINVRB, "ERROR: Image size (0x%x bytes) is larger " 
                "than buffer size (0x%x bytes)\n",
                image_size, max_size); 
        exit(42);
    }

    /*
     * special case RGB, read one line at a time then insert 0 every 4th byte
     * since CLS doesn't really do RGB but instead 4 bytes with 1 throwaway
     */
    if ((bands == 3) && Rgb_4tap)
    {
        unsigned long i;
        u_char *bp = buffer;
        u_char *stripbuf = (u_char *)malloc(stripSize);

        for (stripCount = 0; stripCount < stripMax; stripCount++)
        {
            if((result = TIFFReadEncodedStrip (image, stripCount,
                            stripbuf, (tsize_t) -1)) == -1) 
            {
                edt_msg_output(lp,MINVRB, "ERROR: Read error on input strip number %d\n", 
                        stripCount);
                exit(42);
            } 

            for (i=0; i<result; i+=3)
            {
                *bp++ = stripbuf[i];
                *bp++ = stripbuf[i+1];
                *bp++ = stripbuf[i+2];
                *bp++ = 0;
            }
        }
        free(stripbuf);
    }
    else
    {
        for (stripCount = 0; stripCount < stripMax; stripCount++)
        {
            if((result = TIFFReadEncodedStrip (image, stripCount,
                            buffer + imageOffset, (tsize_t) -1)) == -1) 
            {
                edt_msg_output(lp,MINVRB, "ERROR: Read error on input strip number %d\n", 
                        stripCount);
                exit(42);
            } 

            imageOffset += result;
        }
    }


    img_details->width = width;
    img_details->height = height;

    if (img_details->fillA == -1) 
        img_details->fillA = buffer[0];

    if (img_details->fillB == -1) 
        img_details->fillB = buffer[width-1];

    TIFFClose(image);

    return image_size; 
}

