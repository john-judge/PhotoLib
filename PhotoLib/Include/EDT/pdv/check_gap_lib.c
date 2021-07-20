/* check_gap_lib.c: common functionality of checkgap, made into library */

#include "check_gap_lib.h"
#include "edtinc.h"
#include <stdlib.h>
#include <fcntl.h>


/* the following macro is used inside loops. if channel is 0, 
 * channel becomes 2 and the camera link stuff is done.  If channel
 * is not 0 (like if it is 2), the CL stuff was already done, and 
 * we're finished. */
#define CL_CODE(chan) \
if (d->board_type == BOARD_TYPE_CL) { \
	if ((chan) == 0) { \
		chan = 2; \
	} else { \
		break; \
	} \
}



int chkgap_parse_options(int argc, char **argv, CheckGapData *d) 
{
	while (argc > 1 && argv[1][0] == '-') {
		switch(argv[1][1]) {
			case 'u': /*specified unit number*/
				++argv;
				--argc;
				d->unitstr = argv[1];
				break;

			case 'F': /*specified clock speed*/
				++argv;
				--argc;
				d->clock_speed = (float) atof( argv[1] );
				break;

			case 'l': /*specified number of loops*/
				++argv;
				--argc;
				d->loops = atoi( argv[1] );
				break;
								
			case 'n': /*specified # channels (CL ignores this)*/
				++argv;
				--argc;
				d->nchannels = atoi( argv[1] );
				/* the check for nchannels correctness is later.*/
				break;

			case 's': /*specified buffer size to use*/
				++argv;
				--argc;
				d->bufsize = strtol(argv[1], 0, 0);
				break;

			case 'S': /*bytes need swapping on Sun*/
				d->sunswap = 1;
				break;

			case 'o': /*specified raw output filename*/
				++argv;
				--argc;
				if (argc > 0)
					d->rawout = argv[1];
				break;

			case 't': /*specified text output filename*/
				++argv;
				--argc;
				if (argc > 0)
					d->textout = argv[1];
				break;

			case 'B': /*don't run bitload; option currently only for SS */
				d->bitload = !d->bitload;
				break;

			case '-':
				printf("debug; user gave longopt: %s\n", argv[1]);
				if (strcmp(argv[1], "--help") == 0) {
						chkgap_usage();
						exit(0);
				} else if (strcmp(argv[1], "--ss1") == 0) {
						/* user says the board is a SS 1-channel */
						d->ss1_flag = 1;
						d->ss4_flag = 0;
				} else if (strcmp(argv[1], "--ss4") == 0) {
						/* user says board is PSS4_ID */
						d->ss4_flag = 1;
						d->ss1_flag = 0;
				}
				break;

			/* TODO: add support for --help. Remove all segfaulting 
			 * situations (like "command -u ") */

			default:
				chkgap_usage();
				exit(1);
				break;
			}
			++argv;
			--argc;
	}
	d->unit = edt_parse_unit(d->unitstr, d->dev, NULL);
}


/* chkgap_initialize: 
 *
 * configures check_gap's internal data (d), and 
 * initializes the boards to so they are ready to be run
 * through the test */
void chkgap_initialize (CheckGapData *d) 
{
	EdtDev *tmp_edt_p; /* used for probing board type */
	char *bitfilename;
	int channel; /* loop variable */

	/* open device and figure out its board type (devid). */
	if ( (tmp_edt_p = edt_open(d->dev, d->unit)) == NULL) {
		edt_perror("edt_open");
		exit(1);
	}
	d->exact_board_type = edt_device_id(tmp_edt_p);
	edt_close(tmp_edt_p);

	/* Set up card specific variables */
	switch (d->exact_board_type) {
		case PDVCL_ID:
			printf("DEBUG: setting up CL data\n");
			/* set up for camera link */
			d->board_type = BOARD_TYPE_CL;
			/* In the CL, we actually only run on channel two. */
			d->max_channels = 3; /* admittedly, this is a bit of a hack... */
			d->default_channel = 2;
			d->buf_type = USHORT;
			d->type_size = sizeof(u_short);
			break;
		case PCDA_ID:
			puts("DEBUG: setting up CDa data");
			/* set up for pcd */
			bitfilename = "pciss1test.bit";
			d->board_type = BOARD_TYPE_PCD;
			d->max_channels = 4;
			d->buf_type = UINT;
			d->type_size = sizeof(u_int);
			break;

			/* Deal with GS1,4,& 16 boards */
		case PGS4_ID: /* 1 and 4 channel report as this */
		case PGS16_ID:
			/* The GS is so much like the SS that we just call it BOARD_TYPE_SS */
			d->board_type = BOARD_TYPE_SS; 
			d->buf_type = UINT;
			d->type_size = sizeof(u_int);
			if (d->exact_board_type == PGS16_ID) {
					bitfilename = "pcigs16test.bit";
					d->max_channels = 16;
			} else if (d->ss1_flag) {
					bitfilename = "pcigs1test.bit";
					d->max_channels = 1;
			} else {
					/* assume, since ss1 not specified & not SS16, 
					 * that they want 4 chan */
					bitfilename = "pcigs4test.bit";
					d->max_channels = 4;
			}
			break;

			/* Deal with SS1,4,& 16 boards */
		case PSS4_ID: /* 1 and 4 channel report as this */
		case PSS16_ID:
			puts("DEBUG: setting up SS data");
			/* set up for ss */
			d->board_type = BOARD_TYPE_SS;
			d->buf_type = UINT;
			d->type_size = sizeof(u_int);
			if (d->exact_board_type == PSS16_ID) {
					bitfilename = "pciss16test.bit";
					d->max_channels = 16;
			} else if (d->ss1_flag) {
					bitfilename = "pciss1test.bit";
					d->max_channels = 1;
			} else {
					/* assume, since ss1 not specified & not SS16, 
					 * that they want 4 chan */
					bitfilename = "pciss4test.bit";
					d->max_channels = 4;
			}
			break;
		default: /*user has unsupported board type*/
			fprintf(stderr, "Unsupported board type devid=0x%x (only PCDA_ID, "
					"PDVCL_ID, PSS4_ID, PSS16_ID, PGS4_ID, and PGS16_ID supported)\n", 
					d->exact_board_type);
			exit(1);
			break;
	}

	/* load the correct bitfile for SS, GS, and PCDa boards */
	if ( (d->board_type == BOARD_TYPE_SS ||
				d->board_type == BOARD_TYPE_PCD) && d->bitload) {
			char cmd[256];
			sprintf(cmd, "bitload -u %s %s\n", d->unitstr, bitfilename); 
			system(cmd);
	}


	if (d->nchannels > d->max_channels) {
		d->nchannels = d->max_channels;
	} else if (d->nchannels <= 0) {
		d->nchannels = 1; /* default used if user didn't specify this.*/
	}


	/* initialize arrays we'll need (size based on # channels) */
	d->edt_p = calloc(d->max_channels, sizeof(EdtDev *));
	d->gaps = calloc(d->max_channels, sizeof(GapHistory)); 
	d->finished_count = calloc(d->max_channels, sizeof(int));



	/* open all channels and configure edt device */
	for (channel = 0; channel < d->nchannels; ++channel) {
		CL_CODE (channel);

		if (NULL == 
			(d->edt_p[channel] = edt_open_channel(d->dev, d->unit, channel)) ) {
			edt_perror("edt_open");
			exit(1);
		}

		if (d->board_type == BOARD_TYPE_PCD && channel == 0) {
				/* The PCDa uses the same bitfile as the SS cards, and this line
				 * is needed in order for the test to work on both types of cards.
				 * (It is called below for the SS) */
				edt_reg_write(d->edt_p[0],PCITESTSS,0x7ff); 
		}


		if (d->board_type == BOARD_TYPE_SS && channel == 0) {
			u_char cmd = edt_reg_read(d->edt_p[0], PCD_CMD);
			cmd |= 0x08;
			edt_reg_write(d->edt_p[0], PCD_CMD, cmd);
			if (d->exact_board_type == PSS4_ID || d->exact_board_type == PGS4_ID)
				edt_reg_write(d->edt_p[0],PCITESTSS,0x7ff);
			else
				edt_reg_write(d->edt_p[0], SSD16_CHEN, 0xffff) ;
		}

		/* BUG: calling config_bufs with nbufs=d->loops causes problems if 
		 the user want to run more than like 4 loops.
		(running with -l 1000 causes Windows crash). */
		//debug:bad
		//note: now changed to use only 4 bufs.
		if ( edt_configure_ring_buffers(d->edt_p[channel], 
					d->bufsize, 4, EDT_READ, NULL) == -1) {
					//d->bufsize, d->loops, EDT_READ, NULL) == -1) {
			edt_perror("edt_configure_ring_buffers");
			exit(1);
		}

	}
		
	/* set byte swap and short swap if this is a Sun */
	if (d->sunswap) {
		edt_reg_write(d->edt_p[d->default_channel], 
		PCD_CONFIG, PCD_BYTESWAP | PCD_SHORTSWAP);
		/* TODO: find out which swapping CL needs on suns.
		 * I'll do that simply by running the test and seeing if it works,
		 * or using chkgap_mini to print the data values */
	}

}


/* chkgap_run_test: 
 *
 * run the main chunk of the check gap test 
 *
 * This is the function which should be called multiple times 
 * if the test is going to be proformed more than once in 
 * the same instance of a program (such as in a GUI app
 * where the test is initialized and then run as often
 * as the user dictates) 
 *
 * ResultHandler gets called with the results of the test,
 * possibly more than once if d->loops > 1 or something.  See the code.
 */
void chkgap_run_test (CheckGapData *d, ResultHandler handler)
{
	FILE *f = NULL;
	int i, channel; /* loop variables */
	edt_flush_fifo(d->edt_p[d->default_channel]);

	/* Start DMA */	
	for (channel = 0; channel < d->nchannels; ++channel) {
		CL_CODE (channel);
		//debug:good (regarding d->loops, next line is fine).
		edt_start_buffers(d->edt_p[channel], d->loops);
	}

	if (d->board_type == BOARD_TYPE_CL) {
		edt_reg_or(d->edt_p[d->default_channel], PDV_CFG, 4);
	}

	//debug:bad (waiting for loopcount buffers overwrites nearly all of them,
	// if we are only allocating 4 buffers as we should be).
	//fixing this may require a whole restructuring of the code...
	d->finished_count[d->default_channel] = d->loops;
	for (i = 0; i < d->loops; ++i) {
		edt_wait_for_buffers(d->edt_p[d->default_channel], 1);
	}

	if (d->board_type != BOARD_TYPE_CL) {
		for (channel = 1; channel < d->nchannels; ++channel) {
			d->finished_count[channel] = edt_done_count(d->edt_p[channel]);
		}
	}

	if (d->rawout) {
		f = fopen(d->rawout, "wb+");
	}

	/* for each buffer of each channel, run count_gap on it
	 * and output the findings. (then close the edt board/channel) */
	for (channel = 0; channel < d->nchannels; ++channel) {
		CL_CODE (channel);
		for (i = 0; i < d->finished_count[channel]; ++i) {
			chkgap_count_gap((void *) d->edt_p[channel]->ring_buffers[i],
				d->bufsize / d->type_size, 
				&d->gaps[channel], d->textout, d->buf_type);

			if (f) {
				fwrite(d->edt_p[channel]->ring_buffers[i], d->bufsize,1,f);
			}
		}
		/* display results for throughput/gaps/blocksize */
		/* output gap is a func(GapHistory *, int clockspeed) */
		/* typdef int (*ResultHandler) (GapHistory *, int clock_speed) */
		/* Hmm... what if this just returned a list of channels that the
		 * test has been run on? So, run_test would just return { 2 } 
		 * for a camera link, or { 1 } for a PCDa by default, or { 1, 2 }
		 * for a SS which the user wanted to run the test on two channels */
		handler(d, channel);
		edt_close(d->edt_p[channel]);
	}

	if (f) {
		fclose(f);
	}

	/* we are done with the test, so stop data acquisition */
	for (channel = 0; channel < d->nchannels; ++channel) {
		CL_CODE (channel);
		edt_stop_buffers(d->edt_p[channel]);
	}
		
	return 0;
}



		
void chkgap_do_swab(u_short *buf, u_int size)
{
	int i ;
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
 * count_gap takes in an array of unsigned integer buffer and keeps
 * track of the number of gaps in the array
 * buf: array of either u_ints or u_shorts, as specified by buf_type.
 *   buf[n] should be one larger than buf[n-1]. If not, there is a gap.
 * pgaps: the data structure holding information on gaps and blocks.
 * size: the number of elements in the buffer/array
 * textout: name of file to send output to.
 * buf_type: the type of numbers stored in buf, should be UINT or SHORT
 * returns: 0 for success, >0 for failure. (like, 5 means wrong buf_type).
 * TODO: create enum for error_type and return that.
 * If this function is called multiple times, it should add the results for
 * the current count to the pgaps structure (so the end result is cumulative).
 */
int
chkgap_count_gap (void * buf, u_int size, GapHistory * pgaps, 
				char *textout, int buf_type)
{
    /*Declaring variables */
    u_int lastvalue, newvalue;
    u_int firstvalue;
    u_int blockstart;
    u_int maxcount; /* maximum value for the buffer's data type */
    FILE  *fout = NULL;

	u_int gaps_max_index = 0, blocks_max_index = 0;
		

    int xferred = 0;
    int index = 0;
    int runlength = 1;
    int clock = 0, gaplength, real_gaplength;

    int num_of_element = size;
    

    if (!edt_little_endian()) chkgap_do_swab(buf,size) ;

	if (buf_type == UINT) {
		maxcount = (u_int) (-1);
		lastvalue = ((u_int*)buf)[0];
	} else if (buf_type == USHORT) {
		maxcount = (u_short) (-1);
		lastvalue = ((u_short*)buf)[0];
	} else {
		return 5; /* buf_type (arg 5) must be u_int or u_short. */
	}

    firstvalue = lastvalue;
    blockstart = lastvalue;
    index = 1;

	if (textout)
		fout = fopen (textout, "w+");

	if (fout)
		fprintf (fout, "Start Value %d\n", firstvalue);

    /* loops through until end of buffer */
    while (index < num_of_element)
      {
		/* read in new value */
		if (buf_type == UINT) {
			newvalue = ((u_int*)buf)[index];
		} else if (buf_type == USHORT) {
			newvalue = ((u_short *)buf)[index];
		}
		if (newvalue < lastvalue) /* data wrapped around */
		{
		    gaplength = maxcount - lastvalue + newvalue;
		} else {
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
		real_gaplength = gaplength - 1;
		xferred += (runlength * 4);	/*Number of bytes transferred */
		if (real_gaplength > gaps_max_index) {
		    gaps_max_index = real_gaplength;
		} 
		if (runlength > blocks_max_index) {
		    blocks_max_index = runlength;
		}
		pgaps->blocks[runlength]++;
		pgaps->gaps[real_gaplength]++;

		if (fout)
		  {
		      fprintf (fout, "Block %8d -> %8d size %8d\n",
			       blockstart, lastvalue, runlength);
		      fprintf (fout, "Gap   %8d -> %8d size %8d\n",
			       lastvalue + 1, newvalue - 1, real_gaplength);
		  }

		blockstart = newvalue;

		runlength = 1;
		clock += gaplength + 1;

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
	if (runlength > blocks_max_index) {
	    blocks_max_index = runlength;
	}

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
    pgaps->gaps_largest_index = gaps_max_index;
    pgaps->blocks_largest_index = blocks_max_index;

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
chkgap_output_gap(CheckGapData *d, int cur_channel)
{
	int i;
	double t;
	int totalgaps = 0;
	GapHistory *pgaps = &d->gaps[cur_channel];
	int clock_speed = d->clock_speed;


	printf("Transferred:  %10f bytes\n", pgaps->t_xferred);
	printf("Clocks: %10f \n", pgaps->total_clocks);

	t = pgaps->total_clocks/clock_speed;

	printf("Bytes per second: %10f \n\n", (pgaps->t_xferred/t));

	/*Print block frequency result table*/
	for (i = 0; i < MAXFREQ; i++)
	{
	   if (pgaps->blocks[i])
		  printf("\t Block of: %d \t Frequency: %d\n", i, pgaps->blocks[i]);
	}

	/*Print gap frequency result table*/
	for (i = 0; i < MAXFREQ; i++)
	{
	   if (pgaps->gaps[i])
		  printf("\t Gap of: %d \t Frequency: %d\n", i, pgaps->gaps[i]);
	   totalgaps += pgaps->gaps[i];
	}
	printf("Total # of gaps: %d\n", totalgaps);

	return 0;
}



/*
 * command_error displays the command line description for each command
 */
void 
chkgap_usage()
{
	printf("usage: check_gap [options]\n") ;
	printf("  -u <unit>         - specifies edt board unit number\n") ;
	printf("  -F <clock speed>  - specifies the clock's speed\n") ;
	printf("  -l <loops size>   - specifies the # of loops to run\n") ;
	printf("  -n <numchannels>  - specifies the number of channels\n");
	printf("  -s <bufsize>      - specifies the buffer size to use\n");
	printf("  -S                - do byte and short swap if this is a Sun\n");
	printf("  -o <rawfilename>  - specifies filename of raw output file\n");
	printf("  -t <textfilename> - specifies filename of text output file\n");
	printf("  --ss1             - specifies that uut is 1 chan SS or GS\n");
	printf("  --ss4             - specifies that uut is 4 chan SS or GS\n");
	printf("  -h, --help        - this help message\n");
}


