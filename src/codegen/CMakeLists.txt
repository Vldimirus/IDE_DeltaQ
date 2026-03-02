# Генератор кода — IR + GraphCompiler + Pipeline
add_library(dq_codegen STATIC
    IR.cpp
    GraphCompiler.cpp
    PreBuildProcessor.cpp
    BuildPipeline.cpp
    CompilerDetector.cpp
)

target_include_directories(dq_codegen PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(dq_codegen PUBLIC
    dq_core
    dq_ui_designer
    Qt6::Core
)
