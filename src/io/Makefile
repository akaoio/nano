CC = gcc
CFLAGS = -std=c2x -Wall -Wextra -fPIC -O2 -I../libs/rkllm
LDFLAGS = -shared -L../libs/rkllm -Wl,-rpath,../libs/rkllm -lrkllmrt

SRCDIR = .
OBJDIR = obj

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
TARGET = libio.so

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/lib/
	ldconfig
