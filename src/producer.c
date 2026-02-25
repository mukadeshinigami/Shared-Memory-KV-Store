#include "shared_memory_kv.h"

// Global variables for cleanup
static shared_memory_kv_store_t *g_store = NULL;
static int g_shm_fd = -1;
static volatile int g_running = 1; // Flag to control main loop

/**
 * Signal handler for SIGINT (Ctrl+C)
 * 
 * Sets the running flag to 0 to exit the main loop gracefully
 */
void signal_handler(int sig) {
  (void)sig; // Suppress unused parameter warning
  printf("\nReceived SIGINT, shutting down...\n");
  g_running = 0;
}

/**
 * Cleanup function to release shared memory resources
 */
void cleanup(void) {
  if (g_store != NULL) {
    shared_memory_kv_destroy(g_shm_fd, g_store);
    g_store = NULL;
  }

  // Unlink shared memory object (only producer should do this)
  if (shared_memory_kv_unlink() == -1) {
    perror("Failed to unlink shared memory");
  } else {
    printf("Shared memory object unlinked\n");
  }
}

int main(void) {
  // Register signal handler for graceful shutdown
  if (signal(SIGINT, signal_handler) == SIG_ERR) {
    perror("Failed to register signal handler");
    return EXIT_FAILURE;
  }

  // Register cleanup function to be called on exit
  if (atexit(cleanup) != 0) {
    perror("Failed to register cleanup function");
    return EXIT_FAILURE;
  }

  printf("Producer: Creating shared memory KV store...\n");

  // Step 1: Create shared memory object
  g_store = shared_memory_kv_create(&g_shm_fd);
  if (g_store == NULL) {
    fprintf(stderr, "Failed to create shared memory object\n");
    return EXIT_FAILURE;
  }

  printf("Producer: Shared memory created successfully\n");
  printf("Producer: Writing key-value pairs...\n\n");

  // Step 2: Write some key-value pairs to the store
  struct {
    const char *key;
    const char *value;
  } test_data[] = {
      {"username", "john_doe"},
      {"email", "john@example.com"},
      {"age", "25"},
      {"city", "New York"},
      {"status", "active"},
      {"score", "100"},
      {"level", "5"},
      {"role", "admin"},
  };

  size_t num_pairs = sizeof(test_data) / sizeof(test_data[0]);

  for (size_t i = 0; i < num_pairs; i++) {
    if (shared_memory_kv_set(g_store, test_data[i].key, test_data[i].value) ==
        0) {
      printf("Producer: Set '%s' = '%s'\n", test_data[i].key,
             test_data[i].value);
    } else {
      fprintf(stderr, "Producer: Failed to set '%s' = '%s'\n",
              test_data[i].key, test_data[i].value);
      perror("  Error");
    }
  }

  printf("\nProducer: All pairs written. Store version: %u, Entry count: %u\n",
         g_store->version, g_store->entry_count);
  printf("Producer: Waiting for consumer to read data...\n");
  printf("Producer: Press Ctrl+C to exit\n\n");

  // Step 3: Keep running until SIGINT is received
  // This allows consumer to read the data
  while (g_running) {
    sleep(1); // Sleep for 1 second
  }

  printf("Producer: Exiting...\n");
  return EXIT_SUCCESS;
}

