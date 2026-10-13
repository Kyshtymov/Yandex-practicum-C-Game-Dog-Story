# 🐕 Dog Story — игровой сервер

Многопользовательская онлайн-игра на C++: клиенты управляют собаками, которые собирают потерянные вещи и относят их в бюро находок. Проект реализует серверную часть игры с REST API, игровой логикой, визуализацией состояния, автосохранением и таблицей рекордов в PostgreSQL.

---

## 📋 Описание

Игра выполнена в виде клиент-серверного приложения. Сервер хранит и обновляет игровое состояние, обрабатывает команды управления собаками и отдаёт актуальные данные клиентам через REST API. Клиентская часть (HTML/CSS/JavaScript) работает в браузере и получает состояние с сервера.

Сервер написан на современном C++ с использованием **Boost.Asio**, **Boost.Beast**, **Boost.Json**, **Boost.ProgramOptions**, **Boost.Serialization** и **libpqxx** для работы с PostgreSQL. Проект развивался итеративно: от простой выдачи карт до полноценной игры с генерацией предметов, маршрутизацией, автосохранением и таблицей рекордов.

---

## ✨ Возможности

- 🗺️ **Несколько игровых карт** — загружаются из JSON-конфигурации при старте сервера
- 🐕 **Многопользовательская игра** — подключение нескольких игроков к одной карте
- 🎮 **Управление собаками** через REST API (направление и скорость движения)
- 📦 **Потерянные предметы** — генерируются случайным образом по карте
- 🎒 **Рюкзак собаки** — сбор и доставка предметов в бюро находок
- 🏆 **Начисление очков** за доставленные предметы
- 🗺️ **SVG-карта маршрутов** — эндпоинт возвращает карту для отрисовки на клиенте
- ⏱️ **Игровые часы** — ручное обновление через `/api/v1/game/tick` или автоматическое с `--tick-period`
- 💾 **Автосохранение состояния** в файл с возможностью восстановления
- 🏅 **Таблица рекордов** в PostgreSQL с сортировкой и пагинацией
- 🔐 **Токены авторизации** для игроков
- 🚪 **Уход собак на пенсию** при бездействии (`dogRetirementTime`)
- 📊 **Логирование** в JSON-формате через Boost.Log

---

## 🧩 Структура проекта

```
.
├── main.cpp                        # Точка входа, сборка компонентов
├── sdk.h                           # Заголовок SDK (Windows)
├── boost_json.cpp                  # Подключение реализации Boost.Json
├── boost_log.h                     # Настройка JSON-логирования
├── boost_parse_command_line.h      # Парсинг командной строки через Boost.ProgramOptions
├── http_server.h / .cpp            # Асинхронный HTTP-сервер на Boost.Beast
├── request_handler.h               # Обработчик запросов и логирующий декоратор
├── respond_maker.h / .cpp          # Формирование HTTP-ответов
├── body_maker.h / .cpp             # Формирование JSON-тела ответов
├── model.h / .cpp                  # Игровая модель: Game, Map, Dog, GameSession
├── players.h / .cpp                # Игроки и токены
├── extra_objects.h / .cpp          # Дополнительные данные (типы трофеев)
├── loot_generator.h / .cpp         # Генератор потерянных предметов
├── collision_detector.h / .cpp     # Обнаружение коллизий
├── geom.h                          # Геометрические примитивы
├── tagged.h                        # Tagged-типы
├── tagged_uuid.h / .cpp            # UUID как Tagged-тип
├── json_loader.h / .cpp            # Загрузка конфигурации игры из JSON
├── application_handler.h / .cpp    # ApplicationHandler и Ticker
├── application_listener.h / .cpp   # Application и интерфейс слушателя
├── serialization_listener.h / .cpp # Сохранение и восстановление состояния
├── model_serialization.h / .cpp    # Сериализация модели
├── players_serialization.h / .cpp  # Сериализация игроков
├── tagged_serialization.h          # Сериализация Tagged-типов
├── connection_pool.h / .cpp        # Пул соединений с БД
├── config.json                     # Пример конфигурации игры
├── conanfile.txt                   # Зависимости Conan
├── CMakeLists.txt
├── Dockerfile
└── README.md
```

---

## 🚀 Сборка и запуск

### Требования

- Компилятор с поддержкой **C++20**
- **CMake 3.11+**
- **Conan 1.52.0** — установить командой `pip install conan==1.52.0`
  > ⚠️ Версия 1.51 не работает. Используйте именно **1.52.0** или новее из ветки 1.x.
- **Boost 1.78+** (Asio, Beast, Json, Log, ProgramOptions, Serialization, UUID, Signals2)
- **PostgreSQL** и **libpqxx**
- **Catch2** для юнит-тестов

### Зависимости (`conanfile.txt`)

```ini
[requires]
boost/1.78.0
catch2/3.1.0
libpqxx/7.7.4

[generators]
cmake
```

> На **Windows** генератор `cmake` нужно заменить на `cmake_multi`, а в `CMakeLists.txt` — подключить `conanbuildinfo_multi.cmake` вместо `conanbuildinfo.cmake`.

---

### 🐧 Сборка под Linux

Обязательно указывайте флаги `-s compiler.libcxx=libstdc++11` и `-s build_type=<тип>`.

**Release:**

```bash
mkdir -p build-release
cd build-release
conan install .. --build=missing -s build_type=Release -s compiler.libcxx=libstdc++11
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
cd ..
```

**Debug:**

```bash
mkdir -p build-debug
cd build-debug
conan install .. --build=missing -s build_type=Debug -s compiler.libcxx=libstdc++11
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
cd ..
```

---

### 🪟 Сборка под Windows

1. В `conanfile.txt` замените генератор `cmake` на `cmake_multi`.
2. В `CMakeLists.txt` замените
   `include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)` на
   `include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)`.

Затем соберите нужные конфигурации:

```cmd
mkdir build
cd build
conan install .. --build=missing -s build_type=Debug
conan install .. --build=missing -s build_type=Release
conan install .. --build=missing -s build_type=RelWithDebInfo
conan install .. --build=missing -s build_type=MinSizeRel
cmake ..
cmake --build . --config Release
```

> 💡 Собирать все конфигурации долго — оставьте только нужные. Запускайте команды **только из родного cmd** — в других консолях возможны проблемы.

---

### 🐳 Запуск через Docker (рекомендуется)

Каноничный способ запуска — через Docker. Все детали сборки описаны в `Dockerfile`.

```bash
docker build -t my_http_server .
docker run --rm -p 80:8080 my_http_server
```

После запуска сервер доступен по адресу `http://127.0.0.1:8080`.

---

### ▶️ Запуск собранного сервера

Из каталога `build`:

```bash
bin/game_server ../data/config.json ../static/
```

После запуска откройте в браузере:

- **`http://127.0.0.1:8080/api/v1/maps`** — список карт
- **`http://127.0.0.1:8080/api/v1/map/map1`** — подробная информация о карте `map1`
- **`http://127.0.0.1:8080/`** — статический контент из каталога `static`

---

## ⚙️ Параметры командной строки

| Параметр | Описание |
|---|---|
| `-h, --help` | Справка |
| `-t, --tick-period <ms>` | Период автоматического обновления игровых часов |
| `-c, --config-file <path>` | Путь к JSON-конфигурации игры |
| `-w, --www-root <dir>` | Каталог со статическими файлами |
| `-s, --state-file <path>` | Файл для сохранения состояния |
| `-p, --save-state-period <ms>` | Период автосохранения состояния |
| `--randomize-spawn-points` | Случайные точки появления собак |

Переменная окружения `GAME_DB_URL` задаёт URL для подключения к PostgreSQL.

---

## 🌐 REST API

Сервер принимает запросы на порт **8080**.

| Метод | Эндпоинт | Описание |
|---|---|---|
| GET | `/api/v1/maps` | Список всех карт |
| GET | `/api/v1/maps/{id}` | Подробное описание карты, включая `lootTypes` |
| POST | `/api/v1/game/join` | Подключение игрока к карте |
| GET | `/api/v1/game/players` | Список игроков на карте |
| GET | `/api/v1/game/state` | Текущее состояние игры |
| POST | `/api/v1/game/player/action` | Команда управления собакой |
| POST | `/api/v1/game/tick` | Ручное обновление игровых часов |
| GET | `/api/v1/game/records` | Таблица рекордов |

Также сервер отдаёт статические файлы из каталога `--www-root`.

---

## 🏗️ Архитектура

Проект следует принципу **инверсии зависимостей** и разделению на слои.

```
┌──────────────────────────────────────────┐
│              main                        │
└────────────────┬─────────────────────────┘
                 │
     ┌───────────┼───────────┐
     ▼           ▼           ▼
┌─────────┐ ┌─────────┐ ┌──────────────┐
│  HTTP   │ │  App    │ │Serialization │
│ Server  │ │ Handler │ │  Listener    │
└────┬────┘ └────┬────┘ └──────┬───────┘
     │           │             │
     ▼           ▼             ▼
┌──────────────────────────────────────┐
│          Application                 │
│  (наблюдатель + точка входа)         │
└────────────────┬─────────────────────┘
                 │
                 ▼
┌──────────────────────────────────────┐
│              Model                   │
│  Game, Map, Dog, GameSession         │
└──────────────────────────────────────┘
```

### Ключевые решения

- **ApplicationHandler** — прослойка между HTTP-обработчиком и игровой моделью. Скрывает детали модели от инфраструктуры.
- **Ticker** — асинхронный таймер на Boost.Asio, работающий внутри strand. Периодически вызывает `Application::Tick`.
- **ApplicationListener** — интерфейс наблюдателя для сериализации. Реализуется `SerializationListener`, что позволяет модели не зависеть от инфраструктуры.
- **Паттерн «Наблюдатель»** через `ApplicationListener`: `Application` уведомляет слушателя о шагах игровых часов, слушатель решает, когда сохранять состояние.
- **ConnectionPool** — пул соединений с PostgreSQL. Потокобезопасен, выдаёт прокси-объекты `ConnectionWrapper`, возвращающие соединение в деструкторе.
- **AtomicSave** — сохранение через временный файл и переименование, чтобы избежать порчи данных при сбое.
- **Tagged-типы** — строгая типизация идентификаторов (`Map::Id`, `Office::Id`, `Token`) через `util::Tagged`.
- **Логирование** — Boost.Log с JSON-форматтером, включающим таймстемп, дополнительные данные и сообщение.
- **Коллизии** — модуль `collision_detector` ищет пересечения отрезков пути с предметами, события сортируются по времени.
- **LootGenerator** — генерирует количество трофеев в зависимости от прошедшего времени, количества трофеев и мародёров.

---

## 📥 Формат конфигурации

```json
{
  "defaultDogSpeed": 3.0,
  "lootGeneratorConfig": {
    "period": 5.0,
    "probability": 0.5
  },
  "dogRetirementTime": 60.0,
  "defaultBagCapacity": 3,
  "maps": [
    {
      "id": "map1",
      "name": "Map 1",
      "dogSpeed": 4.0,
      "bagCapacity": 5,
      "lootTypes": [
        { "name": "key", "file": "assets/key.obj", "type": "obj",
          "rotation": 90, "color": "#338844", "scale": 0.03, "value": 10 }
      ],
      "roads": [
        { "x0": 0, "y0": 0, "x1": 40 },
        { "x0": 40, "y0": 0, "y1": 30 }
      ],
      "buildings": [
        { "x": 5, "y": 5, "w": 30, "h": 20 }
      ],
      "offices": [
        { "id": "o0", "x": 40, "y": 30, "offsetX": 5, "offsetY": 0 }
      ]
    }
  ]
}
```

---

## 🏅 Таблица рекордов

При уходе собаки на пенсию её результат записывается в таблицу `retired_players` в PostgreSQL:

| Поле | Тип | Описание |
|---|---|---|
| `id` | UUID | Первичный ключ |
| `name` | VARCHAR(100) | Имя собаки |
| `score` | DOUBLE PRECISION | Набранные очки |
| `playtime` | DOUBLE PRECISION | Время в игре (секунды) |

Индексы:

```sql
CREATE INDEX idx_retired_players_score ON retired_players(score DESC);
CREATE INDEX idx_retired_players_playtime ON retired_players(playtime ASC);
CREATE INDEX idx_retired_players_name ON retired_players(name ASC);
```

Таблица создаётся автоматически при первом запуске, если её ещё нет.

---

## 💾 Сохранение состояния

- Если задан `--state-file`, при старте сервер пытается восстановить состояние из файла.
- Автосохранение происходит при накоплении игрового времени не меньше `--save-state-period`.
- При получении `SIGINT` или `SIGTERM` состояние сохраняется принудительно.
- Сохранение атомарное: сначала записывается временный файл, затем переименовывается в целевой.
- Формат — Boost.Serialization (текстовый архив).

---

## 🧪 Тестирование

Проект использует **Catch2** для юнит-тестов. Основные цели:

- Генератор трофеев `LootGenerator`
- Коллизии и сбор предметов
- Сериализация и восстановление модели
- Модель игры (собаки, карты, сессии)

Сборка и запуск тестов:

```bash
./game_server_tests
```

---

Проект выполнен в рамках прохождения курса от Яндекс практикума.

---