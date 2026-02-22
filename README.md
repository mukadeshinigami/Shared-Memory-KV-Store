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
│   └── shared_memory_kv.c    # Function implementations
├── build/                     # Directory for compiled files
└── README.md
```

## API

### Data Structures

- `kv_pair_t` - structure for a single key-value pair
- `shared_memory_kv_store_t` - main store structure in shared memory

### Functions

- `shared_memory_kv_create()` - creates a new shared memory object
- `shared_memory_kv_open()` - opens an existing shared memory object
- `shared_memory_kv_destroy()` - destroys shared memory object and releases resources
- `shared_memory_kv_unlink()` - unlinks (removes) shared memory object from system

## Limitations

- Maximum number of entries: `MAX_ENTRIES` (10)
- Maximum key size: `KEY_SIZE - 1` (63 characters)
- Maximum value size: `VALUE_SIZE - 1` (255 characters)

## Requirements

- Linux/Unix system with POSIX Shared Memory support
- GCC compiler
- Standard C library (C11)

## Compilation

```bash
gcc -std=c11 -Wall -Wextra -c src/shared_memory_kv.c -o build/shared_memory_kv.o
```

## Usage

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
- ⏳ Functions for working with KV pairs (get, set, delete) - in development
- ⏳ `producer.c` - in development
- ⏳ `consumer.c` - in development

## License

Educational project.
