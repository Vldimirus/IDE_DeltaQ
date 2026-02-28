# DeltaQ IDE - Модуль 3: Дизайнер интерфейсов (UI Designer)

## Обзор

Модуль UI Designer предоставляет визуальный конструктор пользовательских интерфейсов для создаваемых приложений. Пользователь размещает виджеты методом drag&drop, настраивает свойства, задаёт компоновку (layout) и привязывает обработчики событий. Результат компилируется в код на C с использованием библиотеки SDL2.

---

## Архитектура модуля

```
┌────────────────────────────────────────────────────────┐
│                   UI Designer Module                    │
│                                                         │
│  ┌──────────────┐  ┌───────────────┐  ┌─────────────┐ │
│  │   Widget     │  │  Design       │  │  Property   │ │
│  │   Palette    │  │  Canvas       │  │  Editor     │ │
│  └──────┬───────┘  └───────┬───────┘  └──────┬──────┘ │
│         │                  │                  │         │
│  ┌──────┴──────────────────┴──────────────────┴──────┐ │
│  │              UIDesignerWidget                      │ │
│  └────────────────────────┬──────────────────────────┘ │
│                           │                             │
│  ┌────────────────────────┴──────────────────────────┐ │
│  │              Code Generation                       │ │
│  │  ┌──────────┐  ┌──────────────┐  ┌─────────────┐ │ │
│  │  │  Layout  │  │  SDL2 Code   │  │   Event     │ │ │
│  │  │  Engine  │  │  Generator   │  │   Binder    │ │ │
│  │  └──────────┘  └──────────────┘  └─────────────┘ │ │
│  └───────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────┘
```

---

## Drag&Drop виджеты

### Палитра виджетов

Виджеты организованы по категориям:

#### Категория: Контейнеры

| Виджет | Описание | SDL2 реализация |
|--------|----------|-----------------|
| **Window** | Главное окно | `SDL_CreateWindow` |
| **Panel** | Панель-контейнер | Прямоугольник с фоном |
| **ScrollPanel** | Панель с прокруткой | Область отсечения + скроллбар |
| **TabPanel** | Панель с вкладками | Набор кнопок + переключение панелей |
| **GroupBox** | Группа с заголовком | Рамка + текст |

#### Категория: Элементы ввода

| Виджет | Описание | SDL2 реализация |
|--------|----------|-----------------|
| **Button** | Кнопка | Прямоугольник + текст + обработка клика |
| **TextField** | Текстовое поле | Поле с курсором + обработка клавиатуры |
| **TextArea** | Многострочное поле | Многострочный TextField |
| **Checkbox** | Флажок | Квадрат + галочка + текст |
| **RadioButton** | Переключатель | Круг + точка + текст |
| **ComboBox** | Выпадающий список | Кнопка + выпадающий список |
| **Slider** | Ползунок | Полоса + бегунок |
| **SpinBox** | Числовое поле | TextField + кнопки +/- |

#### Категория: Отображение

| Виджет | Описание | SDL2 реализация |
|--------|----------|-----------------|
| **Label** | Текстовая метка | `SDL_RenderText` / `SDL_ttf` |
| **Image** | Изображение | `SDL_Texture` + `SDL_RenderCopy` |
| **ProgressBar** | Индикатор прогресса | Два прямоугольника |
| **Canvas** | Область рисования | Пользовательский рендеринг |
| **Table** | Таблица | Сетка + текст |
| **ListView** | Список | Вертикальный набор элементов |
| **TreeView** | Дерево | Иерархический список |

#### Категория: Меню и навигация

| Виджет | Описание | SDL2 реализация |
|--------|----------|-----------------|
| **MenuBar** | Строка меню | Горизонтальная панель + выпадающие меню |
| **ToolBar** | Панель инструментов | Горизонтальная панель + кнопки-иконки |
| **StatusBar** | Строка состояния | Панель внизу окна |

### Класс WidgetItem

```cpp
class WidgetItem : public QGraphicsObject {
    Q_OBJECT
    Q_PROPERTY(QRectF geometry READ geometry WRITE setGeometry)
public:
    enum WidgetType {
        Window, Panel, Button, TextField, Label,
        Checkbox, RadioButton, ComboBox, Slider,
        Image, ProgressBar, Table, ListView,
        ScrollPanel, TabPanel, GroupBox, MenuBar,
        ToolBar, StatusBar, TextArea, SpinBox,
        Canvas, TreeView
    };

    WidgetItem(WidgetType type, QGraphicsItem* parent = nullptr);

    // Идентификация
    QString widgetId() const;
    WidgetType widgetType() const;
    QString widgetName() const;
    void setWidgetName(const QString& name);

    // Геометрия
    QRectF geometry() const;
    void setGeometry(const QRectF& rect);

    // Свойства
    QVariant widgetProperty(const QString& name) const;
    void setWidgetProperty(const QString& name, const QVariant& value);
    QMap<QString, QVariant> allProperties() const;

    // Дочерние виджеты
    void addChild(WidgetItem* child);
    void removeChild(WidgetItem* child);
    QList<WidgetItem*> children() const;

    // Компоновка
    void setLayout(LayoutType layout);
    LayoutType layout() const;

    // События
    void bindEvent(const QString& event, const QString& handler);
    QMap<QString, QString> eventBindings() const;

    // Отрисовка
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    void drawButton(QPainter* painter);
    void drawTextField(QPainter* painter);
    void drawLabel(QPainter* painter);
    void drawCheckbox(QPainter* painter);
    // ... другие методы отрисовки

    QString m_id;
    WidgetType m_type;
    QString m_name;
    QRectF m_geometry;
    QMap<QString, QVariant> m_properties;
    QMap<QString, QString> m_eventBindings;
    LayoutType m_layoutType;
    QList<WidgetItem*> m_children;
};
```

---

## Редактор свойств (Property Editor)

### Структура панели свойств

```cpp
class PropertyEditor : public QWidget {
    Q_OBJECT
public:
    explicit PropertyEditor(CommandBus* commandBus, QWidget* parent = nullptr);

    void setWidget(WidgetItem* widget);
    void clear();

signals:
    void propertyChanged(const QString& widgetId, const QString& property,
                          const QVariant& value);

private:
    void buildPropertyList(WidgetItem* widget);
    QWidget* createEditor(const PropertyDef& prop);

    QScrollArea* m_scrollArea;
    QFormLayout* m_layout;
    WidgetItem* m_currentWidget;
    CommandBus* m_commandBus;
};
```

### Свойства виджетов

#### Общие свойства (все виджеты)

| Свойство | Тип | Описание |
|----------|-----|----------|
| `name` | string | Программное имя виджета |
| `x` | int | Координата X |
| `y` | int | Координата Y |
| `width` | int | Ширина |
| `height` | int | Высота |
| `visible` | bool | Видимость |
| `enabled` | bool | Активность |
| `tooltip` | string | Всплывающая подсказка |
| `background_color` | color | Цвет фона |
| `border_color` | color | Цвет рамки |
| `border_width` | int | Толщина рамки |
| `font_family` | string | Шрифт |
| `font_size` | int | Размер шрифта |
| `font_color` | color | Цвет текста |

#### Свойства Button

| Свойство | Тип | Описание |
|----------|-----|----------|
| `text` | string | Текст на кнопке |
| `icon` | string | Путь к иконке |
| `style` | enum | Стиль: normal, flat, toggle |

#### Свойства TextField

| Свойство | Тип | Описание |
|----------|-----|----------|
| `text` | string | Текст в поле |
| `placeholder` | string | Подсказка |
| `max_length` | int | Максимальная длина |
| `read_only` | bool | Только для чтения |
| `password` | bool | Режим пароля |

#### Свойства Label

| Свойство | Тип | Описание |
|----------|-----|----------|
| `text` | string | Текст метки |
| `alignment` | enum | Выравнивание: left, center, right |
| `word_wrap` | bool | Перенос слов |

#### Свойства Slider

| Свойство | Тип | Описание |
|----------|-----|----------|
| `min` | int | Минимальное значение |
| `max` | int | Максимальное значение |
| `value` | int | Текущее значение |
| `step` | int | Шаг |
| `orientation` | enum | Ориентация: horizontal, vertical |

#### Свойства ProgressBar

| Свойство | Тип | Описание |
|----------|-----|----------|
| `min` | int | Минимум |
| `max` | int | Максимум |
| `value` | int | Текущее значение |
| `show_text` | bool | Показывать процент |

---

## Система компоновки (Layout System)

### Типы компоновки

| Тип | Описание | Поведение |
|-----|----------|-----------|
| **None** | Абсолютное позиционирование | Виджеты размещаются по координатам |
| **HBox** | Горизонтальная компоновка | Виджеты размещаются слева направо |
| **VBox** | Вертикальная компоновка | Виджеты размещаются сверху вниз |
| **Grid** | Табличная компоновка | Виджеты размещаются в ячейках сетки |
| **Flow** | Поточная компоновка | Как HBox, но с переносом на следующую строку |

### Реализация Layout Engine

```cpp
class LayoutEngine {
public:
    struct LayoutConstraints {
        int margin = 5;
        int spacing = 5;
        int minWidth = 0;
        int minHeight = 0;
        int maxWidth = INT_MAX;
        int maxHeight = INT_MAX;
        Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignTop;
        int stretch = 0;  // Коэффициент растяжения (для HBox/VBox)
    };

    static void applyLayout(WidgetItem* container);

private:
    static void applyHBoxLayout(WidgetItem* container);
    static void applyVBoxLayout(WidgetItem* container);
    static void applyGridLayout(WidgetItem* container);
    static void applyFlowLayout(WidgetItem* container);
};

void LayoutEngine::applyVBoxLayout(WidgetItem* container) {
    auto children = container->children();
    if (children.isEmpty()) return;

    int margin = container->widgetProperty("layout_margin").toInt();
    int spacing = container->widgetProperty("layout_spacing").toInt();

    qreal availableWidth = container->geometry().width() - 2 * margin;
    qreal y = margin;

    // Подсчёт общего stretch
    int totalStretch = 0;
    qreal fixedHeight = 0;
    for (auto* child : children) {
        int stretch = child->widgetProperty("stretch").toInt();
        if (stretch > 0) {
            totalStretch += stretch;
        } else {
            fixedHeight += child->geometry().height();
        }
    }

    qreal remainingHeight = container->geometry().height() - 2 * margin
                            - fixedHeight - spacing * (children.size() - 1);

    for (auto* child : children) {
        QRectF childRect = child->geometry();
        childRect.setX(margin);
        childRect.setWidth(availableWidth);

        int stretch = child->widgetProperty("stretch").toInt();
        if (stretch > 0 && totalStretch > 0) {
            childRect.setHeight(remainingHeight * stretch / totalStretch);
        }

        childRect.setY(y);
        child->setGeometry(childRect);
        y += childRect.height() + spacing;
    }
}
```

### Визуальные подсказки компоновки

В режиме редактирования UI Designer показывает:

- Синие направляющие (guides) при перетаскивании виджетов
- Оранжевые рамки контейнеров с компоновкой
- Зелёные маркеры spacing между виджетами
- Пунктирные линии выравнивания

---

## Генерация кода SDL2

### SDL2 Code Generator

```cpp
class SDL2CodeGenerator : public QObject {
    Q_OBJECT
public:
    struct GeneratedCode {
        QString mainFile;        // main.c -- инициализация SDL2, главный цикл
        QString uiFile;          // ui.c -- создание и отрисовка виджетов
        QString uiHeaderFile;    // ui.h -- объявления виджетов
        QString eventsFile;      // events.c -- обработка событий
        QString eventsHeaderFile;// events.h -- объявления обработчиков
    };

    GeneratedCode generate(const UILayout& layout) const;

private:
    QString generateMainFile(const UILayout& layout) const;
    QString generateUIFile(const UILayout& layout) const;
    QString generateUIHeader(const UILayout& layout) const;
    QString generateEventsFile(const UILayout& layout) const;
    QString generateEventsHeader(const UILayout& layout) const;

    QString generateWidgetCode(const WidgetItem* widget) const;
    QString generateRenderCode(const WidgetItem* widget) const;
    QString generateEventHandlerCode(const WidgetItem* widget) const;
};
```

### Пример сгенерированного кода

#### Исходный макет (.dqui)

```json
{
  "id": "ui-001",
  "name": "calculator",
  "window": {
    "title": "Калькулятор",
    "width": 300,
    "height": 400,
    "children": [
      {
        "type": "TextField",
        "name": "display",
        "x": 10, "y": 10,
        "width": 280, "height": 40,
        "properties": {"read_only": true, "text": "0", "font_size": 24}
      },
      {
        "type": "Button",
        "name": "btn_1",
        "x": 10, "y": 60,
        "width": 65, "height": 50,
        "properties": {"text": "1"},
        "events": {"on_click": "on_digit_click"}
      }
    ]
  }
}
```

#### Сгенерированный main.c

```c
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "ui.h"
#include "events.h"

int main(int argc, char* argv[]) {
    // Инициализация SDL2
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    // Создание окна
    SDL_Window* window = SDL_CreateWindow(
        "Калькулятор",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        300, 400,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Создание рендерера
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    // Загрузка шрифта
    TTF_Font* font = TTF_OpenFont("fonts/default.ttf", 16);
    TTF_Font* font_large = TTF_OpenFont("fonts/default.ttf", 24);

    // Инициализация UI
    UIState ui;
    ui_init(&ui, renderer, font, font_large);

    // Главный цикл
    int running = 1;
    SDL_Event event;

    while (running) {
        // Обработка событий
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
            ui_handle_event(&ui, &event);
        }

        // Отрисовка
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        ui_render(&ui, renderer);

        SDL_RenderPresent(renderer);
    }

    // Очистка
    ui_cleanup(&ui);
    TTF_CloseFont(font);
    TTF_CloseFont(font_large);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
```

#### Сгенерированный ui.h

```c
#ifndef DQ_UI_H
#define DQ_UI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// Структуры виджетов
typedef struct {
    SDL_Rect rect;
    char text[256];
    int read_only;
    int focused;
    int cursor_pos;
    TTF_Font* font;
} DQ_TextField;

typedef struct {
    SDL_Rect rect;
    char text[64];
    int hovered;
    int pressed;
    TTF_Font* font;
    void (*on_click)(void* userdata);
    void* userdata;
} DQ_Button;

// Состояние UI
typedef struct {
    SDL_Renderer* renderer;
    TTF_Font* font_default;
    TTF_Font* font_large;

    // Виджеты
    DQ_TextField display;
    DQ_Button btn_1;
    // ... другие виджеты
} UIState;

// Функции
void ui_init(UIState* ui, SDL_Renderer* renderer,
             TTF_Font* font, TTF_Font* font_large);
void ui_handle_event(UIState* ui, SDL_Event* event);
void ui_render(UIState* ui, SDL_Renderer* renderer);
void ui_cleanup(UIState* ui);

#endif
```

#### Сгенерированный ui.c

```c
#include "ui.h"
#include "events.h"
#include <string.h>

void ui_init(UIState* ui, SDL_Renderer* renderer,
             TTF_Font* font, TTF_Font* font_large) {
    ui->renderer = renderer;
    ui->font_default = font;
    ui->font_large = font_large;

    // display (TextField)
    ui->display.rect = (SDL_Rect){10, 10, 280, 40};
    strcpy(ui->display.text, "0");
    ui->display.read_only = 1;
    ui->display.focused = 0;
    ui->display.cursor_pos = 0;
    ui->display.font = font_large;

    // btn_1 (Button)
    ui->btn_1.rect = (SDL_Rect){10, 60, 65, 50};
    strcpy(ui->btn_1.text, "1");
    ui->btn_1.hovered = 0;
    ui->btn_1.pressed = 0;
    ui->btn_1.font = font;
    ui->btn_1.on_click = on_digit_click;
    ui->btn_1.userdata = ui;
}

static void render_textfield(SDL_Renderer* renderer, DQ_TextField* tf) {
    // Фон
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &tf->rect);

    // Рамка
    SDL_SetRenderDrawColor(renderer, tf->focused ? 0 : 100,
                           tf->focused ? 122 : 100,
                           tf->focused ? 204 : 100, 255);
    SDL_RenderDrawRect(renderer, &tf->rect);

    // Текст
    if (strlen(tf->text) > 0) {
        SDL_Color color = {255, 255, 255, 255};
        SDL_Surface* surface = TTF_RenderUTF8_Blended(tf->font, tf->text, color);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

        SDL_Rect dst = {tf->rect.x + 5, tf->rect.y + 5,
                        surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &dst);

        SDL_DestroyTexture(texture);
        SDL_FreeSurface(surface);
    }
}

static void render_button(SDL_Renderer* renderer, DQ_Button* btn) {
    // Фон (зависит от состояния)
    if (btn->pressed) {
        SDL_SetRenderDrawColor(renderer, 0, 90, 158, 255);
    } else if (btn->hovered) {
        SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 45, 45, 45, 255);
    }
    SDL_RenderFillRect(renderer, &btn->rect);

    // Рамка
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderDrawRect(renderer, &btn->rect);

    // Текст (по центру)
    if (strlen(btn->text) > 0) {
        SDL_Color color = {255, 255, 255, 255};
        SDL_Surface* surface = TTF_RenderUTF8_Blended(btn->font, btn->text, color);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

        SDL_Rect dst = {
            btn->rect.x + (btn->rect.w - surface->w) / 2,
            btn->rect.y + (btn->rect.h - surface->h) / 2,
            surface->w, surface->h
        };
        SDL_RenderCopy(renderer, texture, NULL, &dst);

        SDL_DestroyTexture(texture);
        SDL_FreeSurface(surface);
    }
}

void ui_render(UIState* ui, SDL_Renderer* renderer) {
    render_textfield(renderer, &ui->display);
    render_button(renderer, &ui->btn_1);
    // ... другие виджеты
}

void ui_handle_event(UIState* ui, SDL_Event* event) {
    if (event->type == SDL_MOUSEMOTION) {
        SDL_Point pt = {event->motion.x, event->motion.y};

        ui->btn_1.hovered = SDL_PointInRect(&pt, &ui->btn_1.rect);
        // ... другие кнопки
    }

    if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT) {
        SDL_Point pt = {event->button.x, event->button.y};

        if (SDL_PointInRect(&pt, &ui->btn_1.rect)) {
            ui->btn_1.pressed = 1;
        }
        // ... другие кнопки
    }

    if (event->type == SDL_MOUSEBUTTONUP && event->button.button == SDL_BUTTON_LEFT) {
        SDL_Point pt = {event->button.x, event->button.y};

        if (ui->btn_1.pressed && SDL_PointInRect(&pt, &ui->btn_1.rect)) {
            if (ui->btn_1.on_click) {
                ui->btn_1.on_click(ui->btn_1.userdata);
            }
        }
        ui->btn_1.pressed = 0;
        // ... другие кнопки
    }
}

void ui_cleanup(UIState* ui) {
    // Освобождение ресурсов
}
```

---

## Привязка событий (Event Binding)

### Поддерживаемые события

| Виджет | Событие | Описание |
|--------|---------|----------|
| **Button** | `on_click` | Нажатие кнопки |
| **Button** | `on_hover` | Наведение мыши |
| **TextField** | `on_change` | Изменение текста |
| **TextField** | `on_submit` | Нажатие Enter |
| **TextField** | `on_focus` | Получение фокуса |
| **TextField** | `on_blur` | Потеря фокуса |
| **Checkbox** | `on_toggle` | Переключение |
| **Slider** | `on_change` | Изменение значения |
| **ComboBox** | `on_select` | Выбор элемента |
| **Window** | `on_resize` | Изменение размера |
| **Window** | `on_close` | Закрытие окна |
| **Any** | `on_key_down` | Нажатие клавиши |
| **Any** | `on_key_up` | Отпускание клавиши |
| **Any** | `on_mouse_down` | Нажатие мыши |
| **Any** | `on_mouse_up` | Отпускание мыши |
| **Any** | `on_mouse_move` | Движение мыши |

### Способы привязки обработчиков

#### 1. Привязка к функции модуля

В UI Designer пользователь может привязать событие к функции из любого модуля:

```json
{
  "type": "Button",
  "name": "btn_calculate",
  "events": {
    "on_click": {
      "type": "module_function",
      "module_id": "calc-uuid",
      "function": "dq_calculate",
      "args_mapping": {
        "a": "display.text"
      }
    }
  }
}
```

#### 2. Привязка к узлу графа

Событие может запускать выполнение графа начиная с определённого узла:

```json
{
  "events": {
    "on_click": {
      "type": "graph_trigger",
      "graph_id": "main-graph-uuid",
      "entry_node": "on_button_click_node"
    }
  }
}
```

#### 3. Привязка к пользовательской C-функции

```json
{
  "events": {
    "on_click": {
      "type": "custom_function",
      "function": "on_digit_click",
      "source": "handlers/calculator.c"
    }
  }
}
```

### Диалог привязки событий

```cpp
class EventBindingDialog : public QDialog {
    Q_OBJECT
public:
    EventBindingDialog(const QString& widgetName, const QString& eventName,
                       ModuleRegistry* registry, QWidget* parent = nullptr);

    EventBinding result() const;

private:
    void setupUI();
    void populateModules();
    void populateGraphs();

    QTabWidget* m_tabs;         // Вкладки: Модуль / Граф / Пользовательская функция
    QComboBox* m_moduleSelector;
    QComboBox* m_functionSelector;
    QComboBox* m_graphSelector;
    QComboBox* m_nodeSelector;
    QLineEdit* m_customFunction;
    QLineEdit* m_customSource;

    EventBinding m_result;
    ModuleRegistry* m_registry;
};
```

---

## Формат .dqui

```json
{
  "id": "uuid",
  "name": "main_window",
  "version": "1.0.0",

  "window": {
    "title": "My Application",
    "width": 800,
    "height": 600,
    "resizable": true,
    "min_width": 400,
    "min_height": 300,
    "background_color": "#1e1e1e",

    "children": [
      {
        "type": "Panel",
        "name": "toolbar_panel",
        "x": 0, "y": 0,
        "width": 800, "height": 40,
        "layout": "HBox",
        "properties": {
          "background_color": "#2d2d2d",
          "layout_spacing": 5,
          "layout_margin": 5
        },
        "children": [
          {
            "type": "Button",
            "name": "btn_new",
            "width": 80, "height": 30,
            "properties": {"text": "Новый", "icon": "icons/new.png"},
            "events": {"on_click": "on_new_file"}
          },
          {
            "type": "Button",
            "name": "btn_open",
            "width": 80, "height": 30,
            "properties": {"text": "Открыть", "icon": "icons/open.png"},
            "events": {"on_click": "on_open_file"}
          }
        ]
      },
      {
        "type": "Panel",
        "name": "content_panel",
        "x": 0, "y": 40,
        "width": 800, "height": 530,
        "layout": "VBox",
        "properties": {
          "layout_spacing": 0,
          "layout_margin": 10
        },
        "children": []
      },
      {
        "type": "StatusBar",
        "name": "status_bar",
        "x": 0, "y": 570,
        "width": 800, "height": 30,
        "properties": {"text": "Готово"}
      }
    ]
  },

  "resources": {
    "fonts": {
      "default": "fonts/default.ttf",
      "large": "fonts/default.ttf"
    },
    "icons": [
      "icons/new.png",
      "icons/open.png"
    ]
  },

  "metadata": {
    "created": "2026-01-25T10:00:00Z",
    "modified": "2026-02-28T12:00:00Z",
    "designer_zoom": 1.0
  }
}
```

---

## Предпросмотр (Preview)

UI Designer поддерживает режим предпросмотра, который запускает SDL2-рендеринг макета в отдельном окне:

```cpp
class UIPreview : public QObject {
    Q_OBJECT
public:
    explicit UIPreview(QObject* parent = nullptr);

    void startPreview(const UILayout& layout);
    void stopPreview();
    bool isRunning() const;

signals:
    void previewStarted();
    void previewStopped();
    void previewError(const QString& message);

private:
    void compileAndRun(const UILayout& layout);

    QProcess* m_previewProcess;
    SDL2CodeGenerator m_codeGen;
};
```

Предпросмотр:
1. Генерирует временный C-код из текущего макета
2. Компилирует его с SDL2
3. Запускает полученный исполняемый файл
4. Пользователь может взаимодействовать с интерфейсом
5. По закрытии окна предпросмотра временные файлы удаляются
