/********************************************************
* HDLC(High level Data Link Control protocol)
* Compile:	gcc
* Author:  	grey	
* Last Modify:  		2009.05.19
*********************************************************/


#include <sys/types.h>

//#define	_USE_DEBUG_
#include "debug.h"
#include "hdlc.h"

/****************************************************************
* HDLC is a type of Data_Link_Level protocols. 
* It uses 0x7e and 0x7d as the keywords for data packet control.
*
* Every packet is defined by a pair of 0x7e
*		0x7e ...data... 0x7e
*
* If 0x7e or 0x7d has been in the data queue, uses 0x7d to indicate
* it. 
* When the data is 0x7e, it will be indicated by these 2 bytes
* 	0x7e --> 0x7d, 0x5e
* When the data is 0x7d, it will be indicated by these 2 bytes
*  	0x7d --> 0x7d, 0x5d
*
*****************************************************************/


/* define HDLC keywords */
#define	HDLC_KEY_FLAG		0x7e
#define	HDLC_KEY_ESC		0x7d

/* define	port receive state */
#define	HDLC_ST_NOSYNC		0
#define	HDLC_ST_SYNC			1
#define	HDLC_ST_INFO			2	
#define	HDLC_ST_ESC				3



/***********************************************************
*	hdlc_init_recv()
*		Init receive buffer for the HDLC object. The object save
*		the buffer and the max length for process control.
*
* return:
*		0 --> success
*		-1 --> failed
*
* parameter:
*		@hdlc		HDLC control object pointer
*		@buffer	The receive buffer pointer for this HDLC control object
* 	@buffer_length	The max receive buffer length
* 
************************************************************/
int			
hdlc_init_recv(								
	HDLC				*hdlc,
	__uint8_t		*buffer,				
	__int32_t		buffer_length		
	)
{
	hdlc->state = HDLC_ST_NOSYNC;
	hdlc->recv_buffer = buffer;
	hdlc->recv_buffer_length = buffer_length;
	hdlc->recvd_length = 0;
	
	return 0;
}



/***********************************************************
*	hdlc_recv_char()
*		Receiving one char and set it into data stream buffer.
*
* return:
*		> 0 --> the data queue length of received packet.
*		0 --> the receiving is not complete.
*	  -1 --> receive buffer overflow, failed.
*
* parameter:
*		@hdlc		HDLC control object pointer
*		@ch			A new byte of received data stream
* 
************************************************************/
__int32_t			
hdlc_recv_char(					
	HDLC				*hdlc,
	__uint8_t		ch				
	)
{
	__int32_t 	ret;
	
	switch(hdlc->state)
	{
		case HDLC_ST_NOSYNC:
			if (ch == HDLC_KEY_FLAG)
			{
				hdlc->recvd_length = 0;
				hdlc->state = HDLC_ST_SYNC;
			}
			break;
			
		case HDLC_ST_SYNC:
			if (ch != HDLC_KEY_FLAG)	/* the next byte after 0x7e must not be 0x7e */
			{
				if (ch == HDLC_KEY_ESC)
				{
					hdlc->state = HDLC_ST_ESC;
				}
				else
				{
					if (hdlc->recvd_length == hdlc->recv_buffer_length)	/* packet length overflow */
						return -1;
					hdlc->recv_buffer[hdlc->recvd_length] = ch;		/* save the first byte */
					hdlc->recvd_length++;
					hdlc->state = HDLC_ST_INFO;
				}
			}
			break;
			
		case HDLC_ST_INFO:
			if (ch == HDLC_KEY_FLAG)
			{
				hdlc->state = HDLC_ST_NOSYNC;
				ret = hdlc->recvd_length;
				hdlc->recvd_length = 0;
				return ret;		/* receive done */
			}
			else if (ch == HDLC_KEY_ESC)
			{
				hdlc->state = HDLC_ST_ESC;
			}
			else
			{
				if (hdlc->recvd_length == hdlc->recv_buffer_length)	/* packet length overflow */
					return -1;
				hdlc->recv_buffer[hdlc->recvd_length] = ch;
				hdlc->recvd_length++;
			}	
			break;
			
		case HDLC_ST_ESC:
			if (hdlc->recvd_length == hdlc->recv_buffer_length)	/* packet length overflow */
				return -1;
			hdlc->recv_buffer[hdlc->recvd_length] = (ch ^ 0x20);	/* get 0x7d or 0x7e */
			hdlc->recvd_length++;
			hdlc->state = HDLC_ST_INFO;
			break;
			
		default:
			hdlc->state = HDLC_ST_NOSYNC;
			break;
	}/* end swtich */
	
	return 0;
}



/***********************************************************
*	hdlc_send_buffer()
*		Make data queue with HDLC, from src to dst.
*
* return:
*		> 0 --> the HDLC data queue length of dst buffer.
*
* parameter:
*		@dst		HDLC destination buffer pointer
*		@src		source data stream buffer pointer
*		@src_length		source data stream length
* 
************************************************************/
__int32_t			
hdlc_send_buffer(
	__uint8_t		*dst,
	__uint8_t		*src,
	__int32_t		src_length
	)
{
	__int32_t		i, len = 0;
	__uint8_t		ch;
	
	/* start flag */
	*dst++ = HDLC_KEY_FLAG;
	len++;
	
	for (i = 0; i < src_length; ++i)
	{
		ch = src[i];
		if (ch == HDLC_KEY_FLAG)
		{
			*dst++ = HDLC_KEY_ESC;
			*dst++ = 0x5e;
			len += 2;
		}
		else if (ch == HDLC_KEY_ESC)
		{
			*dst++ = HDLC_KEY_ESC;
			*dst++ = 0x5d;
			len += 2;
		}
		else
		{
			*dst++ = ch;
			len++;
		}
	}/* end for */
	
	/* end flag; */
	*dst = HDLC_KEY_FLAG;
	len++;
	
	return len;
}

