
#ifndef __PACKET_H__
#define	__PACKET_H__

#define	MAX_FILE_NAME_LENGTH			32
#define	MAX_MSG_BUFFER_LENGTH			32
#define	MAX_MOTE_NUM							64
#define	MAX_SERIAL_MSG_LENGTH			64

// define packet struct, reference to the d
typedef struct _PACKET
{
	__uint8_t		type;	//cmd, data, state, cc, event
	
	union		// packet content
	{
		// state packet
		__uint8_t		state;
		
		// complete code packet
		__uint8_t		cc;
		
		// event packet description
		struct
		{
			__uint8_t		map_length;
			struct
			{
				__uint8_t		usb_num;
				__uint8_t		usb_dev;
				__uint8_t		msg_port;
				__uint8_t		upd_port;
			}mote_des[MAX_MOTE_NUM];
		}__attribute__((__packed__)) ev_mote_map;
	
		struct
		{
			__uint8_t		sniffer_num;
		}__attribute__((__packed__)) ev_sniffer_conn;
		
		struct
		{
			__uint8_t		sniffer_num;
			__uint8_t		mote_num;
		}__attribute__((__packed__)) ev_mote_conn;
		
		// data packet message
		struct
		{
			__uint8_t		length;
			__uint32_t	ref_time_sec;		//fix it
			__uint16_t	ref_time_msec;	
			__uint16_t	mid;
			__uint8_t		gid;
			__uint8_t		io;
			__uint8_t		result;
			__uint8_t		queue_cnt;
			__uint8_t		msg[MAX_MSG_BUFFER_LENGTH];
		}__attribute__((__packed__)) data_msg;
		
		// command packet argument
		struct
		{
			__uint8_t		mote_type;
			__uint8_t		tos_version;
			__uint16_t	mid;
			__uint8_t		file_name_length;
			__uint8_t		file_name[MAX_FILE_NAME_LENGTH];
		}__attribute__((__packed__))cmd_upload_mote;
		
		struct
		{
			__uint8_t		file_name_length;
			__uint8_t		file_name[MAX_FILE_NAME_LENGTH];
		}__attribute__((__packed__)) cmd_file_transfer;
		
		
		struct
		{
			__uint8_t		mote_num;
		}__attribute__((__packed__)) cmd_open_mote;		

//*/		
	}__attribute__((__packed__)) content;
	
	int		send_fd;		//fix it
	int		(*send)(int fd, unsigned char *buffer, int length);
	int		recv_fd;
	int		(*recv)(int fd, unsigned char *buffer, int length);
	
}PACKET;



// define command code
#define	CMD_OPEN_MOTE				0x80
#define	CMD_START_SAMPLING	0x70
#define	CMD_STOP_SAMPLING		0x71
#define	CMD_UPLOAD_MOTE			0x72
#define	CMD_CLOSE_MOTE			0x73
#define	CMD_GET_FILE				0x74
#define	CMD_PUT_FILE				0x75
#define	CMD_GET_STATE				0x76
#define	CMD_STOP_UPLOAD			0x77
#define	CMD_STOP_TRANSMIT		0x78

// define data code
#define	DATA_SERIAL_MSG			0x90

// define event code
#define	EV_UPD_MOTE_MAP			0x60
#define	EV_SNIFFER_CONN			0x61
#define	EV_MOTE_CONN				0x62

#define	COMPLETE_CODE				0xaa
// define	complete code
#define	CC_SUCCESS					0
#define	CC_FAIL							1
#define	CC_SKIP							2
#define	CC_ARGUMENT					3
#define	CC_TIMEOUT					4

#define	STATE_CODE					0x55


int
packet_recv(
	PACKET		*packet
	);

int
packet_send(
	PACKET		*packet
	);
	
	
#endif /* __PACKET_H__ */

