# План: Модульная система DeltaQ IDE — полная доработка

> Реализован 2026-03-01

## Контекст

Модуль — законченная функция/процедура, представленная пользователю как графический блок. Программа строится соединением блоков на графовом холсте. При компиляции — сборщик собирает весь код в единый файл с дедупликацией includes и определений.

## Реализованные этапы

### Этап 1: Исправление соединений между модулями

- **1.1** `connectBlockEditorSignals(BlockEditorWidget*)` — подключение сигналов nodeDropped, connectionRequested, nodeMovedByUser для вкладочных BlockEditorWidget
- **1.2** ScrollHandDrag → NoDrag, панорамирование средней кнопкой + Space+ЛКМ через eventFilter
- **1.3** Увеличение зоны попадания порта: NormalRadius 6→8, HoverRadius 8→11, portItemAt() с областью 28×28px

### Этап 2: Расширение Module struct

- Поля: `includes` (QStringList), `testStatus` (passed/failed/untested/modified), `sourceCode` (тело функции)
- Обновлены `toJson()` / `fromJson()` для сериализации

### Этап 3: Полная сборка в GraphCompiler

- `IR::moduleSources` — тела функций модулей (дедуплицированные)
- `emitCCode()` генерирует 3 секции: includes → определения → main()
- `GraphCompiler::generateIR()` собирает уникальные moduleId, их includes и sourceCode

### Этап 4: ModuleTestRunner + ModuleManagerWidget

- `ModuleTestRunner`: compile(), generateTestHarness(), runTest(), executeTest(), parseOutput()
- `ModuleManagerWidget`: библиотека + редактор + компиляция + тестирование

### Этап 5: Мультиязычная фильтрация

- `ModulePalette::setLanguageFilter()` — C и C++ совместимы
- `ModuleManagerWidget` — QComboBox фильтр по языку
- `GraphCompiler` — проверка совместимости языков

### Этап 6: UI-виджеты как модули

- `UIModuleFactory` — 8 модулей (Button, TextField, Label, Slider, Checkbox, ProgressBar, Image, ComboBox)
- Регистрация при инициализации, категория "ui" в палитре
- GraphCompiler: SDL2-специфичная генерация для UI-модулей
