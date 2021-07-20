/* #pragma ident "@(#)setdebug.c	1.138 06/19/09 EDT" */

/** 
 * @file setdebug.c
 *
 * Utility program to display register values, and set various driver
 * related options
 */


#include "edtinc.h"
#include "edt_trace.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define edt_swab32(x) \
    (\
     ((u_int)( \
         (((u_int)(x) & (u_int)0x000000ffUL) << 24) | \
         (((u_int)(x) & (u_int)0x0000ff00UL) <<  8) | \
         (((u_int)(x) & (u_int)0x00ff0000UL) >>  8) | \
         (((u_int)(x) & (u_int)0xff000000UL) >> 24) )) \
    )

EdtDev *edt_p;

u_int tracebuf[EDT_TRACESIZE];

int curcount;

void print_hex (u_char * buf, int count);
void print_ascii (char *buf);

typedef struct _dbg_level
{
    char *name;
    int dbg_mask;
    int dbg_level;
} EdtDebugLevel;

EdtDebugLevel dbg_states[] = {
    {"INIT", DBG_INIT},
    {"PNP", DBG_PNP},
    {"POWER", DBG_POWER},
    {"CREATE_CLOSE", DBG_CREATE_CLOSE},
    {"IOCTLS", DBG_IOCTLS},
    {"WRITE", DBG_WRITE},
    {"READ", DBG_READ},
    {"DPC", DBG_DPC},
    {"ISR", DBG_ISR},
    {"LOCKS", DBG_LOCKS},
    {"EVENTS", DBG_EVENTS},
    {"HW", DBG_HW},
    {"REG", DBG_REG},
    {"DMA", DBG_DMA},
    {"DMA_SETUP", DBG_DMA_SETUP},
    {"SERIAL", DBG_SERIAL},
    {"BOARD", DBG_BOARD},
    {"ALLOC", DBG_ALLOC},
    {"MEMMAP", DBG_MEMMAP},
    {"TIME", DBG_TIME},
    {"TIMEOUT", DBG_TIMEOUT},
    {NULL}
};


void
edt_dump_cache_state(EdtDev *edt_p)

{
	int i = 0;
        edt_ioctl (edt_p, EDTS_TEST_STATUS, &i);

}

    void
edt_get_dbg_state (EdtDev * edt_p, EdtDebugLevel * states)
{
    int i;
    int bit;

    u_int mask;

    for (i = 0; i < EDT_DEBUG_LEVELS; i++)
    {
        mask = i;
        edt_ioctl (edt_p, EDTG_DEBUG_MASK, &mask);

        for (bit = 0; states[bit].name; bit++)
        {
            if (mask & states[bit].dbg_mask)
                states[bit].dbg_level = i;
        }
    }
}

    void
edt_print_dbg_state (EdtDev * edt_p, char *edt_devname, int unit, int channel)
{
    int bit;
    int i;

    i = edt_get_debug (edt_p);

    printf ("\n%s%d.%d:  current debug %d type %d\n", edt_devname, unit,
            channel, i, edt_get_drivertype (edt_p));

    edt_get_dbg_state (edt_p, dbg_states);

    for (bit = 0; dbg_states[bit].name; bit++)
    {
        printf ("%-15s : %d\n", dbg_states[bit].name,
                dbg_states[bit].dbg_level);
    }

}

    void
edt_set_debug_mask (EdtDev * edt_p, unsigned int mask, int level)
{
    u_int fullmask = DBG_MASK (level, mask);

    printf ("Setting dbg_mask to fullmask mask %08x level %08x -> %08x\n", mask,
            level, fullmask);

    edt_ioctl (edt_p, EDTS_DEBUG_MASK, &fullmask);

}

    static void
usage (char *err)
{
    fprintf(stderr, "%s", err);
    fprintf(stderr, "usage: setdebug \n");
    fprintf(stderr, "\t -u <unit>    - unit number\n");
    fprintf(stderr, "\t -c channel   - channel number\n");
    fprintf(stderr, "\t -t timeout   - set read and write timeout to ARG milliseconds\n");
    fprintf(stderr, "\t -r           - report additional diagnostics\n");
    fprintf(stderr, "\t -v           - print version information for library and driver\n");
    fprintf(stderr, "\t -l           - loop forever printing results from edt_get_bytecount()\n");
    fprintf(stderr, "\t                 and edt_get_bufbytecount() showing progress of DMA buffers\n");
    fprintf(stderr, "\t -d level     - driver debugging: set driver debugging level (CAUTION: levels\n");
    fprintf(stderr, "\t                other than 0 may cause programs to malfunction or crash the system)\n");
    fprintf(stderr, "\t -g           - print the driver diagnostic trace buffer once\n");
    fprintf(stderr, "\t -G           - loop while printing the updated driver diagnostic trace buffer\n");
    fprintf(stderr, "\t -e           - driver debugging only: set board interrupt register (can \n");
    fprintf(stderr, "\t                crash the system) \n");
    fprintf(stderr, "\t -s           - driver debugging: get and print the driver DMA buf scatter-gather list\n");
    fprintf(stderr, "\t -b           - set the maximum number of ring buffers to argument \n");
    fprintf(stderr, "\t -q           - don't print the board main registers \n");
    fprintf(stderr, "\t -x           - driver debugging: set the driver scatter-gather list \n");
    fprintf(stderr, "\t                (can crash the system) \n");
    fprintf(stderr, "\t -k           - controls kernel buffering for DMA. -kq means turn off kernel buffers.\n");
    fprintf(stderr, "\t                -kc means do an intermediate copy (bounce buffer.) -km means\n");
    fprintf(stderr, "\t                use memory mapped buffers.\n");
    fprintf(stderr, "\t -V           - turn on FVAL done on pdv board.\n");
    fprintf(stderr, "\t -Tbits       - Turn on or off bits in the trace regs mask.\n");
    fprintf(stderr, "\t -f           - dump last bits into the fifo with ssdio\n");
    fprintf(stderr, "\t -m mask      - set the debug mask associated with the current debug value.\n");
    fprintf(stderr, "\t                -r shows the names of mask bits. \n");
    fprintf(stderr, "\t -o           - gets values for allocated DMA.\n");
    fprintf(stderr, "\t \n");

    exit(1);
}

/*
 * checks for a numeric argument and return; otherwise usage / bomb out
 */
int checkintarg(int argc, char *arg, char option)
{
    char errmsg[256];
    if ((argc < 1) || (arg[0] < '0') || (arg[0] > '9'))
    {
        sprintf(errmsg, "Error: option %c requires a numeric argument\n", option);
        usage(errmsg);
        exit(1);
    }
    else return atoi(arg);
}

    u_int
edt_get_dbg_mask_from_names (char *names)
{
    char namebuf[512];
    u_int val = 0;
    char *s;

    strcpy (namebuf, names);

    s = strtok (names, " ");

    while (s)
    {
        int bit;

        for (bit = 0; dbg_states[bit].name; bit++)
        {
            if (strcasecmp (dbg_states[bit].name, s) == 0)
            {
                val |= dbg_states[bit].dbg_mask;
                break;
            }
        }

        s = strtok (NULL, " ");

    }

    return val;

}

/*
 * Main module. NO_MAIN is typically only defined when compiling for vxworks; if you
 * want to use this code outside of a main module in any other OS, just copy the code
 * and modify it to work as a standalone subroutine, including adding parameters in
 * place of the command line arguments
 */
#ifdef NO_MAIN
#include "opt_util.h"
char *argument;
int option;
    int
setdebug (char *command_line)
#else
    int
main (int argc, char **argv)

#endif
{
    int i, unit = 0, report = 0, debug = -1, quiet = 0;
    int channel = 0;
    char unitstr[128];
    char edt_devname[128];
    int test_timestamp = 0;
    int get_trace = 0;
    int wait_trace = 0;
    int get_sglist = 0;
    int set_sglist = 0;
    int set_solaris_dma_mode = 0;
    int solaris_dma_mode = 0;
    int set_umem_lock_mode = 0;
    int umem_lock_mode = 0;
    int do_timeout = 0;
    int clock_fifo = 0;
    int do_loop = 0;
    u_int dbg_mask = 0;

    int mstimeout = -1;
    int sgbuf = -1;
    int version = 0;

    int process_isr = -1;
    int do_process_isr = 0;

    int multi_done = 0x1234;
    int check_openmask = 0;

    int use_kernel_buffers = -1;
    int do_kernel_buffers = 0;
    int use_persistent_buffers = 0;

    int max_buffers = 0;
    int set_fv_done = -1;

    int lock_on = 0;
    int lock_on_value = 0;

    u_int set_trace = 0;
    u_int trace_reg = 0;
    u_int trace_state = 0;
    u_int trace_regs_enable = 0;
    int enable_intr = -1;
    int check_cache = 0;



#ifdef NO_MAIN
    char **argv = 0;
    int argc = 0;
    opt_create_argv ("setdebug", command_line, &argc, &argv);
#endif

#ifdef P53B
    int soft_reset = 0;
    int hard_reset = 0;
    int set_coupled = 0;
    int direct_coupled = 0;
    int arg;

#endif

    sprintf(unitstr, "%s", EDT_INTERFACE);

    --argc;
    ++argv;
    while (argc && argv[0][0] == '-')
    {
        switch (argv[0][1])
        {
            case 'h':
            case 'H':
                usage("");
                break;

            case 'u':
                ++argv;
                --argc;
                if (argc < 1) 
                    usage("Error: option 'u' requires an argument\n");
                if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
                    sprintf(unitstr, "%s%s", EDT_INTERFACE, argv[0]);
                else strncpy(unitstr, argv[0], sizeof(unitstr) - 1);
                break;

            case 'c':
                ++argv;
                --argc;
                channel = checkintarg(argc, argv[0], 'c');
                break;

            case 't':
                ++argv;
                --argc;
                mstimeout = checkintarg(argc, argv[0], 't');
                do_timeout = 1;
                break;

            case 'i':
                ++argv;
                --argc;
                process_isr = checkintarg(argc, argv[0], 'i');
                do_process_isr = 1;
                break;

            case 'k':
                if (argc > 1)
                {
                    ++argv;
                    --argc ;
                    if (argc < 1)
                        usage("Error: option 'k' requires an supplemental argument (q, c, m, p or hex value)\n");
                    if (argv[0][0] == 'q')
                        use_kernel_buffers = -1;
                    else if (argv[0][0] == 'c')
                        use_kernel_buffers = EDT_COPY_KBUFS;
                    else if (argv[0][0] == 'm')
                        use_kernel_buffers = EDT_MMAP_KBUFS;
                    else
                        use_kernel_buffers = 0;
                    if (isdigit (argv[0][0]))
                        use_kernel_buffers = strtol (argv[0], 0, 0);

                    if (argv[0][0] && (argv[0][1] == 'p'))
                    {
                        use_persistent_buffers = 1;
                        use_kernel_buffers |= EDT_PERSISTENT_KBUFS;
                    }

                }
                do_kernel_buffers = 1;
                break;


            case 'r':
                report = 1;
                break;

            case 'v':
                version = 1;
                break;

            case 'T':
                set_trace = 1;
                if (argv[0][2] == '+')
                    trace_state = 1;
                else if (argv[0][2] == '-')
                    trace_state = 0;
                else
                    trace_regs_enable = 1;

                if (argc > 1)
                {
                    ++argv;
                    --argc;
                    trace_reg = strtol (argv[0], 0, 0);
                    if (!trace_regs_enable)
                    {
                        if (strlen (argv[0]) < 7 && trace_reg < 0x10000)
                        {
                            trace_reg |= 0x01010000;
                        }
                    }
                }

                break;

            case 'l':
                do_loop = 1;
                break;

            case 'L':
                ++argv;
                --argc;
                lock_on_value = checkintarg(argc, argv[0], 'L');
                lock_on = 1;
                break;

            case 'V':
                ++argv;
                --argc;
                set_fv_done = checkintarg(argc, argv[0], 'V');
                break;

            case 'd':
                if ((argc < 2) || (argv[1][0] == '-'))
                    debug = 0;
                else
                {
                    ++argv;
                    --argc;
                    debug = strtol (argv[0], 0, 0);
                }
                break;

            case 'g':
                get_trace = 1;
                break;

            case 'G':
                get_trace = 1;
                wait_trace = 1;
                break;

	    case 'M':
		check_cache = 1;
		break;


            case 'e':
                ++argv;
                --argc;
                if (argc < 1)
                    usage("Error: option 'e' requires a hex argument\n");
                enable_intr = strtol(argv[0],0,0);
                break;

#ifdef P53B
            case 'D':
                soft_reset = 1;
                break;

            case 'C':
                ++argv;
                --argc;
                direct_coupled = checkintarg(argc, argv[0], 'C');
                set_coupled = 1;
                break;

            case 'R':
                hard_reset = 1;
                break;
#else
            case 'f':
                ++argv;
                --argc;
                clock_fifo = checkintarg(argc, argv[0], 'f');
                break;

            case 's':
                ++argv;
                --argc;
                sgbuf = checkintarg(argc, argv[0], 's');
                get_sglist = 1;
                break;

            case 'S':
                ++argv;
                --argc;
                solaris_dma_mode = checkintarg(argc, argv[0], 'S');
                set_solaris_dma_mode = 1;
                break;

            case 'U':
                ++argv;
                --argc;
                umem_lock_mode = checkintarg(argc, argv[0], 'U');
                set_umem_lock_mode = 1;
                break;

            case 'b':
                ++argv;
                --argc;
                max_buffers = checkintarg(argc, argv[0], 'b');
                break;

            case 'm':
                while (argc > 1 && argv[1][0] != '-')
                {
                    ++argv;
                    --argc;
                    dbg_mask |= edt_get_dbg_mask_from_names (argv[0]);
                    printf ("Adding %s to mask %08x\n", argv[0], dbg_mask);
                }
                break;

            case 'o':
                check_openmask = 1;
                break;

            case 'q':
                quiet = 1;
                break;
            case 'x':
                set_sglist = 1;
                break;

#endif
            default:
#ifdef P53B
                puts(
                        "Usage: setdebug [-u unit_no] [-c chan] [-R] [-D] [-r] [-d N] [-i N]");
#else
                usage("unrecognized option\n");
                exit(0);
#endif
        }
        argc--;
        argv++;
    }

    if (version)
    {

        u_int v;

        edt_version_string version;

        edt_get_library_version(NULL, version, sizeof(version));
        v = edt_get_version_number();

        printf("\nLibrary version %s (%08x)\n",
                version,
                v);

    }

    unit = edt_parse_unit_channel(unitstr, edt_devname, EDT_INTERFACE, &channel);

    if ((edt_p = edt_open_channel (edt_devname, unit, channel)) == NULL)
    {
        char str[64];

        sprintf (str, "%s%d_%d", edt_devname, unit, channel);
        perror (str);
        exit (1);
    }

    if (check_cache)
	edt_dump_cache_state(edt_p);

    if (trace_regs_enable)
    {
        edt_trace_regs_enable (edt_p, trace_reg);
    }
    else if (set_trace)
    {
        edt_set_trace_regs (edt_p, trace_reg, trace_state);
    }

    if (check_openmask)
    {
        u_int tmpmask;
        edt_dma_info tmpinfo;
        tmpmask = edt_get_dma_info (edt_p, &tmpinfo);
        printf ("dma info mask %08x used %08x alloc %08x active %08x\n",
                tmpmask,
                tmpinfo.used_dma, tmpinfo.alloc_dma, tmpinfo.active_dma);
        exit (0);
    }

    if (lock_on)
    {

        edt_ioctl (edt_p, EDTS_TEST_LOCK_ON, &lock_on_value);

    }

    if (enable_intr != -1)
        edt_set_intr_mask(edt_p, enable_intr);

#ifdef P53B
    if (hard_reset)
    {
        arg = 1;
        if (edt_ioctl (edt_p, P53S_RESET, &arg) == -1)
            perror ("edt_ioctl(P53S_RESET)");
    }
    if (soft_reset)
    {
        arg = 0;
        if (edt_ioctl (edt_p, P53S_RESET, &arg) == -1)
            perror ("edt_ioctl(P53S_RESET)");
    }
    if (set_coupled)
    {
        if (direct_coupled)
            printf ("setting direct_coupled\n");
        else
            printf ("setting transformer_coupled\n");
        if (edt_ioctl (edt_p, P53S_COUPLED, &direct_coupled) == -1)
            perror ("edt_ioctl(P53S_DIRECT)");
    }
#endif /* P53B */


    if (do_timeout)
    {
        printf ("setting read timeout to %d ms\n", mstimeout);
        edt_set_rtimeout (edt_p, mstimeout);
        edt_set_wtimeout (edt_p, mstimeout);
    }

    if (do_process_isr)
    {
        edt_ioctl (edt_p, EDTS_PROCESS_ISR, &process_isr);
        printf ("Process ISR state = %d\n", process_isr);

    }

    if (multi_done != 0x1234)
    {

        if (multi_done < 0)
        {
            edt_ioctl (edt_p, EDTG_MULTI_DONE, &multi_done);
            printf ("multi done = %d\n", multi_done);
        }
        else
        {
            printf ("setting multi done = %d\n", multi_done);
            edt_ioctl (edt_p, EDTS_MULTI_DONE, &multi_done);
        }
    }

    if (do_kernel_buffers)
    {
        int old_state;

        if (use_kernel_buffers != -1)
        {
            printf ("Setting to %d\n", use_kernel_buffers);
            old_state = edt_set_kernel_buffers (edt_p, use_kernel_buffers);
            printf ("old_state = %d\n", old_state);
        }

        old_state = edt_get_kernel_buffers (edt_p);
        use_persistent_buffers = edt_get_persistent_buffers (edt_p);

        printf ("Use Kernel buffers is %s %s\n", (old_state == 0) ?
                "off" : (old_state == 2) ? "mmapped" : "copy",
                (use_persistent_buffers) ? "(persistent)" : "");

    }

    if (max_buffers > 0)
    {
        edt_set_max_buffers (edt_p, max_buffers);
        printf ("Max Buffers = %d\n", edt_get_max_buffers (edt_p));

    }

    if (version)
    {

        edt_version_string version;
        edt_version_string build;

        edt_get_driver_version (edt_p, version, sizeof (version));
        edt_get_driver_buildid (edt_p, build, sizeof (version));

        printf("\n%s%d:  Driver version %s \n       Build %s\n", 
                edt_devname, 
                unit, 
                version,
                build);

    }

    if (report)
    {
        edt_print_dbg_state (edt_p, edt_devname, unit, channel);
    }
    if (test_timestamp)
    {
        for (i = 0; i < 4; i++)
        {
            edt_ref_tmstamp (edt_p, i);
            edt_msleep (1000);
        }
    }

    if (set_fv_done != -1)
    {
        int i;
        i = (set_fv_done) ? 1 : 0;
        edt_ioctl (edt_p, EDTS_FVAL_DONE, &i);
        if (set_fv_done == 1)
            edt_reg_and (edt_p, PDV_UTIL3, ~PDV_TRIGINT);
    }


    if (debug != -1 || report)
    {

        if (debug != -1)
        {
            if (dbg_mask != 0)
            {
                edt_set_debug_mask (edt_p, dbg_mask, debug);
            }
            else
                (void) edt_set_debug (edt_p, debug);
            if (report)
            {
                edt_print_dbg_state (edt_p, edt_devname, unit, channel);
            }
        }

        if (!quiet)
        {
            printf("%s %d %d registers:\n", edt_devname, unit, channel);
            edt_dump_registers (edt_p, debug);
        }

        printf ("xfer %d bytes\n", edt_get_bytecount (edt_p));

        {
            u_int mapsize;

            mapsize = edt_get_mappable_size (edt_p, 1);

            if (mapsize != 0)
            {
                printf ("Second memory space size = 0x%x bytes\n", mapsize);

            }
        }
    }

    if (get_trace)
    {
        edt_trace (edt_p, stdout, FALSE, (wait_trace) ? 0 : 1, NULL, 0);

    }
    if (get_sglist)
    {
        edt_buf sg_args;
        u_int todo_size;
        u_int sg_size;
        u_int *todo_list;
        u_int *sg_list;
        u_int *log_list;
        u_int sg_virtual;
        u_int sg_physical;
        u_int todo_virtual;
        u_int first_sg;
        u_int *ptr;
        u_int **ptrptr;
        u_int *tptr;
        u_int lsize;
        u_int pagesize = 0x1000;
        u_int sg;
        u_int ii;

        sg_args.desc = EDT_SGLIST_SIZE;
        sg_args.value = sgbuf;
        edt_ioctl (edt_p, EDTG_SGINFO, &sg_args);
        sg_size = (u_int) sg_args.value;

        sg_args.desc = EDT_SGLIST_VIRTUAL;
        sg_args.value = sgbuf;
        edt_ioctl (edt_p, EDTG_SGINFO, &sg_args);
        sg_virtual = (u_int) sg_args.value;

        sg_args.desc = EDT_SGLIST_PHYSICAL;
        sg_args.value = sgbuf;
        edt_ioctl (edt_p, EDTG_SGINFO, &sg_args);
        sg_physical = (u_int) sg_args.value;

        sg_args.desc = EDT_SGTODO_SIZE;
        sg_args.value = sgbuf;
        edt_ioctl (edt_p, EDTG_SGINFO, &sg_args);
        todo_size = (u_int) sg_args.value;

        sg_args.desc = EDT_SGTODO_VIRTUAL;
        sg_args.value = sgbuf;
        edt_ioctl (edt_p, EDTG_SGINFO, &sg_args);
        todo_virtual = (u_int) sg_args.value;

        sg_args.desc = EDT_SGTODO_FIRST_SG;
        sg_args.value = sgbuf;
        edt_ioctl (edt_p, EDTG_SGINFO, &sg_args);
        first_sg = (u_int) sg_args.value;

        printf ("todo size 0x%x addr  %x\n", todo_size, todo_virtual);
        printf ("sg size 0x%x viraddr %x physaddr %x first_sg %x\n",
                sg_size, sg_virtual, sg_physical, first_sg);


        todo_list = (u_int *) malloc (todo_size);
        sg_list = (u_int *) malloc (sg_size);

        if (todo_size)
        {
            printf ("return to read todo list: ");
            getchar ();
            printf ("todo list %p\n", todo_list);
            edt_ioctl (edt_p, EDTG_SGTODO, todo_list);
            ptr = todo_list;
            printf ("todo_list:\n");
            printf ("todo size %x\n", todo_size);
            printf ("return to continue");
            getchar ();
            for (ii = 0; ii < todo_size / 8; ii++)
            {
                printf ("%08x %08x\n", *ptr, *(ptr + 1));
                ptr += 2;
            }
        }
        if (sg_size)
        {
            buf_args sg_getargs;
            sg_getargs.addr = (uint64_t) ((unsigned long) sg_list);
            sg_getargs.size = sg_size;
            sg_getargs.index = 0;
            printf ("return to read sg list: %x at %p", sg_size, sg_list);
            getchar ();
            edt_ioctl (edt_p, EDTG_SGLIST, &sg_getargs);
#ifdef __APPLE_
            edt_ioctl (edt_p, EDTG_SGTODO, todo_list);
            for (sg=0; sg<todo_size; sg++);
            {
                ptrptr = sg_list;
                ptr = ptrptr[sg];
                tptr = todo_list;
                
                lsize = *(tptr + 2);                

                if (edt_little_endian())
                    for (ii = 0; ii < lsize / 8; ii++)
                    {
                        printf ("%08x %08x\n", *ptr, *(ptr + 1));
                        ptr += 2;
                    }
                else
                    for (ii = 0; ii < lsize / 8; ii++)
                    {
                        printf ("%08x %08x\n", edt_swab32 (*ptr),
                               edt_swab32 (*(ptr + 1)));
                        ptr += 2;
                    }
                tptr += 2; 
            }
#else
            ptr = sg_list;
            printf ("sg list:\n");
            if (edt_little_endian ())
                for (ii = 0; ii < sg_size / 8; ii++)
                {
                    printf ("%08x %08x\n", *ptr, *(ptr + 1));
                    ptr += 2;
                }
            else
                for (ii = 0; ii < sg_size / 8; ii++)
                {
                    printf ("%08x %08x\n", edt_swab32 (*ptr),
                            edt_swab32 (*(ptr + 1)));
                    ptr += 2;
                }
#endif
        }
        if (set_sglist)
        {

            printf ("return to test sg list: ");
            getchar ();
            log_list = (u_int *) malloc (1024 * 8);
            for (i = 7; i >= 0; i--)
            {
                log_list[i * 2] = 1024 * (1024 / 8) * i;
                log_list[i * 2 + 1] = 1024 * (1024 / 8);
            }
            edt_set_sglist (edt_p, 0, log_list, 8);
        }
    }
    if (clock_fifo)
    {
        /* tmp test code for ssdio(1) and pcd(2) */
        for (;;)
        {
            if (clock_fifo == 1)
            {
                u_int stat;
                u_int tmp;
                u_int bytenum;

                stat = edt_reg_read (edt_p, PCD_STAT);
                bytenum = (stat & (SSDIO_BYTECNT_MSK)) >> SSDIO_BYTECNT_SHFT;
                printf ("stat %02x next strobe ", stat);
                if (stat & SSDIO_LAST_BIT)
                    printf ("will fill byte to start filling %d\n", bytenum);
                else
                    printf ("filling byte %d\n", bytenum);
                tmp = edt_reg_read (edt_p, PCD_FUNCT);
                printf ("return to strobe: ");
                getchar ();
                edt_reg_write (edt_p, PCD_FUNCT, tmp & ~SSDIO_STROBE);
                edt_reg_write (edt_p, PCD_FUNCT, tmp | SSDIO_STROBE);
            }
            else
            {
                u_int cfg;
                u_int cnt;

                cnt = edt_get_bytecount (edt_p);
                cfg = edt_reg_read (edt_p, EDT_DMA_CFG);
                printf ("0x%x tranferred  fifo %d cfg %08x\n",
                        cnt, (cfg & EDT_FIFO_CNT) >> EDT_FIFO_SHIFT, cfg);
                printf ("return to clock fifo: ");
                getchar ();
                edt_reg_write (edt_p, EDT_DMA_INTCFG, cfg | EDT_WRITE_STROBE);
            }
        }
    }
    if (do_loop)
    {
        u_int bcount;
        u_int buf;

        for (;;)
        {
            edt_msleep (1000);
            bcount = edt_get_bytecount (edt_p);
            printf ("xfer %d (0x%x) bytes\n", bcount, bcount);
            bcount = edt_get_bufbytecount (edt_p, &buf);
            printf ("xfer %d (0x%x) bytes buf %x\n", bcount, bcount, buf);
        }
    }
    if (set_solaris_dma_mode)
    {
        edt_ioctl (edt_p, EDTS_SOLARIS_DMA_MODE, &solaris_dma_mode);
    }
    if (set_umem_lock_mode)
    {
        edt_ioctl (edt_p, EDTS_UMEM_LOCK, &umem_lock_mode);
    }


#ifndef NO_MAIN
    exit (0);
#endif
    return (0);
}


    void
print_hex (u_char * buf, int count)
{
    int i;

    printf ("<");
    for (i = 0; i < count; i++)
        printf ("%02x%s", buf[i], i == count - 1 ? ">" : " ");
    printf ("\n");
}

    void
print_ascii (char *buf)
{
    unsigned int i;

    printf ("<");
    for (i = 0; i < strlen (buf); i++)
    {
        if (buf[i] >= ' ' && buf[i] <= '~')
            printf ("%c", buf[i]);
        else if (buf[i] == '\t')
            printf ("\\t");
        else if (buf[i] == '\n')
            printf ("\\n");
        else if (buf[i] == '\r')
            printf ("\\r");
    }
    printf (">\n");
}
