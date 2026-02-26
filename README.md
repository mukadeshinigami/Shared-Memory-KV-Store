# Shared Memory KV Store

Educational project implementing Inter-Process Communication (IPC) using POSIX Shared Memory in C.

## Description

This project implements a key-value store using shared memory for data exchange between processes. The store supports access synchronization through semaphores and data version tracking.

## Technologies

- **Language**: C11
- **IPC mechanism**: POSIX Shared Memory (`shm_open`, `mmap`)
- **Synchronization**: POSIX Semaphores (`sem_init`, `sem_wait`, `sem_post`)
- **Compiler**: GCC

## Project Structure

```
.
├── src/
│   ├── shared_memory_kv.h    # Header file with API
│   ├── shared_memory_kv.c    # Function implementations
│   ├── producer.c            # Producer program (writes data)
│   └── consumer.c            # Consumer program (reads data)
├── build/                     # Directory for compiled files
├── Makefile                   # Build automation
└── README.md
```

## API

### Data Structures

- `kv_pair_t` - structure for a single key-value pair
- `shared_memory_kv_store_t` - main store structure in shared memory

### Functions

**Shared Memory Management:**
- `shared_memory_kv_create()` - creates a new shared memory object
- `shared_memory_kv_open()` - opens an existing shared memory object
- `shared_memory_kv_destroy()` - destroys shared memory object and releases resources
- `shared_memory_kv_unlink()` - unlinks (removes) shared memory object from system

**Key-Value Operations:**
- `shared_memory_kv_set()` - adds or updates a key-value pair
- `shared_memory_kv_get()` - retrieves a value by key
- `shared_memory_kv_delete()` - removes a key-value pair by key

## Limitations

- Maximum number of entries: `MAX_ENTRIES` (10)
- Maximum key size: `KEY_SIZE - 1` (63 characters)
- Maximum value size: `VALUE_SIZE - 1` (255 characters)

## Requirements

- Linux/Unix system with POSIX Shared Memory support
- GCC compiler
- Standard C library (C11)

## Compilation

### Using Makefile (Recommended)

```bash
# Build all components (library, producer, consumer)
make all

# Build individual components
make producer
make consumer
make lib

# Clean build artifacts
make clean

# Rebuild everything
make rebuild
```

### Manual Compilation

```bash
# Create build directory
mkdir -p build

# Compile library
gcc -std=c11 -Wall -Wextra -c src/shared_memory_kv.c -o build/shared_memory_kv.o

# Compile producer
gcc -std=c11 -Wall -Wextra src/producer.c build/shared_memory_kv.o -o build/producer -lrt -lpthread

# Compile consumer
gcc -std=c11 -Wall -Wextra src/consumer.c build/shared_memory_kv.o -o build/consumer -lrt -lpthread
```

## Usage

### Running Producer and Consumer

The project includes two programs that demonstrate IPC using shared memory:

1. **Producer** - writes key-value pairs to shared memory
2. **Consumer** - reads key-value pairs from shared memory

**Step 1: Start Producer** (in first terminal)
```bash
./build/producer
```

The producer will:
- Create shared memory object
- Write 8 test key-value pairs
- Wait for consumer to read data
- Press Ctrl+C to exit

**Step 2: Start Consumer** (in second terminal)
```bash
./build/consumer
```

The consumer will:
- Open existing shared memory object
- Read and display all key-value pairs
- Monitor for updates (version tracking)
- Press Ctrl+C to exit

**Note:** Producer must be started before consumer, as consumer opens an existing shared memory object.

### Programmatic Usage

### Creating a shared memory object

```c
#include "shared_memory_kv.h"

int shm_fd;
shared_memory_kv_store_t *store = shared_memory_kv_create(&shm_fd);

if (store == NULL) {
    // Error handling
    return 1;
}

// Use store...

// Cleanup
shared_memory_kv_destroy(shm_fd, store);
```

### Opening an existing shared memory object


```c
int shm_fd;
shared_memory_kv_store_t *store = shared_memory_kv_open(&shm_fd);

if (store == NULL) {
    // Error handling (object may not exist)
    return 1;
}

// Use store...

// Cleanup
shared_memory_kv_destroy(shm_fd, store);
```

### Working with key-value pairs

```c
// Set (add or update) a key-value pair
if (shared_memory_kv_set(store, "username", "john_doe") == -1) {
    perror("Failed to set key-value pair");
}

// Get a value by key
char value[VALUE_SIZE];
if (shared_memory_kv_get(store, "username", value) == 0) {
    printf("Value: %s\n", value);
} else {
    if (errno == ENOENT) {
        printf("Key not found\n");
    }
}

// Delete a key-value pair
if (shared_memory_kv_delete(store, "username") == -1) {
    perror("Failed to delete key-value pair");
}
```

### Unlinking shared memory object (producer only)

```c
// Should be called only by the creator process (producer)
if (shared_memory_kv_unlink() == -1) {
    perror("Failed to unlink shared memory");
}
```

## Important Notes

1. **Resource cleanup**: Always call `shared_memory_kv_destroy()` after use to properly release resources
2. **Synchronization**: Use semaphore `store->sem` for access synchronization between processes
3. **Shared memory size**: Structure size must be known at compile time
4. **Unlinking**: Only the creator process (producer) should call `shared_memory_kv_unlink()`

## Development Status

- ✅ `shared_memory_kv_create()` - implemented
- ✅ `shared_memory_kv_open()` - implemented
- ✅ `shared_memory_kv_destroy()` - implemented
- ✅ `shared_memory_kv_unlink()` - implemented
- ✅ `shared_memory_kv_set()` - implemented
- ✅ `shared_memory_kv_get()` - implemented
- ✅ `shared_memory_kv_delete()` - implemented
- ✅ `producer.c` - implemented
- ✅ `consumer.c` - implemented
- ✅ `Makefile` - implemented

**Project Status: Complete** ✅

## License

Educational project.
