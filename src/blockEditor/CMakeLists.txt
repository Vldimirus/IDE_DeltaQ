# Модуль 2: Визуальный блочный редактор — заглушка (header-only)
# TODO: полная реализация в Фазе 2
add_library(dq_block_editor INTERFACE)

target_include_directories(dq_block_editor INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)
