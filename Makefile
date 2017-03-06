CC = gcc
OBJECTS = downhill.c
LIBS =
CFLAGS = -Wall -Wextra -O2
BINDIR = $(DESTDIR)/usr/bin
NAME = downhill

downhill: $(OBJECTS)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJECTS) $(LIBS)

clean:
	rm $(NAME)
