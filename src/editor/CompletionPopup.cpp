// Popup-окно автодополнения
#include "CompletionPopup.h"

#include <QVBoxLayout>
#include <QKeyEvent>
#include <QApplication>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QPainter>
#include <QStyle>

namespace DeltaQ {

CompletionPopup::CompletionPopup(QWidget *parent)
    : QFrame(parent, Qt::Popup | Qt::FramelessWindowHint)
{
    setFrameStyle(QFrame::Box | QFrame::Plain);
    setLineWidth(1);
    setFixedWidth(300);
    setMaximumHeight(200);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_listWidget = new QListWidget(this);
    m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listWidget->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(m_listWidget);

    connect(m_listWidget, &QListWidget::itemDoubleClicked,
            this, [this]() { insertCurrentItem(); });

    // Устанавливаем фильтр событий на родительский виджет
    if (parent)
        parent->installEventFilter(this);
}

void CompletionPopup::setItems(const QVector<CompletionItem> &items)
{
    m_allItems = items;
    m_prefix.clear();
    m_listWidget->clear();

    for (const auto &item : items) {
        auto *listItem = new QListWidgetItem(iconForKind(item.kind), item.label);
        if (!item.detail.isEmpty())
            listItem->setToolTip(item.detail);
        listItem->setData(Qt::UserRole, item.insertText);
        m_listWidget->addItem(listItem);
    }

    if (m_listWidget->count() > 0)
        m_listWidget->setCurrentRow(0);

    // Подгоняем высоту
    int rows = qMin(m_listWidget->count(), 8);
    int rowHeight = m_listWidget->sizeHintForRow(0);
    if (rowHeight <= 0) rowHeight = 20;
    setFixedHeight(rows * rowHeight + 4);
}

void CompletionPopup::popup(const QPoint &globalPos)
{
    move(globalPos);
    show();
    raise();
}

void CompletionPopup::updateFilter(const QString &prefix)
{
    m_prefix = prefix;
    m_listWidget->clear();

    for (const auto &item : m_allItems) {
        QString filterText = item.filterText.isEmpty() ? item.label : item.filterText;
        if (!prefix.isEmpty() && !filterText.startsWith(prefix, Qt::CaseInsensitive))
            continue;

        auto *listItem = new QListWidgetItem(iconForKind(item.kind), item.label);
        if (!item.detail.isEmpty())
            listItem->setToolTip(item.detail);
        listItem->setData(Qt::UserRole, item.insertText);
        m_listWidget->addItem(listItem);
    }

    if (m_listWidget->count() == 0) {
        hide();
        return;
    }

    m_listWidget->setCurrentRow(0);

    // Подгоняем высоту
    int rows = qMin(m_listWidget->count(), 8);
    int rowHeight = m_listWidget->sizeHintForRow(0);
    if (rowHeight <= 0) rowHeight = 20;
    setFixedHeight(rows * rowHeight + 4);
}

bool CompletionPopup::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress && isVisible()) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);

        switch (keyEvent->key()) {
        case Qt::Key_Escape:
            hide();
            return true;

        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Tab:
            insertCurrentItem();
            return true;

        case Qt::Key_Up:
            if (m_listWidget->currentRow() > 0)
                m_listWidget->setCurrentRow(m_listWidget->currentRow() - 1);
            return true;

        case Qt::Key_Down:
            if (m_listWidget->currentRow() < m_listWidget->count() - 1)
                m_listWidget->setCurrentRow(m_listWidget->currentRow() + 1);
            return true;

        default:
            break;
        }
    }

    return QFrame::eventFilter(obj, event);
}

void CompletionPopup::insertCurrentItem()
{
    auto *item = m_listWidget->currentItem();
    if (!item) {
        hide();
        return;
    }

    QString text = item->data(Qt::UserRole).toString();
    emit itemSelected(text);
    hide();
}

QIcon CompletionPopup::iconForKind(CompletionItemKind kind) const
{
    // Создаём простые цветные иконки для разных типов
    QPixmap pix(16, 16);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor color;
    QString letter;

    switch (kind) {
    case CompletionItemKind::Function:
    case CompletionItemKind::Method:
        color = QColor(100, 50, 200);   // фиолетовый
        letter = "F";
        break;
    case CompletionItemKind::Variable:
    case CompletionItemKind::Field:
        color = QColor(50, 120, 200);   // синий
        letter = "V";
        break;
    case CompletionItemKind::Class:
    case CompletionItemKind::Struct:
        color = QColor(200, 120, 0);    // оранжевый
        letter = "C";
        break;
    case CompletionItemKind::Enum:
    case CompletionItemKind::EnumMember:
        color = QColor(180, 80, 0);     // тёмно-оранжевый
        letter = "E";
        break;
    case CompletionItemKind::Keyword:
        color = QColor(0, 120, 80);     // зелёный
        letter = "K";
        break;
    case CompletionItemKind::Snippet:
        color = QColor(120, 120, 120);  // серый
        letter = "S";
        break;
    case CompletionItemKind::Property:
        color = QColor(50, 120, 200);
        letter = "P";
        break;
    case CompletionItemKind::Constructor:
        color = QColor(100, 50, 200);
        letter = "N";
        break;
    default:
        color = QColor(100, 100, 100);
        letter = "T";
        break;
    }

    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(1, 1, 14, 14, 3, 3);

    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPixelSize(10);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(QRect(0, 0, 16, 16), Qt::AlignCenter, letter);

    return QIcon(pix);
}

} // namespace DeltaQ
