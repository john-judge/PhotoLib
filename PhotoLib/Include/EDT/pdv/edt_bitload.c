/* #pragma ident "@(#)edt_bitload.c	1.88 11/05/07 EDT" */

/**
* @file
* edt_bitload.c - functions to load the EDT PCI interface FPGA .bit file
* 
* HISTORY
*   1997 creation
*   1998-1999 various mods, including incrementally add 4010, 4013, etc.
*      capability
*   2000 rewritten from standalone to library routine
*   8/2001 rewrote FPGA header code to be smarter, now actually
*       decodes header and finds data size and start of data reliably and
*       independent of the FPGA size. Also rewrote to look at and get
*       sizes of and create a list of files found, then sort by size and
*       try in order smallest to largest, since can SMOKE a 600 part
*       trying to load a larger file in it.
* 
* (C) 1997-2007 Engineering Design Team, Inc.
*/
#include "edtinc.h"
#include "edt_bitload.h"
#include "pciload.h"
#include <stdlib.h>
#include <ctype.h>

#ifdef WIN32
#include <errno.h>
#endif


static int xa_get_magic(u_char **ba);
static int xf_get_magic(FILE *fp);
static int xb_get_magic(u_char *buf);

/* shorthand debug level */
#define DEBUG1 EDTLIB_MSG_INFO_1
#define DEBUG2 EDTLIB_MSG_INFO_2

/* FPGA magic number (first 13 bytes) is always the same ( so far... ) */
#define MGK_SIZE 13


u_char XilinxMagicArray[MGK_SIZE] = {0x00, 0x09, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x00, 0x00, 0x01 };

/* EDT magic number used for Altera converted from .rbt, different num. but same size as Xilinx, for compat. */
u_char AlteraMagicArray[MGK_SIZE] = {0x00, 0x0e, 0x0d, 0x0f, 0x0a, 0x00, 0x0f, 0x0e, 0x08, 0x08, 0x0a, 0x00, 0x01 };

/* EDT magic number used for Altera converted from .rpd, different num. but same size as Xilinx, for compat. */
u_char Altera2MagicArray[MGK_SIZE] = {0x00, 0x0e, 0x0d, 0x0f, 0x0a, 0x02, 0x0f, 0x0e, 0x08, 0x08, 0x0a, 0x00, 0x01 };


#ifdef NO_FS
#include "nofs_bitfiles.h"
#else
#define MAPBITARRAY(a,b,c) (b=NULL,c=0)
#endif

/** EdtSizedBuffer functions -
 * these are a hack to get C++ behavior in C
 */

void bfh_init(EdtBitfileHeader *bfh)
{
    memset(bfh, 0, sizeof(EdtBitfileHeader));
}


/******************************
 * EdtBitfile is a wrapper around a file handle or a 
 * SizedBuffer
 *
 */

void edt_bitfile_init(EdtBitfile *bfd)

{
    bfd->is_file = FALSE;
    bfd->filename = NULL;
    bfd->f = NULL_HANDLE;

    bfd->buffer_allocated = 0;
    bfd->full_buffer_size = 0;
    bfd->full_buffer= NULL;
    bfd->data_size = 0;
    bfd->data_size = 0;

    bfh_init(&bfd->hdr);
}

int edt_bitfile_open_file(EdtBitfile *bfd, const char *name, u_char writing)

{
    if ((bfd->f = edt_open_datafile(NULL, name, writing, FALSE, FALSE)) != 
            NULL_HANDLE)
    {
        bfd->is_file = TRUE;
        bfd->filename = strdup(name);
        return 0;
    }

    return -1;
}

int edt_bitfile_close_file(EdtBitfile *bfd)

{
    if (bfd->f != NULL_HANDLE)
    {
        edt_close_datafile(bfd->f);
        bfd->f = NULL_HANDLE;
        bfd->is_file = FALSE;

    }

    return 0;
}

void edt_bitfile_destroy(EdtBitfile *bfd)

{
    edt_bitfile_close_file(bfd);
    if (bfd->full_buffer)
    {
        free(bfd->full_buffer);
        bfd->full_buffer = NULL;
        bfd->buffer_allocated = 0;
        bfd->full_buffer_size = 0;
    }
    if (bfd->filename)
    {
        free(bfd->filename);
        bfd->filename = NULL;
    }      
}

int edt_bitfile_read(EdtBitfile *bfd, void *target, int length)

{

    if (bfd->is_file)
        return (int) edt_read_datafile(bfd->f,target, length);
    else
    {

        int len;
        
        if (length + bfd->cur_index > bfd->full_buffer_size)
        {
            len = bfd->full_buffer_size - bfd->cur_index;
        }
        else 
            len = length;

        if (len)
        {
            memcpy(target, bfd->full_buffer + bfd->cur_index, len);
            bfd->cur_index += len;
        }

        return len;
    }
}

int edt_bitfile_seek(EdtBitfile *bfd, int offset)

{
    if (bfd->is_file)
        return (int) edt_file_seek(bfd->f, offset);
    else
    {
        if (offset >= 0 && offset <= (int) bfd->full_buffer_size)
            bfd->cur_index = offset;

        return bfd->cur_index;
    }
}

int edt_bitfile_tell(EdtBitfile *bfd)

{
    if (bfd->is_file)
        return (int) edt_get_file_position(bfd->f);
    else
        return bfd->cur_index;
}

int edt_bitfile_size(EdtBitfile *bfd)

{
    if (bfd->is_file)
    {
        return (int) edt_get_file_size(bfd->f);
    }
   
    return bfd->full_buffer_size;
}


int edt_bitfile_allocate(EdtBitfile *bfd, u_int size)

{
    
    if (bfd->full_buffer)
    {
        if (bfd->buffer_allocated != size)
        {
            free(bfd->full_buffer);
            bfd->full_buffer = 0;
            bfd->buffer_allocated = 0;
        }
        else
        {
            return 0;
        }
    }

    if (size)
    {
        bfd->full_buffer = (u_char *) calloc(size,1);
        if (bfd->full_buffer == NULL)
        {
            bfd->buffer_allocated = 0;
            return -1;
        }
        else
            bfd->buffer_allocated = size;
    }
    
    return 0;


}

int edt_bitfile_load_file(EdtBitfile *bfd, const char *name)

{

    int rc;
    u_int size;

    /* get file size */

    if ((rc = edt_bitfile_open_file(bfd, name, FALSE)) != 0)
    {
        edt_msg(EDT_MSG_FATAL, "edt_bitfile_load_file: Unable to open file %s\n", 
                name);

        return rc;
    }

    /* allocate */
    size = (u_int) edt_get_file_size(bfd->f);

    if (edt_bitfile_allocate(bfd, size) != 0)
    {
        edt_msg(EDT_MSG_FATAL, "edt_bitfile_load_file: Unable to allocate %d bytes\n", 
                size);

        return -1;

    }

    /* load */

    if (edt_read_datafile(bfd->f, bfd->full_buffer, size) != size)
    {
        edt_msg(EDT_MSG_FATAL, "edt_bitfile_load_file: Unable to read all %d bytes\n", 
                size);

        return rc;

    }

    edt_get_bitfile_header(bfd, &bfd->hdr);

    edt_bitfile_close_file(bfd);

    bfd->full_buffer_size = size;
    bfd->data = bfd->full_buffer + bfd->hdr.data_start;
    bfd->data_size = bfd->hdr.dsize;

    return 0;
}

int
edt_bitfile_load_array(EdtBitfile *bfd, u_char *data, u_int size)

{
    int rc = -1;

    if (edt_bitfile_allocate(bfd, size) != 0)
    {
        edt_msg(EDT_MSG_FATAL, "edt_bitfile_load_file: Unable to allocate %d bytes\n", 
                size);

        return rc;

    }

    /* load */

    memcpy( bfd->full_buffer, data, size);

    bfd->full_buffer_size = size;
    
    bfd->is_file = FALSE;

    return 0;
}


/*
* get magic number, return magic number enum (XilinxMagic, AlteraMagic, AlteraMagic2), or -1
*/
static int
edt_bitfile_get_magic(EdtBitfile *bp)
{

    u_char buf[MGK_SIZE+1];
    int rc;

    if ((rc = edt_bitfile_read(bp, buf, MGK_SIZE)) != MGK_SIZE)
    {
	printf("Error reading %d magic bytes got %d\n", MGK_SIZE, rc);
        return -1;    
    }

    return xb_get_magic(buf);

}

/*
 * get magic from buffer (called by edt_bitfile_get_magic and xf_get_magic, on buf
 * or array 
 * buf has been read in)
 */

static int
xb_get_magic(u_char *buf)
{

    if (memcmp(buf, XilinxMagicArray, MGK_SIZE) == 0)
        return XilinxMagic;

    if (memcmp(buf, AlteraMagicArray, MGK_SIZE) == 0)
        return AlteraMagic;

    if (memcmp(buf, Altera2MagicArray, MGK_SIZE) == 0)
        return Altera2Magic;

    return -1;
}


static int
edt_bitfile_get_field_id(EdtBitfile *bp, u_char *fi)
{
    edt_bitfile_read(bp, fi, 1);
    return 0;
}

static int
edt_bitfile_get_string(EdtBitfile *bp, u_char *str, u_int maxcount)
{
    u_char tmpcount[2];
    u_short count;

    edt_bitfile_read(bp, tmpcount, 2);

    count = tmpcount[0] << 8 | tmpcount[1];
    if (count > maxcount)
    {
	edt_msg(DEBUG1, "invalid FPGA header string (count %d)\n", count);
	return -1;
    }

    edt_bitfile_read(bp, str, count);

    return 0;
}

static int
edt_bitfile_get_long_size(EdtBitfile *bp,  u_int *size)
{
    u_char tmpsize[4];

    edt_bitfile_read(bp, tmpsize, 4);

    *size =  (tmpsize[0] << 24) | (tmpsize[1] << 16) | (tmpsize[2] << 8) | (tmpsize[3]);

    return 0;
}


/*
 * Get the FPGA file header.
 *
 * Extract the FPGA header (Xilinx, OR faked one for Alterra, etc.)
 *
 * @param bp pointer to an EdtBitfile struct that will be populated with the FPGA file information.
 * @param bfh pointer to the EdtBitfileHeader struct that will be populated with the FPGA file header information.
 *
 * @return -1 on error, otherwise the address of the beginning of the FPGA data (after the header).
 */

int
edt_get_bitfile_header(EdtBitfile *bp, 
                         EdtBitfileHeader *bfh)
{
    int i;
    int remaining;
    char *p;
    u_char tmpname[MAXPATH];
    u_char dmy[2];

    edt_msg(DEBUG2, "edt_get_bitfile_header:\n");

    bfh_init(bfh);
    edt_bitfile_seek(bp, 0);

    if ((bfh->magic = edt_bitfile_get_magic(bp)) < 0)
    {
        edt_msg(DEBUG1, "Invalid FPGA magic bytes in header array\n");
        return  -1;
    }
    else 
        edt_msg(DEBUG2, "magic=%s (%d)\n", 
        (bfh->magic == XilinxMagic)?"XILINX":
        (bfh->magic == AlteraMagic)?"ALTERA":
        (bfh->magic == Altera2Magic)?"ALTERA2":
        "UNKNOWN (huh?)", bfh->magic);

    if ((edt_bitfile_get_field_id(bp, &bfh->fi[0]) != 0)
	|| (edt_bitfile_get_string(bp, tmpname, sizeof(tmpname)) != 0)
	|| (edt_bitfile_get_field_id(bp, &bfh->fi[1]) != 0)
	|| (edt_bitfile_get_string(bp, bfh->id, sizeof(bfh->id)) != 0)
	|| (edt_bitfile_get_field_id(bp, &bfh->fi[2]) != 0)
	|| (edt_bitfile_get_string(bp, bfh->date, sizeof(bfh->date)) != 0)
	|| (edt_bitfile_get_field_id(bp, &bfh->fi[3]) != 0)
	|| (edt_bitfile_get_string(bp, bfh->time, sizeof(bfh->time)) != 0)
	|| (edt_bitfile_get_field_id(bp, &bfh->fi[4]) != 0)
	|| (edt_bitfile_get_long_size(bp, &bfh->dsize) != 0))
    {
	edt_msg(EDT_MSG_FATAL, "Error: bad FPGA header\n");
        return -1;
    }

    /* NEW 11/24/2008 RWH: if it's a full path, strip off and leave just the name */
    if (bitload_has_slashes((char *)tmpname))
    {
	edt_back_to_fwd((char *)tmpname); /* just so we can find last one with strrchr */
    	strcpy((char *)bfh->ncdname, strrchr((char *)tmpname, '/')+1);
	edt_msg(DEBUG2, "stripping off FPGA header path\n");
    }
    else strcpy((char *)bfh->ncdname, (char *)tmpname);

   /* 
     * work around a new BUG (?) in xilinx tools; they're sticking debug or some such in
     * the #@*&ing xilinx header nowadays (something like EP2SGS30D;DEBUG_HANDLE=2)
     */
    if ((p = strchr((char *)bfh->ncdname, ';')) != NULL)
    {
	edt_msg(DEBUG2, "FIXING: Xh.name %s => ", bfh->ncdname);
	*p = '\0';
	edt_msg(DEBUG2, "%s\n", bfh->ncdname);
    }
    if ((p = strchr((char *)bfh->id, ';')) != NULL)
    {
	edt_msg(DEBUG2, "FIXING: Xh.id %s => ", bfh->id);
	*p = '\0';
	edt_msg(DEBUG2, "%s\n",bfh->id);
    }

    for (i=0; i < 5; i++)
    {
	if (bfh->fi[i] != 'a' + i)
	{
	    edt_msg(EDT_MSG_FATAL, "invalid fpga field id: '%c' (0x%02x) s/b '%c' (0x%02x)\n", bfh->fi[i], bfh->fi[i], 'a'+i, 'a'+i);
	    return -1;
	}
    }

    edt_msg(DEBUG2, "%c) '%s'  %c) '%s'  %c) '%s'  %c) '%s'  %c) %d\n",
	bfh->fi[0], bfh->ncdname,
	bfh->fi[1], bfh->id,
	bfh->fi[2], bfh->date,
	bfh->fi[3], bfh->time,
	bfh->fi[4], bfh->dsize);

    /*
    * check remaining size of file against dsize, then rewind file
    * pointer back to start of data 
    */


    /* if header is odd length and Altera, the data will be even */ 
    if ((edt_bitfile_tell(bp) % 2) && ((bfh->magic == AlteraMagic) || (bfh->magic == Altera2Magic)))
        edt_bitfile_read(bp, dmy, 1);

    /* find preamble */
    bfh->data_start = edt_bitfile_tell(bp);

    edt_bitfile_read(bp, bfh->extra, BFH_EXTRASIZE);

    remaining = edt_bitfile_size(bp) - bfh->data_start;
    bfh->filesize = edt_bitfile_size(bp);

    if (bfh->dsize < (unsigned long)(remaining) && bfh->dsize + 12 < (unsigned long)(remaining) )
	edt_msg(EDT_MSG_WARNING, 
        "Warning! dsize/file size mismatch (dsize %u, EOF @ +%u (may work anyway)\n", 
        bfh->dsize, remaining);
#if 0 /* FIX THIS -- throws error on rcxload but should be fixed */
    else if (bfh->dsize > (unsigned long)(remaining))
	edt_msg(EDT_MSG_FATAL, 
        "Error! dsize/file size mismatch -- dsize %u but EOF at +%u\n", 
        bfh->dsize, remaining);
#endif

    edt_bitfile_seek(bp, 0);

    sprintf(bfh->promstr, "%s %s %s %s", bfh->ncdname, bfh->id, bfh->date, bfh->time );

    return bfh->data_start;
}

/*
 * get the FPGA header from a FILE -- backwards compat (no magic #).
 *
 * Takes as an argument an already open file pointer. for this reason, not to be
 * called directly by an application, ony from within the library. Application 
 * programs should instead use edt_get_bitfile_header.
 *
 * At the end of this function, the file position will be set to the
 * beginning of the file.
 *
 * @return 0 on success, -1 on failure.
 */
int
edt_bitfile_read_header(const char *bitpath, EdtBitfileHeader *bfh, char *header)

{
    int     ret;
  
    EdtBitfile bfd;

    edt_bitfile_init(&bfd);

    if (edt_bitfile_open_file(&bfd, bitpath, EDT_READ) != 0)
    {
        edt_msg_perror(DEBUG2, bitpath);
        return -1;
    }

    ret = edt_get_bitfile_header(&bfd, bfh);

    edt_bitfile_destroy(&bfd);

    strncpy(bfh->filename, bitpath, sizeof(bfh->filename)-1);
    strncpy(header, bfh->promstr, sizeof(bfh->promstr)-1);
    
    return ret;
}

/*
 * Get the FPGA header from a FILE.
 *
 * takes as an argument an already open file pointer. for this reason, not to be
 * called directly by an application, ony from within the library.
 *
 * At the end of this function, the file position will be set to the
 * beginning of the file.
 *
 * Returns 0 on success, -1 on failure.
 */

int
edt_get_x_file_header(const char *bitname, char *header, int *size)

{
    EdtBitfileHeader bfh;

    int offset = edt_bitfile_read_header(bitname, &bfh, header);

    *size = bfh.filesize;

   return offset;
}

/*
 * Get the FPGA header from an array (typically as read out from a hardware FPGA).
 *
 * @param ba Array containing data bytes presumably read from the FPGA.
 * @param header string that will be populated with the header info in a printable format.
 * @param size size of the formatted string (header)
 *
 * @returns 0 on success, -1 on failure.
 */
int
edt_get_x_array_header(u_char *ba, 
                       char *header, 
                       int *size)

{
    EdtBitfile bfd;
    int ret;

    edt_bitfile_init(&bfd);

    bfd.full_buffer = ba;

    bfd.full_buffer_size = *size;

    bfd.cur_index = 0;

    ret = edt_get_bitfile_header(&bfd, &bfd.hdr);

    if (ret != -1)
    {
        strncpy(header, bfd.hdr.promstr, sizeof(bfd.hdr.promstr)-1);  
        return 0;
    }

    return -1;

}

/* All existing boards */

EdtBoardFpgas board_chips[] = 
{

    {
        PDV_ID,
        {"4005","4010", "4013"} 
    },
    {
        PCD20_ID,
        {"4005","4010", "4013"} 
    },
    {
        PCD40_ID,
        {"4005","4010", "4013"} 
    },
    {
        PCD60_ID,
        {"4005","4010", "4013"} 
    },
    {
        PDVK_ID,
        {"4005","4010", "4013"} 
    },
    {
        PDV44_ID,
        {"4005","4010", "4013"} 
    },
    {
        PDVAERO_ID,
        {"4005","4010", "4013"} 
    },
    {
        PGP20_ID,
        {"4036", "4085"}
    },
    {
        PGP40_ID,
        {"4036", "4085"}
    },
    {
        PGP60_ID,
        {"4036", "4085"}
    },
    {
        PGP_THARAS_ID,
        {"4036", "4085"}
    },
    {
        PGP_ECL_ID,
        {"4036", "4085"}
    },
    {
        PCD_16_ID,
        {"4036", "4085"}
    },
    {
        PSS4_ID,
        {"XCV600E", "XCV1000E", "XCV2000E"}
    },
    {
        PSS16_ID,
        {"XCV600E", "XCV1000E", "XCV2000E"}
    },
    {
        PCDA_ID,
        {"XC2S100E", "XC2S600E"}
    },
    {
        PCDA16_ID,
        {"XC2S100E", "XC2S600E"}
    },
    {
        PDVA_ID,
        {"xc2s100e", "xc2s600e"}
    },
    {
        PDVCL2_ID,
        {"xc2s400e"}
    },
    {
        PDVFOX_ID,
        {"xc2s400e", "xc2vp2"}
    },
    {
        PCDFOX_ID,
        {"XC2S400E"}
    },
    {
        PGS4_ID,
        {"XC2VP50", "XC2VP70"}
    },
    {
        PGS16_ID,
        {"XC2VP50", "XC2VP70"}
    },
    {
        PE8LX16_ID,
        {"XC5VLX110T",  "XC5VLX220T", "XC5VLX330T", "XC5VFX100T",
         "XC5VFX130T", "XC5VFX200T","XC5VSX240T"}
    },
    {
        PE16G3_OCTEON3_ID,
        {"PE16G3_OCTEON3_ID_SOMETHING"} /* ALERT: placeholder */
    },
    {
        PE8LX32_ID,
        {"XC5VLX110T",	"XC5VLX220T", "XC5VLX330T", "XC5VFX100T",
         "XC5VFX130T", "XC5VFX200T","XC5VSX240T"}
    },
    {
        PE8G2V7_ID,
        {"XVIRTEX7_SOMETHING"} /* ALERT: placeholder */
    },
    {
        PE8LX16_LS_ID,
        {"XC5VLX110T",  "XC5VLX220T", "XC5VLX330T", "XC5VFX100T",
         "XC5VFX130T", "XC5VFX200T","XC5VSX240T"}
    },
    {
        PE4AMC16_ID,
        {"XC6VLX240T",  "XC6VLX365T", "XC6VLX550T"}
    },
    {
        0
    }

};


EdtBoardFpgas mezz_chips[] = { /* in the order of the board_t enum */
    { 
        MEZZ_OCM,     
        {"XC2VP4",0},
        {"XC3S200",0}
    }, /* OCM (w/o -n) */
    { 
        MEZZ_OC192 ,
        {"XC4VLX40",0}
    }, /* OC192 */
    { 
        MEZZ_SSE,        
        {"XC2VP2",0}
    }, 
    {
        MEZZ_SRXL,
        {"XC3S1500",0}
    }, /* SRXL (4 channel) */
    {
        MEZZ_SRXL2, 
        {"XC4VSX55",0}
    }, /* SRXL2 (16 channel) */
    { 
        MEZZ_3X3G,        
        {"XC2VP30",0}
    }, /* 3X3g */
    { 
        MEZZ_MSDV,    
        {"XC5VLX30T",0} 
    }, /* MSDV */
    {
        MEZZ_NET10G,
        {"XC5VLX110","XC5VLX220",0},
        {"XC3S100E",0}
    }, /* net10g */
    {
        MEZZ_DDSP,
        {"XC5VFX30T",0},
        {"XC5VLX30T",0}
    }, /* ddsp */
    {
        MEZZ_SRXL2_IDM_LBM, 
        {"XC4VSX55",0}
    }, /* SRXL2 (16 channel) */
    {
        MEZZ_SRXL2_IDM_IDM, 
        {"XC4VSX55",0}
    }, /* SRXL2 (16 channel) */
    {
        MEZZ_SRXL2_IMM_IMM, 
        {"XC4VSX55",0}
    }, /* SRXL2 (16 channel) */
    {
        MEZZ_SRXL2_IMM_LBM, 
        {"XC4VSX55",0}
    }, /* SRXL2 (16 channel) */
    {
        MEZZ_SRXL2_IDM_IMM, 
        {"XC4VSX55",0}
    }, /* SRXL2 (16 channel) */
    {
        MEZZ_DRX16, 
        {"XC6VLX240T",0}
    }, /* DRX16 (16 channel) */
    {
        MEZZ_OCM2P7G, 
        {"XC6VLX240T",0}
    }, /* OCM2P7G */ /* TEMP: SB: */
    {
        MEZZ_THREEP, 
        {"XC6VLX240T","XC6VLX365T","XC6VSX475T", "XC6VSX315T", 0}
    }, /* DRX16 (16 channel) */
    {
        MEZZ_V4ACL, 
        {"XC4VLX160", NULL},
        {"XC4VLX160", NULL}
    }, /* V4ACL mezzanine */
    {
        MEZZ_UNKN_EXTBDID,
        {0}
    }
};

char ** edt_lookup_fpgas(EdtBoardFpgas *fpgas, int id, int channel)

{
    int i;

    for (i=0;fpgas[i].id;i++)
    {
        if (fpgas[i].id == id)
            return (channel)?
                fpgas[i].fpga_1:fpgas[i].fpga_0;
    }

    return NULL;
}


char ** edt_lookup_ui_names(EdtDev *edt_p)

{
    return edt_lookup_fpgas(board_chips, edt_p->devid, 0);
}

char ** edt_lookup_mezz_names(EdtDev *edt_p, int channel)

{
    return edt_lookup_fpgas(mezz_chips, edt_p->mezz.id, channel);
}

/** lookup an id in the list,
    then if fpga_hint is non-NULL, put it at the front of the list
*/


int
edt_get_sorted_fpga_names(EdtBoardFpgas *fpga_list, 
                          int id, 
                          int channel,
                          char *fpga_hint,
                          char **sorted)
{
    int first_choice = 0;
    int i;

    char ** possibles = edt_lookup_fpgas(fpga_list, id, channel);

    if (possibles == NULL)
    {
        sorted[0] = NULL;
        return -1;
    }

    for (i=0;possibles[i];i++)
    {
        sorted[i] = possibles[i];
        if (fpga_hint && !strcasecmp(fpga_hint, possibles[i]))
            first_choice = i;
    }

    /* finish list with NULL */

    sorted[i] = NULL;

    if (first_choice != 0)
    {
        char *tmp;

        tmp = sorted[0];
        sorted[0] = sorted[first_choice];
        for (i=1;i<=first_choice;i++)
        {
            char *tmp2 = sorted[i];
            sorted[i] = tmp;
            tmp = tmp2;
        }                
    }

    return 0;
}

/*
* get files and sizes from all subdirs in a given dir
* 
* return 0 on success, -1 on failure
*/


static int
bf_get_possible_files(const char *basedir, 
                    const char *devdir, 
                    const char *fname,
                         EdtBitfileList *bf,
                         char **match_list
                         )
{

    char    dirpath[MAXPATH];

    sprintf(dirpath, "%s%s%s", basedir, (devdir && (*devdir)) ? "/" : "", devdir);

    if (match_list)
    {
        int i = 0;

        while (match_list[i])
        {
           static char    tmppath[MAXPATH];

            /* ALERT: check if d_name is directory here! */
            /* See stat (2) and stat (5) */
            sprintf(tmppath, "%s/%s/%s", dirpath, match_list[i], fname);

            bf_check_and_add(bf, tmppath);

            i++;
        }
    }
    else
    {
        edt_msg(EDT_MSG_FATAL, "No FPGA match...\n");
        return -1;

    }

    return 0;
}

/**
 *
 * Assumes bf has been initialized
 */


int edt_get_bitfile_list(const char *basedir,
                         const char *devdir,
                         const char *fname,
                         EdtBoardFpgas *fpga_list,
                         int id,
                         int channel,
                         EdtBitfileList *bf,
                         char *fpga_hint)

{
    int fullpath;

    fullpath = bitload_has_slashes(fname);

    if (fullpath)
    {
         bf_check_and_add(bf, fname);
    }
    else
    {
        char    tmppath[MAXPATH];

        char * sorted[MAX_CHIPS_PER_ID];
 
        if (id)
        {
            if (edt_get_sorted_fpga_names(fpga_list, 
                              id, 
                              channel,
                              fpga_hint,
                              sorted) != 0)
            {
                edt_msg(EDT_MSG_FATAL, 
                    "1: Unable to determine FPGA for id 0x%02x\n",
                    id);
                return -1;
            }
        }
     
       /* dir/file */
        sprintf(tmppath, "%s/%s", basedir, fname);

        bf_check_and_add(bf, tmppath);

        if (id)
        {

            /* dir[/devdir]/subdirs.../file */
            bf_get_possible_files(basedir, devdir, fname, bf, sorted);
     
            /* dir[/devdir]/bitfiles/subdirs.../file */
            sprintf(tmppath, "%s/bitfiles", basedir);
            edt_correct_slashes(tmppath);
            bf_get_possible_files(tmppath, devdir, fname, bf, sorted);

        }
    }

    return (bf->nbfiles)?0:-1;
}

/*******************************
 * BitfileList routines
 *
 */

void bf_init(EdtBitfileList *bf)

{
    bf->nbfiles = 0;
    bf->bitfiles = NULL;

}

void bf_destroy(EdtBitfileList *bf)

{
    if (bf->bitfiles)
    {
        free(bf->bitfiles);
    }

    bf_init(bf);
}

void bf_add_entry(EdtBitfileList *bf, 
                  EdtBitfileHeader *bfh)

{
    if (bf)
    {
        if (bf->bitfiles)
            bf->bitfiles = (EdtBitfileHeader *) realloc(bf->bitfiles,(bf->nbfiles + 1) * 
                sizeof(EdtBitfileHeader));
        else
            bf->bitfiles = (EdtBitfileHeader *)calloc(1,sizeof(EdtBitfileHeader));

        memcpy(&bf->bitfiles[bf->nbfiles],
            bfh, sizeof(EdtBitfileHeader));

        bf->nbfiles++;
    }
}

void bf_check_and_add(EdtBitfileList *bf, const char *fname)

{
    int found_bitfile;
    char header[256];
    EdtBitfileHeader hdr;

    edt_msg(DEBUG2, "Checking existence of %s\n", fname);

    found_bitfile = (edt_access_bitfile(fname, 0) == 0);

    if (found_bitfile)
    {
        if (edt_bitfile_read_header(fname,&hdr,header) >= 0)
        {
            bf_add_entry(bf, &hdr);
            edt_msg(DEBUG2, "Adding %s\n", fname);
        }
    }
}


int bf_allocate_max_buffer(EdtBitfileList *bf, EdtBitfile *data)

{
    int i;
    u_int size = 0;

    for (i=0;i<bf->nbfiles;i++)
         if (size < bf->bitfiles[i].filesize)
             size = bf->bitfiles[i].filesize;

    return edt_bitfile_allocate(data, size);
}

void
ensure_bitfile_name(const char *name, char *bitname)

{
    if ((strlen(name) < 4)
     || (strcasecmp(&(name[strlen(name)-4]), ".bit") != 0))
	sprintf(bitname, "%s.bit", name);
    else strcpy(bitname, name);

}


int
edt_get_fpga_hint(EdtDev *edt_p, char *bd_xilinx)

{
    char    esn[128];
    Edt_embinfo ei;

    bd_xilinx[0] = 0;

    edt_get_esn(edt_p, esn);
    if (edt_parse_esn(esn, &ei) == 0)
    {
        /* if xxx-xxxxx-00, replace last 2 digits with rev */
        if ((strcmp(&(ei.pn[8]), "00") == 0))
            sprintf(&(ei.pn[8]), "%02d", ei.rev);
        if (*ei.ifx)
            strcpy(bd_xilinx, ei.ifx);
        else if (edt_find_xpn(ei.pn, bd_xilinx))
            edt_msg(DEBUG2, "matching xilinx found: pn <%s> xilinx <%s>\n", ei.pn, bd_xilinx);
        else edt_msg(DEBUG2, "no matching xilinx found: for pn <%s>\n", ei.pn);
    }
    else edt_msg(DEBUG2, "missing or invalid pn/xilinx, can't xref (esn <%s>)\n", esn);

    /* success if there's sometjiong in bd_xilinx */

    return bd_xilinx[0]? 0: -1;
}




/** Appends a '2' to the bitname if the OC192 Mezzanine board rev is > 1.
* @param edt_p The EDT board structure for the unit with the OC192
* mezzanine attached.
* @param hacked_bitname A string into which the new name will be put
* (should be at least 256 characters long).
* @param bitname the generic bitfile name, with or without the absolute
* path.
* @return 0 on success, -1 on failure.
*/
int edt_oc192_rev2_fname_hack(EdtDev *edt_p, const char *bitname, char *hacked)
{
    char work[256];
    int rev_id;

    edt_get_full_board_id(edt_p, NULL, &rev_id, NULL);


    /* hack to deal with rev 2 boards */

    if (rev_id > 1)
    {
	size_t len = strlen(bitname);

	if (len < 4 || strcasecmp(bitname + len - 4, ".bit"))
	{ /* bitname doesn't end in .bit */
	    if (bitname[len-1] != '2') /* bitname doesn't have rev_id */
		sprintf(work,"%s2",bitname);
	    else
		strcpy(work, bitname);
	}
	else
	{ /* bitname does end in .bit */
	    if (bitname[len-5] != '0' + 2) /* bitname doesn't have rev_id */
	    {
		if (hacked != bitname)
                    strcpy(hacked,bitname); /* hacked = foo.bit */

		hacked[len-4] = 0;  /* hacked = 'foo' */
		sprintf(work,"%s2.bit",hacked); /* work = foo2 */
	    }
	    else /* bitname is foo2.bit - what we want*/
		strcpy(work,bitname);
	}
    }
    else /* board is rev1. we assume bitname is in correct form (has .bit at the end) */
	strcpy(work, bitname);

    strcpy(hacked,work);

    return 0; /* no problems. */
}

int
edt_bitload_select_fox_file(EdtDev *edt_p,
                            char *rbtfile)
                            /* Kludge to address the tlk1 vs. tlk4 issue */
{
    char name[MAX_STRING+1];
    int pci_channels;
    char bitname[128];
    size_t len;

    edt_flash_get_fname(edt_p, name);

    edt_msg(DEBUG1, "Looking at board pci id = %s\n", name);

    if (!strcmp(name, "dvtlk1"))
    {
        pci_channels = 1;
    }
    else if (!strcmp(name, "dvtlk4"))
    {
        pci_channels = 4;
    }
    else 
    {
        edt_msg(EDT_MSG_FATAL, "DVFOX pci bitfile <%s> not dvtlk4 or dvtlk4\n", name);
        return -1;
    }

    /* split file name to remove .bit */
    strcpy(bitname, rbtfile);
    edt_msg(DEBUG1, "Expand bitfile name %s -> ", bitname);
    len = strlen(bitname);
    if ((len >= 4) && (strcasecmp(&bitname[len-4], ".bit") == 0))
    {
        bitname[len-4] = '\0';

        sprintf(rbtfile,"%s%d.bit", bitname,pci_channels);


    }
    edt_msg(DEBUG1, "%s\n", rbtfile);

    return 0;
}


/**
* @return 0 if bitfile available, including uncompressing
* analogous to edt_access
*/

int
edt_access_bitfile(const char *path, int perm)

{
    int found_bitfile = 0;
    int     found_compressed = 0 ;
    static char    tmppath[MAXPATH];
    static char    ztmppath[MAXPATH];
    static char    cmd[MAXPATH];

    strcpy(tmppath, path);

    edt_correct_slashes(tmppath);

    if (edt_access(tmppath, perm) == 0)
        found_bitfile = 1 ;

    else /* Check for compressed version of bitfile (.Z or .gz) */
    {
        strcpy(ztmppath, tmppath);
        strcat(ztmppath, ".Z");

        if (edt_access(ztmppath, perm) == 0)
        {
            sprintf(cmd, "uncompress %s", tmppath) ;
            edt_msg(DEBUG1, "%s\n", cmd);
            edt_system(cmd) ;
            found_compressed = 1 ;
        }

        if (!found_compressed)
        {
            strcpy(ztmppath, tmppath);
            strcat(ztmppath, ".gz");
            if (edt_access(ztmppath, perm) == 0)
            {
                sprintf(cmd, "gunzip %s", tmppath) ;
                edt_msg(DEBUG1, "%s\n", cmd);
                edt_system(cmd) ;
                found_compressed = 1 ;
            }
        }

        if (!found_compressed)
        {
            strcpy(ztmppath, tmppath);
            strcat(ztmppath, ".zip");
            if (edt_access(ztmppath, perm) == 0)
            {

                size_t  i;
                i = strlen(ztmppath)-1;

                while (i && ztmppath[i] != '/' && ztmppath[i] != '\\')
                    i--;

                if (i)
                {  
                    char *fname; 
                    char ch = ztmppath[i];
                    ztmppath[i] = 0;
                    fname = ztmppath + i + 1;

                    sprintf(cmd,"cd %s ; unzip %s",
                        ztmppath, fname);

                    ztmppath[i] = ch;

                }
                else
                    sprintf(cmd, "unzip %s", ztmppath);

                edt_correct_slashes(tmppath);
                edt_msg(DEBUG1, "%s\n", cmd);
                edt_system(cmd) ;
                edt_msg(DEBUG1, "rm %s\n", ztmppath) ;
                unlink(ztmppath) ;
                found_compressed = 1 ;
            }
        }

        if (found_compressed && edt_access(tmppath, perm) == 0)
            found_bitfile = 1 ;
    }

    /* return 0 for success, so it matches edt_access */

    return (found_bitfile) ? 0 : -1;
}


int
bitload_has_slashes(const char *name)
{
    const char   *p = name;

    while (*p)
    {
        if ((*p == '/') || (*p == '\\'))
            return 1;
        ++p;
    }
    return 0;
}


/*
* fill in the devdir string based on the device ID
*/


const char *
edt_bitload_basedir(EdtDev *edt_p, const char *in, char *out)

{
    if (in)
        return in;
    else
    {
        const char * home = edt_home_dir(edt_p);

        switch (edt_p->devtype)

        {
        case pcd:
                
            if (out)
                sprintf(out, "%s", home);
              
            break;

        case pdv:

            if (out)
                sprintf(out, "%s/camera_config", home);
            break;

        }

        return (out)?out:home;
    
    }
}


void
edt_bitload_devid_to_bitdir(int id, char *devdir)
{
    switch(id)
    {
    case PDV44_ID:
    case PDVK_ID:
        strcpy(devdir, "dvk");
        break;

    case PDV_ID:
    case PGP_RGB_ID:
        strcpy(devdir, "dv");
        break;

    case PDVA_ID:
    case PDVA16_ID:
        strcpy(devdir, "dva");
        break;

    case PDVFOX_ID:
        strcpy(devdir, "dvfox");
        break;

    case PDVFCI_AIAG_ID:
    case PDVFCI_USPS_ID:
        strcpy(devdir, "dvfci");
        break;

    case PDVAERO_ID:
        strcpy(devdir, "dvaero");
        break;

    case PDVCL2_ID:
        strcpy(devdir, "dvcl2");
        break;

    default:
        devdir[0] = '\0';
    }
}
/**
* Searches for and loads a gate array bit file into an EDT
* PCI board.  Searches under \<basedir\>/bitfiles/xxx, or if a PCI DV, in the
* appropriate sub-directory (\<basedir\>/bitfiles/dv/.../\<file\>.bit or
* \<basedir\>/bitfiles/dvk/.../\<file\>.bit. '...' stands for all the subdirs
* found under the base path.)  Quits after the first successful load.
* 
* @param edt_p	device handle returned from #edt_open
* @param basedir	base directory to start looking for the file
* @param name     name of the bitfile to load
* @param flags	misc flag bits -- should be combination of
*        BITLOAD_FLAGS_* which are defined in edt_bitload.h.
*        (This variable was formerly rcam which is obsolete.)
* @param skip	if nonzero, don't actually load, just find the files (debugging)
*
* @return 0 on success, -1 on failure
*/

int
edt_bitload(EdtDev *edt_p, 
            const char *indir, 
            const char *name, 
            int flags, 
            int skip)
{

    int     nofs = 0, ovr=0;
    int i;

    int     fullpath;

    int     do_sleep = 0;
    int     do_readback = 0;

    char    devdir[MAXPATH];
    char    bitpath[MAXPATH];
    char    basework[MAXPATH];

    const   char *basedir;

    EdtBitfileList boardfiles;

    int channel = 0;
    int rc = 1;
    char *hint = NULL;
    char fpga[32];

    u_char *ba;


    if ( ID_HAS_COMBINED_FPGA(edt_p->devid) )

	return 0;	// Nothing to do and no error.


    if (flags & BITLOAD_FLAGS_NOFS)
        nofs = 1;
    if (flags & BITLOAD_FLAGS_OVR)
        ovr = 1;
    if (flags & BITLOAD_FLAGS_SLEEP)
        do_sleep = 1;
    if (flags & BITLOAD_FLAGS_READBACK)
        do_readback = 1;

    edt_ioctl(edt_p, EDTS_PROG_READBACK, &do_readback);

    basedir = edt_bitload_basedir(edt_p, indir, basework);

#ifdef PCD

    if (flags & (BITLOAD_FLAGS_MEZZANINE | BITLOAD_FLAGS_OCM | BITLOAD_FLAGS_SRXL))
    {
        if (flags & BITLOAD_FLAGS_CH1)
            channel = 1;

        return edt_load_mezzfile(edt_p, basedir, name, flags, skip, channel);
    }

#endif


    /* added so the FOX HACK can modify the file name */

    /* make sure name ends in .bit */
    ensure_bitfile_name(name, bitpath);

    /* FOX HACK - we need to change the name to 
    match the PCI bitfile */

    if (edt_p->devid == PDVFOX_ID)
    {

        if (!strcmp(bitpath,"aiag.bit") ||
            !strcmp(bitpath,"aiagcl.bit"))
            if (edt_bitload_select_fox_file(edt_p,
                bitpath) == -1)
                return -1;
    }
    /* special case nofs  -- bitpath isn't really bitpath but instead pointer to data */

    edt_get_fpga_hint(edt_p, fpga);

    if (fpga[0] != 0)
        hint = fpga;

    if (nofs)
    {
        char nofsname[MAXPATH];
        int size;
        int found_one = FALSE;

        char * sorted[5];
 
        int which = 0;

        if (edt_get_sorted_fpga_names(board_chips, 
                          edt_p->devid, 
                          0,
                          hint,
                          sorted) != 0)
        {
            edt_msg(EDT_MSG_FATAL, 
                "2: Unable to determine FPGA for id 0x%02x\n",
                edt_p->devid);
            return -1;
        }

        which = 0;

        while (sorted[which])
        {
            sprintf(nofsname, "%s_%s", sorted[which], bitpath);

            {
                int len = strlen(nofsname);
                if ((len >= 4) && (strcasecmp(&nofsname[len-4], ".bit") == 0))
                    nofsname[len-4] = '\0';
            }

            MAPBITARRAY(nofsname, ba, size);

            if (ba != NULL)
            {

                found_one = TRUE;

                edt_msg(DEBUG2, "edt_bitload(basedir=%s array=%x nofs=%d flags=0x%x skip=%d)\n",
                    basedir, bitpath, nofs, flags, skip);

                rc = edt_program_flash(edt_p, 
                    ba,
                    size,
                    do_sleep);

                if (rc == 0)
                {
                    edt_set_bitpath(edt_p, bitpath) ;
                    edt_set_mezz_bitpath(edt_p, "");
                    break;
                }

                

            }

            which ++;
        }

        if (found_one == FALSE)
        {
            edt_msg(EDT_MSG_FATAL, "No matching bitfiles found\n");
            return -1;
        }

        return rc;
    }

    edt_msg(DEBUG2, "edt_bitload(basedir=%s bitpath=%s nofs=%d flags=0x%x skip=%d)\n",
        basedir, bitpath, nofs, flags, skip);

    edt_bitload_devid_to_bitdir(edt_p->devid, devdir);

    fullpath = bitload_has_slashes(bitpath);
 
    edt_set_bitpath(edt_p, "") ;
     
    bf_init(&boardfiles);

    if ((rc = edt_get_bitfile_list(
            basedir,  devdir , bitpath, 
            board_chips, 
            edt_p->devid, 
            channel, 
            &boardfiles, hint)) == 0)
    {
        EdtBitfile data;
        edt_bitfile_init(&data);
        
        bf_allocate_max_buffer(&boardfiles, &data);

        for (i=0;i<boardfiles.nbfiles;i++)
        {
            char *bitfile = boardfiles.bitfiles[i].filename;

            if (edt_bitfile_load_file(&data,bitfile) == 0)
            {
                rc =  edt_program_flash(edt_p, 
                        data.data,
                        data.data_size,
                        do_sleep);

		edt_msg(DEBUG1, "Tried %s result = %s\n",
			bitfile, (rc == 0) ? "success": "failure");
                if (rc == 0)
                {
                    edt_set_bitpath(edt_p, bitpath) ;
                    edt_set_mezz_bitpath(edt_p, "");
                    break;
                }

            }
        }
        
        edt_bitfile_destroy(&data);

    }
    if (rc != 0)
    {
        edt_msg(EDTAPP_MSG_FATAL, "bitload(name=%s ) failed\n", bitpath);
        if (edt_get_verbosity() < 2)
            edt_msg(EDTAPP_MSG_FATAL, " (run with -v to look for the cause of the failure)\n");
        else edt_msg(EDTAPP_MSG_FATAL, "\n");
        bf_destroy(&boardfiles);
        return rc;
    }
    else 
    {
        edt_correct_slashes(bitpath);
        edt_msg(EDTAPP_MSG_WARNING,"Loaded: %s %s\n", bitpath, boardfiles.bitfiles[i].promstr);
    }
    bf_destroy(&boardfiles);
    return rc;
}

/**
 * Program the interface fpga flash PROM. Typically only called by edt_bitload.
 * 
 * @param edt_p pointer to edt device structure returned by #edt_open 
 * @param buf buffer containing the data to be loaded
 * @param size number of bytes to load from buffer
 * @do_sleep add sleeps to slow it down in case of host speed issues
 * @return 0 on success, nonzero on failure
 */
int
edt_program_flash(EdtDev *edt_p, const u_char *buf, int size, int do_sleep)
{

#ifdef DO_DIRECT_LOAD
    return edt_program_flash_direct(edt_p, buf, size, do_sleep);
#else

    int ret;
    buf_args args;

    memset(&args, 0, sizeof(args));
    args.addr = (uint64_t) (buf);
    args.size = size;
    args.index = do_sleep;

    ret = edt_ioctl(edt_p, EDTS_BITLOAD, &args);

    if (args.index == EINVAL)
        puts("Unable to load %s, failed key check.\nThis board doesn't appear to have the correct key loaded for the bitfile\n");

    return args.index;
#endif

}


/*
 * SPECIAL CODE only for loading from prom (currently only PMC FOX
 * for SLAC).
 * Loads part or all of a bitfile, reading out of the PROM, starting
 * at a given pronm address, called by edt_bitload_from_prom which also
 * does the bitload preamble
 */
static int
bitload_prom_loadit(EdtDev *edt_p, u_int addr, int size, u_char *out_addr)
{

    int i;
    u_int addr_p = addr;
    u_char *buf_p = out_addr;

    /* read/program */
    edt_msg(DEBUG1, "\naddr_p %06x\n", addr_p);
    for (i=0;i<size; i++)
    {
	*buf_p = edt_flipbits(edt_flash_read8(edt_p, addr_p, FTYPE_BT));
	/* ALERT: hard-coded ftype */

#if DUMP_HEX
	if (!(i%16))
	    printf("\n%07d ", i);
	printf("%02x%s", *buf_p, i%2?" ":"" );
#endif

	++addr_p;
	++buf_p;

    }



    return 0;
}



/**
 * Bitload from a given address in the PCI PROM.
 *
 */
int
edt_bitload_from_prom(EdtDev *edt_p, u_int addr1, int size1, u_int addr2, int size2, int flags)
{
    int ret, do_sleep, do_readback = 0;
    int size = size1 + size2;

    u_char *buffer;

    if (flags & BITLOAD_FLAGS_SLEEP)
	do_sleep = 1;
    if (flags & BITLOAD_FLAGS_READBACK)
	do_readback = 1;

    edt_ioctl(edt_p, EDTS_PROG_READBACK, &do_readback);

    /* allocate a single buffer for all data */

    buffer = edt_alloc(size);

    /* read from prom */


    if (bitload_prom_loadit(edt_p, addr1, size1, buffer) != 0)
    	return -1;
    if (bitload_prom_loadit(edt_p, addr2, size2, buffer + size1) != 0)
    	return -1;

    /* call ioctl to program */

    ret = edt_program_flash(edt_p, buffer, size, do_sleep);

    edt_free(buffer);

    return 0;
}


