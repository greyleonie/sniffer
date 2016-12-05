/********************************************************
* Serial Port Control Head
* Compile:	gcc
* Author:  	grey	
* Last Modify:  		2009.05.19
*********************************************************/

#ifndef __COM_PORT_H__
#define	__COM_PORT_H__


/***********************************************************
*	com_open()
*		Open serial port by char device file 
*
* return:
*		>0 --> serial device file descriptor
*   -1 --> failed
*
* parameter:
*		@pathname		serial device file path name
* 
************************************************************/
int		
com_open(			
	char			*pathname
	);
	
	
/***********************************************************
*	com_set()
*		Config serial port parameter
*
* return:
*		0 --> success
*   -1 --> failed
*
* parameter:
*		@fd			serial device file descriptor
*		@baud_rate		serial communication baud rate
*		@data_bit			data bit count, default is 8
*		@stop_bit     stop bit count, default is 1
*		@parity				odd or even parity, defalut is no parity
*		@flow_ctrl		hareware or software flow control, default is no fc
* 
************************************************************/
int		
com_set(			
	int			fd,
	int			baud_rate,
	char		data_bit,
	char		stop_bit,
	char		parity,
	char		flow_ctrl
	);
	
	
/***********************************************************
*	com_close()
*		close serial port
*
* return:
*
* parameter:
*		@fd			serial device file descriptor
* 
************************************************************/
void
com_close(		
	int			fd
	);


/***********************************************************
*	com_read()
*		Read data from serial port
*
* return:
*		>0 --> read data length
*   -1 --> failed
*
* parameter:
*		@fd			serial device file descriptor
*		@buffer		read data buffer pointer
*		@length			read data max length
* 
************************************************************/
int
com_read(
	int						fd,
	unsigned char	*buffer,
	int						length
	);
	
	
/***********************************************************
*	com_write()
*		Write data to serial port
*
* return:
*		>0 --> write data length
*   -1 --> failed
*
* parameter:
*		@fd			serial device file descriptor
*		@buffer		write data buffer pointer
*		@length			write data length
* 
************************************************************/
int
com_write(
	int						fd,
	unsigned char	*buffer,
	int						length
	);
	
	
#endif/* __COM_PORT_H__ */


