

#include "edtinc.h"

#include "libedtdma.h"


#ifdef __linux__

#include <sys/user.h>

#endif

static double usleep_time = 0.0;

// default min 5 ms

u_int configured_min_sample = 50000;

// if dt < 10 us, don't bother waiting
double edt_llp_min_wait = .000010;


#ifndef PAGE_SHIFT
#define PAGE_SHIFT (12)
#endif
                                                                                                                                
#ifndef PAGE_SIZE
#define PAGE_SIZE (1UL << PAGE_SHIFT)
#endif
                                                                                                                                
#ifndef PAGE_MASK
#define PAGE_MASK (PAGE_SIZE-1)
#endif


double edt_min_usleep_time()

{
   
    if (usleep_time == 0.0)
    {
	int loops = 100;
	int i;
	double t;

    	t = edt_dtime();
	
	for (i=0;i<loops;i++)
	{
	    edt_usleep(1);
	}

	t = edt_dtime();

	usleep_time = (t / (double) loops) * 1000000.0;

    }

    return usleep_time;
}


void dump_llp(EdtDev *edt_p)

{
    printf("-----------edt_p Dump-------------\n");
    printf("blocks               = %p\n", 	edt_p->blocks);
    printf("period               = %u\n", 	edt_p->period);
    printf("next_sample          = %f\n", 	edt_p->next_sample);
    printf("cursample            = %d\n", 	edt_p->cursample);
    printf("minchunk             = %d\n", 	edt_p->minchunk);
    printf("base_buffer           = %p\n", 	edt_p->base_buffer);
    printf("data_end             = %p\n", 	edt_p->data_end);
    printf("last_sample_end      = %p\n", 	edt_p->last_sample_end);
    printf("wait_mode            = %d\n", 	edt_p->wait_mode);
    printf("loops                = %d\n", 	edt_p->loops);
    printf("started              = %d\n", 	edt_p->todo);
}
/**


*/

int
edt_compute_buffer_size(u_int bytespersec, 
			u_int sample_us, 
			u_int granularity)

{
    u_int bufsize;

    if (sample_us)
    {
	bufsize = (bytespersec * sample_us) / 1000000;
    }
    else
    {
	bufsize = bytespersec;
    }

    if (granularity)
    {
	if (bufsize % granularity)
	{
	    bufsize = (((bufsize/granularity) + 1) * granularity);
	}
    }

    return bufsize;
       
}

 
int
edt_compute_actual_interval(u_int bytespersec, u_int bufsize)

{
    return 0;
}


/**
*
*
*
*/

int  
edt_dma_configure(EdtDev *edt_p, 
			  u_int bytespersec,	/* maximum speed */
			  u_int sample_us,	/* us per sample */
			  u_int buffer_ms,	/* total buffered time in ms */
			  u_int fixed_buffer_size, /* buffer size multiple of this */
			  u_int flags)

{

    u_int bufsize;
    u_int numbufs;
    u_int min_sample_us = sample_us;
    u_int sample_size;
    u_int totaldata;
    u_int granularity;

    if (NULL == edt_p)
	return -1;

    edt_p->wait_mode = EDT_DMA_SPIN;

    /* calculate sample_size */
    if (flags & EDT_USE_USLEEP)
    {
	
	min_sample_us = (u_int) (edt_min_usleep_time());

	edt_p->wait_mode = EDT_DMA_SLEEP;
    }
    else if (min_sample_us >= (u_int) edt_min_usleep_time())
    {
	edt_p->wait_mode = EDT_DMA_SLEEP;
    }
    printf("usleep_time() = %f\n", edt_min_usleep_time());

    sample_size = (u_int) (((double) bytespersec / 1000000.0) * min_sample_us);    
    /* align sample size to desired granularity */

    granularity = edt_p->buffer_granularity;

    if (granularity)
    {
	if (sample_size > granularity)
	{
	    if (sample_size % granularity)
	    {
		sample_size = ((((sample_size + (granularity/2))/granularity)) * granularity);
	    }
	}
	else
	{
	    
	}
    }

    /* calculate buffer size and numbufs */
    if (fixed_buffer_size)
    {

	sample_size = fixed_buffer_size;
	flags |= EDT_FORCE_BUFSIZE;

    }

    if (flags & EDT_FORCE_BUFSIZE)
    {
	bufsize = sample_size;
	edt_p->wait_mode = EDT_DMA_WAIT;
    }
    else
    {
	printf("min_sample_us = %d\n", min_sample_us);

	if (min_sample_us >= configured_min_sample)
	{
	    bufsize = sample_size;
	}
	else
	{

	    /* how many samples / configured_min_sample */
	    /* constrain to multiples of page size */
	    
	    u_int samples_per_buf = configured_min_sample / min_sample_us;
	    
	    printf("samples_per_buf = %d sample_size = %d\n", samples_per_buf, sample_size);

	    bufsize = samples_per_buf * sample_size;

	    if (bufsize % PAGE_SIZE)
	    {
		bufsize = ((bufsize/PAGE_SIZE)+1)*PAGE_SIZE;
	    }
	    
	}
    
    }

    totaldata = (int) ((double)buffer_ms * (double) bytespersec / 1000.0);

    printf("buffer_ms = %d bytespersec = %d totaldata = %d\n",
	    buffer_ms ,bytespersec ,totaldata);

    numbufs = totaldata / bufsize;

    if (numbufs > (u_int) edt_get_max_buffers(edt_p))
	numbufs = (u_int) edt_get_max_buffers(edt_p);
    

    if (bufsize <= 0 && numbufs <= 0)
    {
	/* ERROR */
	return -1;
    }

    edt_set_buffer_granularity(edt_p, 0);

    edt_configure_block_buffers(edt_p, 
	    bufsize, numbufs, EDT_READ, 0, 0);

    edt_p->blocks = 
	(EdtDMADataBlock *) calloc(numbufs, sizeof(EdtDMADataBlock));

    edt_p->data_end = edt_p->base_buffer + (bufsize * numbufs);

    edt_p->period = (u_int) min_sample_us;

    printf("***************************\n");
    printf("edt_dma_configure\n");
    printf("sample_us     = %d\n", sample_us);
    printf("bytespersec   = %d\n", bytespersec);
    printf("buffer_ms     = %d\n", buffer_ms);
    printf("granularity   = %d\n", granularity);
    printf("flags         = 0x%02x\n", flags);
    printf("***************************\n");
    printf("bufsize       = 0x%x\n", bufsize);
    printf("numbufs       = %d\n", numbufs);
    printf("period        = %d us\n", min_sample_us);
    printf("total data    = %d bytes\n", totaldata);
    printf("wait mode     = %s\n", 
	   (edt_p->wait_mode == EDT_DMA_WAIT) ? "wait_for_buffers":
	   (edt_p->wait_mode == EDT_DMA_SLEEP) ? "usleep" : "spin on timestamp");
	  
    dump_llp(edt_p);
 
    return 0;
}

void
edt_dma_start(EdtDev *edt_p, int loops)

{
    int ntostart = edt_p->ring_buffer_numbufs-1;

    edt_p->loops = loops;
    if (edt_p->freerun)
	ntostart = 0;
    else if (loops && (ntostart < loops))
	ntostart = loops;
    
    edt_start_buffers(edt_p, ntostart);
    edt_p->todo = ntostart;
    edt_p->next_sample = edt_timestamp() + ((double)edt_p->period * 0.000001);

    edt_p->last_sample_end = edt_p->base_buffer;
}

u_char * 
edt_dma_next_sample(EdtDev *edt_p, u_int *sample_size)

{
    u_char *data = NULL;

    if (edt_p->pending_samples > 0)
    {
	
	data = edt_p->blocks[edt_p->cursample].pointer;
	*sample_size = edt_p->blocks[edt_p->cursample].length;

	edt_p->pending_samples--;
	edt_p->cursample++;

    }
    else
	/* this is really an error */
	*sample_size = 0;

    return data;

}

/** 
  Updates the internal table of pointers and lengths 
  */

void edt_dma_get_available(EdtDev *edt_p)

{
    u_int ntowait;

    u_int start_buffer;
    EdtDMADataBlock * blockp;
    u_char *last_data_ptr;
    u_char *start_p;
    u_int whichblock;
    u_int current_offset;
    u_int current_buffer = 0;
    int whack_count = 0;

    blockp = edt_p->blocks;

    /* check where last sample ended */
    start_buffer = edt_p->donecount % edt_p->ring_buffer_numbufs;
    start_p = edt_p->last_sample_end;

    /* whichblock is where last_sample_end resides */
    whichblock = (u_int) (edt_p->last_sample_end - edt_p->base_buffer)/
	    edt_p->ring_buffer_bufsize;

    /* check where we are now */

    do {
	current_offset = edt_get_bufbytecount(edt_p, &current_buffer);

    // printf(" %10f buf = %4d offset = %8d\n", 
    //	   edt_dtime(), current_buffer, current_offset);

        if (current_buffer >= edt_p->ring_buffer_numbufs)
        {
    	printf("\ncurrent_buffer whacked %x %d\n", current_buffer, current_offset);
        }
	edt_msleep(0);
	if (++whack_count > 10)
		exit(1);
    } while (current_buffer >= edt_p->ring_buffer_numbufs);

    if (edt_p->minchunk && (current_offset % edt_p->minchunk))
    {
	current_offset -= (current_offset % edt_p->minchunk);
    }
    
    last_data_ptr = edt_p->ring_buffers[current_buffer] + current_offset;

    if (start_p == last_data_ptr)
    {
	/* we haven't gotten a whole chunk */
	edt_p->pending_samples = 0;
	return;
    }

    if (current_buffer != start_buffer)
    {
	ntowait = (current_buffer + 
	    edt_p->ring_buffer_numbufs - start_buffer) % 
		edt_p->ring_buffer_numbufs;
    }
    else
    {
	ntowait = 0;
    }
   
    if (ntowait)
    {
	u_int i;
	for (i=0;i<ntowait;i++)
	{
	    u_int timebuf[2];

	    edt_wait_buffers_timed(edt_p, 1, timebuf);

	    edt_p->last_buffer_time = (double) timebuf[0] * 1000000.0 + timebuf[1] * 0.001;
	    
	    if (!edt_p->freerun && (!edt_p->loops || 
		(edt_p->loops > edt_p->todo)))
	    {
		edt_start_buffers(edt_p, 1);
		edt_p->todo++;
	    }
	}
    }

    if (start_p < last_data_ptr)
    {
	edt_p->pending_samples = 1;
	blockp->pointer = start_p;
	blockp->length = (u_int) (last_data_ptr - start_p);
    }
    else
    {
	edt_p->pending_samples = 2;
	blockp->pointer = start_p;
	blockp->length = (u_int) (edt_p->data_end - start_p);
	blockp++;
	blockp->pointer = edt_p->base_buffer;
	blockp->length = (u_int) (last_data_ptr - edt_p->base_buffer);	
    }

    edt_p->last_sample_end = last_data_ptr;
    if (edt_p->last_sample_end >= edt_p->data_end)
	edt_p->last_sample_end = edt_p->base_buffer;

    edt_p->cursample = 0;

}


/** Equivalent to wait function, except blocks gets filled in every time
   and return starts at end of last sample always */

u_char * edt_dma_get_sample(EdtDev *edt_p, u_int *datasize, DmaTimeStamp *timestamp)

{
    u_char *data;
    double t;
    double pending_time;

    // check to see if we just return the next chunk
    if (edt_p->pending_samples)
    {
	return edt_dma_next_sample(edt_p,datasize);
    }

    //Wait for Data

    else
    {
	if (edt_p->wait_mode == EDT_DMA_WAIT)
	{
	    u_int timebuf[2];
	/* force wait for buffers */
	    data = edt_wait_buffers_timed(edt_p,1, timebuf);
	    edt_p->last_buffer_time = (double) timebuf[0] * 1000000.0 + timebuf[1] * 0.001;
	    if (!edt_p->freerun && (!edt_p->loops || 
		(edt_p->loops > edt_p->todo)))
	    {
		edt_start_buffers(edt_p, 1);
		edt_p->todo++;
	    }		
	    *datasize = edt_p->ring_buffer_bufsize;
	    return data;
	}
	t = edt_timestamp();
	pending_time = edt_p->next_sample - t;
		
	// wait for the next sample 
	if (pending_time > edt_llp_min_wait)
	{

	    if (edt_p->wait_mode == EDT_DMA_SLEEP && pending_time >= usleep_time)
	    {
		edt_usleep((int) (pending_time * 1000000));
	    }
	    else 
	    {
		int loops = 0;
		/* spin on timestamp() */

		while (edt_p->next_sample > edt_timestamp())
		    loops ++;

	    }
	}
    }

    /* do sample */

    edt_dma_get_available(edt_p);

    data = edt_dma_next_sample(edt_p, datasize);

    edt_p->next_sample = edt_p->next_sample + ((double)edt_p->period * 0.000001);

    return data;
}


int
edt_get_reserved_pages(EdtDev *edt_p)

{
    int pages;

    edt_ioctl(edt_p, EDTG_RESERVED_PAGES, &pages);

    return (pages);

}

int
edt_get_reserved_mem(EdtDev *edt_p)

{
    int pages;

    pages = edt_get_reserved_pages(edt_p);

    return (pages * PAGE_SIZE);

}

unsigned int
edt_ring_buffer_pages_used(int n, int size)

{
    unsigned int required;
    unsigned int sg_pages;

    required = (size * n) / PAGE_SIZE;
   
    sg_pages = (size / 0x200000);
    if (size % 0x200000)
    	sg_pages ++;

    required += (sg_pages * n);

    return required;
}

unsigned int
edt_check_ring_buffers_reserved(EdtDev *edt_p, int n, int size)

{
    int pages;
    unsigned int required;

    edt_ioctl(edt_p, EDTG_RESERVED_PAGES, &pages);
   
    if (pages == -1)
    	return 0;

    required = edt_ring_buffer_pages_used(n, size);

    if ((int)required > pages)
    	return 1;
    else
    	return 0;
    
}

