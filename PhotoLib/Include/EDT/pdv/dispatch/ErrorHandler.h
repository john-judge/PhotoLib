/**
*
*  @file ErrorHandler.h 
* 
*  @copyright Copyright Engineering Design Team 2012
*  All Rights Reserved
*
**/
// EdtImage/ErrorHandler.h: interface for the ErrorHandler class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_EDT_TEXT_OUT_H_)
#define _EDT_TEXT_OUT_H_

#include "edt_error.h"

#include "edt_utils.h"

#include <stdarg.h>

#include <stdio.h>

typedef void (*LogHandlerCallback)(void *objp, const char *fmt, ...);

typedef void (*log_function)(void *target, int tag, const char *s);

#include <string>
#include <iostream>
#include <ostream>
#include <stack>
#include <sstream>
#include <fstream>

#include "EdtLogStreambuf.h"

using namespace std;


class ErrorHandler : public ostream 
{
protected:

    bool own_buffer;

    mutex_t m_lock;

    ErrorHandler *secondary;

    stack<streambuf *> main_stash; 
    stack<streambuf *> log_stash; 

    // options
    bool storing_string;

    string stored_string;

    ostringstream string_out;

    ofstream logfile;

    EdtLogStreambuf dual_buf;

 public:

    ErrorHandler(streambuf *b);
    virtual ~ErrorHandler();

    void SetTarget(const string &fname); 
    void OpenLog(const string &fname);

    void LogOnly();
    void OutputOnly();

    streambuf * MainStreambuf();
    streambuf * LogStreambuf();
    EdtLogStreambuf *    MainBuf();

    void SetTarget(streambuf *sbuf);
    void SetLogStreambuf(streambuf *lbuf);

    void PushTarget(streambuf *t, streambuf *l);
    void PopTarget();

    // print a message to the file

    void Message(const char *fmt, ...);
    void Message(int tag, const char *fmt, ...);

    // Prints the system error message
    void SysErrorMessage(const int errorid, const char *fmt, ...);

    // Prints the last system error
    void SysErrorMessage(const char *fmt, ...);

    void LogString(int tag, const char *s);

    void Lock();
    void Unlock();

    static int MessageFunction(void *target, int level, const char *message);

    const string &LastResponse();
    void ClearResponse();

    void SetStoringString(const bool state);
    bool StoringString();

    void SetLogging(const bool state);
    void SetMessaging(const bool state);
    bool Logging() const;
    bool Messaging() const ;

};

extern ErrorHandler SysError;
extern ErrorHandler SysLog;

void SetEdtMsgTextOut();


#endif 

