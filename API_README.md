# Shared Memory KV Store - FastAPI Server

REST API сервер для взаимодействия с C-based shared memory key-value store через Python.

## Требования

- Python 3.8+
- Собранная shared library (`libshared_memory_kv.so`)
- Linux/Unix система с поддержкой POSIX Shared Memory

## Установка

1. Установите Python зависимости:
```bash
pip install -r requirements.txt
```

2. Соберите shared library:
```bash
make libso
```

Это создаст `build/libshared_memory_kv.so`, который будет использоваться Python оберткой.

## Запуск сервера

```bash
python3 api_server.py
```

Или через uvicorn напрямую:
```bash
uvicorn api_server:app --host 0.0.0.0 --port 8000
```

Сервер запустится на `http://localhost:8000`

## API Endpoints

### GET `/`
Информация об API и доступных эндпоинтах.

### GET `/get/{key}`
Получить значение по ключу.

**Пример:**
```bash
curl http://localhost:8000/get/mykey
```

**Ответ:**
```json
{
  "key": "mykey",
  "value": "myvalue"
}
```

**Ошибки:**
- `404`: Ключ не найден
- `503`: Store не инициализирован

### POST `/set`
Установить key-value пару.

**Пример:**
```bash
curl -X POST http://localhost:8000/set \
  -H "Content-Type: application/json" \
  -d '{"key": "mykey", "value": "myvalue"}'
```

**Тело запроса:**
```json
{
  "key": "string",
  "value": "string"
}
```

**Ответ:**
```json
{
  "success": true,
  "message": "Key 'mykey' set successfully"
}
```

**Ошибки:**
- `400`: Некорректный запрос
- `413`: Ключ или значение слишком длинные
- `507`: Таблица заполнена (max 10 entries)
- `503`: Store не инициализирован

### GET `/status`
Получить статус store: версию, количество записей и все key-value пары.

**Пример:**
```bash
curl http://localhost:8000/status
```

**Ответ:**
```json
{
  "version": 5,
  "entry_count": 3,
  "max_entries": 10,
  "entries": [
    {
      "key": "key1",
      "value": "value1",
      "timestamp": 1699123456
    },
    {
      "key": "key2",
      "value": "value2",
      "timestamp": 1699123500
    }
  ]
}
```

## Архитектура

### Компоненты

1. **`kv_store_wrapper.py`** - Python обертка для C библиотеки через ctypes
   - Определяет C структуры через ctypes.Structure
   - Загружает shared library
   - Предоставляет Python-friendly интерфейс

2. **`api_server.py`** - FastAPI сервер
   - Инициализирует shared memory при старте
   - Предоставляет REST API эндпоинты
   - Корректно обрабатывает cleanup при завершении

### Обработка семафоров

Семафоры обрабатываются автоматически внутри C функций (`sem_wait`/`sem_post`). Python обертка не требует дополнительной синхронизации, так как все операции уже защищены на уровне C кода.

### Инициализация

При запуске сервер:
1. Пытается открыть существующий shared memory store
2. Если не найден - создает новый
3. При завершении - освобождает ресурсы (munmap, close fd)

**Важно:** Сервер НЕ вызывает `unlink()` при завершении, так как это должно делать только создающее приложение. Для полной очистки используйте отдельный скрипт или вызовите `unlink()` вручную.

## Интерактивная документация

После запуска сервера доступна автоматическая документация:
- Swagger UI: `http://localhost:8000/docs`
- ReDoc: `http://localhost:8000/redoc`

## Отладка

Если возникают проблемы:

1. **Library not found**: Убедитесь, что выполнили `make libso`
2. **Permission denied**: Проверьте права доступа к `/dev/shm/`
3. **Store not initialized**: Проверьте логи сервера при старте

## Ограничения

- Максимум 10 записей (MAX_ENTRIES)
- Максимальный размер ключа: 63 символа
- Максимальный размер значения: 255 символов
- Shared memory существует до перезагрузки системы или явного unlink


