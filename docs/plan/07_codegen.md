# DeltaQ IDE - Генерация кода

## Обзор

Система генерации кода DeltaQ IDE отвечает за преобразование визуальных конструкций (графов, UI-макетов) в исходный код на **целевом языке программирования**. Архитектура построена по принципу **язык-агностичного IR** (промежуточного представления) с подключаемыми **языковыми бэкендами**.

Первый целевой язык — **C/C++**, но конвейер спроектирован так, что добавление Python, Rust, Go и других языков не требует изменения ядра генерации.

**Ключевой конвейер:**
```
Граф/UI → GraphCompiler → IR (язык-агностичный) → LanguageBackend → исходный код → сборка
```

---

## Архитектура системы генерации кода

```
┌─────────────┐  ┌─────────────┐  ┌─────────────┐
│   Графы     │  │  UI-макеты  │  │   Модули    │
│  (.dqgraph) │  │  (.dqui)    │  │  (.dqmod)   │
└──────┬──────┘  └──────┬──────┘  └──────┬──────┘
       │                │                │
       ▼                ▼                │
┌─────────────┐  ┌─────────────┐        │
│   Graph     │  │     UI      │        │
│   Compiler  │  │   Compiler  │        │
└──────┬──────┘  └──────┬──────┘        │
       │                │                │
       ▼                ▼                │
┌───────────────────────────────────┐   │
│    IR (промежуточное представл.)  │   │
│   Язык-агностичные типы и инстр. │   │
│   IRType, IRFunction, IRCall...   │   │
└───────────────┬───────────────────┘   │
                │                       │
       ┌────────┼────────┐              │
       ▼        ▼        ▼              ▼
┌────────┐ ┌────────┐ ┌────────┐  ┌──────────┐
│   C    │ │ Python │ │  Rust  │  │  Модули  │
│Backend │ │Backend │ │Backend │  │ (исходн.)│
└───┬────┘ └───┬────┘ └───┬────┘  └────┬─────┘
    │          │          │            │
    ▼          ▼          ▼            │
 .c/.h      .py       .rs             │
    │          │          │            │
    ▼          ▼          ▼            ▼
┌─────────────────────────────────────────┐
│        Система сборки (pluggable)       │
│  CMake+GCC │ pip/python │ cargo │ ...   │
└──────────────────┬──────────────────────┘
                   │
                   ▼
            ┌─────────────┐
            │ Исполняемый │
            │ файл/скрипт │
            └─────────────┘
```

### Матрица: Язык × Тип приложения

Каждый `LanguageBackend` реализует один или несколько типов приложений (AppType):

| AppType | C/C++ | Python | Rust |
|---------|-------|--------|------|
| **Console** | `main()` + stdio | `if __name__` + `input()/print()` | `fn main()` + `std::io` |
| **SDL2 GUI** | SDL2 C API | SDL2 через ctypes/pygame | sdl2 crate |
| **Custom** | пользовательский шаблон | пользовательский шаблон | пользовательский шаблон |

---

## Двухуровневая архитектура бэкендов

Система генерации разделена на два уровня:

1. **LanguageBackend** — язык программирования (C, Python, Rust) — определяет синтаксис, типы, систему сборки
2. **AppTypeBackend** — тип приложения (Console, SDL2, Custom) — определяет точку входа, шаблон проекта, зависимости

```
LanguageBackend (язык)
    └── AppTypeBackend (тип приложения)
         ├── Console  — консольная программа
         ├── SDL2     — графическое приложение
         └── Custom   — пользовательский шаблон
```

### AppType (тип приложения)

```cpp
class AppTypeBackend {
public:
    virtual ~AppTypeBackend() = default;

    virtual QString id() const = 0;           // "console", "sdl2", "custom"
    virtual QString displayName() const = 0;

    // Генерация точки входа и структуры проекта
    // LanguageBackend передаётся для emit-функций конкретного языка
    virtual GeneratedFiles generate(const IR& ir, const ProjectConfig& config,
                                     LanguageBackend* lang) = 0;

    // Дополнительные зависимости для файла сборки
    virtual QStringList requiredDependencies() const = 0;
};
```

### Бэкенды генерации

#### 1. Console AppType

Генерирует приложение с точкой входа и stdio-взаимодействием. Работает с любым языком.

```cpp
class ConsoleAppType : public AppTypeBackend {
public:
    QString id() const override { return "console"; }

    GeneratedFiles generate(const IR& ir, const ProjectConfig& config,
                             LanguageBackend* lang) override {
        GeneratedFiles files;

        // Генерация main-файла через языковой бэкенд
        QString mainCode;

        // Импорты/включения через бэкенд
        for (const auto& dep : ir.requiredModules) {
            mainCode += lang->emitImport(dep) + "\n";
        }
        mainCode += "\n";

        // Точка входа через бэкенд
        mainCode += lang->emitEntryPoint(ir, config);

        QString ext = lang->fileExtension();  // ".c", ".py", ".rs"
        files.addFile("generated/main" + ext, mainCode);
        return files;
    }
};
```

Результат для разных языков:

**C:**
```c
#include <stdio.h>
#include "add.h"
int main(int argc, char* argv[]) {
    int a = dq_read_int();
    int result = dq_add(a, 5);
    printf("%d\n", result);
    return 0;
}
```

**Python:**
```python
from modules.add import dq_add
from modules.read_int import dq_read_int

def main():
    a = dq_read_int()
    result = dq_add(a, 5)
    print(result)

if __name__ == "__main__":
    main()
```

**Rust:**
```rust
mod modules;
use modules::{add::dq_add, read_int::dq_read_int};

fn main() {
    let a = dq_read_int();
    let result = dq_add(a, 5);
    println!("{}", result);
}
```

#### 2. SDL2 AppType

Генерирует приложение с графическим интерфейсом на SDL2.

```cpp
class SDL2AppType : public AppTypeBackend {
public:
    QString id() const override { return "sdl2"; }

    GeneratedFiles generate(const IR& ir, const ProjectConfig& config,
                             LanguageBackend* lang) override {
        GeneratedFiles files;
        QString ext = lang->fileExtension();

        // Точка входа с SDL2-инициализацией
        files.addFile("generated/main" + ext, lang->emitSDL2Main(ir, config));

        // Виджеты
        files.addFile("generated/ui" + ext, lang->emitUICode(ir));

        // Обработчики событий
        files.addFile("generated/events" + ext, lang->emitEventHandlers(ir));

        // Логика из графов
        files.addFile("generated/graph_logic" + ext, lang->emitGraphLogic(ir));

        return files;
    }

    QStringList requiredDependencies() const override {
        return {"SDL2", "SDL2_ttf"};
    }
};
```

#### 3. Custom AppType

Позволяет пользователю определить собственный шаблон генерации кода.

```cpp
class CustomAppType : public AppTypeBackend {
public:
    CustomAppType(const QString& templateDir);
    QString id() const override { return "custom"; }

    GeneratedFiles generate(const IR& ir, const ProjectConfig& config,
                             LanguageBackend* lang) override;

private:
    QString processTemplate(const QString& templateContent, const IR& ir,
                             LanguageBackend* lang);
    QString m_templateDir;
};
```

---

## Кроссплатформенная и мультиязычная поддержка

### Таблица поддержки: Язык × Платформа

| Возможность | Linux | Windows | macOS |
|-------------|:-----:|:-------:|:-----:|
| **C/C++** | GCC, Clang | MSVC, MinGW, Clang | Clang, GCC |
| **Python** | python3 | python3 | python3 |
| **Rust** | cargo + rustc | cargo + rustc | cargo + rustc |
| SDL2 UI | Все | Все | Все |
| Отладка C (GDB) | Да | MinGW | Нет |
| Отладка C (LLDB) | Да | Нет | Да |
| Отладка Python (debugpy) | Да | Да | Да |
| Отладка Rust (rust-gdb) | Да | Да | Да |
| LSP (clangd) | Да | Да | Да |
| LSP (pyright) | Да | Да | Да |
| LSP (rust-analyzer) | Да | Да | Да |

### Генерация файлов сборки (язык-зависимая)

Каждый `LanguageBackend` генерирует свой файл сборки через `generateBuildFile()`.

#### C/C++: CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.24)
project(MyDeltaQProject VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

# Обнаружение платформы
if(WIN32)
    set(PLATFORM_SOURCES src/platform/win32.c)
    set(PLATFORM_LIBS ws2_32 winmm)
elseif(APPLE)
    set(PLATFORM_SOURCES src/platform/macos.c)
    set(PLATFORM_LIBS "-framework Cocoa" "-framework IOKit")
else()
    set(PLATFORM_SOURCES src/platform/linux.c)
    set(PLATFORM_LIBS m pthread dl)
endif()

# SDL2 (если AppType = sdl2)
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

file(GLOB MODULE_SOURCES "modules/src/*.c" "modules/src/*.cpp")
file(GLOB GENERATED_SOURCES "generated/*.c")
file(GLOB WRAPPER_SOURCES "wrappers/*.c" "wrappers/*.cpp")

add_executable(${PROJECT_NAME}
    ${MODULE_SOURCES} ${GENERATED_SOURCES}
    ${WRAPPER_SOURCES} ${PLATFORM_SOURCES}
)
target_include_directories(${PROJECT_NAME} PRIVATE
    include modules/include wrappers generated ${SDL2_INCLUDE_DIRS}
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL2::SDL2 SDL2_ttf::SDL2_ttf ${PLATFORM_LIBS}
)
```

#### Python: pyproject.toml

```toml
[project]
name = "my-deltaq-project"
version = "1.0.0"
requires-python = ">=3.10"
dependencies = [
    "pygame>=2.5",  # если AppType = sdl2
]

[project.scripts]
main = "generated.main:main"
```

#### Rust: Cargo.toml

```toml
[package]
name = "my-deltaq-project"
version = "1.0.0"
edition = "2021"

[dependencies]
sdl2 = "0.36"  # если AppType = sdl2
```

---

## Генерация SDL2 кода (подробно)

### Структура сгенерированного SDL2-приложения

```
generated/
├── main.c              # Точка входа, инициализация SDL2
├── ui.h                # Объявления виджетов и UIState
├── ui.c                # Создание, рендеринг, обработка событий виджетов
├── events.h            # Объявления пользовательских обработчиков
├── events.c            # Реализация обработчиков событий
├── graph_logic.h       # Объявления функций из графов
├── graph_logic.c       # Логика, сгенерированная из графов
├── dq_widgets.h        # Общая библиотека виджетов
└── dq_widgets.c        # Реализация базовых виджетов SDL2
```

### Библиотека виджетов (dq_widgets)

```c
// dq_widgets.h
#ifndef DQ_WIDGETS_H
#define DQ_WIDGETS_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// Базовые типы
typedef struct {
    int r, g, b, a;
} DQ_Color;

typedef enum {
    DQ_ALIGN_LEFT,
    DQ_ALIGN_CENTER,
    DQ_ALIGN_RIGHT
} DQ_TextAlign;

typedef enum {
    DQ_WIDGET_HIDDEN    = 0,
    DQ_WIDGET_VISIBLE   = 1,
    DQ_WIDGET_DISABLED  = 2
} DQ_WidgetState;

// Прямоугольник с координатами
typedef struct {
    int x, y, w, h;
} DQ_Rect;

// Текстовый рендеринг
void dq_render_text(SDL_Renderer* renderer, TTF_Font* font,
                    const char* text, DQ_Rect rect,
                    DQ_Color color, DQ_TextAlign align);

// Базовые формы
void dq_render_rect(SDL_Renderer* renderer, DQ_Rect rect,
                    DQ_Color fill, DQ_Color border, int borderWidth);

void dq_render_rounded_rect(SDL_Renderer* renderer, DQ_Rect rect,
                             int radius, DQ_Color fill, DQ_Color border);

// Проверка нажатия
int dq_point_in_rect(int px, int py, DQ_Rect rect);

// Обрезка (clipping)
void dq_push_clip(SDL_Renderer* renderer, DQ_Rect rect);
void dq_pop_clip(SDL_Renderer* renderer);

// Виджеты
typedef void (*DQ_ClickCallback)(void* userdata);
typedef void (*DQ_ChangeCallback)(const char* text, void* userdata);

// Button
typedef struct {
    DQ_Rect rect;
    char text[128];
    DQ_WidgetState state;
    int hovered;
    int pressed;
    TTF_Font* font;
    DQ_Color bg_normal;
    DQ_Color bg_hover;
    DQ_Color bg_pressed;
    DQ_Color text_color;
    DQ_Color border_color;
    DQ_ClickCallback on_click;
    void* userdata;
} DQ_Button;

void dq_button_init(DQ_Button* btn, DQ_Rect rect, const char* text, TTF_Font* font);
void dq_button_render(SDL_Renderer* renderer, DQ_Button* btn);
int  dq_button_handle_event(DQ_Button* btn, SDL_Event* event);

// TextField
typedef struct {
    DQ_Rect rect;
    char text[1024];
    char placeholder[256];
    DQ_WidgetState state;
    int focused;
    int cursor_pos;
    int selection_start;
    int selection_end;
    int scroll_offset;
    int read_only;
    int password_mode;
    int max_length;
    TTF_Font* font;
    DQ_Color bg_color;
    DQ_Color text_color;
    DQ_Color placeholder_color;
    DQ_Color border_normal;
    DQ_Color border_focused;
    DQ_Color selection_color;
    DQ_ChangeCallback on_change;
    DQ_ClickCallback on_submit;
    void* userdata;
} DQ_TextField;

void dq_textfield_init(DQ_TextField* tf, DQ_Rect rect, TTF_Font* font);
void dq_textfield_render(SDL_Renderer* renderer, DQ_TextField* tf);
int  dq_textfield_handle_event(DQ_TextField* tf, SDL_Event* event);

// Checkbox
typedef struct {
    DQ_Rect rect;
    char label[256];
    int checked;
    DQ_WidgetState state;
    int hovered;
    TTF_Font* font;
    DQ_Color check_color;
    DQ_Color text_color;
    DQ_ClickCallback on_toggle;
    void* userdata;
} DQ_Checkbox;

void dq_checkbox_init(DQ_Checkbox* cb, DQ_Rect rect, const char* label, TTF_Font* font);
void dq_checkbox_render(SDL_Renderer* renderer, DQ_Checkbox* cb);
int  dq_checkbox_handle_event(DQ_Checkbox* cb, SDL_Event* event);

// Slider
typedef struct {
    DQ_Rect rect;
    int min_value;
    int max_value;
    int value;
    int step;
    int dragging;
    DQ_WidgetState state;
    DQ_Color track_color;
    DQ_Color fill_color;
    DQ_Color thumb_color;
    DQ_ChangeCallback on_change;
    void* userdata;
} DQ_Slider;

void dq_slider_init(DQ_Slider* slider, DQ_Rect rect, int min, int max);
void dq_slider_render(SDL_Renderer* renderer, DQ_Slider* slider);
int  dq_slider_handle_event(DQ_Slider* slider, SDL_Event* event);

// ProgressBar
typedef struct {
    DQ_Rect rect;
    int min_value;
    int max_value;
    int value;
    int show_text;
    DQ_Color bg_color;
    DQ_Color fill_color;
    DQ_Color text_color;
    TTF_Font* font;
} DQ_ProgressBar;

void dq_progressbar_init(DQ_ProgressBar* pb, DQ_Rect rect, int min, int max);
void dq_progressbar_render(SDL_Renderer* renderer, DQ_ProgressBar* pb);

#endif
```

---

## Конвейер компиляции: граф → код

Конвейер работает в 3 этапа. Первые два — язык-агностичные, третий зависит от выбранного `LanguageBackend`.

### Полный пример

#### Исходный граф

```json
{
  "name": "calculator",
  "nodes": [
    {"id": "n1", "module": "read_int", "props": {"prompt": "Введите a: "}},
    {"id": "n2", "module": "read_int", "props": {"prompt": "Введите b: "}},
    {"id": "n3", "module": "add"},
    {"id": "n4", "module": "print_int", "props": {"format": "Сумма: %d\n"}}
  ],
  "connections": [
    {"from": "n1:result", "to": "n3:a"},
    {"from": "n2:result", "to": "n3:b"},
    {"from": "n3:result", "to": "n4:value"},
    {"exec_from": "entry", "exec_to": "n1"},
    {"exec_from": "n1", "exec_to": "n2"},
    {"exec_from": "n2", "exec_to": "n3"},
    {"exec_from": "n3", "exec_to": "n4"}
  ]
}
```

#### Этап 1: Топологическая сортировка

```
Порядок выполнения (по execution): n1 -> n2 -> n3 -> n4
Порядок вычислений (по data):      n1, n2 -> n3 -> n4
Итоговый порядок: n1, n2, n3, n4
```

#### Этап 2: Генерация IR (язык-агностичный)

```
IR Instructions:
  [COMMENT]   "Узел n1: read_int"
  [CALL]      var_n1_result = dq_read_int("Введите a: ")
  [COMMENT]   "Узел n2: read_int"
  [CALL]      var_n2_result = dq_read_int("Введите b: ")
  [COMMENT]   "Узел n3: add"
  [CALL]      var_n3_result = dq_add(var_n1_result, var_n2_result)
  [COMMENT]   "Узел n4: print_int"
  [CALL]      dq_print_int("Сумма: %d\n", var_n3_result)

Variables (IR-типы, не привязаны к языку):
  var_n1_result : Integer(32)
  var_n2_result : Integer(32)
  var_n3_result : Integer(32)
```

IR одинаков для любого целевого языка. Различия появляются только на этапе 3.

#### Этап 3: Генерация кода (через LanguageBackend)

Один и тот же IR даёт разный код в зависимости от выбранного бэкенда:

**C (CLanguageBackend):**
```c
/* Сгенерировано DeltaQ IDE */
/* Граф: calculator */

#include "read_int.h"
#include "add.h"
#include "print_int.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    int var_n1_result;
    int var_n2_result;
    int var_n3_result;

    var_n1_result = dq_read_int("Введите a: ");
    var_n2_result = dq_read_int("Введите b: ");
    var_n3_result = dq_add(var_n1_result, var_n2_result);
    dq_print_int("Сумма: %d\n", var_n3_result);

    return 0;
}
```

**Python (PythonBackend):**
```python
# Сгенерировано DeltaQ IDE
# Граф: calculator

from modules.read_int import dq_read_int
from modules.add import dq_add
from modules.print_int import dq_print_int

def main():
    var_n1_result: int = dq_read_int("Введите a: ")
    var_n2_result: int = dq_read_int("Введите b: ")
    var_n3_result: int = dq_add(var_n1_result, var_n2_result)
    dq_print_int(f"Сумма: {var_n3_result}")

if __name__ == "__main__":
    main()
```

**Rust (RustBackend):**
```rust
// Сгенерировано DeltaQ IDE
// Граф: calculator

mod modules;
use modules::{read_int::dq_read_int, add::dq_add, print_int::dq_print_int};

fn main() {
    let var_n1_result: i32 = dq_read_int("Введите a: ");
    let var_n2_result: i32 = dq_read_int("Введите b: ");
    let var_n3_result: i32 = dq_add(var_n1_result, var_n2_result);
    dq_print_int(&format!("Сумма: {}", var_n3_result));
}
```

---

## Обработка сложных конструкций

### Ветвления (Branch)

Когда в графе есть модуль ветвления (branch), генерируются `if-else` конструкции:

```c
/* Узел n5: compare */
int var_n5_is_equal = (var_n3_a == var_n4_b);

if (var_n5_is_equal) {
    /* Ветка exec_true */
    /* Узел n6: print */
    dq_print("Числа равны\n");
} else {
    /* Ветка exec_false */
    /* Узел n7: print */
    dq_print("Числа не равны\n");
}
```

### Циклы

Для поддержки циклов предусмотрены специальные модули:

```c
/* Узел n_loop: for_loop (start=0, end=10, step=1) */
for (int var_n_loop_i = 0; var_n_loop_i < 10; var_n_loop_i += 1) {
    /* Тело цикла (exec_body) */
    /* Узел n_body: print_int */
    dq_print_int("%d\n", var_n_loop_i);
}
/* Продолжение после цикла (exec_done) */
```

### Составные модули (матрёшка): граф → исходный код → модуль

Ключевая особенность DeltaQ — **граф сам является модулем**. Каждый граф компилируется в отдельную функцию на **целевом языке** и автоматически получает `.dqmod`, после чего может быть использован как блок в другом графе на любой глубине вложенности.

#### Конвейер: граф → модуль (язык-агностичный)

```
┌──────────────┐     ┌──────────────┐     ┌────────────────┐     ┌──────────────┐
│  .dqgraph    │────▶│ GraphCompiler│────▶│  IR             │────▶│ Language     │
│  (визуальный │     │ (→ IR)       │     │  (IRFunction)   │     │ Backend     │
│   граф)      │     │              │     │                 │     │             │
└──────────────┘     └──────────────┘     └────────────────┘     └──────┬──────┘
                                                                         │
                                                                         ▼
                                                             ┌──────────────────┐
                                                             │ .c/.py/.rs + .h  │
                                                             │ + .dqmod         │
                                                             │ (origin: "graph")│
                                                             └──────┬───────────┘
                                                                    │
                                                                    ▼
                                                           Регистрация в палитре
                                                           → блок можно перетащить
                                                             в другой граф
```

#### Пример: трёхуровневая вложенность

**Уровень 0** — атомарные модули (написаны вручную):

```c
// add.cpp
int dq_add(int a, int b) { return a + b; }

// multiply.cpp
int dq_multiply(int a, int b) { return a * b; }

// print_int.cpp
void dq_print_int(int value) { printf("%d\n", value); }
```

**Уровень 1** — составной модуль `calc` (граф из атомарных):

```
calc.dqgraph:
  [GraphInput "a":int] → a → [add] → result → sum → [multiply(sum, 2)] → result → [GraphOutput "result":int]
  [GraphInput "b":int] → b → [add] ↗
```

Сгенерированный `calc.c`:

```c
/* Сгенерировано из calc.dqgraph */
#include "add.h"
#include "multiply.h"

int dq_calc(int a, int b) {
    int sum = dq_add(a, b);
    int result = dq_multiply(sum, 2);
    return result;
}
```

**Уровень 2** — составной модуль `app` (граф с использованием `calc`):

```
app.dqgraph:
  [read_int "a"] → a → [calc] → result → value → [print_int]
  [read_int "b"] → b → [calc] ↗
```

Сгенерированный `app.c`:

```c
/* Сгенерировано из app.dqgraph */
#include "read_int.h"
#include "calc.h"
#include "print_int.h"

int main(int argc, char* argv[]) {
    int a = dq_read_int();
    int b = dq_read_int();
    int result = dq_calc(a, b);  /* Вызов составного модуля как обычной функции */
    dq_print_int(result);
    return 0;
}
```

#### Порядок компиляции

Компилятор определяет порядок генерации кода через анализ зависимостей между графами:

```cpp
class CompositeModuleCompiler {
public:
    // Компилирует все графы проекта в правильном порядке (листья → корень)
    CompilationResult compileAll(const Project& project,
                                  const ModuleRegistry& registry);

private:
    // Топологическая сортировка графов по зависимостям
    QList<Graph*> sortGraphsByDependency(const QList<Graph*>& graphs);

    // Обнаружение циклических зависимостей между графами
    bool detectCycles(const QList<Graph*>& graphs);

    // Компиляция одного графа в C-функцию + .dqmod
    GeneratedModule compileGraph(const Graph& graph,
                                  const ModuleRegistry& registry);
};

struct GeneratedModule {
    QString sourceCode;     // Сгенерированный .c файл
    QString headerCode;     // Сгенерированный .h файл
    Module dqmod;           // Описание модуля для регистрации
};
```

Порядок компиляции для примера выше:
```
1. add.cpp, multiply.cpp, read_int.cpp, print_int.cpp  (атомарные — уже есть)
2. calc.dqgraph → calc.c + calc.h + calc.dqmod          (зависит от add, multiply)
3. app.dqgraph → app.c                                   (зависит от calc, read_int, print_int)
```

#### Защита от рекурсии

Циклические зависимости (граф A использует граф B, а граф B использует граф A) **запрещены** и обнаруживаются при:

- Попытке перетащить блок на холст (IDE предупреждает)
- Сохранении графа (валидация)
- Компиляции проекта (ошибка с указанием цепочки зависимостей)

---

## Оптимизации генерируемого кода

### 1. Устранение мёртвого кода

Узлы, выходные порты которых не подключены и которые не имеют побочных эффектов, не включаются в генерируемый код.

### 2. Свёртка констант

Если все входы узла -- константы, результат вычисляется на этапе компиляции графа.

```
До:  var_result = dq_add(5, 3);
После: var_result = 8;
```

### 3. Устранение промежуточных переменных

Если переменная используется только один раз, она подставляется inline.

```
До:
    int var_n1 = dq_read_int();
    int var_n2 = dq_abs(var_n1);

После:
    int var_n2 = dq_abs(dq_read_int());
```

### 4. Пул строковых литералов

Одинаковые строковые литералы объединяются в одну константу.

---

## CodeGenerator API

```cpp
class CodeGenerator : public QObject {
    Q_OBJECT
public:
    explicit CodeGenerator(ModuleRegistry* registry,
                           LanguageBackendRegistry* langRegistry,
                           QObject* parent = nullptr);

    // === Язык ===

    // Активный языковой бэкенд (из настроек проекта)
    void setLanguage(const QString& langId);      // "c", "python", "rust"
    QString activeLanguage() const;
    LanguageBackend* activeBackend() const;

    // === Тип приложения ===

    void registerAppType(std::unique_ptr<AppTypeBackend> appType);
    QStringList availableAppTypes() const;
    void setActiveAppType(const QString& name);   // "console", "sdl2", "custom"

    // === Генерация ===

    // Полная генерация проекта: IR → LanguageBackend → файлы
    GenerationResult generate(const Project& project);

    // Генерация отдельного графа (для предпросмотра)
    GenerationResult generateGraph(const Graph& graph);

    // Генерация UI-макета
    GenerationResult generateUI(const UILayout& layout);

    // === Настройки ===

    void setOptimizationLevel(int level);  // 0-3
    void setDebugInfo(bool enable);
    void setComments(bool enable);

signals:
    void generationStarted();
    void generationProgress(int percent, const QString& stage);
    void generationFinished(bool success);
    void generationError(const QString& message);

private:
    ModuleRegistry* m_registry;
    LanguageBackendRegistry* m_langRegistry;
    QMap<QString, std::unique_ptr<AppTypeBackend>> m_appTypes;
    QString m_activeAppType;
    GraphCompiler m_graphCompiler;       // IR-генератор (язык-агностичный)
    int m_optimizationLevel;
    bool m_debugInfo;
    bool m_comments;
};

struct GenerationResult {
    bool success;
    GeneratedFiles files;
    QString targetLanguage;              // "c", "python", "rust"
    QString targetAppType;               // "console", "sdl2", "custom"
    QStringList errors;
    QStringList warnings;
    struct Statistics {
        int totalLines;
        int totalFiles;
        int nodesCompiled;
        int connectionsProcessed;
        qint64 generationTimeMs;
    } stats;
};
```

---

## Отладочная информация

При генерации с отладочной информацией в код добавляются маппинги:

```c
#line 1 "graphs/main.dqgraph:node_n1"
var_n1_result = dq_read_int("Введите a: ");
#line 2 "graphs/main.dqgraph:node_n2"
var_n2_result = dq_read_int("Введите b: ");
```

Также генерируется файл маппинга `.dqmap`:

```json
{
  "source": "generated/main.c",
  "mappings": [
    {"line": 15, "graph": "main.dqgraph", "node": "n1"},
    {"line": 18, "graph": "main.dqgraph", "node": "n2"},
    {"line": 21, "graph": "main.dqgraph", "node": "n3"},
    {"line": 24, "graph": "main.dqgraph", "node": "n4"}
  ]
}
```

Этот файл используется визуальным отладчиком для подсветки узлов в графе во время отладки.
