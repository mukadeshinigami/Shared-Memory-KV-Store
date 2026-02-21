#ifndef SHM_KV_H
#define SHM_KV_H

// Необходимые заголовочные файлы для работы с shared memory и синхронизацией
#include <sys/mman.h>      // mmap, munmap, shm_open, shm_unlink
#include <sys/stat.h>      // режимы доступа (S_IRUSR, S_IWUSR и т.д.)
#include <fcntl.h>         // O_CREAT, O_RDWR, O_RDONLY
#include <semaphore.h>     // sem_t, sem_init, sem_wait, sem_post, sem_destroy
#include <unistd.h>        // ftruncate, close
#include <time.h>          // time_t, time()
#include <string.h>        // memset, strncpy, strnlen
#include <stdio.h>         // printf, perror
#include <stdlib.h>        // exit, EXIT_SUCCESS, EXIT_FAILURE
#include <signal.h>        // signal, SIGINT

// ============================================================================
// КОНСТАНТЫ
// ============================================================================

// Имя shared memory объекта
// Важно: должно начинаться с "/" для POSIX shared memory
// Будет создан файл в /dev/shm/gitflow_kv_store
#define SHM_NAME "/gitflow_kv_store"

// Максимальное количество KV-пар в таблице
// Фиксированный размер для простоты реализации
#define MAX_ENTRIES 10

// Размеры полей (фиксированные для простоты)
// Почему фиксированные: shared memory требует известного размера на этапе компиляции
#define KEY_SIZE 64
#define VALUE_SIZE 256

// ============================================================================
// СТРУКТУРЫ ДАННЫХ
// ============================================================================

/**
 * Структура одной KV-пары
 * 
 * Содержит ключ, значение и метку времени последнего обновления.
 * Все поля имеют фиксированный размер для работы в shared memory.
 */
typedef struct {
    char key[KEY_SIZE];        // Ключ (строка, максимум KEY_SIZE-1 символов + '\0')
    char value[VALUE_SIZE];    // Значение (строка, максимум VALUE_SIZE-1 символов + '\0')
    time_t timestamp;          // Время последнего обновления (Unix timestamp)
} kv_pair_t;

/**
 * Главная структура shared memory
 * 
 * Содержит:
 * - Таблицу KV-пар (фиксированный массив)
 * - Семафор для синхронизации доступа между процессами
 * - Версию данных для отслеживания изменений
 * - Счётчик записей для оптимизации обхода таблицы
 * 
 * Важно: размер этой структуры должен быть известен на этапе компиляции!
 */
typedef struct {
    kv_pair_t kv_table[MAX_ENTRIES];  // Таблица KV-пар
    sem_t sem;                         // Семафор для синхронизации (shared между процессами)
    unsigned int version;              // Версия данных (инкрементируется при каждом изменении)
    unsigned int entry_count;          // Текущее количество непустых записей в таблице
} shared_memory_kv_store_t;

// ============================================================================
// ФУНКЦИИ ДЛЯ РАБОТЫ С SHARED MEMORY KV STORE
// ============================================================================

/**
 * Создаёт новый shared memory объект для KV-хранилища
 * 
 * @param shared_memory_file_descriptor_out Указатель для возврата файлового дескриптора shared memory
 * @return Указатель на структуру shared_memory_kv_store_t в shared memory, или NULL при ошибке
 */
shared_memory_kv_store_t* shared_memory_kv_create(int* shared_memory_file_descriptor_out);

/**
 * Открывает существующий shared memory объект для KV-хранилища
 * 
 * @param shared_memory_file_descriptor_out Указатель для возврата файлового дескриптора shared memory
 * @return Указатель на структуру shared_memory_kv_store_t в shared memory, или NULL при ошибке
 */
shared_memory_kv_store_t* shared_memory_kv_open(int* shared_memory_file_descriptor_out);

/**
 * Уничтожает shared memory объект и освобождает ресурсы
 * 
 * @param shared_memory_file_descriptor Файловый дескриптор shared memory
 * @param store Указатель на структуру в shared memory (для munmap)
 */
void shared_memory_kv_destroy(int shared_memory_file_descriptor, shared_memory_kv_store_t* store);

#endif // SHM_KV_H

