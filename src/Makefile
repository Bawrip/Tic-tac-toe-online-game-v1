all:
	gcc -Wall server.c llist.c -o server
	gcc -Wall -g client.c -o client `pkg-config --cflags gtk+-2.0 --libs gtk+-2.0`

