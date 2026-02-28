set(DEBUG_SOURCES
    DebugManager.cpp
)

add_library(dq_debug STATIC ${DEBUG_SOURCES})

target_include_directories(dq_debug PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(dq_debug PUBLIC
    Qt6::Core
)
