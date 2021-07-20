/*
 * A utility / example program to send multiple images (TIFF format only so far) through
 * an EDT CLS series Camera Link simulator (PCI DV CLS, PCIe8 DV CLS, PCIe8a DV CLS or
 * equivalent), to a Camera Link receiver (framegrabber).  Assumes the board parameters
 * (e.g. width, height, depth, number of taps, clock frequency, vertical and horizontal
 * blanking) has previously been set via clsiminit or equivalent.
 *
 * Note that the order of the files is how they come in from the readdir call; typically
 * this is sorted alphabetically, however the order from edt_readdir will be os-specific
 * so that cannot be guaranteed. This being example code, verifying the order or other
 * modifications is left up to the user.
 * 
 * This example is a simplified version of send_tiffs, and is generally more appropriate
 * than that application to use as example code for simulator applications.  For more
 * complex operations including variable width images, left/right fill, etc. see
 * send_tiffs.c.
 * 
 * See the EDT CLS Simulator users guide for detailed information on this application.
 *
 * Run "simple_clsend --help" for arguments and usage.
 */

#include "edtinc.h"
#include "clsim_lib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tiffio.h>

#define DEFAULT_LISTFILE "imagelist.txt"

typedef enum {
    fmt_raw,
    fmt_ras,
    fmt_gif,
    fmt_jpg,
    fmt_tif,
    fmt_bmp,
    fmt_unk
} ImgFmt ;



/* information about each image stored in imagelist is 
 * stored in this image_info_t struct.
 */
typedef struct {
    char *pathname;
    int width;
    int height;
    short depth;
    int size;
    short bands; /* image dimensions */
    int image_size;
    ImgFmt format;
    unsigned char *data;
} image_info_t;


/*
 * forward declarations
 */
int is_valid_tiff(TIFF *image) ;
void setup_clsim(PdvDev *pdv_p, image_info_t *image);
int fill_buffer(image_info_t *img_details, u_char *buffer, int max_size);
int fill_buffer_tif(image_info_t *img_details, u_char *buffer, int max_size);
int get_tif_info(image_info_t *img_details);
ImgFmt check_extension(char *fname);
int check_image_sizes(image_info_t *list, int nimages);
int comment_or_blank(char *str);
int get_image_info(ImgFmt fmt, image_info_t *image_p);
int load_next_image(PdvDev *pdv_p, int current_image, image_info_t *image_details,
        int total_images, u_char **buffers, int nbuffers, int buffer_size);
int get_images_info_dir(char *dirname, image_info_t **imagelist);
int get_images_info_list(char *listpath, image_info_t **imagelist);
int get_image_info_file(char *filename, image_info_t **imagelist);
char *strip_nl(char *s);
void error_exit(char *errstr);
int Rgb_4tap = 0;

/*
 * one-liners
 */
int frame_valid_low(PdvDev *pdv_p) { return ! (edt_reg_read(pdv_p, PDV_STAT) & 2); }

/*
 * stubs -- fill these or others in to add different image formats. See get_tif_info for example.
 */
int get_ras_info(image_info_t *img_details) { return -1; /* stub */ }
int get_raw_info(image_info_t *img_details) { return -1; /* stub */ }
int get_gif_info(image_info_t *img_details) { return -1; /* stub */ }
int get_bmp_info(image_info_t *img_details) { return -1; /* stub */ }
int get_jpg_info(image_info_t *img_details) { return -1; /* stub */ }
int fill_buffer_ras(image_info_t *img_details, u_char *buffer, int max_size) { return -1; /* stub */ }
int fill_buffer_raw(image_info_t *img_details, u_char *buffer, int max_size) { return -1; /* stub */ }
int fill_buffer_gif(image_info_t *img_details, u_char *buffer, int max_size) { return -1; /* stub */ }
int fill_buffer_bmp(image_info_t *img_details, u_char *buffer, int max_size) { return -1; /* stub */ }
int fill_buffer_jpg(image_info_t *img_details, u_char *buffer, int max_size) { return -1; /* stub */ }



/* print usage text */
void usage(const char *progname, const char *errstr)
{
    if (errstr && *errstr)
        puts(errstr);
    printf("usage: %s [options] dirname\n", progname);
    printf("Last argument (dirname) is required, and is the name of a directory where all images to send are stored\n");
    printf("Options:\n");
    printf("-u <unit>         unit (board) number (default 0)\n");
    printf("-c <channel>      channel number (0 [default] or 1)\n");
    printf("-l LOOPS          loop through image set LOOPS times\n");
    printf("-m                load all images into memory up front (CAUTION: must have\n");
    printf("                   adequate memory for the #images in the list!)\n");
    printf("-rgb4             emulate RGB with 4-taps for RGB (R+G+B+dummy, needed for PCI\n");
    printf("                   boards, and PCIe boards with pre-10/2012 FW)\n");
    printf("-d                images dir -- if specified, cycles through all valid files in this dir\n");
    printf("-i <listfile>     imagelist filename -- if specified, rotates through all images ref'd in the file.\n");
    printf("                   format for imagelist file is one filename (path)  per line\n");
    printf("-t <#images>      number of images to send (must be fewer the # found in the image list\n");
    printf("-h                print this help message\n");

    return;
}


/*
 * main module
 */
int main(int argc, char *argv[])
{
    int unit = 0, channel = 0;	/* default simulator device */
    int loops = 1;		/* number of times to loop through and send images */
    int loop_indefinitely = 0;	/* Loop forever. Set true if loops arg is 0. */
    int num_buffers = 4;	/* # of ring buffers to use */
    int num_images;		/* # of images to xfer, is # of images found in image dir */
    int total_images;		/* # of images to xfer times # of loops */
    int t1, t2;			/* timeout counters */
    char *imagedir = NULL;	/* pathname of dir from which to read and transfer images */
    char *imagelist = NULL;	/* pathname of list from which to read and transfer images */
    char *imagefile = NULL;	/* pathname of single image file if that's all they want */
    int buffer_size;		/* size of each buffer.  width * height. */
    int current_image;
    int current_buffer;
    int exitval = 0;
    int cam_width = 0, cam_height = 0, cam_depth = 0;
    PdvDev *pdv_p;		/* pointer to simulator device structure */
    u_char **buffers;		/* array of buffers used for ring buffer */
    image_info_t *image_details; /* list of images & parameters */

    /* -1 specifies not-set, and will be changed to DEFAULT_* later */
    image_info_t defaults = { 
        "FAKE_FILE",	/* pathname */
        0,		/* width */
        0,		/* height */
        -1,		/* depth */
        -1,		/* size */
        -1,		/* bands */
        0,		/* image_size */
        fmt_unk,	/* format */
        NULL 		/* data */
    };	

    int started = 0;
    int usemem = 0;
    char *progname = argv[0];


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
                    usage(progname, "Error: option 'N' requires an argument\n");
                    exit(1);
                }
                num_buffers = atoi(argv[0]);
                break;

            case 'd': /* directory */
                ++argv;
                --argc;
                if (argc < 1) {
                    usage(progname, "Error: option 'd' requires an argument\n");
                    exit(1);
                }
                imagedir = argv[0];
                break;

            case 'i': /* imagelist */
                ++argv;
                --argc;
                if (argc < 1) {
                    usage(progname, "Error: option 'i' requires an argument\n");
                    exit(1);
                }
                imagelist = argv[0];
                break;

            case 'f': /* file */
                ++argv;
                --argc;
                if (argc < 1) {
                    usage(progname, "Error: option 'f' requires an argument\n");
                    exit(1);
                }
                imagefile = argv[0];
                break;

            case 'l': /* loops */
                ++argv;
                --argc;
                if (argc < 1) {
                    usage(progname, "Error: option 'l' requires an argument\n");
                    exit(1);
                }
                loops = atoi(argv[0]);
                break;

            case 'm': /* load all images into memory up front */
                usemem = 1;
                break;

            case 'r': /* fake RGB with 4-tap (R+G+B+dummy) */
                if (strcmp(argv[0], "-rgb4") == 0)
                    Rgb_4tap = 1;
                break;

            case 'h': /* help message */
                usage(progname, NULL);
                exit(0);
                break;

            case '-':
                if (strcmp(argv[0], "--help") == 0) {
                    usage(progname, NULL);
                    return 0;
                }
                else {
                    printf("ERROR: unknown option \"%s\"", argv[0]);
                    usage(progname, NULL);
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

    /* last arg may be pathname for images dir for historical reasons, so if no -d or -i look or that. If 
     * that's not there use the default imagelist filename ala send_tiffs.
     */
    if (!(imagedir || imagelist || imagefile))
    {
        if (argc)
            imagedir = argv[0];
        else imagelist = DEFAULT_LISTFILE;
    }

    if (imagefile)
        num_images = get_image_info_file(imagefile, &image_details);
    else if (imagedir)
        num_images = get_images_info_dir(imagedir, &image_details);
    else if (imagelist)
        num_images = get_images_info_list(imagelist, &image_details);


    if (num_images < 1)
        error_exit("Error: no valid images found -- check list or directory pathnames");

    if ((pdv_p = pdv_open_channel("pdv", unit, channel)) == NULL)
    {
        char errmsg[256];
        sprintf(errmsg, "Error: Could not open EDT unit %d channel %d", unit, channel);
        pdv_perror(errmsg);
        exit(1);
    }

    /* error if not CLSIM */
    if (!pdv_is_simulator(pdv_p))
        error_exit("Error: specified pdv unit is not a Camera Link Simulator!");

    cam_width = pdv_get_width(pdv_p);
    cam_height = pdv_get_height(pdv_p);
    cam_depth = pdv_get_depth(pdv_p);

    edt_set_wtimeout(pdv_p, 0);
    edt_set_rtimeout(pdv_p, 0);

    pdv_flush_fifo(pdv_p);

    /* emulate 24 bit rgb with 32 bit; last 8 bits are throwaway */
    if ((cam_depth == 24) && Rgb_4tap)
        cam_depth = 32;

    /* create ring buffers */

    if ((buffer_size = cam_height * pdv_bytes_per_line(cam_width, cam_depth)) == 0)
    {
        printf("Error: Simulator not initialized. Use clsiminit or equivalent\n");
        printf("to initialize the simulator before running %s.\n", progname);
        exit(1);
    }

    if (!check_image_sizes(image_details, num_images))
    {
        error_exit("Error: image / configuration size mismatch!");
    }

    /* turn off counter data */
    pdv_cls_set_datacnt(pdv_p, 0);

    /* allow the set of images to be sent multiple times.  */
    if (loops != 0) /* Run thru imagelist 'loops' times */
        total_images = num_images * loops;
    else if (loops == 0) /* Loop forever */
    {
        loop_indefinitely = 1;
        total_images = num_images;
    }

    printf("width: %d height: %d buffer_size: 0x%x bytes\n",
            cam_width, cam_height, buffer_size); 

    edt_configure_ring_buffers(pdv_p, 
            buffer_size, num_buffers, 
            EDT_WRITE, NULL);

    buffers = edt_buffer_addresses(pdv_p);

    /*
     * if specified, read all images into memory first (load_next_image will alloc the memory)
     */
    if (usemem)
    {
        int i;

        for (i=0; i<num_images; i++)
        {
            if (!load_next_image(pdv_p, i, image_details, num_images, NULL, num_buffers, buffer_size))
                error_exit("Error: unsupported image type");

        }
    }

    /*
     * fill all but the last ring buffer, and get the details about
     * the images stored in those buffers.
     */
    for (current_image = 0; current_image < num_buffers - 1 && current_image < total_images; ++current_image)
    {
        load_next_image(pdv_p,current_image, image_details, num_images, buffers, num_buffers, buffer_size);
    }


    /* to keep things simple (or at least less complicated), best to just skip this and count on clsiminit */
    /* having set things up for the same size as the images being read in. Leaving the call (and the */
    /* subroutine) in however as example code for programmers who might want to reset the size to the */
    /* incoming image size, or for any other reason */
    /* setup_clsim(pdv_p, image_details); */

    if (current_image != edt_min(num_buffers - 1, total_images))
    {
        printf("Error: current_image != first buffer!\n");
        exitval = 1;
    }

    edt_start_buffers(pdv_p, current_image);

    started = current_image;

    for (current_buffer = 0; current_buffer < total_images || loop_indefinitely; 
            ++current_buffer, ++current_image) 
    {

        double size = 0;
        int load_detail;

        /* start current_buffer */

        if (started < total_images || loop_indefinitely) 
        {
            edt_start_buffers(pdv_p,1);
            started++;
        }

        if (current_image < total_images || loop_indefinitely) 
        {
            size = load_next_image(pdv_p,current_image,
                    image_details,
                    num_images,
                    buffers,
                    num_buffers,
                    buffer_size);
        }

        load_detail = current_image % num_images;

        t1 = edt_timeouts(pdv_p);

        edt_wait_for_buffers(pdv_p, 1);

        t2 = edt_timeouts(pdv_p);

        if (t1 != t2)
        {
            printf("Apparent timeout\n");
        }

        printf("\rSent %6d %-20s Loaded %6d %-20s", 
                current_buffer, 
                image_details[current_buffer % num_images].pathname, load_detail, 
                image_details[load_detail].pathname);

    }

    edt_free((unsigned char *)image_details);
    pdv_close(pdv_p);
    exit(exitval);
}



/*
 * basic setup configuration registers on simulator. For a more elaborate example of
 * setting up the simulator, see clsiminit.c
 */
void setup_clsim(PdvDev *pdv_p, image_info_t *image)
{
    int depth_bytes = bits2bytes(pdv_get_depth(pdv_p));
    int taps = pdv_p->dd_p->cls.taps;
    int hblank = pdv_cls_get_hgap(pdv_p);
    int vblank = pdv_cls_get_vgap(pdv_p);

    if (depth_bytes == 3)
    {
        taps = 1;
        depth_bytes = 4;
    }

    if (hblank == 0)
        hblank = pdv_p->dd_p->cls.hblank = PDV_CLS_DEFAULT_HGAP;
    if (vblank == 0)
        hblank = pdv_p->dd_p->cls.vblank = PDV_CLS_DEFAULT_VGAP;

    pdv_cls_set_size(pdv_p,
            taps,
            (int)image->depth,
            image->width,
            image->height,
            hblank,
            (image->width / taps) + hblank,
            vblank,
            image->height + vblank);

    return;
}

/*
 * check for known image format extensions
 */
    ImgFmt
check_extension(char *fname)
{
    char *ext;
    size_t len = strlen(fname);

    if (len < 5) /* must be ?.ext */
        return fmt_unk;
    ext = &fname[len-4];

    if (strcasecmp(ext, ".tif") == 0) return fmt_tif;
    if (strcasecmp(ext, ".gif") == 0) return fmt_gif;
    if (strcasecmp(ext, ".raw") == 0) return fmt_raw;
    if (strcasecmp(ext, ".ras") == 0) return fmt_ras;
    if (strcasecmp(ext, ".bmp") == 0) return fmt_bmp;
    if (strcasecmp(ext, ".jpg") == 0) return fmt_jpg;
    return fmt_unk;
}

/*
 * get image info from one file
 *
 * return 1 if file found and meets format specs, otherwise -1 
 */
    int
get_image_info_file(char *fname, image_info_t **imagelist)
{
    image_info_t *image_p;
    char fullpath[256];
    ImgFmt fmt;

    if ((image_p = *imagelist = (image_info_t *)edt_alloc(1 * sizeof(image_info_t))) == NULL)
        return -1;

    if ((fmt = check_extension(fname)) != fmt_unk)
    {
        strcpy(fullpath, fname);
        edt_correct_slashes(fullpath);
        image_p->pathname = malloc(strlen(fullpath) + 1);
        strcpy(image_p->pathname, fullpath);
        image_p->format = fmt;

        if (get_image_info(fmt, image_p) != 0)
            return -1;
        ++image_p;
    }

    return 1;
}
/*
 * get directory, filenames and  image info, using the OS independent edt_
 * directory open / read functions
 *
 * return number of images found, or -1 if error
 */
    int
get_images_info_dir(char *dirpath, image_info_t **imagelist)
{
    image_info_t *image_p;
    int nimages = 0;
    DIRHANDLE dirp;
    char d_name[256];

    if ((dirp = edt_opendir(dirpath)) == NULL)
        return -1;

    /*
     * read the directory entries once to check for valid image filename
     * extensions, then alloc that many entries
     */
    while (edt_readdir(dirp, d_name))
    {
        if (check_extension(d_name) != fmt_unk)
            ++nimages;
    }

    /* we don't have an edt_rewinddir so just close and reopen to go thru them again */
    edt_closedir(dirp);

    if ((image_p = *imagelist = (image_info_t *)edt_alloc(nimages * sizeof(image_info_t))) == NULL)
        return -1;

    if ((dirp = edt_opendir(dirpath)) == NULL)
        return -1;

    while (edt_readdir(dirp, d_name))
    {
        char fullpath[256];
        ImgFmt fmt;

        if ((fmt = check_extension(d_name)) != fmt_unk)
        {
            sprintf(fullpath, "%s/%s", dirpath, d_name);
            edt_correct_slashes(fullpath);
            image_p->pathname = malloc(strlen(fullpath) + 1);
            strcpy(image_p->pathname, fullpath);
            image_p->format = fmt;

            if (get_image_info(fmt, image_p) != 0)
                return -1;
            ++image_p;
        }
    }

    return nimages;
}

/*
 * filenames and image info from imagelist
 *
 * return number of images found, or -1 if error
 */
    int
get_images_info_list(char *listpath, image_info_t **imagelist)
{
    image_info_t *image_p;
    int nimages = 0;
    char f_name[256];
    FILE *fp;



    /*
     * read the list entries once and check for valid image filename
     * extensions and file existence, then alloc that many entries
     */
    if ((fp = fopen(listpath, "r")) == NULL)
    {
        edt_perror(listpath);
        exit(1);
    }


    while (fgets(f_name, 255, fp))
    {
        strip_nl(f_name);
        if ((!comment_or_blank(f_name)) && (check_extension(f_name) != fmt_unk) && (edt_access(f_name, 0) == 0))
            ++nimages;
    }
    if (!nimages)
        return 0;

    /* go back to the beginning */
    fseek(fp, 0, SEEK_SET);

    if ((image_p = *imagelist = (image_info_t *)edt_alloc(nimages * sizeof(image_info_t))) == NULL)
    {
        printf("Error allocating space for %d image entries\n", nimages);
        exit(1);
    }

    while (fgets(f_name, 255, fp))
    {
        ImgFmt fmt;

        strip_nl(f_name);
        if ((!comment_or_blank(f_name)) && ((fmt = check_extension(f_name)) != fmt_unk) && (edt_access(f_name, 0) == 0))
        {
            edt_correct_slashes(f_name);
            image_p->pathname = malloc(strlen(f_name) + 1);
            strcpy(image_p->pathname, f_name);
            image_p->format = fmt;

            if (get_image_info(fmt, image_p) != 0)
                return -1;
            ++image_p;
        }
    }

    return nimages;
}

/*
 * check string for comment or blank: return 1 (true) if all blanks or first
 * nonblank is '#' 
 */
    int
comment_or_blank(char *str)
{
    int i;

    for (i=0; i<(int)strlen(str); i++)
    {
        if (str[i] == '#')
            return 1;

        if ((str[i] != ' ') && (str[i] != '\t') && (str[i] != '\r') && (str[i] != '\n'))
            return 0;
    }
    return 1;
}

    int
get_image_info(ImgFmt fmt, image_info_t *image_p)
{
    /* so far only tiff format is implemented; this example provides for
     * filling in this table with other formats 
     */
    switch(fmt)
    {
        case fmt_tif:
            get_tif_info(image_p);
            break;

        case fmt_gif:
            get_gif_info(image_p);
            break;

        case fmt_ras:
            get_ras_info(image_p);
            break;

        case fmt_raw:
            get_raw_info(image_p);
            break;

        case fmt_bmp:
            get_bmp_info(image_p);
            break;

        case fmt_jpg:
            get_jpg_info(image_p);
            break;

            /* add your format here! */

        default:
            return -1;
    }
    return 0;
}


    int
get_tif_info(image_info_t *img_details)
{
    int stripMax = 0, stripCount = 0;
    unsigned long image_size;	/* in bytes */
    TIFF *image;
    uint32 width = 0, height = 0, depth = 0;

    /* printf("get_tif_info(): processing image: [%s]\n", img_details->pathname); */

    /* turn off tiff warnings */
    TIFFSetWarningHandler(NULL);

    /* Open the TIFF image */
    if((image = TIFFOpen(img_details->pathname, "r")) == NULL)
    {
        printf("Error: Could not open image %s\n", img_details->pathname);
        exit(42);
    }

    /* Check that it is of a type that we support */
    if (!is_valid_tiff(image)) 
    {
        error_exit("Error: invalid image found");
    } 

    TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &img_details->width);
    TIFFGetField(image, TIFFTAG_IMAGELENGTH, &img_details->height);


    TIFFGetField(image, TIFFTAG_BITSPERSAMPLE, &img_details->depth);
    TIFFGetField(image, TIFFTAG_SAMPLESPERPIXEL, &img_details->bands);

    if (img_details->bands == 3)
        image_size = width * height * bits2bytes(depth);

    image_size = width * height * bits2bytes(depth);

    TIFFClose(image);

    return 0;

}



/**
 * Load image # current_image into memory OR the appropriate ring-buffer
 *
 * @param total_images: number of image_info_t's in image_details
 *
 * If current_image is larger than total_images, it wraps around
 * (current_image = current_image % total_images)
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
    int current_buffer = current_image % nbuffers;
    u_char *buf_p;


    current_image = current_image % total_images;

    /* printf("Filling buffer %d with image %d of %d (%s)\n",
       current_buffer, current_image + 1, total_images,
       image_details[current_image].pathname); */

    /*
     * can either read and load buffers directly ...
     */
    if (buffers)
    {
        buf_p = buffers[current_buffer];
    }
    else /* ... or just load them into memory and call this subroutine later to cycle through them */
    {
        if ((buf_p = image_details[current_image].data = (unsigned char *)edt_alloc(buffer_size)) == NULL)
        {
            printf("error allocating space for %s\n", image_details[current_image].pathname);
            exit(1);
        }
    }

    if (image_details[current_image].size)
    {
        memcpy(buffers[current_buffer], image_details[current_image].data, image_details[current_image].size);
        image_size = image_details[current_image].size;
        edt_set_buffer_size(pdv_p, current_buffer, image_size, EDT_WRITE);

    }

    switch(image_details->format)
    {
        case fmt_tif:
            image_size = fill_buffer_tif(&image_details[current_image], buf_p, buffer_size);
            break;
        case fmt_gif:
            image_size = fill_buffer_gif(&image_details[current_image], buf_p, buffer_size);
            break;
        case fmt_bmp:
            image_size = fill_buffer_bmp(&image_details[current_image], buf_p, buffer_size);
            break;
        case fmt_jpg:
            image_size = fill_buffer_jpg(&image_details[current_image], buf_p, buffer_size);
            break;
        case fmt_raw:
            image_size = fill_buffer_raw(&image_details[current_image], buf_p, buffer_size);
            break;
        case fmt_ras:
            image_size = fill_buffer_ras(&image_details[current_image], buf_p, buffer_size);
            break;
        default:
            return 0;

    }

    return image_size; /* success */

}


/*
 * check image sizes to make sure they're all the same
 */
    int
check_image_sizes(image_info_t *list, int nimages)
{
    int i;

    for (i=0;i<nimages;i++)
    {
        if (list[i].width != list[0].width ||
                list[i].height != list[0].height ||
                list[i].depth != list[0].depth)
            return 0;
    }

    return 1;

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
        printf("Error: undefined number of bits per sample\n");
        return 0;

    }
    else if (value > 16)
    {  
        printf("Error: unsupported number of bits per sample: %d\n", value);
        return 0;
    }

    if(TIFFGetField(image, TIFFTAG_SAMPLESPERPIXEL, &value) == 0) 
    {
        printf("Error: undefined number of samples per pixel\n");
        return 0;

    }

    else if ((value != 1)  && (value != 3))
    {
        /* TODO: support 3 sample images (rgb) as long as depth isn't > 8  */
        printf("Error: unsupported number of samples per pixel: %d\n", value);
        return 0;
    }

    if(TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &value) == 0) 
    {
        printf("Error: Image does not define its width\n");
        return 0;
    }

    if(TIFFGetField(image, TIFFTAG_IMAGELENGTH, &value) == 0) 
    {
        printf("Error: Image does not define its height\n");
        return 0;
    }

    return 1;
}


/**
 * Stores the image found in img_details->pathname to the specified buffer.
 *
 * @param img_details Contains pathname from which to get image.
 * @param buffer a pointer to a buffer in which to store the image.
 * @param max_size the maximum size (width*height*samples*depth_bytes) which can
 * 
 * @return the size in bytes of image loaded into buffer
 * be stored in the buffer (size of the buffer in bytes.)
 *
 * Information about the image (width, height) will be
 * added to the img_details, and buffer will be filled in .
 */
int fill_buffer_tif(image_info_t *img_details, u_char *buffer, int max_size)
{

    TIFF *image;

    uint32 width = 0, height = 0, depth = 0, bands = 0;
    unsigned long imageOffset, result;
    int stripMax = 0, stripCount = 0;
    int stripSize = 0;

    unsigned long image_size; /* in bytes */

    /* printf("fill_buffer_tif(): processing image: [%s]\n", img_details->pathname); */

    /* Open the TIFF image */
    if((image = TIFFOpen(img_details->pathname, "r")) == NULL)
    {
        printf("Error: Could not open image %s\n", img_details->pathname);
        exit(42);
    }

    /* Check that it is of a type that we support */
    if (!is_valid_tiff(image)) 
    {
        printf("Error: Invalid image; exiting.\n");
        exit(42);
    } 

    TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(image, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetField(image, TIFFTAG_BITSPERSAMPLE, &depth);
    TIFFGetField(image, TIFFTAG_SAMPLESPERPIXEL, &bands);

    /* Print out info about image for the heck of it.  */
    /* printf("fill_buffer_tif(): Image: %s Width: %d Height: %d Depth: %d Bands %d\n", 
       img_details->pathname, width, height, depth, bands); */


    /* CLS doesn't do RGB per se' so we instead send 4 channels with 1 throwaway */
    if ((bands == 3) && Rgb_4tap)
        image_size = width * height * bits2bytes(depth) * (bands + 1) ;
    else image_size = width * height * bits2bytes(depth) * bands ;

    /* printf("fill_buffer_tif(): Width*Height*depth_bytes*channels=%d (0x%x)\n",  
       image_size, image_size); */

    /* Read in the possibly multiple strips */
    stripMax = TIFFNumberOfStrips (image);
    stripSize = TIFFStripSize(image);


    imageOffset = 0;

    if (image_size > (uint32)max_size) 
    {
        printf("Error: Image size (0x%lx bytes) is larger " 
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
                printf("Error: Read error on input strip number %d\n", 
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
                printf("Error: Read error on input strip number %d\n", 
                        stripCount);
                exit(42);
            } 

            imageOffset += result;
        }
    }

    img_details->width = width;
    img_details->height = height;


    TIFFClose(image);

    return image_size; 
}

/* Strip the newline off a string by replacing any terminating (\r or \n), 
 * with a NULL. */
    char *
strip_nl(char *s)
{
    int len = (int) strlen(s);
    if (len && ((s[len-1] == '\n') || (s[len-1] == '\r')))
        s[len-1] = '\0';
    return s;
}

    void
error_exit(char *errstr)
{
    puts(errstr);
    exit(1);
}
