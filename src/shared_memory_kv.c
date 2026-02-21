#include "shared_memory_kv.h"

/**
 * Creates a new shared memory object for KV store
 * 
 * This function creates a new shared memory object, allocates memory,
 * initializes semaphore and data structures.
 * 
 * @param shm_fd_out Pointer to return shared memory file descriptor
 *                   (needed for subsequent close/removal)
 * @return Pointer to shm_kv_store_t structure in shared memory, or NULL on error
 * 
 * @note After use, must call shm_kv_destroy() for cleanup
 * @note If object already exists, function returns NULL (use shm_kv_open())
 */
shared_memory_kv_store_t* shared_memory_kv_create(int* shared_memory_file_descriptor_out) {
    // Step 1: Create shared memory object
    // O_CREAT - create if doesn't exist
    // O_EXCL - return error if already exists (protection from overwrite)
    // O_RDWR - read and write mode
    // S_IRUSR | S_IWUSR - access rights: read and write for owner
    int shared_memory_file_descriptor = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    
    if (shared_memory_file_descriptor == -1) {
        perror("shm_open failed");
        return NULL;
    }
    
    // Save descriptor for return
    if (shared_memory_file_descriptor_out != NULL) {
        *shared_memory_file_descriptor_out = shared_memory_file_descriptor;
    }
    
    // Step 2: Set size of shared memory object
    // ftruncate sets file/object size
    // Important: shm_open creates object with size 0, need to explicitly set size
    // Size must be equal to sizeof(shared_memory_kv_store_t) - size of our structure
    if (ftruncate(shared_memory_file_descriptor, sizeof(shared_memory_kv_store_t)) == -1) {
        perror("ftruncate failed");
        // If ftruncate failed, need to close descriptor and return error
        close(shared_memory_file_descriptor);
        // Also need to remove object, as it's already created but incomplete
        shm_unlink(SHM_NAME);
        return NULL;
    }
    
    // TODO: Step 3 - mmap for memory mapping
    // TODO: Step 4 - sem_init for semaphore initialization
    // TODO: Step 5 - Initialize structure fields
    
    return NULL; // Temporary stub
}

