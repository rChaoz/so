CC = gcc
CPPFLAGS = -I../utils
CFLAGS = -fPIC -Wall -Wextra -g
LDFLAGS = -shared

SRCS = osmem.c ../utils/printf.c mmap_alloc.c brk_alloc.c
OBJS = $(SRCS:.c=.o)
TARGET = libosmem.so

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) ${LDFLAGS} -o $@ $^

pack: clean
	-rm -f ../src.zip
	zip -r ../src.zip *

clean:
	-rm -f ../src.zip
	-rm -f $(TARGET)
	-rm -f $(OBJS)
