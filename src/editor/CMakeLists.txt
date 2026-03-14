set(EDITOR_SOURCES
    CodeEditorWidget.cpp
    CodeEditorTab.cpp
    CompletionPopup.cpp
    FindReplaceBar.cpp
    ProjectTreeView.cpp
    BuildManager.cpp
    ProjectExecutableResolver.cpp
    LinuxProjectExporter.cpp
    CompilerOutputParser.cpp
    CMakeGenerator.cpp
    AnnotationParser.cpp
    SyntaxHighlighter.cpp
    ModuleManagerWidget.cpp
    ModuleTestRunner.cpp
)

add_library(dq_editor STATIC ${EDITOR_SOURCES})

target_include_directories(dq_editor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(dq_editor PUBLIC
    dq_core
    dq_block_editor
    Qt6::Core
    Qt6::Widgets
)

if(QSCINTILLA_FOUND)
    target_include_directories(dq_editor PRIVATE ${QSCINTILLA_INCLUDE_DIRS})
    target_link_libraries(dq_editor PRIVATE ${QSCINTILLA_LIBRARIES})
endif()

if(LIBCLANG_FOUND)
    if(LIBCLANG_INCLUDE_DIR)
        target_include_directories(dq_editor PRIVATE ${LIBCLANG_INCLUDE_DIR})
    endif()
    if(LIBCLANG_LIBRARY)
        target_link_libraries(dq_editor PRIVATE ${LIBCLANG_LIBRARY})
    endif()
endif()
