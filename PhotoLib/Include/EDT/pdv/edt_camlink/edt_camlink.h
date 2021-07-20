
#ifndef _EDT_CAMLINK_DLL_
#define _EDT_CAMLINK_DLL_


#ifdef EDT_CAMLINK_EXPORTS

#ifdef _NT_
#define EDT_CAMLINK_API __declspec(dllexport)
#else
#define EDT_CAMLINK_API __attribute__((visibility("default")))
#endif

#else

#ifdef _NT_
#define EDT_CAMLINK_API __declspec(dllimport)
#else
#define EDT_CAMLINK_API // no dllimport equivalent for linux?
#endif

#endif

#ifndef _NT_
#define APIENTRY
typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned int UNINT32;
typedef void * LPVOID;
#endif

#ifdef __cplusplus

extern "C"
{

#endif


// pulled from supplied clallserial.h

#ifndef _CL_hSerRef_DEFINED_
    #define _CL_hSerRef_DEFINED_
    typedef void*        hSerRef;
#endif

#ifndef _CL_INT32_DEFINED_
    #define _CL_INT32_DEFINED_
    typedef int          CLINT32;
#endif


#ifndef _CL_UINT32_DEFINED_
    #define _CL_UINT32_DEFINED_
    typedef unsigned int CLUINT32;
#endif

#ifndef _CL_INT8_DEFINED_
    #define _CL_INT8_DEFINED_
    typedef char         CLINT8;
#endif


#ifndef CL_DLL_VERSION_NO_VERSION
//------------------------------------------------------------------------------
//  CL serial DLL versions
//------------------------------------------------------------------------------
#define CL_DLL_VERSION_NO_VERSION               1   // Not a CL dll
#define CL_DLL_VERSION_1_0                      2   // Oct 2000 compliant
#define CL_DLL_VERSION_1_1                      3   // Oct 2001 compliant
#endif


//------------------------------------------------------------------------------
//  Baud Rates
//------------------------------------------------------------------------------
#ifndef CL_BAUDRATE_9600
#define CL_BAUDRATE_9600                        1
#define CL_BAUDRATE_19200                       2
#define CL_BAUDRATE_38400                       4
#define CL_BAUDRATE_57600                       8
#define CL_BAUDRATE_115200                      16
#define CL_BAUDRATE_230400                      32
#define CL_BAUDRATE_460800                      64
#define CL_BAUDRATE_921600                      128
#endif


//------------------------------------------------------------------------------
//  Error Codes
//------------------------------------------------------------------------------
#ifndef CL_ERR_NO_ERR
#define CL_ERR_NO_ERR                               0
#define CL_ERR_BUFFER_TOO_SMALL                     -10001
#define CL_ERR_MANU_DOES_NOT_EXIST                  -10002
#define CL_ERR_PORT_IN_USE                          -10003
#define CL_ERR_TIMEOUT                              -10004
#define CL_ERR_INVALID_INDEX                        -10005
#define CL_ERR_INVALID_REFERENCE                    -10006
#define CL_ERR_ERROR_NOT_FOUND                      -10007
#define CL_ERR_BAUD_RATE_NOT_SUPPORTED              -10008
#define CL_ERR_OUT_OF_MEMORY                        -10009
#define CL_ERR_REGISTRY_KEY_NOT_FOUND               -10010
#define CL_ERR_INVALID_PTR                          -10011

#define CL_ERR_UNABLE_TO_LOAD_DLL                   -10098
#define CL_ERR_FUNCTION_NOT_FOUND                   -10099

// EDT-specific errors
#define CL_ERR_EDT_NOT_INITIALIZED					-12301
#endif


//==============================================================================
// IMported functions, clserialedt.dll version
//==============================================================================
EDT_CAMLINK_API CLINT32 clFlushPort(hSerRef serialRef);
EDT_CAMLINK_API CLINT32 clGetErrorText(const CLINT8* manuName, CLINT32 errorCode, CLINT8* errorText, CLUINT32* errorTextSizeconst);
EDT_CAMLINK_API CLINT32 clGetNumPorts(CLUINT32* Ports); 
EDT_CAMLINK_API CLINT32 clGetNumSerialPorts(CLUINT32 *numPorts);
EDT_CAMLINK_API CLINT32 clGetNumBytesAvail(hSerRef serialRef, CLUINT32* numBytes);
EDT_CAMLINK_API CLINT32 clGetPortInfo(CLUINT32 serialIndex, CLINT8* manufacturerName, CLUINT32* nameBytes, CLINT8* portID, CLUINT32* IDBytes, CLUINT32* version);
EDT_CAMLINK_API CLINT32 clGetSupportedBaudRates(hSerRef serialRef, CLUINT32* baudRates);
EDT_CAMLINK_API CLINT32 clSerialClose(hSerRef serialRef);
EDT_CAMLINK_API CLINT32 clSerialInit(CLUINT32 serialIndex, hSerRef* serialRefPtr);
EDT_CAMLINK_API CLINT32 clSerialRead(hSerRef serialRef, CLINT8* buffer, CLUINT32* bufferSize, CLUINT32 serialTimeout);
EDT_CAMLINK_API CLINT32 clSerialWrite(hSerRef serialRef, CLINT8* buffer, CLUINT32* bufferSize, CLUINT32 serialTimeout);
EDT_CAMLINK_API CLINT32 clSetBaudRate(hSerRef serialRef, CLUINT32 baudRate);


#ifdef __cplusplus

}

#endif

#endif
