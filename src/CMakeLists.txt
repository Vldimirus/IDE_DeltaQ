add_subdirectory(core)
add_subdirectory(editor)
add_subdirectory(blockEditor)
add_subdirectory(uiDesigner)
add_subdirectory(libProcessor)
add_subdirectory(codegen)
add_subdirectory(lsp)

# Главный исполняемый файл
# MainWindow вынесен из dq_core сюда, чтобы избежать
# циклической зависимости (MainWindow зависит от всех модулей)
add_executable(deltaq
    main.cpp
    core/MainWindow.cpp
)

target_include_directories(deltaq PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(deltaq PRIVATE
    dq_core
    dq_editor
    dq_block_editor
    dq_ui_designer
    dq_lib_processor
    dq_codegen
    dq_lsp
    Qt6::Core
    Qt6::Widgets
)
