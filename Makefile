all: 
	gcc llfs.c -o llfs -g -Wall `pkg-config fuse --cflags --libs`
clean:
	rm -f llfs