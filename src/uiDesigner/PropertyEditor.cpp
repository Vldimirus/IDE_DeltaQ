// Редактор свойств виджета — реализация
#include "PropertyEditor.h"
#include "WidgetItem.h"
#include "DesignScene.h"

#include <deltaq/UILayout.h>

#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QColorDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QFrame>

namespace DeltaQ {

PropertyEditor::PropertyEditor(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(2, 2, 2, 2);

    auto *titleLabel = new QLabel(tr("Properties"), this);
    titleLabel->setStyleSheet("font-weight: bold; padding: 4px;");
    mainLayout->addWidget(titleLabel);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    mainLayout->addWidget(m_scrollArea);

    m_contentWidget = new QWidget;
    m_formLayout = new QFormLayout(m_contentWidget);
    m_formLayout->setLabelAlignment(Qt::AlignRight);
    m_formLayout->setContentsMargins(4, 4, 4, 4);
    m_formLayout->setSpacing(4);
    m_scrollArea->setWidget(m_contentWidget);
}

void PropertyEditor::setWidget(WidgetItem *widget)
{
    m_currentWidget = widget;
    buildPropertyList();
}

void PropertyEditor::clearWidget()
{
    m_currentWidget = nullptr;
    m_windowScene = nullptr;
    clearLayout();
}

void PropertyEditor::setWindowProperties(DesignScene *scene)
{
    m_currentWidget = nullptr;
    m_windowScene = scene;
    buildWindowPropertyList();
}

void PropertyEditor::buildWindowPropertyList()
{
    clearLayout();
    if (!m_windowScene) return;

    m_updating = true;

    addSectionHeader(tr("Window"));
    addProperty(tr("Type"), "type", "Window", "readonly");

    // Заголовок окна
    auto *titleEdit = new QLineEdit(m_windowScene->windowTitle(), m_contentWidget);
    connect(titleEdit, &QLineEdit::editingFinished, this, [this, titleEdit]() {
        if (m_updating || !m_windowScene) return;
        m_windowScene->setWindowTitle(titleEdit->text());
        emit windowPropertyChanged();
    });
    m_formLayout->addRow(tr("Title:"), titleEdit);

    // Ширина окна
    auto *widthSb = new QDoubleSpinBox(m_contentWidget);
    widthSb->setRange(200, 4000);
    widthSb->setDecimals(0);
    widthSb->setValue(m_windowScene->windowRect().width());
    connect(widthSb, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double val) {
        if (m_updating || !m_windowScene) return;
        QRectF r = m_windowScene->windowRect();
        m_windowScene->setWindowRect(QRectF(r.x(), r.y(), val, r.height()));
        emit windowPropertyChanged();
    });
    m_formLayout->addRow(tr("Width:"), widthSb);

    // Высота окна
    auto *heightSb = new QDoubleSpinBox(m_contentWidget);
    heightSb->setRange(150, 4000);
    heightSb->setDecimals(0);
    heightSb->setValue(m_windowScene->windowRect().height());
    connect(heightSb, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double val) {
        if (m_updating || !m_windowScene) return;
        QRectF r = m_windowScene->windowRect();
        m_windowScene->setWindowRect(QRectF(r.x(), r.y(), r.width(), val));
        emit windowPropertyChanged();
    });
    m_formLayout->addRow(tr("Height:"), heightSb);

    m_updating = false;
}

void PropertyEditor::buildPropertyList()
{
    clearLayout();
    if (!m_currentWidget) return;

    m_updating = true;

    // Общие свойства
    addSectionHeader(tr("General"));
    addProperty(tr("Name"), "name", m_currentWidget->widgetName(), "readonly");
    addProperty(tr("Type"), "type", m_currentWidget->widgetType(), "readonly");
    addProperty("X", "x", m_currentWidget->pos().x(), "double");
    addProperty("Y", "y", m_currentWidget->pos().y(), "double");
    addProperty(tr("Width"), "width", m_currentWidget->widgetWidth(), "double");
    addProperty(tr("Height"), "height", m_currentWidget->widgetHeight(), "double");

    // Визуальные свойства
    addSectionHeader(tr("Appearance"));
    addProperty(tr("Visible"), "visible",
                m_currentWidget->property("visible").isValid() ?
                m_currentWidget->property("visible") : true, "bool");
    addProperty(tr("Enabled"), "enabled",
                m_currentWidget->property("enabled").isValid() ?
                m_currentWidget->property("enabled") : true, "bool");

    // Типоспецифичные свойства
    QString type = m_currentWidget->widgetType();

    if (type == "Button") {
        addSectionHeader(tr("Button"));
        addProperty(tr("Text"), "text",
                    m_currentWidget->property("text").isValid() ?
                    m_currentWidget->property("text") : m_currentWidget->widgetName());
    }
    else if (type == "Label") {
        addSectionHeader(tr("Label"));
        addProperty(tr("Text"), "text",
                    m_currentWidget->property("text").isValid() ?
                    m_currentWidget->property("text") : m_currentWidget->widgetName());
        addProperty(tr("Alignment"), "alignment",
                    m_currentWidget->property("alignment").isValid() ?
                    m_currentWidget->property("alignment") : "left", "alignment");
        addProperty(tr("Word Wrap"), "word_wrap",
                    m_currentWidget->property("word_wrap").isValid() ?
                    m_currentWidget->property("word_wrap") : false, "bool");
    }
    else if (type == "TextField" || type == "TextArea") {
        addSectionHeader(tr("Text Field"));
        addProperty(tr("Text"), "text",
                    m_currentWidget->property("text").isValid() ?
                    m_currentWidget->property("text") : "");
        addProperty(tr("Placeholder"), "placeholder",
                    m_currentWidget->property("placeholder").isValid() ?
                    m_currentWidget->property("placeholder") : "");
        addProperty(tr("Max Length"), "max_length",
                    m_currentWidget->property("max_length").isValid() ?
                    m_currentWidget->property("max_length") : 0, "int");
        addProperty(tr("Read Only"), "read_only",
                    m_currentWidget->property("read_only").isValid() ?
                    m_currentWidget->property("read_only") : false, "bool");
        addProperty(tr("Password"), "password",
                    m_currentWidget->property("password").isValid() ?
                    m_currentWidget->property("password") : false, "bool");
    }
    else if (type == "Checkbox" || type == "RadioButton") {
        addSectionHeader(type == "Checkbox" ? tr("Checkbox") : tr("Radio Button"));
        addProperty(tr("Text"), "text",
                    m_currentWidget->property("text").isValid() ?
                    m_currentWidget->property("text") : m_currentWidget->widgetName());
        addProperty(tr("Checked"), type == "Checkbox" ? "checked" : "selected",
                    m_currentWidget->property(type == "Checkbox" ? "checked" : "selected")
                    .isValid() ? m_currentWidget->property(
                    type == "Checkbox" ? "checked" : "selected") : false, "bool");
    }
    else if (type == "Slider") {
        addSectionHeader(tr("Slider"));
        addProperty(tr("Min"), "min",
                    m_currentWidget->property("min").isValid() ?
                    m_currentWidget->property("min") : 0, "int");
        addProperty(tr("Max"), "max",
                    m_currentWidget->property("max").isValid() ?
                    m_currentWidget->property("max") : 100, "int");
        addProperty(tr("Value"), "value",
                    m_currentWidget->property("value").isValid() ?
                    m_currentWidget->property("value") : 50, "int");
        addProperty(tr("Step"), "step",
                    m_currentWidget->property("step").isValid() ?
                    m_currentWidget->property("step") : 1, "int");
    }
    else if (type == "ProgressBar") {
        addSectionHeader(tr("Progress Bar"));
        addProperty(tr("Min"), "min",
                    m_currentWidget->property("min").isValid() ?
                    m_currentWidget->property("min") : 0, "int");
        addProperty(tr("Max"), "max",
                    m_currentWidget->property("max").isValid() ?
                    m_currentWidget->property("max") : 100, "int");
        addProperty(tr("Value"), "value",
                    m_currentWidget->property("value").isValid() ?
                    m_currentWidget->property("value") : 40, "int");
        addProperty(tr("Show Text"), "show_text",
                    m_currentWidget->property("show_text").isValid() ?
                    m_currentWidget->property("show_text") : true, "bool");
    }
    else if (type == "ComboBox") {
        addSectionHeader(tr("Combo Box"));
        addProperty(tr("Text"), "text",
                    m_currentWidget->property("text").isValid() ?
                    m_currentWidget->property("text") : m_currentWidget->widgetName());
    }

    // Секция anchor-привязок
    addSectionHeader(tr("Anchors"));
    {
        UIAnchors a = m_currentWidget->anchors();

        auto addAnchorCheckbox = [&](const QString &label, bool checked, auto setter) {
            auto *cb = new QCheckBox(label, m_contentWidget);
            cb->setChecked(checked);
            connect(cb, &QCheckBox::toggled, this, [this, setter](bool on) {
                if (m_updating || !m_currentWidget) return;
                UIAnchors anch = m_currentWidget->anchors();
                setter(anch, on);
                m_currentWidget->setAnchors(anch);
                emit anchorsChanged(m_currentWidget->widgetId());
            });
            m_formLayout->addRow(cb);
        };

        addAnchorCheckbox(tr("Left"), a.left,
            [](UIAnchors &a, bool v) { a.left = v; if (v) a.hCenter = false; });
        addAnchorCheckbox(tr("Right"), a.right,
            [](UIAnchors &a, bool v) { a.right = v; if (v) a.hCenter = false; });
        addAnchorCheckbox(tr("Top"), a.top,
            [](UIAnchors &a, bool v) { a.top = v; if (v) a.vCenter = false; });
        addAnchorCheckbox(tr("Bottom"), a.bottom,
            [](UIAnchors &a, bool v) { a.bottom = v; if (v) a.vCenter = false; });
        addAnchorCheckbox(tr("H Center"), a.hCenter,
            [](UIAnchors &a, bool v) { a.hCenter = v; if (v) { a.left = false; a.right = false; } });
        addAnchorCheckbox(tr("V Center"), a.vCenter,
            [](UIAnchors &a, bool v) { a.vCenter = v; if (v) { a.top = false; a.bottom = false; } });

        // Margin-спинбоксы для активных привязок
        auto addMarginSpin = [&](const QString &label, qreal value, bool visible, auto setter) {
            if (!visible) return;
            auto *sb = new QDoubleSpinBox(m_contentWidget);
            sb->setRange(0, 9999);
            sb->setDecimals(0);
            sb->setValue(value);
            connect(sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    this, [this, setter](double val) {
                if (m_updating || !m_currentWidget) return;
                UIAnchors anch = m_currentWidget->anchors();
                setter(anch, val);
                m_currentWidget->setAnchors(anch);
                emit anchorsChanged(m_currentWidget->widgetId());
            });
            m_formLayout->addRow(label + ":", sb);
        };

        addMarginSpin(tr("Left margin"), a.leftMargin, a.left,
            [](UIAnchors &a, double v) { a.leftMargin = v; });
        addMarginSpin(tr("Right margin"), a.rightMargin, a.right,
            [](UIAnchors &a, double v) { a.rightMargin = v; });
        addMarginSpin(tr("Top margin"), a.topMargin, a.top,
            [](UIAnchors &a, double v) { a.topMargin = v; });
        addMarginSpin(tr("Bottom margin"), a.bottomMargin, a.bottom,
            [](UIAnchors &a, double v) { a.bottomMargin = v; });
    }

    // Секция событий
    addSectionHeader(tr("Events"));
    auto events = m_currentWidget->events();
    if (events.isEmpty()) {
        auto *label = new QLabel(tr("No events bound"), m_contentWidget);
        label->setStyleSheet("color: gray; font-style: italic;");
        m_formLayout->addRow(label);
    } else {
        for (auto it = events.begin(); it != events.end(); ++it) {
            auto *label = new QLabel(it.key() + ": " + it.value(), m_contentWidget);
            m_formLayout->addRow(label);
        }
    }

    auto *evBtn = new QPushButton(tr("Bind Event..."), m_contentWidget);
    connect(evBtn, &QPushButton::clicked, this, [this]() {
        if (m_currentWidget)
            emit eventBindRequested(m_currentWidget->widgetId());
    });
    m_formLayout->addRow(evBtn);

    m_updating = false;
}

void PropertyEditor::addSectionHeader(const QString &title)
{
    auto *header = new QLabel(title, m_contentWidget);
    header->setStyleSheet("font-weight: bold; background: #3a3a3a; padding: 3px 6px; margin-top: 4px;");
    m_formLayout->addRow(header);
}

void PropertyEditor::addProperty(const QString &label, const QString &key,
                                  const QVariant &value, const QString &type)
{
    QWidget *editor = createEditor(key, value, type);
    if (editor)
        m_formLayout->addRow(label + ":", editor);
}

QWidget *PropertyEditor::createEditor(const QString &key, const QVariant &value,
                                       const QString &type)
{
    if (type == "readonly") {
        auto *label = new QLabel(value.toString(), m_contentWidget);
        label->setStyleSheet("color: #aaa;");
        return label;
    }

    if (type == "bool") {
        auto *cb = new QCheckBox(m_contentWidget);
        cb->setChecked(value.toBool());
        connect(cb, &QCheckBox::toggled, this, [this, key](bool checked) {
            if (m_updating || !m_currentWidget) return;
            QVariant old = m_currentWidget->property(key);
            m_currentWidget->setWidgetProperty(key, checked);
            emit propertyChanged(m_currentWidget->widgetId(), key, old, checked);
        });
        return cb;
    }

    if (type == "int") {
        auto *sb = new QSpinBox(m_contentWidget);
        sb->setRange(-99999, 99999);
        sb->setValue(value.toInt());
        connect(sb, QOverload<int>::of(&QSpinBox::valueChanged),
                this, [this, key](int val) {
            if (m_updating || !m_currentWidget) return;
            QVariant old = m_currentWidget->property(key);
            m_currentWidget->setWidgetProperty(key, val);
            emit propertyChanged(m_currentWidget->widgetId(), key, old, val);
        });
        return sb;
    }

    if (type == "double") {
        auto *sb = new QDoubleSpinBox(m_contentWidget);
        sb->setRange(-99999, 99999);
        sb->setDecimals(1);
        sb->setValue(value.toDouble());

        connect(sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, [this, key](double val) {
            if (m_updating || !m_currentWidget) return;

            // Обработка геометрии: x, y, width, height
            if (key == "x") {
                QPointF old = m_currentWidget->pos();
                m_currentWidget->setPos(val, old.y());
                emit propertyChanged(m_currentWidget->widgetId(), key, old.x(), val);
            } else if (key == "y") {
                QPointF old = m_currentWidget->pos();
                m_currentWidget->setPos(old.x(), val);
                emit propertyChanged(m_currentWidget->widgetId(), key, old.y(), val);
            } else if (key == "width") {
                qreal old = m_currentWidget->widgetWidth();
                m_currentWidget->setWidgetSize(val, m_currentWidget->widgetHeight());
                emit propertyChanged(m_currentWidget->widgetId(), key, old, val);
            } else if (key == "height") {
                qreal old = m_currentWidget->widgetHeight();
                m_currentWidget->setWidgetSize(m_currentWidget->widgetWidth(), val);
                emit propertyChanged(m_currentWidget->widgetId(), key, old, val);
            } else {
                QVariant old = m_currentWidget->property(key);
                m_currentWidget->setWidgetProperty(key, val);
                emit propertyChanged(m_currentWidget->widgetId(), key, old, val);
            }
        });
        return sb;
    }

    if (type == "alignment") {
        auto *cb = new QComboBox(m_contentWidget);
        cb->addItems({"left", "center", "right"});
        cb->setCurrentText(value.toString());
        connect(cb, &QComboBox::currentTextChanged, this, [this, key](const QString &val) {
            if (m_updating || !m_currentWidget) return;
            QVariant old = m_currentWidget->property(key);
            m_currentWidget->setWidgetProperty(key, val);
            emit propertyChanged(m_currentWidget->widgetId(), key, old, val);
        });
        return cb;
    }

    // По умолчанию: строковый редактор
    auto *le = new QLineEdit(value.toString(), m_contentWidget);
    connect(le, &QLineEdit::editingFinished, this, [this, le, key]() {
        if (m_updating || !m_currentWidget) return;
        QVariant old = m_currentWidget->property(key);
        QString newVal = le->text();
        m_currentWidget->setWidgetProperty(key, newVal);
        emit propertyChanged(m_currentWidget->widgetId(), key, old, newVal);
    });
    return le;
}

void PropertyEditor::clearLayout()
{
    if (!m_formLayout) return;

    QLayoutItem *child;
    while ((child = m_formLayout->takeAt(0)) != nullptr) {
        if (child->widget())
            child->widget()->deleteLater();
        delete child;
    }
}

} // namespace DeltaQ
