/** ********************************
* 
* @file dispatch/SocketConnection.h
*
* Header file for socket communications between processes
* which can be on separate systems 
* Works on both Windows and Linux* Copyright (c) Engineering Design Team 2010
* All Rights Reserved
*/


#ifndef SC_LIB_H
#define SC_LIB_H

#ifdef WIN32

#pragma comment(lib, "Ws2_32.lib")

#include <winsock2.h>

#include <windows.h>

#else

#include <sys/socket.h>
#include <netinet/in.h>
#include <asm/ioctls.h>
#include <netdb.h>
#include <errno.h>


#define SOCKET int

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define SOCKET_ERROR -1
#define INVALID_SOCKET -1

#endif

#include "ErrorHandler.h"

#define MSG_BUFFER_SIZE 512

#define DEFAULT_TIMEOUT 2000

#define DEFAULT_SERVER_NAME	"."

#define DEFAULT_END_STRING "exit"

class SocketConnection;

#include "EdtCmdPort.h"

#include <string>

typedef int (*ConnectionHandler)(void *target, SocketConnection *sock);

class SocketConnection : public EdtCmdPort

{

protected:

    bool is_server;

    string hostname;
    string my_hostname;
    string net_name;

    char host_buf[256];

    SOCKET s;

    struct sockaddr_in my_address;
    struct sockaddr_in other_side;

    struct sockaddr client_addr;

    char *last_message;
    char *end_string;

    int timeout;

    int buffer_size;

    int verbose;

    bool binary;

    bool is_udp;

    bool spawning;

    void *listen_target;

    ConnectionHandler connector;

    int listen_port;

    thread_type listen_thread;

    bool listening;

    bool m_connected;

    int m_port;



public:

    SocketConnection(SOCKET in_s = NULL, SocketConnection *from = NULL);
    virtual ~SocketConnection() {}

    virtual int Send(const char *message, int length = 0);
    virtual int Send(const string &message);

    virtual int Recv(char *message, int maxsize);

    virtual int RecvAll(char *message, int size);

    virtual int ReadString(char *buffer, int maxlen);
    virtual int ReadString(string &buffer);

    virtual int ReadChar();
    virtual int WriteChar(int c);

    virtual int ReadBuffer(char *buffer, int maxlen);

    virtual int WriteString(const char *buffer, bool newline = true);

    virtual int WriteBuffer(const char *buffer, int len);

    virtual int WaitForInput(const int timeout);

    virtual bool Interactive();
    virtual bool HasPosition();

    void SetPortNumber(int port);
    int GetPortNumber();


    void SetServer(const bool state);
    bool IsServer();

    virtual bool Finished();
    int DataPending();

    int GetHostIP(const char * name);

    int	OpenConnectionServer(const char *in_server_name, int port, bool udp);

    int	OpenConnectionClient(const char *server_name,
        int port, bool udp);

    int Open(const char *name);

    // listen in a separate thread, and spawn connections using hndl

    int ListenServer(int port, 
        int max_buffer, 
        int timeout,
        void *target, 
        ConnectionHandler hndl);


    int Close();

    int Init(const char *server_name, 
        int port,
        int max_buffer, 
        int timeout,
        bool server,
        bool udp = false);

    void SetEndstring(char *endstring);

    void SetBinary(const bool state)
    {
        binary = state;
    }

    bool GetBinary() const
    {
        return binary;
    }

    bool IsUDP()
    {
        return is_udp;
    }

    void SetUDP(bool state)
    {
        is_udp = state;
    }

    unsigned int Listen();

    void StopListening()
    {
        listening = false;
    }

    void AllocateLastMessage(int max_buffer)

    {

        if (last_message)
            free(last_message);

        last_message = (char *) malloc(max_buffer+1);
        last_message[0] = 0;

        buffer_size = max_buffer;

    }

    bool Connected();

    void SetHostName(const char *name);
    void SetHostName(const string &name);

    const char *GetHostName();

    void SetTimeout(const int ms);

    bool CheckConnected();

    const string& GetMyName();

    void SetNetName(const string &name);
    const string &GetNetName();


} ;



#endif

