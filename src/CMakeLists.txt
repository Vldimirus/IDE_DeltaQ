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
    core/NewProjectWizard.cpp
    core/ProjectTemplates.cpp
    core/SettingsDialog.cpp
    core/NewFileDialog.cpp
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

target_compile_definitions(deltaq PRIVATE
    DQ_APP_VERSION="${PROJECT_VERSION}"
)

function(dq_add_runtime_copy_target target_name source_dir dest_name comment_text)
    file(GLOB_RECURSE runtime_files CONFIGURE_DEPENDS "${source_dir}/*")
    set(stamp_file "${CMAKE_CURRENT_BINARY_DIR}/${target_name}.stamp")

    add_custom_command(
        OUTPUT "${stamp_file}"
        COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:deltaq>/${dest_name}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${source_dir}"
            "$<TARGET_FILE_DIR:deltaq>/${dest_name}"
        COMMAND ${CMAKE_COMMAND} -E touch "${stamp_file}"
        DEPENDS ${runtime_files}
        COMMENT "${comment_text}"
    )

    add_custom_target(${target_name} DEPENDS "${stamp_file}")
    add_dependencies(deltaq ${target_name})
endfunction()

# Runtime assets рядом с исполняемым файлом должны обновляться не только
# при перекомпиляции deltaq, но и при изменении файлов шаблонов/examples/modules.
dq_add_runtime_copy_target(
    deltaq_runtime_modules
    "${CMAKE_SOURCE_DIR}/modules"
    "modules"
    "Copying modules library to build output..."
)

dq_add_runtime_copy_target(
    deltaq_runtime_templates
    "${CMAKE_SOURCE_DIR}/resources/templates"
    "templates"
    "Copying project templates to build output..."
)

dq_add_runtime_copy_target(
    deltaq_runtime_examples
    "${CMAKE_SOURCE_DIR}/resources/examples"
    "examples"
    "Copying reference examples to build output..."
)

install(TARGETS deltaq
    RUNTIME DESTINATION .
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/modules/
    DESTINATION modules
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources/templates/
    DESTINATION templates
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources/examples/
    DESTINATION examples
)

# Цели для генерации переводов
set(TS_FILE "${CMAKE_SOURCE_DIR}/resources/translations/deltaq_ru.ts")
set(QM_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/translations")
set(QM_FILE "${QM_OUTPUT_DIR}/deltaq_ru.qm")

if(LUPDATE_EXECUTABLE AND LRELEASE_EXECUTABLE)
    # lupdate: извлечение строк из исходников в .ts
    add_custom_target(update_translations
        COMMAND ${LUPDATE_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/src
            -ts ${TS_FILE}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Extracting translatable strings with lupdate..."
    )

    # lrelease: компиляция .ts → .qm рядом с исполняемым файлом
    add_custom_command(
        OUTPUT ${QM_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${QM_OUTPUT_DIR}
        COMMAND ${LRELEASE_EXECUTABLE} ${TS_FILE}
            -qm ${QM_FILE}
        DEPENDS ${TS_FILE}
        COMMENT "Compiling translations with lrelease..."
    )

    add_custom_target(translations
        DEPENDS ${QM_FILE}
    )

    add_dependencies(deltaq translations)

    install(FILES ${QM_FILE}
        DESTINATION translations
    )
else()
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources/translations/
        DESTINATION translations
        FILES_MATCHING PATTERN "*.ts"
    )
endif()
