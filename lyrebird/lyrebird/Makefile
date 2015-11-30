CC = gcc

#options of gcc command
CFLAGS = -DMEMWATCH -DMW_STDIO -I.

CCMAINC = lyrebird.c

#SRC is all source files, or .c files
SRC = memwatch.c lyrebird.c decrypt.c 

#OBJS is all objects needed for compiling
OBJS = memwatch.o lyrebird.o decrypt.o 

#DEPS is the headers needed
DEPS = decrypt.h memwatch.h scheduling.h

#link .c files and headers to produce .o files
%.o:	%.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< 

#link .o files to produce lyrebird executable files
lyrebird:	$(OBJS) 
		$(CC) $(CFLAGS) -o $@ $^ -lm 

clean:
	rm -f *.o
	rm -f lyrebird
	rm -f core
	rm memwatch.log
