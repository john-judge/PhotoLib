/*
 * Jenny Thai
 * 10/08/02
 * check_gap.c
 *
 * This program performs continuous input from edt dma device
 * and optionally reads in different size of memory buffer.
 * The program keeps track of the number of gaps from the input
 * and the result of throughput is printed out with the table
 * of frequency.
 *
 * INPUT: check_gap -l <loop_size> or check_gap -F <clock_speed>
 */

#include "edtinc.h"
#include <stdlib.h>
#include <fcntl.h>

float   clock_speed = 66000000;

#define MAXFREQ 100000

#define MAXCHANNELS 1

#define PCITESTSS	0x02010010	/* zeros shifted data if bit set */

typedef unsigned short Count;

#define MAXCOUNT (Count) (-1)

#define BLOCKS_GRANULARITY 0x100000

#define MAXBUFS 256


#define INIT_BLOCKS_SIZE    0x1000000 /*  16M blocks should usually work*/



typedef struct {
    int start;
    int runlength;
    int gaplength;
} BlockRecord;

typedef struct
{

    u_int   freqs[MAXFREQ];
    u_int   blocks[MAXFREQ];

    Count   last_index;
    double  t_xferred;
    double  total_clocks;

    int     started;

    int	    nBlocks;
    int     nBlocksAllocated;
    BlockRecord *pBlocks;

} GapHistory;

typedef struct _stats {
    int n;
    double sum;
    double ssq;
    double minval;
    double maxval;
    double mean;
    double sd;
} Stats;

void
stats_clear(Stats *sp)
{

    memset(sp, 0, sizeof(*sp));
}

void
stats_add_value(Stats *sp, double val)

{
    if (sp->n == 0)
    {
	sp->minval = sp->maxval = val;
    }
    else
    {
	if (val < sp->minval)
	    sp->minval = val;
	else if (val > sp->maxval)
	    sp->maxval = val;
    }

    sp->n++;
    sp->sum += val;
    sp->ssq += val * val;

}

void
stats_summarize(Stats *sp)
{

    if (sp->n > 0)
    {
	sp->mean = sp->sum / sp->n;
    }

}

void
compute_minmax_speed(GapHistory *pgaps, int delta, Stats *sp)

{
 
    int i;
    int d1, d2;
    int dcount;
    int gaps;
	int q;

    BlockRecord *pb = pgaps->pBlocks;

    /* scan to delta */
    gaps = 0;

    for (i=0;i < pgaps->nBlocks && pb[i].start < delta; i++)
	gaps += pb[i].gaplength;

    if (i==pgaps->nBlocks)
	return;

    d1 = 0;
    d2 = 0;

    for ( ; i < pgaps->nBlocks; i++)
    {
	/* find previous block at delta */
	dcount = pb[i].start - delta;

	while (dcount > pb[d2].start && 
		d2 < i) 
	{
	    d2++;
	}

	if (d2 > 0)
	    d1 = d2-1;

	/* figure out if delta is in block or gap */
	if (pb[d1].start + pb[d1].runlength > dcount)
	{

	    gaps = pb[d1].gaplength;

	}
	else
	{

	    gaps = pb[d2].start - dcount;
	}

	for (q=d2;q<i;q++)
		gaps += pb[q].gaplength;

	stats_add_value(sp, gaps);

    }
	
    stats_summarize(sp);
}


static void
alloc_block_array(GapHistory *pgaps, int newlength)

{

    if (newlength == 0)
    {
       if (pgaps->nBlocksAllocated && pgaps->pBlocks)
	    free(pgaps->pBlocks);
    }

    if (pgaps->nBlocksAllocated)
    {
	int diff = newlength - pgaps->nBlocksAllocated;
	
	pgaps->pBlocks = realloc(pgaps->pBlocks, newlength * sizeof(BlockRecord));
	
	if (diff > 0)
	    memset(pgaps->pBlocks + pgaps->nBlocksAllocated, 0, 
					diff * sizeof(BlockRecord));

	pgaps->nBlocksAllocated = newlength;
    
    }
    else
    {
	pgaps->pBlocks = calloc(newlength,sizeof(BlockRecord));
    }

    pgaps->nBlocksAllocated = newlength;

}

void
InitGaps (GapHistory * pgaps)
{

    if (pgaps)
      {
	  memset (pgaps, 0, sizeof (*pgaps));

	  alloc_block_array(pgaps, INIT_BLOCKS_SIZE);

      }
}

static void do_swab(u_short *buf, u_int size)
{
    u_int i ;
	u_char *tmp_p ;
	for (i = 0 ; i < size ; i++)
	{
		tmp_p = (u_char *)buf ;
		*buf++ = tmp_p[1] << 8 | tmp_p[0] ;
	}
}
	
/*
 * count_gap
 *
 * count_gap takes in an array of unsigned integer buffer and keep
 * track of the number of gaps in the array
 */
int
count_gap (Count * buf, u_int size, GapHistory * pgaps, char *textout)
{
    /*Declaring variables */
    Count   lastvalue, newvalue;
    Count   firstvalue;
    FILE   *fout = NULL;
    Count   blockstart;

    int     xferred = 0;
    int     index = 0;
    int     runlength = 1;
    int     clock = 0, gaplength;

    int     num_of_element = size;

    if (!edt_little_endian()) do_swab(buf,size) ;

    lastvalue = buf[0];

    firstvalue = lastvalue;
    blockstart = lastvalue;
    index = 1;

    if (textout)
	fout = fopen (textout, "w+");

    if (fout)
	fprintf (fout, "Start Value %d\n", firstvalue);

    pgaps->pBlocks[pgaps->nBlocks].start = clock;
    /*loops through until end of buffer */
    while (index < num_of_element)
      {
	  newvalue = buf[index];	/*Read in new value */
	  if (newvalue < lastvalue)
	    {
		/*wraparound */
		gaplength = MAXCOUNT - lastvalue + newvalue;

	    }
	  else
	    {
		gaplength = newvalue - lastvalue;
	    }

	  /*Out of range */
	  if (gaplength < 0 || gaplength >= MAXFREQ)
	    {
		gaplength = MAXFREQ - 1;
	    }

	  /*Print out index,runlength, and gaplength */
	  if (gaplength > 1)
	    {
		xferred += (runlength * 4);	/*Number of bytes transferred */
		pgaps->blocks[runlength]++;
		pgaps->freqs[gaplength - 1]++;

		if (fout)
		  {
		      fprintf (fout, "Block %8d -> %8d size %8d\n",
			       blockstart, lastvalue, runlength);
		      fprintf (fout, "Gap   %8d -> %8d size %8d\n",
			       lastvalue + 1, newvalue - 1, gaplength - 1);
		  }

		/* append block data */
		pgaps->pBlocks[pgaps->nBlocks].gaplength = gaplength - 1;
		pgaps->pBlocks[pgaps->nBlocks].runlength = runlength;
		pgaps->nBlocks ++ ;

		if (pgaps->nBlocks >= pgaps->nBlocksAllocated)
		    alloc_block_array(pgaps, pgaps->nBlocksAllocated + BLOCKS_GRANULARITY);
	        
		blockstart = newvalue;

		runlength = 1;
		clock += gaplength + 1;
		pgaps->pBlocks[pgaps->nBlocks].start = clock;

	    }
	  else if (gaplength == 1)
	    {
		runlength++;
		clock++;
	    }

	  index++;
	  lastvalue = newvalue;
      }

    /* last block */
    xferred += (runlength * 4);	/*Number of bytes transferred */
    pgaps->blocks[runlength]++;

    if (fout)
      {
	  fprintf (fout, "Block %8d -> %8d size %8d\n",
		   blockstart, lastvalue, runlength);
	  fprintf (fout, "Gap   %8d -> %8d size %8d\n",
		   lastvalue + 1, newvalue - 1, gaplength - 1);
      }

    clock += gaplength + 1;

    pgaps->last_index += index;	/*Keep track of data */
    pgaps->t_xferred += xferred;
    pgaps->total_clocks += clock;

    if (fout)
	fclose (fout);

    return 0;
}

/*
 * output_gap
 *
 * output_gap takes in total length, total bytes transferred, total clock,
 * and frequency to display the result to standard output
 */
int
output_gap (GapHistory * pgaps)
{
    int     i;
    double  t;
    int     totalgaps = 0;


    printf ("Transferred:  %10.0f bytes\n", pgaps->t_xferred);
    printf ("Clocks: %10.0f\n", (double) pgaps->total_clocks);

    t = pgaps->total_clocks / clock_speed;

    printf ("Bytes per second: %10f \n\n", (pgaps->t_xferred / t));

    /*Print frequency result table */
    for (i = 0; i < MAXFREQ; i++)
      {
	  if (pgaps->blocks[i])
	      printf ("\t Block of: %d \t Frequency: %d\n", i,
		      pgaps->blocks[i]);
      }
    /*Print frequency result table */

    for (i = 0; i < MAXFREQ; i++)
      {
	  if (pgaps->freqs[i])
	      printf ("\t Gap of: %d \t Frequency: %d\n", i, pgaps->freqs[i]);
	  totalgaps += pgaps->freqs[i];
      }
    printf ("Total # of gaps: %d\n", totalgaps);

    return 0;
}

/*Display default message*/
void    usage();

/* GapHistory gaps;*/

/*
 * Main module. NO_MAIN is typically only defined when compiling for vxworks; if you
 * want to use this code outside of a main module in any other OS, just copy the code
 * and modify it to work as a standalone subroutine, including adding parameters in
 * place of the command line arguments
 */
#ifdef NO_MAIN
#include "opt_util.h"
char *argument ;
int option ;
int
cl_check_gap(char *command_line)
#else
int
main(int argc, char **argv)
#endif
{
    EdtDev *edt_p;		/*Pointer for device handle */

    int     sunswap = 0;

    int     bufsize = 1024 * 1024;
    int     unit = 0, i, nbuffers = 4;
    int     channel = 2;


    char   *rawout = NULL;
    char   *rawin = NULL;
    char   *textout = NULL;
    FILE   *f = NULL;
    GapHistory *gaps ;
    Stats stats[17];
    int loops = 1;
    int loop;

#ifdef NO_MAIN
    char **argv  = 0 ;
    int argc = 0 ;
    opt_create_argv("cl_check_gap",command_line,&argc,&argv);
#endif

    while (argc > 1 && argv[1][0] == '-')
      {
	  switch (argv[1][1])
	    {
	    case 'u':		/*specified unit number */
		++argv;
		--argc;
		if (argc < 2) 
        {
		    usage("Error: option 'u' requires a numeric argument\n");
	    }
	    if ((argv[1][0] >= '0') && (argv[1][0] <= '9'))
        {
            unit = atoi(argv[1]);
        }
        else 
        {
		    usage("Error: option 'u' requires a numeric argument\n");
	    }
	    break;

	    case 'F':		/*specified clock speed */
		++argv;
		--argc;
		clock_speed = (float) atof (argv[1]);
		if (clock_speed < 1000)
		    clock_speed *= 1000000.0;
		break;

	    case 'N':		/*specified number of nbuffers */
		++argv;
		--argc;
		nbuffers = atoi (argv[1]);
		if (nbuffers > MAXBUFS)
		{
		    printf("Maximum nbuffers is %d\n", MAXBUFS);
		    nbuffers = MAXBUFS;
		}

		break;
	    case 'l':		/*specified number of loops */
		++argv;
		--argc;
		if (argc < 2) 
        {
		    usage("Error: option 'l' requires a numeric argument\n");
	    }
	    if ((argv[1][0] >= '0') && (argv[1][0] <= '9'))
        {
            loops = strtol (argv[1], 0, 0);
        }
        else 
        {
		    usage("Error: option 'l' requires a numeric argument\n");
	    }
	    break;

	    case 's':
		++argv;
		--argc;
		bufsize = strtol (argv[1], 0, 0);
		break;

	    case 'S':
		sunswap = 1;
		break;

	    case 'o':
		++argv;
		--argc;
		if (argc > 0)
		    rawout = argv[1];
		break;

	    case 'i':
		++argv;
		--argc;
		if (argc > 0)
		    rawin = argv[1];
		break;

	    case 't':
		++argv;
		--argc;
		if (argc > 0)
		    textout = argv[1];
		break;

        case 'h':
        case 'H':
            usage("");
            break;

	    default:
		usage("unknown option\n");
		exit(1);
		break;
	    }
	  --argc;
	  ++argv;
      }

    gaps = malloc(sizeof(GapHistory)) ;
    InitGaps (gaps);

    if (rawin)
    {
	int size ;
	Count *rawbuf ;
	for (i=1; i <= 16; i++)
	{
	    stats_clear(&stats[i]);
	}
	f = fopen (rawin, "rb");
	fseek(f, 0, SEEK_END);
	size = ftell(f)  ;
	fseek(f, 0, SEEK_SET);
	printf("size %x %d recs\n",size, (int)(size/sizeof(Count)));
	rawbuf = (Count *)malloc(size) ;
        fread (rawbuf, size, 1, f);

        count_gap (rawbuf, size / sizeof (Count), gaps, textout);

	for (i=1; i <= 16; i++)
	{
	    int delta = i * 128;

	    compute_minmax_speed(gaps,delta,&stats[i]);

	}
	gaps->nBlocks = 0;

    }
    else
    {

    if ((edt_p = edt_open_channel (EDT_INTERFACE, unit, channel)) == NULL)
      {
	  perror ("edt_open");
	  exit (1);
      }
    edt_flush_fifo (edt_p);

    if (edt_configure_block_buffers(edt_p, bufsize, nbuffers, EDT_READ, 0, 0) ==
	-1)
      {
	  perror ("edt_configure_ring_buffers");
	  exit (1);
      }

	for (i=1; i <= 16; i++)
	{
	    stats_clear(&stats[i]);
	}
    /* reset the fifo in PCI Xilinx and in Interface Xilinx */
    /* then we will start all the DMAs and then start the fifos */
    for (loop = 0; loop < loops; loop++)
    {

	edt_start_buffers (edt_p, nbuffers);

	edt_reg_or (edt_p, PDV_CFG, 4);

	for (i = 0; (i < nbuffers); i++)
	{
	  edt_wait_for_buffers (edt_p, 1);
	}

        count_gap ((Count *) edt_p->ring_buffers[0],
		(bufsize * nbuffers) / sizeof (Count), gaps, textout);

	if (rawout)
	    f = fopen (rawout, "wb+");

	if (f)
	    fwrite (edt_p->ring_buffers[0], bufsize * nbuffers, 1, f);

	if (f)
	    fclose (f);
	for (i=1; i <= 16; i++)
	{
	    int delta = i * 128;

	    compute_minmax_speed(gaps,delta,&stats[i]);

	}
	gaps->nBlocks = 0;

	edt_close (edt_p);

   }
   }


    output_gap (gaps);

        printf("Delta   Min Xfer   Speed:  Min    Mean     Max\n");

    for (i=1; i <= 16; i++)
    {
	int delta = i * 128;
	printf("%4d:      %4d             %6.2f  %6.2f  %6.2f  \n",		
		delta * 4,  4 * (delta - (int) stats[i].maxval),
		((delta-stats[i].maxval) / delta) * 4 * clock_speed / 1000000.0,
		((delta-stats[i].mean) / delta) * 4 * clock_speed/ 1000000.0,
		((delta-stats[i].minval) / delta) * 4 * clock_speed/ 1000000.0
		);
    }


    /*Display result for throughput */


    return 0;
}



/*
 * usage displays the command line description for each command
 */
void
usage(char *err)
{
    printf("%s", err);
    printf ("usage: \n");
    printf ("	-u <unit>           - specifies edt board unit number\n");
    printf ("	-F <clock speed>    - specifies the clock's speed\n");
    printf ("	-l <loops size>	    - specifies the loops size in Mbytes\n");
    exit(1);
}
