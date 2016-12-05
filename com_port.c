/********************************************************
* Serial Port Control
* Compile:	gcc
* Author:  	grey	
* Last Modify:  		2009.05.19
*********************************************************/
#include <sys/types.h>
#include <fcntl.h>
#include <termios.h>
#include <strings.h>
#include <unistd.h>

//#define	_USE_DEBUG_
#include "debug.h"


/***********************************************************
*	com_open()
*		Open serial port by char device file 
*
* return:
*		>0 --> serial device file descriptor
*   -1 --> failed
*
* parameter:
*		@filename		serial device file path name
* 
************************************************************/
int		
com_open(			
	char			*pathname
	)
{
	int				fd;
	
	fd = open(pathname, (O_RDWR | O_NOCTTY | O_NONBLOCK));
	if (fd < 0)
	{
		DBG(("ERROR:COM port %s open failed.\n", pathname));
		return -1;
	}
	
	DBG(("port %s open success, fd=%d\n", pathname, fd));
	
	return fd;
}


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
	)
{
	struct 	termios 	termios_new;
	int			baud;
	int			ret;
//	int			i;
//	unsigned char		ch;
	
	bzero(&termios_new, sizeof(termios_new));
	
	cfmakeraw(&termios_new);	/* set termios raw data */
	
	/* set baud rate */
	switch(baud_rate)
	{
		case 2400:
			baud = B2400;
			break;
		case 4800:
			baud = B4800;
			break;
		case 38400:
			baud = B38400;
			break;
		case 57600:
			baud = B57600;
			break;
		case 115200:
			baud = B115200;
			break;
		default:
			baud = B9600;
			break;
	}/* end switch */
	cfsetispeed(&termios_new, baud);
	cfsetospeed(&termios_new, baud);
	
	
	termios_new.c_cflag |= (CLOCAL | CREAD);
	
	/* flow control */
	switch(flow_ctrl)
	{
		case 'H':
		case 'h':	
			termios_new.c_cflag |= CRTSCTS; 		/* hardware flow control */
			break;
		case 'S':
		case 's':	
			termios_new.c_cflag |= (IXON | IXOFF | IXANY);	/* software flow control */
			break;
		default:
			termios_new.c_cflag &= ~CRTSCTS;		/* default no flow control */
			break;
	}/* end switch */
	
	/* data bit */
	termios_new.c_cflag &= ~CSIZE;
	switch(data_bit)
	{
		case 5:
			termios_new.c_cflag |= CS5;
			break;
		case 6:
			termios_new.c_cflag |= CS6;
			break;
		case 7:
			termios_new.c_cflag |= CS7;
			break;
		default:
			termios_new.c_cflag |= CS8;
			break;
	}/* end switch */
	
	/* parity check */
	switch(parity)
	{
		case 'O':
		case 'o':	
			termios_new.c_cflag |= PARENB;		/* odd check */
			termios_new.c_cflag &= ~PARODD;
			break;
		case 'E':
		case 'e':	
			termios_new.c_cflag |= PARENB;		/* even check */
			termios_new.c_cflag |= PARODD;
			break;
		default:
			termios_new.c_cflag &= ~PARENB; 	/* default no check */
			break;
	}/* end switch */
	
	/* stop bit */
	if (stop_bit == 2)
		termios_new.c_cflag |= CSTOPB;		/* 2 stop bit */
	else
		termios_new.c_cflag &= ~CSTOPB;		/* 1 stop bit */

	/* other attribute */
	termios_new.c_oflag &= ~OPOST;
	termios_new.c_iflag &= ~ICANON;
	termios_new.c_cc[VMIN] = 1;				/* read char min quantity */
	termios_new.c_cc[VTIME] = 1; 			/* wait time unit (1/10) second */
	
	/* clear data in receive buffer */
	if (tcflush(fd, TCIFLUSH) != 0)	
		DBG(("com flush failed.\n"));
		
	/* set config data */
	ret = tcsetattr(fd, TCSANOW, &termios_new);
	
	DBG(("com_set ret=%d\n", ret));
	return ret;
}


/***********************************************************
*	com_close()
*		close serial port
*
* return:
*		0 --> success
*
* parameter:
*		@fd			serial device file descriptor
* 
************************************************************/
void 
com_close(	
	int			fd
	)
{
	DBG(("close com port.\n"));
	if (fd > 0)
		close(fd);
}


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
	)
{
	int 	len;
	
	len = read(fd, buffer, length);
	if (len < 0)
	{
		DBG(("read failed.\n"));
		return -1;	
	}
	
	return len;
}


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
	)
{
	int 	len;
	
	len = write(fd, buffer, length);
	if (len < 0)
	{
		DBG(("read failed.\n"));
		return -1;	
	}
	
	return len;
}


