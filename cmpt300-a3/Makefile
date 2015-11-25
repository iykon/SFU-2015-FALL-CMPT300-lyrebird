CC = gcc
CFLAGS = -DMEMWATCH -DMW_STDIO -I.
CCMAINC = lyrebird.c
SRC = memwatch.c lyrebird.c decrypt.c 
OBJS = memwatch.o lyrebird.o decrypt.o 
DEPS = decrypt.h memwatch.h scheduling.h
#hoasdfhoasidfo

%.o:	%.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< 

lyrebird:	$(OBJS) 
		$(CC) $(CFLAGS) -o $@ $^ -lm 

clean:
	rm -f *.o
	rm -f lyrebird
	rm -f core
	rm memwatch.log
