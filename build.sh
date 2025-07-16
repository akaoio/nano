#!/bin/bash
gcc -std=c99 -O2 -Wall -Wextra \
    src/main.c \
    -L./src/libs/rkllm \
    -lrkllmrt \
    -lpthread \
    -Wl,-rpath,./src/libs/rkllm \
    -o nano
echo "Built → ./nano"