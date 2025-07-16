CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -pthread
INCLUDE_DIRS = -I../../../src_new/io/core/io -I../../../src_new/io/core/queue -I../../../src_new/io/core/worker_pool
SRC_DIR = ../../../src_new/io/core
TARGET = test_io
SOURCES = test_io.c \
          $(SRC_DIR)/io/io.c \
          $(SRC_DIR)/io/io_json.c \
          $(SRC_DIR)/io/io_worker.c \
          $(SRC_DIR)/queue/queue.c \
          $(SRC_DIR)/worker_pool/worker_pool.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -o $(TARGET) $(SOURCES)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all test clean
