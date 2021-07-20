/**
* @file
* Generalized error handling for edt and pdv libraries
*
* Copyright 2000 Engineering Design Team
*
*/

#include "edt_utils.h"
#include "edt_error.h"

#include <stdlib.h>
#include <string.h>

#include <stdarg.h>

#ifdef WIN32

#include <windows.h>
#include <process.h>

#else

#ifndef VXWORKS
#include <sys/errno.h>
#endif

#endif



#define MAX_MESSAGE 512


#define EDT_DEFAULT_MSG_LEVEL     \
    EDTAPP_MSG_FATAL \
    | EDTAPP_MSG_WARNING \
    | EDTLIB_MSG_FATAL \
    | EDTLIB_MSG_WARNING \
    | PDVLIB_MSG_FATAL \
    | PDVLIB_MSG_WARNING \
    | EDT_MSG_ALWAYS


static char edt_last_error_message[MAX_MESSAGE];

EdtMsgHandler edt_default_msg;
static int edt_default_msg_initialized = 0;

static int 
edt_msg_printf(void *f_p, int level,  const char *message)
{
    FILE *f = (FILE *) f_p;


    if (f)
	return fprintf(f, "%s", message);

    else
	return -1;

}

static void 
edt_msg_close_file(EdtMsgHandler *msg_p)

{
    if (msg_p->own_file && msg_p->file)
	fclose(msg_p->file);

    if (msg_p->target == msg_p->file)
	msg_p->target = NULL;

    msg_p->file = NULL;
    msg_p->own_file = 0;

}


/* copied from libedt.c, so we don't need to link to that just for this function
*/
static unsigned 
edt_err_num(void)
{
#ifdef _NT_
    return GetLastError();
#else 
    extern int errno;
    return errno;
#endif
}

static int
edt_msg_output_vprintf_perror(EdtMsgHandler *msg_p,  int level, const char *format, va_list ap)
{

    char *fullmsg; /* callers message + perror message */
    int success = -1;
    /* copy of last errno so no problem if it changes while executing this func. */
    unsigned error = edt_err_num(); 

#ifdef _NT_
    LPVOID *  lpMsgBuf;
#else
    char *errstr; /* perror message */
    extern int errno;
#endif

    if (msg_p == &edt_default_msg)
    {
	if (!edt_default_msg_initialized)
	    edt_msg_init(msg_p);
    }

    if (!(level & msg_p->level))
	return 0;


    /* get caller's string...  */ 

    vsprintf(edt_last_error_message, format, ap);

    /* ... then add system error string to it. */
#ifdef _NT_

    FormatMessage(
	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
	NULL,
	error,
	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),	/* Default language */
	(LPTSTR) & lpMsgBuf,
	0,
	NULL
	);
    /* add three for \n and ": " chars. */
    fullmsg = (char *)malloc(strlen(edt_last_error_message) + strlen( (char*)lpMsgBuf ) + 3*sizeof(char) +1);
    sprintf(fullmsg, "%s: %s\n", edt_last_error_message, (char*)lpMsgBuf);
    LocalFree(lpMsgBuf);
#else
    errstr = strerror(error);
    /* add three for \n and ": " chars. */
    fullmsg = (char *)malloc(strlen(edt_last_error_message) + strlen(errstr) + 3*sizeof(char));
    sprintf(fullmsg, "%s: %s\n", edt_last_error_message, errstr);
#endif


    if (msg_p->func) {
	success = msg_p->func(msg_p->target, level, fullmsg);
    }

    free(fullmsg);
    return success;
}



/**
* Initializes a message handler with default values. The message file is
* initialized to \c stderr. The output subroutine pointer is set to \c
* fprintf (to write output to the console). The message level is set to
* EDT_MSG_WARNING | EDT_MSG_FATAL.
*
* @param msg_p pointer to message handler structure to initialize
*
* \Example
* @code
* EdtMsgHandler msgLogger;
* edt_msg_init(&msgLogger);
* @endcode
*
* @see edt_msg_output
*/
void 
edt_msg_init(EdtMsgHandler *msg_p)

{
    if (msg_p)
    {
	msg_p->file = stderr;
	msg_p->own_file = 0;
	msg_p->level = EDT_DEFAULT_MSG_LEVEL;
	msg_p->func = edt_msg_printf;
	msg_p->target = msg_p->file;
    }

    if (msg_p == &edt_default_msg)
	edt_default_msg_initialized = 1;
}


/** 
* Initializes a message handler to use the named file and specified
* level.
*
* @param msg_p pointer to message handler structure to initialize
* @param file the name of a file to open and write messages to.
* @param level the level that future messages must match against if
* they are to be handled by the \a msg_p handler.
*/
void 
edt_msg_init_names(EdtMsgHandler *msg_p, char *file, int level)

{
    edt_msg_init(msg_p);
    edt_msg_set_name(msg_p, file);
    edt_msg_set_level(msg_p, level);

}

/** 
* Initializes a message handler to use the specified file and level.
* Similar to #edt_msg_init_names but rather than opening a named file,
* this takes a pointer to a FILE which has been opened by the caller.
*
* @param msg_p pointer to message handler structure to initialize
* @param file FILE pointer returned by e.g. fopen().
* @param level the level that future messages must match against if
* they are to be handled by the \a msg_p handler.
*/
void 
edt_msg_init_files(EdtMsgHandler *msg_p, FILE *file, int level)

{
    edt_msg_init(msg_p);
    edt_msg_set_file(msg_p, file);
    edt_msg_set_level(msg_p, level);
}



/**
* Closes and frees up memory associated with a message handler. Use only
* on message handlers that have been explicitly initialized by
* edt_msg_init. Do not try to close the default message handler.
* If the message handler has been configured to use a file which the
* user opened, through functions such as #edt_msg_init_files or
* #edt_msg_set_file, then the user is responsible for closing that
* file after calling this function.
*
* @param msg_p  pointer to message handler to close, which was
* initiailzed by #edt_msg_init
*
* @return 0 on success, -1 on failure.  
*/
void 
edt_msg_close(EdtMsgHandler *msg_p)

{
    edt_msg_close_file(msg_p);
}



/**
* Submits a message to the default message handler, which will
* conditionally (based on the flag bits) send the message 
* to the default message handler function. This function uses
* the default message handler, and is equivalent to calling
* edt_msg_output(edt_msg_default_handle(), ...). To submit a message
* for handling by other than the default message handle, use
* #edt_msg_output.
*
* @param level an integer variable that contains flag bits indicating
* what 'level' message it is.  Flag bits are described in the overview.
*
* @param format a string and arguments describing the format. Uses
* vsprintf to print formatted text to a string, and sends the result to
* the handler subroutine. Refer to the \c printf manual page for
* formatting flags and options.
*
* \Example
* @code
* edt_msg(EDTAPP_MSG_WARNING, "file '%s' not found", fname);
* @endcode
*
* @return 0 on success, -1 on failure.   
*/
int
edt_msg(int level, const char *format, ...)
{
    va_list	stack;

    if (!edt_default_msg_initialized)
	edt_msg_init(&edt_default_msg);

    if (!(level & edt_default_msg.level))
	return 0;

    va_start(stack,	format);

    vsprintf(edt_last_error_message, format, stack);

    if (edt_default_msg.func)
	return edt_default_msg.func(edt_default_msg.target, level, edt_last_error_message);
    return -1;

}



/**
* Submits a message using the \a msg_p message handler, which will
* conditionally (based on the flag bits) send the message
* to the handler's function. To submit a
* message to the default message handler, use #edt_msg.
*
* @param msg_p  pointer to message handler, initiailzed by #edt_msg_init
* @param level  an integer variable that contains flag bits indicating
* what 'level' message it is.  Flag bits are described in the overview.
* @param format a string and arguments describing the format. Uses
* \c vsprintf to print formatted text to a string, and sends the result to
* the handler subroutine.  Refer to the \c printf manual page for
* formatting flags and options.
*
* \Example
* @code
* int my_error_popup(void *target, int level, char *message) {
*     GtkWindow * parentWindow = (GtkWindow *)target;
*     GtkWidget * dialog = gtk_message_dialog_new(parentWindow, 0,
*     GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE, message);
* }
*
* if (edt_access(fname, 0) != 0)
*      edt_msg_output(msgLogger, EDTAPP_MSG_WARNING, "file '%s' not
*      found", fname);
*
* GtkWindow *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
* gtk_widget_show(window);
*
* EdtMsgHandler msgLogger;
* edt_msg_init(&msgLogger);
* edt_msg_set_target(window);
* edt_msg_set_function(msgLogger, (EdtMsgFunction *)my_error_popup);
* edt_msg_set_level(msgLogger, EDT_MSG_FATAL | EDT_MSG_WARNING);
* @endcode
*
* @return 0 on success, -1 on failure.  
*
* @todo make this example use wxWidgets, or something. 
* @todo make this example more simple, possibly just point readers
* to an example program.
*/
int
edt_msg_output(EdtMsgHandler *msg_p,  int level, const char *format, ...)
{

    va_list	stack;


    if (msg_p == &edt_default_msg)
    {
	if (!edt_default_msg_initialized)
	    edt_msg_init(msg_p);
    }

    if (!(level & msg_p->level))
	return 0;

    va_start(stack,	format);

    vsprintf(edt_last_error_message, format, stack);

    if (msg_p->func)
	return msg_p->func(msg_p->target, level, edt_last_error_message);

    return -1;
}




/**
* Sets the "message level" flag bits that determine whether to call the
* message handler for a given message. The flags set by this function
* are ANDed with the flags set in each edt_msg call, to determine
* whether the call goes to the message function and actually results in
* any output.
*
* @param msg_p  pointer to message handler
* @param newlevel The new level to set in the message handler.
*
* \Example
* @code
* edt_msg_set_level(edt_msg_default_handle(), 
*                   EDT_MSG_FATAL|EDT_MSG_WARNING);
* @endcode
*/
void 
edt_msg_set_level(EdtMsgHandler *msg_p, int newlevel)

{
    if ((msg_p == &edt_default_msg) && !edt_default_msg_initialized)
	edt_msg_init(&edt_default_msg);

    msg_p->level  = newlevel;

}

/** 
* Gets the message level that messages must match in order to be
* handled by the message handler \a msg_p.  The level is a combination
* of flags OR'ed together as described in the overview.
*
* @param msg_p  pointer to message handler
*/
int 
edt_msg_get_level(EdtMsgHandler *msg_p) 
{
    return msg_p->level;
}


/**
* Sets the message level to the combination of the specified level with
* the message handler's previous level.
*
* @param msg_p  pointer to message handler, initiailzed by #edt_msg_init
* @param level a message level flag, as defined in the overview.
*/
void 
edt_msg_add_level(EdtMsgHandler *msg_p, int level) 
{
    edt_msg_set_level(msg_p, edt_msg_get_level(msg_p) | level);
}


/**
* Sets the function to call when a message event occurs. The default
* message function is \c fprintf() (which outputs to \c stderr);
* this routine allows programmers to substitute any type of
* message handler (pop-up callback, file write, etc).
*
* For an example of how this could be used, see #edt_msg.
*
* @param msg_p  pointer to message handler
* @param f The funtion to call when a message event occurs.
*
* @see edt_msg_set_level, edt_msg
*/
void 
edt_msg_set_function(EdtMsgHandler *msg_p, EdtMsgFunction f)

{
    if ((msg_p == &edt_default_msg) && !edt_default_msg_initialized)
	edt_msg_init(&edt_default_msg);

    msg_p->func = f;
}

/**
* Sets the output file pointer for the message handler. 
*
* The user still owns the file, so they are responsible for closing it
* after this message handler is done with it, such as after this function
* is called again, or after #edt_msg_close is called.
*
* @param msg_p  pointer to message handler
* @param fp FILE pointer to an opened file, to which the
* messages should be output.
*
* \Example
* @code
* EdtMsgHandler msg;
* EdtMsgHandler *msg_p = &msg;
* FILE *fp = fopen("messages.out", "w");
* edt_msg_init(msg_p);
* edt_msg_set_file(msg_p, fp);
*
* ... some time later ...
*
* edt_msg_close(msg_p);
* fclose(fp);
* @endcode
*
*/
void 
edt_msg_set_file(EdtMsgHandler *msg_p, FILE *fp)

{
    if ((msg_p == &edt_default_msg) && !edt_default_msg_initialized)
	edt_msg_init(&edt_default_msg);

    edt_msg_close_file(msg_p);

    msg_p->file = fp; 

    msg_p->own_file = 0;
}


/**
* Sets the output file to the named file.
*
* @param msg_p  pointer to message handler
* @param name the name of a file to open.  Future messages will be
* written to that file.
*/
void 
edt_msg_set_name(EdtMsgHandler *msg_p, const char *name)

{
    if ((msg_p == &edt_default_msg) && !edt_default_msg_initialized)
	edt_msg_init(&edt_default_msg);

    edt_msg_close_file(msg_p);

    msg_p->file = fopen(name, "wb");

    msg_p->own_file = (msg_p->file != NULL);
    edt_msg_set_target(msg_p, msg_p->file);

}

/**
* Sets the target in the message handler.
*
* The target would usually be an object that messages are sent to, such
* as a window, but exactly what it will be depends on what the message
* handler's function expects.
*
* @see edt_msg_set_function, EdtMsgFunction
*/
void 
edt_msg_set_target(EdtMsgHandler *msg_p, void *target)

{
    if ((msg_p == &edt_default_msg) && !edt_default_msg_initialized)
	edt_msg_init(&edt_default_msg);

    msg_p->target = target;
}


/**
* Conditionally outputs a system perror using the default message
* handler.
*
* This function is equivalent to calling
* edt_msg_output_perror(edt_msg_default_handle(), level, msg);
*
* @param level message level for the current message, as described in the overview
* @param msg   message to concatenate to the system error message
*
* \Example
* @code
* if ((fp = fopen ("file.txt", "r")) == NULL)
*     edt_msg_perror(EDT_FATAL, "couldn't open file.txt for reading");
* @endcode
*
* @return 0 on success, -1 on failure.  
* @see edt_perror, edt_msg_output_perror
*/
int
edt_msg_perror(int level, const char *msg)

{
    /** @todo this function should just call edt_msg_output_perror */
    char message[MAX_MESSAGE];

#ifdef _NT_
    LPVOID *  lpMsgBuf;
#else
    extern int errno;
#endif

    if (!edt_default_msg_initialized)
	edt_msg_init(&edt_default_msg);

    if (!(level & edt_default_msg.level))
	return 0;

#ifdef _NT_

    FormatMessage(
	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
	NULL,
	GetLastError(),
	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),	/* Default language */
	(LPTSTR) & lpMsgBuf,
	0,
	NULL
	);

    sprintf(message, "%s: %s\n", msg, lpMsgBuf);
    LocalFree(lpMsgBuf);
#else
    sprintf(message, "%s: %s\n", msg, strerror(errno));
#endif
    return edt_msg(level, message);
}


/**
* Conditionally (based on the flag bits) outputs \p message, followed by
* the last system error message, to \p msg_p.
*
* To output to the default message handler, use #edt_msg_perror.
*
* @param msg_p  pointer to message handler, initiailzed by #edt_msg_init
* @param level message level for the current message, as described in the overview
* @param msg   message to concatenate to the system error message
*
* @see edt_perror 
*/
int
edt_msg_output_perror(EdtMsgHandler *msg_p, int level, const char *msg)

{
    char message[MAX_MESSAGE];

#ifdef _NT_
    LPVOID *  lpMsgBuf;
#else
    extern int errno;
#endif

    if ((msg_p == &edt_default_msg) && !edt_default_msg_initialized)
	edt_msg_init(&edt_default_msg);

#ifdef _NT_

    FormatMessage(
	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
	NULL,
	GetLastError(),
	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),	/* Default language */
	(LPTSTR) & lpMsgBuf,
	0,
	NULL
	);

    sprintf(message, "%s: %s\n", msg, lpMsgBuf);
    LocalFree(lpMsgBuf);
#else
    sprintf(message, "%s: %s\n", msg, strerror(errno));
#endif

    return edt_msg_printf(msg_p->target, level, message);
}

/** Outputs a caller-specified message to the output, followed by the last system
* error message.
*
* This is useful when an error occurs, and you want your error message to
* be followed by the system's error message. 
*
* \Example:
* @code
* char *file_name = "/aFileThatDoesNotExist";
* FILE *file_ptr = fopen(file_name, "r");
* if (file_ptr == NULL) {
*     edt_msg_printf_perror(
*           EDTAPP_MSG_FATAL, 
*           "Couldn't open file '%s'",
*           file_name);
*     exit(1);
* }
* @endcode
* Which will print something like 
*   "Couldn't open file '/aFileThatDoesNotExist': No such file or
*   directory"
*
* @param level  the EDT Message level. This function will only output the
* message if level is greater than or equal to that set by #edt_msg_init,
* #edt_msg_set_level, #edt_msg_add_level, or #edt_msg_add_default_level.
*
* @param format  a printf() style format string.  Like printf(), it should be
* followed by arguments to match the format.
*
* @return 0 on success, -1 on failure.  
*/
int 
edt_msg_printf_perror(int level, const char *format, ...)
{
    va_list stack;
    va_start(stack, format);

    return edt_msg_output_vprintf_perror(&edt_default_msg, level, format, stack);
}


/** Writes to the specified EdtMsgHandler a caller-specified message (in the printf-style format)
* followed by the last system error message.
*
* If you want to just use the default handler (to just print to the console), use
* #edt_msg_printf_perror instead.
*
* @param msg_p  pointer to message handler, initiailzed by #edt_msg_init
* @param level  the EDT Message level. This function will only output the
* message if level is greater than or equal to that set by #edt_msg_init,
* #edt_msg_set_level, #edt_msg_add_level, or #edt_msg_add_default_level.
*
* @param format  a printf() style format string.  Like printf(), it should be
* followed by arguments to match the format.
*
* @see edt_msg_printf_perror for an example
*/
int
edt_msg_output_printf_perror(EdtMsgHandler *msg_p,  int level, const char *format, ...)
{
    va_list stack;
    va_start(stack, format);

    return edt_msg_output_vprintf_perror(msg_p, level, format, stack);
}


/** 
* Gets the default message handler.
*
* This is useful if you want to modify the default handler's behaviour,
* with functions such as #edt_msg_set_level, #edt_msg_set_function,
* #edt_msg_set_file, #edt_msg_set_name, or #edt_msg_set_target.
*/
EdtMsgHandler *
edt_msg_default_handle()
{
    if (!edt_default_msg_initialized)
	edt_msg_init(&edt_default_msg);

    return &edt_default_msg;
}

/**
* Gets the message level that messages must match in order to be
* handled by the default message handler.  The level is a combination
* of flags OR'ed together as described in the overview.
*
* The equivalent function for a user defined message handler is
* #edt_msg_get_level.
*/
int
edt_msg_default_level()
{
    if (!edt_default_msg_initialized)
	edt_msg_init(&edt_default_msg);

    return edt_default_msg.level;
}


/** 
* Gets the message last sent to the output by the edt message
* handling system. 
*/
char *edt_msg_last_error()
{
    return edt_last_error_message;
}

static int lvl_indent = 0;

int
lvl_printf(int delta, char *format, ...)
{
    va_list	stack;
    int i;
    char message[512];
    va_start(stack,	format);

    if (delta < 0)
	lvl_indent += delta;

    for (i=0;i<lvl_indent;i++)
	printf("    ");

    vsprintf(message, format, stack);

    printf("%s",message);
    if (delta > 0)
	lvl_indent += delta;

    return lvl_indent;
}

/*
* output message simplified - levels 0 - 3
* level 0 - fatal only
* level 1 - warnings
* level 2 - info level 1
* level 3 - info level 2
*/

void
edt_set_verbosity(int verbose)

{
    int level = edt_msg_default_level();
    if (verbose < 1)
	level = 0;
    else if (verbose > 1) /* default is slightly verbose */
    {
	level |= EDTAPP_MSG_INFO_1;
	level |= EDTLIB_MSG_INFO_1;
    }
    if (verbose > 2)
    {
	level |= EDTAPP_MSG_INFO_1
	    | EDTAPP_MSG_INFO_2
	    | EDTLIB_MSG_INFO_1
	    | EDTLIB_MSG_INFO_2;
    }
    edt_msg_set_level(edt_msg_default_handle(), level);
}

int
edt_get_verbosity()

{
    if (edt_default_msg.level & (EDTAPP_MSG_INFO_2 | EDTLIB_MSG_INFO_2))
        return 3;
    if (edt_default_msg.level & (EDTAPP_MSG_INFO_1 | EDTLIB_MSG_INFO_1))
        return 2;
    if (edt_default_msg.level & (EDT_DEFAULT_MSG_LEVEL))
        return 1;

    return 0;

}

