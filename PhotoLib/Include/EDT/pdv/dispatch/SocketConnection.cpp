/** ********************************
*
* @file SocketConnection.cpp
*
* Copyright (c) Engineering Design Team 2014
* All Rights Reserved
* Socket communications class for socket communications between processes
* which can be on separate systems
* Works for both Windows and Linux
*
*******************************************/


#include <stdlib.h>
#include <stdio.h>

#include <string>
#include "SocketConnection.h"


#include "ErrorHandler.h"

#ifdef WIN32

#include <windows.h>

typedef int socklen_t;

#ifndef LaunchThread

#define LaunchThread(pThread,func,pointer) { \
    unsigned int thrdaddr; \
    pThread = (thread_t) _beginthreadex(NULL, 0, func, pointer, CREATE_SUSPENDED, &thrdaddr);\
    ResumeThread(pThread); }

#define WaitForThread(pThread,timeout) \
    WaitForSingleObject(pThread,timeout)


typedef unsigned int  (__stdcall *edt_thread_function)(void *);

typedef HANDLE thread_type;

#define THREAD_RETURN UINT
#define THREAD_FUNC_DECLARE  UINT __stdcall


#endif

#else

#include <sys/ioctl.h>
#include <unistd.h>
#include "pthread.h"

#ifndef LaunchThread

#define LaunchThread(pThread,func,pointer) \
    pthread_create(&pThread,NULL,func,pointer);

#define WaitForThread(pThread,timeout) \
{ int return; pthread_join(pThread, &return;}

ypedef pthread_t thread_type;

#define THREAD_RETURN void *
#define THREAD_FUNC_MODIFIER
#define THREAD_FUNC_DECLARE THREAD_RETURN
typedef void * (*edt_thread_function)(void *);

#endif

#endif


#include <stdarg.h>


static bool quit_on_error = false;

static void
    print_sys_error(int err, const char *fmt, ...)
{

    char buffer[1024];
    va_list stack;
    va_start(stack,fmt);

    
#ifdef WIN32
    
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL, GetLastError(),0,
        buffer, 1023, NULL);
#else
    strerror_r(err, buffer, 1023);
    

#endif

    vsprintf(buffer + strlen(buffer), fmt, stack);

    SysError.Message("err=%x %s\n", err, buffer);

    if (quit_on_error)
        exit(1);
}

int
    SocketConnection::Send(const char *message, const int size)

{

    int actual = 0;
    int messagelen;

    if (!m_connected)
        return -1;

    if (size)
        messagelen = size;
    else
        messagelen = (int) strlen(message);

    if (messagelen == 0)
    {
        return 0;
    }

    if (verbose)
    {
        char buf[512];
        int i;
        int len = messagelen;
        if (len > 511)
            len = 511;

        for (i=0;i<len;i++)
            buf[i] = message[i];
        buf[len] = 0;

        SysLog.Message("Sending <%s> len %d\n", buf, messagelen);
    }
    if (is_udp)
    {
        int addrsize = sizeof(client_addr);

        if (is_server)
            actual = sendto(s, message, messagelen, 0, &client_addr, addrsize);
        else
            actual = sendto(s, message, messagelen, 0, NULL, 0);
    }
    else
    {
        int sent = 0;
        int remaining = messagelen;

        while (sent < messagelen)
        {
            actual = send(s, message + sent, remaining, 0);
            sent += actual;
            remaining -= actual;
        }

    }

    if (actual == SOCKET_ERROR)
    {
        print_sys_error(actual, "Problem with send ... ");
        return -1;
    }

    return 0;

}

int
    SocketConnection::Send(const string &s)

{
    return Send(s.c_str());
}

int
    SocketConnection::Recv(char *message, int maxsize)

{

    int actual;

    if (!m_connected)
        return -1;

    if (!last_message || buffer_size < maxsize)
        AllocateLastMessage(maxsize);

    if (verbose)
    {
        int n = DataPending();
        SysLog.Message("Pending data = %d\n",n);
    }

    if (is_udp)
    {
        socklen_t addrsize = sizeof(client_addr);
        actual = recvfrom(s, last_message, maxsize, 0, &client_addr, &addrsize);
        //actual = recvfrom(s, last_message, maxsize, 0, NULL, 0);

    }
    else
    {
        actual = recv(s, last_message, maxsize, 0);
    }

    if (actual == SOCKET_ERROR)
    {
        int err;
#ifdef WIN32
        
        err = GetLastError();

        if (err == WSAECONNRESET || err == WSAECONNABORTED || err == WSAENETRESET || err == WSAENETDOWN)
        {
            Close();
            return -1;
        }
#else
        err=actual;
#endif
        print_sys_error(err, "Problem with recv ... ");
        return -1;
    }
    else if (actual == 0)
    {
        Close();
    }
    else
    {
        if (message)
        {
            memcpy(message,last_message,actual);

            if (!binary)
            {


                int i;
                for (i=0;i<actual;i++)
                    if (message[i] == 0)
                        message[i] = ' ';

                message[actual] = 0;
            }
        }
    }


    return actual;

}
int
    SocketConnection::RecvAll(char *message, int size)

{

    int cursize;
    int needsize ;
    char *curptr ;

    curptr = message ;
    needsize = size ;

    while(needsize > 0)
    {
        cursize = Recv(curptr, needsize) ;
        if (cursize <= 0) return (cursize) ;
        else
        {
            needsize -= cursize ;
            curptr += cursize ;
        }
    }
    return(size) ;
}



int
    SocketConnection::DataPending()

{
    unsigned long nbytes = 0;
    int rc;
#ifdef WIN32

    rc = ioctlsocket(s, FIONREAD, &nbytes);

#else
    ioctl(s, FIONREAD, &nbytes);

#endif

    if (rc != 0)
        print_sys_error(rc, "DataPending error\n");
    return nbytes;

}

#ifdef __linux__

#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>



int SocketConnection::GetHostIP(const char * name)

{
    struct ifaddrs * if_list=NULL;
    struct ifaddrs * if_addr=NULL;
    void * p =NULL;
    bool found = false;

    getifaddrs(&if_list);

    for (if_addr = if_list; if_addr != NULL; if_addr = if_addr->ifa_next) {
        if (if_addr ->ifa_addr->sa_family==AF_INET &&
            !strcmp(name, if_addr->ifa_name))
        {

            p=&((struct sockaddr_in *)if_addr->ifa_addr)->sin_addr;

            char buf[INET_ADDRSTRLEN];

            inet_ntop(AF_INET, p, buf, INET_ADDRSTRLEN);

            hostname = buf;

            SysLog.Message("%s IP Address %s\n", if_addr->ifa_name, buf);

            found = true;
        }
    }
    if (if_list!=NULL) freeifaddrs(if_list);

    return (found)?0:-1;
}

#else

int SocketConnection::GetHostIP(const char * name)

{
    GetMyName();

    hostname = my_hostname;

    return 0;
}


#endif

int
    SocketConnection::OpenConnectionServer(const char *in_server_name,
    int port,
    bool udp)

{

    struct hostent *he;
    int value;
    socklen_t other_len;
    int rc;
    SOCKET accepted_socket;

    /* set up the socket */

    is_server = TRUE;

    /* create the socket */
    is_udp = udp;

    if (udp)
        s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    else
        s = socket(AF_INET, SOCK_STREAM, 0);

    /* bind the socket */
    if (in_server_name && strlen(in_server_name))
        hostname = in_server_name;

    if (hostname.size() == 0)
    {
        if (verbose)
            SysLog.Message("calling gethostname\n");
        int rc = GetHostIP(net_name.c_str());
        if (verbose)
            SysLog.Message("rc = %d\n", rc);
    }


    he = gethostbyname(hostname.c_str());

    if (he == NULL)
    {
	    SysError.Message("Unable to find host <%s>\n", hostname.c_str());
	    m_connected = false;
	    return -1;

    }
    my_address.sin_addr.s_addr = *(unsigned long *)he->h_addr;
    my_address.sin_family = AF_INET;
    my_address.sin_port = htons(port);

    if (verbose)
        SysLog.Message("listening at %d.%d.%d.%d:%d\n",
        (unsigned char) he->h_addr_list[0][0],
        (unsigned char) he->h_addr_list[0][1],
        (unsigned char) he->h_addr_list[0][2],
        (unsigned char) he->h_addr_list[0][3],
        port);

    value = 1;

    if(rc = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *) &value,
        sizeof(value)))
    {
        print_sys_error(rc, "Setsockopt failed\n");
        return -1;
    }

#ifdef WIN32
    if (!udp)
    {
        if(rc = setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char *) &value,
            sizeof(value)))
        {
            print_sys_error(rc, "Setsockopt failed\n");
            return -1;
        }
    }
#endif


    {
        int ret ;
        /* CANT BIND udp socket to addr not n this machine */
        ret = bind(s,(const struct sockaddr *)&my_address,
            sizeof(my_address));
        if(ret == SOCKET_ERROR)  {
            print_sys_error(ret, "bind socket failed\n");
            //return -1;

        }
    }


    /* listen */

    if (udp)
    {

    }
    else
    {

        SysLog.Message("Listening on TCPIP socket host %d.%d.%d.%d port %d\n",
            (unsigned char) he->h_addr_list[0][0],
            (unsigned char) he->h_addr_list[0][1],
            (unsigned char) he->h_addr_list[0][2],
            (unsigned char) he->h_addr_list[0][3],
            port);

        rc = listen(s,5);

        SysLog.Message("Return from listen ...\n");

        if(rc) {
            print_sys_error(rc, "Listen FAILED\n");
            return -1;
        }

        other_len = sizeof(other_side);

        /* accept */
        accepted_socket  = accept(s, (struct sockaddr *)&other_side, &other_len );
        if(accepted_socket == INVALID_SOCKET) {
            print_sys_error(accepted_socket, "Accept FAILED: err=%d\n", errno);
            return -1;
        }

        if (verbose)
            SysLog.Message("Connection made to %d.%d.%d.%d\n",
            (unsigned char) other_side.sin_addr.s_addr & 0xff,
            (unsigned char) ((other_side.sin_addr.s_addr & 0xff00) >> 8),
            (unsigned char) ((other_side.sin_addr.s_addr & 0xff0000) >> 16),
            (unsigned char) ((other_side.sin_addr.s_addr & 0xff000000) >> 24));

        /* Keep the actual accepted socket */
#ifdef WIN32
        closesocket(s);
#else
        close(s);

#endif
        s = accepted_socket;

    }

    m_connected = true;

    return 0;

}

int
    SocketConnection::OpenConnectionClient(const char *in_server_name, int port, bool udp)

{
    struct hostent *he;


    is_udp = udp;

    if (in_server_name && strlen(in_server_name))
        hostname = in_server_name;

    if (hostname.size() == 0)
    {
        if (verbose)
            SysLog.Message("calling gethostname\n");
        int rc = gethostname(host_buf, 256);
        if (verbose)
            SysLog.Message("rc = %d\n", rc);
        hostname = host_buf;
    }

    he = gethostbyname(hostname.c_str());

    if (he == NULL)
    {
	    SysError.Message("Unable to find host <%s>\n", hostname.c_str());
	    m_connected = false;
	    return -1;
    }
    if (verbose)
        SysLog.Message("he = %p hostname = %s\n", he, hostname.c_str());

    my_address.sin_addr.s_addr = *(unsigned long *)he->h_addr;
    my_address.sin_family = AF_INET;
    my_address.sin_port = htons(port);

    if (verbose)
        SysLog.Message("connecting to %d.%d.%d.%d:%d\n",
        (unsigned char) he->h_addr[0],
        (unsigned char) he->h_addr[1],
        (unsigned char) he->h_addr[2],
        (unsigned char) he->h_addr[3],
        port);

    if (udp)
        s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    else
        s = socket(AF_INET, SOCK_STREAM, 0);

    int rc; 

    if ((rc = connect(s, (const struct sockaddr *) &my_address,
        sizeof(my_address))) != 0)
    {
        if (verbose)
		print_sys_error(rc, "Error on connect\n");
        return -1;
    }

    is_server = FALSE;

    m_connected = true;

    return 0;

}

int
    SocketConnection::Close()

{

    if (s)
    { 

#ifdef WIN32
        closesocket(s);
#else
        close(s);
#endif

        s = 0;
    }

    m_connected = false;

    return 0;

}

int SocketConnection::Init(const char *server_name,
    int port,
    int max_buffer,
    int in_timeout,
    bool server,
    bool udp)

{

    int rc;



    AllocateLastMessage(max_buffer);

    timeout = in_timeout;

    is_server = server;


    if (is_server)
    {
        rc = OpenConnectionServer(server_name, port, udp);

    }
    else
    {
        /* reverse base names for client */
        rc = OpenConnectionClient(server_name, port, udp);
    }

    if (rc != 0)
    {

        SysLog.Message("Problem opening sockets\n");
        return -1;
    }

    return 0;


}

void SocketConnection::SetEndstring(char *endstring)

{
    if (end_string)
        free(end_string);

    end_string = strdup(endstring);
}


SocketConnection::SocketConnection(SOCKET in_s, SocketConnection *from)
{
#ifdef WIN32

    {
        WSADATA WSAData;

        WSAStartup(0x0202, &WSAData);
    }

#endif
    last_message = NULL;
    end_string = NULL;
    verbose = 0;
    listening = false;
    timeout = 1;
    m_connected = false;

    listen_thread = (thread_type) NULL;

    if (from)
    {
        s = in_s;
        my_address = from->my_address;
        other_side = from->other_side;

        listen_port = from->listen_port;
        buffer_size = from->buffer_size;

        AllocateLastMessage(buffer_size);
        is_server = true;
        connector = from->connector;
        listen_target = from->listen_target;
        is_udp = from->is_udp;
        binary = from->binary;
        m_connected = from->m_connected;
        verbose = from->verbose;
        m_port = from->m_port;
        hostname = from->hostname;
    }
    else
    {

        
        buffer_size = 1024;
        is_server = false;
        end_string = NULL;
        is_udp = false;
        binary = true ;
        listen_target = NULL;
        connector = NULL;
        s = 0;
        memset(&my_address,0,sizeof(my_address));
        memset(&other_side,0,sizeof(other_side));
        listen_port = 0;
        m_port = 0;
    }

    net_name = "eth0";

}

unsigned int
    SocketConnection::Listen()

{

    struct hostent *he;
    int value;
    socklen_t other_len;
    int rc;
    SOCKET accepted_socket;

    /* set up the socket */

    is_server = TRUE;

    /* create the socket */

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
        print_sys_error(s, "Failed to open socket");


    if (hostname.size() == 0)
    {
        if (verbose)
            SysLog.Message("calling gethostname\n");
        int rc = gethostname(host_buf, 256);
        if (verbose)
            SysLog.Message("rc = %d\n", rc);
        hostname = host_buf;
    }

    /* bind the socket */

    he = gethostbyname(hostname.c_str());

    if (he == NULL)
    {
	    SysError.Message("Unable to find host <%s>\n", hostname.c_str());
	    m_connected = false;
	    return -1;
    }
     
    my_address.sin_addr.s_addr = *(unsigned long *)he->h_addr;
    my_address.sin_family = AF_INET;
    my_address.sin_port = htons(listen_port);

    if (verbose)
        SysLog.Message("thread listening at %d.%d.%d.%d:%d\n",
        (unsigned char) he->h_addr_list[0][0],
        (unsigned char) he->h_addr_list[0][1],
        (unsigned char) he->h_addr_list[0][2],
        (unsigned char) he->h_addr_list[0][3],
        listen_port);

    value = 1;

    if(rc = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *) &value,
        sizeof(value)))
    {
        print_sys_error(rc, "Setsockopt failed\n");
        return -1;
    }

#ifdef WIN32

    if(rc = setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char *) &value,
        sizeof(value)))
    {
        print_sys_error(rc, "Setsockopt failed\n");
        return -1;
    }

#endif


    if(bind(s,(const struct sockaddr *)&my_address,
        sizeof(my_address)) == SOCKET_ERROR)  {
            print_sys_error(SOCKET_ERROR, "bind socket failed\n");
            return -1;

    }

    while (listening)
    {
        if (verbose)
            SysLog.Message("Listening on TCPIP socket host %d.%d.%d.%d port %d\n",
            (unsigned char) he->h_addr_list[0][0],
            (unsigned char) he->h_addr_list[0][1],
            (unsigned char) he->h_addr_list[0][2],
            (unsigned char) he->h_addr_list[0][3],
            listen_port);

        rc = listen(s,5);

        if (verbose)
            SysLog.Message("rc from listen %d listening %d\n", rc, listening);

        if (listening)
        {
            if(rc) {
                print_sys_error(rc, "Listen FAILED\n");
                return -1;
            }

            other_len = sizeof(other_side);

            /* accept */
            accepted_socket  = accept(s, (struct sockaddr *)&other_side, &other_len );
            if(accepted_socket == INVALID_SOCKET) {
                print_sys_error(INVALID_SOCKET, "Accept FAILED: err=%d\n", errno);
                return -1;
            }

            if (verbose)
                SysLog.Message("Connection made to %d.%d.%d.%d\n",
                (unsigned char) other_side.sin_addr.s_addr & 0xff,
                (unsigned char) ((other_side.sin_addr.s_addr & 0xff00) >> 8),
                (unsigned char) ((other_side.sin_addr.s_addr & 0xff0000) >> 16),
                (unsigned char) ((other_side.sin_addr.s_addr & 0xff000000) >> 24));

            SocketConnection * instance = new SocketConnection(accepted_socket, this);

            instance->m_connected = true;
            connector(listen_target, instance);

        }


    }

    return 0;

}



THREAD_FUNC_DECLARE listen_func(void *p)

{
    SocketConnection *obj = (SocketConnection *) p;

    return (THREAD_RETURN) obj->Listen();

}


int SocketConnection::ListenServer(int port,
    int max_buffer,
    int in_timeout,
    void *target,
    ConnectionHandler hndl)
{
    if (listening)
    {
        SysLog.Message("Already listening\n");
        return -1;
    }
    connector = hndl;
    listen_target = target;
    listen_port = port;
    timeout  = in_timeout;
    is_udp = false;
    AllocateLastMessage(max_buffer);

    listening = true;

    LaunchThread(listen_thread, listen_func, this);

    return 0;
}


int SocketConnection::ReadString(char *buffer, int maxlen)

{
    int rc =  Recv(buffer, maxlen);
    if (rc >= 0)
        buffer[rc] = 0;

    return rc;
}


int SocketConnection::ReadString(string &buffer)

{
    if (buffer_size == 0)
        AllocateLastMessage(4096);

    int rc =  Recv(NULL, buffer_size);
    if (rc >= 0)
    {
        last_message[rc] = 0;
        buffer = last_message;

        if (verbose)
            printf("Received <%s>\n", last_message);
    }

    return rc;
}

int SocketConnection::WriteString(const char *buffer, bool newline)

{

    return Send(buffer);

}


int SocketConnection::WriteBuffer(const char *buffer, int len)

{

    return Send(buffer, len);

}

int SocketConnection::ReadBuffer(char *buffer, int len)

{

    return Recv(buffer, len);
}

int SocketConnection::ReadChar()

{
    char buf[2];
    Recv(buf,1);
    return buf[0];
}

int SocketConnection::WriteChar(int c)

{
    char buf[2];
    buf[0] = c;
    Send(buf,1);
    return c;
}

bool SocketConnection::Interactive()

{
    return false;
}

bool SocketConnection::HasPosition()

{
    return false;
}

bool SocketConnection::Finished()

{
    return !Connected();
}


int SocketConnection::Open(const char *name)

{

    int rc;

    if (IsServer())
        rc = OpenConnectionServer(name, m_port, false);
    else
        rc = OpenConnectionClient(name, m_port, false);

    return rc;
}

bool SocketConnection::IsServer()

{
    return is_server;
}

void SocketConnection::SetPortNumber(int port)

{
    m_port = port;
}

int SocketConnection::GetPortNumber()

{
    return m_port;
}


const char *SocketConnection::GetHostName()

{
    return hostname.c_str();
}


void SocketConnection::SetServer(const bool state)

{
    is_server = state;
}

bool SocketConnection::CheckConnected()

{
    if (!is_udp)
    {
        int rc;
        char buf[2];
        rc = recv(s,buf,0, MSG_PEEK);

        if (rc != 0)
        {
            m_connected = false;
        }
    }

    return m_connected;
}

bool SocketConnection::Connected()

{

    return m_connected;
}

void SocketConnection::SetHostName(const char *name)

{
    hostname = name;
}
void SocketConnection::SetHostName(const string &name)

{
    hostname = name;
}

void SocketConnection::SetTimeout(const int ms)

{

    timeout = ms;

}

int SocketConnection::WaitForInput(const int in_timeout)

{
    int ms;

    timeval t;
    fd_set rset;


#ifdef WIN32

    rset.fd_count = 1;
    rset.fd_array[0] = s;

#else
    FD_ZERO(&rset);
    FD_SET(s,&rset);
#endif

    if (in_timeout != -1)
        ms  = in_timeout;
    else
        ms  = timeout;


    t.tv_usec = (ms % 1000) * 1000;
    t.tv_sec = ms/1000;

#ifdef WIN32
    int rc = select(5, &rset, NULL, NULL, &t);
#else
    int rc = select(s+1, &rset, NULL, NULL, &t);
#endif


    if (rc > 0)
    {
        if (CheckConnected())
        {
            rc = DataPending();
            if (rc == 0)
                return -1;
            else
                return rc;
        }
        else
            return -1;
    }
    else if (rc == -1)
    {
        SysLog.Message("Socket select error\n");
        return -1;
    }
    else
        return 0;
}

const string &SocketConnection::GetMyName()

{
    char my_host_buf[256];

    int rc = gethostname(my_host_buf, 256);

    my_hostname = my_host_buf;

    return my_hostname;
}

void SocketConnection::SetNetName(const string &name)

{
    net_name = name;
}

const string &SocketConnection::GetNetName()

{
    return net_name;
}
