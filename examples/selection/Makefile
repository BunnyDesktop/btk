
CC = gcc

CFLAGS = -Wall			 	\
	-DG_DISABLE_DEPRECATED 	 	\
	-DBDK_DISABLE_DEPRECATED 	\
	-DBDK_PIXBUF_DISABLE_DEPRECATED \
	-DBTK_DISABLE_DEPRECATED

all: gettargets setselection

gettargets: gettargets.c 
	$(CC) gettargets.c -o gettargets $(CFLAGS) `pkg-config btk+-2.0 --cflags --libs`

setselection: setselection.c
	$(CC) setselection.c -o setselection $(CFLAGS) `pkg-config btk+-2.0 --cflags --libs`

clean: 
	rm -f *.o gettargets setselection
