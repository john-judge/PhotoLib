/** ********************************
 * 
 * @file EdtImage/EdtCmdPort.h
 *
 * Copyright (c) Engineering Design Team 2010
 * All Rights Reserved
 */


#ifndef _EDT_CMD_PORT_
#define _EDT_CMD_PORT_

// Base abstract class for
// very simple socket/file/console i/o
//
// 

#include <string>

#include "edt_utils.h"

using namespace std;

class EdtCmdPort {

public:

    virtual int Open(const char *name) = 0;
    virtual int Close() = 0;

    virtual int ReadString(char *buffer, int maxlen) = 0;
    virtual int WriteString(const char *buffer) = 0;

    virtual int WriteBuffer(const char *buffer, int len) = 0;
    virtual int ReadBuffer(char *buffer, int len) = 0;

    virtual int InputReady(const int timeout = 0) = 0;

    virtual bool Finished() = 0;

    virtual bool Interactive() = 0;
    virtual bool HasPosition() = 0;

    virtual uint64_t Seek(uint64_t pos)
    {
        return 0;
    }

    virtual uint64_t Size()
    {
        return 0;
    }

};

typedef EdtCmdPort * (*EdtCmdPortCreator)(const char *name);

EdtCmdPort *NewPort(const string &type, const char *name);

void AddPortType(const string &type, EdtCmdPortCreator create);

#endif
