# Makefile for Shared Memory KV Store
# Compiler and flags
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra
LDFLAGS = -lrt -lpthread

# Directories
SRC_DIR = src
BUILD_DIR = build

# Source files
LIB_SRC = $(SRC_DIR)/shared_memory_kv.c
PRODUCER_SRC = $(SRC_DIR)/producer.c
CONSUMER_SRC = $(SRC_DIR)/consumer.c

# Object files
LIB_OBJ = $(BUILD_DIR)/shared_memory_kv.o

# Shared library
LIB_SO = $(BUILD_DIR)/libshared_memory_kv.so

# Executables
PRODUCER = $(BUILD_DIR)/producer
CONSUMER = $(BUILD_DIR)/consumer

# Default target
all: $(PRODUCER) $(CONSUMER)

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build library object file
$(LIB_OBJ): $(LIB_SRC) $(SRC_DIR)/shared_memory_kv.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $(LIB_SRC) -o $(LIB_OBJ)

# Build shared library (.so) for Python ctypes
# -fPIC: Position Independent Code (required for shared libraries)
# -shared: Create shared library instead of executable
$(LIB_SO): $(LIB_SRC) $(SRC_DIR)/shared_memory_kv.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -fPIC -shared $(LIB_SRC) -o $(LIB_SO) $(LDFLAGS)

# Build producer executable
$(PRODUCER): $(PRODUCER_SRC) $(LIB_OBJ) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(PRODUCER_SRC) $(LIB_OBJ) -o $(PRODUCER) $(LDFLAGS)

# Build consumer executable
$(CONSUMER): $(CONSUMER_SRC) $(LIB_OBJ) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CONSUMER_SRC) $(LIB_OBJ) -o $(CONSUMER) $(LDFLAGS)

# Individual targets
producer: $(PRODUCER)
consumer: $(CONSUMER)
lib: $(LIB_OBJ)
libso: $(LIB_SO)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Rebuild everything
rebuild: clean all

# Phony targets
.PHONY: all clean rebuild producer consumer lib libso


