CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
INCLUDE_DIRS = -I../../../src_new/io/mapping/handle_pool -I../../../src_new/libs
SRC_DIR = ../../../src_new/io/mapping/handle_pool
LIBS = -L../../../src_new/libs/rkllm -lrkllmrt
TARGET = test_handle_pool
SOURCES = test_handle_pool.c $(SRC_DIR)/handle_pool.c $(SRC_DIR)/handle_pool_utils.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -o $(TARGET) $(SOURCES) $(LIBS)

test: $(TARGET)
	LD_LIBRARY_PATH=../../../src_new/libs/rkllm ./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all test clean
