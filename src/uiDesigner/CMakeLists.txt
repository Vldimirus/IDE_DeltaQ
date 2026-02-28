# Модуль 3: Дизайнер UI
add_library(dq_ui_designer STATIC
    DesignScene.cpp
    WidgetItem.cpp
    WidgetPalette.cpp
    PropertyEditor.cpp
    UIDesignerWidget.cpp
    UICommands.cpp
    LayoutEngine.cpp
    EventBindingDialog.cpp
    SDL2CodeGenerator.cpp
    UIPreview.cpp
)

target_include_directories(dq_ui_designer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(dq_ui_designer PUBLIC
    dq_core
    Qt6::Core
    Qt6::Widgets
)
