#Author:			Weida Pan
#student nunmber:	301295773
#SFU user name: 	lykonp
#lecture section:	D1
#instructor:		Brian G. Booth
#TA:				Scott Kristjanson
#date:				Dec. 1, 2015

CC = gcc

#options of gcc command
CFLAGS = -DMEMWATCH -DMW_STDIO -I.

#SRC is all source files
SRC1 = memwatch.c lyrebird.server.c commonfunc.c
SRC2 = mamwatch.c lyrebird.client.c decrypt.c commonfunc.c

#OBJS is all objects needed for compiling
OBJS1 = memwatch.o lyrebird.server.o commonfunc.o
OBJS2 = memwatch.o lyrebird.client.o decrypt.o commonfunc.o

#DEPS is the headers needed
DEPS1 = memwatch.h commonfunc.h
DEPS2 = memwatch.h decrypt.h commonfunc.h

ALL: lyrebird.client lyrebird.server

lyrebird.client:	$(OBJS2)
	$(CC) $(CFLAGS) -o $@ $^ -lm

%.o:	%.c $(DEPS2)
	$(CC) $(CFLAGS) -c -o $@ $<

lyrebird.server:	$(OBJS1)
	$(CC) $(CFLAGS) -o $@ $^ -lm

%.o:	%.c $(DEPS2)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o
	rm -f lyrebird.client
	rm -f lyrebird.server
	rm -f memwatch.log
	rm -f core
