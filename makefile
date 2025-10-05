# Requires: libhdf5-dev (provides pkg-config 'hdf5')
VERSION   = $(shell git describe --tags --always)
CC        = gcc
CFLAGS    = -std=c11 -O2 -Wall -Wextra -Iinclude -DVERSION=\"$(VERSION)\"

# Default values (used if config.mk is missing)
CPPFLAGS  ?= $(shell pkg-config --cflags hdf5)
LDLIBS    ?= $(shell pkg-config --libs hdf5)
PREFIX    ?= /usr/local

# Override with configure-generated values if present
-include config.mk

TARGET    = h5tree

SRC       = src/main.c \
            src/utils.c \
            src/view/tree.c \
            src/view/json.c

OBJ       = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ) config.mk

install: $(TARGET)
	install -d $(PREFIX)/bin
	install -m 0755 $(TARGET) $(PREFIX)/bin/$(TARGET)