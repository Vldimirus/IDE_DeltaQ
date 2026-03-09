#!/bin/bash
# ============================================================
# DeltaQ IDE — Скрипт сборки релиза
# ============================================================
#
# Использование:
#   ./scripts/build_release.sh              — полная сборка
#   ./scripts/build_release.sh --clean      — чистая сборка (удаляет build/)
#   ./scripts/build_release.sh --no-tests   — без тестов
#   ./scripts/build_release.sh --package    — собрать + упаковать в архив
#
# Результат: build/release/DeltaQ/
#   ├── deltaq              — исполняемый файл
#   ├── modules/            — библиотека модулей
#   ├── templates/          — файловые шаблоны проектов
#   ├── examples/           — reference examples
#   └── config/             — создаётся при первом запуске
# ============================================================

set -e  # Остановка при любой ошибке

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Корень проекта — папка уровнем выше scripts/
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
RELEASE_DIR="$BUILD_DIR/release/DeltaQ"
NPROC=$(nproc 2>/dev/null || echo 4)

# Разбор аргументов
DO_CLEAN=0
DO_TESTS=1
DO_PACKAGE=0

for arg in "$@"; do
    case "$arg" in
        --clean)    DO_CLEAN=1 ;;
        --no-tests) DO_TESTS=0 ;;
        --package)  DO_PACKAGE=1 ;;
        --help|-h)
            echo "Использование: $0 [--clean] [--no-tests] [--package]"
            echo "  --clean      Чистая сборка (удаляет build/)"
            echo "  --no-tests   Пропустить тесты"
            echo "  --package    Упаковать в .tar.gz архив"
            exit 0
            ;;
        *)
            echo -e "${RED}Неизвестный аргумент: $arg${NC}"
            exit 1
            ;;
    esac
done

echo -e "${CYAN}============================================${NC}"
echo -e "${CYAN}  DeltaQ IDE — Сборка релиза${NC}"
echo -e "${CYAN}============================================${NC}"
echo ""

# ----------------------------------------------------------
# 1. Проверка зависимостей
# ----------------------------------------------------------
echo -e "${YELLOW}[1/6] Проверка зависимостей...${NC}"

MISSING=""
command -v cmake  >/dev/null 2>&1 || MISSING="$MISSING cmake"
command -v cpack  >/dev/null 2>&1 || MISSING="$MISSING cpack"
command -v g++    >/dev/null 2>&1 || MISSING="$MISSING g++"

if [ -n "$MISSING" ]; then
    echo -e "${RED}Не найдены:$MISSING${NC}"
    echo "Установите: sudo apt install$MISSING"
    exit 1
fi

# Проверка Qt6
if ! pkg-config --exists Qt6Core 2>/dev/null; then
    if [ ! -d "/usr/include/x86_64-linux-gnu/qt6" ] && [ ! -d "/usr/include/qt6" ]; then
        echo -e "${RED}Qt6 не найден. Установите: sudo apt install qt6-base-dev${NC}"
        exit 1
    fi
fi

echo -e "${GREEN}  Зависимости OK${NC}"

# ----------------------------------------------------------
# 2. Чистка (если запрошена)
# ----------------------------------------------------------
if [ "$DO_CLEAN" -eq 1 ]; then
    echo -e "${YELLOW}[2/6] Чистая сборка — удаление build/...${NC}"
    rm -rf "$BUILD_DIR"
    echo -e "${GREEN}  Очищено${NC}"
else
    echo -e "${YELLOW}[2/6] Инкрементальная сборка${NC}"
fi

# ----------------------------------------------------------
# 3. Конфигурация CMake (Release)
# ----------------------------------------------------------
echo -e "${YELLOW}[3/6] Конфигурация CMake (Release)...${NC}"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

CMAKE_ARGS=(
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_CXX_FLAGS_RELEASE="-O2 -DNDEBUG"
)

if [ "$DO_TESTS" -eq 1 ]; then
    CMAKE_ARGS+=(-DDQ_BUILD_TESTS=ON)
else
    CMAKE_ARGS+=(-DDQ_BUILD_TESTS=OFF)
fi

cmake "$PROJECT_DIR" "${CMAKE_ARGS[@]}"
echo -e "${GREEN}  Конфигурация OK${NC}"

# ----------------------------------------------------------
# 4. Сборка
# ----------------------------------------------------------
echo -e "${YELLOW}[4/6] Компиляция ($NPROC потоков)...${NC}"

cmake --build . -j"$NPROC"
echo -e "${GREEN}  Компиляция OK${NC}"

# ----------------------------------------------------------
# 5. Тесты
# ----------------------------------------------------------
if [ "$DO_TESTS" -eq 1 ]; then
    echo -e "${YELLOW}[5/6] Запуск тестов...${NC}"
    cd "$BUILD_DIR"
    if [ -z "${DISPLAY:-}" ] && [ -z "${WAYLAND_DISPLAY:-}" ] && [ -z "${QT_QPA_PLATFORM:-}" ]; then
        export QT_QPA_PLATFORM=offscreen
        echo -e "${CYAN}  Headless-режим: QT_QPA_PLATFORM=offscreen${NC}"
    fi
    if ctest --output-on-failure; then
        TOTAL=$(ctest -N 2>/dev/null | tail -1 | grep -oP '\d+' || echo "?")
        echo -e "${GREEN}  Все тесты пройдены ($TOTAL)${NC}"
    else
        echo -e "${RED}  Тесты провалились!${NC}"
        exit 1
    fi
else
    echo -e "${YELLOW}[5/6] Тесты пропущены (--no-tests)${NC}"
fi

# ----------------------------------------------------------
# 6. Сборка релизной папки
# ----------------------------------------------------------
echo -e "${YELLOW}[6/6] Формирование релиза...${NC}"

rm -rf "$RELEASE_DIR"
mkdir -p "$RELEASE_DIR"
cmake --install "$BUILD_DIR" --prefix "$RELEASE_DIR"

# Информация о версии
VERSION=$(grep -A1 'project(DeltaQ' "$PROJECT_DIR/CMakeLists.txt" | grep -oP 'VERSION \K[0-9.]+')
BUILD_DATE=$(date '+%Y-%m-%d %H:%M')
cat > "$RELEASE_DIR/VERSION" << EOF
DeltaQ IDE v${VERSION}
Build: ${BUILD_DATE}
Platform: $(uname -s) $(uname -m)
Compiler: $(g++ --version | head -1)
Qt: $(pkg-config --modversion Qt6Core 2>/dev/null || echo "unknown")
EOF

"$PROJECT_DIR/scripts/verify_release_bundle.sh" "$RELEASE_DIR" --expect-version-file
echo -e "${CYAN}  First-run smoke: isolated DELTAQ_HOME${NC}"
"$PROJECT_DIR/scripts/smoke_linux_first_run.sh" "$RELEASE_DIR" >/dev/null
echo -e "${GREEN}  First-run smoke OK${NC}"
echo -e "${CYAN}  Example smoke: open -> build -> run${NC}"
"$PROJECT_DIR/scripts/smoke_linux_example_build_run.sh" "$RELEASE_DIR" >/dev/null
echo -e "${GREEN}  Example smoke OK${NC}"

echo -e "${GREEN}  Релиз собран: $RELEASE_DIR${NC}"

# ----------------------------------------------------------
# Упаковка в архив (если запрошена)
# ----------------------------------------------------------
if [ "$DO_PACKAGE" -eq 1 ]; then
    PACKAGE_DIR="$BUILD_DIR/package"
    echo -e "${YELLOW}Упаковка в архив...${NC}"
    rm -rf "$PACKAGE_DIR"
    mkdir -p "$PACKAGE_DIR"
    cpack --config "$BUILD_DIR/CPackConfig.cmake" -B "$PACKAGE_DIR"
    ARCHIVE=$(find "$PACKAGE_DIR" -maxdepth 1 -type f -name "*.tar.gz" | head -1)
    if [ -z "$ARCHIVE" ]; then
        echo -e "${RED}  Архив не был создан${NC}"
        exit 1
    fi
    (
        cd "$PACKAGE_DIR"
        sha256sum "$(basename "$ARCHIVE")" > SHA256SUMS
    )
    "$PROJECT_DIR/scripts/verify_package_archive.sh" "$ARCHIVE" "$PACKAGE_DIR/SHA256SUMS"
    SIZE=$(du -h "$ARCHIVE" | cut -f1)
    echo -e "${GREEN}  Архив: $ARCHIVE ($SIZE)${NC}"
    echo -e "${GREEN}  SHA256: $PACKAGE_DIR/SHA256SUMS${NC}"
fi

# ----------------------------------------------------------
# Итог
# ----------------------------------------------------------
echo ""
echo -e "${CYAN}============================================${NC}"
echo -e "${GREEN}  Сборка завершена успешно!${NC}"
echo -e "${CYAN}============================================${NC}"
echo ""
echo "  Релиз:   $RELEASE_DIR"
echo "  Запуск:  $RELEASE_DIR/deltaq"
if [ "$DO_PACKAGE" -eq 1 ]; then
    echo "  Архив:   $ARCHIVE"
    echo "  SHA256:  $PACKAGE_DIR/SHA256SUMS"
fi
echo ""
