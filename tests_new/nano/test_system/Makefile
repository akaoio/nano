CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
INCLUDE_DIRS = -I../../../src_new/nano/system
SRC_DIR = ../../../src_new/nano/system
TARGET = test_system
SOURCES = test_system.c \
          $(SRC_DIR)/system_info/system_info.c \
          $(SRC_DIR)/system_info/system_model.c \
          $(SRC_DIR)/system_info/system_memory.c \
          $(SRC_DIR)/resource_mgr/resource_mgr.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -o $(TARGET) $(SOURCES)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all test clean
