/**
 * @file
 * Library routines for parsing parsing c-link logic analyzer output; depends on
 * EDT c-link logic analyzer FPGA loaded into the board (for example camlkla_xx.bit)
 */

#include <stdio.h>
#include <string.h>

#include "edtinc.h"

#include "cl_logic_lib.h"

#if defined(__sun) || defined(__linux__) || defined(__APPLE__)
#include <sys/wait.h>
#endif

#include "pciload.h"



#define FLAGS(v, testmask) (v & testmask)
#define CLOCKS(v) (((v) & CLOCK_MASK)+1)

#define FV(v) (v & FV_MASK)
#define DV(v) (v & DV_MASK)
#define LV(v) (v & LV_MASK)



static int default_numbufs = 8;
static int default_bufsize = CL_LOGIC_DEFAULT_BUFSIZE;
static double default_pixel_clock = 20000000; 



static int default_testmask = FV_MASK | LV_MASK;
static int default_timeout = 0;


/*********************************************
*
*
*
*********************************************/

void
cl_logic_summary_init(ClLogicSummary * cls_p, 
		      int testmask, 
		      int numbufs, 
		      int bufsize, 
		      int timeout,
		      double pixel_clock)

{
    memset(cls_p, 0, sizeof(ClLogicSummary));

    if (testmask)
	cls_p->testmask = testmask;
    else
	cls_p->testmask = default_testmask;
 
    if (numbufs)
	cls_p->numbufs = numbufs;
    else
	cls_p->numbufs = default_numbufs;

    if (bufsize)
	cls_p->bufsize = bufsize;
    else
	cls_p->bufsize = default_bufsize;

    if (timeout)
	cls_p->timeout = timeout;
    else
	cls_p->timeout = default_timeout;

    if (pixel_clock)
	cls_p->pixel_clock = pixel_clock;
    else
	cls_p->pixel_clock = default_pixel_clock;

}

cl_logic_stat_clear(ClLogicStat *clp)

{
    memset(clp, 0, sizeof(ClLogicStat));

}

/*********************************************
*
*
*
*********************************************/

void
cl_logic_stat_add(ClLogicStat * clp, int value)

{

    if (clp->n > 0)
    {
	if (clp->high < value)
	    clp->high = value;
	if (clp->low > value)
	    clp->low = value;

    }
    else
    {
	clp->high = clp->low = value;

    }

    clp->sum += value;
    clp->n++;

    clp->mean = (int) (clp->sum / clp->n);

}



void
cl_logic_stat_print(char *label, ClLogicStat * clp, double clockspeed, int verbose)

{
    if (verbose)
	printf("%-30s: %8d (%8d - %-8d) %8d\n", label,
	clp->mean, clp->low, clp->high, clp->n);
    else
    {
	printf("%-30s: %8d clocks %10g ms", label,
	    clp->mean, (clp->mean * 1000.0) / clockspeed );

	if (clp->low != clp->high)
	    printf(" (%8d - %8d)\n", clp->low, clp->high);
	else
	    printf("\n");
    }

}

int
cl_logic_stats_neq(ClLogicStat * clp1, ClLogicStat * clp2)

{
    return (clp1->high != clp2->high ||
	clp1->low != clp2->low ||
	clp1->mean != clp2->mean ||
	clp1->n != clp2->n);
}


u_int
edt_read_pci_config(EdtDev * edt_p, int addr)
{
    int     ret;
    edt_buf buf;

    buf.desc = addr;
    if ((addr < 0) || (addr > 0x3c))
    {
	printf("pr_cfg: addr out of range\n");
	return (0);
    }
    ret = edt_ioctl(edt_p, EDTG_CONFIG, &buf);
    if (ret < 0)
    {
	perror("EDTG_CONFIG");
    }
    return (u_int) (buf.value);
}

void
edt_write_pci_config(EdtDev * edt_p, int addr, int value)
{
    int     ret;
    edt_buf buf;

    buf.desc = addr;
    if ((addr < 0) || (addr > 0x3c))
    {
	printf("pw_cfg: addr out of range\n");
	return;
    }
    buf.value = value;
    ret = edt_ioctl(edt_p, EDTS_CONFIG, &buf);
    if (ret < 0)
    {
	perror("EDTS_CONFIG");
    }
}

void 
edt_reboot_pci(EdtDev * edt_p, int verbose)

{

    int     addr, data;
    int     buf[0x40];
    int     old, copy, new_one;
    FILE   *fd2;

    if ((fd2 = fopen("./pdb_reboot.cfg", "wb")) == NULL)
    {
	fprintf(stderr, "cfgr: Couldn't write to ./pdb_reboot.cfg\n");
	return;

    }

    for (addr = 0; addr <= 0x3c; addr += 4)
    {
	data = edt_read_pci_config(edt_p, addr);
	buf[addr] = data;
	putc(data, fd2);
	putc(data >> 8, fd2);
	putc(data >> 16, fd2);
	putc(data >> 24, fd2);
	/* printf("%02x:  %08x\n", addr, data); */
    }
    fclose(fd2);

    printf("Wrote config space state out to ./pdb_reboot.cfg\n");

    edt_reg_write(edt_p, 0x01000085, 0x40);

    edt_msleep(500);

    if (verbose)
	printf("	 old	   copy	     new\n");

    for (addr = 0; addr <= 0x3c; addr += 4)
    {
	old = edt_read_pci_config(edt_p, addr);
	copy = buf[addr];
	edt_write_pci_config(edt_p, addr, copy);

	new_one = edt_read_pci_config(edt_p, addr);

	if (verbose)
	{
	    printf("%02x:  %08x  %08x  %08x	 ", addr, old, copy, new_one);

	    if (copy != new_one)
		printf("ERROR\n");
	    else if (old != new_one)
		printf("changed\n");
	    else
		printf("\n");

	}

    }

    printf("PCI firmware reconfigured...\n");

    edt_msleep(2000);

}

/*********************************************
*
*
*
*********************************************/

size_t
get_buffer_file(unsigned short *buffer, int size, FILE * f)

{
    size_t  r = fread(buffer, 2, size, f);

    return r;
}

/*********************************************
*
*
*
*********************************************/


size_t
get_buffer_card(unsigned short *buffer, int size, PdvDev * f)

{
    unsigned short *b;

    int     done = f->donecount;

    b = (unsigned short *) edt_wait_for_buffers(f, 1);

    memcpy(buffer, b, size);

    edt_start_buffers(f, 1);

    return (size_t) size / 2;

}

/*********************************************
*
*
*
*********************************************/


size_t
get_next_logic_buffer(unsigned short *buffer, int size, PdvDev * pdv_p, FILE * f)

{
    if (pdv_p)
	return get_buffer_card(buffer, size, pdv_p);
    else
	return get_buffer_file(buffer, size, f);

}

/* increase frame storage 4096 at a time */
#define CL_LOGIC_FRAME_GRAN 256

void
cl_logic_summary_add_frame(ClLogicSummary *clp, int width, int height, int hblank, int vblank)

{
    if (clp->nframes + 1 > clp->nframesallocated)
    {
	int newsize = clp->nframesallocated + CL_LOGIC_FRAME_GRAN;

	if (clp->nframesallocated == 0)
	{

	    clp->frames = (ClFrameSummary *) calloc(newsize, sizeof(ClFrameSummary));

	}
	else
	{
	    

	    clp->frames = (ClFrameSummary *) realloc(clp->frames, newsize * sizeof(ClFrameSummary));

	}

	clp->nframesallocated = newsize;

    }

    clp->frames[clp->nframes].width = width;
    clp->frames[clp->nframes].height = height;
    clp->frames[clp->nframes].line_blank = hblank;
    clp->frames[clp->nframes].frame_blank = vblank;

    clp->nframes ++;
    
}

int
pdv_cl_logic_sample(PdvDev *pdv_p, 
		    FILE *f, 
		    ClLogicSummary *clsp, 
		    int verbose, 
		    int quiet, 
		    int load, 
		    char *outfilename,
		    unsigned int loops,
		    int timeout,
		    int max_timeouts)

{
    FILE *outfile = NULL;
    size_t  r, offset;
    int     i;
    int     clocks = 0;
    int     lastflags = 0;
    int     lastval = 0;
    int     start_h_gap;
    int     end_h_gap;

    int frame_gap = 0;
    int lastframegap = 0;

    int     lines = 0;
    int     inframe = 0;
    int     frameclocks = 0;
    int     lineclocks = 0;
    int     fv_state = -1; /* # of finished frames */

    unsigned short * buffer;
    int unit = 0;
    char oldname[MAX_STRING+1];
    char *trunc;
    u_short stat;
    int vb = 0;
    int promcode;

    int last_width = 1;
    int last_hblank = 0;
    int ret = 0;
    

    if (pdv_p)
    {
	unit = pdv_p->unit_no;

	promcode = edt_flash_prom_detect(pdv_p, &stat);
	edt_flash_get_fname(pdv_p, oldname);

	/* truncate anything including/after a '-' in the name */
	if ((trunc = strchr(oldname,  '-')) != NULL)
		*trunc = '\0';

	if (!quiet)
	    printf("Current firmware is %s\n", oldname);

	if (load || (strncmp(oldname,"camlkla", 7) != 0))
	{
	    char    cmd[80];
	    int retval;

	    FILE *f = fopen("pciload.y","w");
		if (f == NULL) {
			edt_msg_perror(EDTLIB_MSG_FATAL, "Couldn't open file pciload.y for writing");
			return -1;
		} 
	    fprintf(f,"\n");
	    fclose(f);
	    if (!quiet)
	    {
		printf("\n*********************************\n");

		printf("Loading PCI firmware camlkla - this takes about 20 sec\n");
	    }

	    sprintf(cmd, "pciload -u %d -q camlkla < pciload.y > pciload.out", unit);
	    retval = system(cmd);

#ifdef __sun
	    if (WEXITSTATUS(retval) != 0) {
#else
	    if (retval != 0) {
#endif
		edt_msg(EDTLIB_MSG_FATAL, "ERROR: 'pciload camlkla' failed. Run it with -v for more info.\n");
		return -1;
	    } 

	    edt_reboot_pci(pdv_p, verbose); 
	    edt_reboot_pci(pdv_p, verbose); /* why twice? */
	    if (!quiet)
		printf("*********************************\n");


	}

	
	edt_set_rtimeout(pdv_p, timeout);

	if (outfilename && outfilename[0])
	{
	    outfile = fopen(outfilename, "wb");
	}

	if (pdv_p)
	{

	    
	    edt_reg_write(pdv_p, PDV_CMD, 0x80);

	    edt_msleep(50);

	    clsp->pixel_clock = edt_intfc_read(pdv_p, 4) |
		(edt_intfc_read(pdv_p, 5) << 8) |
		(edt_intfc_read(pdv_p, 6) << 16);

	    clsp->pixel_clock *= 100;

	    /* set the mask in hardware */
	    edt_reg_write(pdv_p, PDV_CFG, (~clsp->testmask >> 12) & 0xf);

	    edt_reg_write(pdv_p, PDV_CMD, 2);
	}

	edt_configure_ring_buffers(pdv_p, clsp->bufsize, clsp->numbufs, EDT_READ, NULL);

	edt_start_buffers(pdv_p, clsp->numbufs);
    }

    buffer = (unsigned short *) edt_alloc(2 * clsp->bufsize);

    offset = 0;

    if (!quiet)
    {
	printf("\n*********************************\n");
	printf("Sampling transition data from the camera\n");
	printf("*********************************\n");
    }

    while (clsp->nframes < (int) loops && 
	((r = get_next_logic_buffer(buffer, clsp->bufsize, pdv_p, f)) > 0))
    {
	if (pdv_p)
	{
	    if ((max_timeouts != 0) && (edt_timeouts(pdv_p) > max_timeouts))
	    {
		break;
	    }
	}
	if (outfile)
	    fwrite(buffer,clsp->bufsize,1,outfile);

	for (i = 0; i < (int) r; i++)
	{

	    if (lastflags != FLAGS(buffer[i], clsp->testmask))
	    {
		if (FV(lastflags) != FV(buffer[i]))
		{
		    if (FV(buffer[i]))
		    {
			frame_gap = clocks;

			/* start frame valid */
			if (LV(buffer[i]))
			{
			    start_h_gap = 0;
			}
			else
			{
			    start_h_gap = CLOCKS(buffer[i]);
			}

			if (fv_state > 1)
			{

			    cl_logic_stat_add(&clsp->start_hblank, start_h_gap);
			    if (clocks)
			    {
				cl_logic_stat_add(&clsp->frame_gap, clocks);
				lastframegap = clocks;
			    }

			}

			inframe = 1;
		    }
		    else
		    {
			/* end frame valid */
			if (LV(lastflags))
			{
			    end_h_gap = 0;
			}
			else
			{
			    end_h_gap = CLOCKS(lastval);
			}

			if (fv_state > 1)
			{
			    cl_logic_stat_add(&clsp->end_hblank, end_h_gap);

			    cl_logic_stat_add(&clsp->height, lines);

			    cl_logic_stat_add(&clsp->frameclocks, frameclocks);
			    cl_logic_stat_add(&clsp->totalframeclocks, frameclocks + lastframegap);
			
			    cl_logic_summary_add_frame(clsp, last_width, lines, last_hblank, frame_gap/last_width);

			}

			if (verbose && fv_state > 1)
			    printf("%5d: F gap = %6d frame = %d total = %d width = %d height = %6d hblank = %d\n", 
			    clsp->frame_gap.n, frame_gap, frameclocks, frame_gap + frameclocks, last_width, lines,  
			    clsp->hblank_frame.mean);


			lines = 0;
			frameclocks = 0;
			inframe = 0;
			cl_logic_stat_clear(&clsp->hblank_frame);
			fv_state++;


		    }
		} /* end toggle fv */

		if (inframe && fv_state > 1)
		{
		    if (LV(buffer[i]))
		    {
			lineclocks = CLOCKS(buffer[i]);
			if (lines > 0)
			{
			    cl_logic_stat_add(&clsp->hblank, CLOCKS(lastval));
			    cl_logic_stat_add(&clsp->hblank_frame, CLOCKS(lastval));
			    cl_logic_stat_add(&clsp->totallineclocks, CLOCKS(lastval) + lineclocks);
			}

			last_hblank = CLOCKS(lastval);
			last_width = lineclocks;

			cl_logic_stat_add(&clsp->width, lineclocks);
			cl_logic_stat_add(&clsp->line_stats[lines], lineclocks);

			lines++;
		    }
		}

		lastflags = FLAGS(buffer[i],clsp->testmask);
		lastval = buffer[i];

		if (inframe)
		    clocks = 0;

	    }
	    clocks += CLOCKS(buffer[i]);
	    if (inframe)
		frameclocks += clocks;
	    if (clsp->height.n >= loops)
	    {

		break;

	    }
	}

	offset += r;
    }


    if (outfile)
	fclose(outfile);

    if (pdv_p)
    {
	if (load)
	{
	    char    cmd[80];

	    if (!quiet)
	    {
		printf("\n*********************************\n");

		printf("Loading PCI firmware %s - this takes about 20 sec\n", oldname);
	    }
	    sprintf(cmd, "pciload -u %d -q %s < pciload.y > pciload.out", unit, oldname);
	    ret = system(cmd);

	    edt_reboot_pci(pdv_p, verbose);
	    edt_reboot_pci(pdv_p, verbose);
	    if (!quiet)
		printf("*********************************\n");

	}

    }

    return 0;

}
