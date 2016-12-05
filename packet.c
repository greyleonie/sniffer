
#include <sys/types.h>
//#define	_USE_DEBUG_
#include "debug.h"
#include "hdlc.h"
#include "packet.h"


static int
packet_length(
	PACKET		*packet
	)
{
	int		length = 0;
	
	switch(packet->type)
	{
		case CMD_OPEN_MOTE:
			length = sizeof(packet->content.cmd_open_mote) + ((void *)&packet->content - (void *)packet);
			break;
			
		case CMD_UPLOAD_MOTE:
			length = packet->content.cmd_upload_mote.file_name_length + ((void *)&packet->content.cmd_upload_mote.file_name[0] - (void *)packet);
			break;
			
		case CMD_GET_FILE:
		case CMD_PUT_FILE:
			length = packet->content.cmd_file_transfer.file_name_length + ((void *)&packet->content.cmd_file_transfer.file_name[0] - (void *)packet);
			break;
			
		case CMD_START_SAMPLING:
		case CMD_STOP_SAMPLING:
		case CMD_CLOSE_MOTE:
		case CMD_GET_STATE:
		case CMD_STOP_UPLOAD:
		case CMD_STOP_TRANSMIT:
			length = ((void *)&packet->content - (void *)packet);
			break;
		
		case DATA_SERIAL_MSG:
			length = packet->content.data_msg.length + ((void *)&packet->content.data_msg.ref_time_sec - (void *)packet);
			break;
	
		case EV_UPD_MOTE_MAP:
			length = packet->content.ev_mote_map.map_length + ((void *)&packet->content.ev_mote_map.mote_des[0] - (void *)packet);
			break;
			
		case EV_SNIFFER_CONN:
			length = sizeof(packet->content.ev_sniffer_conn) + ((void *)&packet->content - (void *)packet);
			break;
			
		case EV_MOTE_CONN:
			length = sizeof(packet->content.ev_mote_conn) + ((void *)&packet->content - (void *)packet);
			break;
			
		case STATE_CODE:
			length = sizeof(packet->content.state) + ((void *)&packet->content - (void *)packet);
			break;
			
		case COMPLETE_CODE:
			length = sizeof(packet->content.cc) + ((void *)&packet->content - (void *)packet);
			break;
			
		default:
			break;
	}//end switch(packet->type) 
	
	DBG(("packet length = %d.\n", length));
	return length;
}

int
packet_recv(
	PACKET		*packet
	)
{
	HDLC					hdlc;
	unsigned char	recv_buffer[64];	//socket data receive buffer
	int						recv_length;
	int						length;
	int						i;

	if (packet->recv == NULL)		// packet recv() not define
	{
		DBG(("packet recv() not define.\n"));
		return -1;
	}
	
	recv_length = packet->recv(packet->recv_fd, recv_buffer, 64);
	if (recv_length == -1)
	{
		DBG(("ERROR:data receive failed.\n"));
		return -1;
	}
		
	hdlc_init_recv(&hdlc, (unsigned char *)packet, ((void *)&packet->send_fd - (void *)packet));
	for (i = 0; i < recv_length; ++i)
	{
		length = hdlc_recv_char(&hdlc, recv_buffer[i]);
		if (length > 0)
			return packet_length(packet);
		else if (length == -1)
		{
			DBG(("ERROR:packet data overflow"));
			return -1;
		}
	}// end for

	return 0;
}

int
packet_send(
	PACKET		*packet
	)
{
	unsigned char		send_buffer[sizeof(PACKET) + 10];
	int							length;
	
	if (packet->send == NULL)
	{
		DBG(("packet send() not define.\n"));
		return -1;
	}
	
	length = packet_length(packet);
	if (length == 0)
		return -1;
		
	length = hdlc_send_buffer(send_buffer, (unsigned char *)packet, length);
	
	return packet->send(packet->send_fd, send_buffer, length);
}



