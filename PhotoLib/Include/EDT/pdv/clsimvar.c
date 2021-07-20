#include "edtinc.h"
#include <stdlib.h>

#include "clsim_lib.h"

void
usage()
{
    puts("usage: clsimvar [-u unit] [-B] [-b startval] [-r] [-F freq] [-l loops] [-s] [-b   ");
    puts("-u sets device number - default is 0");
    puts("-B load bitfile");
    puts("-b startval - starts # of lines at startval, incrementing every <loops> frames");
    puts("-e endval - last value in incrementing counter, or last random value");
    puts("-r randomize line lengths between startval and endval");
    puts("-s setup initializes simulator");
    puts("-v vgap - interval between frames in lines");
    puts("-V endvgap - ending # of lines between frames - from vgap") ;
    
    puts("-F sets pixel clock in MHz, takes a floating point arg (default 40.0");
    puts("-l <loops> repetititions at a particular line length -> 0 means stay constant");

    puts("-w width (default 1024) \n");
    puts("-h height (default 1024) \n");
    puts("-d depth (default 8) \n");
    puts("-f <config file> Use camera config for parameters\n");
    puts("-D Do dma of internal buffers instead of simulator\n");
}

/* compute the buffer height for a particular buffer */
/* If using image files, replace this with the image file height */

int
get_buffer_height(int buffernum, int baseheight, 
		  int deltaheight, int randomize,
		  int loops)
{
    int nextht = baseheight;

    if (randomize && deltaheight)
	nextht = baseheight + (rand() % deltaheight);
    else if (loops && deltaheight)
    {
        nextht = baseheight + ((buffernum / loops) % deltaheight);
    }

    return nextht;
}

int
get_buffer_vgap(int buffernum, int basevgap, 
		  int maxvgap, int randomize,
		  int loops)
{
    int nextgp = basevgap;

    if (randomize && maxvgap)
	nextgp = basevgap + (rand() % maxvgap);
    else
    if (loops && maxvgap)
    {
        nextgp = basevgap + ((buffernum / loops) % maxvgap);
    }

    return nextgp;
}
    
/* get the next buffer data loaded */
/* for this example, fill with counter data */
/* first four bytes of each row have the index (started) */

void
load_new_buffer16(EdtDev *pdv_p, u_short **buffers, int numbufs, int nextbuf, 
		    int width, int height, int depth, int started)

{
   int column, row;

   u_short mask = 0;

   u_short *bp;
    
   nextbuf %= numbufs;
    
   bp = buffers[nextbuf];

   for (row=0;row<depth;row++)
       mask = (mask << 1) | 1;

   for (row=0;row < height;row++)
   {	
       int *ip = (int *) bp;

       *ip = started;

        bp += 2;

	for (column = sizeof(int); column < width; column ++)
	{
	    *bp = ((column + row) & mask);
	    bp++;
	}
   }

}

void
load_new_buffer8(EdtDev *pdv_p, u_char **buffers, int numbufs, int nextbuf, 
		    int width, int height, int started)

{
   int column, row;

   u_char *bp;
    
   nextbuf %= numbufs;
    
   bp = buffers[nextbuf];

   for (row=0;row < height;row++)
   {	
       int *ip = (int *) bp;

       *ip = started;

       bp += sizeof(int);

	for (column = sizeof(int); column < width; column ++)
	{
	    *bp = ((column + row) & 0xff);
	    bp++;
	}
   }

}

void
load_new_buffer(EdtDev *pdv_p, u_char **buffers, int numbufs, int nextbuf, 
		    int width, int height, int depth, int started)

{
    if (depth == 8)
        load_new_buffer8(pdv_p, buffers, numbufs, nextbuf, width, height, started);
    else if (depth > 8 && depth <= 16)
        load_new_buffer16(pdv_p, (u_short **) buffers, numbufs, nextbuf, width, height, depth, started);
}


int
main(int argc, char **argv)
{
    PdvDev *pdv_p;

    char   *filename = NULL ;

    u_char **buffers;
    int numbufs = 4;
    int bufsize;
    int started = 0;

    int     unit = 0;
    int     bcnt = 0;
    int     chan = 0;
    int	oneshot = 1 ;
    int     nbufs = 0;
    float   freq = 0.0;
    int     i;

    int width = 0;
    int endwidth = 0;
    int nextwidth = 0;
    int lastwidth = 0;

    int channels = 1;
    int height = 0;
    int simen = 0;
    int framecount = 1;
    int hgap = 300;
    int delta_rasters = 0x8000;
    int depth = 8;

    int baseheight = 0;
    int endheight = 0;
    int deltaheight;
    int deltawidth;
    int varmask = 0x5ff; /* this will give values 0 -> 1024 all divisible by 8 */
    int nextht;
    int lastheight;
    int lastgap;

    int setup = 0;
    int loops = 0;
    int count = 0;
    int randomize = 0;
    int basevgap = 300 ;
    int endvgap = basevgap ;
    int nextgp;
    int deltavgap = 32 ;
    int no_bitload = 0;
    char bitfile_name[256];
    char *camera = NULL;

    int do_dma = 0;

    int depth_bytes = 1;

    strcpy(bitfile_name, "clsim.bit");

    i = 1;

    while (i<argc && argv[i][0] == '-')
    {
	switch (argv[i][1])
	{
	case 'l':
	    i++;;
	    
	    loops = atoi(argv[0]);
	    break;

	case 'b':
	    i++;
	    
	    baseheight = atoi(argv[i]);
	    break;

	case 'e':
	    i++;
	    
	    endheight = atoi(argv[i]);
	    break;

	case 's':
	    /* do dma */
	    simen = 1;
	    break;

	case 'r':
	    randomize = !randomize;
	    break;

       case 'u':
	    i++;
	    
	    unit = atoi(argv[i]);
	    break;

	case 'F':    /* Set PLL for desired pixel clock freq in MHz */
	    i++;
	    
	    setup=1;
	    freq = (float)atof(argv[i]);    /* Convert arg to float */
	    break;

     
	case 'v':
	    i++;
	    
	    basevgap = atoi(argv[i]);
	    break;

	case 'V':
	    i++;
	    
	    endvgap = atoi(argv[i]);
	    break;

	case 'N':
	    i++;
	    
	    numbufs = atoi(argv[i]);
	    break;

	    case 'L':
		oneshot = 0;
		break;

	case 'B':
	    no_bitload = 1;
	    break;

	case 'f':
	    i++;
	    
	    camera = argv[i];
	break;

	case 'w':
	    i++;
	    
	    setup=1;
	    width = atoi(argv[i]);
	break;

	case 'W':
	    i++;
	    
	    setup=1;
	    endwidth = atoi(argv[i]);
	break;

	case 'H':
	    i++;
	    
	    hgap = atoi(argv[i]);
	break;

       case 'h':
	    i++;
	    
	    setup=1;
	    height = atoi(argv[i]);
	break;

       case 'd':
	    i++;
	    
	    setup=1;
	    depth = atoi(argv[i]);
	break;
     
       case 'c':
	    i++;
	    
	    channels = atoi(argv[i]);
	break;

	default:
	    usage();
	    exit(0);
	}
	i++;
    }

    if (camera)
    {
        char cmd[80];
        sprintf(cmd, "clsiminit -u %d -f %s\n", unit, camera);
        system(cmd);
    }

    pdv_p = pdv_open_channel("pdv", unit, chan);


    if (pdv_p == NULL)
    {
	fprintf(stderr, "Could not open EDT device for writes");
	exit(1);
    }

    if (!width)
    {
        width = pdv_get_width(pdv_p);
        if (!endwidth)
            endwidth = width;
        if (!nextwidth)
            nextwidth = width;
        if (!lastwidth)
            lastwidth = width;
    }

    if (!height)
        height = pdv_get_height(pdv_p);

    edt_set_autodir(pdv_p, 0) ;

    delta_rasters = height + basevgap;

    if (baseheight == 0)
	baseheight = height;
    if (endheight == 0)
	endheight = height;

    nextht = baseheight;
    nextgp = basevgap;
    lastheight = -1;
    lastgap = -1;

    depth = pdv_get_depth(pdv_p);
    depth_bytes = (depth + 7)/8;

    bufsize = width * height * depth_bytes;


    deltaheight = endheight - baseheight;
    deltawidth = endwidth - width;
    deltavgap = endvgap - basevgap;

    if (!simen)
    {

	edt_configure_ring_buffers(pdv_p, bufsize, numbufs, EDT_WRITE, NULL);

	buffers = (u_char **)pdv_buffer_addresses(pdv_p);

	for (i=0;i<numbufs;i++)
	{
	    load_new_buffer(pdv_p, buffers, numbufs, i, width, height, depth, i);	    
	}
	
	
	edt_start_buffers(pdv_p, 1);

	started = 1;

    }

    while (count < loops || loops == 0)
    {
	int lastbuf, curbuf;

	if (simen)
	{
	while (!(edt_reg_read(pdv_p, PDV_STAT) & 2))
	    edt_msleep(10);
	}
	else
	{
	    edt_wait_for_buffers(pdv_p,1);
	}
	
	/* set up next output buffer */
	lastbuf = edt_done_count(pdv_p)-1;
	curbuf = lastbuf+1;
	nextht = get_buffer_height(curbuf, baseheight, 
		  deltaheight,  randomize,
		  loops);

	nextgp = get_buffer_vgap(curbuf, basevgap, 
		  deltavgap,  randomize,
		  loops);

	nextwidth = get_buffer_height(curbuf, width, 
		  deltawidth,  randomize,
		  loops);

	nextwidth &= 0xfff0;


	count ++;
	


	if (nextht != lastheight || nextgp != lastgap || nextwidth != lastwidth)
        {
	pdv_cls_set_height(pdv_p, nextht, nextgp);
	pdv_cls_set_width(pdv_p, nextwidth, 300);

	if (!simen)
	{
	    /* set the next dma buffers size */

	    curbuf %= numbufs;

	    edt_set_buffer_size(pdv_p, curbuf, nextht*width, EDT_WRITE);

	}
        printf("\nHeight: %d Vgap: %d Width %d\n", nextht,nextgp, nextwidth);


        }
        else {
            printf(".");
            fflush(stdout);
	}

	lastheight = nextht;
	lastgap = nextgp;
	lastwidth = nextwidth;

	if (simen)
	    while ((edt_reg_read(pdv_p, PDV_STAT) & 2))
		edt_msleep(10);
	else
	{
	    /* this is where you load an image into the buffer that just finished DMA */
	    
	    started++;

	    load_new_buffer(pdv_p, buffers, numbufs, lastbuf, width, height, depth, started);
	    
	    edt_start_buffers(pdv_p, 1);
	    
	}
    }

 
    return(0) ;
}
