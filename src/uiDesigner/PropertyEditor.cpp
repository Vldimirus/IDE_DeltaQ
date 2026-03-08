// Редактор свойств виджета — реализация
#include "PropertyEditor.h"
#include "WidgetItem.h"
#include "DesignScene.h"

#include <deltaq/UILayout.h>
#include <deltaq/UIContract.h>

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
#include <QMetaType>

namespace DeltaQ {

namespace {

// Возвращает человекочитаемую подпись свойства UI-контракта.
QString propertyLabelForContract(const QString &name)
{
    if (name == "text")
        return QObject::tr("Text");
    if (name == "placeholder")
        return QObject::tr("Placeholder");
    if (name == "max_length")
        return QObject::tr("Max Length");
    if (name == "read_only")
        return QObject::tr("Read Only");
    if (name == "password")
        return QObject::tr("Password");
    if (name == "checked")
        return QObject::tr("Checked");
    if (name == "selected")
        return QObject::tr("Selected");
    if (name == "items")
        return QObject::tr("Items");
    if (name == "min")
        return QObject::tr("Min");
    if (name == "max")
        return QObject::tr("Max");
    if (name == "value")
        return QObject::tr("Value");
    if (name == "step")
        return QObject::tr("Step");
    if (name == "show_text")
        return QObject::tr("Show Text");
    if (name == "alignment")
        return QObject::tr("Alignment");
    if (name == "word_wrap")
        return QObject::tr("Word Wrap");
    if (name == "path")
        return QObject::tr("Path");
    return name;
}

// Возвращает тип редактора по свойству контракта.
QString propertyEditorTypeForContract(const QString &name, const QVariant &value)
{
    if (name == "alignment")
        return "alignment";
    if (value.metaType().id() == QMetaType::Bool)
        return "bool";
    if (value.canConvert<int>() && value.metaType().id() != QMetaType::QString)
        return "int";
    if (value.canConvert<double>() && value.metaType().id() != QMetaType::QString)
        return "double";
    return "string";
}

// Собирает effective value для contract-свойства: текущее значение либо catalog default.
QVariant effectiveContractPropertyValue(WidgetItem *widget, const UIContractPropertySpec &property)
{
    const QVariant currentValue = widget->property(property.name);
    if (currentValue.isValid())
        return currentValue;
    if (property.useWidgetNameAsDefault)
        return widget->widgetName();
    return property.defaultValue;
}

} // namespace

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
    addProperty(tr("Contract"), "contract", "window", "readonly");

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
    const QString contractType = m_currentWidget->widgetContractType();
    const QString displayType = m_currentWidget->widgetDisplayType();

    addSectionHeader(tr("General"));
    addProperty(tr("Name"), "name", m_currentWidget->widgetName(), "readonly");
    addProperty(tr("Type"), "type", displayType, "readonly");
    if (!contractType.isEmpty())
        addProperty(tr("Contract"), "contract", contractType, "readonly");
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

    // Типоспецифичные свойства теперь берём из единого UI-каталога, чтобы
    // дизайнер и модульный контракт использовали одинаковый словарь.
    if (const auto *spec = findUIContractSpecByAny(contractType)) {
        if (!spec->designerProperties.isEmpty()) {
            addSectionHeader(displayType);
            for (const auto &property : spec->designerProperties) {
                const QVariant effectiveValue = effectiveContractPropertyValue(m_currentWidget, property);
                addProperty(propertyLabelForContract(property.name),
                            property.name,
                            effectiveValue,
                            propertyEditorTypeForContract(property.name, effectiveValue));
            }
        }
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
    while (m_formLayout->count() > 0 && (child = m_formLayout->takeAt(0)) != nullptr) {
        if (child->widget())
            child->widget()->deleteLater();
        delete child;
    }
}

} // namespace DeltaQ
