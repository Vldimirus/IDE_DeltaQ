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
