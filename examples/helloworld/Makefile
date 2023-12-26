
CC = gcc

CFLAGS = -Wall			 	\
	-DG_DISABLE_DEPRECATED 	 	\
	-DBDK_DISABLE_DEPRECATED 	\
	-DBDK_PIXBUF_DISABLE_DEPRECATED \
	-DBTK_DISABLE_DEPRECATED

helloworld: helloworld.c 
	$(CC) helloworld.c -o helloworld $(CFLAGS) `pkg-config btk+-2.0 --cflags --libs`

clean: 
	rm -f *.o helloworld
