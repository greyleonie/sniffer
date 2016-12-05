/********************************************************
* Socket Network Transfer Client  
* Compile:	gcc
* Author:  	grey	
* Last Modify:  		2009.05.19
*********************************************************/

#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>

//#define	_USE_DEBUG_
#include "debug.h"

/***********************************************************
*	socket_open()
*		Open one TCP/IP stream socket
*
* return:
*		>0 --> socket number
*		-1 --> failed
*
* parameter:
*	
************************************************************/
int
socket_open(void)
{
	int		socket_fd;
	
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		DBG(("ERROR:socket open failed.\n"));
		return -1;
	}
	
	DBG(("Socket %d opened.\n", socket_fd));
	
	return socket_fd;
}
	
	
/***********************************************************
*	socket_connect_dst()
*		Connect client socket to server
*
* return:
*		0 --> success 
*		-1 --> failed
*
* parameter:
*		@socket_fd		socket file descriptor
*		@dst_ip				server IP address
* 	@dst_port			server listen port number
* 
************************************************************/
int
socket_connect_dst(
	int		socket_fd,
	char	*dst_ip,
	int		dst_port
	)
{
	struct sockaddr_in	dst_addr;
	
	bzero(&dst_addr, sizeof(struct sockaddr_in));
	dst_addr.sin_family = AF_INET;
	dst_addr.sin_port = htons(dst_port);
	dst_addr.sin_addr.s_addr = inet_addr(dst_ip);
	if (connect(socket_fd, (struct sockaddr *)&dst_addr, sizeof(struct sockaddr_in)) == -1)
	{
		DBG(("WARNING:socket connect to %s port %d failed.\n", dst_ip, dst_port));
		return -1;
	}
	
	DBG(("Socket connected to %s port %d.\n", dst_ip, dst_port));
	return 0;
}
	
	
/***********************************************************
*	socket_close()
*		Close socket of the fd
*
* return:
*
* parameter:
*		@socket_fd		socket file descriptor
* 
************************************************************/
void
socket_close(
	int		socket_fd
	)
{
	DBG(("EVENT:socket %d closed.\n", socket_fd));
	if (socket_fd > 0)
		close(socket_fd);
}


/***********************************************************
*	socket_recv()
*		Receive data from socket
*
* return:
*		>0 --> Received bytes counter
*		-1 --> failed
*
* parameter:
*		@socket_fd		socket file descriptor
*		@buffer				receive data buffer pointer
* 	@length				receive data buffer max length
* 
************************************************************/
int				
socket_recv(
	int						socket_fd,
	unsigned char	*buffer,
	int						length
	)
{
	int		ret;
	
	ret = recv(socket_fd, buffer, length, 0);
	if (ret == -1)
		DBG(("ERROR:socket receive data failed.\n"));

	return ret;
}


/***********************************************************
*	socket_send()
*		Send data by socket
*
* return:
*		>0 --> sent bytes counter
*		-1 --> failed
*
* parameter:
*		@socket_fd		socket file descriptor
*		@buffer				send data buffer pointer
* 	@length				send data length
* 
************************************************************/
int 			
socket_send(
	int						socket_fd,
	unsigned char	*buffer,
	int						length
	)
{
	int 	ret;
	
	ret = send(socket_fd, buffer, length, 0);
	if (ret == -1)
		DBG(("ERROR:socket send data failed.\n"));
	
	return ret;
}


