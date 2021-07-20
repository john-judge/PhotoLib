/* #pragma ident "@(#)edt_sysinfo.c	1.6 06/06/05 EDT" */

/* edt_sysinfo.c: gets information about the current system and writes it
* to a file which can then be emailed to EDT to speed up diagnosing problems.
*
* This program collects the following information:
*  EDT driver version # (from version file and "setdebug -v")
*  Cards found in system (from pciload)
*  System type (from uname on unix, or windoes api calls on NT)
*  Bitfile info (setdebug -d 1)
*  cpu & memory info (from /proc on linux, psrinfo on sun, 
*  and windows api on NT)
*  camera configuration from pdvload file
*
* The information is all stored in edt_sysinfo.out by default. 
*
*/


#include <stdio.h>
#include <stdlib.h>
#include "edtinc.h"
#ifndef _NT_
#include <unistd.h>
#endif


#ifdef _NT_
#define popen(cmd, mode) _popen(cmd, mode)
#define pclose(cmd) _pclose(cmd)
#endif

#ifdef _NT_
#define PREPATH ".\\"
#else
#define PREPATH "./"
#endif

/* pciload.h */
#define MAX_STRING 128

int Ndiags = 0;

/* print a little border with a newline first */
void topborder(FILE *out) {
    fputs("\n------------------------------------------------------\n", out);
    return;
}

/* print a little border, no newline */
void bottomborder(FILE *out) {
    fputs("------------------------------------------------------\n", out);
    return;
}

/* print title followed by border */
void header(const char *title, FILE *out) {
    fputs(title, out);
    bottomborder(out);
    return;
}

/* copy all of in to out */
void copy(FILE *in, FILE *out) {
    int c; /* char read */
    while ((c = fgetc(in)) != EOF) {
        fputc(c, out);
    }
    return;
}

/* copy file named in_name to out.  
* opens file named, copys it to out, and prints border */
void copy_named(const char *in_name, FILE *out) 

{
    FILE *in = fopen(in_name, "r");
    char *errstr = (char *) calloc(1, 32 + strlen(in_name));

    topborder(out);
    if (NULL == in) {
        sprintf(errstr, "Error opening input file \"%s\"", in_name);
        edt_perror(errstr);
        free(errstr);
        fprintf(out, "Error opening input file \"%s\" (errno: %d)\n", 
            in_name, errno);
        bottomborder(out);
    } else {
        fprintf(out, "contents of file %s:\n", in_name);
        bottomborder(out);
        fflush(stdout);
        copy(in, out);
    }
    fflush(out);
    ++Ndiags;
    return;
}

/* try but do nothing if doesn't exist */
void
try_copy_named(char *filename, FILE *out)
{
    if (edt_access(filename, 0) == 0)
        copy_named(filename, out);
}



/* prints name of command being run, its output, and a border */
void do_cmd(const char *cmd, FILE *outfile) 

{
    /* run command */
    FILE *cmdout;
    char *errstr = (char *) calloc(1, 32 + strlen(cmd));

    topborder(outfile);
    fprintf(outfile, "doing command \"%s\":\n", cmd);
    bottomborder(outfile);
    fflush(stdout);
    cmdout = popen(cmd, "r");
    fflush(outfile);

    if (NULL == cmdout) {
        sprintf(errstr, "Error opening pipe from \"%s\"", cmd);
        edt_perror(errstr);
        free(errstr);
        exit(EXIT_FAILURE);
    }

    /* read output and print it */
    copy(cmdout, outfile);
    fflush(outfile);

    /* close and we're done */
    pclose(cmdout);
    Ndiags ++;
    return;
}


void
usage(char *progname)
{
    puts("");
    printf("%s: collect system information for diagnostic use.\n\n", progname);
    printf("Running '%s' with no arguments will collect and store certain system\n", progname);
    printf("and EDT board information in the file 'edt_sysinfo.out'.  The information\n");
    printf("collected can be useful to EDT in helping diagnose any problems. This\n");
    printf("program only collects information deemed useful for diagnostic purposes\n");
    printf("and EDT will not use it for any other purpose.\n\n");
    printf("Before running %s, be sure to close any other programs talking to the\n", progname);
    printf("the EDT board(s).\n\n");
    printf("usage: %s\n\n", progname);
}

int main (int argc, char **argv) {
    char *out_name = "edt_sysinfo.out";
    FILE *out;
    char   *progname = argv[0];
    Edt_bdinfo *boards;
    int num_boards = -1;
    int i;
    /* int err_fd = dup(2);  copy of stderr */

    /*
    * process command line arguments
    */
    --argc;
    ++argv;
    while (argc && ((argv[0][0] == '-') || (argv[0][0] == '/')))
    {
        switch (argv[0][1])
        {
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

    if ((out = fopen(out_name, "w")) == NULL) {
        edt_perror("Error opening output file");
        exit(EXIT_FAILURE);
    }

    printf("\nedt_sysinfo will collect system and EDT board diagnostic information and dump\n");
    printf("it into the file %s.\n\n", out_name);
    printf("Close any other EDT interface applications, then press ENTER > "); fflush(stdout);
    getchar();

    dup2(fileno(out), 2); /* stderr is duplicate of output file */
    /* any output to stderr should now go to output file,
    * write to err_fd if we need to really tell user something */


    /* version file and setdebug -v */
    copy_named(PREPATH "version", out);

    do_cmd(PREPATH "setdebug -v", out);

    /* pciload (on everything but p53b) */
#ifndef P53B
    do_cmd(PREPATH "pciload", out);
    do_cmd(PREPATH "pciediag", out);
#endif

#ifdef PDV
    /* pdvload (only PDV) */
#ifndef _NT_ 
    copy_named(PREPATH "pdvload", out);
#endif
    /* NT uses camsetup*_*.bat, not pdvload */
    /* TODO: create pdvinfo to get:
    *  - contents of camsetup/pdvload files
    *  - copy of .cfg file (in case user modified it)
    */
#endif

    /* system name, kernel, type, etc. */
#ifndef _NT_
    do_cmd("uname -a", out);
    do_cmd("lsb_release -a", out);
#endif

    /*  dmesg, cpu & memory info (from /proc on linux, psrinfo on sun, 
    *  and windows api on NT) */
#ifdef __sun
    do_cmd("dmesg", out);
    do_cmd("psrinfo -v", out);
    do_cmd("prtconf -v -D", out);
    do_cmd("isainfo -kv", out);
#elif __linux__
    do_cmd("dmesg", out);
    do_cmd("/sbin/lspci -v", out);
    copy_named("/proc/cpuinfo", out);
    copy_named("/proc/meminfo", out);
#endif

    /* We want register info (from setdebug) for all cards in the system */
    boards = edt_detect_boards("", -1, &num_boards, 0);

    for (i = 0; i < num_boards; ++i) {
        char unitstr[32]; /* stores something like "p53b32" (or "pcd0") */
        char cmd[MAX_STRING];
        sprintf(unitstr, "%s%d", boards[i].type, boards[i].id);

        /*  Register info (setdebug)  */

        sprintf(cmd, PREPATH "setdebug -d 1 -u %s", unitstr);
        do_cmd(cmd, out);

        sprintf(cmd, PREPATH "setdebug -g -u %s", unitstr);
        do_cmd(cmd, out);
    }


    /* now to windows specific stuff. */
#ifdef _NT_
    do_cmd(PREPATH "win_sysinfo", out);
#endif

    try_copy_named(PREPATH "install_pdv.log", out);
    try_copy_named(PREPATH "install_pcd.log", out);
    try_copy_named(PREPATH "install_sse.log", out);
    try_copy_named(PREPATH "install_srxl.log", out);
    try_copy_named(PREPATH "install_drx16.log", out);
    try_copy_named(PREPATH "install_v4.log", out);

    printf("\n%d diagnostics complete. Please include the file %s in any\n", Ndiags, out_name);
    printf("technical coorespondence with EDT.\n\n");

    return 0;
}

