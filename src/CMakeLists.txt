add_subdirectory(core)
add_subdirectory(editor)
add_subdirectory(blockEditor)
add_subdirectory(uiDesigner)
add_subdirectory(libProcessor)
add_subdirectory(codegen)
add_subdirectory(lsp)
add_subdirectory(debug)

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
    dq_debug
    Qt6::Core
    Qt6::Widgets
)

# Цели для генерации переводов
set(TS_FILE "${CMAKE_SOURCE_DIR}/resources/translations/deltaq_ru.ts")
set(QM_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/translations")

if(LUPDATE_EXECUTABLE AND LRELEASE_EXECUTABLE)
    # lupdate: извлечение строк из исходников в .ts
    add_custom_target(update_translations
        COMMAND ${LUPDATE_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/src
            -ts ${TS_FILE}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Extracting translatable strings with lupdate..."
    )

    # lrelease: компиляция .ts → .qm + копирование рядом с исполняемым файлом
    add_custom_target(translations
        COMMAND ${CMAKE_COMMAND} -E make_directory ${QM_OUTPUT_DIR}
        COMMAND ${LRELEASE_EXECUTABLE} ${TS_FILE}
            -qm ${QM_OUTPUT_DIR}/deltaq_ru.qm
        COMMENT "Compiling translations with lrelease..."
    )
endif()
