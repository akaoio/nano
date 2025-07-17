CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
INCLUDES = -I../../src/common

# Common utilities test
test_json_utils: test_json_utils.c ../../src/common/json_utils/json_utils.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

.PHONY: test clean
test: test_json_utils
	./test_json_utils

clean:
	rm -f test_json_utils
