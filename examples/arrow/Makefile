
CC = gcc

CFLAGS = -Wall			 	\
	-DG_DISABLE_DEPRECATED 	 	\
	-DBDK_DISABLE_DEPRECATED 	\
	-DBDK_PIXBUF_DISABLE_DEPRECATED \
	-DBTK_DISABLE_DEPRECATED

arrow: arrow.c 
	$(CC) arrow.c -o arrow $(DEPRECTATED) `pkg-config btk+-2.0 --cflags --libs`

clean: 
	rm -f *.o arrow
