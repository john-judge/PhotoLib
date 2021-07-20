// edt_camlink.cpp : Defines the entry point for the DLL application.
//

// #include "stdafx.h"

#include "edtinc.h"

#include "edt_camlink.h"


#ifdef _NT_ // Not needed for Linux (doesn't do anything anyway...)
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }


    return TRUE;
}
#endif

#ifdef __cplusplus

extern "C"
{

#endif

// ===== DEBUG STUFF ==========================================================================
//
// set the envvar  CLSEREDTDEBUG to enable debug output to $PDVDIR/clseredt<unit>_<channel>out
//
// #define CLSEREDTDEBUG 1

#ifdef CLSEREDTDEBUG
static	EdtMsgHandler *msg_p;
static  char EdtDebugOutFile[256];
#define EDTMSG EDTAPP_MSG_INFO_1

#define ENTERMSG(subroutine) edt_msg(EDTMSG, "ENTER %-40s delta-T %lf\n", subroutine, edt_dtime());
#define EXITMSG(subroutine, ret) edtcl_printErrorDebugTag("EXIT", subroutine, ret);

typedef struct {
    char name[32];
    int num;
} ErrorReturn;
static ErrorReturn ClDebugErrorTags[] = { 
    {"CL_ERR_NO_ERR",                       0},
    {"CL_ERR_BUFFER_TOO_SMALL",        -10001},
    {"CL_ERR_MANU_DOES_NOT_EXIST",     -10002},
    {"CL_ERR_PORT_IN_USE",             -10003},
    {"CL_ERR_TIMEOUT",                 -10004},
    {"CL_ERR_INVALID_INDEX",           -10005},
    {"CL_ERR_INVALID_REFERENCE",       -10006},
    {"CL_ERR_ERROR_NOT_FOUND",         -10007},
    {"CL_ERR_BAUD_RATE_NOT_SUPPORTED", -10008},
    {"CL_ERR_OUT_OF_MEMORY",           -10009},
    {"CL_ERR_REGISTRY_KEY_NOT_FOUND",  -10010},
    {"CL_ERR_INVALID_PTR",             -10011},
    {"CL_ERR_UNABLE_TO_LOAD_DLL",      -10098},
    {"CL_ERR_FUNCTION_NOT_FOUND",      -10099},
    {"CL_ERR_EDT_NOT_INITIALIZED",     -12301},
    {"",                               -99999}
};

void edtcl_printErrorDebugTag(char *what, char *sub, CLINT32 err)
{
    int i;

    for (i=0; ClDebugErrorTags[i].num != -99999; i++)
    {
        if (err == ClDebugErrorTags[i].num)
        {
            edt_msg(EDTMSG, "%-6s %-33s delta-T %0.6lf, ret %s (%d)\n", what, sub, edt_dtime(), ClDebugErrorTags[i].name, ClDebugErrorTags[i].num);
            return;
        }
    }
    edt_msg(EDTMSG, "%-6s %-32s delta-T %0.6lf, %s (%d)\n", what, sub, edt_dtime(), "UNKNOWN", ClDebugErrorTags[i].num);
}

static void edtcl_print_debug_serial(char *msg, CLINT8 *buffer, CLUINT32 count)
{
    edt_msg(EDTMSG, "%-21s", msg);
    for (CLUINT32 i=0; i<count; i++)
        edt_msg(EDTMSG, "[%c]<%02x> ", isprint((u_char)(buffer[i]) & 0xff)?(u_char)(buffer[i] & 0xff):'?', (u_char)(buffer[i] & 0xff));
    edt_msg(EDTMSG, "\n");
    
}

#else

#define ENTERMSG(nevermind) /* nevermind */
#define EXITMSG(nevermind, alsonevermind) /* nevermind */

#endif // ===== DEBUG STUFF END ==================================================================



EDT_CAMLINK_API CLINT32 clSerialInit(CLUINT32 serialIndex, hSerRef *serialRefPtr)

{
	// serialIndex must be unit * 2 + channel
	PdvDev *pdv_p;

	int unit = serialIndex >> 1;
	int channel = serialIndex & 1;
	CLINT32 ret;

#ifdef CLSEREDTDEBUG
    char *PDVHOME = getenv("PDVHOME");
     
#ifdef _NT_
    if (PDVHOME != NULL)
        PDVHOME = "c:\\EDT\\pdv";
    _snprintf_s(EdtDebugOutFile, 255, "%s\\clseredt%d_%d.out", PDVHOME, unit, channel);
#else
    if (PDVHOME != NULL)
        PDVHOME = "/opt/EDTpdv";
    snprintf(EdtDebugOutFile, 255, "%s/clseredt%d_%d.out", PDVHOME, unit, channel);
#endif // _NT_
#endif // CLSEREDTDEBUG

	if (!serialRefPtr)
        return CL_ERR_INVALID_PTR;

	if ((pdv_p = pdv_open_device(NULL,unit,channel, 0)) == NULL)
	{
		ret = CL_ERR_INVALID_REFERENCE;
        *serialRefPtr = (void *)NULL;
	}
    else
    {

#ifdef CLSEREDTDEBUG
        msg_p = edt_msg_default_handle();
        edt_msg_set_name(msg_p, EdtDebugOutFile);
        edt_msg_set_target(msg_p, msg_p->file);
        edt_msg_set_level(msg_p, 0xffff);
#endif

        ENTERMSG("clSerialInit:");

        *serialRefPtr = (void *) pdv_p;

        if (pdv_get_dmasize(pdv_p) <= 0)
            ret = CL_ERR_EDT_NOT_INITIALIZED;
		else ret = CL_ERR_NO_ERR;
	}

    EXITMSG("clSerialInit:", ret);
	return ret;
}

EDT_CAMLINK_API CLINT32 clSerialRead(hSerRef serialRef,
			CLINT8 buffer[], CLUINT32 *numbytes, CLUINT32 serialTimeout)

{
	PdvDev *pdv_p = (PdvDev *) serialRef;
	CLINT32 ret = CL_ERR_TIMEOUT;
    int bytes_inbuf=0, bytes_read=0;
    
    ENTERMSG("clSerialRead:");

	if ((pdv_p == NULL) || (buffer == NULL) || (numbytes == NULL))
        ret = CL_ERR_INVALID_REFERENCE;

    else if (*numbytes == 0)
        ret= CL_ERR_NO_ERR;

    else
    {
        if ((bytes_inbuf = pdv_serial_wait(pdv_p, serialTimeout, *numbytes)) >= *numbytes)
        {
#ifdef CLSEREDTDEBUG
            edt_msg(EDTMSG, "IN    clSerialRead:  wait got %d bytes,       delta-T %lf\n", bytes_inbuf, edt_dtime());
#endif
            if ((bytes_read = pdv_serial_read_nullterm(pdv_p, buffer, *numbytes, FALSE)) >= *numbytes)
            {
                *numbytes = bytes_read;
                ret = CL_ERR_NO_ERR;
            }
        }
#ifdef CLSEREDTDEBUG
        else edt_msg(EDTMSG, "IN   clSerialRead:  wait got %d bytes, < %d requested (no read), delta-T %lf sec.\n", bytes_inbuf, *numbytes, edt_dtime());
#endif
    }

#ifdef CLSEREDTDEBUG
    edt_msg(EDTMSG, "IN    clSerialRead:  timeout %d, requested %d, wait returned %d, read returned %d\n", serialTimeout, *numbytes, bytes_inbuf, bytes_read);;
    edtcl_print_debug_serial("IN    clSerialRead:", buffer, bytes_read);
    EXITMSG("clSerialRead:", ret);
#endif // CLSEREDTDEBUG

	return ret;
}

EDT_CAMLINK_API CLINT32 clSerialWrite(hSerRef serialRef,
			CLINT8 buffer[], CLUINT32 *bufferSize, CLUINT32 serialTimeout)

{
    CLINT32 ret = CL_ERR_NO_ERR;
	PdvDev *pdv_p = (PdvDev *) serialRef;

    ENTERMSG("clSerialWrite:");

	if (pdv_p && buffer && bufferSize )
	{
		int len = *bufferSize;

#ifdef CLSEREDTDEBUG
        edt_msg(EDTMSG, "IN   clSerialWrite: timeout %d, len %d ---\n", serialTimeout, len);
        edtcl_print_debug_serial("clSerialWrite: ", buffer, len);
#endif // CLSEREDTDEBUG
		
		pdv_serial_binary_command(pdv_p, buffer, len);

	}
	else ret = CL_ERR_INVALID_REFERENCE;

    EXITMSG("clSerialWrite:", ret);
    return ret;
}


EDT_CAMLINK_API CLINT32 clFlushPort(hSerRef serialRef)

{
	PdvDev *pdv_p = (PdvDev *) serialRef;
	CLINT8 buf[12];
	CLINT32 ret;
	int i = 0;

    ENTERMSG("clFlushPort:");

    if (pdv_p)
    {
		// flush up to 10000 bytes, 10 at a time;
		for(i=0; i<1000; i++)
		{
			if (pdv_serial_read(pdv_p, buf, 10) == 0)
				break;
		}
		ret = CL_ERR_NO_ERR;
    }
    else
		ret = CL_ERR_INVALID_REFERENCE;

#ifdef CLSEREDTDEBUG
    edt_msg(EDTMSG, "EXIT  clFlushPort:   looped %d times, ret %d,  delta-T %0.6lf\n", i, ret, edt_dtime());
#endif // CLSEREDTDEBUG

	return ret;
}

EDT_CAMLINK_API CLINT32 clGetErrorText(const CLINT8 *manuName,
				 CLINT32 errorCode, CLINT8 *errorText, CLUINT32 *errorTextSize)
{
	CLINT32 ret = CL_ERR_NO_ERR;
	char *s = NULL;

    ENTERMSG("clGetErrorText:");

    if ((manuName) && (strcmp(manuName, "EDT") == 0))
    {
		 // we don't have any special error codea at this time
		switch(errorCode)
		{
			case CL_ERR_EDT_NOT_INITIALIZED:
				s = (char *)"EDT DV board has not been initialized";
				break;
			case CL_ERR_INVALID_PTR: // this code is in the template .h file but not the CL spec
						// or some versions of clallserial.dll? So here just in case?
				s = (char *)"Invalid pointer";
				break;
			default:
				ret = CL_ERR_ERROR_NOT_FOUND;
				break;
		}
    }
   	else
		ret = CL_ERR_MANU_DOES_NOT_EXIST;

	if (s)
	{
		if (*errorTextSize >= strlen(s))
		{
			*errorTextSize = strlen(s);
			strcpy(errorText, s);
		}
		else ret = CL_ERR_BUFFER_TOO_SMALL;
	}
	else ret = CL_ERR_ERROR_NOT_FOUND;

    EXITMSG("clGetErrorText:", ret);
	return ret;
}

EDT_CAMLINK_API CLINT32 clGetManufacturerInfo(CLINT8 *mfgName,
						CLUINT32 *bufferSize, CLUINT32 *version)
{
	CLINT32 ret;

    ENTERMSG("clGetManufacturerInfo:");
	
	if (mfgName && version && bufferSize && ((*bufferSize) > 3))
	{
		strcpy(mfgName, "EDT");
		*version = CL_DLL_VERSION_1_1;
		ret = CL_ERR_NO_ERR;
	}
	else
	{
		ret = CL_ERR_BUFFER_TOO_SMALL;
		*bufferSize = 4;
	}

	
#ifdef CLSEREDTDEBUG
    edt_msg(EDTMSG, "EXIT  clGetManufacturerInfo: name '%s', version %d ret %d, delta-T %0.6lf\n", mfgName, version, ret, edt_dtime());
#endif // CLSEREDTDEBUG

	return ret;
}



EDT_CAMLINK_API CLINT32 clGetNumBytesAvail(hSerRef serialRef, CLUINT32 *numBytes)

{
    PdvDev *pdv_p = (PdvDev *) serialRef;
	CLINT32 ret;

    ENTERMSG("clSerialGetNumBytesAvail:");


    if (pdv_p && numBytes)
    {
		*numBytes = (CLUINT32)pdv_serial_get_numbytes(pdv_p);
		ret = CL_ERR_NO_ERR;
	}
    else
		ret = CL_ERR_INVALID_REFERENCE;


#ifdef CLSEREDTDEBUG
    edt_msg(EDTMSG, "EXIT  clGetNumBytesAvail: bytes %d, ret %d, %0.6lf\n", *numBytes, ret, edt_dtime());
#endif // CLSEREDTDEBUG

	return ret;
}

// alias subroutine due to bogus inconsistency in the spec between VB and C++ versions
EDT_CAMLINK_API CLINT32 clGetNumPorts(CLUINT32 *numPorts)
{
    return clGetNumSerialPorts(numPorts);
}

EDT_CAMLINK_API CLINT32 clGetNumSerialPorts(CLUINT32 *numPorts)
{
	CLINT32 ret;
	int nunits;

    ENTERMSG("ClGetgNumSerialPorts:");
	

	if (numPorts)
	{
		Edt_bdinfo *bdinfo;

	    // always assume base mode and return 2 ports per board
		// documentation will need to reflect that with medium/full
		// cameras they need to always access only even-numbered ports
		bdinfo = edt_detect_boards((char *)"pdv", -1, &nunits, 0);
	    *numPorts = nunits * 2;

	    ret = CL_ERR_NO_ERR;
	}
	else
	    ret = CL_ERR_INVALID_PTR;


#ifdef CLSEREDTDEBUG
    edt_msg(EDTMSG, "EXIT  clGetNumSerialPorts: numPorts %d, ret %d, delta-T %0.6lf\n", *numPorts, ret, edt_dtime());
#endif // CLSEREDTDEBUG

	return ret;
}

EDT_CAMLINK_API CLINT32 clGetPortInfo(CLUINT32 serialIndex,
	CLINT8* manufacturerName, CLUINT32* nameBytes, CLINT8* portID, CLUINT32* IDBytes, CLUINT32* version)
{
	CLINT32 ret = CL_ERR_NO_ERR;
	int unit = 0, channel = 0;

    ENTERMSG("clSerialGetPortInfo:");

	if (manufacturerName && nameBytes && portID && IDBytes && version)
	{

	    if (*nameBytes < 4)
	    	ret = CL_ERR_BUFFER_TOO_SMALL;
	    else
	    	strcpy((char *)manufacturerName, "EDT");

	    *nameBytes = 4;

	    if (*IDBytes < 7)
	    	ret = CL_ERR_BUFFER_TOO_SMALL;
	    else
	    {
		unit = serialIndex/2;
		channel = serialIndex % 2;
		sprintf((char *)portID, "pdv%d_%d", unit, channel);
	    }
	    *IDBytes = 7;

	    *version = CL_DLL_VERSION_1_1;

	}
	else ret = CL_ERR_BUFFER_TOO_SMALL;


#ifdef CLSEREDTDEBUG
    edt_msg(EDTMSG, "EXIT  clGetPortInfo: name '%s' ID '%s' version %d ret %d, delta-T %0.6lf\n", manufacturerName, portID, *version, ret, edt_dtime());
#endif // CLSEREDTDEBUG

	return ret;
}

EDT_CAMLINK_API CLINT32 clGetSupportedBaudRates(hSerRef serialRef,
								CLUINT32 *baudRates)
{
    PdvDev *pdv_p = (PdvDev *) serialRef;
	CLINT32 ret;

    ENTERMSG("clGetSupportedBaudRates:");

    if (pdv_p && baudRates)
    {
		*baudRates = 0x1ffff;
		ret = CL_ERR_NO_ERR;
    }
    else
		ret = CL_ERR_INVALID_PTR;


#ifdef CLSEREDTDEBUG
    edt_msg(EDTMSG, "EXIT  clGetSupportedBaudRates: rates 0x%08x ret %d, delta-T %0.6lf\n", *baudRates, ret, edt_dtime());
#endif // CLSEREDTDEBUG

	return ret;
}

EDT_CAMLINK_API CLINT32 clSetBaudRate(hSerRef serialRef, CLUINT32 baudRate)
{
    CLINT32 ret = CL_ERR_NO_ERR;
	int brate = 0;
    PdvDev *pdv_p = (PdvDev *) serialRef;

    ENTERMSG("clSetBaudRate:");

    if (pdv_p)
    {
		switch(baudRate)
		{
	    case CL_BAUDRATE_9600: brate = 9600; break;
	    case CL_BAUDRATE_19200: brate = 19200; break;
	    case CL_BAUDRATE_38400: brate = 38400; break;
	    case CL_BAUDRATE_57600: brate = 57600; break;
	    case CL_BAUDRATE_115200: brate = 115200; break; // most but not all PDV models...

	    case CL_BAUDRATE_230400: 			// nope
	    case CL_BAUDRATE_460800:			// nope
	    case CL_BAUDRATE_921600:			// nope
	    default: ret = CL_ERR_BAUD_RATE_NOT_SUPPORTED;
		}
    }
    else
		ret = CL_ERR_INVALID_REFERENCE;

    if (brate)
		pdv_set_baud(pdv_p, brate);


#ifdef CLSEREDTDEBUG
    edt_msg(EDTMSG, "EXIT  clSetBaudRate: rate %d, set %d, ret %d, delta-T %0.6lf\n", baudRate, brate, ret, edt_dtime());
#endif // CLSEREDTDEBUG

    return ret;
}

EDT_CAMLINK_API CLINT32 clGetSerialPortIdentifier (CLUINT32 serialIndex, CLINT8* portID, CLUINT32* bufferSize)

{
    int ret = CL_ERR_NO_ERR;
    int unit = 0, channel = 0;

    ENTERMSG("ENTER clGetSerialPortIdentifier:");

    if (bufferSize)
    {
		if (*bufferSize < 7)
		    ret = CL_ERR_BUFFER_TOO_SMALL;
		else if (portID)
		{
			 unit = serialIndex/2;
			 channel = serialIndex % 2;
			 sprintf((char *)portID, "pdv%d_%d", unit, channel);
		}
		*bufferSize = 7;
    }
    else
		ret = CL_ERR_INVALID_INDEX;


#ifdef CLSEREDTDEBUG
    edt_msg(EDTMSG, "EXIT  clGetSerialPortIdentifier: ID '%s' size %d ret %d, delta-T %0.6lf\n", portID, bufferSize, ret, edt_dtime());
#endif // CLSEREDTDEBUG

	return ret;

}

EDT_CAMLINK_API CLINT32 clSerialClose(hSerRef serialRef)
{
	PdvDev *pdv_p = (PdvDev *) serialRef;
	CLINT32 ret;

    ENTERMSG("clSerialClose:");

	if (pdv_p)
	{
	    pdv_close(pdv_p);	

	    ret = CL_ERR_NO_ERR;
	}
	else
	    ret = CL_ERR_INVALID_REFERENCE;

    EXITMSG("clSerialClose:", ret);
	return ret;
}


#ifdef __cplusplus

}


#endif
