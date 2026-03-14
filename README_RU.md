# DeltaQ IDE

[![Лицензия: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Qt 6](https://img.shields.io/badge/Qt-6-green.svg)](https://www.qt.io/)
[![Платформа: Linux](https://img.shields.io/badge/Platform-Linux-lightgrey.svg)]()

**Linux-first IDE для модульных C/C++ workflow, объединяющая редактор кода, композицию графов и SDL2-дизайн интерфейсов.**

> **[English version](README.md)**

---

## Что такое DeltaQ

DeltaQ IDE построена вокруг одного прозрачного workflow:

`модуль -> граф -> generated C code -> build -> run`

Текущая цель проекта намеренно уже, чем абстрактное обещание "универсальной IDE на все случаи". DeltaQ фокусируется на **модульной разработке C/C++ приложений под Linux**, где редактор кода, композиция графов, generated code, UI-раскладка и build/debug работают внутри одного обозримого toolchain.

Сейчас репозиторий держится на трёх продуктовых принципах:

- одна модель модуля для кода, графов, UI-контрактов и imported pack-ов;
- generated C остаётся видимым и прозрачно связанным с source-of-truth;
- шаблоны и примеры лежат в репозитории как готовые деревья исходников, а не синтезируются логикой IDE на лету.

## Текущий scope

- **Основная платформа:** Linux. В репозитории уже есть Linux CI, Linux release bundle, Linux tarball packaging и проверенный Linux AppImage flow.
- **Самые сильные встроенные сценарии:** console flow, desktop/UI flow, reusable composition и imported-pack integration.
- **Граница текущего релиза:** Windows и macOS находятся в roadmap и не должны восприниматься как уже закрытый delivery scope.

## С чего начать

Если нужно, чтобы DeltaQ читалась как один цельный Linux-first продукт, а не набор отдельных подсистем, используйте такие точки входа:

- [docs/onboarding/README_RU.md](docs/onboarding/README_RU.md) — путь нового пользователя от first run к более сильным showcase-примерам.
- [resources/examples/README_RU.md](resources/examples/README_RU.md) — каталог examples, рекомендуемый порядок и пояснение, что доказывает каждый checked-in проект.
- [docs/release/linux_first_release_checklist_ru.md](docs/release/linux_first_release_checklist_ru.md) — ручной путь приёмки Linux tarball/AppImage artifacts.
- [docs/release/linux_project_export_checklist_ru.md](docs/release/linux_project_export_checklist_ru.md) — путь handoff-проверки для Linux-приложений, экспортированных из проектов DeltaQ.

---

## Возможности

- **Редактор кода** — полнофункциональный редактор C/C++ с подсветкой синтаксиса, интеграцией LSP (clangd), автодополнением, поиском и заменой, переходом к определению и подсветкой парных скобок
- **Визуальный блочный редактор** — графовый редактор, где модули соединяются в программу визуально; графы компилируются в чистый C-код через топологическую сортировку и генерацию IR
- **Дизайнер UI** — конструктор интерфейсов с drag & drop на основе SDL2; проектируйте окна визуально, привязывайте события к обработчикам-графам и генерируйте компилируемый C-код
- **Обработчик библиотек** — импорт существующих C/C++ библиотек через парсинг AST с помощью libclang; автоматическая декомпозиция функций и классов в переиспользуемые модули
- **Модульная система** — всё является модулем (`.dqmod`). Модули вкладываются рекурсивно (принцип матрёшки): граф — это модуль, модуль может содержать граф. В репозитории сейчас 43 модуля стандартной библиотеки в 7 категориях
- **Шаблоны проектов** — 6 файловых стартовых шаблонов загружаются из `resources/templates/` и копируются в новый проект как готовое дерево исходников
- **Встроенный отладчик** — интеграция с GDB через MI-протокол: точки останова, пошаговое выполнение, инспекция переменных, стек вызовов и визуальная отладка прямо на графе
- **Система сборки** — конвейер сборки на CMake с парсингом вывода компилятора, навигацией к ошибкам и сборкой/запуском в один клик

---

## Архитектура

DeltaQ IDE построена на **4-слойной архитектуре** с центральной шиной команд:

```
┌─────────────────────────────────────────────────┐
│              Слой представления                  │
│  ┌───────────┬──────────────┬──────────────────┐ │
│  │ Редактор  │   Блочный   │   Дизайнер UI    │ │
│  │   кода    │  редактор   │                  │ │
│  └───────────┴──────────────┴──────────────────┘ │
├─────────────────────────────────────────────────┤
│              Слой приложения                     │
│    CommandBus · UndoManager · ActionManager      │
│    SessionManager · ModuleRegistry               │
├─────────────────────────────────────────────────┤
│              Слой сервисов                       │
│    GraphCompiler · BuildManager · LSPClient      │
│    SDL2CodeGenerator · LibclangParser            │
├─────────────────────────────────────────────────┤
│              Слой данных                         │
│    ProjectManager · GraphStore · UILayoutStore   │
│    ModuleStore · FileSystem                      │
└─────────────────────────────────────────────────┘
```

Все операции проходят через **CommandBus**, обеспечивая полную поддержку undo/redo во всех редакторах.

---

## Быстрый старт

Ниже описан **Linux-first** путь сборки и релиза. Теоретически проект может собираться и на других платформах, но Windows/macOS пока не оформлены как завершённые delivery targets.

### Зависимости

```bash
# Ubuntu / Debian
sudo apt install \
    cmake ninja-build g++ \
    qt6-base-dev \
    libqscintilla2-qt6-dev \
    libclang-dev \
    libsdl2-dev \
    libsdl2-ttf-dev \
    gdb clangd

# Fedora
sudo dnf install \
    cmake ninja-build gcc-c++ \
    qt6-qtbase-devel \
    qscintilla-qt6-devel \
    clang-devel \
    SDL2-devel \
    SDL2_ttf-devel \
    gdb clang-tools-extra
```

**Опциональные зависимости:**
- `libqscintilla2-qt6-dev` — продвинутый редактор кода (без него используется QPlainTextEdit)
- `libclang-dev` — импорт/парсинг библиотек (обработчик библиотек отключается без него)
- `libsdl2-dev` и `libsdl2-ttf-dev` — нужны только для сборки сгенерированных UI-проектов
- `ninja-build` — рекомендуемый generator, который используют checked-in CMake presets и release workspace
- `clangd` — LSP-сервер для интеллектуальных подсказок
- `gdb` — бэкенд отладчика

### Сборка

```bash
git clone https://github.com/Vldimirus/IDE_DeltaQ.git
cd IDE_DeltaQ

cmake --preset qt-dev
cmake --build --preset qt-dev -j$(nproc)
```

### Запуск

```bash
./build/qt-dev/src/deltaq
```

### Опции сборки

| Опция | По умолчанию | Описание |
|-------|-------------|----------|
| `DQ_BUILD_TESTS` | `ON` | Собирать тесты |
| `DQ_USE_LSP` | `ON` | Включить LSP-клиент (требуется clangd) |
| `DQ_USE_LIBCLANG` | `ON` | Включить интеграцию с libclang |

```bash
# Пример: сборка без тестов
cmake --preset qt-dev -DDQ_BUILD_TESTS=OFF
```

### Структура сборок

- `build/qt-dev/` — локальная dev-сборка для Qt Creator и shell
- `build/release/` — release workspace, который создаёт `scripts/build_release.sh`
- `build/ci-linux/` — отдельный CI workspace

В репозитории теперь есть `CMakePresets.json`, поэтому Qt Creator можно направить на preset `qt-dev`, и корень `build/` остаётся контейнером для именованных сборок, а не свалкой из `CMakeFiles/`, `src/` и `tests/`.

---

## Использование

Типичный рабочий процесс в DeltaQ IDE:

1. **Создать проект** — Файл → Новый проект, выбрать один из 6 файловых шаблонов: Console Hello World, Console Counter Until Q, Desktop Empty Window, Desktop UI Graph Example, Desktop Text Editor или Desktop Multi Window Workspace
2. **Написать модули** — создать C-функции с аннотациями `@dqmodule` или использовать Менеджер модулей для написания и тестирования модулей с мгновенным превью
3. **Построить граф** — открыть блочный редактор, перетащить модули из палитры и соединить их порты для определения логики программы
4. **Спроектировать UI** *(Desktop-проекты)* — открыть дизайнер UI, разместить виджеты (кнопки, текстовые поля, слайдеры...), задать свойства и привязать события к обработчикам-графам
5. **Собрать и запустить** — нажать Build (Ctrl+B) для компиляции графа в C-код, генерации CMakeLists.txt и создания исполняемого файла; затем Run (Ctrl+R)
6. **Отладить** — расставить точки останова (F9) и запустить отладку (F5); отладчик подсвечивает активный узел на графе и показывает значения переменных на портах

## Первый запуск

Если нужен самый короткий воспроизводимый путь знакомства с DeltaQ, начинайте с:

- `resources/examples/minimal_console_flow/minimal_console_flow.dqproj`

Откройте `graphs/main.dqgraph`, соберите проект, запустите его, введите `DeltaQ`, а затем посмотрите на сгенерированный `src/main.c`.

Этот пример специально сделан маленьким и показывает главный путь:

`модуль -> граф -> generated C code -> build -> run`

Подробное пошаговое руководство лежит в:

- `docs/onboarding/first_run_ru.md`

Если нужен не один walkthrough, а полный путь нового пользователя, переходите дальше:

- [docs/onboarding/README_RU.md](docs/onboarding/README_RU.md)
- [resources/examples/README_RU.md](resources/examples/README_RU.md)

Если вы проверяете не source checkout, а packaged Linux artifact, используйте:

- [docs/release/linux_first_release_checklist_ru.md](docs/release/linux_first_release_checklist_ru.md)

## Шаблоны проектов и примеры

Теперь DeltaQ загружает шаблоны проектов напрямую из файлов в `resources/templates/`, а не генерирует стартовые исходники внутри IDE.

- **Console Hello World** — минимальное консольное приложение, которое печатает `Hello, world!` и завершает работу
- **Console Counter Until Q** — консольный цикл, печатающий возрастающий счётчик до нажатия `q`
- **Desktop Empty Window** — SDL2-приложение, открывающее пустое окно
- **Desktop UI Graph Example** — desktop-проект с готовым графом, UI-раскладкой и сгенерированными SDL2 runtime-файлами
- **Desktop Text Editor** — DeltaQ desktop text-pad starter с графом, UI layout, editable text area и кастомными UI event handlers
- **Desktop Multi Window Workspace** — рабочая область с дочерними окнами внутри главного окна

Сейчас в репозитории есть 5 готовых проектов-примеров в `resources/examples/`:

- `minimal_console_flow` — самый короткий onboarding-путь для `модуль -> граф -> generated C code -> build -> run`
- `desktop_ui_flow` — desktop/UI showcase с generated SDL2 runtime и живыми event handlers
- `reusable_composition_console` — подмодули и повторное использование в одном корневом графе
- `imported_pack_sensor_console` — curated imported pack путь от внешней библиотеки до рабочего graph/runtime
- `imported_pack_checksum_console` — второй curated imported-pack путь для algorithmic/text-processing расширения

Полный каталог, рекомендуемый порядок и различие между user-facing examples и fixture SDK trees зафиксированы в:

- [resources/examples/README_RU.md](resources/examples/README_RU.md)

## Почему стандартная библиотека важна

Checked-in библиотека `core` нужна не для того, чтобы выиграть количеством модулей. Её задача — убрать повторяющийся glue-code в первых полезных сценариях.

- `minimal_console_flow` показывает, что `core.io.string_constant`, `core.io.read_line` и `core.io.println` уже достаточно, чтобы собрать реальный интерактивный console flow без написания стартовых пользовательских модулей.
- `reusable_composition_console` показывает, что `core.string.str_concat`, `core.string.str_length`, `core.conversion.int_to_string` и `core.io.println` уже позволяют собирать переиспользуемые составные модули вместо одноразового helper-кода.
- `desktop_ui_flow` показывает, что `core.desktop.*` уже выражает SDL2 lifecycle как graphable building blocks, поэтому граф остаётся на уровне логики приложения, а не ручного init/event-loop/teardown boilerplate.

Подробная раскладка по сценариям зафиксирована в:

- `docs/library/value_proof.md`
- `docs/library/README.md`

---

## Модульная система

DeltaQ построена на **модуле-центричной архитектуре**. Каждая функция — это модуль (`.dqmod`) с типизированными портами ввода/вывода.

- **Стандартная библиотека** — 43 модуля в категориях `control`, `conversion`, `desktop`, `io`, `logic`, `math` и `string`; устанавливаются в `~/.deltaq/modules/`
- **Локальные модули** — модули проекта в каталоге `dqmods/`
- **Подмодули (матрёшка)** — выделите узлы на графе → «Создать подмодуль» → выделение становится переиспользуемым составным модулем со своим внутренним графом. Вложенность неограничена
- **UI-модули** — UI-виджеты (Button, Label, Slider...) отображаются как модули со входами-свойствами и выходами-событиями
- **Импорт библиотек** — импорт C/C++ заголовков через libclang → функции/классы автоматически декомпозируются в модули
- **Центр library docs** — `docs/library/README.md` теперь является общей точкой входа в curation standard library, verification story и imported pack guides

---

## Технологический стек

| Компонент | Технология |
|-----------|-----------|
| Язык | C++20 |
| GUI-фреймворк | Qt 6 |
| Редактор кода | QScintilla (fallback: QPlainTextEdit) |
| Система сборки | CMake 3.20+ |
| Генерируемый UI | SDL2 |
| Парсинг C/C++ | libclang |
| LSP-сервер | clangd |
| Отладчик | GDB (MI-протокол) |
| Компилятор графов | Собственный IR → генерация C-кода |

---

## Тесты

В репозитории сейчас 48 файлов тестов, покрывающих `core`, `editor`, `uiDesigner`, `blockEditor`, `libProcessor`, `codegen`, `lsp` и `debug`.

```bash
ctest --preset qt-dev
```

Или запуск конкретного теста:

```bash
./build/qt-dev/tests/test_CommandBus
```

В репозитории также есть Linux CI baseline в `.github/workflows/ci.yml`: GitHub Actions выполняет полный `configure -> build -> ctest` на Ubuntu и дополнительно проверяет install/package smoke для самодостаточного bundle-layout, включая translation payload и bundled templates/examples.

---

## Дорожная карта

- [x] Linux CI baseline (GitHub Actions: полная сборка, 48 тестов, install/package smoke)
- [ ] Кроссплатформенность (Windows, macOS)
- [x] Linux AppImage packaging
- [ ] Windows Installer / macOS DMG packaging
- [ ] Бэкенды Python и Rust (генерация IR → Python/Rust-код)
- [ ] Система плагинов для сторонних расширений
- [ ] Оптимизация производительности для больших графов (100+ узлов)
- [ ] Пользовательская документация и обучающие материалы
- [x] Примеры проектов

---

## Участие в разработке

Приветствуются любые вклады в проект! Как помочь:

1. **Форкните** репозиторий
2. **Создайте ветку** для вашей фичи (`git checkout -b feature/my-feature`)
3. **Закоммитьте** изменения
4. **Отправьте** ветку (`git push origin feature/my-feature`)
5. **Откройте Pull Request**

Убедитесь, что:
- Код компилируется без предупреждений
- Все существующие тесты проходят (`ctest --output-on-failure`)
- Новая функциональность покрыта тестами

---

## Лицензия

Проект лицензирован под **GNU General Public License v3.0** — подробности в файле [LICENSE](LICENSE).

```
Copyright (c) 2024-2026 Vladimir Kononenko
```

---

## Благодарности

- [Qt Project](https://www.qt.io/) — GUI-фреймворк
- [QScintilla](https://riverbankcomputing.com/software/qscintilla/) — компонент редактора кода
- [LLVM/Clang](https://clang.llvm.org/) — парсинг C/C++ и LSP
- [SDL2](https://www.libsdl.org/) — целевая платформа для генерируемого UI
- [CMake](https://cmake.org/) — система сборки
