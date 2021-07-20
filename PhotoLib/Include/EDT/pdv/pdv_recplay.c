/**

*/


#include "edtinc.h"

#include "edt_threads.h"

#include "pdv_recplay.h"

#ifndef WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#endif

static EdtMsgHandler logger;
static EdtMsgHandler *lp = &logger;

/* Message logger masks */
#define MINVRB	 1    /* print only the minimum */
#define CHECKDMA 2
#define CHECKIMG 4
#define CHECK_FV 8
#define CHECK_EVENT 16
#define CHECK_IMG_READ 32
#define MAXVRB	 0x80 /* print everything */

/* Modify this function to do things like add a date string to
the file name */

void prepare_file_name(char *basename, char *fullname)

{
    /* add .raw extension to basename */

    sprintf(fullname, "%s.raw", basename);

}


HANDLE
edt_open_raw_output(char *filename, int append, int size)

{

    HANDLE h;


#ifdef WIN32

    int rwmode;
    int createmode;
    int flags = FILE_FLAG_NO_BUFFERING;

    /* check for divisibility by 512 */

    if (size % SECTOR_SIZE)
	flags = 0;

    printf("Opening output file %s as %s\n", filename, (flags)?"Unbuffered":"Buffered");

    rwmode = GENERIC_WRITE;

    if (append)
	createmode = OPEN_ALWAYS;
    else 
	createmode = CREATE_ALWAYS;


    h = CreateFile(filename,
	rwmode,
	FILE_SHARE_READ,
	NULL,
	createmode,
	flags,
	NULL
	);


#else



    h = (HANDLE) open(filename, O_WRONLY | O_CREAT, S_IWRITE);


#endif

    return h;

}

HANDLE
edt_open_raw_input(char *filename, int size)

{

    HANDLE h;


#ifdef WIN32

    int rwmode;
    int createmode;
    int flags = FILE_FLAG_NO_BUFFERING;

    /* check for divisibility by 512 */

    if (size % SECTOR_SIZE)
	flags = 0;

    printf("Opening input file %s as %s\n", filename, (flags)?"Unbuffered":"Buffered");

    rwmode = GENERIC_READ;

    createmode = OPEN_ALWAYS;

    h = CreateFile(filename,
	rwmode,
	FILE_SHARE_READ,
	NULL,
	createmode,
	flags,
	NULL
	);


#else



    h = (HANDLE) open(filename, O_CREAT, S_IREAD);


#endif

    return h;

}

int
edt_raw_io_init(EdtRecPlayIO *io, char *filename, int direction, int imagesize)

{
    if (filename)
	strcpy(io->basename, filename);
    else
	strcpy(io->basename, DEFAULT_FILENAME);

    /* open the file for output */

    prepare_file_name(io->basename, io->bmpfname);

    if (direction == EDT_WRITE)
	io->f = edt_open_raw_input(io->bmpfname, imagesize);  
    else
	io->f = edt_open_raw_output(io->bmpfname, FALSE, imagesize);  

    return (io->f == 0);
}


int
edt_raw_io_close(EdtRecPlayIO *io)

{
#ifdef WIN32
    CloseHandle(io->f);
#else
    close(io->f);
#endif

    free(io);

    return 0;
}

uint64_t
edt_raw_io_get_size(EdtRecPlayIO *io)

{
    
#ifdef WIN32

    LARGE_INTEGER size;

#if _MSC_VER > 1200
    size.QuadPart = 0;
    GetFileSizeEx(io->f, &size);

#else
    u_int slow, shigh;
    
    slow = GetFileSize(io->f,&shigh);
    size.QuadPart =  ((uint64_t) shigh << 32) | slow;

#endif

    return (uint64_t) size.QuadPart;


#else

    uint64_t size;

    size = lseek(io->f, 0, SEEK_END);

    return size;

#endif

}

void
edt_raw_file_rewind(HANDLE f)

{

#ifdef WIN32

    SetFilePointer(f,0,NULL,FILE_BEGIN);


#else

    lseek(f, 0, SEEK_SET);

#endif
    

}


int
edt_raw_io_write(EdtRecPlayIO *io, int index,
		   u_char *data, int width, int height, int depth, int size)

{

    int byteswritten;

    if (index == 0)
	edt_raw_file_rewind(io->f);

#ifdef WIN32
    WriteFile(io->f, data, size, &byteswritten, NULL);

#else
    byteswritten = write(io->f, data, size);

#endif

    if (byteswritten != size)
    {
	printf("Error: wrote %d bytes, should have been %d\n",
	    byteswritten, size);
	return -1;
    }
    return 0;

}


int
edt_raw_io_read(EdtRecPlayIO *io, int index,
		   u_char *data, int *width, 
		   int *height, int *depth, int *size)

{

    int bytesread;

    if (index == 0)
	edt_raw_file_rewind(io->f);

#ifdef WIN32

    ReadFile(io->f, data, *size, &bytesread, NULL);

#else

    bytesread = read(io->f, data, *size);

#endif

    if (bytesread != *size)
    {
	printf("Error: read %d bytes, should have been %d\n",
	    bytesread, *size);
	return -1;
    }
    return 0;

}

/* */

EdtRecPlayIO *
new_raw_io()

{
    EdtRecPlayIO *p;

    p = calloc(1,sizeof(EdtRecPlayIO));

    p->initialize =	    edt_raw_io_init;
    p->read_next_image =    edt_raw_io_read;
    p->write_next_image =   edt_raw_io_write;
    p->close =		    edt_raw_io_close;
    p->get_size =	    edt_raw_io_get_size;

    return p;
}

/******************************************
 * Record/Playback routines
 *
 ******************************************/


/* print current buffer status */

void
edt_record_playback_print_status(EdtRecPlay *rpb)

{
#define SHOWDELTA

#ifdef SHOWDELTA
    int done;
    done = edt_done_count(rpb->pdv_p);
    if (rpb->direction == EDT_WRITE)
	printf("%3d - %3d = %3d :: file %5d ==> simulator %d\r", 
	rpb->started, done, rpb->started - done,
	rpb->images_read, rpb->images_written);
    else
	printf("%3d - %3d = %3d :: capture %5d ==> file %d\r", 
	rpb->started, done, rpb->started - done,
	rpb->images_read, rpb->images_written);
#else
    if (rpb->direction == EDT_WRITE)
	printf("file %5d ==> card %d\r", 
		rpb->images_read, rpb->images_written);
    else
	printf("card %5d ==> file %d\r", 
		rpb->images_read, rpb->images_written);

#endif

    fflush(stdout);
}


/* 
Write a single image 
*/

int
edt_record_write_image(EdtRecPlay *rpb, u_char *image_p)

{
    /* For now, this only appends images to a raw file */

    rpb->io->write_next_image(rpb->io, 
	    rpb->images_written % rpb->nimages, 
	    image_p, rpb->width, rpb->height, rpb->depth, 
	    rpb->imagesize);

    rpb->images_written++;

    return 0;
}

/*
Add a new image to the recording - if writing on the fly,
write immediately, otherwise store to membuffers
*/

int
edt_record_append_image(EdtRecPlay *rpb, u_char *image_p)

{

    if (!rpb->inmemory)
    {
	edt_record_write_image(rpb, image_p);
    }
    else
    {
	memcpy(rpb->membuffers[rpb->images_read],image_p, rpb->imagesize);
    }

    rpb->images_read ++;


    return 0;

}

/* Write out all unwritten images from memory */


int
edt_record_write_images(EdtRecPlay *rpb)

{
    int i;

    for (i=rpb->images_written;i<rpb->images_read;i++)
    {
	edt_record_write_image(rpb,rpb->membuffers[i]);
	edt_record_playback_print_status(rpb);
    }

    return 0;
}

/* 
Write a single image 
*/

int
edt_playback_read_image(EdtRecPlay *rpb, u_char *image_p)

{
    /* For now, this only appends images to a raw file */
    int index = rpb->images_written % rpb->nimages;

    rpb->io->read_next_image(rpb->io, index, image_p, 
	&rpb->width,&rpb->height,&rpb->depth, &rpb->imagesize);

    rpb->images_read++;

    return 0;
}

/*
Add a new image to the recording - if writing on the fly,
write immediately, otherwise store to membuffers
*/

int
edt_playback_next_image(EdtRecPlay *rpb, u_char *image_p)

{
    int index = rpb->images_written % rpb->nimages;

    if (!rpb->inmemory)
    {
	edt_playback_read_image(rpb, image_p);
    }
    else
    {

	memcpy(image_p, rpb->membuffers[index], rpb->imagesize);
    }
	
    rpb->images_written ++;

    return 0;

}

/* Write out all unwritten images from memory */


int
edt_playback_read_images(EdtRecPlay *rpb)

{
    int i;

    for (i=rpb->images_read;i<rpb->nimages;i++)
    {
	edt_playback_read_image(rpb,rpb->membuffers[i]);
	edt_record_playback_print_status(rpb);
    }

    return 0;
}

/* Factory function for default raw io*/

EdtRecPlayIO * new_raw_io();

/* 
Open the record structure for recording
*/

EdtRecPlay *
edt_record_playback_open(EdtRecPlay *rpb, 
			 char *devname, 
			 int unit, 
			 int channel, 
			 int direction,
			 EdtRecPlayIO * (*iofactory)())

{
    u_char own_rpb = FALSE;
    PdvDependent *dd_p;

    if (rpb)
	memset(rpb,0,sizeof(*rpb));
    else
    {
	rpb = calloc(1,sizeof(*rpb));
	own_rpb = TRUE;
	if (!rpb)
	{
	    edt_msg(EDTLIB_MSG_FATAL, "ERROR: unable to allocate %d bytes\n",
		sizeof(*rpb));	
	    return NULL;
	}
    }

    rpb->pdv_p = pdv_open_channel(devname, unit, channel);

    if (rpb->pdv_p == NULL)
    {
	edt_msg(EDTLIB_MSG_FATAL, 
	    "ERROR: unanble to open pdv unit %d\n",
	    unit);	
	if (own_rpb)
	    free(rpb);
	return NULL;
    }

    /* error if not CLSIM */
    if (direction == EDT_WRITE && !pdv_is_simulator(rpb->pdv_p))
    {
	edt_msg(EDTLIB_MSG_FATAL, "ERROR: unit %d is not a Camera Link Simulator!\n",
	    unit);	

	pdv_close(rpb->pdv_p);

	if (own_rpb)
	    free(rpb);


	return NULL;

    }

    rpb->direction = direction;

    /*
    * get image size and name for display, save, printfs, etc.
    */
    rpb->width = pdv_get_width(rpb->pdv_p);

    if (rpb->width == 0)
    {

	edt_msg(EDTLIB_MSG_FATAL,
	    "%s  not initialized\n", (direction == EDT_WRITE)?"simulator":"camera");
	pdv_close(rpb->pdv_p);

	if (own_rpb)
	    free(rpb);

	return NULL;

    }

    dd_p = rpb->pdv_p->dd_p;

    rpb->height = pdv_get_height(rpb->pdv_p);
    rpb->depth = pdv_get_depth(rpb->pdv_p);

    if (direction == EDT_WRITE && rpb->pdv_p->dd_p->vactv)
    {
	rpb->dataheight = rpb->pdv_p->dd_p->vactv;
	rpb->datastart  = dd_p->vskip * 
	    pdv_bytes_per_line(rpb->pdv_p->dd_p->width, 
		rpb->pdv_p->dd_p->depth); 
    }
    else
    {
	rpb->dataheight = rpb->height;
	rpb->datastart  = 0;
    }

    rpb->imagesize = rpb->dataheight * 
			pdv_bytes_per_line(rpb->pdv_p->dd_p->width, 
			rpb->pdv_p->dd_p->depth);
      
    rpb->dmasize = pdv_get_dmasize(rpb->pdv_p);
    
    rpb->cameratype = pdv_get_cameratype(rpb->pdv_p);

    rpb->numbufs = 8;
    rpb->nimages = 100;

   if (iofactory != NULL)
	rpb->io = iofactory();
    else
	rpb->io = new_raw_io();

    return rpb;
}

/* Shut things down */

int
edt_record_playback_close(EdtRecPlay *rpb)

{

    pdv_close(rpb->pdv_p);    
    rpb->pdv_p = NULL;

    if (rpb->membuffers)
    {
	if (rpb->membuffers[0])
	    free(rpb->membuffers[0]);

	rpb->membuffers[0] = NULL;

	free(rpb->membuffers);

	rpb->membuffers = NULL;

    }

    if (rpb->io)
    {
	rpb->io->close(rpb->io);

	rpb->io = NULL;
    }

    return 0;

}

/* 
Initialize recording for nimages. If inmem is true, allocate enough memory 
to hold the captured images for later writing to file. 
*/

int
edt_record_init(EdtRecPlay *rpb, char *filename, 
		int inmem, int nimages)

{

    int ret = 0;
    printf("reading %d image%s from '%s'\nwidth %d height %d depth %d\n",
	nimages, nimages == 1 ? "" : "s", rpb->cameratype, rpb->width, rpb->height, rpb->depth);

 

    rpb->nimages = nimages;
    rpb->inmemory = inmem;

    ret = rpb->io->initialize(rpb->io, filename, rpb->direction, rpb->imagesize);
    if (ret)
        printf("Failed");

    if (rpb->inmemory)
    {
	int i;

	rpb->membuffers = calloc(nimages, sizeof(u_char *));
	if (rpb->membuffers == NULL)
	{
	    edt_msg(EDTLIB_MSG_FATAL,
		"Unable to allocate image buffer memory for in-memory storage\n");

	    return -1;

	}
	rpb->membuffers[0] = calloc(nimages,rpb->imagesize);

	if (rpb->membuffers[0] == NULL)
	{
	    edt_msg(EDTLIB_MSG_FATAL,
		"Unable to allocate image buffer memory for in-memory storage\n");

	    return -1;
	}

	for (i=1;i<nimages;i++)
	    rpb->membuffers[i] = rpb->membuffers[i-1] + rpb->imagesize;

    }

    return 0;
}





/* Capture images */

int
edt_record(EdtRecPlay *rpb)

{
    int i;
    u_char *image_p;
    int last_timeouts = 0;
    int timeouts;

    /*
    * prestart the first image or images outside the loop to get the pipeline
    * going. Start multiple images unless force_single set in config file,
    * since some cameras (e.g. ones that need a gap between images or that
    * take a serial command to start every image) don't tolerate queueing
    * of multiple images
    */
    /*
    * allocate numbufs buffers for optimal pdv ring buffer pipeline (reduce
    * if memory is at a premium)
    */

    rpb->images_read = rpb->images_written = 0;

    pdv_multibuf(rpb->pdv_p, rpb->numbufs);

    edt_dtime();

    if (rpb->pdv_p->dd_p->force_single)
    {
	pdv_start_image(rpb->pdv_p);
	rpb->started = 1;
    }
    else
    {
	pdv_start_images(rpb->pdv_p, rpb->numbufs);
	rpb->started = rpb->numbufs;
    }


    for (i = 0; i < rpb->nimages; i++)
    {
	int overrun;

	/*
	* get the image and immediately start the next one (if not the
	* last time through the loop). Processing (saving to a file in
	* this case) can then occur in parallel with the next acquisition
	*/
	/* Use raw image data */

	image_p = pdv_wait_image_raw(rpb->pdv_p);

	if (rpb->started < rpb->nimages)
	{
	    pdv_start_image(rpb->pdv_p);
	    rpb->started++;
	}

	timeouts = pdv_timeouts(rpb->pdv_p);
	overrun = edt_reg_read(rpb->pdv_p, PDV_STAT) & PDV_OVERRUN;
	if (timeouts > last_timeouts)
	    printf("\nTimeout at %d\n", i);

	if (overrun)
	    printf("\nOverrun at %d\n", i);

	if (timeouts > last_timeouts)
	{
	    /*
	    * pdv_timeout_cleanup helps recover gracefully after a
	    * timeout, particularly if multiple buffers were prestarted
	    */
	    if (rpb->numbufs > 1)
	    {
		int pending = pdv_timeout_cleanup(rpb->pdv_p);
		pdv_start_images(rpb->pdv_p, pending);
	    }
	    last_timeouts = timeouts;
	}

	edt_record_append_image(rpb, image_p);
	edt_record_playback_print_status(rpb);


    }

    rpb->capturetime = edt_dtime();

    if (rpb->inmemory)
    {
	edt_record_write_images(rpb);
	rpb->savetime = edt_dtime();
    }
    else
	rpb->savetime = rpb->capturetime;

    {
	double totaldata;

	totaldata = (double) rpb->nimages * (double) rpb->imagesize * 0.000001;


	printf("\nCapture = %10.3f Save %10.3f Capture speed %10.3f MB/s Save speed %10.3f MB/s\n",
	    rpb->capturetime, 
	    rpb->savetime, 
	    totaldata/rpb->capturetime,
	    totaldata/rpb->savetime);
    }

    return 0;
}




/* 
Initialize recording for nimages. If inmem is true, allocate enough memory 
to hold the captured images for later writing to file. 
*/

int
edt_playback_init(EdtRecPlay *rpb, char *filename, int inmem, int nloops)

{


    printf("writing %d image%s from '%s'\nwidth %d height %d depth %d\n",
	rpb->nimages, rpb->nimages == 1 ? "" : "s", rpb->cameratype, rpb->width, rpb->height, rpb->depth);

    rpb->loops = nloops;
    rpb->inmemory = inmem;

    rpb->io->initialize(rpb->io, filename, rpb->direction, rpb->imagesize);
    /* open the file for output */

    /* get file size */

    rpb->totalbytes = rpb->io->get_size(rpb->io);

    if (rpb->totalbytes % rpb->imagesize)
    {
	edt_msg(EDTLIB_MSG_WARNING,"Warning: file size not multiple of image size\n");
    }

    rpb->nimages = (int) (rpb->totalbytes / rpb->imagesize);

    if (rpb->nimages == 0)
    {
	edt_msg(EDTLIB_MSG_FATAL,
	    "Unable to allocate image buffer memory for in-memory storage\n");
	
	return -1;
    }

    if (rpb->inmemory)
    {
	int i;

	rpb->membuffers = calloc(rpb->nimages, sizeof(u_char *));
	if (rpb->membuffers == NULL)
	{
	    edt_msg(EDTLIB_MSG_FATAL,
		"Unable to allocate image buffer memory for in-memory storage\n");

	    return -1;

	}
	rpb->membuffers[0] = calloc(rpb->nimages,rpb->dmasize);

	if (rpb->membuffers[0] == NULL)
	{
	    edt_msg(EDTLIB_MSG_FATAL,
		"Unable to allocate image buffer memory for in-memory storage\n");

	    return -1;
	}

	for (i=1;i<rpb->nimages;i++)
	    rpb->membuffers[i] = rpb->membuffers[i-1] + rpb->dmasize;

	edt_dtime();

	edt_playback_read_images(rpb);

	rpb->savetime = edt_dtime();
    }


    return 0;
}


/* Capture images */

int
edt_playback(EdtRecPlay *rpb)

{
    int i;
    u_char *image_p;
    int last_timeouts = 0;
    int timeouts;
    int totalimages;
    double t1;

    /*
    * prestart the first image or images outside the loop to get the pipeline
    * going. Start multiple images unless force_single set in config file,
    * since some cameras (e.g. ones that need a gap between images or that
    * take a serial command to start every image) don't tolerate queueing
    * of multiple images
    */
    /*
    * allocate numbufs buffers for optimal pdv ring buffer pipeline (reduce
    * if memory is at a premium)
    */


    edt_configure_ring_buffers(rpb->pdv_p, rpb->dmasize, rpb->numbufs, EDT_WRITE, NULL);

    totalimages = rpb->loops * rpb->nimages;

    rpb->images_written = 0;

    for (i=0;i<rpb->numbufs;i++)
    {
	edt_playback_next_image(rpb, rpb->pdv_p->ring_buffers[i]);
    }

    t1 = edt_dtime();

    edt_start_buffers(rpb->pdv_p, rpb->numbufs);
    rpb->started = rpb->numbufs;

    for (i = 0; i < totalimages || rpb->loops == 0 ; i++)
    {
	double totaldata;

	/* preload */

	/*
	* get the image and immediately start the next one (if not the
	* last time through the loop). Processing (saving to a file in
	* this case) can then occur in parallel with the next acquisition
	*/
	/* Use raw image data */

	image_p = edt_wait_for_buffers(rpb->pdv_p, 1);

	if (rpb->started < totalimages || rpb->loops == 0 )
	{
	    edt_playback_next_image(rpb, image_p);
	    edt_record_playback_print_status(rpb);
	    edt_start_buffers(rpb->pdv_p,1);
	    rpb->started++;
	}

	timeouts = pdv_timeouts(rpb->pdv_p);

	if (timeouts > last_timeouts)
	{
	    /*
	    * pdv_timeout_cleanup helps recover gracefully after a
	    * timeout, particularly if multiple buffers were prestarted
	    */
	    if (rpb->numbufs > 1)
	    {
		int pending = pdv_timeout_cleanup(rpb->pdv_p);
		pdv_start_images(rpb->pdv_p, pending);
	    }
	    last_timeouts = timeouts;
	}


	if ((i % rpb->nimages == 0 && i > 0) || i == totalimages -1)
	{
	    rpb->capturetime = edt_dtime();

	    totaldata = (double) rpb->nimages * (double) rpb->dmasize * 0.000001;

	    printf("\nPlayback = %10.3f Load %10.3f Playback speed %10.3f MB/s Load speed %10.3f MB/s\n",
		rpb->capturetime, 
		rpb->savetime, 
		totaldata/rpb->capturetime,
		totaldata/rpb->savetime);

	}

    }
    return 0;
}


