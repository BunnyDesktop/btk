
CC = gcc

CFLAGS = -Wall			 	\
	-DG_DISABLE_DEPRECATED 	 	\
	-DBDK_DISABLE_DEPRECATED 	\
	-DBDK_PIXBUF_DISABLE_DEPRECATED \
	-DBTK_DISABLE_DEPRECATED

scribble-simple: scribble-simple.c 
	$(CC) scribble-simple.c -o scribble-simple $(CFLAGS) `pkg-config btk+-2.0 --cflags --libs`

clean: 
	rm -f *.o scribble-simple
