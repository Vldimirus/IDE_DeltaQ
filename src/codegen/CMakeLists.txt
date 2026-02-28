# Генератор кода — заглушка (пока нет исходников)
# TODO: реализация CBackend, GraphToC и т.д.
add_library(dq_codegen INTERFACE)

target_include_directories(dq_codegen INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)
