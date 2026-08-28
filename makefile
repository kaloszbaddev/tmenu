.PHONY: all clean install uninstall 

CC = cc
TARGET = tmenu
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS = -I./include -Wall -D_DEFAULT_SOURCE -std=c23
LIBS = -L./lib -ltui

all: clean install

$(TARGET): $(OBJS)
	$(CC) $^ -o $@ $(LIBS)

$(OBJS): %.o : %.c
	$(CC) $(CFLAGS) $^ -c

install: $(TARGET)
	cp $(TARGET) /usr/local/bin
	cp $(TARGET) /usr/bin

uninstall: 
	rm -f /usr/local/bin/$(TARGET)
	rm -f /usr/bin/$(TARGET)

clean:
	rm -f *.o $(TARGET)
