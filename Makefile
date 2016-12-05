CC = gcc
CFLAGS = -g -O2 -Wall -pipe
INCLUDE = -I include
ALL = mote sniffer server

obj = mote.o socket.o com_port.o hdlc.o server_test.o sniffer.o packet.o misc.o
$(obj): %.o:%.c
	$(CC) -c $(CFLAGS) $(INCLUDE) $< -o $@

mote_obj = mote.o socket.o com_port.o hdlc.o packet.o misc.o
mote: $(mote_obj)
	$(CC) $^ -o $@


server_obj = server_test.o
server: $(server_obj)
	$(CC) $^ -o $@


sniffer_obj = sniffer.o	socket.o hdlc.o packet.o misc.o
sniffer: $(sniffer_obj)
	$(CC) $^ -o $@
	

all: $(ALL)

.PHONY: clean
clean:
	-rm -f $(ALL) *.o

