#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "edtinc.h"
#include "pciload.h" /* ESN_SIZE */
#include "edt_net_msg.h"
#include "clsim_lib.h"

#define ABOUT_TEXT "Tests EDT C-Link (including FOX) boards using EDT CL-Simulator.\n\
    Serial, Mode Codes, and Data lines are tested."
#define EXTRA_HELP \
    "To run test, connect frame grabber and simulator, then two instances\n"\
"of the program need to be started in separate windows.\n"\
"First start the Simulator side: 'clink_tester -u CLSIM-unit'.\n"\
"After that, start the frame grabber side: 'clink_tester -u CL-or-FOX-unit'\n"\
"(Note, the simulator side just waits and responds to serial commands\n"\
"from the frame grabber and can be left running like a 'test server')."


#define CAMERA_CFG_FILE "camera_config/generic24cl.cfg"

#define RESPONSE_LEN 2

/* FYI: these are all two-letter commands so pdv_serial_wait can 
 * return after it gets two characters instead of waiting for a
 * timeout. */
#define START_CMD "HI"
#define READY_CMD "GO"
#define STOP_CMD "NO"
#define OK_CMD "OK"
#define MC_CMD "MC"
#define HEX_CMD "HC"
#define LONG_CMD "LC"
#define LONG_STRING "LONG1234567890987654321ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()-_=+]}[{'\";:.>,</?`~\\|"
#define LONG_RESP   "RESP1234567890987654321ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()-_=+]}[{'\";:.>,</?`~\\|"

#define HEX_MSG_LEN 16
unsigned char Hex_Message[]  = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0x00, 0xf1, 0xe2, 0xd3, 0xc4, 0xb5, 0xa6, 0x97, 0x86 } ;
unsigned char Hex_Response[] = {0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0x6f, 0x00, 0x81, 0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6, 0x07, 0x18 } ;


/* to pause (do_pause=1) or not to pause (do_pause=0), this is the answer */
static int do_pause = 0;

/* frequency argument passed to clsiminit */
char *freq = NULL;

/* camera config file */
char *camera = CAMERA_CFG_FILE;


#define SERIAL_TIMEOUT 2000
#define SEND_WAIT_TIME 2000 
#define RECEIVE_WAIT_TIME 1000 

/* flags for edt_msg... */

#define TESTER_ERROR 0x1000
#define TESTER_INFO 0x2000
#define TESTER_DEBUG 0x4000

#ifndef WIN32

#ifdef __linux__

#include <asm/ioctls.h>

#elif defined(VXWORKS)

#include "ioLib.h"

#else

#include <sys/filio.h>    /* for kbhitedt() */

#endif /* __linux __ */

int kbhit()
{
    int numchars ;
    /* int n, ch ;*/
    ioctl(0, FIONREAD, &numchars) ;

    /* for (n=0; n<numchars; n++)  ch=getchar() ;*/
    return(numchars);
}

#else

#include <conio.h>

#endif

#ifdef WIN32
#define DIR_SEP '\\'
#else
#define DIR_SEP '/'
#endif

int check_console(char * buffer)

{
    if (kbhit())
    {
        int len = (int) strlen(buffer);
        buffer[len] = getchar();
        buffer[len+1] = 0;
        putchar(buffer[len]); fflush(stdout);
        return buffer[len];
    }

    return 0;

}
/* print the character c to stdout n times */
    void 
edt_repeat(char c, int n) 
{
    int i;
    for (i = 0; i < n; ++i) {
        putchar(c);
    }
}

    int
send_test_message(PdvDev *pdv_p, char *message, char *response, int response_len)

{
    int rc;

    pdv_serial_command(pdv_p, message);

    response[0] = 0;

    rc = pdv_serial_wait(pdv_p, SEND_WAIT_TIME, response_len);

    if (pdv_serial_read(pdv_p, response, rc) != rc)
        return 1;

    edt_msg(TESTER_DEBUG,"Sent <%s>, got <%s>\n", message, response);

    return 0;

}

    int
send_test_message_hex(PdvDev *pdv_p, unsigned char *message, int len, unsigned char *response, int response_len)

{
    int rc;

    pdv_serial_binary_command(pdv_p, (char *) message, len);

    response[0] = 0;

    rc = pdv_serial_wait(pdv_p, SEND_WAIT_TIME, response_len);

    if (pdv_serial_read(pdv_p, response, rc) != rc)
        return 1;

    /* edt_msg(TESTER_DEBUG,"Sent <%s>, got <%s>\n", message, response); */

    return 0;

}


/* prints something like this:
 *      Sent        UUT -> TD        Received
 *      _______                      _______
 * 1 __|       |__  MC4 -> MC4  1 __|       |__
 *
 * The title ("Sent...") is printed if do_title is true.
 * Wire tells us if we're printing MC-WIRE.
 * value1 is what was sent, value2 was recieved.
 */
    void 
print_mode_code(int wire, int value1, int value2, int do_title)
{

    char start[] = " 0 __|";
    char end[] = "|__  ";
    char center_label[] = "MC1 -> MC1 ";

    int mid_len = 8; /* length of high value */

    int side;
    int start_len = sizeof(start) - 1;
    int end_len = sizeof(end) - 1;
    int center_label_len = sizeof(center_label) - 1;


    /* Note: keep titles smaller than mid_len + end_len */
    char title1[] = "Sent", title2[] = "Received"; 
    int title1_len = sizeof(title1) - 1; 

    char center[] = "UUT -> TD";
    int center_len = sizeof(center) - 1;

    /* optional title */
    if (do_title) {
        int to_go;
        edt_repeat(' ', start_len);
        edt_msg(TESTER_ERROR,title1);
        to_go = mid_len - title1_len; 
        edt_repeat(' ', to_go + end_len);
        edt_msg(TESTER_ERROR,center);
        to_go = center_label_len - center_len;
        edt_repeat(' ', to_go + start_len);
        edt_msg(TESTER_ERROR,title2);
        edt_msg(TESTER_ERROR,"\n");
    }


    /* top line */
    for (side = 0; side <= 1; ++side) {
        int value;
        if (0 == side) 
            value = value1;
        else 
            value = value2;

        edt_repeat(' ', start_len);
        if (value) 
            edt_repeat('_', mid_len);
        else 
            edt_repeat(' ', mid_len);
        if (side == 0) 
            edt_repeat(' ', end_len + center_label_len);
    }
    edt_msg(TESTER_ERROR,"\n");

    /* print value 1 bottom  value 2 bottom */
    for (side = 0; side <= 1; ++side) {
        int value;
        if (0 == side) 
            value = value1;
        else 
            value = value2;

        edt_msg(TESTER_ERROR," %d __", value ? 1 : 0);
        if (0 == value) {
            edt_repeat('_', mid_len + 2);
        } else {
            edt_msg(TESTER_ERROR,"|");
            edt_repeat(' ', mid_len);
            edt_msg(TESTER_ERROR,"|");
        }
        if (0 == side) {
            edt_msg(TESTER_ERROR,"__  MC%d -> MC%d ", wire, wire);
        } else {
            edt_msg(TESTER_ERROR,"__");
        }
    }
    edt_msg(TESTER_ERROR,"\n");
    return;
}




    static void 
usage (char *progname, char *err)
{
    printf("%s", err);
    printf("usage: %s [Options]\n", progname);
    puts(ABOUT_TEXT);
    puts("");
    puts(EXTRA_HELP);
    puts("");
    puts("Options:");
    puts("-u unit     C-Link frame grabber unit, or Simulator unit (default 0)");
    puts("-I          disable initialization via initcam/clsiminit");
    printf("-f cfg      uses the specified camera config file \n\
            (default %s)\n", CAMERA_CFG_FILE);
    puts("-v          increase verbosity");
    puts("-q          decrease verbosity (quiet mode)");
    puts("-h          this help message");
    puts("-b <baudrate> change buadrate from default 19200 (must set same baud rate");
    puts("            on both simulator and unit under test)");

    puts("\nOptions only for Simulator:");
    puts("-F freq     pass pixel clock frequency argument to clsiminit");

    puts("\nOptions only for Framegrabber under test:");
    puts("-c channel  channel of pdv unit under test (default 0)");
    puts("-l loops    number of loops");
    puts("-p          pause between errors");
    puts("-H <host>   specifies host to log results to (default: none)");
    puts("-nodma      test serial only");
    puts("-noascii    skip ascii serial, only test hex serial\n");
    puts("-nohex      skip hex serial, only test ascii serial\n");
    puts("-noserial   test DMA only");
    puts("-compat     compatability mode, use if rcv (EDT FG) side is < EDT v5.3.3.8");
    exit(1);
}


    void 
pauseit(void) 
{
    edt_msg(TESTER_ERROR,"hit <ENTER> to continue");
    getchar();
    return;
}

void assert_error()

{
    if (do_pause) {
        pauseit();
    }
}

char *fourbitstr(int n, char *vals)

{
    int i;

    for (i=0;i < 4; i++)
    {
        vals[i] = (n & (1 << (3-i))) ? '1':'0';
    }
    vals[4] = 0;

    return vals;
}


/* tests transmission of mode codes from PDV to CLSIM.
 * @param pdv_p the PDV 
 * returns # errors seen */
int test_modecodes(PdvDev *pdv_p)
{
    /* to test four mode code lines thoroughly (checking for shorts between
     * wires and whatnot) write the equiv. of 1->15 to the four "bits".
     *
     * The alternative is to walk a one up, then walk a zero up, which may
     * produce more obvious error output but it is not as complete a check
     * and it is actually more lines of code.
     */
    int n; 
    char query[128];
    char response[128];
    int rdval;
    int errors = 0;
    edt_msg(TESTER_INFO,"Testing mode codes... "); 

    rdval = pdv_serial_wait(pdv_p, 5, 5);

    if (rdval)
        pdv_serial_read(pdv_p, response, rdval);

    for (n = 0; n < 16; ++n)
    {

        /* set mode code */


        edt_reg_write(pdv_p, PDV_MODE_CNTL, n);

        sprintf(query, MC_CMD);

        send_test_message(pdv_p, query, response, RESPONSE_LEN);

        rdval = strtol(response, NULL, 0);

        if (rdval != n) {

            char s1[5], s2[5];

            if (!errors) puts("");
            ++errors;
            edt_msg(TESTER_ERROR,"MC Test: expected %s got %s\n",
                    fourbitstr(n, s1), fourbitstr(rdval, s2));
#if 0

            print_mode_code(1, n & 1, rdval & 1, 1);
            print_mode_code(2, n & 2, rdval & 2, 0);
            print_mode_code(3, n & 4, rdval & 4, 0);
            print_mode_code(4, n & 8, rdval & 8, 0);
            edt_msg(TESTER_ERROR,"\n\n\n");
#endif
            assert_error();
        }

        /* "No News is Good News", so we don't print anything
         * if there was no error. */
    }
    if (errors) {
        edt_msg(TESTER_ERROR,"%d errors seen while testing mode codes\n", errors);
    }
    return errors;
}


    int
check_rgb_cls_buffer(u_char *buf, int size)

{

    u_char rval = 255;
    u_short gbval = 0;
    u_char testrval;
    u_short testgbval;
    int i;
    byte *bp = buf;

    size /= 3;

    for (i=0;i<size;i++)
    {
        testrval = bp[0];
        testgbval = (bp[1] << 8) + bp[2];
        if (testrval != rval)
        {
            edt_msg(TESTER_ERROR,"\nFailed at pixel blue %d got <%d> should be <%d>\n",
                    i, testrval, rval);
            return -(i+1);
        }
        if (testgbval != gbval)
        {
            edt_msg(TESTER_ERROR,"\nFailed at pixel greenred %d got <%d> should be <%d>\n",
                    i, testgbval, gbval);
            return -(i+1);
        }

        rval --;
        gbval ++;

        bp += 3;

    }

    return 0;

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

/* To test DMA, tell simulator to generate data and then
 * verify the data looks as it should. 
 * Note the clsim board and pdv board s/b initialized with the
 * generic24cl.cfg file 
 * Returns number of errors */
    int 
test_dma(PdvDev *uut) 
{
    /* BPP = bytes per pixel, so 3 for RGB */
#define BPP 3
    unsigned int size, bytes_returned;
    unsigned char *image;
    int errors = 0, max_errors;

    unsigned short counter1 = 0; /* 16bit RG counts up */
    unsigned char counter2 = 255; /* 8 bit B counts down */

    int x, y, width, height;

    int timeouts = 0, last_timeouts = 0;

    int init;

    edt_msg(TESTER_INFO,"Testing DMA ... ");
    fflush(stdout);

    width = pdv_get_width(uut);
    height = pdv_get_height(uut);
    size = pdv_get_dmasize(uut);

    if (size == 0) {
        edt_msg(TESTER_ERROR,"ERROR: DMA size is 0! Did initcam fail?\n");
        exit(1);
    }

    pdv_multibuf(uut, 3);

    image = pdv_image(uut);
    bytes_returned = size;
    bytes_returned = edt_get_timeout_count(uut);

    if ((timeouts = pdv_timeouts(uut)) > last_timeouts)
    {
        last_timeouts = timeouts;
        bytes_returned = edt_get_timeout_count(uut);
    }

    if (!edt_little_endian()) do_swab((u_short*)image,bytes_returned/2);
    if (check_rgb_cls_buffer(image, size))
    {

        if (!errors) puts("");
        edt_msg(TESTER_ERROR,"ERROR: check_rgb_cls_buffer() says image has errors.\n");
        ++errors;

        /* scan images for errors, printing the up to max_errors. */
        /* Simulator counter data is 16 bits counter plus 16 bits inverted,
         * which when using the 24bit rgb config file looks to us like:
         * B G R 
         *
         * Simulator output is something like RG XB but we get BGR because
         * the cameralink card's firmware outputs it in that order. The 'X'
         * is the upper byte of the second word and is discarded since it
         * goes out the second channel on the CLSIM board (port D specifically).
         *
         * Anyway, so RG together make a 16 bit word counting up, and B
         * is just R inverted. 
         *
         * If you are looking at the output as an image in pdvshow, with
         * the generic24cl.cfg loaded, the image will appear as a grid of color 
         * bands, blue -> red from left to right (repeating every 255 pixels),
         * with the green value increasing from 0 to 255 from top to bottom.
         * For example, the top left corner of the image should be (r,g,b) =
         * (0,0,255) and the bottom right corner of that band should be (255,252,0).
         */
        init = 1;
        max_errors = 10; /* don't carry on too long printing errs! */
        for (y = 0; y < height && errors <= max_errors ; ++y) {
            for (x = 0; x < width; ++x) {
                int index = BPP * (y * width + x);
                u_short image_value = (image[index+1] << 8) | image[index+2];
                if (init) {
                    init = 0;
                } else {
                    ++counter1;
                    --counter2;
                }

                if (image_value != counter1) {
                    if (!errors) puts("");
                    edt_msg(TESTER_ERROR,"ERROR: got %3hd (0x%2x), expected %3hd (0x%2x) ",
                            image_value, image_value,
                            counter1, counter1);
                    edt_msg(TESTER_ERROR,"at (x,y)=(%d,%d) (offset=%d, RG)\n",
                            x,y, index);
                    ++errors;
                }
                if (image[index] != counter2) {
                    if (!errors) puts("");
                    edt_msg(TESTER_ERROR,"ERROR: got %3hd (0x%2x), expected %3hd (0x%2x) ",
                            image[index], image[index],
                            counter2, counter2);
                    edt_msg(TESTER_ERROR,"at (x,y)=(%d,%d) (offset=%d, B)\n",
                            x,y, index);
                    ++errors;
                }

                if (errors > max_errors) {
                    break;
                }
            }
        }

    }
    if (errors) 

        assert_error();
    return errors;
}

#ifdef VXWORKS
extern void initcam(char *cmdline);
extern void clsiminit(char *cmdline);
#endif

    PdvDev *
initialize_unit_under_test(int unit, int channel, int do_init, int baudrate)

{
    char command_string[128];
    PdvDev *pdv_p;
    int ret = 0;

    /* run  */
#ifndef VXWORKS
    sprintf(command_string, ".%cinitcam -u %d -c %d -f %s", 
            DIR_SEP, unit, channel, camera);

    edt_msg(TESTER_INFO, "\nsystem(\"%s\")\n", command_string), 

    ret = system(command_string);
#else
    sprintf(command_string, "-u %d -c %d -f %s",unit, channel, camera);
    initcam (command_string) ;
#endif



    /* open board */
    if ((pdv_p = pdv_open_channel(EDT_INTERFACE, unit, channel)) == NULL)
    {
        edt_perror("pdv_open_channel()");
        return NULL;
    }

    /* initialize serial */

    pdv_serial_read_enable(pdv_p);

    pdv_set_serial_delimiters(pdv_p,NULL,NULL);

    pdv_set_baud(pdv_p, baudrate);


    return pdv_p;

}

    int
run_test(PdvDev *pdv_p, int do_ascii, int do_hex, int do_dma, int do_mc, int compat)

{   
    int errors = 0;
    char response[128];
    unsigned char hresponse[128];
    /* open communication with tester */

    /* Test Mode Codes */
    if (do_ascii || do_hex)
    {
        send_test_message(pdv_p, START_CMD, response, RESPONSE_LEN);
        if (strcmp(response,READY_CMD))
        {
            edt_msg(TESTER_ERROR,"Error in serial, unable to contact tester\n");
            errors += 1;
            assert_error();

        }

        if (!compat && do_ascii)
        {
            send_test_message(pdv_p, LONG_CMD, response, RESPONSE_LEN);
            if (strcmp(response,OK_CMD))
            {
                edt_msg(TESTER_ERROR,"Error in serial setting up long command\n");
                edt_msg(TESTER_ERROR,"Possible version mismatch. If your receiving system (EDT framegrabber) is on\n");
                edt_msg(TESTER_ERROR, "a system running EDT package version 5.3.3.7 or earler, use the -compat\n");
                edt_msg(TESTER_ERROR, "option on the sending (this) side. For more info, run clink_tester --help.\n");
                errors += 1;
                assert_error();
            }

            send_test_message(pdv_p, LONG_STRING, response, (int)strlen(LONG_RESP));
            if (strcmp(response,LONG_RESP))
            {
                edt_msg(TESTER_ERROR,"Error in serial: sent %s got %s expected %s\n", LONG_STRING, response, LONG_RESP);
                errors += 1;
                assert_error();
            }
        }

        if (!compat && do_hex)
        {

            send_test_message(pdv_p, HEX_CMD, response, RESPONSE_LEN);
            if (strcmp(response,OK_CMD))
            {
                edt_msg(TESTER_ERROR,"Error in serial setting up hex command\n");
                edt_msg(TESTER_ERROR,"Possible version mismatch. If your receiving system (EDT framegrabber) is on\n");
                edt_msg(TESTER_ERROR, "a system running EDT package version 5.3.3.7 or earler, use the -compat\n");
                edt_msg(TESTER_ERROR, "option on the sending (this) side. For more info, run clink_tester --help.\n");
                errors += 1;
                assert_error();
            }

            send_test_message_hex(pdv_p, Hex_Message, HEX_MSG_LEN, hresponse, HEX_MSG_LEN);
            if (memcmp(hresponse, Hex_Response, HEX_MSG_LEN))
            {
                edt_msg(TESTER_ERROR,"Error in serial: sent %*x got %%x expected %%x\n", HEX_MSG_LEN, Hex_Message, HEX_MSG_LEN, hresponse, HEX_MSG_LEN, Hex_Response);
                errors += 1;
                assert_error();
            }
        }

        if (do_mc)
            errors += test_modecodes(pdv_p);

        /* Test DMA */


        send_test_message(pdv_p,STOP_CMD, response, RESPONSE_LEN);

        if (strcmp(response,OK_CMD))
        {
            edt_msg(TESTER_ERROR,"Error in serial, unable to contact tester\n");
            errors += 1;
            assert_error();

        }
    }

    if (do_dma)
        errors += test_dma(pdv_p);

    if (!errors)
    {
        printf(".");
        fflush(stdout);
    }

    return errors ;
}


/* loghost s/b NULL if logging is unwanted */
    int
clink_testsub(int unit, int channel, int loops, int do_init, int baudrate, 
        char *loghost, int do_ascii, int do_hex, int do_dma, int do_mc, int compat)

{
    PdvDev *pdv_p;
    int current_loop;
    int total_errors = 0;
    Edt_embinfo ei;
    char *id_str;
    char pn_str[16];
    char board_info[ESN_SIZE + 256];
    char esn[ESN_SIZE]; /* edt serial number */
    char osn[OSN_SIZE]; /* oem serial number */

    /* EdtMsgHandler logger; */
    /* EdtMsgHandler *lp = &logger; */
    /* edt_msg_init(lp); */

    EdtMsgHandler *lp = edt_msg_default_handle();



#ifdef DO_NET_MSG

    /* Fix - not working on Windows */
    if (loghost) {
        net_data.host = loghost;
        net_data.use_stdout = 1; /* so we log to net, and print locally */
        edt_msg_set_function(lp, edt_msg_net_send);
        edt_msg_set_target(lp, &net_data);
    }

#endif


    edt_msg_output(lp, TESTER_ERROR, 
            "Starting frame grabber side of test (simulator side needs to be running).");

    if ((pdv_p = initialize_unit_under_test(unit, channel, do_init, baudrate)) == NULL)
    {
        edt_perror("Error initializing board\n");
    }

    for (current_loop = 0; current_loop < loops; ++current_loop) 
    {
        total_errors += run_test(pdv_p, do_ascii, do_hex, do_dma, do_mc, compat);
        edt_msg_output(lp, TESTER_INFO,"%d loops %d errors\n", current_loop+1, total_errors);
    }

    id_str = edt_idstr(edt_device_id(pdv_p));
    memset(&ei, 0, sizeof(ei));
    edt_get_sns(pdv_p, esn, osn);
    if (edt_parse_esn(esn, &ei) == 0) {
        sprintf(board_info, "%s with s/n %s, p/n %s", 
                id_str, ei.sn, edt_fmt_pn(ei.pn, pn_str));
    } else {
        sprintf(board_info, "%s with unknown s/n: %s",
                id_str, esn);
    }


    if (total_errors) {
        edt_msg_output(lp, TESTER_ERROR,"\nBAD Card (%d error%s during %d loop%s of tests ) : %s\n", 
                total_errors, total_errors > 1 ? "s" : "", 
                loops, loops > 1 ? "s" : "",
                board_info);
    } else {
        edt_msg_output(lp, TESTER_ERROR,"\nGOOD Card : %s\n",
                board_info);
    }

    pdv_close(pdv_p) ;

    /* reset msg handler to default so any extra stuff isn't logged to net. */
    edt_msg_init(lp);

    return 0;
}

    int
clink_test_cam(int unit, int channel, int do_init, int baudrate)

{
    int ch;
    PdvDev *pdv_p ;
    char command_string[128];
    char freq_arg[32];

    char query[128];
    char response[128];
    unsigned char hresponse[128];
    int done = 0;
    int rc = 0;
    int rdval = 0;
    int len;


    edt_msg(TESTER_INFO,"\nInitializing Camera Simulator");
    edt_msg(TESTER_INFO,"\n\n*****************************************************\n\n");

    if (do_init)
    {
        if (freq != NULL) {
            sprintf(freq_arg, "-F %s", freq);
        } else {
            freq_arg[0] = '\0';
        }
#ifndef VXWORKS
        sprintf(command_string, ".%cclsiminit -u %d -c %d %s -f %s -s", 
                DIR_SEP, unit, channel, freq_arg, camera); 

        edt_msg(TESTER_INFO, "system(\"%s\")\n", command_string), 

        rc = system(command_string);
#else
        sprintf(command_string, "-u %d -c %d %s -f %s -s", 
                unit, channel, freq_arg, camera); 
        clsiminit(command_string);
#endif

    }

    if ((pdv_p = pdv_open_channel(EDT_INTERFACE, unit, channel)) == NULL)
    {
        edt_perror("pdv_open_channel(pdv_p)");
        return (2);
    }

    pdv_serial_read_enable(pdv_p);
    pdv_set_baud(pdv_p, baudrate);
    /* pdv_set_baud(pdv_p, 19200); */

    pdv_set_serial_delimiters(pdv_p,NULL, NULL);

    /* pdv_cls_set_lvcont(pdv_p, 1); */

    printf("Baud rate         :    %d\n", pdv_p->dd_p->serial_baud);

    query[0] = 0;
    response[0] = 0;

    edt_msg(TESTER_INFO,"\n\n*****************************************************\n\n");
    edt_msg(TESTER_INFO,"\nWaiting for commands from frame grabber - type q to quit\n\n");

    len = RESPONSE_LEN;

    while (!done)
    {
        rc = pdv_serial_wait(pdv_p,SERIAL_TIMEOUT, len);

        if (rc)
        {
            /* get the whole command */
            rc = pdv_serial_read(pdv_p, query, rc);
            if (strlen(query) > 1) 
            {
                edt_msg(TESTER_DEBUG,"Query: <%s>\n", query);

                if (!strcmp(query,START_CMD))
                {
                    strcpy(response,READY_CMD);
                    edt_msg(TESTER_INFO,"%s\n", START_CMD);
                }
                else if (!strcmp(query,STOP_CMD))
                {
                    strcpy(response,OK_CMD);
                    edt_msg(TESTER_INFO,"%s\n", STOP_CMD);
                }
                else if (!strcmp(query,MC_CMD))
                {
                    rdval = edt_reg_read(pdv_p, PDV_MODE_CNTL);
                    sprintf(response,"%2d", rdval);
                }
                else if (!strcmp(query, LONG_CMD))
                {
                    strcpy(response,OK_CMD);
                    edt_msg(TESTER_INFO,"%s\n", LONG_CMD);
                    len = (int)strlen(LONG_STRING); /* setting length for long string next */
                }
                else if (!strcmp(query, LONG_STRING))
                {
                    strcpy(response, LONG_RESP);
                    edt_msg(TESTER_INFO,"%s\n", LONG_STRING);
                    len = 2; /* set response length back to default for short cmds */ 
                }
                else if (!strcmp(query, HEX_CMD))
                {
                    strcpy(response,OK_CMD);
                    edt_msg(TESTER_INFO,"%s\n", HEX_CMD);
                    len = HEX_MSG_LEN; /* setting length for hex buffer next */
                }
                else if ((rc == HEX_MSG_LEN) && (!memcmp(query, Hex_Message, HEX_MSG_LEN)))
                {
                    memcpy(hresponse, Hex_Response, HEX_MSG_LEN);
                    edt_msg(TESTER_INFO,"%*x\n", HEX_MSG_LEN, Hex_Response);
                    len = 2; /* set response length back to default for short cmds */ 
                }

                else
                    strcpy(response,"??");

                edt_msg(TESTER_DEBUG,"Response: <%s>\n", response);

                if (memcmp(query, Hex_Message, HEX_MSG_LEN) == 0)
                    pdv_serial_binary_command(pdv_p, (char *)Hex_Response, HEX_MSG_LEN);
                else pdv_serial_command(pdv_p, response);
            }
            query[0] = 0;
            response[0] = 0;

        }


        if ((ch = check_console(query)))
        {
            if (ch == 'q')
                done = 1;
        }
    }
    pdv_close(pdv_p) ;
    return 0;

}


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
clink_tester(char *command_line)
#else
    int
main(int argc, char **argv)
#endif
{

    int loops = 1;
    int unit = 0;
    int channel = 0; /* channel applies only to board under test (not CLSIM) */
    PdvDev *pdv_p ;
    char *progname;
    char *loghost = NULL;
    int do_ascii = 1;
    int do_hex = 1;
    int do_dma = 1;
    int do_mc = 1;
    int do_init = 1; 
    int compat = 0;
    int verbosity = 0;
    int baudrate = 19200; /* should we just test fastest (115200?) */

#ifdef NO_MAIN
    char **argv  = 0 ;
    int argc = 0 ;
    opt_create_argv("clink_tester",command_line,&argc,&argv);
#endif
    progname = argv[0] ;

    --argc;
    ++argv;
    while (argc && argv[0][0] == '-')
    {
        switch (argv[0][1])
        {

            case 'u':
                ++argv;
                --argc;
                if (argc < 1) {
                    usage(progname, "Error: option 'u' requires argument\n");
                }
                unit = atoi(argv[0]);
                break;

            case 'c':
                if (!strcmp(argv[0],"-compat"))
                    compat = 1;
                else if (!strcmp(argv[0],"-c"))
                {
                    ++argv;
                    --argc;
                    if (argc < 1) {
                        usage(progname, "Error: option 'c' requires argument\n");
                    }
                    channel = atoi(argv[0]);
                }
                break;

            case 'f':
                ++argv;
                --argc;
                if (argc < 1) {
                    usage(progname, "Error: option 'f' requires argument\n");
                }
                camera = argv[0];
                break;

            case 'F':
                ++argv;
                --argc;
                if (argc < 1) {
                    usage(progname, "Error: option 'F' requires argument\n");
                }
                freq = argv[0];
                break;

            case 'l': /* number of loops */
                ++argv;
                --argc;
                if (argc < 1) {
                    usage(progname, "Error: option 'l' requires argument\n");
                }
                loops = atoi(argv[0]);
                break;

            case 'p':
                do_pause = 1;
                break;

            case 'h':
                usage(progname, "");
                exit(0);
                break;

            case 'H': /* host to send results to */
                ++argv; --argc;
                if (!argc) {
                    fprintf(stderr, "-H requires argument\n");
                    exit(1);
                }
                loghost = argv[0];
                break;

            case 'I': /* disable initialization via initcam/clsiminit*/
                do_init = 0;
                break;

            case 'v':  
                verbosity = 2;
                break;

            case 'q':  
                verbosity = 0;
                break;

            case 'n':
                if (!strcmp(argv[0],"-nomc"))
                    do_mc = FALSE;
                else if (!strcmp(argv[0],"-noserial"))
                {
                    do_ascii = FALSE;
                    do_hex = FALSE;
                }
                else if (!strcmp(argv[0],"-noascii"))
                    do_ascii = FALSE;
                else if (!strcmp(argv[0],"-nohex"))
                    do_hex = FALSE;
                else if (!strcmp(argv[0], "-nodma"))
                    do_dma = FALSE;
                break;

            case 'b':
                ++argv; --argc;
                if (!argc) {
                    fprintf(stderr, "-b requires argument\n");
                    exit(1);
                }
                baudrate = strtol(argv[0],0,0);
                break;
            default:
                edt_msg(TESTER_ERROR,"unknown option \"-%c\"\n", argv[0][1]);
                usage(progname, "");
                exit(1);
                break;
        }
        argc--;
        argv++;
    }

    edt_msg_add_default_level(TESTER_ERROR);
    if (verbosity > 0)
        edt_msg_add_default_level(TESTER_INFO);

    if (verbosity == 2)
    {
        edt_msg_add_default_level(TESTER_DEBUG);
    }

    if ((pdv_p = pdv_open_channel(EDT_INTERFACE, unit, channel)) == NULL)
    {
        edt_perror("pdv_open_channel(pdv_p)");
        return (2);
    }

    if (pdv_is_simulator(pdv_p))
    {
        pdv_close(pdv_p);

        clink_test_cam(unit, channel, do_init, baudrate);
    }
    else if (edt_is_dvfox(pdv_p) || edt_is_dvcl(pdv_p))
    {
        double elapsed;

        pdv_close(pdv_p);
        elapsed = edt_dtime(); /* init */
        clink_testsub(unit, channel, loops, do_init, baudrate, loghost, do_ascii, do_hex, do_dma, do_mc, compat);
        elapsed = edt_dtime();  /* gives delta time */

        edt_msg(TESTER_ERROR, "\n%d loops in %f seconds (%f loops/sec)\n",
                loops, elapsed, loops / ((double) elapsed));
    }

    if (do_pause) {
        edt_msg(TESTER_INFO,"\nreturn to close") ; getchar();
    }

    exit(0) ;
}
