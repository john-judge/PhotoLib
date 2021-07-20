/*
 * rcxmake
 *
 * usage: rcxmake outfile cmdfile bitfile0 bitfile1 ... bitfileN
 *
 * create an rcxload file from a command file and 1 or more xilinx bitfiles
 *
 * Each file starts out with 64 bytes of arbitrary data, could be an
 * ASCII description of what is in the file. The first 32 bits is
 * a std magic number for unix, selected to not confuse vi ("RCX0")
 *
 * RCXLOAD file format (fname.rcx):
 *
 * HEADER: 4 bytes magic number: "RCX\0" (0x42 0x43 0x58 0x00)
 *         60 bytes arbitrary data (ASCII description)
 *
 * BLOCK0: 4 byte magic #:  0xASC1C0DE for ASCII cmd block, 0x0101C0DE for binary
 *         44 bytes of ASCII comment
 *         4 byte bytecount
 *         4 byte checksum (CRC?)
 *         8 bytes spare (currently zero)
 *         binary data of length specified above in bytecount
 *
 * BLOCK1 (same as above)
 *    .
 *    .
 *    .
 * BLOCKN  (last block)
 *
 * see rcxload.c for command syntax
 *
 */
#include "edtinc.h"
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define RCXMAGIC "RCX\0"
#define MAGIC_ASC 0xA5C1C0DE
#define MAGIC_BIN 0x0101C0DE

typedef struct {
    unsigned int magic;
    char name[44];
    unsigned int bytecount;
    unsigned int cksum;
    u_char spare[8];
} BLOCK_HEADER;
BLOCK_HEADER block;

char hbuf[65];
unsigned char *bindata;

typedef struct {
    char name[128];
    char dir[256];
} BFNAME ;

BFNAME bflist[16];
int bf_count = 0;

/*
 * back_to_fwd: correct windows goofy back slashes
 */
void
back_to_fwd(char *str)
{
    char   *p = str;

    while (*p != '\0')
    {
	if (*p == '\\')
	    *p = '/';
	++p;
    }
}

int
binchars(u_char *data, int count)
{
    int i, na=0;

    for (i=0; i<count; i++)
	if (((data[i] < 0x20) || (data[i] > 0x7e))
	 && (data[i] != '\n') && (data[i] != '\r') && (data[i] != '\t'))
	    ++na;

    return na;
}

void
error_out(char *msg)
{
    printf("%s, exiting\n", msg);
    exit(1);
}

int gettok(char *tbuf, char **pbufp)		/* use strtok_r() instead? */
{
    int c;

    do   {  
		c = *(*pbufp)++;   
		if (c=='\0')  { *tbuf = 0;  return(0);  }
    } while (isspace(c));

    do  { 
		*tbuf++ = c;
		c = *(*pbufp)++;   
		if (c=='\0')  { *tbuf = 0; return(1);  }
    }  while (!isspace(c));

    *tbuf=0;
    return(1);
}

void
usage(char *progname, char *errmsg)
{
    puts(errmsg);
    printf("usage: %s [opts] <cmdfile> <outfile> [binfile0 binfile1 ... binfileN]\n", progname);
    printf("  -d <dir> directory path for bitfiles (default ./ )\n");
    printf("  -y       'yes' to overwrite outfile without asking\n");
}

main(int argc, char **argv)
{
    int i, bfi, n;
    int bc, bn=0, nf=0;
    int cmdfile = 1;
    int overwrite_yes=0;
    FILE *ofp, *bfp;
    char *cmdfname;
    char cmdpath[256];
    char tmpstr[256], dirname[256];
    char idstr[256], resp[256], name[256];
    char *progname = argv[0], *ofname;
    time_t	ttime;
    struct tm *ltime;

    --argc;
    ++argv;

    strcpy(dirname, ".");
    strcpy(tmpstr, "unknown");

    while (argc && ((argv[0][0] == '-')))
    {
	switch (argv[0][1])
	{
	case 'y':		/* device unit number */
	    overwrite_yes = 1;
	    break;

	case 'd':		/* directory name */
	    ++argv;
	    --argc;
	    strcpy(dirname, argv[0]);
	    break;

	default:
	case '?':
	case 'h':
	    sprintf(tmpstr, "unknown flag -'%c' (%s)\n", argv[0][1], argv[0]);
	    usage(progname, tmpstr);
	    exit(0);
	}
	argc--;
	argv++;
    }

    if (argc < 1)
    {
	usage(progname, "missing command (.cmd) and output (.rcx) filename arguments");
	exit(0);
    }
    else if (argc < 2)
    {
	usage(progname, "missing output (.rcx) filename argument");
	exit(1);
    }
    else 
    {
	cmdfname = argv[0];
	++argv;
	--argc;
	ofname = argv[0];
	++argv;
	--argc;

	printf("cmdfname %s ofname %s\n", cmdfname, ofname);
    }

    /* check for output file already there; ask to overwrite if it is (unles 'y' arg says not to) */
    if (!overwrite_yes)
    {
	if ((ofp = fopen(ofname, "r")) != NULL)
	{
	    printf("output file %s  exists -- overwrite? [y/n][n] > ", ofname);
	    fflush(stdout);
	    fgets(tmpstr, 255, stdin);
	    if (tolower(tmpstr[0]) != 'y')
	    {
		printf("aborting\n");
		exit (0);
	    }
	    fclose(ofp);
	}
    }


    /* if no bit filename arguments, glean them from the command file */
    if (argc < 2)
    {
	char buf[256], tbuf[256], *bufp;
	u_int flashaddr;
	FILE *cfp;

	strcpy(bflist[bf_count].name, cmdfname);
	strcpy(bflist[bf_count++].dir, dirname);

	strcpy(name, cmdfname);
	name[43] = '\0'; /* truncated if too long */

	/* input file */
	sprintf(cmdpath, "%s/%s", dirname, cmdfname);
	if ((cfp = fopen(cmdpath, "r")) == NULL)
	{
	    perror(cmdpath);
	    exit(1);
	}

	while (fgets(buf, 255, cfp))
	{
	    bufp = buf;
	    if (gettok(tbuf, &bufp) == 0)  continue;    /* skip blank lines */

	    /* burn blockname flashaddr */
	    if (strcmp(tbuf, "burn") == 0)
	    {
		if (gettok(tbuf, &bufp))
		{
		    if (sscanf(tbuf,"%x", &flashaddr) != 1)
			error_out("burn (bad flashaddr argument)");

		    if (gettok(tbuf, &bufp))
		    {
			char *p;

			if (sscanf(tbuf,"%s", (char *)&tmpstr) != 1)
			    error_out("burn (bad block name)");

			back_to_fwd(tmpstr);

			if ((p = strrchr(tmpstr, '/')) != NULL)
			{
			    *p = '\0';
			    strcpy(bflist[bf_count].dir, tmpstr);
			    strcpy(bflist[bf_count++].name, p+1);
			}
			else
			{
			    strcpy(bflist[bf_count].dir, dirname);
			    strcpy(bflist[bf_count++].name, tmpstr);
			}
		    }
		}
	    }
	}

	fclose(cfp);
    }
    else
    {
	while (argc--)
	{
	    char *p;

	    strcpy(tmpstr, argv[0]);
	    ++argv;
	    back_to_fwd(tmpstr);

	    if ((p = strrchr(tmpstr, '/')) != NULL)
	    {
		*p = '\0';
		strcpy(bflist[bf_count].dir, tmpstr);
		strcpy(bflist[bf_count++].name, p+1);
	    }
	    else
	    {
		strcpy(bflist[bf_count].dir, dirname);
		strcpy(bflist[bf_count++].name, tmpstr);
	    }
	}
    }

    /* file header comment */
    time(&ttime);
    ltime = localtime(&ttime);
    sprintf(idstr, "%s %02d:%02d:%02d %02d/%02d/%d", ofname, ltime->tm_hour, ltime->tm_min, ltime->tm_sec, ltime->tm_mon, ltime->tm_mday, (ltime->tm_year < 1900)?ltime->tm_year+1900:ltime->tm_year);

    /* init/write the magic # and ASCII comment */
    sprintf(hbuf, "%c%c%c%c\n#%s", RCXMAGIC[0],RCXMAGIC[1],RCXMAGIC[2],RCXMAGIC[3], idstr);


    for (bfi=0; bfi<bf_count; bfi++)
    {
	char *openarg;
	char bfname[256];

	strcpy(name, bflist[bfi].name);

	printf("%s from %s\n", bflist[bfi].name, strcmp(bflist[bfi].dir, ".") == 0?"local dir":bflist[bfi].dir);

	sprintf(bfname, "%s/%s", bflist[bfi].dir, bflist[bfi].name);

	name[43] = '\0'; /* truncated if too long */

	if (cmdfile)
	    openarg = "r";
	else openarg = "rb";

	/* input file */
	if ((bfp = fopen(bfname, openarg)) == NULL)
	{
	    perror(bfname);
	    exit(1);
	}

	fseek(bfp, 0, SEEK_END); /* go to end */
	bc = ftell(bfp);
	rewind(bfp);

	printf("bc %d\n", bc);

	/* read the binary data */
	bindata = (u_char *)malloc(bc);
	fread(bindata, 1, bc, bfp); 

	/* reality check */
	if (cmdfile && (n = binchars(bindata, bc)))
	{
	    printf("WARNING -- %d non-ASCII characters found in cmdfile %s -- proceed (y/n)[n] > ", n, bfname);
	    fgets(resp, 255, stdin);
	    if ((resp[0] != 'y') && (resp[0] != 'Y'))
		exit(1);
	}
	else if ((!cmdfile) && (binchars(bindata, bc) == 0))
	{
	    printf("WARNING -- only ASCII characters found in xilinx file %s -- proceed (y/n)[n] > ", bfname);
	    fgets(resp, 255, stdin);
	    if ((resp[0] != 'y') && (resp[0] != 'Y'))
		exit(1);
	}

	/* calculate the checksum */
	block.cksum = 0;
	for (i=0; i<bc; i++)
	    block.cksum += bindata[i];

    printf("cksum: %04x\n", block.cksum);

	/* init the block header */
	block.bytecount = bc;
	if (cmdfile)
	    block.magic = MAGIC_ASC; 
	else block.magic = MAGIC_BIN; 
	block.bytecount = bc; 
	memset(block.name, '\0', 44); /* make sure unused bytes are zeroed out */
	strcpy(block.name, name);
	memset(block.spare, '\0', 8);

	/* wait till now to open output file, just to get past possible errors first */
	if (cmdfile)
	{
	    if ((ofp = fopen(ofname, "wb")) == NULL)
	    {
		perror(ofname);
		exit(1);
	    }
	    fwrite((u_char *)hbuf, 1, 64, ofp);
	}

	/* write the block header */
	fwrite((u_char *)&block, 1, sizeof(block), ofp);

	/* write the binary data */
	fwrite(bindata, 1, block.bytecount, ofp);

	free(bindata);
	fclose(bfp);

	cmdfile = 0;
    }
    fclose(ofp);
}
