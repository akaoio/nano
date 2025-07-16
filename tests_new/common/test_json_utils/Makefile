CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
INCLUDE_DIRS = -I../../../src_new/common/json_utils -I../../../src_new/common
SRC_DIR = ../../../src_new/common/json_utils
TARGET = test_json_utils
SOURCES = test_json_utils.c $(SRC_DIR)/json_utils.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -o $(TARGET) $(SOURCES)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all test clean
