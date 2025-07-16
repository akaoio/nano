# Root Makefile for nano + io project

CC = gcc
CFLAGS = -std=c2x -Wall -Wextra -O2 -I./src/libs/rkllm -I./tests/io
LDFLAGS = -L./src/io -L./src/libs/rkllm -Wl,-rpath,./src/io -Wl,-rpath,./src/libs/rkllm -lio -lrkllmrt -lpthread

# Directories
SRC_DIR = src
IO_DIR = $(SRC_DIR)/io
NANO_DIR = $(SRC_DIR)/nano
TEST_DIR = tests
TEST_IO_DIR = $(TEST_DIR)/io

# Targets
LIBIO = $(IO_DIR)/libio.so
NANO_BIN = nano
TEST_BIN = test

# Source files
IO_SOURCES = $(wildcard $(IO_DIR)/*.c) $(wildcard $(IO_DIR)/operations/*.c)
IO_OBJECTS = $(IO_SOURCES:$(IO_DIR)/%.c=$(IO_DIR)/obj/%.o)

TEST_IO_SOURCES = $(wildcard $(TEST_IO_DIR)/*.c)
TEST_SOURCES = $(TEST_DIR)/test.c

.PHONY: all clean test io nano

all: io nano

io: $(LIBIO)

nano: $(NANO_BIN)

$(LIBIO): $(IO_OBJECTS)
	$(CC) -shared -fPIC -o $@ $^ -L$(SRC_DIR)/libs/rkllm -Wl,-rpath,$(SRC_DIR)/libs/rkllm -lrkllmrt

$(IO_DIR)/obj/%.o: $(IO_DIR)/%.c | $(IO_DIR)/obj
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(IO_DIR)/obj/operations/%.o: $(IO_DIR)/operations/%.c | $(IO_DIR)/obj/operations
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(IO_DIR)/obj:
	mkdir -p $(IO_DIR)/obj

$(IO_DIR)/obj/operations:
	mkdir -p $(IO_DIR)/obj/operations

$(NANO_BIN): $(LIBIO)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/main.c $(LDFLAGS)

test: $(LIBIO) $(TEST_BIN)

$(TEST_BIN): $(TEST_SOURCES) $(TEST_IO_SOURCES)
	$(CC) $(CFLAGS) -o $@ $(TEST_SOURCES) $(TEST_IO_SOURCES) $(LDFLAGS)

clean:
	rm -rf $(IO_DIR)/obj $(LIBIO) $(NANO_BIN) $(TEST_BIN)
	@echo "Clean completed"

install: all
	cp $(LIBIO) /usr/local/lib/
	cp $(NANO_BIN) /usr/local/bin/
	ldconfig
	@echo "Installation completed"

help:
	@echo "Available targets:"
	@echo "  all    - Build both libio.so and nano executable"
	@echo "  io     - Build only libio.so"
	@echo "  nano   - Build only nano executable"
	@echo "  test   - Build and run tests"
	@echo "  clean  - Clean all build artifacts"
	@echo "  install - Install to system"
	@echo "  help   - Show this help"
