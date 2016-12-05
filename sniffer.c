
#include <sys/time.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>

#define	_USE_DEBUG_
#include "debug.h"
#include "socket.h"
#include "hdlc.h"
#include "packet.h"
#include "misc.h"


#define	VERSION		"sniffer-0.1.4"

#define	HELP_INFO()		printf("Usage: sniffer [OPTION]... \n\n");		\
				printf("\t-U [U]pd_dir\n\t\tDirectory path for upload files, default is /opt/sniffer/upd/ .\n\n");		\
				printf("\t-B [B]in_dir\n\t\tDirectory path for .sh and exe files, default is /opt/sniffer/bin/ .\n\n");		\
				printf("\t-i server_[i]p\n\t\tServer IP address of data server. Format xxx.xxx.xxx.xxx is valid, default is 192.168.14.252 .\n\n");		\
				printf("\t-p server_[p]ort_sniffer\n\t\tServer listen port for sniffer on data server, like 10000, default is 5000.\n\n");		\
				printf("\t-q server_[p]ort_mote\n\t\tServer listen port for mote on data server, like 10000, default is 5001.\n\n");		\
				printf("\t-P [P]oll_interval\n\t\tThe time interval of polling USB device port, default is 3 seconds.\n\n");		\
				printf("\t-n sniffer_[n]umber\n\t\tUser must set the sniffer number, default is 0.\n\n");		\
				printf("\t-v\n\t\tShow version.\n\n");		\
				printf("\t-h\n\t\tShow help.\n\n");			\
				
				


typedef struct _SNIFFER
{
	char			*num;
	char			*upd_dir;
	char			*bin_dir;
	int				poll_interval;
	char			*server_ip;
	char			*server_port_sniffer;
	char			*server_port_mote;
	
}SNIFFER;





int
sniffer_open_mote(
	SNIFFER		*sniffer,
	int				dev_num
	)
{
	FILE		*mote_map;
	int			temp_num = dev_num, cnt;
	char		line[64];
	char		num[4];
	char		dev_type[8];
	char		msg_port[16];
	char		upd_port[16];
	char		*psrc, *pdst;
	int			pid;
	char		file[64];
	
	DBG(("mote open... %d\n", dev_num));
	
	/* set dev_num */
	cnt = 0;
	while(temp_num)	// get mid bit counter in dec
	{
		temp_num /= 10;
		cnt++;
	}
	num[cnt] = '\0';
	do
	{
		cnt--;
		num[cnt] = (dev_num % 10) + '0';
		dev_num /= 10;
	}
	while(cnt);
	DBG(("dev num = %s\n", num));

	str_combine(file, sniffer->bin_dir, 0, "mote-map", 0);

	mote_map = fopen(file, "r");
	if (mote_map == NULL)
		return -1;
		
	while(fgets(line, 64, mote_map) != NULL)
	{
		if (strncmp(line, num, strlen(num)) != 0)
			continue;
		
		DBG(("map = %s\n", line));
		// fork child process to open the mote
		psrc = line;
		while(*psrc++ != ':');
		
		pdst = dev_type;
		do
		{
			*pdst++ = *psrc++;
		}
		while(*psrc != ':');
		*pdst = '\0';
		
		psrc++;
		msg_port[0] = '\0';
		strcat(msg_port, "/dev/");
		pdst = &msg_port[sizeof("/dev/") - 1];
		do
		{
			*pdst++ = *psrc++;
		}
		while(*psrc != ':');
		*pdst = '\0';
		
		psrc++;
		upd_port[0] = '\0';
		strcat(upd_port, "/dev/");
		pdst = &upd_port[sizeof("/dev/") - 1];
		do
		{
			*pdst++ = *psrc++;
		}
		while(*psrc != '\n');
		*pdst = '\0';
	
		DBG(("%s %s %s\n", dev_type, msg_port, upd_port));
		
		pid = fork();
		if (pid < 0)
			return -1;
		else if (pid == 0)
		{
			execlp("/opt/sniffer/src/mote",
							"mote",
							"-m", msg_port,
							"-u", upd_port,
							"-b", "57600",
							"-U", sniffer->upd_dir,
							"-B", sniffer->bin_dir,
							"-t", dev_type,
							"-i", sniffer->server_ip,
							"-p", sniffer->server_port_mote,
							"-V", "tos1",
							"-N", num,
							"-n", sniffer->num,
							(char *)0);
			DBG(("open mote child process failed.\n"));
		}
		else
		{
			fclose(mote_map);
			DBG(("mote process id = %d\n", pid));
			return pid;
		}
	}// end while
	
	fclose(mote_map);
	DBG(("ERROR:no device in the mote-map.\n"));
	
	return -1;
}



int
sniffer_ev_mote_map(
	SNIFFER		*sniffer,
	int		sock_fd
	)
{
	FILE		*mote_map;
	char		file[64];
	char		num[4];
	char		dev_type[8];
	char		msg_port[16];
	char		upd_port[16];
	char		line[64];
	char		*psrc, *pdst;
	int			cnt = 0;
	
	PACKET	packet;
	
	DBG(("send mote map.\n"));
	
	str_combine(file, sniffer->bin_dir, 0, "mote-map", 0);

	mote_map = fopen(file, "r");
	if (mote_map == NULL)
	{
		DBG(("file open failed.\n"));
		return -1;
	}
	
	packet.type = EV_UPD_MOTE_MAP;
	packet.content.ev_mote_map.map_length = 0;
	
	while(fgets(line, 64, mote_map) != NULL)
	{
		DBG(("%s", line));
		
		psrc = line;
		
		// get usb device number
		pdst = num;
		do
		{
			*pdst++ = *psrc++;
		}
		while(*psrc != ':');
		*pdst = '\0';
		psrc++;
		
		// get usb device type
		pdst = dev_type;
		do
		{
			*pdst++ = *psrc++;
		}
		while(*psrc != ':');
		*pdst = '\0';
		psrc++;
		
		// get message port
		pdst = msg_port;
		do
		{
			if (*psrc >= '0' && *psrc <= '9')
				*pdst++ = *psrc;
			psrc++;
		}
		while(*psrc != ':');
		*pdst = '\0';
		psrc++;
		
		// get upload port
		pdst = upd_port;
		do
		{
			if (*psrc >= '0' && *psrc <= '9')
				*pdst++ = *psrc;
			psrc++;
		}
		while(*psrc != '\n');
		*pdst = '\0';
		
		DBG(("%s %s %s %s\n", num, dev_type, msg_port, upd_port));
		
		if (strcmp(dev_type, "mib520") == 0)
		{
			packet.content.ev_mote_map.mote_des[cnt].usb_dev = 1;
		}
		else if (strcmp(dev_type, "telosb") == 0)
		{
			packet.content.ev_mote_map.mote_des[cnt].usb_dev = 2;
		}
		else
		{
			continue;
		}
		
		packet.content.ev_mote_map.mote_des[cnt].usb_num = (__uint8_t)atoi(num);
		packet.content.ev_mote_map.mote_des[cnt].msg_port = (__uint8_t)atoi(msg_port);
		packet.content.ev_mote_map.mote_des[cnt].upd_port = (__uint8_t)atoi(upd_port);
		packet.content.ev_mote_map.map_length += 4;
		cnt++;
	}//end while
	
	fclose(mote_map);
	
	packet.send_fd = sock_fd;
	packet.send = socket_send;
	
	return packet_send(&packet);
}



int
sniffer_ev_connect(
	SNIFFER		*sniffer,
	int				sock_fd
	)
{
	PACKET			packet;
	
	packet.type = EV_SNIFFER_CONN;
	packet.content.ev_sniffer_conn.sniffer_num = (__uint8_t)atoi(sniffer->num);
	packet.send_fd = sock_fd;
	packet.send = socket_send;
	
	return packet_send(&packet);
}


int
sniffer_cc(
	int						sock_fd,
	__uint8_t			cc
	)
{
	PACKET			packet;
	
	packet.type = COMPLETE_CODE;
	packet.content.cc = cc;
	packet.send_fd = sock_fd;
	packet.send = socket_send;
	
	return packet_send(&packet);
}


int
main(
	int 	argc, 
	char 	**argv
	)
{
	// socket
	int		sock_fd;
	
	// select control
	int				fs_sel;
	fd_set		fs_read;
	struct timeval to;
	
	// command receive
	PACKET		packet;

	// device polling
	char			is_wait_polling = 0;
	SNIFFER		sniffer;
	char			file[64];
	char			map[64];
	int				pid = 0;
	int				child_state;
	time_t tc, told = 0;
	char			is_system_on = 1;

	// command line argument
	int				ch;
	
	// set default value
	sniffer.num = "0";
	sniffer.upd_dir = "/opt/sniffer/upd/";
	sniffer.bin_dir = "/opt/sniffer/bin/";
	sniffer.poll_interval = 3;
	sniffer.server_ip = "192.168.14.252";
	sniffer.server_port_sniffer = "5000";
	sniffer.server_port_mote = "5001";
	
	
	// get command line option
	while ((ch = getopt(argc, argv, ":n:U:B:i:p:q:P:hv")) != -1)
	{
		switch(ch)
		{
			case 'n':		//sniffer_[n]umber
				sniffer.num = optarg;
				break;
			case 'U':		//[U]pd_dir
				sniffer.upd_dir = optarg;
				break;
			case 'B':		//[B]in_dir
				sniffer.bin_dir = optarg;
				break;
			case 'i':		//server_[i]p
				sniffer.server_ip = optarg;
				break;
			case 'p':		//server_[p]ort for sniffer
				sniffer.server_port_sniffer = optarg;
				break;
			case 'q':		//server_port for mote
				sniffer.server_port_mote = optarg;
				break;
			case 'P':		//[P]oll_time_interval
				sniffer.poll_interval = atoi(optarg);
				break;
			case 'v':		//[v]ersion
				printf("Version:%s\n", VERSION);
				exit(0);
			case '?':
				DBG(("WARNING:unknow option: \"-%c\"\n", optopt));
				HELP_INFO();
				exit(0);
			case ':':
				DBG(("WARNING:option \"-%c\" need argument.\n", optopt));
				HELP_INFO();
				exit(0);
			case 'h':		//[h]elp
			default:
				HELP_INFO();
				exit(0);
		}// end switch
	}// end while
	
	// connect to server
	// socket open
	if ((sock_fd = socket_open()) == -1)
		return -1;
		
	// socket connect to server
	if (socket_connect_dst(sock_fd, sniffer.server_ip, atoi(sniffer.server_port_sniffer)) != 0)
		return -1;
		
	if (sniffer_ev_connect(&sniffer, sock_fd) == -1)
		return -1;
	
	packet.recv_fd = sock_fd;
	packet.recv = socket_recv;
	
	str_combine(file, sniffer.bin_dir, 0, "mote-map.sh", 0);
	str_combine(map, sniffer.bin_dir, 0, "mote-map", 0);
			
	// main loop
	while(1)
	{
		FD_ZERO(&fs_read);
		FD_SET(sock_fd, &fs_read);
		to.tv_sec = 1;
		to.tv_usec = 0;
		
		// receive server command
		fs_sel = select(sock_fd + 1, &fs_read, NULL, NULL, &to);	
		if (fs_sel)
		{
			// socket received data
			if (FD_ISSET(sock_fd, &fs_read))
			{
				if (packet_recv(&packet) > 0)
				{
					switch (packet.type)//check packet type
					{
						case CMD_OPEN_MOTE:
							if (sniffer_open_mote(&sniffer, packet.content.cmd_open_mote.mote_num) == -1)
							{
								DBG(("ERROR:Mote open failed.\n"));
								if (sniffer_cc(sock_fd, CC_FAIL) == -1)
									return -1; 
							}
							else
							{
								if (sniffer_cc(sock_fd, CC_SUCCESS) == -1)
									return -1; 
							}
							break;
							
						default:
							DBG(("sniffer ignore other packets.\n"));
							break;
	
					}//end switch packet type
				}//end if (packet_recv(packet) > 0)
				
			}//end select socket
		}
		else if (fs_sel == 0)
		{
//			DBG(("WARNING: select() timeout...\n"));
		}
		else
		{
			DBG(("ERROR: select() return %d\n", fs_sel));
			return -1;
		}
		
		// polling usb mote connection state each 5 seconds
		tc = time(NULL);
//		DBG(("tc = %d\n", tc));
		if (tc - told > sniffer.poll_interval || is_system_on == 1)
		{
			told = tc;
			DBG(("start polling device.\n"));
		
			// fork child process mote-map.sh to update mote-map file
			pid = fork();
			if (pid < 0)
				_exit(1);
			else if (pid == 0)		// child process update mote-map
			{
				execlp("/bin/bash",
								"sh",
								file,
								map,
								(char *)0);
				_exit(1);
			}
			else
			{
				is_wait_polling = 1;
			}
			
			if (is_system_on == 1)
				is_system_on = 2;
		}// end if (tc - told > poll_interval)
		
		if (is_wait_polling && waitpid(pid, &child_state, WNOHANG) == pid)
		{
			is_wait_polling = 0;
			DBG(("child process state = %d\n", child_state));
			
			// if mote-map changed, send new mote-map to data server
			if (child_state != 0 || is_system_on == 2)		// device changed
			{
				is_system_on = 0;
					
				if (sniffer_ev_mote_map(&sniffer, sock_fd) == -1)
				{
					DBG(("ERROR:send mote map failed.\n"));
					_exit(1);
				}
			}
		}

	}// end main loop

	return 0;
}



