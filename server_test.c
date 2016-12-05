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
#include <sys/time.h>
#include <arpa/inet.h>


#define	STDIN		0

int
recv_msg(
	int		sock_fd
	)
{
	// select control
	int				fs_sel;
	fd_set		fs_read;
	
	int			receive_len;
	unsigned char	buffer[64];
	int			j;
	
	for (;;)
	{
		// config select system call
		FD_ZERO(&fs_read);
		FD_SET(sock_fd, &fs_read);
		
		fs_sel = select(sock_fd + 1, &fs_read, NULL, NULL, NULL);	
		if (fs_sel)
		{
			if (FD_ISSET(sock_fd, &fs_read))
			{
				receive_len = recv(sock_fd, buffer, 64, 0);
				if (receive_len > 0)
				{
					for (j = 0; j < receive_len; ++j)
						printf("%.2x ", buffer[j]);
					printf("\n");
				}
			}//end if (FD_ISSET(sock_fd, &fs_read))
		}
		else if (fs_sel == 0)
		{
			
		}
		else
		{
			return -1;
		}
	}//end for
}



int main(int argc, char **argv)
{
	int		listen_fd;
	pid_t	child_id;
	socklen_t	client_len;
	struct	sockaddr_in		client_addr, server_addr;
	unsigned char	buffer[20];
	
	int			conn_fd_temp;
	int			conn_fd[8];
	int			conn_fd_cnt = 0;
	int			i;
		
	int		fs_sel;
//	int		sel_id;
	fd_set	fs_read;
//	struct timeval to;
	
	char		cmd[32];
	int			cmd_length;
	char		*pcmd;
	
//	struct timeval tv;
//	struct timezone tz;
	
	printf("server start...\n");
	
/*	gettimeofday(&tv, &tz);
	printf("tv_sec; %d\n", tv.tv_sec);
	printf("tv_usec; %d\n",tv.tv_usec);
	printf("tz_minuteswest; %d\n", tz.tz_minuteswest);
	printf("tz_dsttime, %d\n", tz.tz_dsttime);
//*/
	
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	
	bzero(&server_addr, sizeof(struct	sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(5001);
	
	bind(listen_fd, (struct	sockaddr *)&server_addr, sizeof(struct	sockaddr_in));
	
	printf("listen port %d...", ntohs(server_addr.sin_port));
	listen(listen_fd, 8);

	for (;;)
	{
		// config select system call
		FD_ZERO(&fs_read);
		
		// set all select fd
		FD_SET(STDIN, &fs_read);
		FD_SET(listen_fd, &fs_read);
		
		fs_sel = select(listen_fd + 1, &fs_read, NULL, NULL, NULL);
		if (fs_sel)	
		{
			if (FD_ISSET(listen_fd, &fs_read))		//process accept
			{
				printf("do accept...\n");
				client_len = sizeof(struct	sockaddr_in);
				conn_fd_temp = accept(listen_fd, (struct	sockaddr *)&client_addr, &client_len);
				
				printf("accept done\n");
				if (conn_fd_temp > 0)	// get new conn_fd
				{
					printf("connect from %s, port %d\n", 
										inet_ntop(AF_INET, &client_addr.sin_addr, buffer, sizeof(buffer)),
										ntohs(client_addr.sin_port));
					
					child_id = fork();	// open a child process for send command
					if (child_id == 0)
					{
						close(listen_fd);	// close parent process listen_fd
						recv_msg(conn_fd_temp);
						_exit(0);
					}
					else if (child_id > 0)
					{
						printf("open receive message process id = %d\n", child_id);
						conn_fd[conn_fd_cnt] = conn_fd_temp;
						conn_fd_cnt++;
					}
					else
					{
						printf("fork failed.\n");
						return -1;
					}
				}//end if (conn_fd_temp > 0)
				else
				{
					printf("accept failed.\n");
					return -1;
				}
			}//end if (FD_ISSET(listen_fd, &fs_read))
			
			if (FD_ISSET(STDIN, &fs_read))		//process command input
			{
				fgets(cmd, 32, stdin);
//				printf("%s\n", cmd);
				if (memcmp(cmd, "state", sizeof("state") - 1) == 0)
				{
					cmd[0] = 0x7e;
					cmd[1] = 0x76;
					cmd[2] = 0x7e;
					cmd_length = 3;
					printf("send command:state.\n");
				}
				else if (memcmp(cmd, "start", sizeof("start") - 1) == 0)
				{
					cmd[0] = 0x7e;
					cmd[1] = 0x70;
					cmd[2] = 0x7e;
					cmd_length = 3;
					printf("send command:start.\n");
				}
				else if (memcmp(cmd, "stops", sizeof("stops") - 1) == 0)
				{
					cmd[0] = 0x7e;
					cmd[1] = 0x71;
					cmd[2] = 0x7e;
					cmd_length = 3;
					printf("send command:stop sampling.\n");
				}
				else if (memcmp(cmd, "stopu", sizeof("stopu") - 1) == 0)
				{
					cmd[0] = 0x7e;
					cmd[1] = 0x77;
					cmd[2] = 0x7e;
					cmd_length = 4;
					printf("send command:stop upload.\n");
				}
				else if (memcmp(cmd, "stopt", sizeof("stopt") - 1) == 0)
				{
					cmd[0] = 0x7e;
					cmd[1] = 0x78;
					cmd[2] = 0x7e;
					cmd_length = 4;
					printf("send command:stop transmit.\n");
				}
				else if (memcmp(cmd, "gett", sizeof("gett") - 1) == 0)
				{
					pcmd = cmd;
					*pcmd++ = 0x7e;
					*pcmd++ = 0x74;
					*pcmd++ = 9;
					memcpy(pcmd, "main.ihex", 9);
					pcmd += 9;
					*pcmd++ = 0x7e;
					cmd_length = 13;
					printf("send command:get.\n");
				}
				else if (memcmp(cmd, "getm", sizeof("getm") - 1) == 0)
				{
					pcmd = cmd;
					*pcmd++ = 0x7e;
					*pcmd++ = 0x74;
					*pcmd++ = 9;
					memcpy(pcmd, "main.srec", 9);
					pcmd += 9;
					*pcmd++ = 0x7e;
					cmd_length = 13;
					printf("send command:get.\n");
				}
				else if (memcmp(cmd, "gete", sizeof("gete") - 1) == 0)
				{
					pcmd = cmd;
					*pcmd++ = 0x7e;
					*pcmd++ = 0x74;
					*pcmd++ = 8;
					memcpy(pcmd, "main.exe", 8);
					pcmd += 8;
					*pcmd++ = 0x7e;
					cmd_length = 12;
					printf("send command:get.\n");
				}
				else if (memcmp(cmd, "put", sizeof("put") - 1) == 0)
				{
					pcmd = cmd;
					*pcmd++ = 0x7e;
					*pcmd++ = 0x75;
					*pcmd++ = 9;
					memcpy(pcmd, "main.ihex", 9);
					pcmd += 9;
					*pcmd++ = 0x7e;
					cmd_length = 13;
					printf("send command:put.\n");
				}
				else if (memcmp(cmd, "uploadt", sizeof("uploadt") - 1) == 0)
				{
					pcmd = cmd;
					*pcmd++ = 0x7e;
					*pcmd++ = 0x72;
					*pcmd++ = 0x02;		// telosb
					*pcmd++ = 0x01;		// tos1
					*pcmd++ = 0x23;		// mid
					*pcmd++ = 0x00;		// mid
					*pcmd++ = 9;
					memcpy(pcmd, "main.ihex", 9);
					pcmd += 9;
					*pcmd++ = 0x7e;
					cmd_length = 18;
					printf("send command:upload.\n");
				}
				else if (memcmp(cmd, "uploadm", sizeof("uploadm") - 1) == 0)
				{
					pcmd = cmd;
					*pcmd++ = 0x7e;
					*pcmd++ = 0x72;
					*pcmd++ = 0x01;		// telosb
					*pcmd++ = 0x01;		// tos1
					*pcmd++ = 0x23;		// mid
					*pcmd++ = 0x00;		// mid
					*pcmd++ = 9;
					memcpy(pcmd, "main.srec", 9);
					pcmd += 9;
					*pcmd++ = 0x7e;
					cmd_length = 18;
					printf("send command:upload.\n");
				}
				else if (memcmp(cmd, "close", sizeof("close") - 1) == 0)
				{
					cmd[0] = 0x7e;
					cmd[1] = 0x73;
					cmd[2] = 0x7e;
					cmd_length = 3;
					printf("send command:close.\n");
				}
				else if (memcmp(cmd, "2state", sizeof("2state") - 1) == 0)
				{
					cmd[0] = 0x7e;
					cmd[1] = 0x76;
					cmd[2] = 0x7e;
					cmd[3] = 0x7e;
					cmd[4] = 0x76;
					cmd[5] = 0x7e;
					cmd_length = 6;
					printf("send command:close.\n");
				}
				else if (memcmp(cmd, "2ss", sizeof("2ss") - 1) == 0)
				{
					cmd[0] = 0x7e;
					cmd[1] = 0x70;
					cmd[2] = 0x7e;
					cmd[3] = 0x7e;
					cmd[4] = 0x71;
					cmd[5] = 0x7e;
					cmd_length = 6;
					printf("send command:close.\n");
				}
				else if (memcmp(cmd, "mote", sizeof("mote") - 1) == 0)
				{
					cmd[0] = 0x7e;
					cmd[1] = 0x80;
					cmd[2] = cmd[4];
					cmd[3] = 0x7e;
					cmd_length = 4;
					printf("send command:mote open.\n");
				}
				else
				{
					cmd[0] = 0x7e;
					cmd[1] = 0x22;
					cmd[2] = 0x7e;
					cmd_length = 3;
					printf("send command:invalid.\n");
				}
				
				// send command to all clients
				for (i = 0; i < conn_fd_cnt; ++i)
				{
					send(conn_fd[i], cmd, cmd_length, 0);
				}
				
			}//end if (FD_ISSET(STDIN, &fs_read))
			
		}//end if (fs_sel)	
		else if (fs_sel == 0)
		{
			printf("select timeout.\n");
		}
		else
		{
			printf("select failed.\n");
			return -1;
		}
		
	}//end for
	
	return 0;
}

