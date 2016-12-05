


#include <string.h>
#include <stdlib.h>
#include <sys/times.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/wait.h>

#define	_USE_DEBUG_
#include "debug.h"
#include "socket.h"
#include "com_port.h"
#include "hdlc.h"
#include "packet.h"
#include "misc.h"


#define	VERSION		"mote-0.1.4"

#define	HELP_INFO()		printf("Usage: mote [OPTION]... \n\n");		\
				printf("\t-m [m]sg_port\n\t\tSerial port file for message sampling. /dev/ttyUSB* are valid, default is /dev/ttyUSB0.\n\n");		\
				printf("\t-b msg_port_[b]aud\n\t\tSerial baud for message sampling, 2400,4800,9600,38400,56700,115200 are valid, default is 57600.\n\n");		\
				printf("\t-u [u]pd_port\n\t\tSerial port file for uploading mote. /dev/ttyUSB* are valid, default is /dev/ttyUSB0.\n\n");		\
				printf("\t-U [U]pd_dir\n\t\tDirectory path for upload files, default is /opt/sniffer/upd/ .\n\n");		\
				printf("\t-B [B]in_dir\n\t\tDirectory path for .sh and exe files, default is /opt/sniffer/bin/ .\n\n");		\
				printf("\t-i server_[i]p\n\t\tServer IP address of data server. Format xxx.xxx.xxx.xxx is valid, default is 192.168.14.252 .\n\n");		\
				printf("\t-p server_[p]ort\n\t\tServer listen port of data server, like 10000, default is 5001.\n\n");		\
				printf("\t-t mote_[t]ype\n\t\tMote type option, micaz and telosb are valid, default is telosb.\n\n");		\
				printf("\t-V tinyos_[V]ersion\n\t\tMote program of tinyos verison, tos1(tinyos-1.x) and tos2(tinyos-2.x) are valid, default is tos1.\n\n");		\
				printf("\t-n sniffer_[n]umber\n\t\tThe sniffer number of this mote.\n\n");		\
				printf("\t-N usb_[N]umber\n\t\tThe usb device number of this mote in the mote-map.\n\n");		\
				printf("\t-v\n\t\tShow version.\n\n");		\
				printf("\t-h\n\t\tShow help.\n\n");			\
				printf("Notes: Mostly, it is used by sniffer process. It also can be used independently. Before use it, make sure the listen port of the server is open. \n\n");		\
				
				

// define mote control struct
typedef struct _MOTE
{
	// mote attribute
	char					*sniffer_num;
	char					*usb_num;
	char					*msg_port;
	int						msg_port_baud;
	char					*upd_port;
	char					*upd_dir;
	char					*bin_dir;
	char					*server_ip;
	int						server_port;
	char					*mote_id;
	char					*group_id;
	char					*mote_type;
	char					*tos_version;
	
	// socket attribute
	int						socket_fd;
	HDLC					socket_hdlc;
	
	// sampling config
	int						com_fd;
	HDLC					com_hdlc;
	time_t				start_sampling_time_sec;
	suseconds_t		start_sampling_time_usec;
	char					is_start_sampling;
	
	// clear buffer
	char					is_clear_buffer;
	time_t				start_clear_time_sec;
	suseconds_t		start_clear_time_usec;
	
	// logic control
	char					state;	
	int						child_pid;	// child process id, include upload and ftp
	
}MOTE;



// define	mote state
#define	ST_MOTE_IDLE						0
#define	ST_MOTE_SAMPLING				1
#define	ST_MOTE_UPLOADING				2
#define	ST_MOTE_FILE_TRANSFER		3


#define	TIMEOUT_SEC(len,baud)	(len * 20 / baud + 2)
#define	TIMEOUT_USEC	0

#define	MAX(X,Y)	(((X) > (Y)) ? (X) : (Y))


#define	TFTP_GET		1
#define	TFTP_PUT		2
/*
#define TOSH_DATA_LENGTH 28

typedef struct _SERIAL_MSG
{
	struct
	{
	  __uint8_t		msg;
	  __uint8_t 	length;
	  __uint8_t 	fcfhi;
	  __uint8_t 	fcflo;
	  __uint8_t 	dsn;
	  __uint16_t 	destpan;
	  __uint16_t 	addr;
	  __uint8_t 	type;
	  __uint8_t 	group;
	  __uint8_t 	data[TOSH_DATA_LENGTH];
	}content;
	
  __uint8_t		msg_length;
  
}SERIAL_MSG;
//*/

int 
mote_open(
	MOTE					*mote,
	int						argc,
	char					**argv
	)
{
	int				ch;
	
	// set mote default parameter
	mote->msg_port = "/dev/ttyUSB0";
	mote->msg_port_baud = 57600;
	mote->upd_port = "/dev/ttyUSB0";
	mote->upd_dir = "/opt/sniffer/upd/";
	mote->bin_dir = "/opt/sniffer/bin/";
	mote->server_ip = "192.168.14.252";
	mote->server_port = 5001;
	mote->mote_id = "0";
	mote->group_id = "125";
	mote->mote_type = "telosb";
	mote->tos_version = "tos1";
	mote->sniffer_num = "0";
	mote->usb_num = "2";

	// get command line option
	while ((ch = getopt(argc, argv, ":m:b:u:U:B:i:p:t:V:n:N:hv")) != -1)
	{
		switch(ch)
		{
			case 'n':		//sniffer_[n]umber
				mote->sniffer_num = optarg;
				break;
			case 'N':		//usb_[N]umber
				mote->usb_num = optarg;
				break;
			case 'm':		//[m]sg_port
				mote->msg_port = optarg;
				break;
			case 'b':		//[b]aud
				mote->msg_port_baud = atoi(optarg);
				break;
			case 'u':		//[u]pd_port
				mote->upd_port = optarg;
				break;
			case 'U':		//[U]pd_dir
				mote->upd_dir = optarg;
				break;
			case 'B':		//[B]in_dir
				mote->bin_dir = optarg;
				break;
			case 'i':		//server_[i]p
				mote->server_ip = optarg;
				break;
			case 'p':		//server_[p]ort
				mote->server_port = atoi(optarg);
				break;
			case 't':		//mote_[t]ype
				mote->mote_type = optarg;
				break;
			case 'V':		//tos_[V]ersion
				mote->tos_version = optarg;
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
				HELP_INFO()
				exit(0);
			case 'h':		//[h]elp
			default:
				HELP_INFO();
				exit(0);
		}// end switch
	}// end while

	DBG(("msg_port=%s\n", mote->msg_port));
	DBG(("msg_port_baud=%d\n", mote->msg_port_baud));
	DBG(("upd_port=%s\n", mote->upd_port));
	DBG(("upd_dir=%s\n", mote->upd_dir));
	DBG(("bin_dir=%s\n", mote->bin_dir));
	DBG(("server_ip=%s\n", mote->server_ip));
	DBG(("server_port=%d\n", mote->server_port));
	DBG(("mote_id=%s\n", mote->mote_id));
	DBG(("group_id=%s\n", mote->group_id));
//	DBG(("mote_type=%s\n", mote->mote_type));
//	DBG(("tos_version=%s\n", mote->tos_version));
	
	// init mote attribute
	mote->socket_fd = -1;
	mote->com_fd = -1;
	mote->state = ST_MOTE_IDLE;
	mote->start_sampling_time_sec = 0;
	mote->start_sampling_time_usec = 0;
	mote->is_start_sampling = 1;
	mote->is_clear_buffer = 1;
	mote->child_pid = -1;
	
	return 0;
}


int
mote_connect(
	MOTE				*mote
	)
{
	// socket open
	if ((mote->socket_fd = socket_open()) == -1)
		return -1;
		
	// socket connect to server
	if (socket_connect_dst(mote->socket_fd, mote->server_ip, mote->server_port) != 0)
		return -1;
		
	return 0;
}
	

int 
mote_process_sampling(
	MOTE					*mote,
	unsigned char	*buffer,
	int						length
	)
{
	int						len;
	int						i;
	unsigned char	ch;
	
	struct timeval tv;
	struct timezone tz;
	time_t				curr_time_sec;
	suseconds_t		curr_time_usec;
	
	PACKET				packet;
	
	// read one byte from serial port
	len = com_read(mote->com_fd, &ch, 1);
	if (len != 1)
		return len;
	
	// clear buffer
	if (mote->is_clear_buffer)
	{
		gettimeofday(&tv, &tz);
		DBG(("%.2x ", ch));
		if (tv.tv_sec > mote->start_clear_time_sec
				/*&& tv.tv_usec > mote->start_clear_time_usec*/)
		{
			mote->is_clear_buffer = 0;
			DBG(("end clear time:\ntv_sec = %d\n", (int)tv.tv_sec));
			DBG(("tv_usec = %d\n",(int)tv.tv_usec));
		}
		else
		{
			return 0;
		}
	}//end if (mote->is_clear_buffer)
		
	len = hdlc_recv_char(&mote->com_hdlc, ch);
	if (len < 0)
		return -1;
	else if (len == 0)
		return 0;
		
	if (len > 7)		// at least, serial message length is 7 bytes
	{
		// get current time
		gettimeofday(&tv, &tz);
//	DBG(("\ntv_sec = %d\n", tv.tv_sec));
//	DBG(("tv_usec = %d\n",tv.tv_usec));
		curr_time_sec = tv.tv_sec;
		curr_time_usec = tv.tv_usec;
		
		if (mote->is_start_sampling)
		{
			mote->is_start_sampling = 0;
			
			mote->start_sampling_time_sec = curr_time_sec;
			mote->start_sampling_time_usec = curr_time_usec;
		}

		if (curr_time_usec < mote->start_sampling_time_usec)
		{
			curr_time_sec--;
			curr_time_usec += 1000000;
		}

		for (i = 0; i < len; ++i)
			DBG(("%.2x ", buffer[i]));
		DBG(("\n"));
		
		// make data packet
		packet.type = DATA_SERIAL_MSG;
		packet.content.data_msg.length = (__uint8_t)(((void *)packet.content.data_msg.msg - (void *)&packet.content.data_msg.ref_time_sec) + len - 7); 
		packet.content.data_msg.ref_time_sec = (__uint32_t)(curr_time_sec - mote->start_sampling_time_sec);
		packet.content.data_msg.ref_time_msec = (__uint16_t)((curr_time_usec - mote->start_sampling_time_usec) / 1000);
		packet.content.data_msg.mid = buffer[len - 6];
		packet.content.data_msg.mid <<= 8;
		packet.content.data_msg.mid += buffer[len - 7];
		packet.content.data_msg.gid = buffer[len - 5];
		packet.content.data_msg.io = (buffer[len - 4] == 0xff) ? 1 : 0;	// 1:in, 0:out
		packet.content.data_msg.result = buffer[len - 4];	// in:0xff out:!= 0xff
		packet.content.data_msg.queue_cnt = buffer[len - 3];		// queue used counter
		buffer[1] -= 5;		// get radio message packet length
		len -= 7;					// cut last information data
		memcpy(packet.content.data_msg.msg, buffer, len);
		packet.send_fd = mote->socket_fd;
		packet.send = socket_send;
		
		if (packet_send(&packet) == -1)
		{
			DBG(("\nERROR: socket send failed...\n"));
			return -1;
		}
	}//end if (len > 7)

	return 0;
}
	
	
int
mote_cmd_start_sampling(
	MOTE				*mote
	)
{
	int					fd;
	struct timeval tv;
	struct timezone tz;

	DBG(("Start sampling..."));

	// open serial
//	if (strcmp(mote->mote_type, "telosb") == 0)
//	{
		// open message and upload port
		fd = com_open(mote->msg_port);
		if (fd < 0)
			return -1;
			
		if (com_set(fd, mote->msg_port_baud, 8, 1, 'n', 'n') != 0)
		{
			return -1;
		}
		mote->com_fd = fd;
//	}
//	else
//	{
//		printf("WARNING:Unsupport mote.\n");
//		return -1;
//	}

	// start sampling
	mote->state = ST_MOTE_SAMPLING;
	mote->is_start_sampling = 1;
	mote->is_clear_buffer = 1;
	
	gettimeofday(&tv, &tz);
	mote->start_clear_time_sec = tv.tv_sec;
//	mote->start_clear_time_usec = tv.tv_usec;
	DBG(("start clear time:\ntv_sec = %d\n", (int)tv.tv_sec));
	DBG(("tv_usec = %d\n",(int)tv.tv_usec));

	return 0;
}



int
mote_cmd_stop_sampling(
	MOTE				*mote
	)
{
	DBG(("Stop sampling.\n"));
	
	// close serial
	com_close(mote->com_fd);
	mote->com_fd = -1;

	// stop sampling
	mote->state = ST_MOTE_IDLE;
	
	return 0;
}


int
mote_cmd_tftp(
	MOTE					*mote,
	int						cmd,
	char					*filename,
	unsigned char	length
	)
{
	int			i;
	int			pid;
	char		file[32];
	int			ret;
	char		ftp[64];
	char		*ftp_cmd;
	
	DBG(("Put file to data server...\n"));
	
	DBG(("filename = "));
	for (i = 0; i < length; ++i)
		DBG(("%c", filename[i]));
	DBG(("\n"));
	
	/* set file name */
	str_combine(file, filename, length, "", 0);
	DBG(("Put file %s\n", file));
//	if (access(file, F_OK) != 0)
//	{
//		DBG(("ERROR:File \"%s\" is not existence.\n", file));
//		return 1;
//	}
	
	str_combine(ftp, mote->bin_dir, 0, "tftp.sh", 0);
	
	if (cmd == TFTP_GET)
		ftp_cmd = "get";
	else if (cmd == TFTP_PUT)
		ftp_cmd = "put";
	else
	{
		DBG(("ERROR:invalid tftp command.\n"));
		return -1;
	}
	
	// fork put file process, call tftp to get file from data server
	pid = fork();
	if (pid == 0)	// put child process
	{
		ret = execlp("/bin/bash",
									"sh",
									ftp,
									mote->server_ip,
									mote->upd_dir,		
									ftp_cmd,				
									file,	
									(char *)0);
		
		if (ret == -1)
		{
			DBG(("execlp failed.\n"));									
			_exit(1);
		}	
	}// end if sampling_pid
	else if (pid > 0)
	{
		DBG(("put file process id = %d.\n", pid));
		mote->child_pid = pid;
	}
	else
	{
		DBG(("fork put file process failed.\n"));
		return -1;
	}
	
	// update state
	mote->state = ST_MOTE_FILE_TRANSFER;
	
	return 0;
}	


int
mote_cmd_upload_mote(
	MOTE						*mote,
	unsigned char		type,
	unsigned char		tos,
	unsigned short	mid,
	char						*filename,
	unsigned char		length
	)
{
	int			i;
	int			pid;
	
	char		pathname[256];
	char		*mote_type;
	char		*tos_v;
	char		mote_id[8];
	int			mid_temp = mid;
	int			cnt;
	int			ret;
	char		upd[64];

	DBG(("Upload mote programme...\n"));
	
	DBG(("tos = %d, mid = %d, filename = ", tos, mid));
	for (i = 0; i < length; ++i)
		DBG(("%c", filename[i]));
	DBG(("\n"));
	
	/* set mote type */
	switch(type)
	{
		case 1:
			mote_type = "micaz";
			break;
		case 2:
			mote_type = "telosb";
			break;
		default:
			return -1;
	}
	
	DBG(("type = %s\n", mote_type));
	
	/* set tos version */
	switch(tos)
	{
		case 1:
			tos_v = "tos1";
			break;
		case 2:
			tos_v = "tos2";
			break;
		default:
			return -1;
	}
	DBG(("tos = %s\n", tos_v));
	
	/* set mid */
	cnt = 0;
	while(mid_temp)	// get mid bit counter in dec
	{
		mid_temp /= 10;
		cnt++;
	}
	mote_id[cnt] = '\0';
	do
	{
		cnt--;
		mote_id[cnt] = (mid % 10) + '0';
		mid /= 10;
	}
	while(cnt);
	DBG(("mid = %s\n", mote_id));
	
	/* mote upload port */
	DBG(("port = %s\n", mote->upd_port));
	
	/* set path name */
	str_combine(pathname, mote->upd_dir, 0, filename, length);
	
	DBG(("pathname = %s\n", pathname));
	if (access(pathname, F_OK) != 0)
	{
		DBG(("ERROR:File \"%s\" is not existence.\n", pathname));
		return 1;
	}
	
	/* set upload.sh path */
	str_combine(upd, mote->bin_dir, 0, "upload.sh", 0);
	
	// fork put file process, call usip or tos-bsl to upload programme
	pid = fork();
	if (pid == 0)	// upload child process
	{
		DBG(("upload child process.\n"));
		
		ret = execlp("/bin/bash",
										"sh",
										upd,
										mote_type,		
										mote_id,				
										mote->upd_port,	
										pathname,	
										tos_v,	
										(char *)0);

		if (ret == -1)
		{
			DBG(("execlp failed.\n"));									
			_exit(1);
		}											
	}// end if sampling_pid
	else if (pid > 0)
	{
		DBG(("upload process id = %d.\n", pid));
		mote->child_pid = pid;
	}
	else
	{
		DBG(("fork upload process failed.\n"));
		return -1;
	}
	
	// update state
	mote->state = ST_MOTE_UPLOADING;

	return 0;
}


int
mote_cmd_stop_child(
	MOTE				*mote
	)
{
	DBG(("stop upload mote.\n"));

	// kill upload process
	if (mote->child_pid != -1 
			&& kill(mote->child_pid, SIGKILL) == -1)	
	{
		DBG(("kill upload process failed.\n"));	
		return -1;
	}
	mote->child_pid = -1;
	
	// update state
	mote->state = ST_MOTE_IDLE;
	
	return 0;
}


int
mote_cmd_close_mote(
	MOTE				*mote
	)
{
	DBG(("Close mote.\n"));
	
	// close this process
	
	return 0;
}


int
mote_ev_connect(
	MOTE				*mote
	)
{
	PACKET			packet;
	
	packet.type = EV_MOTE_CONN;
	packet.content.ev_mote_conn.sniffer_num = (__uint8_t)atoi(mote->sniffer_num);
	packet.content.ev_mote_conn.mote_num = (__uint8_t)atoi(mote->usb_num);
	packet.send_fd = mote->socket_fd;
	packet.send = socket_send;
	
	return packet_send(&packet);
}


int 
mote_cc(
	MOTE					*mote,
	__uint8_t			cc
	)
{
	PACKET			packet;
	
	packet.type = COMPLETE_CODE;
	packet.content.cc = cc;
	packet.send_fd = mote->socket_fd;
	packet.send = socket_send;
	
	return packet_send(&packet);
}


int
mote_state(
	MOTE					*mote
	)
{
	PACKET			packet;
	
	packet.type = STATE_CODE;
	packet.content.state = mote->state;
	packet.send_fd = mote->socket_fd;
	packet.send = socket_send;
	
	return packet_send(&packet);
}


int 
mote_process(
	MOTE			*mote
	)
{
	// select control
	int				fs_sel;
	fd_set		fs_read;
	int				max_fd;
	struct timeval to;
	
	// command control
	int						cmd_ret;
	unsigned char	complete;
	unsigned char	msg_buffer[64];
	PACKET				packet;
	
	int						state;
	
	if (mote == NULL)
	{
		DBG(("ERROR:Invalid pointer.\n"));
		return -1;
	}
	
	if (mote_ev_connect(mote) == -1)
		return -1;
	

	// init serial message receive buffer
	hdlc_init_recv(&mote->com_hdlc, msg_buffer, 64);

	packet.recv_fd = mote->socket_fd;
	packet.recv = socket_recv;
	
	DBG(("start receive command...\n"));	
		
	while(1)
	{
		// config select system call
		FD_ZERO(&fs_read);
		if (mote->socket_fd > 0)
			FD_SET(mote->socket_fd, &fs_read);
		if (mote->com_fd > 0)
			FD_SET(mote->com_fd, &fs_read);
		max_fd = MAX(mote->socket_fd, mote->com_fd);
		to.tv_sec = 1;
		to.tv_usec = 0;

//		DBG(("start select...\n"));
		// select socket and serial input without timeout.
		fs_sel = select(max_fd + 1, &fs_read, NULL, NULL, &to);	
		if (fs_sel)
		{
			// socket received data
			if (FD_ISSET(mote->socket_fd, &fs_read))
			{
				if (packet_recv(&packet) > 0)
				{
					// process command
					switch (packet.type)//check packet type
					{
						case CMD_GET_STATE:
							if (mote_state(mote) == -1)
								return -1;
							break;
							
						case CMD_START_SAMPLING:
							if (mote->state == ST_MOTE_IDLE)
							{
								cmd_ret = mote_cmd_start_sampling(mote);
								complete = (cmd_ret == 0) ? CC_SUCCESS : CC_FAIL;
								if (mote_cc(mote, complete) == -1)
									return -1; 				
							}// end if ST_MOTE_IDLE
							else
							{
								if (mote_cc(mote, CC_SKIP) == -1)
									return -1;
							}
							break;
							
						case CMD_STOP_SAMPLING:
							if (mote->state == ST_MOTE_SAMPLING)
							{							
								cmd_ret = mote_cmd_stop_sampling(mote);
								complete = (cmd_ret == 0) ? CC_SUCCESS : CC_FAIL;
								if (mote_cc(mote, complete) == -1)
									return -1; 
							}
							else
							{
								if (mote_cc(mote, CC_SKIP) == -1)
									return -1;
							}
							break;
							
						case CMD_GET_FILE:
							if (mote->state == ST_MOTE_IDLE)
							{
								cmd_ret = mote_cmd_tftp(mote, TFTP_GET, 
																			packet.content.cmd_file_transfer.file_name, 
																			packet.content.cmd_file_transfer.file_name_length);
								if (cmd_ret != 0)
								{
									if (cmd_ret == 1)
										complete = CC_ARGUMENT;	// command argument invalid
									else
										complete = CC_FAIL;		// operation failed
									if (mote_cc(mote, complete) == -1)
										return -1; 
								} 
							}
							else
							{
								if (mote_cc(mote, CC_SKIP) == -1)
									return -1;
							}
							break;
							
						case CMD_PUT_FILE:
							if (mote->state == ST_MOTE_IDLE)
							{
								cmd_ret = mote_cmd_tftp(mote, TFTP_PUT, 
																			packet.content.cmd_file_transfer.file_name, 
																			packet.content.cmd_file_transfer.file_name_length);
								if (cmd_ret != 0)
								{
									if (cmd_ret == 1)
										complete = CC_ARGUMENT;	// command argument invalid
									else
										complete = CC_FAIL;		// operation failed
									if (mote_cc(mote, complete) == -1)
										return -1; 
								}
							}
							else
							{
								if (mote_cc(mote, CC_SKIP) == -1)
									return -1;
							}
							break;
							
						case CMD_UPLOAD_MOTE:
							if (mote->state == ST_MOTE_IDLE)
							{
								cmd_ret = mote_cmd_upload_mote(mote, 
																							packet.content.cmd_upload_mote.mote_type, 
																							packet.content.cmd_upload_mote.tos_version, 
																							packet.content.cmd_upload_mote.mid, 
																							packet.content.cmd_upload_mote.file_name, 
																							packet.content.cmd_upload_mote.file_name_length);
								if (cmd_ret != 0)
								{
									if (cmd_ret == 1)
										complete = CC_ARGUMENT;	// command argument invalid
									else
										complete = CC_FAIL;		// operation failed
									if (mote_cc(mote, complete) == -1)
										return -1; 
								}
							}
							else
							{
								if (mote_cc(mote, CC_SKIP) == -1)
									return -1;
							}
							break;
							
						case CMD_CLOSE_MOTE:
							cmd_ret = mote_cmd_close_mote(mote);
							complete = (cmd_ret == 0) ? CC_SUCCESS : CC_FAIL;
								if (mote_cc(mote, complete) == -1)
									return -1; 
							return 0;
						
						case CMD_STOP_UPLOAD:
						case CMD_STOP_TRANSMIT:
							if (mote->state == ST_MOTE_UPLOADING)
							{
								cmd_ret = mote_cmd_stop_child(mote);
								complete = (cmd_ret == 0) ? CC_SUCCESS : CC_FAIL;
								if (mote_cc(mote, complete) == -1)
									return -1;
							}
							else
							{
								if (mote_cc(mote, CC_SKIP) == -1)
									return -1;
							}
							break;
						
						default:
							DBG(("WARNING:ignore other packets.\n"));
							break;
					}//end switch packet type
				}//end if (packet_recv(&packet) > 0)
			}//end select socket
			
			// serial received data
			if (mote->state == ST_MOTE_SAMPLING
					&& FD_ISSET(mote->com_fd, &fs_read))
			{	
				if (mote_process_sampling(mote, msg_buffer, 64) != 0)
				{
					DBG(("ERROR:Sampling failed.\n"));
					return -1;
				}
			}//end select serial received data
			
		}//end if (fs_sel)	
		else if (fs_sel == 0)
		{
//			DBG(("WARNING: select() timeout...\n"));
		}
		else
		{
			DBG(("ERROR: select() return %d\n", fs_sel));
			return -1;
		}

		if ( (mote->state == ST_MOTE_UPLOADING || mote->state == ST_MOTE_FILE_TRANSFER)
				 && mote->child_pid != -1 )
		{
			if (waitpid(mote->child_pid, &state, WNOHANG) == mote->child_pid)		//no block waitpid
			{
				DBG(("child state = %d.\n", state));	
				mote->state = ST_MOTE_IDLE;
				mote->child_pid = -1;
				complete = (state == 0) ? CC_SUCCESS : CC_FAIL;
				if (mote_cc(mote, complete) == -1)
					return -1;
			}
		}	
	}//end while(1)
	
	return 0;
}
	
	
int
mote_close(
	MOTE			*mote
	)
{
	
	// kill child process
	if (mote->child_pid != -1 
			&& kill(mote->child_pid, SIGKILL) == -1)	
	{
		DBG(("kill upload process failed.\n"));	
		return -1;
	}
	mote->child_pid = -1;
		
	com_close(mote->com_fd);
	mote->com_fd = -1;
	
	
	if (mote->socket_fd >= 0)
	{
		close(mote->socket_fd);
		mote->socket_fd = -1;
	}
		
	return 0;
}



int 
main(
	int 	argc, 
	char 	**argv
	)
{
	MOTE				mote;
	
	/* parser process argument */
	if (mote_open(&mote, argc, argv) != 0)
		return 1;
	
	/* mote connect to server */
	if (mote_connect(&mote) != 0)
	{
		mote_close(&mote);
		return 1;
	}
	
	/* mote receive and process command */
	if (mote_process(&mote) != 0)
	{
		mote_close(&mote);
		return 1;
	}
	
	if (mote_close(&mote) != 0)
	{
		return 1;
	}
	
	return 0;
}





