
CC = gcc

CFLAGS = -Wall			 	\
	-DG_DISABLE_DEPRECATED 	 	\
	-DBDK_DISABLE_DEPRECATED 	\
	-DBDK_PIXBUF_DISABLE_DEPRECATED

all: menu itemfactory

menu: menu.c 
	$(CC) menu.c -o menu $(CFLAGS) `pkg-config btk+-2.0 --cflags --libs`

itemfactory: itemfactory.c
	$(CC) itemfactory.c -o itemfactory $(CFLAGS) `pkg-config btk+-2.0 --cflags --libs`

clean: 
	rm -f *.o menu itemfactory
