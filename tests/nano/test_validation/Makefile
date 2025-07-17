CC = gcc
CFLAGS = -Wall -Wextra -std=c2x -g
INCLUDE_DIRS = -I../../../src/nano/validation
SRC_DIR = ../../../src/nano/validation/model_checker
TARGET = test_validation
SOURCES = test_validation.c $(SRC_DIR)/model_checker.c $(SRC_DIR)/model_compatibility.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -o $(TARGET) $(SOURCES)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) *.tmp

.PHONY: all test clean
