CC=g++-4.4
AR=ar
LIBDIR=./lib
intermod: intermod-common.o List.o
	$(AR) cr $(LIBDIR)/libcommon.a intermod-common.o List.o

intermod-common.o: intermod-common.cpp intermod-common.h
	$(CC) -g -O0 -c intermod-common.cpp 	-o $@

List.o: List.cpp List.h
	$(CC) -g -O0 -c List.cpp 	-o $@

clean:
	rm -rf *.o lib/*
