# Ядро IDE — базовые сервисы (без MainWindow, он в исполняемом файле)
add_library(dq_core STATIC
    BuildGuidanceAnalyzer.cpp
    CommandBus.cpp
    ModuleRegistry.cpp
    GraphStore.cpp
    UILayoutStore.cpp
    ProjectManager.cpp
    SessionManager.cpp
    ActionManager.cpp
    UndoManager.cpp
    StandardLibrary.cpp
    toolbar_icons.qrc
)

target_include_directories(dq_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(dq_core PUBLIC
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
)
