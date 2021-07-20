
#include "edtinc.h"
#include "edt_net_msg.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h> 

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> /* ntoa */

#include <netdb.h> 

/** Sends message to an already opened socket. */
static void
edt_net_send_socket (char *msg, int sockfd) 
{
	int len, bytes_sent;

	/* send message */
	len = strlen(msg);
	if ((bytes_sent = send(sockfd, msg, len, 0)) == -1) {
		/* We use console output here, in case the default msg
		 * handler has been replaced with this network handler and
		 * we could otherwise have a infinite recursion while trying
		 * to send() the error message regarding send().
		 */
		EdtMsgHandler console_out;
		edt_msg_init(&console_out);
		edt_msg_output_printf_perror(&console_out, EDTLIB_MSG_WARNING,
				"send() failed");
		edt_msg_close(&console_out);
	} else if (bytes_sent < len) {
		EdtMsgHandler console_out;
		edt_msg_init(&console_out);
		edt_msg_output_printf_perror(&console_out, EDTLIB_MSG_WARNING,
				"not all bytes sent");
		edt_msg_close(&console_out);
	}

	#ifdef DEBUG
	printf("sent %d bytes\n", bytes_sent);
	#endif

	return;
}


	
int 
edt_msg_net_send (void *target, int level, char *message)
{
	NetMsgData *net_info = (NetMsgData*)target;
	edt_net_send_socket(message, net_info->sockfd);
	if (net_info->use_stdout) {
		/* send duplicate message to stdout.
		 * useful for apps wanting to print & log to network. */
		 printf("%s", message);
	}
	return 0;
}



/** Initializes networking. 
 * Connects socket to host.
 */
void
edt_net_init (NetMsgData *net_info) 
{
	struct hostent *host;
	struct sockaddr_in dest_addr;

	if ((host = gethostbyname(net_info->host)) == NULL) 
	{
		char errstr[128];

		switch (h_errno) {
			case HOST_NOT_FOUND:
				sprintf(errstr, "Host '%s' not found.\n", 
						net_info->host);
				break;
			case NO_ADDRESS:
				sprintf(errstr, "No IP address available for host '%s'\n",
						net_info->host);
				break;
			case NO_RECOVERY:
				sprintf(errstr, "A non-recoverable name server error occured.\n");
				break;
			case TRY_AGAIN:
				sprintf(errstr, "A tempory error occured on an authoritative "\
						"name server. Try again later.\n");
				break;
			default:
				sprintf(errstr, "Unknown error occurred.\n");
				break;

		}
		edt_msg(EDT_MSG_FATAL, errstr);
		exit(1);
	}


	net_info->sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (net_info->sockfd == -1) 
	{
		edt_perror("socket() failed");
		exit(1);
	}

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(EDT_LOG_PORT);
	dest_addr.sin_addr = *((struct in_addr *)host->h_addr);
	memset(&(dest_addr.sin_zero), '\0', 8); /* zero rest of struct */


	/* connect to remote host */
	#ifdef DEBUG
	printf("connecting to %s (%s)\n", 
			net_info->host, inet_ntoa(dest_addr.sin_addr));
	#endif

	if (connect(net_info->sockfd, 
				(struct sockaddr *)&dest_addr, 
				sizeof(struct sockaddr)) == -1) 
	{
		char errstr[256];
		sprintf(errstr, "connect() to host %s (%s) port %d failed",
			net_info->host, inet_ntoa(dest_addr.sin_addr), EDT_LOG_PORT);
		edt_perror(errstr);
		exit(1);
	}
}

/** Frees resources.
 */
void 
edt_net_close (NetMsgData *net_info) 
{
	close(net_info->sockfd);
}




/*
 * Connects to the host specified by hostname and sends the specified
 * message. 
 *
 * @param hostname Name or IP address to send message to.
 * @param msg      The message to send.
 */
void 
edt_net_send (char *msg, char *hostname)
{

	NetMsgData net_info;

	net_info.host = hostname;
	net_info.use_stdout = 0;
	edt_net_init(&net_info);

	edt_net_send_socket(msg, net_info.sockfd);

	close(net_info.sockfd);

	return;
}
