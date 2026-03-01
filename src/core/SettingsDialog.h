// Диалог настроек IDE
#pragma once

#include <QDialog>

class QLineEdit;
class QSpinBox;
class QComboBox;
class QFontComboBox;

namespace DeltaQ {

class SessionManager;

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(SessionManager *session, QWidget *parent = nullptr);

private:
    void apply();

    SessionManager *m_session;

    // Общие
    QLineEdit *m_defaultDirEdit = nullptr;

    // Редактор
    QFontComboBox *m_fontCombo = nullptr;
    QSpinBox *m_fontSizeSpin = nullptr;
    QSpinBox *m_tabWidthSpin = nullptr;

    // Язык
    QComboBox *m_langCombo = nullptr;
};

} // namespace DeltaQ
