# Makefile for treeh5
# Requires: libhdf5-dev (provides pkg-config 'hdf5')

CC       = gcc
CFLAGS   = -std=c11 -O2 -Wall -Wextra
CPPFLAGS = $(shell pkg-config --cflags hdf5)
LDLIBS   = $(shell pkg-config --libs hdf5)

TARGET   = treeh5
SRC      = treeh5.c
OBJ      = treeh5.o

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) $(LDLIBS)

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $(SRC) -o $(OBJ)

clean:
	rm -f $(TARGET) $(OBJ)

install:
	install -m 0755 $(TARGET) /usr/local/bin/$(TARGET)
