set(LSP_SOURCES
    LSPClient.cpp
)

add_library(dq_lsp STATIC ${LSP_SOURCES})

target_include_directories(dq_lsp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(dq_lsp PUBLIC
    Qt6::Core
    Qt6::Network
)
