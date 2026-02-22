#ifndef SHM_KV_H
#define SHM_KV_H

// Необходимые заголовочные файлы для работы с общей памятью и синхронизацией
#include <errno.h>     // errno
#include <fcntl.h>     // O_CREAT, O_RDWR, O_RDONLY
#include <semaphore.h> // sem_t, sem_init, sem_wait, sem_post, sem_destroy
#include <signal.h>    // signal, SIGINT
#include <stdio.h>     // printf, perror
#include <stdlib.h>    // exit, EXIT_SUCCESS, EXIT_FAILURE
#include <string.h>    // memset, strncpy, strnlen
#include <sys/mman.h>  // mmap, munmap, shm_open, shm_unlink
#include <sys/stat.h>  // Режимы доступа (S_IRUSR, S_IWUSR и т.д.)
#include <time.h>      // time_t, time()
#include <unistd.h>    // ftruncate, close

// ============================================================================
// КОНСТАНТЫ
// ============================================================================

// Имя объекта общей памяти
// Важно: должно начинаться с "/" для POSIX shared memory
// Файл будет создан в /dev/shm/gitflow_kv_store
#define SHM_NAME "/gitflow_kv_store"

// Максимальное количество пар ключ-значение в таблице
// Фиксированный размер для упрощения реализации
#define MAX_ENTRIES 10

// Размеры полей (фиксированные для простоты)
// Почему фиксированные: общая память требует известного размера во время
// компиляции
#define KEY_SIZE 64
#define VALUE_SIZE 256

// ============================================================================
// СТРУКТУРЫ ДАННЫХ
// ============================================================================

/**
 * Структура для одной пары ключ-значение
 *
 * Содержит ключ, значение и временную метку последнего обновления.
 * Все поля имеют фиксированный размер для работы в общей памяти.
 */
typedef struct {
  char key[KEY_SIZE];     // Ключ (строка, макс. KEY_SIZE-1 символов + '\0')
  char value[VALUE_SIZE]; // Значение (строка, макс. VALUE_SIZE-1 символов +
                          // '\0')
  time_t timestamp; // Время последнего обновления (Unix timestamp)
} kv_pair_t;

/**
 * Основная структура в общей памяти
 *
 * Содержит:
 * - Таблицу пар ключ-значение (фиксированный массив)
 * - Семафор для межпроцессной синхронизации
 * - Версию данных для отслеживания изменений
 * - Счетчик записей для оптимизации обхода таблицы
 *
 * Важно: размер этой структуры должен быть известен во время компиляции!
 */
typedef struct {
  kv_pair_t kv_table[MAX_ENTRIES]; // Таблица пар ключ-значение
  sem_t sem;            // Семафор для синхронизации (общий для процессов)
  unsigned int version; // Версия данных (инкрементируется при каждом изменении)
  unsigned int entry_count; // Текущее количество непустых записей в таблице
} shared_memory_kv_store_t;

// ============================================================================
// ФУНКЦИИ ДЛЯ РАБОТЫ С KV-ХРАНИЛИЩЕМ В ОБЩЕЙ ПАМЯТИ
// ============================================================================

/**
 * Создает новый объект общей памяти для KV-хранилища
 *
 * @param shared_memory_file_descriptor_out Указатель для возврата дескриптора
 * файла общей памяти
 * @return Указатель на структуру shared_memory_kv_store_t в общей памяти, или
 * NULL на случай ошибки
 */
shared_memory_kv_store_t *
shared_memory_kv_create(int *shared_memory_file_descriptor_out);

/**
 * Открывает существующий объект общей памяти для KV-хранилища
 *
 * @param shared_memory_file_descriptor_out Указатель для возврата дескриптора
 * файла общей памяти
 * @return Указатель на структуру shared_memory_kv_store_t в общей памяти, или
 * NULL на случай ошибки
 */
shared_memory_kv_store_t *
shared_memory_kv_open(int *shared_memory_file_descriptor_out);

/**
 * Уничтожает объект общей памяти и освобождает ресурсы
 *
 * @param shared_memory_file_descriptor Дескриптор файла общей памяти
 * @param store Указатель на структуру в общей памяти (для munmap)
 */
void shared_memory_kv_destroy(int shared_memory_file_descriptor,
                              shared_memory_kv_store_t *store);

#endif // SHM_KV_H
