/* #pragma ident "@(#)libedt_timing.c	1.28 04/22/11 EDT" */

/**
 * @file
 * File containing definitions for IRIG-B Timecode library
 */


/**
 * @defgroup libedt_timing IRIG-B Timecode Library
 * The functions in libedt_timing.c and libedt_timing.h provide services for the IRIG-B 
 * Timecode package running on an EDT I/O board.  The services include:
 * 
 *    - Functions to acquire and display the IRIG-B timecode from the
 *      embedded MSP430 timecode processor.  Normally the timecode
 *      will be embedded in the DMA stream the EDT board firmware.
 *      Contact EDT for more information.
 *
 *    - Functions to control the configuration of the embedded MSP430
 *      timecode processor.
 *
 *    - Functions to load updated firmware to the MSP430 timecode processor
 *      boot flash prom.
 */ 


/**
 * @defgroup timecode_configuration Configuration Functions
 * @ingroup libedt_timing
 * Configuration Functions
 */


/**
 * @defgroup timecode_display Display Functions
 * @ingroup libedt_timing
 * Display Functions
 */


/**
 * @defgroup timecode_update Firmware Update Functions
 * @ingroup libedt_timing
 * Firmware Update Functions
 */

/**
 * @defgroup timecode_utility Utility Functions
 * @ingroup libedt_timing
 * Utility Functions
 */


#include <time.h>
//#include "libedt_timing.h"
#include "edtinc.h"

/* Timing register offsets: */
/* 	Time Dist. Board:	0x60 (default) */
/* 	MSDV:			0xa4 */
/* 	Net10G:			0xac */
/* 	SRXL-2:			0x5c */
/* 	CLINK:			0x31 */

/**
 * @brief Read an SPI access register using the correct register base address.
 * 
 * @param edt_p The EDT open device handle.
 * @param desc  One of the SPI_ access macros from libedt_timing.h
 * 
 * @return The value returned from the register read.
 * 
 * @ingroup timecode_utility
 */
u_int
edt_spi_reg_read(EdtDev *edt_p, u_int desc)
{
    u_int adj_desc = desc;
    u_int retval;

    if (edt_p->spi_reg_base)
    {
	adj_desc &= ~0x000000ff;
	adj_desc |= edt_p->spi_reg_base + (desc & 0x0f);
    }
    retval= edt_reg_read(edt_p, adj_desc);
    return retval;
}

/**
 * @brief Write an SPI access register using the correct register base address.
 * 
 * @param edt_p The EDT open device handle.
 * @param desc  One of the SPI_ access macros from libedt_timing.h
 * @param val  Value to write to the SPI register.
 * 
 * @ingroup timecode_utility
 */
void
edt_spi_reg_write(EdtDev *edt_p, u_int desc, u_int val)
{
    u_int adj_desc = desc;

    if (edt_p->spi_reg_base)
    {
	adj_desc &= ~0x000000ff;
	adj_desc |= edt_p->spi_reg_base + (desc & 0x0f);
    }
    edt_reg_write(edt_p, adj_desc, val);
}

/**
 * @brief Opens the EDT SPI master.
 * 
 * @param edt_interface string containing the type of board; "pcd", "pdv", "pe53b", etc.
 * @param unit Board unit number
 * @param spi_reg_base Selects an alternative spi register base address (0 selects default 0x60)
 * 
 * @return A pointer to the EdtDev structure if successful or NULL if the open
 * fails or the device is not attached to the SPI master on the timing board.
 *
 * This EDT device handle should be used with edt_spi_ prefixed subroutines.
 *
 * See the included example program timing_test.c.
 * 
 * @ingroup timecode_configuration
 */
EdtDev *
edt_spi_open(char *edt_interface, int unit, u_int spi_reg_base)   /* edt_interface is "pcd" or "pdv" or "pe53b", etc. */
{
    EdtDev *edt_p = NULL;

    printf("Trying PCD unit %d\n", unit);
    edt_p = edt_open(edt_interface, unit);

    if (edt_p != NULL)
    {
	edt_p->spi_reg_base = spi_reg_base;
	if ((edt_spi_reg_read(edt_p, SPI_STAT_CTRL) & 0x80) != 0) /* Master clears this bit. */
	{
	    fputs("Warning:  SPI unit is not master device; may be read-only\n", stderr);
	    fflush(stderr);
	}

    }

    return edt_p;
}

/**
 * @brief Closes the SPI master EDT device handle.
 * 
 * @param edt_p The EDT open device handle.
 * 
 * @return The value from edt_close().
 *
 * @ingroup timecode_configuration
 */
int
edt_spi_close(EdtDev *edt_p)
{
    return edt_close(edt_p);
}

/**
 * @brief Polls the EDT SPI master for a single byte.
 * 
 * @param edt_p The EDT open device handle.
 * 
 * @return The byte obtained from the SPI interface.
 *
 * @ingroup timecode_utility
 */
u_char
edt_spi_get_byte(EdtDev *edt_p)
{
    u_char ch;

    while (edt_spi_get_stat(edt_p) & FIFO_IN_EMPTY)
    {
	edt_usec_busywait(10);					/* Each character takes about 8 usec */
    }
    ch = edt_spi_reg_read(edt_p, SPI_DATA);
    edt_spi_reg_write(edt_p, SPI_STROBE, 0x00);	/* SPI fifo must be strobed after each read */
    return ch;
}

u_char
edt_spi_get_byte_nointr(EdtDev *edt_p)
{
    u_char ch;

    while (edt_spi_get_stat(edt_p) & FIFO_IN_EMPTY)
    {
	edt_usec_busywait(10);					/* Each character takes about 8 usec */
    }
    ch = edt_spi_reg_read(edt_p, SPI_DATA);
    edt_spi_reg_write(edt_p, SPI_STROBE, 0x00);	/* SPI fifo must be strobed after each read */
    return ch;
}

/**
 * @brief Inputs a single binary byte from the EDT SPI interface
 * 
 * @param edt_p The EDT open device handle.
 * @param byte  Pointer to an unsigned char. 
 * 
 * @return 0 on success; -1 on timeout or other failure.
 *
 */
int
edt_spi_get_binary(EdtDev *edt_p, unsigned char *byte)
{
    u_char ch;

    if ((ch = edt_spi_get_byte(edt_p)) == 0)
	return -1;

    if (ch == '\\')		/* Translate \\ to '\' and \z to zero */
    {
	if ((ch = edt_spi_get_byte(edt_p)) == 0)
	    return -1;

	if (ch == '\\')
	{
	    *byte = ch;
	}
	else if (ch == 'z')
	{
	    *byte = 0;
	}
	else
	    return -1;
    }
    else
	*byte = ch;

    return 0;
}


/**
 * @brief Returns true if the SPI fifo is empty.
 * 
 * @param edt_p The EDT open device handle.
 * 
 * @return Nonzero if the SPI fifo is empty, zero otherwise.
 *
 */
int
edt_spi_fifo_empty(EdtDev *edt_p)
{
    if (edt_spi_get_stat(edt_p) & FIFO_IN_EMPTY)
	return 1;
    else
	return 0;
}

/**
 * @brief Sends one byte to the EDT SPI master.
 * 
 * @param edt_p The EDT open device handle.
 *
 * @param ch The byte to send to the SPI master.
 * 
 * @return The byte sent to the SPI interface.
 *
 * @ingroup timecode_utility
 */
u_char
edt_spi_put_byte(EdtDev *edt_p, u_char ch)
{
    edt_spi_reg_write(edt_p, SPI_DATA, ch);		/* Write byte to SPI fifo */
    return ch;
}

/**
 * @brief Returns the value of the EDT SPI status register.
 * 
 * @param edt_p The EDT open device handle.
 * 
 * @return The value of the EDT SPI status register.
 *
 * @ingroup timecode_display
 */
u_char
edt_spi_get_stat(EdtDev *edt_p)
{
    return edt_spi_reg_read(edt_p, SPI_STAT_CTRL);
}

/**
 * @brief Flushes the EDT SPI input and output fifos, discarding all data.
 * 
 * @param edt_p The EDT open device handle.
 * 
 * @return No return value.
 *
 * @ingroup timecode_utility
 */
void
edt_spi_flush_fifo(EdtDev *edt_p)
{
    edt_spi_reg_write(edt_p, SPI_STAT_CTRL, FIFO_RESET);	/* Reset SPI fifo */
    edt_spi_reg_write(edt_p, SPI_STAT_CTRL, 0x00);
}

static int
spi_pkt_crc_fails(u_char *pkt)					/* Check packet CRC.  Return 0 for success, nonzero for failure. */
{
    int i;
    u_char len = pkt[1], ckl, ckh;				/* Second byte is data length. */

    len += 2;							/* Include cmd and len bytes in CRC computation. */

    ckl = pkt[0];
    ckh = pkt[1];

    for (i = 2; i < len; i += 2)
    {
	ckl ^= pkt[i];
	ckh ^= pkt[i+1];
    }

    ckl = ~ckl;
    ckh = ~ckh;

    return ((ckl ^ pkt[len]) | (ckh ^ pkt[len+1]));
}

/**
 * @brief After a packet-start byte is received, call this routine to input the packet and check the CRC.
 * 
 * @param edt_p The EDT open device handle.
 * @param byte  Pointer to an unsigned char array of 64-bytes which will contain the packet on success.
 * 
 * @return packet data length on success; -1 on timeout or other failure.
 *
 */
int
edt_spi_get_pkt(EdtDev *edt_p, u_char *pkt)
{
    int i, len;
    u_char byte;

    if (edt_spi_get_binary(edt_p, &byte) == -1)			/* Get Packet command byte */
	return -1;
    pkt[0] = byte;

    if (edt_spi_get_binary(edt_p, &byte) == -1)			/* Get Packet data length byte */
	return -1;
    len = pkt[1] = byte;

    if (len > 60)
    {
	puts("Packet length check failed");
	puts("\nPacket dump:");
	printf("	cmd 0x%x len %d ", pkt[0], pkt[1]);
	puts("\n");

	return -1;
    }

    /* Capture the packet data and CRC bytes and check the packet against the CRC. */
    for (i = 2; i < len + 4; ++i)				/* command byte + length byte + data bytes + two CRC bytes */
    {
	if (edt_spi_get_binary(edt_p, &byte) == -1)		/* Get Packet data or CRC byte */
	    return -1;
	pkt[i] = byte;
    }

    if (spi_pkt_crc_fails(pkt))					/* Check packet CRC.  Return -1 if failure. */
    {
	puts("crc check failed");
	puts("\nPacket dump:");
	printf("	cmd 0x%x len %d ", pkt[0], len);
	for (i = 2; i < len + 4; ++i)				/* command byte + length byte + data bytes + two CRC bytes */
	    printf(" pkt[%d] 0x%x ", i, pkt[i]);
	puts("\n");

	return -1;
    }
    else
    {
	return len;
    }
}

/**
 * @brief Obtains a timestamp packet from the timing msp430 via SPI
 * 
 * @param edt_p The EDT open device handle.
 *
 * @return unsigned 64-bit unix based time (seconds since Jan 1, 1970)
 *
 */
uint64_t
edt_spi_get_time_pkt(EdtDev *edt_p)
{
    u_char command, len;
    uint64_t tval = 0;
    static u_char pkt[64];
    int i, retval;

    if ((retval = edt_spi_get_pkt(edt_p, pkt)) == -1)
	return 0;

    command = pkt[0];
    len     = pkt[1];

    switch (command)
    {
     case SPI_TIMECODE_PKT:
     case SPI_TIMECODE_PKT_RAW:
     {
	for (i = 0; i < len; ++i)
	    tval |= pkt[i+2] << (8 * i);

	tval |= (uint64_t) command << 56;
	break;
     }
    }

    return tval;
}

/**
 * @brief Prints the timecode in ascii once per second for N loops.
 * 
 * @param edt_p The EDT open device handle.
 *
 * @param loops The number of seconds to print.
 * 
 * @return No return value.
 *
 * This function is mostly used for testing and debugging as the timestamp is normally
 * inserted into the DMA data stream by the board FPGA firmware.
 *
 * See the included example program timing_test.c.
 *
 * @ingroup timecode_display
 */
void
edt_spi_display_time(EdtDev *edt_p, int loops)
{
    int loop, i, done = 0, retry = 0;
    u_char inchar[2] = {0, 0};			/* Single char NULL terminated string */
    uint64_t irigb_lltime;			/* irigb time in 64-bit unix-based time format */
    unsigned int irigb_time;	/* CPU word-size irigb and computer unix time format */
    time_t timevar, computer_time;
    u_char stat;				/* SPI stat register */
    int new_timestamp_format = 0;				/* New SPI interface from msp430? */

    for (loop = 0; !done && (loop < loops || loops == 0); /* loop incremented only upon timestamp input */ ) {
	/* Wait for SPI input fifo to become non-empty */
	if ((((stat = edt_spi_get_stat(edt_p)) & 0x02)) != 0x02) {
	    irigb_lltime = 0;
	    
	    for (i = 0; i < 16; ++i) {		/* Capture 16 ascii-hex bytes */
		inchar[0] = edt_spi_get_byte(edt_p);

		if (inchar[0] == 0) {		/* If user had hit Control-C, break out of loop back to prompt */
		    done = 1;
		    break;
		}
		else if (inchar[0] == SPI_PKT_START) {	/* Forward compatibility (after July '08 V3.0 firmware) */
		    irigb_lltime = edt_spi_get_time_pkt(edt_p);
		    if (irigb_lltime == 0)			/* packet reception error */
		    {
			puts("Searching for timestamp packet version...");
			retry = 1;
		    }

		    new_timestamp_format = 1;
		    break;
		}
		else {					/* Backward compatibility (before July '08 V3.0 firmware) */
		    irigb_lltime |= (uint64_t) (strtoul((const char *)inchar, NULL, 16) << (i*4));
		    new_timestamp_format = 0;
		}
	    }

	    if (retry)
	    {
		retry = 0;
		continue;
	    }
	    else
	    {

		    ++loop;

		    irigb_time = (unsigned int) irigb_lltime;
		    timevar = irigb_time;
		    computer_time = time(NULL);

		    if (irigb_time)
		    {
			char *p;
			static char computer_str[32];
			static char irigb_str[32];
			int timecode_format = (int) (irigb_lltime >> 56) & 0xff;

			p = ctime((const time_t *) &computer_time);
			strcpy(computer_str, p);

			if ((p = ctime((const time_t *) &timevar)) == NULL)
			{
			    puts("Flushing SPI fifo...");
			    edt_spi_flush_fifo(edt_p);
			}
			else
			{
			    strcpy(irigb_str, p);

			    printf("\n\nPolling loop %d (%s spi format; press Control-C to end loop)\n\n", loop, (new_timestamp_format) ? "new" : "old");
			    if (timecode_format == SPI_TIMECODE_PKT_RAW)
			    {
				ts_raw_t *raw_p = (ts_raw_t *) & irigb_time;

				printf("IRIG-B BCD values:  seconds %d minutes %d hours %d day-of-year %d year %d\n",
				    raw_p->seconds,
				    raw_p->minutes,
				    raw_p->hours,
				    raw_p->days,
				    raw_p->years);
			    }
			    else
			    {
				printf("IRIG-B seconds:    %010d;  TZ corrected:  ", (int) irigb_time);
				fputs(irigb_str, stdout);
			    }
			    printf("Computer seconds:  %010d;  TZ corrected:  ", (int) computer_time);
			    fputs(computer_str, stdout);
			    fflush(stdout);
			}
		    }
	    }

	    if (new_timestamp_format == 0)
		edt_spi_flush_fifo(edt_p);
	}
    }
}


/**
 * @brief Puts the EDT SPI msp430 into flash-loader mode.
 * 
 * @param edt_p The EDT open device handle.
 *
 * @return Zero on success, -1 on fault.
 *
 * Puts the EDT SPI msp430 into flash-loader mode, which accepts commands
 * via ascii string to erase and reprogram the msp430 flash memory.  Culminates
 * in rebooting the msp430.
 *
 * See the included utility program msp430_load.c.
 *
 * @ingroup timecode_update
 */
int
edt_spi_invoke_flash_loader(EdtDev *edt_p)
{
    int ch, ret = 0, tries = 0;

    do
    {
	puts("Attention msp430:");
	edt_spi_put_byte(edt_p, 0x80);			 /* Send flash-loader escape */
	edt_msleep(100);

	edt_spi_flush_fifo(edt_p);
	edt_msleep(100);

	/* Empty the FIFO of any leftover bytes */
	/*while ( !edt_spi_fifo_empty(edt_p) ) */
	/*{ */
	    /*ch = edt_spi_get_byte(edt_p); */
	/*} */

	edt_spi_put_byte(edt_p, 0x80);			 /* Verify that flash-loader is running */
	edt_msleep(100);

	ch = edt_spi_get_byte(edt_p);

	++ tries;					/* Retry 20 times */
	if (ch != 0x90)
	{
	    if (ch == 0xa0)
		printf("Expected ACK (0x90) got NAK (0x%x)\n", ch);
	    else
		printf("Expected ACK (0x90) got 0x%x\n", ch);
	}

    } while (ch != 0x90 && ch != 0xa0 && tries < 20);

    if (ch == 0x90)
    {
	puts("Flash programming mode started.\n");
    }
    else
    {
	puts("Flash programming mode faulted.  Insure that firmware is properly loaded.\n");
	ret = -1;
    }

    /* Empty the FIFO of any leftover bytes */
    while ( !edt_spi_fifo_empty(edt_p) )
    {
	volatile int dummy = edt_spi_get_byte(edt_p);
	printf("Flushing SPI FIFO (0x%x left in fifo)\n", dummy);
	edt_spi_flush_fifo(edt_p);
    }


    return ret;
}

/**
 * @brief Send a single line of ascii string to the msp430 over the SPI
 * 
 * @param edt_p The EDT open device handle.
 *
 * @param str The string to send to the EDT SPI interface.
 * 
 * @return A pointer to the argument string.
 *
 * @ingroup timecode_utility
 */
char *
edt_spi_putstr(EdtDev *edt_p, char *str)
{
    char *p;

    for (p = str; *p != '\0'; ++p)
    {
	while ((edt_spi_get_stat(edt_p) & FIFO_OUT_EMPTY) == 0)		/* TODO: change this when almost-full flag is available */
	    edt_usec_busywait(10);					/* Each character takes about 8 usec */

	edt_spi_put_byte(edt_p, *p);
    }

    return str;
}

u_char
edt_crc16_lowbyte(u_char *buf, int len)			/* Create 16-bit CRC low byte */
{
    int i;
    u_char result;

    result = buf[0];

    for (i = 2; i < len; i += 2)
	result ^= buf[i];

    return ~result;
}

u_char
edt_crc16_highbyte(u_char *buf, int len)			/* Create 16-bit CRC high byte */
{
    int i;
    u_char result;

    result = buf[1];

    for (i = 2; i < len; i += 2)
	result ^= buf[i+1];

    return ~result;
}

void
edt_spi_send_binary(EdtDev *edt_p, u_char val)				/* Send binary data to SPI; escape zeros and escape character. */
{
    u_char esc_char = 0;

    if (val == '\0')						/* Translate zero bytes to '\' + 'z' */
    {
	val = '\\';
	esc_char = 'z';
    }
    else if (val == '\\')					/* Translate '\' bytes to '\' + '\' */
	esc_char = '\\';

    while ((edt_spi_get_stat(edt_p) & FIFO_OUT_FULL))		/* Wait for room in SPI output fifo	 */
	edt_usec_busywait(9);					/* Each character takes about 8 usec */

    edt_spi_put_byte(edt_p, val);

    if (esc_char)
    {
	while ((edt_spi_get_stat(edt_p) & FIFO_OUT_FULL))	/* Wait for room in SPI output fifo */
	    edt_usec_busywait(9);				/* Each character takes about 8 usec */

	edt_spi_put_byte(edt_p, esc_char);
    }
}

/**
 * @brief Send a command packet to the msp430 via the SPI; wait for output fifo full but not ACK/NAK reply.
 * 
 * @param edt_p The EDT open device handle.
 *
 * @param str The formatted command buffer to send as a packet.
 *
 *
 * Packets to and from the msp430 microcontroller over the SPI bus always start with
 *
 *  #define SPI_PKT_START   0x82	Start of packet byte.
 *
 * followed by a command buffer, followed by a two-byte command buffer CRC check.
 *
 * The command buffer consists of the following:
 *
 * 	byte 0:		Command byte
 * 	byte 1:		Number of data bytes 0 - 60
 * 	bytes 2 - 62:	Command specific data bytes
 *
 *    Currently supported commands are:
 *
 *
 *	#define SPI_PACKET_LOOP		0	Loop back packet; send back the packet.  Data size 0-60 bytes; no ACK/NAK response.
 *
 *	#define SPI_TIMECODE_EN		1	Enable (1) or disable (0) the 1 Hz ascii-hex timestamp.  Data size 1 byte. ACK/NAK response.
 *
 *	#define SPI_CLK_SELECT		2	Select msp430 clock source as internal DCO (0) or external XIN (1) plus clock rate. ACK/NAK response.
 * 						Data size 5 bytes; first byte selects internal/external, last 4 bytes
 * 						is an integer specifying the clock rate in Hz; LS byte first; MS byte last.
 * 						Currently supported clock rates for DCO (0) are 1, 8, 12 and 16 Mhz.
 * 						Currently supported clock rate for XIN (1) is 10 Mhz.
 *
 *	#define SPI_TIMECODE_PKT	3	Sent from msp430 to FPGA.  Data consists of 4-8 bytes of timecode LSByte first, extended UNIX seconds time format.
 *
 * The CRC check bytes apply to the command buffer (command, length and data bytes), and is computed as follows
 * where CKL is the low CRC byte, CKH is the high CRC byte, B1 is the Command byte and Bn is the last data byte
 * (derived from Texas Instruments Application Report SLAA089D "Features of the MSP430 Bootstrap Loader"):
 *
 * 	CKL = INV [ B1 XOR B3 XOR ... XOR Bn–1 ]
 * 	CKH = INV [ B2 XOR B4 XOR ... XOR Bn ]
 *
 * @ingroup timecode_utility
 */
void
edt_spi_send_packet(EdtDev *edt_p, u_char *cmdbuf)
{
    int i;

    u_char cmd = cmdbuf[0];
    u_char len = cmdbuf[1];

    edt_spi_send_binary(edt_p, 0x82);					/* Start of packet */
    edt_spi_send_binary(edt_p, cmd);
    edt_spi_send_binary(edt_p, len);

    for (i = 0; i < len; ++i)
	edt_spi_send_binary(edt_p, cmdbuf[i+2]);

    edt_spi_send_binary(edt_p, edt_crc16_lowbyte(cmdbuf, len+2));	/* 16-bit CRC low byte */
    edt_spi_send_binary(edt_p, edt_crc16_highbyte(cmdbuf,len+2));	/* 16-bit CRC high byte */
}

/**
 * @brief Get the msp430 IRIG-B firmware version
 * 
 * @param edt_p The EDT open device handle.
 *
 * @return An integer containing the firmware version number
 *
 * Obtain the version of the IRIG-B firmware running on the msp430.
 *
 * See the included example program timing_test.c.
 *
 * @ingroup timecode_update
 */
int
edt_get_timecode_version(EdtDev *edt_p)
{
    int retval;

    edt_set_timecode_enable(edt_p, 0);
    edt_msleep(2000);
    edt_spi_flush_fifo(edt_p);

    edt_spi_put_byte(edt_p, 0x81);
    retval = edt_spi_get_byte(edt_p);

    edt_set_timecode_enable(edt_p, 1);

    return retval & 0x7f;
}

/**
 * @brief Enable or disable timecode production from the MSP430.
 * 
 * @param edt_p The EDT open device handle.
 * @param enable An integer containing 1 to enable timestamp production, 0 to disable it.
 *
 * @return 0 for success, -1 for failure.
 *
 * Packet communication with the MSP430 requires the timestamp production
 * must be disabled before the packets, then reenabled when packet communication
 * is complete.
 *
 * See the included example program timing_test.c.
 *
 * @ingroup timecode_configuration
 */
int
edt_set_timecode_enable(EdtDev *edt_p, int enable)
{
    u_char cmdbuf[16];
    int loopcount = 0;

    cmdbuf[0] = SPI_TIMECODE_EN;
    cmdbuf[1] = 1;				/* length = 1 data byte. */
    cmdbuf[2] = enable;				/* Enable timestamp. */
    edt_spi_send_packet(edt_p, cmdbuf);

    while ((cmdbuf[0] = edt_spi_get_byte(edt_p)) != ACK && cmdbuf[0] != NAK)
    {
	if (++loopcount > 1000)
	{
	    fprintf(stderr, "timeout waiting for ACK or NAK; edt_set_timecode_enable %d failed\n", enable);
	    return -1;
	}
    }


    if (cmdbuf[0] != ACK)
    {
	fprintf(stderr, "edt_set_timecode_enable %d failed on NAK\n", enable);
	return -1;
    }
    else
	return 0;
}

/**
 * @brief Enable or disable raw BCD timecode production from the MSP430.
 * 
 * @param edt_p The EDT open device handle.
 * @param enable An integer containing 1 to enable BCD timestamp production, 0 to disable it.
 *
 * @return 0 for success, -1 for failure.
 *
 * The timecodes produced by the MSP430 are normally UNIX time; i.e. the number of seconds
 * since January 1, 1970.  By enabling raw BCD timecode production, the raw numbers from
 * the IRIG-B timecode signal are produced instead.  The numbers are sent in a struct ts_raw_t
 * defined in libedt_timing.h.
 *
 * See the included example program timing_test.c.
 *
 * @ingroup timecode_configuration
 */
void
edt_set_timecode_raw(EdtDev *edt_p, int enable)
{
    u_char cmdbuf[16];

    cmdbuf[0] = SPI_TIMECODE_RAW;
    cmdbuf[1] = 1;				/* length = 1 data byte. */
    cmdbuf[2] = enable;				/* Enable timestamp. */
    edt_spi_send_packet(edt_p, cmdbuf);

    while ((cmdbuf[0] = edt_spi_get_byte(edt_p)) != ACK && cmdbuf[0] != NAK)
	;

    if (cmdbuf[0] != ACK)
	puts("Got NAK; command fault.\n");
    else
	puts("Got ACK; command succeeded.\n");

    edt_set_timecode_seconds_offset(edt_p, (enable) ? 0 : 1);
}

/**
 * @brief Set a seconds offset for timecode production.
 * 
 * @param edt_p The EDT open device handle.
 * @param seconds An integer containing the number of seconds to add to the timecode.
 *
 * @return void
 *
 * The timecodes produced by the MSP430 are normally one to two seconds later than
 * the PPS signal due to timestamp decoding and datpath transmission.  This function
 * sets a firmware variable to the number of seconds to add to the timestamp to align
 * it with the current PPS.
 *
 * See the included example program timing_test.c.
 *
 * @ingroup timecode_configuration
 */
void
edt_set_timecode_seconds_offset(EdtDev *edt_p, u_int seconds)
{
    u_char cmdbuf[16];

    cmdbuf[0] = SPI_TIMECODE_ADD_SECONDS;
    cmdbuf[1] = 1;				/* length = 1 data byte. */
    cmdbuf[2] = seconds;			/* Seconds to offset timestamp */
    edt_spi_send_packet(edt_p, cmdbuf);

    while ((cmdbuf[0] = edt_spi_get_byte(edt_p)) != ACK && cmdbuf[0] != NAK)
	;

    if (cmdbuf[0] != ACK)
	puts("Got NAK; command fault.\n");
}

/**
 * @brief Select an internal or external clock source for the MSP430 and state the clock rate.
 * 
 * @param edt_p The EDT open device handle.
 * @param clock_sel An integer containing 0 for internal clock and 1 for external clock select.
 * @param clock_hz An integer stating the number of Hertz for the selected clock.
 *
 * @return void
 *
 * The MSP430 normally uses an internal 16 Mhz system clock.  This clock can be set to
 * 1 Mhz, 4 Mhz, 8 Mhz, 12 Mhz or 16 Mhz by using the third argument.  An external clock
 * can also be specified along with its stated clock rate.
 *
 * When an external clock is selected and no clock signal is available on XIN
 * (P26), a clock fault condition is triggered which resets the system clock
 * back to the default internal 16 Mhz clock.
 *
 * See the included example program timing_test.c.
 *
 * @ingroup timecode_configuration
 */
void
edt_set_msp430_clock(EdtDev *edt_p, int clock_sel, int clock_hz)
{
    u_char cmdbuf[16];

    printf("\nSet msp430 clock source to %d and hz rate to %d:\n\n", clock_sel, clock_hz);

    if (clock_sel != 0 && clock_sel != 1)
    {
	puts("Illegal clock_sel value: must be 0 for internal or 1 for external clock.\n");
	return;
    }

    cmdbuf[0] = SPI_CLK_SELECT;
    cmdbuf[1] = 5;					/* Length: 5 data bytes:  byte(1) clock select value; bytes[2-5] integer32 clock hz value */
    cmdbuf[2] = clock_sel;				/* Select internal or external clock */

    if (clock_sel == 0 && clock_hz != 1000000 && clock_hz != 8000000
		       && clock_hz != 12000000 && clock_hz != 16000000)
    {
	puts("Illegal internal clock_hz value.  Must be one of:");
	puts("1000000, 8000000, 12000000 or 16000000.\n");
	return;

    }

    cmdbuf[3] = clock_hz & 0xff;
    cmdbuf[4] = (clock_hz >> 8) & 0xff;
    cmdbuf[5] = (clock_hz >> 16) & 0xff;
    cmdbuf[6] = (clock_hz >> 24) & 0xff;

    edt_set_timecode_enable(edt_p, 0);			/* Disable 1 Hz timecode */
    edt_spi_send_packet(edt_p, cmdbuf);

    while ((cmdbuf[0] = edt_spi_get_byte(edt_p)) != ACK && cmdbuf[0] != NAK)
	printf("Waiting for ACK/NAK got %x\n", cmdbuf[0]);

    if (cmdbuf[0] != ACK)
	puts("Got NAK; command fault.\n");
    else
	puts("Got ACK; command succeeded.\n");

    edt_set_timecode_enable(edt_p, 1);			/* Enable 1 Hz timecode */

    edt_spi_flush_fifo(edt_p);
}

/**
 * @brief Enable the msp430 IRIG-B firmware programmable year mode and set the year.
 * 
 * @param edt_p The EDT open device handle.
 * @param year The year >= 2000.
 *
 * @return void
 *
 * Enable the msp430 IRIG-B firmware programmable year mode and set the year.
 * The IRIG-B spec does not provide the year of the timestamp.  This function
 * enables the user to set the year to be used when computing the timestamp.
 *
 * By default, the CONTROL FIELD of the IRIG-B signal is assumed to provide
 * the current year since 2000.  When this is not the case this function
 * enables the programmed year mode and sets the year to the second argument.
 *
 * The year argument must be >= 2000.  This value is stored in a firmware
 * variable in the MSP430 and included in the timestamps produced by the
 * IRIG-B signal.  When the "days" field of the IRIG-B signal rolls over
 * to 1, the year is incremented by 1.
 *
 * See the included example program timing_test.c.
 *
 * @ingroup timecode_configuration
 */
void
edt_enable_timecode_programmable_year(EdtDev *edt_p, u_short year)
{
    u_char cmdbuf[16];

    cmdbuf[0] = SPI_ENABLE_PROGRAMMED_YEAR;
    cmdbuf[1] = 2;				/* length = 2 data bytes. */
    cmdbuf[2] = year & 0xff;			/* LSByte of year */
    cmdbuf[3] = (year >> 8) & 0xff;		/* MSByte of year */
    edt_spi_send_packet(edt_p, cmdbuf);

    while ((cmdbuf[0] = edt_spi_get_byte(edt_p)) != ACK && cmdbuf[0] != NAK)
	;

    if (cmdbuf[0] != ACK)
	puts("Got NAK; command fault.\n");
}

/**
 * @brief Disable the msp430 IRIG-B firmware programmable year mode.
 * 
 * @param edt_p The EDT open device handle.
 *
 * @return void
 *
 * Disable the msp430 IRIG-B firmware programmable year mode.  Successive
 * timestamps will obtain the year information from the CONTROL FIELD of the
 * IRIG-B signal.
 *
 * See the included example program timing_test.c.
 *
 * @ingroup timecode_configuration
 */
void
edt_disable_timecode_programmable_year(EdtDev *edt_p)
{
    u_char cmdbuf[16];

    cmdbuf[0] = SPI_DISABLE_PROGRAMMED_YEAR;
    cmdbuf[1] = 0;				/* No data */
    edt_spi_send_packet(edt_p, cmdbuf);

    while ((cmdbuf[0] = edt_spi_get_byte(edt_p)) != ACK && cmdbuf[0] != NAK)
	;

    if (cmdbuf[0] != ACK)
	puts("Got NAK; command fault.\n");
}
