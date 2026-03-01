// Диалог настроек IDE
#pragma once

#include <QDialog>

class QLineEdit;
class QSpinBox;
class QComboBox;
class QFontComboBox;
class QListWidget;

namespace DeltaQ {

class SessionManager;
class ModuleRegistry;

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(SessionManager *session, QWidget *parent = nullptr);

    // Установить реестр модулей для вкладки «Модули»
    void setModuleRegistry(ModuleRegistry *registry);

private:
    void apply();
    void buildModulesTab();

    SessionManager *m_session;
    ModuleRegistry *m_registry = nullptr;

    // Общие
    QLineEdit *m_defaultDirEdit = nullptr;

    // Редактор
    QFontComboBox *m_fontCombo = nullptr;
    QSpinBox *m_fontSizeSpin = nullptr;
    QSpinBox *m_tabWidthSpin = nullptr;

    // Язык
    QComboBox *m_langCombo = nullptr;

    // Модули
    QListWidget *m_packList = nullptr;
};

} // namespace DeltaQ
