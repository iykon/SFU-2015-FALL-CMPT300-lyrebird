CC = gcc

#options of gcc command
CFLAGS = -DMEMWATCH -DMW_STDIO -I.

#SRC is all source files
SRC1 = memwatch.c lyrebird.server.c
SRC2 = mamwatch.c lyrebird.client.c decrypt.c

#OBJS is all objects needed for compiling
OBJS1 = memwatch.o lyrebird.server.o
OBJS2 = memwatch.o lyrebird.client.o decrypt.o

#DEPS is the headers needed
DEPS1 = memwatch.h 
DEPS2 = memwatch.h decrypt.h

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
	rm -f *.0
	rm lyrebird.clent
	rm lyrebird.server
	rm memwatch.log
	rm -f core
