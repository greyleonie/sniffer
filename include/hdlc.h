/********************************************************
* HDLC(High level Data Link Control protocol) head 
* Compile:	gcc
* Author:  	grey	
* Last Modify:  		2009.05.19
*********************************************************/

#ifndef __HDLC_H__
#define	__HDLC_H__


/* define HDLC control struct */
typedef struct _HDLC
{
	__uint8_t		state;							/* HDLC process state */
	__uint8_t		*recv_buffer;				/* receive destination buffer pointer */
	__int32_t		recv_buffer_length;	/* receive destination buffer length in bytes */
	__int32_t		recvd_length;				/* the counter of received bytes */
}HDLC;


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
	);
	

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
	);


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
	);


#endif/* __HDLC_H__ */


