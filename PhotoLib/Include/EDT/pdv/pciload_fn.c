/*
 * pciload_fn.c
 *
 */
#include "edtinc.h"
#include "edt_bitload.h"
#include "pciload.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#ifdef _WIN32
#define F_OK 0
#define snprintf _snprintf
#define access _access
#else
#include <libgen.h>
#endif

volatile char *throwaway;

static Edt_bdinfo *Edt_detected_boards_list = NULL;

static void pad_sn(char *sn);
static void increment_sn(char *sn);
static void parse_id(char *idstr, char *lcaname, u_int *date, u_int *time);
static void remove_char(char *str, char ch);
static int  comment_or_blank(char *str);
static char *format_number_string(char *fmt, char *src, char *dest);

/* If the last character of string s is a newline (\r or \n), 
 * that character is replaced with NULL.
 */
void
strip_newline(char *s)
{
    int len = (int) strlen(s);
    if (len && ((s[len-1] == '\n') || (s[len-1] == '\r')))
        s[len-1] = '\0';
}

/* Converts all characters in string s to upper case */
static void
toupper_str(char *s)
{
    unsigned int i;
    unsigned int len = (int) strlen(s);

    for (i=0; i<len; i++)
        s[i] = toupper(s[i]);
}

/* Returns true (1) if every character in string s is a digit. */
int
pciload_isdigit_str(char *s)
{
    unsigned int i;
    unsigned int len = (unsigned int) strlen(s);

    for (i=0; i<len; i++)
        if (!isdigit(s[i]))
            return 0;
    return 1;
}
/* Returns true (1) if every character in string s is a digit. */
int
pciload_isxdigit_str(char *s)
{
    unsigned int i;
    unsigned int len = (unsigned int) strlen(s);

    for (i=0; i<len; i++)
        if (!isxdigit(s[i]))
            return 0;
    return 1;
}

/* Returns true (1) if every character in string s is alphanumeric. */
int
isalnum_str(char *s)
{
    unsigned int i;
    unsigned int len = (unsigned int) strlen(s);

    for (i=0; i<len; i++)
        if (!isalnum(s[i]))
            return 0;
    return 1;
}


/** Takes a part number portion of the embedded infrormation string, as retrieved from
 * the PCI FPGA information header via edt_get_sns; reformats and returns it in a
 * "human readable" form (dashes added). A simple subroutine mainly for use by pciload
 * and other EDT utility programs, but available for use by user apps if desired.
 *
 * The string returned is valid until the next invocation of this function.
 * @param pn the part number without dashes.
 * @param str the output string, which should be at least 13 characters long
 * @see edt_get_esn, edt_parse_esn
 */
char *
edt_fmt_pn(char *pn, char *str)
{
    if (strlen(pn) == 10)
        sprintf(str, "%c%c%c-%c%c%c%c%c-%c%c",
                pn[0],pn[1],pn[2],pn[3],pn[4],pn[5],pn[6],pn[7],pn[8],pn[9]);
    else if (strlen(pn) < 10)
        strcpy(str, pn);
    else strcpy(str, "0");
    return str;
}


/*
 * Detect all EDT boards in the system.
 * Designed for use by pciload & pciload-like applications. 
 *
 * @param dev device (driver) type ("pdv" or "pcd" or NULL for either), or 
 * @param unit a specific unit to find, or -1 for any unit that matches
 * @param nunits fills in the number of units found
 * @param verbose verbose output if nonzero
 * @return Array of zero or more bdinfo structs each with a detected board that meets the parameters
 */
Edt_bdinfo *
edt_detect_boards(char *dev, int unit, int *nunits, int verbose)
{
    return(edt_detect_boards_ids(dev, unit, NULL, nunits, verbose));
}


static char *device_name[NUM_DEVICE_TYPES] = {"pcd", "pdv", "p11w", "p16d", "p53b"};

/*
 * Detect all EDT boards in the system, filtered by an #edtBdFilterFunction
 * Designed for use by pciload & pciload-like applications. 
 *
 * @param filter filter, of type #EdtBdFilterFunction
 * @param data data for filter function
 * @param nunits fills in the number of units found
 * @param verbose verbose output if nonzero
 * @see EdtBdFilterFunction
 * @return Array of zero or more bdinfo structs each with a detected board that meets the parameters
 */
Edt_bdinfo *
edt_detect_boards_filter(EdtBdFilterFunction filter, void *data, int *nunits, int verbose) 
{
    int     board_count = 0;
    Edt_bdinfo buf[NUM_DEVICE_TYPES * MAX_BOARD_SEARCH];
    u_short  stat;
    int i;
    int     d, unit;
    EdtDev *temp;

    for (d = 0; d < NUM_DEVICE_TYPES; d++)
    {
        for (unit = 0; unit < MAX_BOARD_SEARCH; unit++)
        {
            temp = edt_open_quiet(device_name[d], unit);
            if ((temp != NULL) && (filter(device_name[d], unit, temp->devid, data)))
            {
                buf[board_count].bd_id = temp->devid;
                buf[board_count].promcode = edt_flash_prom_detect(temp, &stat);
                printf("set buf[%d] promcode to %d (%x)\n", board_count, buf[board_count].promcode, buf[board_count].promcode);
                if (verbose)
                    printf("  Found a device %s:%d\n", device_name[d], unit);
                strcpy(buf[board_count].type, device_name[d]);
                buf[board_count].id = unit;
                board_count++;
            }
            if (temp != NULL) {
                edt_close(temp);
            }

        }
    }
    buf[board_count].type[0] = 0;
    buf[board_count].id = -1;
    board_count++;

    edt_free_detected_boards_list();

    Edt_detected_boards_list = (Edt_bdinfo *)malloc(board_count * sizeof(Edt_bdinfo));

    for (i = 0; i < board_count; i++)
    {
        strcpy(Edt_detected_boards_list[i].type, buf[i].type);
        Edt_detected_boards_list[i].id = buf[i].id;
        Edt_detected_boards_list[i].bd_id = buf[i].bd_id;
        Edt_detected_boards_list[i].promcode = buf[i].promcode;
    }

    *nunits = board_count-1;

    return Edt_detected_boards_list;
}


/*
 * Detect all EDT boards in the system that match the given id.
 * Designed for use by pciload & pciload-like applications. Superseded
 * by edt_detect_boards_ids.
 *
 * @param dev device
 * @param unit a specific unit to find, or -1 for any unit that matches
 * @param idlist list of device ids to detect
 * @param nunits fills in the number of units found
 * #param verbose verbose
 * @return Array of zero or more bdinfo structs each with a detected board that meets the parameters
 */
Edt_bdinfo *
edt_detect_boards_id(char *dev, int unit, u_int id, int *nunits, int verbose)
{
    u_int ids[2];

    ids[0] = id;
    ids[1] = 0xffff;
    return edt_detect_boards_ids(dev, unit, ids, nunits, verbose);
}

/*
 * Detect any EDT boards in the system that match the given id(s).
 * Designed for use by pciload & pciload-like applications.
 *
 * @param dev device (driver) type ("pdv" or "pcd" or NULL for either), or 
 * @param unit a specific unit to find, or -1 for any unit that matches
 * @param idlist list of device ids to detect
 * @param nunits returns the number of units found
 * #param verbose verbose
 * @return Array of zero or more bdinfo structs each with a detected board that meets the parameters
 */
Edt_bdinfo *
edt_detect_boards_ids(char *dev, int unit, u_int *idlist, int *nunits, int verbose)
{
    int     board_count = 0;
    Edt_bdinfo buf[NUM_DEVICE_TYPES * MAX_BOARD_SEARCH];
    int     all_ids = 0;
    int     d, i, j, gotid;
    EdtDev *temp;

    if (idlist == NULL)
        all_ids = 1;

    /*
     * three possibilities: dev specified, unit specified, or neither
     */
    if (dev && *dev != 0)
    {
        for (i = 0; i < MAX_BOARD_SEARCH; i++)
        {
            if ((temp = edt_open_quiet(dev, i)) != NULL)
            {
                gotid = 0;
                j = 0;
                if (idlist) do
                {
                    if (temp->devid == idlist[j])
                    {
                        gotid = 1;
                        continue;
                    }
                } while (idlist[++j] != 0xffff) ;

                if (all_ids || gotid)
                {
                    u_short stat;

                    buf[board_count].bd_id = temp->devid;
                    buf[board_count].promcode = edt_flash_prom_detect(temp, &stat);
                    if (verbose)
                        printf("  Found a device %s:%d\n", dev, i);
                    strcpy(buf[board_count].type, dev);
                    buf[board_count].id = i;
                    board_count++;
                }
                edt_close(temp);
            }
        }
    }
    else if (unit != -1)
    {
        for (d = 0; d < NUM_DEVICE_TYPES; d++)
        {
            if ((temp = edt_open_quiet(device_name[d], unit)) != NULL)
            {
                gotid = 0;
                j = 0;
                if (idlist) do
                {
                    if (temp->devid == idlist[j])
                    {
                        gotid = 1;
                        continue;
                    }
                } while (idlist[++j] != 0xffff) ;

                if (all_ids || gotid)
                {
                    u_short  stat;
                    buf[board_count].bd_id = temp->devid;
                    buf[board_count].promcode = edt_flash_prom_detect(temp, &stat);
                    if (verbose)
                        printf("  Found a device %s:%d\n", device_name[d], unit);
                    strcpy(buf[board_count].type, device_name[d]);
                    buf[board_count].id = unit;
                    board_count++;
                }
                edt_close(temp);
            }
        }
    }
    else
    {
        for (d = 0; d < NUM_DEVICE_TYPES; d++)
        {
            for (i = 0; i < MAX_BOARD_SEARCH; i++)
            {
                if ((temp = edt_open_quiet(device_name[d], i)) != NULL)
                {
                    gotid = 0;
                    j = 0;
                    if (idlist) do
                    {
                        if (temp->devid == idlist[j])
                        {
                            gotid = 1;
                            continue;
                        }
                    } while (idlist[++j] != 0xffff) ;

                    if (all_ids || gotid)
                    {
                        u_short  stat;
                        buf[board_count].bd_id = temp->devid;
                        buf[board_count].promcode = edt_flash_prom_detect(temp, &stat);
                        if (verbose)
                            printf("  Found a device %s:%d\n", device_name[d], i);
                        strcpy(buf[board_count].type, device_name[d]);
                        buf[board_count].id = i;
                        board_count++;
                    }
                    edt_close(temp);
                }
            }
        }
    }
    buf[board_count].type[0] = 0;
    buf[board_count].id = -1;
    board_count++;

    edt_free_detected_boards_list();

    Edt_detected_boards_list = (Edt_bdinfo *)malloc(board_count * sizeof(Edt_bdinfo));

    for (i = 0; i < board_count; i++)
    {
        strcpy(Edt_detected_boards_list[i].type, buf[i].type);
        Edt_detected_boards_list[i].id = buf[i].id;
        Edt_detected_boards_list[i].bd_id = buf[i].bd_id;
        Edt_detected_boards_list[i].promcode = buf[i].promcode;
    }

    *nunits = board_count-1;

    return Edt_detected_boards_list;
}

/**
 * Free memory containing board list created & returned by any of the edt_detect_boards
 * functions. Note that there's just one list that gets freed & re-used when any of
 * the edt_detect_boards functions is called.
 * @see edt_detect_boards, edt_detect_boards_id, edt_detect_boards_ids, edt_detect_boards_filter
 */
void
edt_free_detected_boards_list()
{
    free(Edt_detected_boards_list);
    Edt_detected_boards_list = NULL;
}


#if 0

void
getinfo(EdtDev *edt_p, int promcode, int segment, char *id, char *esn, char *osn, int verbose)
{
    edt_readinfo(edt_p, promcode, segment, id, esn, osn);

    if (*id)
        printf("  PROM  id: <%s>", id);
    printf("\n");

}

void
getinfonf(EdtDev *edt_p, 
        int promcode, 
        int segment, 
        char *id, 
        char *esn, 
        char *osn,
        int verbose)
{
    edt_readinfo(edt_p, promcode, segment, id, esn, osn);

    if (*id)
        printf(" <%s>", id);
    printf("\n");

}

#endif

char *
edt_dev_from_id (int id)
{
    if (id == P11W_ID)   return "p11w";
    if (id == P16D_ID)   return "p16d";
    if (ID_IS_PCD(id))   return "pcd";
    if (ID_IS_PDV(id))   return "pdv";
    if (ID_IS_1553(id))  return "p53b";
    if (ID_IS_DUMMY(id)) return "dummy";
    return "unknown";
}

void
warnuser(EdtDev *edt_p, char *fname, int sector)
{
    char    s[EDT_STRBUF_SIZE];
    char    *foa = "file";
    char *dev;

    dev = edt_dev_from_id(edt_p->devid);

#ifdef NO_FS
    if ((strlen(fname) < 4) || (strcmp(&fname[strlen(fname)-4], ".bit") != 0))
        foa = "embedded array";
#endif

    if (strcmp(fname, "ERASE") == 0)
        return;

    printf("\n");
    printf("pciload is preparing to re-burn the system interface firmware on\n");
    printf("logical sector %d of %s unit %d from the FPGA configuration %s\n'%s'.\n", sector, dev, edt_p->unit_no, foa, fname);
    printf("\nTo confirm that you want to update the board's system interface\n");
    printf("firmware, press <enter> now. Otherwise, press 'q', then <enter> -> ");

    throwaway = fgets(s, EDT_STRBUF_SIZE-1, stdin);
    if ((tolower(s[0]) != 'y') && (s[1])) /* allow 'y' since its what a lot of people do automatically */
    {
        printf("exiting.\n");
#ifdef VXWORKS
        return;
#else
        exit(0);
#endif
    }
}

/**
 * Get the FPGA configuration file (bitfile) revision number.
 * Represented as a two-digit number in some though not all bitfiles;
 * represented as name-XX.ext, where XX is the revision and .ext is
 * the internal representation of the bitfile name, which may differ
 * from the filename. For example a file visionlinkF1.bit would have an
 * internal representation as visionlinkf1-10.rpd
 * 
 * pciload internal only, as this convention can not be relied on for all bitfiles
 *
 * @param FPGA configuration file (bitfile) as read in from 
 * @return the revision number, or 0 if not present
 */
int
get_bitfile_rev(EdtBitfile *bitfile)
{
  int rev = 0;
  char *p;

  if (bitfile && *(bitfile->hdr.ncdname))
  {
      p = &bitfile->hdr.ncdname[strlen(bitfile->hdr.ncdname)];

      /* walk back until '-' or beginning of string */
      while (--p != bitfile->hdr.ncdname)
      {
          if ((*p == '-') && isdigit(*(p+1)) && isdigit(*(p+2)))
          {
            rev = ((p[1] - '0') * 10) + (p[2] - '0');
            return rev;
          }
      }
  }
  return 0;
}


/*
 * check for conflicts between the firmware (bitfile) being loaded and the HW rev of the
 * device, and output a warning if indicated.
 *
 * @param edt_p pointer to edt device structure returned by #edt_open
 * @param bitfile the bitfile 
 */
void
check_hw_fw_conflicts(EdtDev *edt_p, EdtBitfile *bitfile)
{
    /* visionlink F1/rev10+ FW and F4/rev08+ FW has IRIG MSP430 code specific to >= rev 20+ HW */
    if ((edt_p->devid == PE1DVVL_ID) && (get_bitfile_rev(bitfile) >= 10) && (edt_get_hw_rev(edt_p) < 20))
    {
        printf("\nWARNING: Programming a rev 19 or earlier VisionLink F1 board with rev 10 or\n");
        printf("newer firmware; IRIG is deprecated with this combination. If you don't use\n");
        printf("IRIG (external timecode) functionality, there will be no negative impact.\n");
    }
    else if ((edt_p->devid == PE4DVVL_ID) && (get_bitfile_rev(bitfile) >= 8) && (edt_get_hw_rev(edt_p) < 20))
    {
        printf("\nWARNING: Programming a rev 19 or earlier VisionLink F4 board with rev 08 or\n");
        printf("newer firmware; IRIG is deprecated with this combination. If you don't use\n");
        printf("IRIG (external timecode) functionality, there will be no negative impact.\n");
    }
}


/*
 * local defines for mainboard type, for internal checking/
 * conflict warnings
 */
#define MBTYPE_OTHER 0
#define MBTYPE_SS 1
#define MBTYPE_GS 2

static int
check_idname_type(char *name)
{
    int ret = MBTYPE_OTHER;

    if (((strlen(name) > 5) && strncasecmp(name, "pciss", 5) == 0)
            || ((strlen(name) > 6) && strncasecmp(name, "pci_ss", 6) == 0)
            || ((strlen(name) > 6) && strncasecmp(name, "pci-ss", 6) == 0))
        ret = MBTYPE_SS;
    else if (((strlen(name) > 5) && strncasecmp(name, "pcigs", 5) == 0)
            || ((strlen(name) > 6) && strncasecmp(name, "pci_gs", 6) == 0)
            || ((strlen(name) > 6) && strncasecmp(name, "pci-gs", 6) == 0))
        ret = MBTYPE_GS;
    return ret;
}

/*
 * check the bitfile ID against the PROM id.
 * If verify_only flag is set just print a message saying they differ and
 * return 1 (means different).
 * If bitfile date is older than PROM date, print a warning.
 * If bitfile id indicates SS and PROM id indicates GS (or vice versa), print a
 * warning.
 * 
 *
 * ARGUMENTS
 *    bitfile_id    id string from bitfile
 *    prom_id           id string from prom
 *    verify_only       flag whether we're verifying (print just "differ")
 *                      or burning (print warning if bitfile older than PROM
 *
 * RETURNS
 *    1 if the same, 0 if different
 *
 */
int
check_id_stuff(char *bitfile_id, char *prom_id, int devid, int task, char *fname, int sector)
{
    int idmatch = 0;
    u_int   fdate, ftime, pdate = 0, ptime = 0, flashed_type, bitfile_type;
    char    s[MAX_STRING], bfname[MAX_STRING], prname[MAX_STRING];

    idmatch = ((strncmp(bitfile_id, prom_id, MAX_STRING) == 0)? 1:0);

    if (task == EDT_CheckIdOnly || task == EDT_VerifyOnly)
    {
        return idmatch;
    }
    else if ((task == EDT_CheckUpdate) && idmatch)
        return 1;

    parse_id(bitfile_id, bfname, &fdate, &ftime);
    parse_id(prom_id, prname, &pdate, &ptime);
    bitfile_type = check_idname_type(bfname);
    flashed_type = check_idname_type(prname);

    /* erase */
    if (strcmp(fname, "ERASE") == 0)
    {
        printf("\nPreparing to ERASE all sector %d EEprom info including board ID! To confirm,\n", sector);
        printf("press <enter> now.  Otherwise, press 'q', then <enter> -> ");
        fflush(stdout);
        throwaway = fgets(s, MAX_STRING - 1, stdin);
        if (s[1])
        {
            printf("exiting\n");
            exit(0);
        }
    }
    else
    {
        /* check /warn if trying to load a peXdva bitfile into a peXdv (or vice versa) */
        if ((strlen(bfname) > 6) && (strlen(prname) > 6))
        {
            char tmp_prname[32];
            char tmp_bfname[32];

            strncpy(tmp_prname, prname, 6);
            strncpy(tmp_bfname, bfname, 6);
            tmp_bfname[2] = 'X';
            tmp_prname[2] = 'X';

            /* check / warn on #lanes mismatch  */
            if ((strncmp(bfname, "pe", 2) == 0) && (bfname[2] > '0') && (bfname[2] < '9'))
            {
                int bf_lanes = bfname[2] - '0';
                int bd_lanes = 0;

                if ((bf_lanes < 1) || (bf_lanes > 8))
                    bf_lanes = 0;

                if (ID_IS_1LANE(devid)) bd_lanes = 1;
                if (ID_IS_4LANE(devid)) bd_lanes = 4;
                if (ID_IS_8LANE(devid)) bd_lanes = 8;

                if ((bd_lanes != 0) && (bf_lanes != 0) && (bd_lanes != bf_lanes))
                {
                    printf("\nWARNING: You appear to be attempting to reprogram a PCI Express %d lane\n", bd_lanes);
                    printf("board with with an FPGA configuration file for %d lane boards. Doing so\n", bf_lanes);
                    printf("can render your board inoperable.  If you are sure you want to do this,\n");
                    printf("press <enter> now. Otherwise quit by pressing 'q'<enter> -> ");
                    fflush(stdout);
                    throwaway = fgets(s, MAX_STRING - 1, stdin);
                    if (s[1])
                    {
                        printf("exiting\n");
                        exit(0);
                    }
                }
            }

            /* check / warn on 'a' versus non 'a' */ 
            if (strncmp(tmp_bfname, "peXdv", 5) == 0 && strncmp(tmp_prname, "peXdv", 5) == 0)
            {
                if ((tmp_bfname[5] == 'a') && (tmp_prname[5] != 'a'))
                {
                    printf("\nWARNING: You appear to be attempting to program an older PCIe DV board\n");
                    printf("with an FPGA file for PCIe DVa model boards. Reprogramming a non-'a' board\n");
                    printf("with 'a' board firmware will likely render the board inoperative. If you\n");
                    printf("are sure you want to do this, press <enter> now. Otherwise quit by pressing\n");
                    printf("'q'<enter>,  then check the FPGA names before trying again -> ");
                    fflush(stdout);
                    throwaway = fgets(s, MAX_STRING - 1, stdin);
                    if (s[1])
                    {
                        printf("exiting\n");
                        exit(0);
                    }
                }
                else if 
                    ((tmp_prname[5] == 'a') && (tmp_bfname[5] != 'a'))
                    {
                        printf("\nWARNING: You appear to be attempting to re-flash a PCIe DVa board with an\n");
                        printf("FPGA file for older PCIe DV model boards. Flashing 'a' boards with non-'a'\n");
                        printf("board firmware will likely render the board inoperative. If you are sure you\n");
                        printf("want to do this, press <enter> now. Otherwise quit by pressing 'q'<enter>,\n");
                        printf("then check the FPGA names before trying again -> ");
                        fflush(stdout);
                        throwaway = fgets(s, MAX_STRING - 1, stdin);
                        if (s[1])
                        {
                            printf("exiting\n");
                            exit(0);
                        }
                    }
            }
        }

        /* check / warn if loading SS bitfile into GS board (or vice versa) */
        if (((devid == PSS4_ID) || (devid == PSS16_ID)) && (bitfile_type == MBTYPE_GS))
        {
            printf("\nYou appear to be attempting to re-flash the PROM with an SS bitfile,\n");
            printf("but the board's hardware ID indicates it is a GS board. To program\n");
            printf("anyway, press <enter> now.  Otherwise, press 'q', then <enter> -> ");
            fflush(stdout);
            throwaway = fgets(s, MAX_STRING - 1, stdin);
            if (s[1])
            {
                printf("exiting\n");
                exit(0);
            }
        }
        if (((devid == PGS4_ID) || (devid == PGS16_ID)) && (bitfile_type == MBTYPE_SS))
        {
            printf("\nYou appear to be attempting to re-flash the PROM with a GS bitfile,\n");
            printf("but the board's hardware ID indicates it is an SS board. To program\n");
            printf("anyway, press <enter> now.  Otherwise, press 'q', then <enter> -> ");
            fflush(stdout);
            throwaway = fgets(s, MAX_STRING - 1, stdin);
            if (s[1])
            {
                printf("exiting\n");
                exit(0);
            }
        }

        /* check / warn re. date */
        if ((task == EDT_AutoUpdate || task == EDT_CheckUpdate) && ((fdate < pdate) || ((fdate == pdate) && (ftime < ptime))))
        {
            printf("\nALERT: Data in file is older than data in PROM. If your intention is to\n");
            printf("update to newer FPGA configuration file (as opposed to making a change in\n");
            printf("functionality for example), check the firmware timestamps before proceeding.\n");
            fflush(stdout);
        }
    }

    return idmatch;
}


static void
remove_char(char *str, char ch)
{
    char    tmpstr[EDT_STRBUF_SIZE];
    char   *p = tmpstr;
    char   *pp = str;

    strcpy(tmpstr, str);

    while (*p)
    {
        if (*p != ch)
            *pp++ = *p;
        ++p;
    }
    *pp = '\0';
}

/*
 * check for and fix reversed date string
 * that is, mm/dd/yyyy becomes yyyy/mm/dd
 * (to work around Xilinx tools funny-business -- hopefully
 * they won't do some other thing like dd/mm/yyyy)
 */
void
check_reversed(char *datestr)
{
    char mm[4], dd[4], yyyy[6];

    if ((!datestr) || (strlen(datestr) != 10))
        return;

    if ((strrchr(datestr, '/') - datestr) == 5)
        if (sscanf(datestr, "%[^/]/%[^/]/%s", dd, mm, yyyy) == 3)
            sprintf(datestr, "%s/%s/%s", yyyy, dd, mm);
}
/**
 * given a date string in the format yy/mm/dd or yyy/mm/dd or yyyy/mm/dd,
 * correct for the millenium. Will only work for the next 100 years or
 * less, depending on rollover.
 * 
 * @return 0 on success, -1 if format error
 */
int
edt_fix_millennium(char *str, int rollover)
{
    char    tmpstr[16];
    int     century=20;
    int     yr;

    if (strlen(str) > 15)
    {
        str[16] = '\0';
        return -1;
    }

    if (str[2] == '/')                /* yy/mm/dd */
    {
        yr = ((str[0] - '0') * 10) + (str[1] - '0');
        if (yr >= rollover)
            century = 19;
    }
    else if (str[3] == '/')            /* yyy/mm/dd */
    {
        yr = ((str[1] - '0') * 10) + (str[2] - '0');
        sprintf(str, "%02d", yr);
    }
    else if (str[4] == '/')            /* yyyy/mm/dd */
        return 0;
    else return  -1;

    sprintf(tmpstr, "%d%s", century, str);
    strcpy(str, tmpstr);
    return 0;
}


static void
parse_id(char *idstr, char *lcaname, u_int * date, u_int * time)
{
    int     ss;
    char    id[32];
    char    datestr[32];
    char    timestr[32];

    ss = sscanf(idstr, "%s %s %s %s", lcaname, id, datestr, timestr);

    if (ss == 4)
    {
#if 0 /* DEBUG */
        printf("read %d strings from <%s>:\n", ss, idstr);
        printf("'%s'\n", lcaname); printf("'%s'\n", id); printf("'%s'\n",
                datestr); printf("'%s'\n", timestr);
#endif

        check_reversed(datestr);

        edt_fix_millennium(datestr, 90);
        remove_char(datestr, '/');
        *date = (u_int) atoi(datestr);
        remove_char(timestr, ':');
        *time = (u_int) atoi(timestr);

    }
}

void
edt_print_dev_flashstatus(EdtDev *edt_p, u_short stat, int sector)
{
    int ftype = edt_flash_get_ftype(edt_p);
    int is_prot = edt_flash_is_protected(edt_p);

    edt_msg(EDT_MSG_INFO_1," FLASH STATUS: %08x\n", stat);

    if (sector != IS_DEFAULT_SECTOR)
    {
        edt_msg(EDT_MSG_INFO_1,"  Using %s %d\n", (ftype == FTYPE_MIC)? "flash #": "sector", sector);
    }

    if (is_prot)
        edt_msg(EDT_MSG_INFO_1,"    - Protected boot jumper installed on pins 1-2, using PROTECTED sector\n");
    else
        edt_msg(EDT_MSG_INFO_1,"    - Protected boot jumper removed or on pin 2-3; using unprotected sector\n");

    if (ftype != FTYPE_MIC)
    {
        if (stat & EDT_5_VOLT)
            edt_msg(EDT_MSG_INFO_1,"    - Installed in 5 volt slot\n");
        else
            edt_msg(EDT_MSG_INFO_1,"    - Installed in 3.3 volt slot\n");
    }

    if (is_prot)
    {
        edt_msg(EDT_MSG_FATAL,"  WARNING: Detected boot jumper(s) in PROTECTED position. Protected\n");
        edt_msg(EDT_MSG_FATAL,"           sectors may not contain the most recently burned FPGA.\n");
    }
}

/* obsolete, use edt_print_dev_flashstatus */
void
edt_print_flashstatus(u_short stat, int sector, int cpld)
{
    edt_msg(EDT_MSG_INFO_1,"  FLASHROM_DATA %08x\n", cpld);

    if (sector != IS_DEFAULT_SECTOR)
    {
        edt_msg(EDT_MSG_INFO_1,"  Using sector %d\n", sector);
    }
    edt_msg(EDT_MSG_INFO_1,"  Flash environment status = %02x %02x (flash id %02x straps %02x):\n", stat >> 8, stat & 0xff, (stat & 0x78) >>2, stat & 0x3);

    if (stat & EDT_ROM_JUMPER)
        edt_msg(EDT_MSG_INFO_1,"    - Protected boot jumper removed or on pin 2-3; using unprotected sector\n");
    else
        edt_msg(EDT_MSG_INFO_1,"    - Protected boot jumper installed on pins 1-2, using PROTECTED sector\n");

    if (stat & EDT_5_VOLT)
        edt_msg(EDT_MSG_INFO_1,"    - Installed in 5 volt slot\n");
    else
        edt_msg(EDT_MSG_INFO_1,"    - Installed in 3.3 volt slot\n");
    if (!(stat & EDT_ROM_JUMPER))
    {
        edt_msg(EDT_MSG_FATAL,"  WARNING: Detected boot jumper(s) in PROTECTED position. Protected\n");
        edt_msg(EDT_MSG_FATAL,"           sectors may not contain the most recently burned FPGA.\n");
    }
}

/*
 * ask whether to reboot the xilinx, do so if yes
 *
 * RETURNS 1 on success, 0 on no, -1 on failure
 */
int
edt_ask_reboot(EdtDev *edt_p)
{
    char s[EDT_STRBUF_SIZE];

    printf("Done. ");
    while(1)
    {

        u_short stat;
        int promcode = edt_flash_prom_detect(edt_p, &stat);

        printf("Attempt to reboot the %s board now? (y/n/h)[y] > ", edt_idstring(edt_p->devid, promcode));
        throwaway = fgets(s,127,stdin);
        if (tolower(s[0] == 'y') || s[0] == '\0')
        {
            if (edt_pci_reboot(edt_p) != 0)
            {
                printf("Couldn't reboot the xilinx; probably no s/w reboot support in this\nparticular bitfile.\n");
                return -1;
            }
            return 1;
        }
        else if (tolower(s[0] == 'h'))
        {
            printf("\nAccepting this option will attempt to reboot the board, forcing it to reload from\n");
            printf("the newly-updated PROM. This capability is only present in some xilinx versions; if\n");
            printf("successful it precludes the necessity of cycling power after a firmware update.\n");
            printf("If you accept this option and it doesn't succeed, simply halt the system and cycle\n");
            printf("power (previously the only way to do it).\n\n");
        }
        else if (tolower(s[0] == 'n'))
            return 0;
    }
}



/*
 * get the 5-10 digit serial number
 */
void
edt_ask_sn(int devid, char *tag, char *sn, int or_none)
{
    int ok = 0;
    int tries = 0;
    char s[EDT_STRBUF_SIZE];

    if ((strcmp(tag, "option") == 0) && (ID_IS_PDV(devid)))
    {
        *sn = '\0';
        return;
    }

    while (!ok)
    {
        if (or_none)
            printf("Enter %s serial number (5-10 characters, or 'n' for none) %s%s%s > ", tag,
                    *sn?"[":"", sn, *sn?"]":"");
        else

            printf("Enter %s serial number (5-10 characters) %s%s%s > ", tag,
                    *sn?"[":"", sn, *sn?"]":"");
        throwaway = fgets(s,127,stdin);
        strip_newline(s);

        if (*sn && !*s)
            return;

        if ((or_none) && (strlen(s) == 1) && (tolower(s[0]) == 'n'))
        {
            *sn = '\0';
            ok = 1;
        }

        else if ((strlen(s) < 5)
                || (strlen(s) > 10)
                || (!isalnum_str(s)))
        {
            if (++tries > 2)
            {
                printf("3 tries, giving up\n");
#ifdef VXWORKS
                return;
#else
                exit(1);
#endif
            }
            printf("\nInvalid %s serial number. Expecting 5-10 alphanumeric characters\n\n", tag);
        }
        else
        {
            strcpy(sn, s);
            toupper_str(sn);
            ok = 1;
        }
    }
}




/*
 * get the 10 digit option string. allow either XXX-XXXXX or
 * XXXXXXXX
 */
void
edt_ask_pn(char *pn)
{
    int i, j;
    int ok = 0;
    int tries = 0;
    char s[EDT_STRBUF_SIZE];
    char tmpstr[11];
    char pn_str[16];

    while (!ok)
    {
        printf("Enter 10-digit part # (with or without dashes) %s%s%s > ",
                *pn?"[":"", edt_fmt_pn(pn, pn_str), *pn?"]":"");
        throwaway = fgets(s,127,stdin);
        strip_newline(s);

        if (*pn && !*s)
            return;

        if (strlen(s) == 12)		/* XXX-XXXXX-XX */
        {
            /* reformat without the dashes */
            ok = 1;
            for (i=0, j=0; i<12; i++)
                if (s[i] != '-')
                    tmpstr[j++] = s[i];
            tmpstr[10] = '\0';
            strcpy(s, tmpstr);
        }
        else if (strlen(s) == 10)	/* XXXXXXXX */
        {
            if (pciload_isdigit_str(s))
                ok = 1;
        }

        if (!ok)
        {
            if (++tries > 2)
            {
                printf("3 tries, giving up\n");
#ifdef VXWORKS
                return;
#else
                exit(1);
#endif
            }
            printf("\nInvalid part number. Format is XXX-XXXXX-XX, all\n");
            printf("numeric (dashes optional)\n\n");
        }
        else
        {
            strcpy(pn, s);
            ok = 1;
        }
    }
}



void
edt_ask_clock(int *clock, char *extra_txt)
{
    int ok = 0;
    int tries = 0;
    int n;
    char s[EDT_STRBUF_SIZE];

    while (!ok)
    {
        printf ("Enter clock speed %s", extra_txt ? extra_txt : "");
        /* printf ("Enter clock speed (usually 10, 20, 30, 40, 100, 125 or 155)"); */
        if (*clock)
            printf(" [%d] > ", *clock);
        else printf(" > ");

        throwaway = fgets(s,127,stdin);
        strip_newline(s);

        if (*clock && !*s)
            return;

        n = atoi(s);

        if (n >= 10 && n <= 200)
        {
            ok = 1;
            *clock = n;
        }
        else
        {
            if (++tries > 2)
            {
                printf("3 tries, giving up\n");
#ifdef VXWORKS
                return;
#else
                exit(1);
#endif
            }
            printf("\nInvalid entry -- must be in the range 10-120\n");
        }
    }
}



void
edt_ask_rev(int *rev)
{
    int ok = 0;
    int tries = 0;
    int n;
    char s[EDT_STRBUF_SIZE];

    while (!ok)
    {
        printf ("Enter rev no.");

        if (*rev)
            printf(" [%02d] > ", *rev);
        else printf(" > ");

        throwaway = fgets(s,127,stdin);
        strip_newline(s);

        if (!*s)
        {
            if (*rev)
                return;
            else n = -1;
        }
        else n = atoi(s);

        if (n >= 0 && n <= 999)
        {
            ok = 1;
            *rev = n;
        }
        else
        {
            if (++tries > 2)
            {
                printf("3 tries, giving up\n");
#ifdef VXWORKS
                return;
#else
                exit(1);
#endif
            }
            printf("\nInvalid entry -- must be in the range 0-999\n");
        }
    }
}

#if 0 /* macaddrs are automatically calculated */
int
edt_ask_nmacs(int devid, int *count, char *maclist)
{
    int ok = 0, tries = 0, tmp;
    char s[EDT_STRBUF_SIZE];

    if (!ID_STORES_MACADDRS(devid))
        return 0;

    while (!ok)
    {
        printf ("Enter number of mac addresses");

        if (*count)
            printf(" [%02d] > ", *count);
        else printf(" > ");

        throwaway = fgets(s,127,stdin);
        strip_newline(s);

        if ((*count >= 0) && (*count <= MAX_MACADDRS) && !*s)
            return *count;

        tmp = atoi(s);

        if (tmp >= 0 && tmp <= MAX_MACADDRS)
        {
            ok = 1;
            *count = tmp;
        }
        else
        {
            if (++tries > 2)
            {
                printf("3 tries, giving up\n");
#ifdef VXWORKS
                return;
#else
                exit(1);
#endif
            }
            else printf("\nInvalid entry -- must be in the range 0-%d\n", MAX_MACADDRS);
        }
    }

    /* write count into maclist */
    sprintf(s, "%02d", *count);
    maclist[0] = s[0];
    maclist[1] = s[1];
    maclist[2] = ',';

    return *count;
}


void
edt_ask_maclist(int count, char *maclist)
{
    int ok = 0;
    int tries = 0;
    int i, j, m;
    char s[EDT_STRBUF_SIZE];
    char tmpstr[EDT_STRBUF_SIZE];
    char macaddr[EDT_STRBUF_SIZE];
    char *mp = &maclist[3];

    for (m=0; m<count; m++) 
    {
        tries = 0;
        ok = 0;
        {
            printf ("Enter mac address %d/%d", m+1, count, m);
            if (sscanf(mp, "%12s,", macaddr) == 1)
                printf(" [%s] > ", macaddr);
            else printf(" > ");

            throwaway = fgets(s,127,stdin);
            strip_newline(s);

            if (*macaddr && !(*s))
                goto edt_ask_maclist_next;

            if (strlen(s) == 17)		/* XX:XX:XX:XX:XX:XX */
            {
                /* reformat without the colons */
                ok = 1;
                for (i=0, j=0; i<17; i++)
                    if (s[i] != ':')
                        tmpstr[j++] = s[i];
                tmpstr[12] = '\0';
                strcpy(s, tmpstr);
            }

            if ((strlen(s) == 12) && pciload_isxdigit_str(s))
            {
                memcpy(mp, s, 12);
                mp[12] = ',';
                ok = 1;
            }
            else if (++tries > 2)
            {
                printf("3 tries, giving up\n");
#ifdef VXWORKS
                return;
#else
                exit(1);
#endif
            }
            else printf("\nInvalid entry. Format is XX:XX:XX:XX:XX:XX, all hex (colons optional)\n");
        }

edt_ask_maclist_next:
        mp += 13;
        ok = 1;
    }
}
#endif /* 0 macaddrs are automatically calculated */

/*
 * get the 10 digit option string
 */
void
edt_ask_options(char *options)
{
    int ok = 0;
    int tries = 0;
    char s[EDT_STRBUF_SIZE];

    while (!ok)
    {
        printf("Enter options, if any (0-10 characters) %s%s%s > ",
                *options?"[":"", options, *options?"]":"");
        throwaway = fgets(s,127,stdin);
        strip_newline(s);

        if (*options && !*s)
            return;

        if (strlen(s) > 10 || strchr(s, ':')) /* no colons allowed */
        {
            if (++tries > 2)
            {
                printf("3 tries, giving up\n");
#ifdef VXWORKS
                return;
#else
                exit(1);
#endif
            }
            printf("\nInvalid option string -- must be 0-10 alphanumeric characters\n\n");
        }
        else
        {
            strcpy(options, s);
            ok = 1;
        }
    }
}



int
edt_ask_addinfo()
{
    char s[EDT_STRBUF_SIZE];

    printf("\n");
    printf("This board is either being programmed for the first time, or has a\n");
    printf("corrupted or erased FPROM. Entering the embedded info (clock speed,\n");
    printf("board part number, serial number, and options) is recommended at\n");
    printf("this time. It is not required however, so if you don't have this\n");
    printf("information, you can skip this step by entering 'n'\n");

    *s = '\0';

    while (1)
    {
        char response;
        printf("\nDo you wish to enter the embedded information? [y/n/q] > ");
        throwaway = fgets(s,127,stdin);
        response = tolower(*s);

        if (response == 'y')
            return 1;
        else if (response == 'n')
            return 0;
        else if (response == 'q')
        {
            printf("exiting\n");
            exit(0);
        }
        printf("\nInvalid entry\n");
    }
}

void
print_ifx_dir(char *base)
{
    DIRHANDLE  dirp = (HANDLE) 0;
    char    d_name[EDT_PATHBUF_SIZE];
    int count = 0;

    if ((dirp = edt_opendir(base)) == (HANDLE)0)
        return;

    while (edt_readdir(dirp, d_name))
    {
        if ((strcmp(d_name, ".") != 0) && (strcmp(d_name, "..") != 0))
        {
            if (++count > 5)
            {
                count = 0;
                puts("");
            }
            printf(" %-12s", d_name);
        }
    }
    edt_closedir(dirp);
    printf("\n");
}

void
print_ifx_dirs()
{
    print_ifx_dir("./camera_config/bitfiles/dv");
    print_ifx_dir("./camera_config/bitfiles/dva");
    print_ifx_dir("./camera_config/bitfiles/dvaero");
    print_ifx_dir("./camera_config/bitfiles/dvfox");
    print_ifx_dir("./camera_config/bitfiles/dvk");
    print_ifx_dir("./camera_config/bitfiles/visionlink"); /* maybe not, but in case... */
    print_ifx_dir("./bitfiles");
}


int
match_ifx_dir(char *base, char *name)
{
    DIRHANDLE  dirp = (HANDLE) 0;
    char    d_name[EDT_PATHBUF_SIZE];
    int     ret = 0;

    if ((dirp = edt_opendir(base)) == (HANDLE)0)
        return 0;

    while (edt_readdir(dirp, d_name))
    {
        if (strcasecmp(name, d_name) == 0)
        {
            ret = 1;
            break;
        }
    }
    edt_closedir(dirp);
    return ret;
}

/*
 * check IF xilinx name against dir names and return 1 if match, 0 if not
 * ADD new dv devices as they come along
 */
int
valid_ifxname(int devid, char *name)
{
    if (ID_HAS_COMBINED_FPGA(devid) || strcmp(name, "none") == 0)
        return 1;

    if (   match_ifx_dir("./camera_config/bitfiles/dv", name)
            || match_ifx_dir("./camera_config/bitfiles/dva", name)
            || match_ifx_dir("./camera_config/bitfiles/dvaero", name)
            || match_ifx_dir("./camera_config/bitfiles/dvfox", name)
            || match_ifx_dir("./camera_config/bitfiles/dvk", name)
            || match_ifx_dir("./camera_config/bitfiles/visionlink", name) /* maybe not, but in case... */
       )
        return 1;

    if (match_ifx_dir("./bitfiles", name))
        return 1;
    return 0;
}


int
edt_ask_intfc_fpga(int devid, char *pn, int rev, char *ifx)
{
    int ok = 0;
    int tries = 0;
    char resp[EDT_STRBUF_SIZE];
    char tmppn[32];

    /* if xxx-xxxxx-00, replace last 2 digits with rev */
    if ((strcmp(&(pn[8]), "00") == 0))
        sprintf(&(tmppn[8]), "%02d", rev);
    else strcpy(tmppn, pn);

    if (ID_HAS_COMBINED_FPGA(devid))
    {
        strcpy(ifx, "none");
        return 0;
    }

    if ((strlen(ifx) < 2) && (strlen(tmppn) > 7))
        edt_find_xpn(tmppn, ifx);

    while (!ok)
    {
        printf("Enter interface FPGA (0-10 characters), or 'none') %s%s%s > ",
                *ifx?"[":"", ifx, *ifx?"]":"");
        throwaway = fgets(resp,127,stdin);
        strip_newline(resp);

        if (*ifx && !*resp)
            return 0;

        if (strlen(resp) > 10 || strchr(resp, ':')) /* no colons allowed */
        {
            if (++tries > 2)
            {
                puts("3 tries, giving up");
#ifdef VXWORKS
                return;
#else
                exit(1);
#endif
            }
            puts("\nInvalid I/F Xilinx string -- must be 0-10 alphanumeric characters\n");
        }
        else if (!valid_ifxname(devid, resp))
        {
            if (++tries > 2)
            {
                printf("3 tries, giving up\n");
#ifdef VXWORKS
                return;
#else
                exit(1);
#endif
            }
            puts("\nInvalid I/F Xilinx name -- must EXACTLY match an existing directory name.\nCheck spelling and case, and try again.\n");
            print_ifx_dirs();
        }
        else
        {
            strcpy(ifx, resp);
            ok = 1;

            if (strcmp(ifx, "none") == 0)
            {
                puts("\n  WARNING! You entered 'none' but pciload thinks this board has a ui FPGA!");
                puts("  If it really doesn't have one, then EDT SW engineering may need to know.");
                puts("  If it does then bitload may not work correctly after the board is programmed.");
                printf("\n  If you are sure you want to continue, press ENTER, or press ctrl-C to quit > ");
                throwaway = fgets(resp,127,stdin);
            }
        }
    }
    return 0;
}

#define EDTOUI 0x00251C

unsigned int
calc_macaddr(unsigned int typenum, unsigned int serial, unsigned int offset, unsigned int count, unsigned int index)
{
    return typenum << 16 | offset + (serial * count) + index;
}

#define EDTMACFNAME "edtmactable.txt"

int
pciload_maccalc(char *sserial, char *maclist)
{
    FILE *fp;
    char line [256];
    char name[32], prefix[32];
    unsigned int i;
    unsigned int offset, count;
    unsigned int nserial;
    unsigned int typenum;
    int finished=0;
    int lineno=0;

    if ((fp = fopen(EDTMACFNAME, "r")) == NULL)
    {
        char *warn = "\nWARNING:"  EDTMACFNAME;

        edt_perror(warn);
        fprintf(stderr, "This may be a problem if the device being programmed (or paired mezzanine)\nis one that uses MAC addresses.\n");
        sprintf(maclist, "00,");
        for (i=0; i<MAX_MACADDRS; i++)
            sprintf(&maclist[strlen(maclist)], ",000000000000");
        return -1;
    }

    while (fgets(line, 255, fp) && !finished)
    {
        ++lineno;
        if (!comment_or_blank(line))
        {
            if (sscanf(line, "%s %x %s %u %u", name, &typenum, prefix, &offset, &count) != 5)
            {
                fprintf(stderr, "Format error in %s, line %d\n", EDTMACFNAME, lineno);
                fprintf(stderr, "This is a potentially serious error, please notify EDT at tech@edt.com\n");
                return -1;
            }
            else
            {
                int plen = (int)strlen(prefix);
                if (strncmp(sserial, prefix, plen) == 0)
                {
                    sprintf(maclist, "%02d", count);
                    nserial = atoi(&(sserial[plen]));
                    for (i=0; i<MAX_MACADDRS; i++)
                    {
                        /* printf("%06X%06X\n", EDTOUI, calc_macaddr(typenum, nserial, offset, count, i)); */
                        if (i < count)
                            sprintf(&maclist[strlen(maclist)], ",%06X%06X", EDTOUI, calc_macaddr(typenum, nserial, offset, count, i));
                        else
                            sprintf(&maclist[strlen(maclist)], ",000000000000");
                    }
                    return 1;
                }
            }
        }
    }
    return 0;
}

/*
 * check string for comment or blank: return 1 (true) if all blanks or first
 * nonblank is '#' 
 */
int
comment_or_blank(char *str)
{
    size_t i;

    for (i=0; i<strlen(str); i++)
    {
        if (str[i] == '#')
            return 1;

        if ((str[i] != ' ') && (str[i] != '\t') && (str[i] != '\r') && (str[i] != '\n'))
            return 0;
    }
    return 1;
}


/*
 * pcie configuration, extracted from peiediag_src/pciecfg.h
 */
typedef struct {
    u_int devId;
    u_int venId;
    u_int capId;    //equals 0x10 for the capabilities reg
    u_int dcr;      //device capabilities reg
    u_int devStat;  //device status register
    u_int devCtrl;  //device control register
    u_int linkStat; //link status register
    u_int masterbit;
    u_int lcr;      //link capabilities reg
} EdtPcieCfg ;

void
edt_pciecfg_set_ids(EdtPcieCfg *cfg, u_int dev, u_int ven)
{
    cfg->devId = dev;
    cfg->venId = ven;
}

void
edt_pciecfg_set_link_stat(EdtPcieCfg *cfg, u_int reg_values)
{
    cfg->linkStat = ((reg_values & 0xffff0000) >> 16);
}


// Keep edt_pciecfg_get_link_stat() from outputting to stdout.
// This is the only place it's called, and it is not part of the
// libedt.h API.  --mcm  09/26/2018
static void
edt_pciecfg_get_link_stat(EdtPcieCfg *cfg, char *dest, size_t n)
{
    char *s_speed = "", *s_width = "";
    char outstr[128];

    switch(cfg->linkStat & 0x000f)
    {
        case 1:
            s_speed="2.5 Gbps";
            break;

        case 2:
            s_speed="5.0 Gbps";
            break;

        case 3:
            s_speed="8.0 Gbps";
            break;

        case 4:
        case 5:
        case 6:
        case 7:
            s_speed="rsvd";
            break;

        default: 
            s_speed="undefined";
            break;
    }

    switch((cfg->linkStat >> 4) & 0x3f) // link width
    {
        case 1:
            s_width="x1";
            break;
        case 2:
            s_width="x2";
            break;
        case 4:
            s_width="x4";
            break;
        case 8:
            s_width="x8";
            break;
        case 12:
            s_width="x12";
            break;
        case 16:
            s_width="x16";
            break;
        case 32:
            s_width="x32";
            break;
        default:
            s_width="rsvd";
            break;
    }

    sprintf(outstr, "  PCIe negotiated link width %s, %s\n", s_width, s_speed);
    strncpy(dest, outstr, n);
}

/**
 * print the negotiated link width & speed
 * code extracted from pciediag -- see pciediag_src 
 * or run pciediag for all the PCIe diagnostic info
 */
int
edt_sprint_pcie_negotiated_link(EdtDev *edt_p, char* dest, size_t n)
{
    EdtPcieCfg cfgObj;
    edt_buf buf;
    u_int addr;
    u_int capId = 0;
    u_int devId;
    u_int venId;
    u_int cap_ptr;
    u_int prev_addr;
    u_int mbit;
    int ret;

    addr = 0x0;
    buf.desc = addr;
    ret = edt_ioctl(edt_p, EDTG_CONFIG, &buf);

    devId = (buf.value >> 16);
    venId = (buf.value & 0x0000ffff);

    //create configuration object
    edt_pciecfg_set_ids(&cfgObj, devId, venId);

    addr = 0x4;
    buf.desc = addr;
    ret = edt_ioctl(edt_p, EDTG_CONFIG, &buf);
    mbit = (buf.value & 0x0002) >> 1;
    cfgObj.masterbit = mbit;

    //get the pointer to the capabilities register
    addr = 0x34;
    buf.desc = addr;
    edt_ioctl(edt_p, EDTG_CONFIG, &buf);
    cap_ptr = buf.value;

    do
    {
        buf.desc = cap_ptr;
        prev_addr = cap_ptr;
        edt_ioctl(edt_p, EDTG_CONFIG, &buf);
        capId = (buf.value & 0x000000ff);
        cap_ptr = ((buf.value & 0x0000ffff) >> 8);


    } while ((capId != cfgObj.capId) && cap_ptr != 0);

    if (capId != 0x10)
        return -1;

    //get [link status][link control]
    buf.desc = prev_addr + 0x10;
    edt_ioctl(edt_p, EDTG_CONFIG, &buf);
    edt_pciecfg_set_link_stat(&cfgObj, buf.value);

    // Keep edt_pciecfg_get_link_stat() from outputting to stdout.
    // This is the only place it's called, and it is not part of the
    // libedt.h API.  --mcm  09/26/2018
    edt_pciecfg_get_link_stat(&cfgObj, dest, n);

    return 0;
}

/**
 * print the negotiated link width & speed
 * code extracted from pciediag -- see pciediag_src 
 * or run pciediag for all the PCIe diagnostic info
 */
int
edt_print_pcie_negotiated_link(EdtDev *edt_p)
{
  char stat_str[128];

  edt_sprint_pcie_negotiated_link(edt_p, stat_str, 127);
  fputs(stat_str, stdout);
}


/*
 * check whether to ask for the serial number and do so if indicated
 * return nonzero if saving of new info is indicated, 0 if not, -1 on error
 */
int
edt_check_ask_info(EdtDev *edt_p, 
        int promcode, 
        int sect,
        EdtPromData *pdata,
        int verify_or_check_only, 
        int proginfo, 
        int nofs)
{
    /* proginfo > 1 means use info in file for
     * defaults instead of whats already on the board
     */
    Edt_embinfo ei;

    memset(&ei,0,sizeof(ei)) ;

    edt_parse_devinfo(pdata->esn, &ei);
    strcpy(ei.optsn, pdata->optsn);
    strcpy(ei.maclist, pdata->maclist);

    if ((!nofs) && (((proginfo) > 1) || (!*pdata->esn)))
    {
        char str[ESN_SIZE+2];

        if (edt_read_info_file(edt_p->devid, str, ESN_SIZE+2) == 0)
        {
            edt_parse_devinfo(str, &ei);
            if (*ei.sn)
                increment_sn(ei.sn);
        }
    }

    if (!verify_or_check_only)
    {
        if (!proginfo && (!*pdata->esn) && (!*pdata->id))
            proginfo = edt_ask_addinfo();

        if (proginfo)
        {
            if (edt_ask_info(edt_p->devid, &ei) != 0)
                return -1;
            edt_zero(pdata->esn, ESN_SIZE);
            edt_make_esn_string(pdata->esn, &ei);
            edt_zero(pdata->optsn, OPTSN_SIZE);
            strcpy(pdata->optsn, ei.optsn);
            edt_zero(pdata->maclist, MACLIST_SIZE);
            strcpy(pdata->maclist, ei.maclist);
        }
#if USE_OPTION_HACK /* need it still? if so add a param, pass it in and re-add the option */
        else if (Option_hack)
        {
            strcpy(ei.opt, "lvds");
            pad_sn(ei.sn);
            edt_make_esn_string(pdata->esn, &ei);
        }
#endif
    }
    return proginfo;
}

/*
 * prompt for info to put into the serial number, etc. part of
 * the FPGA PROM
 * return 0 on success, 1 on failure or quit
 */
int
edt_ask_info(int devid, Edt_embinfo *si)
{
    char resp[EDT_STRBUF_SIZE];
    int ok = 0, n, nmacs = 0;

    while (!ok)
    {   
        printf("\n");
        edt_ask_sn(devid, "board", si->sn, 0);

        edt_ask_pn(si->pn);
        edt_ask_rev(&(si->rev));
        edt_ask_intfc_fpga(devid, si->pn, si->rev, si->ifx);
        edt_ask_clock(&(si->clock), "(usually 10, 20, 30, 40, 100, 125, 155)");
        edt_ask_options(si->opt);

        if (si->maclist[0])
            if ((n = sscanf(si->maclist, "%02d,", &nmacs)) != 1)
                nmacs = 0;

        /* calculate & populate the mezzanine's mac addresses if appropriate */
        /* if it fails with board sn then try asking for mezz */ 
        if (!pciload_maccalc(si->sn, si->maclist))
        {
            char mezz_sn[64];
            memset(mezz_sn,0,64) ;
            if (ID_HAS_MEZZ(devid))
            {
                edt_ask_sn(devid, "paired mezzanine", mezz_sn, 1);
                pciload_maccalc(mezz_sn, si->maclist);
            }
        }

        edt_ask_sn(devid, "option", si->optsn, 1);

        printf("\n\n");
        edt_print_info(devid, si);
        printf("\n");

        *resp = '\0';
        while (tolower(*resp) != 'y' && (tolower(*resp) != 'n'))
        {
            printf("\nOkay? (y/n/q) [y]> ");
            throwaway = fgets(resp,127,stdin);

            if ((*resp == '\n') || (*resp == '\r') || (*resp == '\0'))
                *resp = 'y'; /* default to 'yes' */

            if (*resp == 'y')
                ok = 1;
            else if (*resp == 'q')
            {
                printf("exiting\n");
                return 1;
            }
            else if (*resp != 'n')
                printf("\nEnter 'y' if the above information is correct, 'n' to change, q to quit\n");
        }
    }
    return 0;
}


void
edt_print_info(int devid, Edt_embinfo *si)
{
    char pn_fmt[32];
    int clock_alert = 0;
    int nmacs;

    switch(si->clock)
    {
        case 10:
        case 20:
        case 30:
        case 40:
        case 100:
            clock_alert = 1;
            break;
        default:
            clock_alert = 0;
            break;
    }

    if ((sscanf(si->maclist, "%02d,", &nmacs) != 1) || (nmacs < 0) || (nmacs > MAX_MACADDRS))
        nmacs = 0;

    printf("Serial no:        %s\n", si->sn);
    printf("Part number:      %s\n", format_number_string("XXX-XXXXX-XX", si->pn, pn_fmt));
    printf("I/F FPGA:         %s\n", si->ifx);
    printf("Rev:              %02d\n", si->rev);
    printf("Clock:            %d Mhz\n", si->clock);
    printf("Options:          %s\n", si->opt[0]?si->opt:"<none>");
    if (!ID_IS_PDV(devid))
        printf("Option serial no: %s\n", si->optsn);
    printf("MAC addresses:    ");
    if (nmacs)
    {
        int i, idx = 3;

        for (i=0; i<nmacs; i++)
        {
            char mac_only[16], mac_fmt[24];

            strncpy(mac_only, &si->maclist[idx], 12);
            mac_only[12] = '\0';
            printf("%s%s ", format_number_string("XX:XX:XX:XX:XX:XX", mac_only, mac_fmt), i < nmacs-1?",":"");
            idx +=13;
        }
    }
    else printf("none\n");
}

/*
 * go from edt info struct to ':' formatted string.
 * return the string
 */
char *
edt_make_esn_string(char *str, Edt_embinfo *ei)
{
    sprintf(str, "%s:%s:%d:%s:%d:%s:", ei->sn, ei->pn, ei->clock, ei->opt, ei->rev, ei->ifx);
    return str;
}

int
edt_save_info_file(u_int devid, char *info, int vb)
{
    FILE *fp;
    char fname[32];

    sprintf(fname, "pciload_%02x.esn",  devid);

    if (edt_access(fname, 2) == 0)
    {
        /* if (vb) printf("overwriting existing device info file '%s'\n", fname); */
    }
    else if ((edt_access(fname, 0) == 0) || (edt_access(".", 2) != 0))
    {
        if (vb) printf("can't write or create device info file '%s'\n", fname);
        return -1;
    }

    if ((fp = fopen(fname, "w")) == NULL )
    {
        if (vb) edt_perror(fname);
        return -1;
    }

    fprintf(fp, "%s", info);
    fclose(fp);
    return 0;
}

/*
 * return length of string, or 0 if too big or small 
 */
int
edt_read_info_file(u_int devid, char *esn, int size)
{
    FILE *fp;
    int  ret = 0;
    size_t n;
    char fname[32];

    sprintf(fname, "pciload_%02x.esn",  devid);

    if ((fp = fopen(fname, "r")) == NULL )
    {
        /* edt_perror(str) ; */
        return -1;
    }

    /* read the ESN string */
    if ((!fgets(esn, size, fp)) || ((n = strlen(esn)) >= (size_t)size-1) || (n < 16)) /* arbitrary min. */
    {
        ret = -1;
        esn[0] = '\0';
    }

    fclose(fp);
    return ret;
}


/*
 * copy a maclist would use strncpy but do this instead to ensure format; i.e.
 * all unused entries are 000000000000
 */
void
edt_copy_maclist(char *dest, char *src)
{
    int i, nmacs, nmacs_save;
    char *sp = strchr(src, ',');
    char mac[MACADDR_SIZE];

    if ((sp == NULL) || (sscanf(src, "%02d,", &nmacs) != 1))
        nmacs = 0;

    /* nmacs was getting set to 0 in for loop. Don't know why...*/
    nmacs_save = nmacs;

    sprintf(dest, "%02d", nmacs);

    for (i=0; i<16; i++, sp+=13)
    {
        if ((i < nmacs_save) && (sscanf(sp, ",%12s", mac) == 1))
        {
            strcat(dest, ",");
            strcat(dest, mac);
        }
        else strcat(dest, ",000000000000");
    }
    dest[MACLIST_SIZE-2] = dest[MACLIST_SIZE-1] = '\0';
}

char *
edt_merge_esns(char *new_esn, char *esn, char *new_sn, char *new_opts, char *new_pn, char *new_rev, char *new_clock)
{
    Edt_embinfo ei;

    memset(&ei,0,sizeof(ei)) ;
    edt_zero(new_esn, ESN_SIZE);
    edt_parse_devinfo(esn, &ei);

    if (!(*new_sn || *new_pn || *new_rev || *new_clock || *new_opts))
        return NULL;

    if (*new_sn)
        strncpy(ei.sn, new_sn, 11);
    if (*new_pn)
        strncpy(ei.pn, new_pn, 11);
    if (*new_opts)
        strncpy(ei.opt, new_opts, 15);
    if (*new_rev)
        ei.rev = atoi(new_rev);
    if (*new_clock)
        ei.clock = atoi(new_clock);
    edt_make_esn_string(new_esn, &ei);
    return new_esn;
}

static void
pad_sn(char *sn)
{
    char *p;
    char tmpsn[EDT_STRBUF_SIZE];
    int num;

    strcpy(tmpsn, sn);

    p = &tmpsn[strlen(sn)];

    while (p > tmpsn && (*(p-1) >= '0') && (*(p-1) <= '9'))
        --p;

    num = atoi(p);
    *p = '\0';
#ifdef USE_OPTION_HACK
    if (Option_hack)
        sprintf(sn, "PDVF%04d", num);
    else
#endif
        sprintf(sn, "%s%04d", tmpsn, num);
}


static void
increment_sn(char *sn)
{
    char *p;
    char tmpsn[EDT_STRBUF_SIZE];
    int num;

    strcpy(tmpsn, sn);
    p = &tmpsn[strlen(sn)];

    while (p > tmpsn && (*(p-1) >= '0') && (*(p-1) <= '9'))
        --p;

    num = atoi(p);
    *p = '\0';
    sprintf(sn, "%s%04d", tmpsn, ++num);
}

/*
 * format a string per the fmt argument, place the result in
 * the dest string and return a pointer to that. format string
 * should be upper or lower-case Xs -- anything else is treated
 * as a format character that will be inserted into the dest string.
 * For example xx-xxxxx-xx to add dashes in the dest string. May
 * use dest as src (overwriting the unformatted with the formatted
 * string); in that case src/dest string must be at least as long
 * as the fmt string.
 */
char *
format_number_string(char *fmt, char *src, char *dest)
{
    u_int i, j=0;
    char *tmpstr = (char *)edt_alloc((int)strlen(fmt)+1);

    for (i=0; i<strlen(fmt) && j<strlen(src); i++)
    {
        if (tolower(fmt[i]) == 'x')
            tmpstr[i] = src[j++];
        else tmpstr[i] = fmt[i];
    }
    tmpstr[i] = '\0';
    strcpy(dest, tmpstr);
    edt_free(tmpstr);
    return dest;
}


/*
 * print the serial numbers
 */
void
edt_print_id_info(EdtPromData *pdata)
{
    Edt_embinfo ei;
    char pn_str[16];
    char mac_str[32];
    int nmacs;
    int i, j;

    if ((!pdata->maclist[0]) || (sscanf(pdata->maclist, "%2d,", &nmacs) != 1) || (nmacs < 0) || (nmacs > 16))
        nmacs = 0;

    memset(&ei,0,sizeof(ei)) ;

    if (pdata->esn[0])
    {
        if (edt_parse_devinfo(pdata->esn, &ei) == 0)
        {
            printf("  s/n %s, p/n %s, i/f fpga %s, rev %d, clock %d Mhz", 
                    ei.sn, 
                    edt_fmt_pn(ei.pn, pn_str), 
                    strlen(ei.ifx) > 2?ei.ifx:"(none)", 
                    ei.rev, ei.clock);
            if (*ei.opt)
                printf(", opt %s", ei.opt);

            if (pdata->osn[0])
                printf(", oem s/n %s", pdata->osn);

            if ((*pdata->optsn) || (nmacs))
            {
                printf("\n  option sn %s, %d mac addrs", pdata->optsn[0]?pdata->optsn:"none", nmacs);
                for (i=0, j=3; i<nmacs; i++, j+=13)
                {
                    if (sscanf(&pdata->maclist[j], "%12s,", mac_str) == 1)
                        printf("%s%s", i == 0? ": ":", ", format_number_string("XX:XX:XX:XX:XX:XX", mac_str, mac_str));
                }
            }
            printf("\n");
        }
    }

}

char *
edt_fmt_promcode(int promcode, char promcode_str[])
{
    Edt_prominfo *pi = edt_get_prominfo(promcode);

    if (promcode < 0 || promcode > edt_get_max_promcode())
        sprintf(promcode_str, "Unknown Flash ROM!");
    else sprintf(promcode_str, "%s %s FPGA, %s FPROM", pi->fpga, pi->busdesc, pi->promdesc);

    return promcode_str;
}

/**
 * Update the file name based on what is found in the flash directory. 
 * If the bitfile name has a revision, it will update the bitfile name to be the
 * latest revision.
 * 
 * @param flash_dir_path - Directory path that contains bitfiles.
 * @param fname - The file name to update.
 * 
 * @return - The highest revision number found, 0 indicates no revision was found.
 */
static uint32_t update_file_name_to_latest_rev(const char *const flash_dir_path, char *fname)
{
  char dirent_filename[EDT_PATHBUF_SIZE] = "";
  DIRHANDLE dirhandle = 0;
  uint32_t highest_rev = 0;
  uint32_t rev = 0;
  char *rev_delim = 0;
  char *rev_str = 0;

  if (!(dirhandle = edt_opendir(flash_dir_path)))  
    return -1;

  while (edt_readdir(dirhandle, dirent_filename)) 
  {
    // First check for revision # and get the highest available in the directory
    if (!(rev_delim = strstr(dirent_filename, "-")) || 
        (strstr(dirent_filename, "pe8lx"))) // lx bitfiles can have a '-' that does not indicate revision
    {
      continue;
    }

    rev_str = rev_delim + 1;
    if (!rev_str)
      continue;
  
    rev = strtol(rev_str, NULL, 10);

    // found a later bitfile update name
    if (rev > highest_rev)
    {
      highest_rev = rev;
      memset(fname, 0, EDT_PATHBUF_SIZE);
      strncpy(fname, dirent_filename, strlen(dirent_filename) + 1);
    }
  }          

  edt_closedir(dirhandle);

  return highest_rev;
}

/**
 * A full bitfile name has been provided (name that ends in .bit), but not a 
 * directory path. Look for the  bitfile in 3 places: cwd, flash_dir, and flash_dir/fpga/.
 * If found in one of these places concatenate the appropriate path to 
 * the file name and return, otherwise fail.
 * 
 * @param flash_dir - The base flash directory, (typically /opt/EDTpcd/flash)
 * @param full_dir_path - The flash directory with the fpga appended to it.
 * @param name_to_load - The bitfile name appended to the path.
 * 
 * @return 0 indicates file was found, -1 on failure.
 */
static int find_bitfile_and_set_path(const char *const flash_dir, const char *const full_dir_path, char *name_to_load)
{
  char f_path[EDT_PATHBUF_SIZE] = "";

  if (access(name_to_load, F_OK) != -1)
    return 0;

  snprintf(f_path, EDT_PATHBUF_SIZE, "%s/%s", flash_dir, name_to_load);

  if (access(f_path, F_OK) != -1)
  {
    strncpy(name_to_load, f_path, strlen(f_path) + 1);
    return 0;
  }

  snprintf(f_path, EDT_PATHBUF_SIZE, "%s/%s", full_dir_path, name_to_load);
  if (access(f_path, F_OK) != -1)
  {
    strncpy(name_to_load, f_path, strlen(f_path) + 1);
    return 0;
  }

  return -1;
}

int find_fname_to_load(int promcode, int is_5_volt, int nofs, const char *const orig_fname, const char *const flash_dir, char *name_to_load)
{ 
  char full_dir_path[EDT_PATHBUF_SIZE] = ""; // flash dir path including the correct promdir
  int rev = 0; // bitfile revision number
  char voltage[4] = "";
  char rev_fname[EDT_PATHBUF_SIZE] = ""; // temporary for an updated bitfile name with revision
  Edt_prominfo *pi = NULL;

  if (strcmp("ERASE", orig_fname) == 0)
  {
    strncpy(name_to_load, orig_fname, strlen(orig_fname) + 1);
    return 0;
  }

  /**
   * If a path is specified by the user then it is assumed that 
   * this is the bitfile they want loaded and no further work has to be done.
   * Check for a "/" to indicate a directory path.
   */
  if (strstr(orig_fname, "/"))
  {
    if (access(orig_fname, F_OK) == -1)
    {
      printf("Unable to find file: %s\n", orig_fname);
      return -1;
    }
    
    strncpy(name_to_load, orig_fname, strlen(orig_fname) + 1);
    return 0;
  }

  pi = edt_get_prominfo(promcode);
  
  // construct the full directory path
  snprintf(full_dir_path, EDT_PATHBUF_SIZE, "%s/%s", flash_dir, pi->fpga);

  /**
   * If a full file name is given but no path, 
   * try to find the bitfile.
   */
  if (strstr(orig_fname, ".bit"))
  {
    strncpy(name_to_load, orig_fname, strlen(orig_fname) + 1);
    if (find_bitfile_and_set_path(flash_dir, full_dir_path, name_to_load))
    {
      printf("Unable to find file: %s\n", name_to_load);
      return -1;
    }

    return 0;
  }
  
  /**
   * User did not specify full file name/path or the update option
   * was used, so we need to find the latest version of 
   * the bitfile or complete the bitfile name. First check for 
   * the voltage option.
   */
  if (is_5_volt != -1)
  {
    char *v3 = strstr(orig_fname, "_3v");
    char *v5 = strstr(orig_fname, "_5v");
    if ((!v3 && !v5) && is_5_volt)
        strncpy(voltage, "_5v", 3);
    else if ((!v3 && !v5) && !is_5_volt)
        strncpy(voltage, "_3v", 3);
    else if ((v3 && is_5_volt) || (v5 && !is_5_volt))
    {
      printf("file contains a '_%sv option, while trying to program a %svolt sector\n",
              v3 ? "3.3" : "5", is_5_volt ? "5" : "3.3");
      return -1;
    }
  }
  else // files with voltages will never have rev #s
    rev = update_file_name_to_latest_rev(full_dir_path, rev_fname);

  // Now construct full file path from it's components
  if (nofs)
    snprintf(name_to_load, EDT_PATHBUF_SIZE, "%s/%s%s.bit", pi->fpga, orig_fname, voltage);
  else
  {
    if (rev)
      snprintf(name_to_load,  EDT_PATHBUF_SIZE, "%s/%s", full_dir_path, rev_fname);   
    else
      snprintf(name_to_load,  EDT_PATHBUF_SIZE, "%s/%s%s.bit", full_dir_path, orig_fname, voltage);
  }
  
  // Check to see if file exists
  if (access(name_to_load, F_OK) == -1)
  {
    printf("Unable to find file: %s\n", name_to_load);
    return -1;
  }

  return 0;
}
