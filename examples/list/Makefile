
CC = gcc

#CFLAGS = -Wall			 	\
#	-DG_DISABLE_DEPRECATED 	 	\
#	-DBDK_DISABLE_DEPRECATED 	\
#	-DBDK_PIXBUF_DISABLE_DEPRECATED \
#	-DBTK_DISABLE_DEPRECATED

CFLAGS = 

list: list.c 
	$(CC) list.c -o list $(CFLAGS) `pkg-config btk+-2.0 --cflags --libs`

clean: 
	rm -f *.o list
