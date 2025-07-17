CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
INCLUDE_DIRS = -I../../../src/io/mapping/handle_pool \
               -I../../../src/io/mapping/rkllm_proxy \
               -I../../../src/common \
               -I../../../src/libs
SRC_DIRS = ../../../src/io/mapping/handle_pool \
           ../../../src/io/mapping/rkllm_proxy \
           ../../../src/common/json_utils
TARGET = test_lora
SOURCES = test_lora.c \
          ../../../src/io/mapping/handle_pool/handle_pool.c \
          ../../../src/io/mapping/handle_pool/handle_pool_utils.c \
          ../../../src/io/mapping/handle_pool/handle_pool_global.c \
          ../../../src/io/mapping/rkllm_proxy/rkllm_proxy.c \
          ../../../src/io/mapping/rkllm_proxy/rkllm_methods.c \
          ../../../src/io/mapping/rkllm_proxy/rkllm_advanced.c \
          ../../../src/io/mapping/rkllm_proxy/rkllm_callback.c \
          ../../../src/common/json_utils/json_utils.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -o $(TARGET) $(SOURCES) \
	-L../../../src/libs/rkllm -lrkllmrt

test: $(TARGET)
	LD_LIBRARY_PATH=../../../src/libs/rkllm:$$LD_LIBRARY_PATH ./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all test clean
