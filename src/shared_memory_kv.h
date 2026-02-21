#ifndef SHM_KV_H
#define SHM_KV_H

// Required header files for shared memory and synchronization
#include <sys/mman.h>      // mmap, munmap, shm_open, shm_unlink
#include <sys/stat.h>      // access modes (S_IRUSR, S_IWUSR, etc.)
#include <fcntl.h>         // O_CREAT, O_RDWR, O_RDONLY
#include <semaphore.h>     // sem_t, sem_init, sem_wait, sem_post, sem_destroy
#include <unistd.h>        // ftruncate, close
#include <time.h>          // time_t, time()
#include <string.h>        // memset, strncpy, strnlen
#include <stdio.h>         // printf, perror
#include <stdlib.h>        // exit, EXIT_SUCCESS, EXIT_FAILURE
#include <signal.h>        // signal, SIGINT

// ============================================================================
// CONSTANTS
// ============================================================================

// Shared memory object name
// Important: must start with "/" for POSIX shared memory
// Will create file in /dev/shm/gitflow_kv_store
#define SHM_NAME "/gitflow_kv_store"

// Maximum number of KV pairs in table
// Fixed size for simplicity of implementation
#define MAX_ENTRIES 10

// Field sizes (fixed for simplicity)
// Why fixed: shared memory requires known size at compile time
#define KEY_SIZE 64
#define VALUE_SIZE 256

// ============================================================================
// DATA STRUCTURES
// ============================================================================

/**
 * Single KV pair structure
 * 
 * Contains key, value and timestamp of last update.
 * All fields have fixed size for shared memory work.
 */
typedef struct {
    char key[KEY_SIZE];        // Key (string, max KEY_SIZE-1 characters + '\0')
    char value[VALUE_SIZE];    // Value (string, max VALUE_SIZE-1 characters + '\0')
    time_t timestamp;          // Last update time (Unix timestamp)
} kv_pair_t;

/**
 * Main shared memory structure
 * 
 * Contains:
 * - KV pairs table (fixed array)
 * - Semaphore for synchronization between processes
 * - Data version for tracking changes
 * - Entry counter for table traversal optimization
 * 
 * Important: size of this structure must be known at compile time!
 */
typedef struct {
    kv_pair_t kv_table[MAX_ENTRIES];  // KV pairs table
    sem_t sem;                         // Semaphore for synchronization (shared between processes)
    unsigned int version;              // Data version (incremented on each change)
    unsigned int entry_count;          // Current number of non-empty entries in table
} shared_memory_kv_store_t; 

// ============================================================================
// FUNCTIONS FOR SHARED MEMORY KV STORE
// ============================================================================

/**
 * Creates a new shared memory object for KV store
 * 
 * @param shared_memory_file_descriptor_out Pointer to return shared memory file descriptor
 * @return Pointer to shared_memory_kv_store_t structure in shared memory, or NULL on error
 */
shared_memory_kv_store_t* shared_memory_kv_create(int* shared_memory_file_descriptor_out);

/**
 * Opens existing shared memory object for KV store
 * 
 * @param shared_memory_file_descriptor_out Pointer to return shared memory file descriptor
 * @return Pointer to shared_memory_kv_store_t structure in shared memory, or NULL on error
 */
shared_memory_kv_store_t* shared_memory_kv_open(int* shared_memory_file_descriptor_out);

/**
 * Destroys shared memory object and frees resources
 * 
 * @param shared_memory_file_descriptor Shared memory file descriptor
 * @param store Pointer to structure in shared memory (for munmap)
 */
void shared_memory_kv_destroy(int shared_memory_file_descriptor, shared_memory_kv_store_t* store);

#endif // SHM_KV_H

