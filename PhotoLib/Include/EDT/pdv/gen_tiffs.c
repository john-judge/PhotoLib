
#include "edtinc.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <tiffio.h>



/* default x,y coords of serial number displayed on image */
#define SERIAL_X 25 
#define SERIAL_Y 50


/* default grid width, height, color */
#define GRID_WIDTH 50 
#define GRID_HEIGHT 100 
#define GRID_LINE_COLOR 0 

#define ABOUT_TEXT "generate a TIF or RAW sample image with counter data"

static void set_pixel_color(u_char *bufptr, int c, int depth);

    static void 
usage(char *progname) 
{
    printf("usage: %s [options] -w width -l height\n", progname);
    puts(ABOUT_TEXT);
    printf("--help           this help message\n");
    printf("-n NUM       print serial number NUM on image (default: diabled)\n");
    printf("-x X         X coordinate of serial # (default: %d)\n", SERIAL_X);
    printf("-y Y         Y coordinate of serial # (default: %d)\n", SERIAL_Y);
    printf("-c VAL       set grid line color-value to VAL (default: %d)\n", 
            GRID_LINE_COLOR);
    printf("-b basename  base name of saved image (default 'image')\n");
    printf("-B           disable border\n");
    printf("-D           disable diagonal line\n");
    printf("-G           disable grid lines\n");
    printf("-R           disable line repeat (so counter value isn't reset each line)\n");
    printf("-S           disable serial number overlay\n");
    printf("-T           disable tiff output, writes raw file instead\n");
    printf("-W WIDTH     use WIDTH pixels from one vertical gridline to another\n\
            (default: %d)\n", GRID_WIDTH);
    printf("-L LENGTH    use LENGTH pixels from one horizontal gridline to another\n\
            (default: %d)\n", GRID_HEIGHT);
    printf("-w WIDTH     set image width to WIDTH pixels (should be multiple of 256)\n");
    printf("-h HEIGHT    set image length (height) to HEIGHT pixels\n");
    printf("-d DEPTH     set image depth to DEPTH bits per pixel 8,10,12,14,16,24, or 32 (default 8))\n");
    puts("");
    puts("Options for creating multiple images: \n");
    printf("-N NUM       create NUM images (default 1)\n");
    printf("-X WIDTH [DX]  max width of images created, & DX is optional delta \n\
            added to each one (defaults: width from -w, dx 0)\n");
    printf("-Y LENGTH [DY] max length (height) of images created, & DY is \n\
            optional delta added to each one (defaults: height from -l, dy 0)\n");
    printf("-r           randomize image width/height up to image max width/height\n");
    printf("-i listname  save list of images (for send_tiffs) to listname (default basename_depth.lst)\n");

    return;
}

#define bits2bytes(bits) (((int)bits + 7) / 8)

/* write image to output file */
static void write_tiff(u_char *buffer, char *outfile_name, int width, int height, int depth);
static void 
write_raw(u_char *buffer, char *outfile_name, int image_size);


/* Return the depth mask for the given bit depth */
static u_int get_depth_mask(int depth)

{
    int mask;
    switch (depth)
    {
        case 8:
            mask = 0xff;
            break;
        case 10:
            mask = 0x3ff;
            break;
        case 12: 
            mask = 0xfff;
            break;
        case 14:
            mask = 0x3fff;
            break;
        case 16: 
            mask = 0xffff;
            break;
        case 24:
            mask = 0xffffff;
            break;
        case 32:
            mask = 0xffffff;
            break;
    }
    return mask;
}

/* Fills buffer with width * height * bits2bytes(depth_bits) counter
 * data, repeating as necessary.
 * bits2bytes(depth_bits) just rounds depth_bits up to the smallest 
 * byte that will hold that many bits. 
 *
 * If depth is 24 bits, it is assumed that the image is RGB, and will be
 * filled as 32 bits like: '0RGB', where R, G, and B are 8 bit
 * independing red, green, and blue values (the upper byte is always 0).
 *
 * example: 
 * fill_buffer(buffer, 10, 10, 8) fills a 10x10 image with 
 * the first 100 8-bit values counting from 0 to 255.
 *
 * fill_buffer(buffer, 10, 10, 16) fills a 10x10 16-bit image with 
 * the first 100 16-bit values from 0 to 65535 (so, that would be 0 -> 100).
 *
 * fill_buffer(buffer, 64, 64, 10) fills a 64x64 10-bit image with
 * 4096 10-bit values from 0 to 1023 (repeating 4 times).  The upper 6 bits
 * of the upper byte will be zero.
 *
 * @param repeat_lines  If true, all lines are the same (counter is
 * reset at start of each line). 
 */
    static void 
fill_buffer(u_char *buffer, int width, int height, int depth, int repeat_lines) 
{
    int row;
    int repeat;
    int counter;
    u_char *bufptr;
    u_int x_start = 0; /* initial x color value */

    /* max_val is the maximum value we could count up to on each row. */
    int max_val = (int)pow(2, depth);

    int depth_bytes = bits2bytes(depth);
    if (depth_bytes == 3) depth_bytes = 4; /* rgb adjustment */
    if (depth > 10)
    { 
        /* start at some grey value, otherwise the whole image would be
         * black */
        x_start = get_depth_mask(depth) / 2;
    }


    repeat = width / max_val;
    /* repeat must be at least 1, even if width < max_val */
    if (repeat < 1)
    {
        repeat = 1;
    }

    counter = x_start;
    for (row = 0; row < height; ++row)
    {
        int x = 0;
        int line_done = 0;
        if (repeat_lines)
        {
            /* if we want each line to be the same, reset counter each
             * line */
            counter = x_start;
        }
        /* set bufptr to start of row */
        bufptr = buffer + (row*width*depth_bytes);
        /* fill one line of image */
        while (!line_done)
        { /* fill line */
            if (x >= width || counter >= max_val)
            {
                line_done = 1;
            }
            else
            {
                int i;
                for (i = 0; i < repeat && x < width && counter < max_val; ++i, ++x)
                {
                    set_pixel_color(bufptr, counter, depth);
                    bufptr = bufptr + depth_bytes;
                }
                ++counter;
            }
        }
    }
}







/* set color value of pixels in buffer. 
 * Like memset: fills the first n bytes of the memory area pointed to by 
 * buffer with the constant c. 
 * @param c: value to set
 * @param n: number of pixels to set
 * @param depth: number of bytes per pixel
 */
    static void 
set_color(u_char *buffer, int c, int width, int height, int depth)
{
    assert(depth >= 8 && depth <=32);

    if (depth == 8)
    {
        memset(buffer, c, width*height);
    } else if (depth == 10 || depth == 12 || depth == 14 || depth == 16)
    {
        u_int mask = get_depth_mask(depth);
        int i;
        for (i = 0; i < width*height; ++i)
        {
            ((u_short *)buffer)[i] = c & mask;
        }
    } else if (depth == 32 || depth == 24)
    {
        int i;
        for (i = 0; i < width*height; ++i)
        {
            buffer[i] = c | 0xff000000; /* set alpha channel to full opacity  */
        }
    }
    else
    {
        fprintf(stderr, "unsupported depth in set_color(depth:%d).\n", depth);
        exit(1);
    }
    return;
}


    static int 
fg_from_depth(int depth) 
{
    u_int mask = 1;
    int i;

    for (i=0;i<depth;i++)
        mask <<= 1;

    mask--;

    return mask;

}

    static void 
set_pixel_color(u_char *bufptr, int c, int depth) 
{
    u_int mask = get_depth_mask(depth);
    assert(depth >= 8 && depth <= 32);

    switch(depth)
    {
        case 8:
            bufptr[0] = c & 0xff;
            break;
        case 10:
        case 12:
        case 14: 
        case 16: 
            ((u_short*)bufptr)[0] = c & mask; 
            break;
        case 24:
        case 32:
            ((u_int*)bufptr)[0] = c | 0xff000000; /* set full opacity on alpha channel */
            break;
        default:
            fprintf(stderr, "unsupported depth value: %d bits\n", depth);
            break;
    }
    return;
}


    static void 
generate_image(char *basename, int index, 
        int width, int height, int depth,
        int serial, 
        int do_serial, int do_grid, int do_diag,
        int grid_width, int grid_height, int grid_color, int do_tiff,
        int x_coord, int y_coord, int do_borders,
        int repeat_lines,
        FILE *image_list)

{
    int row;
    unsigned char *orig_buffer;
    int   image_size;
    unsigned char *buffer;
    char outfile_name[256];

    u_int white;
    u_int lightgrey;
    u_int darkgrey;

    int depth_bytes;

    assert(depth >= 8 && depth <= 32 );

    if (depth == 24 || depth == 32)  
    {
        /* Assume 4 bytes for RGB */
        depth_bytes = 4;
        image_size = width * height * depth_bytes;
    }
    else 
    {
        depth_bytes = bits2bytes(depth);
        image_size = width * height * depth_bytes;
    }

    switch (depth) 
    {
        case 8:
            white = 0xff;
            lightgrey = 128; /* 0x80 */
            darkgrey = 192; /* 0xC0 */
            break;
        case 10:
        case 12:
        case 14:
        case 16:
            white = 0xffff;
            lightgrey = 0x97ff; /* based on 10 bit images, it'd be 256 and 768? */
            darkgrey = 0x5000;
            break;
        case 32:
        case 24:
            white = 0xffffffff;
            lightgrey = 0x747170;
            darkgrey = 0x504A4B;
            break;
    }


    /* Create and fill the buffer */
    orig_buffer = buffer = calloc(image_size, 1);
    fill_buffer(buffer, width, height, depth, repeat_lines); 


    /* overlay grid on top of buffer */
    if (do_grid) 
    {
        unsigned char *bufptr;
        /*
         * for every line between 0 and height,
         * if line is multiple of grid_height, fill line with grid value
         * else set the pixel value to the grid value for every multiple of
         * width
         */
        for (row = 0; row < height; ++row) 
        {
            int column;
            bufptr = buffer + row*width*depth_bytes;
            if (row % grid_height == 0)
            {
                for (column = 0; column < width; ++column)
                {
                    set_pixel_color(bufptr, grid_color, depth);
                    bufptr = bufptr + depth_bytes;
                }
            }
            else
            {
                for (column = grid_width; column < width; column += grid_width)
                {
                    bufptr += grid_width * depth_bytes;
                    set_pixel_color(bufptr, grid_color, depth);
                }
            }
        }
    }


    /* TODO: allow user to set border colors?
     * put borders on image */
    if (do_borders) 
    {
        int top_color = white;
        int bottom_color = 0;
        int right_color = darkgrey;
        int left_color = lightgrey;

        int border_height = 5; /* for top/bottom borders */
        int border_width = 10; /* for left/right borders */

        unsigned char *bufptr;

        if (border_height > height) 
            border_height = 1;
        if (border_width > width) 
            border_width = 1;


        /* top and bottom */
        set_color(buffer, top_color, width, border_height, depth);
        bufptr = buffer + width * (height - border_height) * depth_bytes;
        set_color(bufptr, bottom_color, width, border_height, depth);

        /* right and left */
        bufptr = buffer + border_height * width * depth_bytes;
        for (row = border_height; row < height - border_height; ++row)
        {
            set_color(bufptr, right_color, border_width, 1, depth);
            bufptr += (width - border_width) * depth_bytes;
            set_color(bufptr, left_color, border_width, 1, depth);
            bufptr += border_width * depth_bytes;
        }
    }



    /* put diagonal line on image */
    if (do_diag)
    {
        for (row = 0; row < height; ++row)
        {
            float x_mark = (row / (float)height) * width * depth_bytes;
            /* TODO: invert all bits in every pixel to make line visible
             * regardless of pixel values in pattern of image */
            /* *(buffer + depth*(row*width + (int)x_mark)) ^= 0xff; */
            set_pixel_color(buffer + depth_bytes*(row*width) + (int)x_mark, grid_color, depth);
        }
    }



    /* mark image with serial # and other info */
    if (do_serial && serial >= 0 )
    {
        unsigned char *bufptr;

        /* TODO: pdv_mark_ras should have a multibyte version */
        /* ensure that the image is large enough */
        int fg;
        int d = depth;

        if (d == 24)
            d = 32;


        fg = fg_from_depth(d);

        if (width >= 150) 
        {

            pdv_mark_ras_depth((u_short *) buffer, serial, width, height, x_coord, y_coord, d, fg); 
        }

        if (width >= 300)
        {

            pdv_mark_ras_depth((u_short *) buffer, width, width, height, x_coord+150, y_coord,d, fg); 
        }
        if (width >= 450)
        {

            pdv_mark_ras_depth((u_short *) buffer, height, width, height, x_coord+300, y_coord,d,fg); 
        }

        if (depth == 8)
        {
            /* write serial # (binary and ascii), and signature on first line */
            bufptr = buffer + 4; /* skip four bytes for simulator counter */
            *bufptr = serial; 
            bufptr += 4;
            sprintf((char*)bufptr, 
                    "serial #:%d. Width:%d, Height:%d. EDT Image Generator.",
                    serial, width, height);
        }
    }



    /* save image */
    if (serial != -1)
        sprintf(outfile_name, "%s_%d_%dx%dx%d.%s", 
                basename, serial, width, height, depth, do_tiff ? "tif" : "raw");
    else
        sprintf(outfile_name, "%s_%dx%dx%d.%s", 
                basename, width, height, depth, do_tiff ? "tif" : "raw");

    if (do_tiff)
    {
        write_tiff(buffer, outfile_name, width, height, depth);
    }
    else
    {
        write_raw(buffer, outfile_name, image_size);
    }

    printf("wrote image to %s\n", outfile_name);

    if (image_list)
        fprintf(image_list, "%s\n", outfile_name);

    free(buffer);
}



int main(int argc, char *argv[]){
    char *progname = argv[0];

    int width = -1, height = -1; /* in pixels */
    int depth = 8; /* in bytes */
    int image; /* counter */

    int serial = -1; /* serial number to print on image, -1 means don't print */
    int x_coord = SERIAL_X, y_coord = SERIAL_Y; /* x,y coordinates of serial # */

    int do_grid = 1;
    int grid_width = GRID_WIDTH, grid_height = GRID_HEIGHT;
    int grid_color = GRID_LINE_COLOR;

    int widthmod = 4;
    int heightmod = 1;
    int widthmax = -1;
    int heightmax = -1;
    int widthmin = -1;
    int heightmin = -1;

    char basename[40] = "image";

    int n_images = 1;

    int do_borders = 1;
    int do_diag = 1;
    int do_serial = 1;
    int do_tiff = 1;
    int randomize = 0;

    int repeat_lines = 1;

    FILE *image_list = NULL;

    if (argc < 2)
    {
        usage(progname);
        exit(1);
    }

    ++argv; --argc;
    while (argc && argv[0][0] == '-') 
    {
        switch (argv[0][1]) 
        {
            case 'w': /* required width argument */
                ++argv; --argc;
                width = atoi(argv[0]);
                if (width < 6)
                {
                    fprintf(stderr, "Sorry, width must be greater than 5\n");
                    /* because pdv_mark_ras cannot handle less */
                    exit(1);
                }
                widthmin = width ;
                break;
            case 'h':
            case 'l': /* required height argument */
                ++argv; --argc;
                height = atoi(argv[0]);
                if (height < 9)
                {
                    fprintf(stderr, "Sorry, height must be greater than 8\n");
                    /* because pdv_mark_ras cannot handle less */
                    exit(1);
                }
                heightmin = height;
                break;

            case 'd': /* depth */
                ++argv; --argc;
                depth = atoi(argv[0]);
                if (depth < 8 || depth > 32)
                {
                    fprintf(stderr, "Unsupported depth: %d bits\n", depth);
                    exit(1);
                }
                break;

            case 'n': 
                ++argv; --argc;
                serial = atoi(argv[0]);
                break;

            case 'x': 
                ++argv; --argc;
                x_coord = atoi(argv[0]);
                break;

            case 'y': 
                ++argv; --argc;
                y_coord = atoi(argv[0]);
                break;

            case 'i':
                ++argv; --argc;
                image_list = fopen(argv[0],"w");
                break;


            case 'B': /* disable border */
                do_borders = 0;
                break;

            case 'D': /* disable diagonal line */
                do_diag = 0;
                break;

            case 'G': /* disable gridlines */
                do_grid = 0;
                break;

            case 'R': /* disable counter-reset on each line (disable line-repeat) */
                repeat_lines = 0;
                break;

            case 'S': /* disable serial */
                do_serial = 0;
                break;

            case 'T': /* disable tiff output */
                do_tiff = 0;
                break;

            case 'W': 
                ++argv; --argc;
                grid_width = atoi(argv[0]);
                break;

            case 'L': 
                ++argv; --argc;
                grid_height = atoi(argv[0]);
                break;

            case 'c':
                ++argv; --argc;
                grid_color = atoi(argv[0]);
                break;

            case '-':
                if (strcmp(argv[0],"--help") == 0)
                {
                    usage(progname);
                    exit(0);
                }
                break;

            case 'N':
                ++argv; --argc;
                n_images = atoi(argv[0]);
                break;

            case 'X':
                ++argv; --argc;
                widthmax = atoi(argv[0]);
                if (argc > 1 && argv[1][0] != '-')
                {
                    ++argv; --argc;
                    widthmod = atoi(argv[0]);
                }
                break;

            case 'Y':
                ++argv; --argc;
                heightmax = atoi(argv[0]);
                if (argc > 1 && argv[1][0] != '-')
                {
                    ++argv; --argc;
                    heightmod = atoi(argv[0]);
                }
                break;

            case 'r':
                randomize = 1;
                break;

            case 'b':
                ++argv; --argc;
                strcpy(basename, argv[0]);
                break;

            default:
                printf("unknown option: \"-%c\"\n", argv[0][1]);
                usage(progname);
                exit(1);
        }
        ++argv; --argc;
    }

    if (!image_list)
    {
        char s[45];
        sprintf(s,"%s_%d.lst",basename, depth);
        image_list = fopen(s,"w");
    }

    if (widthmax == -1)
        widthmax = widthmin;
    if (heightmax == -1)
        heightmax = heightmin;

    for (image = 0;image < n_images;image++)
    {

        if (width < 1 || height < 1)
        {
            printf("ERROR: width and height need to be specified.\n");
            usage(progname);
            exit(1);
        }

        generate_image(basename, image, 
                width, height, depth,
                serial,
                do_serial, do_grid, do_diag,
                grid_width, grid_height, grid_color, do_tiff,
                x_coord, y_coord, do_borders, repeat_lines, image_list);

        serial++;
        if (randomize)
        {
            int dx = widthmax - widthmin;
            int dy = heightmax - heightmin;

            width = (int)(widthmin + (dx * ((float) rand() / (float) RAND_MAX)));
            height = (int)(heightmin + (dy * ((float) rand() / (float) RAND_MAX)));

        }
        else
        {
            width += widthmod;
            if (width > widthmax)
                width = widthmin;

            height += heightmod;
            if (height > heightmax)
                height = heightmin;
        }

        /* Set width, height to multiples of widthmod, heightmod.
         * The CameraLink Simulator only accepts images which have width
         * a multiple of 4. */
        width = width - (width % widthmod);
        height = height - (height % heightmod);
    }

    if (image_list)
        fclose(image_list);

    return 0;
}



/* image_size: # bytes to write to file */
    static void 
write_raw(u_char *buffer, char *outfile_name, int image_size)
{
    FILE *outfile = fopen(outfile_name, "w");
    if (NULL == outfile)
    {
        perror("error opening output file ");
        exit(-1);
    }
    if (fwrite(buffer, image_size, 1, outfile) == 0 )
    {
        perror("error writing file to disk ");
        exit(-1);
    }
    return;
}



/* depth: # bits per pixel */
    static void 
write_tiff(u_char *buffer, char *outfile_name, int width, int height, int depth)
{
    int row;
    int image_offset;
    int rows_per_strip;

    TIFF *image;

    int bits_per_sample;
    int samples_per_pix;


    int depth_bytes = bits2bytes(depth);
    if (depth == 24 || depth == 32)
    {
        depth_bytes = 4; /* four, because rgb is actually alpha+rgb */
        bits_per_sample = 8;
        samples_per_pix = 4;
    }
    else
    {
        bits_per_sample = depth_bytes * 8;
        samples_per_pix = 1;
    }



    assert(depth >= 8 && depth <= 32);


    /* according to libtiff docs, a strip should not have more than 8K bytes.
     * If a row is wider than 8K though, the strip will contain that one row and
     * will be larger than 8K. */
    rows_per_strip = 8192 / (width*depth_bytes);
    if (rows_per_strip < 1) 
        rows_per_strip = 1;

    /* Open the TIFF file */
    if((image = TIFFOpen(outfile_name, "w")) == NULL){
        printf("Could not open %s for writing\n", outfile_name);
        exit(42);
    }

    /* We need to set some values for basic tags before we can add any
     * data */
    TIFFSetField(image, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(image, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, bits_per_sample); 
    TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, samples_per_pix); 
    TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, rows_per_strip);

    TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    /* LZW compression not available everywhere, do to the Unisys patent */
    /* TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_LZW); */
    if (depth == 24 || depth == 32)
    {
        TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    }
    else
    {
        TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    }
    TIFFSetField(image, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
    TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);


    /* Write the information to the file */
    image_offset = 0; 
    for (row = 0; row < height; ++row)
    {
        int result = TIFFWriteScanline(image, buffer + image_offset, row, 0);
        if (result == -1)
        {
            fprintf(stderr, "Write error on line: %d\n", row);
            exit(-1);
        }
        image_offset += width * depth_bytes;
    }

    /* Close the file */
    TIFFClose(image);
    return;
}
