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
    printf("Consumer: Shared memory resources released\n");
  }
}

/**
 * Read and display a single key-value pair
 */
int read_and_display(const char *key) {
  char value[VALUE_SIZE];

  if (shared_memory_kv_get(g_store, key, value) == 0) {
    printf("Consumer: Got '%s' = '%s'\n", key, value);
    return 0;
  } else {
    if (errno == ENOENT) {
      printf("Consumer: Key '%s' not found\n", key);
    } else {
      fprintf(stderr, "Consumer: Failed to get '%s'\n", key);
      perror("  Error");
    }
    return -1;
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

  printf("Consumer: Opening shared memory KV store...\n");

  // Step 1: Open existing shared memory object
  // Note: Producer must be running first to create the object
  g_store = shared_memory_kv_open(&g_shm_fd);
  if (g_store == NULL) {
    fprintf(stderr, "Consumer: Failed to open shared memory object\n");
    fprintf(stderr, "Consumer: Make sure producer is running first!\n");
    return EXIT_FAILURE;
  }

  printf("Consumer: Shared memory opened successfully\n");
  printf("Consumer: Store version: %u, Entry count: %u\n\n",
         g_store->version, g_store->entry_count);

  // Step 2: Read key-value pairs in a loop
  // This allows us to see updates if producer modifies data
  const char *keys_to_read[] = {
      "username", "email", "age", "city", "status",
      "score",    "level", "role",
  };
  size_t num_keys = sizeof(keys_to_read) / sizeof(keys_to_read[0]);

  printf("Consumer: Reading key-value pairs...\n");
  printf("Consumer: Press Ctrl+C to exit\n\n");

  unsigned int last_version = 0;

  while (g_running) {
    // Check if data has changed (version tracking)
    if (g_store->version != last_version) {
      printf("\n--- Store updated (version %u -> %u) ---\n", last_version,
             g_store->version);
      last_version = g_store->version;

      // Read all keys
      for (size_t i = 0; i < num_keys; i++) {
        read_and_display(keys_to_read[i]);
      }

      printf("\nConsumer: Waiting for updates... (version: %u, entries: %u)\n",
             g_store->version, g_store->entry_count);
    }

    sleep(1); // Sleep for 1 second before checking again
  }

  printf("Consumer: Exiting...\n");
  return EXIT_SUCCESS;
}

