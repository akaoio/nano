CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
INCLUDE_DIRS = -I../../../src/io/mapping/handle_pool -I../../../src/libs
SRC_DIR = ../../../src/io/mapping/handle_pool
LIBS = -L../../../src/libs/rkllm -lrkllmrt
TARGET = test_handle_pool
SOURCES = test_handle_pool.c $(SRC_DIR)/handle_pool.c $(SRC_DIR)/handle_pool_utils.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -o $(TARGET) $(SOURCES) $(LIBS)

test: $(TARGET)
	LD_LIBRARY_PATH=../../../src/libs/rkllm ./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all test clean
