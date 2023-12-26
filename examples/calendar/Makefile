
CC = gcc

CFLAGS = -Wall			 	\
	-DG_DISABLE_DEPRECATED 	 	\
	-DBDK_DISABLE_DEPRECATED 	\
	-DBDK_PIXBUF_DISABLE_DEPRECATED

calendar: calendar.c
	$(CC) calendar.c -o calendar $(CFLAGS) `pkg-config btk+-2.0 --cflags --libs`

clean: 
	rm -f calendar
