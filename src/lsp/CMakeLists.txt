# LSP-клиент — заглушка (пока нет исходников)
# TODO: реализация LspClient, LspProtocol
add_library(dq_lsp INTERFACE)

target_include_directories(dq_lsp INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)
