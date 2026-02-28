# Модуль 3: Дизайнер UI — заглушка (header-only)
# TODO: полная реализация в Фазе 3
add_library(dq_ui_designer INTERFACE)

target_include_directories(dq_ui_designer INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)
