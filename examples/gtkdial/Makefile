
CC = gcc

CFLAGS = -Wall			 	\
	-DG_DISABLE_DEPRECATED 	 	\
	-DBDK_DISABLE_DEPRECATED 	\
	-DBDK_PIXBUF_DISABLE_DEPRECATED \
	-DBTK_DISABLE_DEPRECATED

dial_test: btkdial.o dial_test.o
	$(CC) dial_test.o btkdial.o -o dial_test `pkg-config --libs btk+-2.0`

dial_test.o: dial_test.c btkdial.h
	$(CC) -c dial_test.c -o dial_test.o $(CFLAGS) `pkg-config btk+-2.0 --cflags`

btkdial.o: btkdial.c btkdial.h
	$(CC) -c btkdial.c -o btkdial.o $(CFLAGS) `pkg-config btk+-2.0 --cflags`

clean: 
	rm -f *.o dial_test
