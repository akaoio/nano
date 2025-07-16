CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -pthread
INCLUDE_DIRS = -I../../../src_new/io/core/worker_pool -I../../../src_new/io/core/queue
SRC_DIR = ../../../src_new/io/core
TARGET = test_worker_pool
SOURCES = test_worker_pool.c \
          $(SRC_DIR)/worker_pool/worker_pool.c \
          $(SRC_DIR)/queue/queue.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -o $(TARGET) $(SOURCES)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all test clean
