CC = gcc
CFLAGS = -pthread -Wall -g -I../common
COMMON = ../common/common.c

BUILD_DIR = ../../build
TARGET = $(BUILD_DIR)/client

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): client.c $(COMMON) | $(BUILD_DIR)
	$(CC) $(CFLAGS) client.c $(COMMON) -o $(TARGET)
	@echo "[BUILD] Created $(TARGET)"

clean:
	rm -f $(TARGET)