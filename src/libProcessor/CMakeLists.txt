# Модуль 4: Обработчик библиотек — заглушка (header-only)
# TODO: полная реализация в Фазе 4
add_library(dq_lib_processor INTERFACE)

target_include_directories(dq_lib_processor INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)
