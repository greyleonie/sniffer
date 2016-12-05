/********************************************************
* Socket Network Transfer Client Head 
* Compile:	gcc
* Author:  	grey	
* Last Modify:  		2009.05.19
*********************************************************/

#ifndef __SOCKET_H__
#define	__SOCKET_H__


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
socket_open(void);


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
	);
	
	
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
	);
	
	
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
	);
	
	
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
	);	
	

#endif/* __SOCKET_H__ */

