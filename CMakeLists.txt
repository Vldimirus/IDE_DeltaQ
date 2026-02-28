cmake_minimum_required(VERSION 3.20)

project(DeltaQ
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "DeltaQ IDE — Modular C/C++ Development Environment"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Build options
option(DQ_BUILD_TESTS "Build tests" ON)
option(DQ_USE_LSP "Enable LSP client (requires clangd)" ON)
option(DQ_USE_LIBCLANG "Enable libclang integration" ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Gui
    Network
)

# Find QScintilla (optional for now, will use fallback)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(QSCINTILLA QUIET qscintilla2-qt6)
endif()

if(NOT QSCINTILLA_FOUND)
    find_library(QSCINTILLA_LIBRARY
        NAMES qscintilla2_qt6 qscintilla2-qt6 libqscintilla2_qt6
    )
    find_path(QSCINTILLA_INCLUDE_DIR
        NAMES Qsci/qsciscintilla.h
        PATH_SUFFIXES qt6
    )
    if(QSCINTILLA_LIBRARY AND QSCINTILLA_INCLUDE_DIR)
        set(QSCINTILLA_FOUND TRUE)
        set(QSCINTILLA_LIBRARIES ${QSCINTILLA_LIBRARY})
        set(QSCINTILLA_INCLUDE_DIRS ${QSCINTILLA_INCLUDE_DIR})
    endif()
endif()

if(QSCINTILLA_FOUND)
    message(STATUS "QScintilla found: ${QSCINTILLA_LIBRARIES}")
    add_compile_definitions(DQ_HAS_QSCINTILLA)
else()
    message(WARNING "QScintilla not found — code editor will use QPlainTextEdit fallback")
endif()

# Поиск libclang (опционально)
# Ищем сначала библиотеку напрямую, чтобы избежать проблем
# с битыми cmake-конфигурациями Clang
if(DQ_USE_LIBCLANG)
    find_library(LIBCLANG_LIBRARY NAMES clang libclang)
    find_path(LIBCLANG_INCLUDE_DIR NAMES clang-c/Index.h)
    if(LIBCLANG_LIBRARY AND LIBCLANG_INCLUDE_DIR)
        set(LIBCLANG_FOUND TRUE)
        message(STATUS "libclang found: ${LIBCLANG_LIBRARY}")
        add_compile_definitions(DQ_HAS_LIBCLANG)
    else()
        set(LIBCLANG_FOUND FALSE)
        message(WARNING "libclang not found — парсинг аннотаций и обработка библиотек отключены")
    endif()
endif()

# Инструменты локализации (lupdate/lrelease) — ищем Qt6 версии
find_program(LUPDATE_EXECUTABLE
    NAMES lupdate6 lupdate
    HINTS /usr/lib/qt6/bin ${Qt6Core_DIR}/../../../bin
)
find_program(LRELEASE_EXECUTABLE
    NAMES lrelease6 lrelease
    HINTS /usr/lib/qt6/bin ${Qt6Core_DIR}/../../../bin
)

if(LUPDATE_EXECUTABLE AND LRELEASE_EXECUTABLE)
    message(STATUS "lupdate found: ${LUPDATE_EXECUTABLE}")
    message(STATUS "lrelease found: ${LRELEASE_EXECUTABLE}")
else()
    message(WARNING "lupdate/lrelease not found — цели переводов будут недоступны")
endif()

# Main executable sources
add_subdirectory(src)

# Tests
if(DQ_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
