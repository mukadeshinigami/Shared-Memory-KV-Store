#include "shared_memory_kv.h"

/**
 * Создает новый объект общей памяти для KV-хранилища
 *
 * Эта функция создает новый объект общей памяти, выделяет память,
 * инициализирует семафор и структуры данных.
 *
 * @param shared_memory_file_descriptor_out Указатель для возврата дескриптора
 * файла общей памяти (выходной параметр) (необходим для последующего
 * закрытия/удаления)
 * @return Указатель на структуру shared_memory_kv_store_t в общей памяти, или
 * NULL в случае ошибки
 *
 * @note После использования необходимо вызвать shared_memory_kv_destroy() для
 * очистки ресурсов
 * @note Если объект уже существует, функция возвращает NULL (используйте
 * shared_memory_kv_open())
 */
shared_memory_kv_store_t *
shared_memory_kv_create(int *shared_memory_file_descriptor_out) {
  // Шаг 1: Создание объекта общей памяти
  // O_CREAT - создать, если не существует
  // O_EXCL - вернуть ошибку, если уже существует (защита от перезаписи)
  // O_RDWR - режим чтения и записи
  // S_IRUSR | S_IWUSR - права доступа: чтение и запись для владельца
  int shared_memory_file_descriptor =
      shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);

  if (shared_memory_file_descriptor == -1) {
    perror("shm_open failed");
    return NULL;
  }

  // Сохраняем дескриптор для возврата
  if (shared_memory_file_descriptor_out != NULL) {
    *shared_memory_file_descriptor_out = shared_memory_file_descriptor;
  }

  // Шаг 2: Установка размера объекта общей памяти
  // ftruncate устанавливает размер файла/объекта
  // Важно: shm_open создает объект размером 0, поэтому размер должен быть
  // установлен явно. Размер должен быть равен sizeof(shared_memory_kv_store_t)
  if (ftruncate(shared_memory_file_descriptor,
                sizeof(shared_memory_kv_store_t)) == -1) {
    perror("ftruncate failed");
    // Если ftruncate не удался, закрываем дескриптор и возвращаем ошибку
    close(shared_memory_file_descriptor);
    // Также нужно удалить объект (unlink), так как он был создан, но не
    // завершен
    shm_unlink(SHM_NAME);
    return NULL;
  }

  // Шаг 3: Отображение общей памяти в адресное пространство процесса
  // mmap создает виртуальное отображение объекта общей памяти в память
  // процесса. После mmap мы можем работать с общей памятью через указатель, как
  // с обычной структурой.
  //
  // Параметры:
  // - NULL: ядро само выберет адрес отображения
  // - sizeof(shared_memory_kv_store_t): размер отображения (должен совпадать с
  //   размером ftruncate)
  // - PROT_READ | PROT_WRITE: разрешить чтение и запись
  // - MAP_SHARED: изменения видны другим процессам (важно для IPC!)
  // - shared_memory_file_descriptor: дескриптор от shm_open
  // - 0: смещение от начала объекта (отображаем с самого начала)
  shared_memory_kv_store_t *store =
      mmap(NULL,                             // адрес (NULL = ядро само выберет)
           sizeof(shared_memory_kv_store_t), // размер
           PROT_READ | PROT_WRITE,           // права доступа
           MAP_SHARED,                       // флаги (общая для процессов)
           shared_memory_file_descriptor,    // файловый дескриптор
           0                                 // смещение
      );

  // Проверка ошибок: mmap возвращает MAP_FAILED (обычно (void*)-1) в случае
  // ошибки
  if (store == MAP_FAILED) {
    perror("mmap failed");

    close(shared_memory_file_descriptor);
    shm_unlink(SHM_NAME);
    return NULL;
  }

  // Шаг 4: Инициализация полей структуры
  // После mmap память может содержать "мусор", поэтому все поля должны быть
  // явно инициализированы.
  //
  // memset очищает память: устанавливает все байты в 0.
  // Это гарантирует, что:
  // - Все строки в kv_table будут начинаться с '\0' (пустые)
  // - Все временные метки будут равны 0
  // - version и entry_count будут равны 0
  memset(store, 0, sizeof(shared_memory_kv_store_t));

  // Явная инициализация полей для ясности (хотя memset уже очистил их).
  // Это делает код более читаемым и явно показывает начальные значения.
  store->version = 0;     // Начальная версия данных
  store->entry_count = 0; // Начальное количество записей (таблица пуста)

  // Шаг 5: Инициализация семафора для синхронизации
  // sem_init инициализирует семафор для межпроцессной синхронизации.
  // Важно: делайте это ПОСЛЕ memset, чтобы семафор инициализировался в чистой
  // памяти.
  //
  // Параметры:
  // - &store->sem: указатель на семафор в структуре (в общей памяти)
  // - 1 (pshared): флаг "shared" - семафор будет доступен между процессами
  //   (0 = только текущий процесс, 1 = между процессами через общую память)
  // - 1 (value): начальное значение семафора (1 = семафор свободен, можно
  // занять)
  //   Используется как мьютекс: 1 = разблокирован, 0 = заблокирован
  if (sem_init(&store->sem, 1, 1) == -1) {
    perror("sem_init failed");
    munmap(store, sizeof(shared_memory_kv_store_t));
    close(shared_memory_file_descriptor);
    shm_unlink(SHM_NAME);
    return NULL;
  }

  return store; // Возвращаем указатель на структуру в общей памяти
}

/**
 * Открывает существующий объект общей памяти для KV-хранилища
 *
 *SHM_NAME - имя объекта общей памяти
 *O_RDWR - режим чтения и записи
 *S_IRUSR | S_IWUSR - права доступа: чтение и запись для владельца
 *MAP_SHARED - флаги (общая для процессов)
 *shared_memory_file_descriptor - файловый дескриптор
 *0 - смещение от начала объекта (отображаем с самого начала)

 * @param shared_memory_file_descriptor_out Указатель для возврата дескриптора
 * файла общей памяти
 * @return Указатель на структуру shared_memory_kv_store_t в общей памяти, или
 * NULL на случай ошибки
 */
shared_memory_kv_store_t * shared_memory_kv_open(int *shared_memory_file_descriptor_out) {
  int shared_memory_file_descriptor = shm_open(SHM_NAME, O_RDWR, 0);

  if (shared_memory_file_descriptor == -1) {
    perror("shm_open failed");
    return NULL;  
  }

  if (shared_memory_file_descriptor_out != NULL) {
    *shared_memory_file_descriptor_out = shared_memory_file_descriptor;
  }

  shared_memory_kv_store_t *store = mmap(NULL, sizeof(shared_memory_kv_store_t), PROT_READ | PROT_WRITE, MAP_SHARED, shared_memory_file_descriptor, 0);
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
void shared_memory_kv_destroy(int shared_memory_file_descriptor, shared_memory_kv_store_t *store) {
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
    // - Semaphore is in shared memory and will be destroyed when object is unlinked
    // - Multiple processes should not destroy the same semaphore
    //
    // Note: We do NOT call shm_unlink() here because:
    // - Only the creator (producer) should unlink the object
    // - shm_unlink() should be called separately by producer when shutting down
}
    