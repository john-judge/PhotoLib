/*
 * pdvspeed
 *
 * check bus throughput using a PCI/PCIe DV c-link  or VisionLink board,
 * using the camera link channel 2 simulator
 */
#include "edtinc.h"

void usage(char *progname);
void intro(char *progname);

#ifdef _NT_
#define TAKE "take.exe"
#define INITCAM "initcam.exe"
#define CAT "type"
#define SLASH "\\"
#else
#define TAKE "take"
#define INITCAM "initcam"
#define CAT "cat"
#define SLASH "/"
#endif

#define BADSPEED    "\
Reeeeally slow or 0 -- this indicates a problem. check for misconfiguration,\n\
bad PCI slot, etc.\n"

#define SLOWPCI  "\
This indicates a slow bus, or a heavily loaded 33MHz PCI bus. Data loss\n\
(timeouts) are likely with all but the slowest cameras; remove other\n\
bandwidth \"hogs\" or move the board (or upgrade to a system with) a 66 MHz\n\
or faster PCI bus.\n"

#define SLOWPCIE  "\
This indicates a slow or heavily-loaded PCIe bus, or the board is in a slot that\n\
has (or negoatiated to) fewer PCI Express lanes than the board is designed for.\n\
It may be fast enough for some cameras, but if you have a fast camera or need to\n\
optimize for speed for any other reason, try shutting down other processes, remo-\n\
ving other bandwidth \"hogs\", or moving the board to a different slot or system.\n"

#define NOMINALPCI "\
Indicates the board has most of the bandwidth on a 66MHz or faster PCI\n\
bus and is operating at the nominal speed for a board of this type\n"

#define NOMINALALL "\
For PCI boards, indicates the board has most of the bandwidth on a 66MHz or\n\
faster PCI bus or and is operating at the nominal speed for a board of this type.\n\
Express boards, indicates a slow or heavily-loaded PCIe bus, or the board is in\n\
slot that has (or negoatiated to) fewer PCI Express lanes than the board is de-\n\
signed for. It may be fast enough for some cameras, but if you have a fast camera\n\
or need to optimize for speed for any other reason, try shutting down other proc-\n\
esses, removing other bandwidth \"hogs\", or moving the board to a different slot\n\
or system.\n"

#define NOMINALPCIE  "\
Indicates the board has adequate PCI Express bandwidth and is operating at\n\
the nominal speed for a board of this type\n"

#define FASTPCI  "\
This seems too fast for a board of this type. If you are experiencing problems with\n\
the board, contact EDT (provide this output).\n"

#define FASTPCIE "\
This seems too fast for a board of this type. If you are experiencing problems with\n\
the board, contact EDT (provide this output).\n"

typedef struct {
    int id;
    int slow;
    int nominal;
    int fast;
    char slow_msg[1024];
    char nominal_msg[1024];
    char fast_msg[1024];
} speedstruct ;

speedstruct speed_table[32] = 
{
    { PDV_ID,         10000000,  45000000,  120000000, SLOWPCI,  NOMINALPCI,  FASTPCI},
    { PDVK_ID,        10000000,  45000000,  120000000, SLOWPCI,  NOMINALPCI,  FASTPCI},
    { PDV44_ID,       10000000,  45000000,  120000000, SLOWPCI,  NOMINALPCI,  FASTPCI},
    { PDVA_ID,        10000000,  45000000,  230000000, SLOWPCI,  NOMINALPCI,  FASTPCI},
    { PDVA16_ID,      10000000,  45000000,  230000000, SLOWPCI,  NOMINALPCI,  FASTPCI},
    { PDVCL_ID,       10000000,  95000000,  230000000, SLOWPCI,  NOMINALPCI,  FASTPCI},
    { PDVFOX_ID,      10000000,  95000000,  230000000, SLOWPCI,  NOMINALPCI,  FASTPCI},
    { PE4DVCL_ID,     50000000, 180000000,  700000000, SLOWPCIE, NOMINALPCIE, FASTPCIE},
    { PE4DVAFOX_ID,   50000000, 250000000,  700000000, SLOWPCIE, NOMINALPCIE, FASTPCIE},
    { PE8DVFOX_ID,    50000000, 800000000, 1200000000, SLOWPCIE, NOMINALPCIE, FASTPCIE},
    { PE8DVAFOX_ID,   50000000, 800000000, 1200000000, SLOWPCIE, NOMINALPCIE, FASTPCIE},
    { PE8DVCL_ID,     50000000, 800000000, 1200000000, SLOWPCIE, NOMINALPCIE, FASTPCIE},
    { PE1DVVL_ID,     20000000, 350000000,  600000000, SLOWPCIE, NOMINALPCIE, FASTPCIE},
    { PE4DVVL_ID,     50000000, 800000000, 1200000000, SLOWPCIE, NOMINALPCIE, FASTPCIE},
    { PE4DVVLFOX_ID,  50000000, 800000000, 1200000000, SLOWPCIE, NOMINALPCIE, FASTPCIE},
    { 0,              10000000, 800000000, 1200000000, SLOWPCI,  NOMINALALL,  FASTPCIE}
};
    static int
grepit(char *buf, char *pat)
{
    int     i;

    for (i = 0; i < (int) strlen(buf); i++)
    {
        if (buf[i] == pat[0])
        {
            if (strncmp(&buf[i], pat, strlen(pat)) == 0)
                return 1;
        }
    }
    return 0;
}

main(int argc, char **argv)
{
    int i, idx;
    double fake_speed = -1.0; /* for debugging only */
    int skip_intro = 0;
    int unit=0;
    int devid;
    double bps;
    char progname[64];
    char edt_devname[64];
    char xilinx_name[128];
    char errstr[320];
    char cmdstr[128];
    char s[128];
    FILE *fp;
    PdvDev *pdv_p;

    strcpy(progname, argv[0]);
    puts("");

    /*
     * process command line arguments
     */
    --argc;
    ++argv;
    while (argc && ((argv[0][0] == '-') || (argv[0][0] == '/')))
    {
        switch (argv[0][1])
        {
            case 'u':		/* device unit number */
                ++argv;
                --argc;
                if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
                    unit = atoi(argv[0]);
                else
                    strncpy(edt_devname, argv[0], sizeof(edt_devname) - 1);
                break;

            case 's':		/* skip introductory message and pause for ENTER */
                skip_intro = 1;
                break;

            case '-':
                if (strcmp(argv[0], "--help") == 0) {
                    usage(progname);
                    exit(0);
                } else {
                    fprintf(stderr, "unknown option: %s\n", argv[0]);
                    usage(progname);
                    exit(1);
                }
                break;
            case 'f':
                ++argv;
                --argc;
                if ((argv[0][0] >= '0') && (argv[0][0] <= '9'))
                    fake_speed = atoi(argv[0]);
                else
                    strncpy(edt_devname, argv[0], sizeof(edt_devname) - 1);
                break;

            default:
                fprintf(stderr, "unknown flag -'%c'\n", argv[0][1]);
            case '?':
            case 'h':
                usage(progname);
                exit(0);
        }
        argc--;
        argv++;
    }

    if ((edt_access(TAKE, 0) != 0)
            ||  (edt_access(INITCAM, 0) != 0))
    {
        sprintf(errstr, "\nERROR: Couldn't find required executables %s and/or %s. Make sure these\nprograms are both in the current directory and try again\n\n", TAKE, INITCAM);
        edt_msg(EDTAPP_MSG_FATAL, errstr);
        exit (1);
    }

    if (!skip_intro)
    {
        intro(progname);
        printf("Press ENTER to continue > ");
        fflush(stdout);
        fgets(s, 127, stdin);
    }

    /*
     * open the device, test for proper type
     */
    strcpy(edt_devname, EDT_INTERFACE);
    if ((pdv_p = pdv_open(edt_devname, unit)) == NULL)
    {
        sprintf(errstr, "pdv_open(%s%d)\n", edt_devname, unit);
        pdv_perror(errstr);
        exit (1);
    }

    if (!edt_is_dvcl(pdv_p)) /* not pdv_is_cameralink since that includes FOX */
    {
        sprintf(errstr, "exiting cl_speed (does not support this device type)\n");
        edt_msg(EDTAPP_MSG_FATAL, errstr);
        pdv_close(pdv_p);
        exit (1);
    }

    edt_flash_get_fname_auto(pdv_p, xilinx_name);
    /* printf("xilinx_name %s\n", xilinx_name); */
    if ((strcmp(xilinx_name, "pdvcamlk") != 0)
            && (strcmp(xilinx_name, "visionlinkf1") != 0)
            && (strcmp(xilinx_name, "visionlinkf4") != 0)
            && (strcmp(xilinx_name, "pdvcamlk2") != 0)
            && (strcmp(xilinx_name, "pedvcamlk32") != 0)
            && (strcmp(xilinx_name, "pe8dvcamlk") != 0)
            && (strcmp(xilinx_name, "pe4dvacamlk") != 0)
            && (strcmp(xilinx_name, "pe8dvacamlk") != 0)
            && (strcmp(xilinx_name, "pe8dvcamlk_fm") != 0)
            && (strcmp(xilinx_name, "pe8dvcamlk2") != 0)
            && (strcmp(xilinx_name, "pe8dvcamlk2_fm") != 0))
    {
        sprintf(errstr, "\nWARNING: -- %s requires standard base mode firmware, found\n%s. If cl_speed fails, reload firmware by running running\n'pciload pdvcamlk' (rev 51 and earlier boards) or 'pciload pdvcamlk2'\n(rev 52 and later)' and try again.\n\n", progname, xilinx_name);
        edt_msg(EDTAPP_MSG_WARNING, errstr);
    }

    devid = pdv_p->devid;

    pdv_close(pdv_p);

    sprintf(cmdstr, ".%sinitcam -u %d -c 2 -f camera_config%sgeneric16cl.cfg", SLASH, unit, SLASH);
    system(cmdstr);
    sprintf(cmdstr, ".%stake -J -u %d -c 2 -N 4 -l 500 > cl_speed.out", SLASH, unit);
    system(cmdstr);
    sprintf(cmdstr, "%s cl_speed.out", CAT);
    system(cmdstr);

    if ((fp = fopen("cl_speed.out", "r")) == NULL)
        printf("error opening output file -- check directory permissions\n");
    else
    {
        while (fgets(s, 127, fp))
        {
            if (grepit(s, "bytes/sec"))
            {
                sscanf(s, "%lf", &bps);
                break;
            }
        }
        fclose(fp);
    }

    if (fake_speed >= 0)
        bps = fake_speed * 1048576;

    printf("\nThroughput was measured at %1.2lf Megabytes per second.\n\n", bps/1048576.0);

    i = 0;
    idx = 0;
    while (speed_table[i].id < 0xffff) /* failsafe-ish */
    {
        if ((devid == speed_table[i].id) || (speed_table[i].id == 0)) /* last one */
        {
            idx = i;
            break;
        }
        ++i;
    }

    if (bps < speed_table[idx].slow)
        puts(BADSPEED);
    else  if (bps < speed_table[idx].nominal)
        puts(speed_table[idx].slow_msg);
    else  if (bps < speed_table[idx].fast)
        puts(speed_table[idx].nominal_msg);
    else puts(speed_table[idx].fast_msg);

}

    void
usage(char *progname)
{
    puts("");
    printf("usage: %s [-u unit]\n", progname);
    printf("  -u unit     %s unit number (default 0)\n", EDT_INTERFACE);
    printf("  -s          skip introductory message and pause for ENTER\n");
}

    void
intro(char *progname)
{
    printf("%s measures the bus bandwidth by pushing PCI DV C-Link built-in\n", progname);
    printf("sumulator data as fast as possible across the PCI bus. Note that the\n");
    printf("results represent only a 'snapshot' of the conditions present while the\n");
    printf("test is being run, using internally generated simulator data. Processing,\n");
    printf("displaying, and saving data will all impact the actual sustainable max\n");
    printf("throughput in a given system.\n\n");
    printf("IMPORTANT: Exit all other PCI DV acquisition applications before continuing.\n\n");
}
