# Модуль 2: Визуальный блочный редактор
add_library(dq_block_editor STATIC
    BlockEditorWidget.cpp
    BlockScene.cpp
    NodeItem.cpp
    PortItem.cpp
    ConnectionItem.cpp
    ModulePalette.cpp
    GraphCommands.cpp
    GraphDebugger.cpp
    SubModuleFactory.cpp
    BreadcrumbBar.cpp
    CycleDetector.cpp
)

target_include_directories(dq_block_editor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(dq_block_editor PUBLIC
    dq_core
    dq_debug
    Qt6::Core
    Qt6::Widgets
)
