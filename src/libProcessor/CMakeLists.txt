# Модуль 4: Обработчик библиотек
add_library(dq_lib_processor STATIC
    LibclangParser.cpp
    LibraryDecomposer.cpp
    WrapperGenerator.cpp
    LibraryImportWizard.cpp
    LibProcessorWidget.cpp
)

target_include_directories(dq_lib_processor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(dq_lib_processor PUBLIC
    dq_core
    Qt6::Core
    Qt6::Widgets
)

# Условная линковка libclang
if(LIBCLANG_FOUND)
    target_include_directories(dq_lib_processor PRIVATE ${LIBCLANG_INCLUDE_DIR})
    target_link_libraries(dq_lib_processor PRIVATE ${LIBCLANG_LIBRARY})
endif()
