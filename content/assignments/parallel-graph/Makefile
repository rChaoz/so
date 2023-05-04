SRC := skel
BUILD_DIR := build


CC := gcc
CFLAGS := -c -Wall -g -I $(SRC)
LD := ld
LDFLAGS :=
LDLIBS := -lpthread

SERIAL_SRCS := $(SRC)/serial.c $(SRC)/os_graph.c
PARALLEL_SRCS:= $(SRC)/parallel.c $(SRC)/os_graph.c $(SRC)/os_list.c $(SRC)/os_threadpool.c
SERIAL_OBJS := $(patsubst $(SRC)/%.c,$(BUILD_DIR)/%.o,$(SERIAL_SRCS))
PARALLEL_OBJS := $(patsubst $(SRC)/%.c,$(BUILD_DIR)/%.o,$(PARALLEL_SRCS))

all: serial parallel

always:
	mkdir -p build

serial: always $(SERIAL_OBJS)
	$(CC) $(LDFLAGS) -o serial $(SERIAL_OBJS)

parallel: always $(PARALLEL_OBJS)
	$(CC) $(LDFLAGS) -o parallel $(PARALLEL_OBJS) $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf build serial parallel
