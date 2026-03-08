# Тесты DeltaQ IDE
find_package(Qt6 REQUIRED COMPONENTS Test)

# Вспомогательная функция для добавления тестов
function(dq_add_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE
        dq_core
        Qt6::Test
    )
    set_target_properties(${name} PROPERTIES AUTOMOC ON)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

# Тесты ядра
dq_add_test(test_CommandBus     core/test_CommandBus.cpp)
dq_add_test(test_UndoManager    core/test_UndoManager.cpp)
dq_add_test(test_ActionManager  core/test_ActionManager.cpp)
dq_add_test(test_SessionManager core/test_SessionManager.cpp)

# Тесты моделей данных
dq_add_test(test_Module         core/test_Module.cpp)
dq_add_test(test_Project        core/test_Project.cpp)
dq_add_test(test_Graph          core/test_Graph.cpp)
dq_add_test(test_UILayout       core/test_UILayout.cpp)

# Тесты хранилищ данных
dq_add_test(test_GraphStore      core/test_GraphStore.cpp)
dq_add_test(test_UILayoutStore   core/test_UILayoutStore.cpp)
dq_add_test(test_ProjectManager  core/test_ProjectManager.cpp)

# Тесты LSP-клиента
add_executable(test_LSPClient lsp/test_LSPClient.cpp)
target_link_libraries(test_LSPClient PRIVATE
    dq_lsp
    Qt6::Test
)
set_target_properties(test_LSPClient PROPERTIES AUTOMOC ON)
add_test(NAME test_LSPClient COMMAND test_LSPClient)

# Тесты редактора
add_executable(test_CompilerOutputParser editor/test_CompilerOutputParser.cpp)
target_link_libraries(test_CompilerOutputParser PRIVATE
    dq_editor
    dq_lsp
    Qt6::Test
)
set_target_properties(test_CompilerOutputParser PROPERTIES AUTOMOC ON)
add_test(NAME test_CompilerOutputParser COMMAND test_CompilerOutputParser)

add_executable(test_CMakeGenerator editor/test_CMakeGenerator.cpp)
target_link_libraries(test_CMakeGenerator PRIVATE
    dq_editor
    dq_lsp
    Qt6::Test
)
set_target_properties(test_CMakeGenerator PROPERTIES AUTOMOC ON)
add_test(NAME test_CMakeGenerator COMMAND test_CMakeGenerator)

add_executable(test_BuildManager editor/test_BuildManager.cpp)
target_link_libraries(test_BuildManager PRIVATE
    dq_editor
    dq_lsp
    Qt6::Test
)
set_target_properties(test_BuildManager PROPERTIES AUTOMOC ON)
add_test(NAME test_BuildManager COMMAND test_BuildManager)

add_executable(test_AnnotationParser editor/test_AnnotationParser.cpp)
target_link_libraries(test_AnnotationParser PRIVATE
    dq_editor
    dq_lsp
    Qt6::Test
)
set_target_properties(test_AnnotationParser PROPERTIES AUTOMOC ON)
add_test(NAME test_AnnotationParser COMMAND test_AnnotationParser)

# Тесты отладчика
add_executable(test_DebugManager debug/test_DebugManager.cpp)
target_link_libraries(test_DebugManager PRIVATE
    dq_debug
    Qt6::Test
)
set_target_properties(test_DebugManager PROPERTIES AUTOMOC ON)
add_test(NAME test_DebugManager COMMAND test_DebugManager)

# Тесты LSP: rename, formatting, definition/references массивы
add_executable(test_LSPRename lsp/test_LSPRename.cpp)
target_link_libraries(test_LSPRename PRIVATE
    dq_lsp
    Qt6::Test
)
set_target_properties(test_LSPRename PROPERTIES AUTOMOC ON)
add_test(NAME test_LSPRename COMMAND test_LSPRename)

# Тесты отображения диагностики
add_executable(test_DiagnosticDisplay editor/test_DiagnosticDisplay.cpp)
target_link_libraries(test_DiagnosticDisplay PRIVATE
    dq_editor
    dq_lsp
    Qt6::Test
    Qt6::Widgets
)
set_target_properties(test_DiagnosticDisplay PROPERTIES AUTOMOC ON)
add_test(NAME test_DiagnosticDisplay COMMAND test_DiagnosticDisplay)

# Тесты маркеров breakpoints
add_executable(test_BreakpointMarkers editor/test_BreakpointMarkers.cpp)
target_link_libraries(test_BreakpointMarkers PRIVATE
    dq_editor
    dq_lsp
    Qt6::Test
    Qt6::Widgets
)
set_target_properties(test_BreakpointMarkers PROPERTIES AUTOMOC ON)
add_test(NAME test_BreakpointMarkers COMMAND test_BreakpointMarkers)

# Тесты блочного редактора
add_executable(test_BlockScene blockEditor/test_BlockScene.cpp)
target_link_libraries(test_BlockScene PRIVATE
    dq_block_editor
    dq_core
    Qt6::Test
    Qt6::Widgets
)
set_target_properties(test_BlockScene PROPERTIES AUTOMOC ON)
add_test(NAME test_BlockScene COMMAND test_BlockScene)

add_executable(test_GraphCommands blockEditor/test_GraphCommands.cpp)
target_link_libraries(test_GraphCommands PRIVATE
    dq_block_editor
    dq_core
    Qt6::Test
    Qt6::Widgets
)
set_target_properties(test_GraphCommands PROPERTIES AUTOMOC ON)
add_test(NAME test_GraphCommands COMMAND test_GraphCommands)

# Тесты компилятора графов
add_executable(test_IR codegen/test_IR.cpp)
target_link_libraries(test_IR PRIVATE
    dq_codegen
    Qt6::Test
)
set_target_properties(test_IR PROPERTIES AUTOMOC ON)
add_test(NAME test_IR COMMAND test_IR)

add_executable(test_GraphCompiler codegen/test_GraphCompiler.cpp)
target_link_libraries(test_GraphCompiler PRIVATE
    dq_codegen
    dq_core
    Qt6::Test
    Qt6::Widgets
)
set_target_properties(test_GraphCompiler PROPERTIES AUTOMOC ON)
add_test(NAME test_GraphCompiler COMMAND test_GraphCompiler)

# Тесты popup автодополнения
add_executable(test_CompletionPopup editor/test_CompletionPopup.cpp)
target_link_libraries(test_CompletionPopup PRIVATE
    dq_editor
    dq_lsp
    Qt6::Test
    Qt6::Widgets
)
set_target_properties(test_CompletionPopup PROPERTIES AUTOMOC ON)
add_test(NAME test_CompletionPopup COMMAND test_CompletionPopup)

# Тесты UI Designer
add_executable(test_WidgetItem uiDesigner/test_WidgetItem.cpp)
target_link_libraries(test_WidgetItem PRIVATE
    dq_ui_designer
    dq_core
    Qt6::Test
    Qt6::Widgets
)
set_target_properties(test_WidgetItem PROPERTIES AUTOMOC ON)
add_test(NAME test_WidgetItem COMMAND test_WidgetItem)

add_executable(test_DesignScene uiDesigner/test_DesignScene.cpp)
target_link_libraries(test_DesignScene PRIVATE
    dq_ui_designer
    dq_core
    Qt6::Test
    Qt6::Widgets
)
set_target_properties(test_DesignScene PROPERTIES AUTOMOC ON)
add_test(NAME test_DesignScene COMMAND test_DesignScene)

add_executable(test_UICommands uiDesigner/test_UICommands.cpp)
target_link_libraries(test_UICommands PRIVATE
    dq_ui_designer
    dq_core
    Qt6::Test
    Qt6::Widgets
)
set_target_properties(test_UICommands PROPERTIES AUTOMOC ON)
add_test(NAME test_UICommands COMMAND test_UICommands)

add_executable(test_LayoutEngine uiDesigner/test_LayoutEngine.cpp)
target_link_libraries(test_LayoutEngine PRIVATE
    dq_ui_designer
    dq_core
    Qt6::Test
    Qt6::Widgets
)
set_target_properties(test_LayoutEngine PROPERTIES AUTOMOC ON)
add_test(NAME test_LayoutEngine COMMAND test_LayoutEngine)

add_executable(test_SDL2CodeGenerator uiDesigner/test_SDL2CodeGenerator.cpp)
target_link_libraries(test_SDL2CodeGenerator PRIVATE
    dq_ui_designer
    dq_core
    Qt6::Test
    Qt6::Widgets
)
set_target_properties(test_SDL2CodeGenerator PROPERTIES AUTOMOC ON)
add_test(NAME test_SDL2CodeGenerator COMMAND test_SDL2CodeGenerator)

# Тесты Library Processor
add_executable(test_LibclangTypes libProcessor/test_LibclangTypes.cpp)
target_link_libraries(test_LibclangTypes PRIVATE
    dq_lib_processor
    dq_core
    Qt6::Test
)
set_target_properties(test_LibclangTypes PROPERTIES AUTOMOC ON)
add_test(NAME test_LibclangTypes COMMAND test_LibclangTypes)

add_executable(test_LibraryDecomposer libProcessor/test_LibraryDecomposer.cpp)
target_link_libraries(test_LibraryDecomposer PRIVATE
    dq_lib_processor
    dq_core
    Qt6::Test
)
set_target_properties(test_LibraryDecomposer PROPERTIES AUTOMOC ON)
add_test(NAME test_LibraryDecomposer COMMAND test_LibraryDecomposer)

add_executable(test_WrapperGenerator libProcessor/test_WrapperGenerator.cpp)
target_link_libraries(test_WrapperGenerator PRIVATE
    dq_lib_processor
    dq_core
    Qt6::Test
)
set_target_properties(test_WrapperGenerator PROPERTIES AUTOMOC ON)
add_test(NAME test_WrapperGenerator COMMAND test_WrapperGenerator)
