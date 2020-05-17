CC = gcc
CFLAGS = -g
PROG = x
OBJS = OS.o list.o

x: $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS)

a3.o: OS.c
	$(CC) $(CFLAGS) -c OS.c

clean:
	rm -f OS.o x
