/*
 * Program to split a saved raw image stream as individual
 * image files in tiff, bmp, or sun raster format
 */
#include "edtinc.h"
#include <sys/stat.h>
#include "tiffio.h"

static void usage (char *progname);
static void save_image_bmp (char *basename, u_char * image_p,
			    int width, int height, int depth);

#ifndef WIN32

#define MAX_PATH 1024

#endif

#define OUTPUT_RAW 0
#define OUTPUT_BMP 1
#define OUTPUT_TIF 2
#define OUTPUT_RAS 3


#define DEFAULT_FILENAME "pdvrecord"

static struct
{

    char   *name;
    int     value;

} intlv_names[] =
{
    {
    "SPECINST_SERIAL", SPECINST_SERIAL},
    {
    "KODAK_XHF_INTLC", PDV_BYTE_INTLV},
    {
    "BYTE_INTLV", PDV_BYTE_INTLV},
    {
    "FIELD_INTLC", PDV_FIELD_INTLC},
    {
    "WORD_INTLV_HILO", PDV_WORD_INTLV_HILO},
    {
    "WORD_INTLV_HILO_LINE", PDV_WORD_INTLV_HILO_LINE},
    {
    "BYTE_INTLV_MIDTOP_LINE", PDV_BYTE_INTLV_MIDTOP_LINE},
    {
    "WORD_INTLV_MIDTOP_LINE", PDV_WORD_INTLV_MIDTOP_LINE},
    {
    "WORD_INTLV_TOPMID_LINE", PDV_WORD_INTLV_TOPMID_LINE},
    {
    "WORD_INTLV_ODD", PDV_WORD_INTLV_ODD},
    {
    "ES10_WORD_INTLC", PDV_WORD_INTLV},
    {
    "WORD_INTLV", PDV_WORD_INTLV},
    {
    "DALSA_4CH_INTLV", PDV_DALSA_4CH_INTLV},
    {
    "DALSA_LS_4CH_INTLV", PDV_DALSA_LS_4CH_INTLV},
    {
    "PIRANHA_4CH_INTLV", PDV_PIRANHA_4CH_INTLV},
    {
    "SPECINST_4PORT_INTLV", PDV_SPECINST_4PORT_INTLV},
    {
    "ILLUNIS_INTLV", PDV_ILLUNIS_INTLV},
    {
    "ES10_BGGR_INTLV", PDV_ES10_BGGR},
    {
    "ES10_WORD_BGGR_INTLV", PDV_ES10_WORD_BGGR},
    {
    "ES10_WORD_ODD_BGGR_INTLV", PDV_ES10_WORD_ODD_BGGR},
    {
    "ILLUNIS_BGGR", PDV_ILLUNIS_BGGR},
    {
    "QUADRANT_INTLV", PDV_QUADRANT_INTLV},
    {
    "QUADRANT2_INTLV", PDV_QUADRANT2_INTLV},
    {
    "QUADRANT3_INTLV", PDV_QUADRANT3_INTLV},
    {
    "DALSA_2CH_INTLV", PDV_DALSA_2CH_INTLV},
    {
    "INVERT_RIGHT_INTLV_24_12", PDV_INV_RT_INTLV_24_12},
    {
    "INTLV_24_12", PDV_INTLV_24_12},
    {
    "INTLV_1_8_MSB7", PDV_INTLV_1_8_MSB7},
    {
    "INTLV_1_8_MSB0", PDV_INTLV_1_8_MSB0},
    {
    "INVERT_RIGHT_INTLV", PDV_INVERT_RIGHT_INTLV},
    {
    "INVERT_RIGHT_BGGR_INTLV", PDV_INVERT_RIGHT_BGGR_INTLV},
    {
    "DALSA_2M30_INTLV", PDV_DALSA_2M30_INTLV},
    {
    "EVEN_RIGHT_INTLV", PDV_EVEN_RIGHT_INTLV},
    {
    "BGGR_DUAL", PDV_BGGR_DUAL},
    {
    "BGGR_WORD", PDV_BGGR_WORD},
    {
    "BGGR", PDV_BGGR},
    {
    "KODAK_XHF_SKIP", PDV_BYTE_INTLV_SKIP},
    {
    "BYTE_INTLV_SKIP", PDV_BYTE_INTLV_SKIP},
    {
    "WORD_INTLV_TOPBOTTOM", PDV_WORD_INTLV_TOPBOTTOM},
    {
    "BYTE_INTLV_TOPBOTTOM", PDV_BYTE_INTLV_TOPBOTTOM},
    {
    "none", 0}
};


int
lookup_interlace (char *name)
{
    char    upname[256];
    int     i;

    if (!name)
	return 0;

    strcpy (upname, name);

    for (i = 0; upname[i]; i++)
      {
	  upname[i] = toupper (upname[i]);
      }

    for (i = 0; intlv_names[i].value; i++)
	if (!strcmp (intlv_names[i].name, upname))
	    return intlv_names[i].value;

    return 0;
}


char   *
interlace_name (int swinterlace)
{
    int     i;

    for (i = 0; intlv_names[i].value; i++)
	if (intlv_names[i].value == swinterlace)
	    return intlv_names[i].name;

    return intlv_names[i].name;

}

/***********************************
 *
 * returns an output type from a string
 *
 ***********************************/


int
lookup_output_type (char *name)
{
    int     retval = OUTPUT_RAW;


    if (strncasecmp (name, "tif", 3) == 0)
	retval = OUTPUT_TIF;
    else if (strncasecmp (name, "bmp", 3) == 0)
	retval = OUTPUT_BMP;
    else if (strncasecmp (name, "ras", 3) == 0)
	retval = OUTPUT_RAS;

    return retval;

}

/***********************************
 *  take fullname and remove any extension
 *
 *
 ***********************************/

void
prepare_file_name (char *basename, char *fullname)
{
    char *s;

    strcpy(basename, fullname);

    /* delete extension if it exists */

    s = strrchr(basename, '.');

    if (s)
	s[0] = '\0';


}

/***********************************
 *
 * Modify this function to do things like add a date string to
   the file name
 *
 ***********************************/

void
prepare_indexed_file_name (char *basename, char *fullname,
			   int index, char *extension)
{
    sprintf (fullname, "%s-%04d.%s", basename, index, extension);
}

/***********************************
 *
 * Open a raw image file for input
 *
 ***********************************/


HANDLE
open_raw_input (char *filename, int size)
{

    HANDLE  h;


#ifdef _NT_

    int     rwmode;
    int     createmode;
    int     flags = FILE_FLAG_NO_BUFFERING;

    /* check for divisibility by 512 */

    if (size % SECTOR_SIZE)
	flags = 0;

    printf ("Opening input file %s as %s\n", filename,
	    (flags) ? "Unbuffered" : "Buffered");

    rwmode = GENERIC_READ;

    createmode = OPEN_EXISTING;


    h = CreateFile (filename,
		    rwmode, FILE_SHARE_READ, NULL, createmode, flags, NULL);


#else



    h = (HANDLE) open (filename, O_RDONLY, S_IREAD);

    printf ("File %s -> %d\n", filename, h);

#endif

    return h;

}

/***********************************
 *
 * Read a single frame from a raw input file
 *
 ***********************************/

int
read_image_input (HANDLE f, u_char * data, int size)
{

    int     bytesread;

#ifdef WIN32

    ReadFile (f, data, size, &bytesread, NULL);

#else
    bytesread = read (f, data, size);

#endif

    if (bytesread != size)
	printf ("Error: read %d bytes, should have been %d\n",
		bytesread, size);

    return bytesread;
}

int
save_image_tiff (char *fname, u_char * image_p, int width, int height,
		 int depth)
{
    int     rc = -1;

    int     bitspersample;

    int     samples = 1;
    int     pixel_bytes;
    int     row;
    int     rowsize;
    u_char *pRow;
    TIFF   *tif;

    printf ("writing tif file %s\n", fname);

    if (depth <= 8)
      {
	  pixel_bytes = 1;
	  bitspersample = 8;
      }
    else if (depth <= 16)
      {
	  pixel_bytes = 2;
	  bitspersample = 16;
      }
    else if (depth == 24)
      {
	  pixel_bytes = 3;
	  bitspersample = 8;
	  samples = 3;
      }
    else if (depth == 48)
      {
	  pixel_bytes = 6;
	  bitspersample = 16;
	  samples = 3;
      }
    else
      {
	  printf ("Unable to process tiff data of depth %d\n", depth);
	  return -1;
      }

    tif = TIFFOpen (fname, "w");

    // fill in header
    TIFFSetField (tif, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField (tif, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, bitspersample);
    TIFFSetField (tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);


    TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);

    TIFFSetField (tif, TIFFTAG_DOCUMENTNAME, fname);
    TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, samples);
    TIFFSetField (tif, TIFFTAG_ROWSPERSTRIP, height);
    TIFFSetField (tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

    rowsize = width * pixel_bytes;
    pRow = image_p;

    for (row = 0; row < height; row++, pRow += rowsize)
	TIFFWriteScanline (tif, pRow, row, 0);

    TIFFClose (tif);



    return rc;


}

/***********************************
 *
 * Save to a sun raster file
 *
 ***********************************/

void
save_image_raster (char *fname, u_char * image_p, int width, int height,
		   int depth)
{
    int     s_db = bits2bytes (depth);
    u_char *bbuf = NULL;

    switch (s_db)
      {
      case 1:
	  printf ("writing %dx%dx%d raster file to %s\n",
		  width, height, depth, fname);

	  dvu_write_rasfile (fname, (u_char *) image_p, width, height);
	  break;

      case 2:
	  printf ("converting %dx%dx%d image to 8 bits, writing to %s\n",
		  width, height, depth, fname);


	  dvu_write_rasfile16 (fname, (u_char *) image_p, width, height,
			       depth);

	  break;

      case 3:
	  printf ("writing %dx%dx%d bmp file to %s\n",
		  width, height, depth, fname);

	  dvu_write_rasfile24 (fname, (u_char *) image_p, width, height);

	  break;

      default:
	  printf ("invalid image depth for file write...!\n");
	  break;
      }
}

/***********************************
 *
 * Save an image as a Windows Bitmap
 *
 ***********************************/

void
save_image_bmp (char *fname, u_char * image_p, int width, int height,
		int depth)
{
    int     s_db = bits2bytes (depth);
    u_char *bbuf = NULL;


    /*
     * write bmp file on Windows systems, or Sun Raster on Unux/Linux
     * systems. Switch on number of bytes per pixel
     */
    switch (s_db)
      {
      case 1:

	  dvu_write_bmp (fname, image_p, width, height);
	  break;

      case 2:
	  printf ("converting %dx%dx%d image to 8 bits, writing to %s\n",
		  width, height, depth, fname);

	  if (!bbuf)
	      bbuf = (u_char *) pdv_alloc (width * height);

	  if (bbuf == NULL)
	    {
		pdv_perror ("data buf malloc");
		exit (1);
	    }
	  dvu_word2byte ((u_short *) image_p, (u_char *) bbuf,
			 width * height, depth);
	  dvu_write_bmp (fname, bbuf, width, height);
	  break;

      case 3:
	  printf ("writing %dx%dx%d bmp file to %s\n",
		  width, height, depth, fname);

	  dvu_write_bmp_24 (fname, (u_char *) image_p, width, height);

	  break;

      default:
	  printf ("invalid image depth for file write...!\n");
	  break;
      }
}


/***********************************
 *
 * Write an indexed image to a file of output_type
 *
 ***********************************/

int
write_image_output (char *basename, int index, u_char * data,
		    int width, int height, int depth, int output_type)
{

    char    fullname[MAX_PATH];

    switch (output_type)
      {

      case OUTPUT_BMP:
	  prepare_indexed_file_name (basename, fullname, index, "bmp");
	  save_image_bmp (fullname, data, width, height, depth);

	  break;

      case OUTPUT_TIF:
	  prepare_indexed_file_name (basename, fullname, index, "tif");
	  save_image_tiff (fullname, data, width, height, depth);

	  break;

      case OUTPUT_RAW:

	  prepare_indexed_file_name (basename, fullname, index, "raw");

	  break;

      }

    return 0;
}


/**
 * Returns the actual amount of image data for DMA
 * Normally the "depth" value
 * For some reordering options, uses extdepth
 *
 */

int
get_dmasize (int width, int height, int depth, int extdepth, int swinterlace)
{


    int     size;


    if (swinterlace == PDV_INV_RT_INTLV_24_12)
	size = width * height * 3 / 2;
    else if (depth > extdepth)	/* 24 bit bayer filter  or 1 bit data */
	size = height * pdv_bytes_per_line (width, extdepth);
    else
	size = height * pdv_bytes_per_line (width, depth);


    return size;
}

/***********************************
 *
 *
 ***********************************/


int
get_imagesize (int width, int height, int depth)
{

    return height * pdv_bytes_per_line (width, depth);
}

/***********************************
 *
 * Get the number of images in a file, includes code
 * to deal with files > 2 Gb
 *
 ***********************************/

int
get_n_images (HANDLE pFile, int src_image_size)
{

    if (pFile && src_image_size)
      {

#ifdef WIN32

	  LARGE_INTEGER fullsize;

	  fullsize.LowPart =
	      GetFileSize (pFile, (unsigned long *) &fullsize.HighPart);

	  return (int) (fullsize.QuadPart / ((LONGLONG) src_image_size));
#else
	  unsigned long fpos = lseek (pFile, 0, SEEK_CUR);
	  unsigned long filesize = lseek (pFile, 0, SEEK_END);

	  lseek (pFile, fpos, SEEK_SET);

	  printf("filesize %d\n",filesize);
	  return (int) (filesize / src_image_size);

#endif

      }

    return 0;

}


#define BUMP_ARGC(argc, argv) \
if (argc > 1) {--argc ; ++argv;} else {printf("incomplete parameter\n"); usage(progname); exit(1);}

/*
 * main
 */

struct _rgb
{
    double  scale[3];
    double  gamma;
    int     red_row_first;
    int     green_pixel_first;
    int     offset;
    int     quality;
    int     bias;
    int     gradientcolor;
    int     use;
} rgb =
{
    {
1.0, 1.0, 1.0}, 1.0, 0, 0, 0};

main (int argc, char **argv)
{
    int     i;
    int     unit = 0;
    char   *progname = argv[0];
    char    basename[MAX_PATH];
    char    src_file_name[MAX_PATH];
    char    cfgfile[MAX_PATH];

    int     numbufs = 4;

    u_char *src_image_p;
    u_char *dest_image_p;


    char    errstr[64];
    int     loops = 1;
    int     width = 0, height = 0, depth = 0, extdepth = 0;
    int     src_imagesize = 0;
    int     dest_imagesize = 0;
    int     swinterlace = 0;

    int     src_frame_size;
    int     dest_frame_size;

    int     output_type = OUTPUT_BMP;


    int     nimages;

    EdtPostProc *pCtrl;
    int     frame_height = 0;

    int     use_camera = TRUE;

    int     counter = 0;

    HANDLE  f;

    char    edt_devname[128];
    int     channel = 0;

    edt_devname[0] = '\0';



    cfgfile[0] = 0;
    basename[0] = 0;
    src_file_name[0] = 0;

    --argc;
    ++argv;
    while (argc && ((argv[0][0] == '-') ))
      {
	  switch (argv[0][1])
	    {
	    case 'u':		/* device unit number */
		BUMP_ARGC (argc, argv);
		if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
		    unit = atoi (argv[0]);
		else
		    strncpy (edt_devname, argv[0], sizeof (edt_devname) - 1);
		break;

	    case 'c':		/* device unit number */
		BUMP_ARGC (argc, argv);
		if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
		    channel = atoi (argv[0]);
		break;


	    case 'b':		/* bitmap save filename */
		BUMP_ARGC (argc, argv);
		strcpy (basename, argv[0]);
		break;

	    case 'f':		/* config filename */
		BUMP_ARGC (argc, argv);
		strcpy (cfgfile, argv[0]);
		break;

	    case 't':
		BUMP_ARGC (argc, argv);
		output_type = lookup_output_type (argv[0]);
		break;

	    case 'w':
		BUMP_ARGC (argc, argv);
		width = atoi (argv[0]);
		use_camera = FALSE;
		break;

	    case 'h':
		BUMP_ARGC (argc, argv);
		height = atoi (argv[0]);
		use_camera = FALSE;
		break;

	    case 'd':
		BUMP_ARGC (argc, argv);
		depth = atoi (argv[0]);
		use_camera = FALSE;
		break;

	    case 'e':
		BUMP_ARGC (argc, argv);
		extdepth = atoi (argv[0]);
		use_camera = FALSE;
		break;

	    case 'F':
		BUMP_ARGC (argc, argv);
		frame_height = atoi (argv[0]);
		use_camera = FALSE;
		break;

	    case 'i':
		BUMP_ARGC (argc, argv);
		swinterlace = lookup_interlace (argv[0]);
		use_camera = FALSE;
		break;

	    case 'g':
		BUMP_ARGC (argc, argv);
		rgb.gamma = atof (argv[0]);
		break;

	    case 'r':
		BUMP_ARGC (argc, argv);
		rgb.scale[0] = atof (argv[0]);
		BUMP_ARGC (argc, argv);
		rgb.scale[1] = atof (argv[0]);
		BUMP_ARGC (argc, argv);
		rgb.scale[2] = atof (argv[0]);
		BUMP_ARGC (argc, argv);
		rgb.offset = atoi (argv[0]);
		rgb.use = 1;
		break;

	    case 'q':
		BUMP_ARGC (argc, argv);
		rgb.quality = atoi (argv[0]);
		BUMP_ARGC (argc, argv);
		rgb.bias = atoi (argv[0]);
		BUMP_ARGC (argc, argv);
		rgb.gradientcolor = atoi (argv[0]);
		rgb.use = 1;
		break;

	    case 'o':
		BUMP_ARGC (argc, argv);
		{
		    int     order = atoi (argv[0]);

		    if (order >= 0)
		      {
			  rgb.red_row_first = ((order & 2) != 0);
			  rgb.green_pixel_first = ((order & 1) != 0);
		      }
		    rgb.use = 1;
		}
		break;

	    default:
		fprintf (stderr, "unknown flag -'%c'\n", argv[0][1]);
	    case '?':
		usage (progname);
		exit (0);
	    }
	  argc--;
	  argv++;
      }

    if (argc > 0)
	strcpy (src_file_name, argv[0]);

    if (strlen (src_file_name) == 0)
      {
	  printf ("Error: No source filename defined\n\n");
	  usage (progname);
	  exit (1);
      }

    if (use_camera)
      {
	  PdvDev *pdv_p;

	  /*
	   * open the interface
	   *
	   * EDT_INTERFACE is defined in edtdef.h (included via edtinc.h)
	   */
	  if (edt_devname[0])
	    {
		unit =
		    edt_parse_unit_channel (edt_devname, edt_devname, EDT_INTERFACE,
					    &channel);
	    }
	  else
	    {
		strcpy (edt_devname, EDT_INTERFACE);
	    }

	  if ((pdv_p = pdv_open_channel (edt_devname, unit, channel)) == NULL)
	    {
		sprintf (errstr, "pdv_open_channel(%s%d_%d)", edt_devname, unit,
			 channel);
		pdv_perror (errstr);
		return (1);
	    }

	  /*
	   * get image size and name for display, save, printfs, etc.
	   */
	  width = pdv_get_width (pdv_p);
	  height = pdv_get_height (pdv_p);
	  depth = pdv_get_depth (pdv_p);
	  extdepth = pdv_get_extdepth (pdv_p);
	  swinterlace = pdv_p->dd_p->swinterlace;
	  src_imagesize = pdv_get_dmasize (pdv_p);
	  frame_height = pdv_get_frame_height (pdv_p);


	  pdv_close (pdv_p);



      }
    else if (cfgfile[0])
      {

	  Edtinfo edtinfo;
	  PdvDependent *dd_p;

	  if ((dd_p = pdv_alloc_dependent ()) == NULL)
	    {
		printf ("alloc_dependent FAILED -- exiting\n");
		exit (1);
	    }

	  if (pdv_readcfg (cfgfile, dd_p, &edtinfo) != 0)
	    {
		printf ("Error reading cfg file %s\n", cfgfile);
		exit (1);
	    }

	  width = dd_p->width;
	  height = dd_p->height;
	  depth = dd_p->depth;
	  extdepth = dd_p->extdepth;
	  swinterlace = dd_p->swinterlace;
	  frame_height = dd_p->frame_height;

      }
    else
      {
	  if (extdepth == 0)
	      extdepth = depth;

      }


    if (width == 0 || height == 0 || depth == 0)
      {

	  printf ("width or height or depth is 0, exiting\n");

	  exit (1);

      }

    prepare_file_name (basename, src_file_name);

    printf ("Extracting data from %s with parameters\n\
	%d x %d x %d, \n	extdepth = %d, frame_height = %d\n\
	swinterlace = %s\n\n", src_file_name, width, height, depth, extdepth, frame_height, (swinterlace) ? interlace_name (swinterlace) : "none");

    src_imagesize = get_dmasize (width, height, depth, extdepth, swinterlace);



    if (swinterlace)
      {
	  pCtrl = pdv_lookup_postproc (swinterlace, extdepth, depth);

	  //pCtrl = (EdtPostProc *) edt_alloc(sizeof(EdtPostProc));
	  pdv_set_postproc (pCtrl, depth, extdepth,
			    frame_height, 0, 0, 0, 0, 0);
      }
    else
	pCtrl = 0;

    if (rgb.use && depth == 24)
      {
	  pdv_set_full_bayer_parameters (extdepth,
					 rgb.scale,
					 rgb.gamma,
					 rgb.offset,
					 rgb.red_row_first,
					 rgb.green_pixel_first,
					 rgb.quality,
					 rgb.bias, rgb.gradientcolor);


      }

    if (frame_height)
      {
	  src_frame_size = get_dmasize (width, frame_height,
					depth, extdepth, swinterlace);
      }
    else
      {
	  src_frame_size = src_imagesize;
	  frame_height = height;
      }

#ifdef WIN32
    src_imagesize =
	((src_imagesize + SECTOR_SIZE - 1) / SECTOR_SIZE) * SECTOR_SIZE;
#endif

    f = open_raw_input (src_file_name, src_imagesize);

    if (f == (HANDLE) -1)
    {
	fprintf(stderr, "Unable to open file %s\n", src_file_name);
	exit(-1);
    }

    nimages = get_n_images (f, src_imagesize);

    printf("imagesize %d n images %d\n", src_imagesize, nimages);
    src_image_p = edt_alloc (src_imagesize);

    dest_imagesize = get_imagesize (width, frame_height, depth);

    if (pCtrl)
      {
	  dest_image_p = edt_alloc (dest_imagesize);
	  dest_frame_size = dest_imagesize;
      }
    else
      {
	  dest_image_p = src_image_p;
	  dest_frame_size = src_frame_size;
      }

    counter = 0;

    for (i = 0; i < nimages; i++)
      {
	  u_char *sp, *bp, *target_p;

	  read_image_input (f, src_image_p, src_imagesize);


	  sp = src_image_p;
	  bp = dest_image_p;

	  target_p = sp + src_imagesize;

	  do
	    {

		if (pCtrl)
		  {
		      /* process data */

		      deIntlv_buffers (pCtrl, sp, bp, width, frame_height);

		  }
		else
		    bp = sp;

		write_image_output (basename, counter, bp, width,
				    frame_height, depth, output_type);

		sp += src_frame_size;
		counter++;

	    }
	  while (sp < target_p);

      }

    puts ("");

    exit (0);
}

void
usage (char *progname)
{
    puts ("");
    printf
	("process_raw: simple example program that converts images from a raw stream\n",
	 progname);
    puts ("");
    printf ("usage: process_raw [FLAGS] <source file>\n", progname);
    printf ("   FLAGS:\n");
    printf ("  -u unit         %s unit number (default 0)\n", EDT_INTERFACE);
    printf
	("  -t type		  Output file type, one of \"tif\",\"bmp\",\"ras\"\n");
    printf ("\nThere are three ways to define the images to be converted\n");
    printf
	("1. (default) Use the current pdv configuration (requires board in the system,\n");
    printf ("   the camera config must match that used for capture.)\n\n");
    printf ("2. Use a camera config file with -f <cfgfile name>\n\n");
    printf ("3. Define the camera parameters: \n");
    printf ("   -w width\n");
    printf ("   -h height\n");
    printf ("   -d depth\n");
    printf ("   -e extdepth (depth from framegrabber)\n");
    printf ("   -i interleave type\n");
    printf
	("   -F frame height (when there are multiple frames in a single image)\n");
    printf
	("  The following are parameters for Bayer filter interpolation\n");

    printf ("   -r <red> <green> <blue> <black offset> Bayer Parameters\n");
    printf ("   -g <gamma> Bayer Parameter\n");
    printf ("   -o <bggr order> Color order Options are:");
    printf ("       0 bggr\n");
    printf ("       1 gbrg\n");
    printf ("       2 rggb\n");
    printf ("       3 grbg\n");
    printf
	("   -q <Bayer quality 0|1> <Gradient Threshold 0-255> <Color Gradient 0|1>\n");




}
