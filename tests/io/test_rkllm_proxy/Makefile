CC = gcc
CFLAGS = -Wall -Wextra -std=c2x -g
INCLUDE_DIRS = -I../../../src/io/mapping/rkllm_proxy -I../../../src/io/mapping/handle_pool -I../../../src/common -I../../../src/libs
SRC_DIR = ../../../src/io/mapping
COMMON_DIR = ../../../src/common
TARGET = test_rkllm_proxy
SOURCES = test_rkllm_proxy.c \
          $(SRC_DIR)/rkllm_proxy/rkllm_proxy.c \
          $(SRC_DIR)/rkllm_proxy/rkllm_operations.c \
          $(SRC_DIR)/handle_pool/handle_pool.c \
          $(SRC_DIR)/handle_pool/handle_pool_utils.c \
          $(COMMON_DIR)/json_utils/json_utils.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -o $(TARGET) $(SOURCES) -L../../../src/libs/rkllm -lrkllmrt

test: $(TARGET)
	LD_LIBRARY_PATH=../../../src/libs/rkllm:$$LD_LIBRARY_PATH ./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all test clean
