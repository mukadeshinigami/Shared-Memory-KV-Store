#include "shared_memory_kv.h"

/**
 * Creates a new shared memory object for the KV store
 *
 * This function creates a new shared memory object, allocates memory,
 * initializes the semaphore, and initializes the data structures.
 *
 * @param shared_memory_file_descriptor_out Pointer to return the shared memory
 * file descriptor (output parameter) (required for later closing/deletion)
 * @return Pointer to shared_memory_kv_store_t structure in shared memory, or
 * NULL on error
 *
 * @note shared_memory_kv_destroy() must be called after use for cleanup
 * @note If the object already exists, the function returns NULL (use
 * shared_memory_kv_open())
 */
shared_memory_kv_store_t *
shared_memory_kv_create(int *shared_memory_file_descriptor_out) {
  // Step 1: Create the shared memory object
  // O_CREAT - create if it doesn't exist
  // O_EXCL - return error if already exists (overwrite protection)
  // O_RDWR - read and write mode
  // S_IRUSR | S_IWUSR - permissions: read and write for the owner
  int shared_memory_file_descriptor =
      shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);

  if (shared_memory_file_descriptor == -1) {
    perror("shm_open failed");
    return NULL;
  }

  // Save the descriptor to return it
  if (shared_memory_file_descriptor_out != NULL) {
    *shared_memory_file_descriptor_out = shared_memory_file_descriptor;
  }

  // Step 2: Set the shared memory object size
  // ftruncate sets the size of the file/object
  // Important: shm_open creates an object with size 0, so size must be
  // explicitly set. Size should be equal to sizeof(shared_memory_kv_store_t)
  if (ftruncate(shared_memory_file_descriptor,
                sizeof(shared_memory_kv_store_t)) == -1) {
    perror("ftruncate failed");
    // If ftruncate fails, close the descriptor and return an error
    close(shared_memory_file_descriptor);
    // Also need to unlink the object as it was created but is incomplete
    shm_unlink(SHM_NAME);
    return NULL;
  }

  // Step 3: Map shared memory into the process address space
  // mmap creates a virtual mapping of the shared memory object into process
  // memory. After mmap, we can work with shared memory via a pointer like a
  // regular structure.
  //
  // Parameters:
  // - NULL: the kernel will choose the mapping address
  // - sizeof(shared_memory_kv_store_t): mapping size (must match ftruncate
  //   size)
  // - PROT_READ | PROT_WRITE: allow read and write
  // - MAP_SHARED: changes are visible to other processes (important for IPC!)
  // - shared_memory_file_descriptor: descriptor from shm_open
  // - 0: offset from the start of the object (map from the beginning)
  shared_memory_kv_store_t *store =
      mmap(NULL, // address (NULL = kernel will choose)
           sizeof(shared_memory_kv_store_t), // size
           PROT_READ | PROT_WRITE,           // access rights
           MAP_SHARED,                       // flags (shared between processes)
           shared_memory_file_descriptor,    // file descriptor
           0                                 // offset
      );

  // Error check: mmap returns MAP_FAILED (usually (void*)-1) on error
  if (store == MAP_FAILED) {
    perror("mmap failed");

    close(shared_memory_file_descriptor);
    shm_unlink(SHM_NAME);
    return NULL;
  }

  // Step 4: Initialize structure fields
  // After mmap, memory may contain garbage, so all fields must be explicitly
  // initialized.
  //
  // memset clears memory: sets all bytes to 0.
  // This ensures that:
  // - All strings in kv_table will start with '\0' (empty)
  // - All timestamps will be 0
  // - version and entry_count will be 0
  memset(store, 0, sizeof(shared_memory_kv_store_t));

  // Explicit field initialization for clarity (though memset already cleared
  // them). This makes the code more readable and explicitly shows initial
  // values.
  store->version = 0;     // Initial data version
  store->entry_count = 0; // Initial entry count (table is empty)

  // Step 5: Initialize the semaphore for synchronization
  // sem_init initializes the semaphore for inter-process synchronization.
  // Important: do this AFTER memset so the semaphore is initialized in clean
  // memory.
  //
  // Parameters:
  // - &store->sem: pointer to the semaphore in the structure (in shared memory)
  // - 1 (pshared): "shared" flag - semaphore will be accessible between
  // processes
  //   (0 = current process only, 1 = between processes via shared memory)
  // - 1 (value): initial semaphore value (1 = semaphore is free, can be taken)
  //   Used as a mutex: 1 = unlocked, 0 = locked
  if (sem_init(&store->sem, 1, 1) == -1) {
    perror("sem_init failed");
    munmap(store, sizeof(shared_memory_kv_store_t));
    close(shared_memory_file_descriptor);
    shm_unlink(SHM_NAME);
    return NULL;
  }

  return store; // Return pointer to the structure in shared memory
}

/**
 * Opens an existing shared memory object for the KV store
 *
 * @param shared_memory_file_descriptor_out Pointer to return the shared memory
 * file descriptor
 * @return Pointer to shared_memory_kv_store_t structure in shared memory, or
 * NULL on error
 */
shared_memory_kv_store_t *
shared_memory_kv_open(int *shared_memory_file_descriptor_out) {
  // Step 1: Open existing shared memory object
  // O_RDWR - read and write mode
  int shared_memory_file_descriptor = shm_open(SHM_NAME, O_RDWR, 0);

  if (shared_memory_file_descriptor == -1) {
    perror("shm_open failed");
    return NULL;
  }

  // Save descriptor if requested
  if (shared_memory_file_descriptor_out != NULL) {
    *shared_memory_file_descriptor_out = shared_memory_file_descriptor;
  }

  // Step 2: Map shared memory into process address space
  shared_memory_kv_store_t *store =
      mmap(NULL, sizeof(shared_memory_kv_store_t), PROT_READ | PROT_WRITE,
           MAP_SHARED, shared_memory_file_descriptor, 0);
  if (store == MAP_FAILED) {
    perror("mmap failed");
    close(shared_memory_file_descriptor);
    return NULL;
  }

  return store;
}

/**
 * Destroys the shared memory object and frees resources
 *
 * @param shared_memory_file_descriptor File descriptor of shared memory
 * @param store Pointer to mapped shared memory region
 */
void shared_memory_kv_destroy(int shared_memory_file_descriptor,
                              shared_memory_kv_store_t *store) {
  // Step 1: Unmap shared memory from process address space
  // Check for NULL pointer before using it
  if (store != NULL) {
    if (munmap(store, sizeof(shared_memory_kv_store_t)) == -1) {
      perror("munmap failed");
      // Continue cleanup even if munmap fails
    }
  }

  // Step 2: Close file descriptor
  // Check for invalid descriptor before using it
  if (shared_memory_file_descriptor != -1) {
    if (close(shared_memory_file_descriptor) == -1) {
      perror("close failed");
    }
  }

  // Note: We do NOT call sem_destroy() here because:
  // - Semaphore is in shared memory and will be destroyed when object is
  // unlinked
  // - Multiple processes should not destroy the same semaphore
  //
  // Note: We do NOT call shm_unlink() here because:
  // - Only the creator (producer) should unlink the object
  // - shm_unlink() should be called separately by producer when shutting down
}

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
int shared_memory_kv_unlink(void) {
  if (shm_unlink(SHM_NAME) == -1) {
    if (errno == ENOENT) {
      return 0;
    }
    perror("shm_unlink failed");
    return -1;
  }
  return 0;
}

/**
 * Sets (adds or updates) a key-value pair in the store
 *
 * @param store Pointer to shared memory KV store
 * @param key Key string (max KEY_SIZE-1 characters)
 * @param value Value string (max VALUE_SIZE-1 characters)
 * @return 0 on success, -1 on error
 */
int shared_memory_kv_set(shared_memory_kv_store_t *store, const char *key,
                         const char *value) {
  // Step 1: Validate input parameters
  if (store == NULL || key == NULL || value == NULL) {
    errno = EINVAL;
    return -1;
  }

  // Step 2: Check key and value lengths
  // strnlen returns the length of the string, but not more than the limit
  // If length >= KEY_SIZE, it means the string is too long (no space for '\0')
  size_t key_len = strnlen(key, KEY_SIZE);
  size_t value_len = strnlen(value, VALUE_SIZE);

  if (key_len >= KEY_SIZE) {
    errno = ENAMETOOLONG;
    return -1;
  }

  if (value_len >= VALUE_SIZE) {
    errno = ENAMETOOLONG;
    return -1;
  }

  // Step 3: Lock semaphore for exclusive access
  // sem_wait decrements the semaphore value (blocks if value is 0)
  // This ensures only one process can modify the store at a time
  // Critical for IPC: without this, we could have race conditions
  if (sem_wait(&store->sem) == -1) {
    perror("sem_wait failed");
    return -1;
  }

  // Step 4: Search for existing key in the table
  // We need to find if the key already exists to update it,
  // or find a free slot to add a new entry
  int found_index_key = -1;  // Index of found key, -1 if not found
  int found_index_free = -1;    // Index of first free slot, -1 if table is full

  for (int i = 0; i < MAX_ENTRIES; i++) {
    // Check if this slot has the key we're looking for
    // strncmp compares strings, returns 0 if they match
    if (store->kv_table[i].key[0] != '\0' &&
        strncmp(store->kv_table[i].key, key, KEY_SIZE) == 0) {
      found_index_key = i;
      break; // Found existing key, no need to continue searching
    }

    // Track first free slot (empty key means slot is free)
    if (found_index_free == -1 && store->kv_table[i].key[0] == '\0') {
      found_index_free = i;
    }
  }

  // Step 5: Determine which slot to use
  int target_index;
  int is_new_entry = 0; // Flag: 1 if adding new entry, 0 if updating existing

  if (found_index_key != -1) {
    // Key exists, update existing entry
    target_index = found_index_key;
    is_new_entry = 0;
  } else if (found_index_free != -1) {
    // Key doesn't exist, but we have a free slot
    target_index = found_index_free;
    is_new_entry = 1;
  } else {
    // Key doesn't exist AND table is full
    // Unlock semaphore before returning error
    sem_post(&store->sem);
    errno = ENOSPC;
    return -1;
  }

  strncpy(store->kv_table[target_index].key, key, KEY_SIZE - 1);
  store->kv_table[target_index].key[KEY_SIZE - 1] = '\0';

  strncpy(store->kv_table[target_index].value, value, VALUE_SIZE - 1) ;
  store->kv_table[target_index].value[VALUE_SIZE - 1] = '\0';

  store->kv_table[target_index].timestamp = time(NULL);

  // Step 6: Update entry count and version
  store->version++;

  if (is_new_entry) {
    store->entry_count++;
  }

  // Step 7: Unlock semaphore
  if (sem_post(&store->sem) == -1) {
    perror("sem_post failed");
  }
  

  return 0;
}