/**
*
*  @file ErrorHandler.cpp 
* 
*  @copyright Copyright Engineering Design Team 2012
*  All Rights Reserved
*
**/
/** ********************************
* 
* @file ErrorHandler.cpp
*
* Copyright (c) Engineering Design Team 2010
* All Rights Reserved
*/


// ErrorHandler.cpp: implementation of the ErrorHandler class.
//
//////////////////////////////////////////////////////////////////////

//#include "edtdefines.h"
#include "ErrorHandler.h"

#include <iostream>

using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ErrorHandler::ErrorHandler(streambuf *b) : ostream(b)
{
    streambuf *initbuf;

    initbuf = rdbuf();

    dual_buf.SetMainStreambuf(initbuf);

    rdbuf(&dual_buf);

    own_buffer = false;
    
    create_mutex(m_lock);

    secondary = NULL;

    storing_string = false;

}


ErrorHandler::~ErrorHandler()
{
    
}

void ErrorHandler::SetTarget(const string &fname)

{
    filebuf *s = new filebuf();

    s->open(fname.c_str(), ios_base::out);

    dual_buf.SetMainStreambuf(s);

}

void ErrorHandler::SetLogStreambuf(streambuf *lbuf)

{
     dual_buf.SetLogStreambuf(lbuf);

}
void ErrorHandler::SetTarget(streambuf *lbuf)

{
     dual_buf.SetMainStreambuf(lbuf);

}
void ErrorHandler::LogString(int tag, const char * s)
{

    *this << s;

    if (storing_string)
        stored_string += s;

    if (secondary)
        secondary->LogString(tag, s);

}

void ErrorHandler::SysErrorMessage(const char *fmt, ...)

{
    va_list stack;

    va_start(stack,fmt);

    char buffer[1024];

#ifdef WIN32
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL, GetLastError(),0,
        buffer, 1023, NULL);
#else
    buffer[0] = 0;
#endif
    vsprintf(buffer + strlen(buffer), fmt, stack);

    LogString(0, buffer);

}

void ErrorHandler::SysErrorMessage(const int errorid, const char *fmt, ...)

{
    va_list stack;

    va_start(stack,fmt);

    char buffer[1024];

#ifdef WIN32
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL, errorid,0,
        buffer, 1023, NULL);

#else
    buffer[0] = 0;

#endif
    vsprintf(buffer + strlen(buffer), fmt, stack);

    LogString(0, buffer);

}

// Log a message to the log file
void ErrorHandler::Message(const char *fmt, ...)

{
    va_list stack;

    va_start(stack,fmt);

    char buffer[1024];

    vsprintf(buffer,fmt, stack);

    LogString(0, buffer);

}

// Log a message to the log file
void ErrorHandler::Message(int tag, const char *fmt, ...)

{
    va_list stack;

    va_start(stack,fmt);

    char buffer[1024];

    vsprintf(buffer,fmt, stack);

    LogString(tag, buffer);

}

int ErrorHandler::MessageFunction(void *target, int level, const char *message)

{
    ErrorHandler *obj = (ErrorHandler *) target;

    obj->LogString(level, message);

    return 0;
}

void SetEdtMsgTextOut()

{
    EdtMsgHandler *msg_p = edt_msg_default_handle();

    edt_msg_set_target(msg_p, &SysError);

    edt_msg_set_function(msg_p, ErrorHandler::MessageFunction);

}

void ErrorHandler::Lock()
{
	wait_mutex(m_lock);
}

void ErrorHandler::Unlock()
{
    release_mutex(m_lock);
} 

void ErrorHandler::SetStoringString(const bool state)

{
    storing_string = state;
}
bool ErrorHandler::StoringString(void)

{
    return storing_string;
}

void ErrorHandler::PushTarget(streambuf *t, streambuf *l)

{

    if (t)
    {
        main_stash.push(dual_buf.MainStreambuf());

        dual_buf.SetMainStreambuf(t);
    }

    if (l)
    {
        log_stash.push(dual_buf.LogStreambuf());

        dual_buf.SetLogStreambuf(l);
    }
}


void ErrorHandler::PopTarget()

{
    streambuf * t;

    if (main_stash.size())
    {
        t = main_stash.top();

        main_stash.pop();
    }
    else
        t = NULL;
        
    dual_buf.SetMainStreambuf(t);
    
    if (log_stash.size())
    {
        t = log_stash.top();

        log_stash.pop();
    }
    else
        t = NULL;

    dual_buf.SetLogStreambuf(t);

}

void ErrorHandler::SetLogging(const bool state)

{
    dual_buf.SetLogging(state);

}
    
void ErrorHandler::SetMessaging(const bool state)

{
    dual_buf.SetOutput(state);
}
    
bool ErrorHandler::Logging() const

{
    return dual_buf.Logging();

}
    
bool ErrorHandler::Messaging() const 

{
    return dual_buf.Outputting();
}
    

ErrorHandler SysLog(cout.rdbuf());

ErrorHandler SysError(cerr.rdbuf());


