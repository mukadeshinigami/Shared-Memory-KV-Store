#ifndef SHM_KV_H
#define SHM_KV_H

// Define POSIX feature test macros for POSIX functions
// This enables strnlen, ftruncate, and other POSIX functions
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

// Required header files for shared memory and synchronization
#include <errno.h>     // errno
#include <fcntl.h>     // O_CREAT, O_RDWR, O_RDONLY
#include <semaphore.h> // sem_t, sem_init, sem_wait, sem_post, sem_destroy
#include <signal.h>    // signal, SIGINT
#include <stdio.h>     // printf, perror
#include <stdlib.h>    // exit, EXIT_SUCCESS, EXIT_FAILURE
#include <string.h>    // memset, strncpy, strnlen
#include <sys/mman.h>  // mmap, munmap, shm_open, shm_unlink
#include <sys/stat.h>  // Access modes (S_IRUSR, S_IWUSR, etc.)
#include <time.h>      // time_t, time()
#include <unistd.h>    // ftruncate, close

// ============================================================================
// CONSTANTS
// ============================================================================

// Shared memory object name
// Important: must start with "/" for POSIX shared memory
// A file will be created in /dev/shm/gitflow_kv_store
#define SHM_NAME "/gitflow_kv_store"

// Maximum number of KV pairs in the table
// Fixed size for implementation simplicity
#define MAX_ENTRIES 10

// Field sizes (fixed for simplicity)
// Why fixed: shared memory requires a known size at compile time
#define KEY_SIZE 64
#define VALUE_SIZE 256

// ============================================================================
// DATA STRUCTURES
// ============================================================================

/**
 * Structure for a single KV pair
 *
 * Contains key, value, and last update timestamp.
 * All fields have fixed sizes for shared memory operation.
 */
typedef struct {
  char key[KEY_SIZE];     // Key (string, max KEY_SIZE-1 characters + '\0')
  char value[VALUE_SIZE]; // Value (string, max VALUE_SIZE-1 characters + '\0')
  time_t timestamp;       // Last update time (Unix timestamp)
} kv_pair_t;

/**
 * Main shared memory structure
 *
 * Contains:
 * - KV pair table (fixed array)
 * - Semaphore for inter-process synchronization
 * - Data version for tracking changes
 * - Entry counter for table traversal optimization
 *
 * Important: the size of this structure must be known at compile time!
 */
typedef struct {
  kv_pair_t kv_table[MAX_ENTRIES]; // KV pair table
  sem_t sem; // Semaphore for synchronization (shared between processes)
  unsigned int version;     // Data version (incremented on every change)
  unsigned int entry_count; // Current number of non-empty entries in the table
} shared_memory_kv_store_t;

// ============================================================================
// FUNCTIONS FOR SHARED MEMORY KV STORE
// ============================================================================

/**
 * Creates a new shared memory object for the KV store
 *
 * @param shared_memory_file_descriptor_out Pointer to return the shared memory
 * file descriptor
 * @return Pointer to shared_memory_kv_store_t structure in shared memory, or
 * NULL on error
 */
shared_memory_kv_store_t *
shared_memory_kv_create(int *shared_memory_file_descriptor_out);

/**
 * Opens an existing shared memory object for the KV store
 *
 * @param shared_memory_file_descriptor_out Pointer to return the shared memory
 * file descriptor
 * @return Pointer to shared_memory_kv_store_t structure in shared memory, or
 * NULL on error
 */
shared_memory_kv_store_t *
shared_memory_kv_open(int *shared_memory_file_descriptor_out);

/**
 * Destroys the shared memory object and releases resources
 *
 * @param shared_memory_file_descriptor Shared memory file descriptor
 * @param store Pointer to the structure in shared memory (for munmap)
 */
void shared_memory_kv_destroy(int shared_memory_file_descriptor,
                              shared_memory_kv_store_t *store);

/**
 * Unlinks (removes) the shared memory object from the system
 *
 * This should be called ONLY by the creator process (producer)
 * when shutting down. After unlinking, the object will be removed
 * when all processes close their references to it.
 *
 * @return 0 on success, -1 on error
 * @note This function should be called after all processes have
 * called shared_memory_kv_destroy()
 */
int shared_memory_kv_unlink(void);

/**
 * Sets (adds or updates) a key-value pair in the store
 * 
 * @param store Pointer to shared memory KV store
 * @param key Key string (max KEY_SIZE-1 characters)
 * @param value Value string (max VALUE_SIZE-1 characters)
 * @return 0 on success, -1 on error (errno set: EINVAL for invalid params, 
 *         ENOSPC if table is full, ENAMETOOLONG if key/value too long)
 */
int shared_memory_kv_set(shared_memory_kv_store_t *store, const char *key,
                         const char *value);

/**
 * Gets a value from the store
 * 
 * @param store Pointer to shared memory KV store
 * @param key Key string (max KEY_SIZE-1 characters)
 * @param value_out Pointer to return the value (max VALUE_SIZE-1 characters)
 * @return 0 on success, -1 on error (errno set: EINVAL for invalid params, 
 *         ENOSPC if table is full, ENAMETOOLONG if key/value too long)
 */
int shared_memory_kv_get(shared_memory_kv_store_t *store, const char *key,
                     char *value_out);
                        
/**
 * Deletes a key-value pair from the store
 * 
 * @param store Pointer to shared memory KV store
 * @param key Key string (max KEY_SIZE-1 characters)
 * @return 0 on success, -1 on error (errno set: EINVAL for invalid params, 
 *         ENOSPC if table is full, ENAMETOOLONG if key/value too long)
 */
int shared_memory_kv_delete(shared_memory_kv_store_t *store, const char *key);


#endif // SHM_KV_H
