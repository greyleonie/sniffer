

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/times.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "com_port.h"
#include "debug.h"

#define	TIMEOUT_SEC(len,baud)	(len * 20 / baud + 2)
#define	TIMEOUT_USEC	0



int main(int argc, char **argv)
{
	COM_PORT	port;
	int		fs_sel, sel_id;
	fd_set	fs_read;
	struct timeval to;
	unsigned char	recv_buffer[64];
	int		len, i;
	unsigned char usb_id = 2;
	
	DBG(("start\n"));
	
	//socket
	int		socket_fd;
	struct sockaddr_in	server_addr;
	
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	
	bzero(&server_addr, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(5001);
	server_addr.sin_addr.s_addr = inet_addr("192.168.75.129");
	if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) == -1)
	{
		DBG(("\nsocket connect failed...\n"));
		exit(1);
	}
//*/	
	DBG(("connect to %s, port %d\n",
				inet_ntop(AF_INET, &server_addr.sin_addr, recv_buffer, sizeof(recv_buffer)),
				ntohs(server_addr.sin_port)));

	bzero(port.path_name, 16);
	strcpy(port.path_name, argv[1]);
	
	port.flags = O_RDONLY;
	port.flags |= O_NOCTTY;
	port.flags |= O_NONBLOCK;
	port.baud_rate = atoi(argv[2]);
	port.data_bit = 8;
	port.fctl = 0;
	port.parity = 0;
	port.stop_bit = 1;
	
	com_open(&port);
	com_set(&port);
	

	
	for (;;)
	{
		// config select system call
		FD_ZERO(&fs_read);
		FD_SET(port.fd, &fs_read);
		to.tv_sec = TIMEOUT_SEC(1, port.baud_rate);
		to.tv_usec = TIMEOUT_USEC;
		
		fs_sel = select(port.fd + 1, &fs_read, NULL, NULL, &to);
		if (fs_sel)	
		{
			if (FD_ISSET(port.fd, &fs_read))
			{
				len = com_recv(recv_buffer, &port);
				if (len > 0)
				{
					DBG(("\n"));
					for (i = 0; i < len; ++i)
						DBG(("%.2x ", recv_buffer[i]));
					//*/			
					if (send(socket_fd, recv_buffer, len, 0) == -1)
					{
						DBG(("\nERROR: socket send failed...\n"));
						exit(1);
					}
				}
			}
		}
		else if (fs_sel == 0)
		{
			DBG(("\nWarning: select() timeout...\n"));
			
			//exit(1);
/*			DBG(("no data\n"));
			
			if (access(port.path_name, F_OK) != 0)
			{
				DBG(("%s have pulled out\n", port.path_name));
				break;
			}
			//*/
		}
		else
		{
			DBG(("\nERROR: select() return %d\n", fs_sel));
			exit(1);
		}
	}// end for
	

	
	return 0;
}
