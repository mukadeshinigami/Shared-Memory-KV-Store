#include "shm_kv.h"

/**
 * Создаёт новый shared memory объект для KV-хранилища
 * 
 * Эта функция создаёт новый объект shared memory, выделяет память,
 * инициализирует семафор и структуры данных.
 * 
 * @param shm_fd_out Указатель для возврата файлового дескриптора shared memory
 *                   (нужен для последующего закрытия/удаления)
 * @return Указатель на структуру shm_kv_store_t в shared memory, или NULL при ошибке
 * 
 * @note После использования необходимо вызвать shm_kv_destroy() для очистки
 * @note Если объект уже существует, функция вернёт NULL (используй shm_kv_open())
 */
shared_memory_kv_store_t* shared_memory_kv_create(int* shared_memory_file_descriptor_out) {
    // Шаг 1: Создание shared memory объекта
    // O_CREAT - создать, если не существует
    // O_EXCL - вернуть ошибку, если уже существует (защита от перезаписи)
    // O_RDWR - режим чтения и записи
    // S_IRUSR | S_IWUSR - права доступа: чтение и запись для владельца
    int shared_memory_file_descriptor = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    
    if (shared_memory_file_descriptor == -1) {
        perror("shm_open failed");
        return NULL;
    }
    
    // Сохраняем дескриптор для возврата
    if (shared_memory_file_descriptor_out != NULL) {
        *shared_memory_file_descriptor_out = shared_memory_file_descriptor;
    }
    
    // TODO: Шаг 2 - ftruncate для установки размера
    // TODO: Шаг 3 - mmap для маппинга памяти
    // TODO: Шаг 4 - sem_init для инициализации семафора
    // TODO: Шаг 5 - Инициализация полей структуры
    
    return NULL; // Временная заглушка
}

