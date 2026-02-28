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
