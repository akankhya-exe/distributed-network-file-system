CC = gcc
CFLAGS = -pthread -Wall -g -I../common
COMMON = ../common/common.c

BUILD_DIR = ../../build
TARGET = $(BUILD_DIR)/nm

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): nm.c $(COMMON) | $(BUILD_DIR)
	$(CC) $(CFLAGS) nm.c $(COMMON) -o $(TARGET)
	@echo "[BUILD] Created $(TARGET)"

clean:
	rm -f $(TARGET)