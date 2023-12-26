
CC = gcc

#CFLAGS = -Wall			 	\
#	-DG_DISABLE_DEPRECATED 	 	\
#	-DBDK_DISABLE_DEPRECATED 	\
#	-DBDK_PIXBUF_DISABLE_DEPRECATED \
#	-DBTK_DISABLE_DEPRECATED

CFLAGS = 

tree: tree.c 
	$(CC) tree.c -o tree $(CFLAGS) `pkg-config btk+-2.0 --cflags --libs`

clean: 
	rm -f *.o tree
