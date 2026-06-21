CC = gcc
CFLAGS = -pthread -Wall -g -I../common
COMMON = ../common/common.c
TARGET = ../../build/ss

all: $(TARGET)

$(TARGET): ss.c $(COMMON)
	@mkdir -p ../../build
	$(CC) $(CFLAGS) ss.c $(COMMON) -o $(TARGET)
	@echo "[BUILD] Created $(TARGET)"

clean:
	rm -f $(TARGET)