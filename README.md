# Shared Memory KV Store

Учебный проект по реализации межпроцессного взаимодействия (IPC) через POSIX Shared Memory на языке C.

## Описание

Проект реализует key-value хранилище, использующее разделяемую память (shared memory) для обмена данными между процессами. Хранилище поддерживает синхронизацию доступа через семафоры и отслеживание версий данных.

## Технологии

- **Язык**: C11
- **IPC механизм**: POSIX Shared Memory (`shm_open`, `mmap`)
- **Синхронизация**: POSIX Semaphores (`sem_init`, `sem_wait`, `sem_post`)
- **Компилятор**: GCC

## Структура проекта

```
.
├── src/
│   ├── shared_memory_kv.h    # Заголовочный файл с API
│   └── shared_memory_kv.c    # Реализация функций
├── build/                     # Директория для скомпилированных файлов
└── README.md
```

## API

### Структуры данных

- `kv_pair_t` - структура для одной пары ключ-значение
- `shared_memory_kv_store_t` - основная структура хранилища в shared memory

### Функции

- `shared_memory_kv_create()` - создание нового shared memory объекта
- `shared_memory_kv_open()` - открытие существующего shared memory объекта
- `shared_memory_kv_destroy()` - уничтожение shared memory объекта и освобождение ресурсов

## Ограничения

- Максимальное количество записей: `MAX_ENTRIES` (10)
- Максимальный размер ключа: `KEY_SIZE - 1` (63 символа)
- Максимальный размер значения: `VALUE_SIZE - 1` (255 символов)

## Требования

- Linux/Unix система с поддержкой POSIX Shared Memory
- GCC компилятор
- Стандартная библиотека C (C11)

## Компиляция

```bash
gcc -std=c11 -Wall -Wextra -o build/kv_store src/shared_memory_kv.c
```

## Использование

### Создание shared memory объекта

```c
#include "shared_memory_kv.h"

int shm_fd;
shared_memory_kv_store_t *store = shared_memory_kv_create(&shm_fd);

if (store == NULL) {
    // Обработка ошибки
    return 1;
}

// Использование store...

// Очистка
shared_memory_kv_destroy(shm_fd, store);
```

## Важные замечания

1. **Очистка ресурсов**: Всегда вызывайте `shared_memory_kv_destroy()` после использования для корректного освобождения ресурсов
2. **Синхронизация**: Используйте семафор `store->sem` для синхронизации доступа между процессами
3. **Размер shared memory**: Размер структуры должен быть известен на этапе компиляции

## Статус разработки

- ✅ `shared_memory_kv_create()` - реализовано
- ⏳ `shared_memory_kv_open()` - в разработке
- ⏳ `shared_memory_kv_destroy()` - в разработке
- ⏳ Функции для работы с KV парами (get, set, delete) - в разработке

## Лицензия

Учебный проект.

