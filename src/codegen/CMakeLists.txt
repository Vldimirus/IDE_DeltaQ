# Генератор кода — IR + GraphCompiler
add_library(dq_codegen STATIC
    IR.cpp
    GraphCompiler.cpp
)

target_include_directories(dq_codegen PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(dq_codegen PUBLIC
    dq_core
    Qt6::Core
)
