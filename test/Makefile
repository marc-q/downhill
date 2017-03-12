CC = gcc
OBJECTS = ../src/html.c ../src/parser.c ../src/markdown.c test.c
LIBS =
CFLAGS = -Wall -Wextra -O2
BINDIR = $(DESTDIR)/usr/bin
NAME = test

test: $(OBJECTS)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJECTS) $(LIBS)

clean:
	rm $(NAME)
