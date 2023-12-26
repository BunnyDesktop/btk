
CC = gcc

#CFLAGS = -Wall			 	\
#	-DG_DISABLE_DEPRECATED 	 	\
#	-DBDK_DISABLE_DEPRECATED 	\
#	-DBDK_PIXBUF_DISABLE_DEPRECATED \
#	-DBTK_DISABLE_DEPRECATED

CFLAGS = 

text: text.c 
	$(CC) text.c -o text $(CFLAGS) `pkg-config btk+-2.0 --cflags --libs`

clean: 
	rm -f *.o text
