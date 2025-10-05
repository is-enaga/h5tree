# Requires: libhdf5-dev (provides pkg-config 'hdf5')

CC       = gcc
CFLAGS   = -std=c11 -O2 -Wall -Wextra -Iinclude
CPPFLAGS = $(shell pkg-config --cflags hdf5)
LDLIBS   = $(shell pkg-config --libs hdf5)

TARGET   = h5tree

SRC      = src/main.c \
           src/utils.c \
           src/view/tree.c \
           src/view/json.c

OBJ      = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)

install: $(TARGET)
	install -m 0755 $(TARGET) /usr/local/bin/$(TARGET)
