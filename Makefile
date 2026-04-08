CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -O2
LDFLAGS = -lncurses
TARGET  = tetris
SRCDIR  = src
SRCS    = $(SRCDIR)/main.c $(SRCDIR)/game.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(SRCDIR)/%.o: $(SRCDIR)/%.c $(SRCDIR)/game.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
