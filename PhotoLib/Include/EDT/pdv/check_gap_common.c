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
 * check_gap should work with the following cards: PCDa, PCD SS,
 * and PDV Camera Link.  Normally people will not have multiple
 * different types of boards in their system, so this program will 
 * use the default board type (EDT_INTERFACE). Actually, that will
 * only let us distinguish between pdv (for CL) and pcd (SS and PCD).
 * So, what we do is open(EDT_INTERFACE,unit) and check what exactly
 * it is (with edt_p->devid). 
 *
 * This program determines *exactly* which board it 
 * is using by comparing the edt_p->devid to PCDA_ID, PSS4_ID, and 
 * PDVCL_ID.
 
 * Of course, that *still* doesn't allow
 * us to completely know what we're dealing with. The SS boards
 * can be loaded with a 16, 4, or 1 channel bitfile and we can tell
 * a 16 channel from its devid, but the 4 and 1 channel report 
 * themselves as PSS4_ID.  Due to this, this program assumes the
 * board is 4 channel if it is a PSS4_ID, and the user is required
 * to specify otherwise (if it is indeed the 1 channel)  by using
 * the --ss1 option.
 *
 * INPUT: check_gap -l <loop_size> or check_gap -F <clock_speed>
 *
 * TODO: 1) see other todo's in this code
 *      2) look over code & remove as much board specific code as
 *      possible (make code as general as possible).
 *      3) test
 */

#include "check_gap_lib.h"


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
check_gap(char *command_line)
#else 
int main(int argc, char **argv)
#endif
{
#ifdef NO_MAIN
	char **argv = 0;
	int argc = 0;
	opt_create_argv("check_gap", command_line, &argc, &argv);
#endif

	/* holds all data the program's functions use: */
	CheckGapData *data = calloc(1, sizeof(CheckGapData));
	data->clock_speed = 33000000;
	data->bufsize = 1024*1024;
	data->loops = 1;
	data->bitload = 1;
	data->unitstr = "0";

	chkgap_parse_options(argc, argv, data); /* set flags from user's options */
	printf("ss1: %d, ss4: %d\n", data->ss1_flag, data->ss4_flag);
	printf("unit: %d, unitstr: [%s]\n", data->unit, data->unitstr);
	printf("loops: %d, bufsize: %d, bitload: %d\n", data->loops, data->bufsize,
					data->bitload);
	chkgap_initialize (data); /* get the board and program state ready for test */
	chkgap_run_test(data, ((ResultHandler)(chkgap_output_gap)) ); /* run the test */

	return 0;
}

