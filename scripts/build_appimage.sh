#!/bin/bash

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
RELEASE_DIR="$BUILD_DIR/release/DeltaQ"
APPIMAGE_ROOT="$BUILD_DIR/appimage"
APPDIR="$APPIMAGE_ROOT/AppDir"
APPDIR_BIN="$APPDIR/usr/bin"
TOOLS_DIR="$APPIMAGE_ROOT/tools"
DESKTOP_ID="org.deltaq.deltaq.desktop"

DO_CLEAN=0
DO_TESTS=1
APPDIR_ONLY=0
LINUXDEPLOY_BIN=""
LINUXDEPLOY_QT_PLUGIN=""
APPIMAGETOOL_BIN=""
RUNTIME_FILE=""
QMAKE_BIN=""

while [ "$#" -gt 0 ]; do
    case "$1" in
        --clean)
            DO_CLEAN=1
            ;;
        --no-tests)
            DO_TESTS=0
            ;;
        --appdir-only)
            APPDIR_ONLY=1
            ;;
        --linuxdeploy)
            shift
            LINUXDEPLOY_BIN="${1:-}"
            ;;
        --linuxdeploy-qt-plugin)
            shift
            LINUXDEPLOY_QT_PLUGIN="${1:-}"
            ;;
        --appimagetool)
            shift
            APPIMAGETOOL_BIN="${1:-}"
            ;;
        --runtime-file)
            shift
            RUNTIME_FILE="${1:-}"
            ;;
        --help|-h)
            cat <<'EOF'
Usage: ./scripts/build_appimage.sh [--clean] [--no-tests] [--appdir-only]
                                  [--linuxdeploy <path>]
                                  [--linuxdeploy-qt-plugin <path>]
                                  [--appimagetool <path>]
                                  [--runtime-file <path>]
EOF
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 1
            ;;
    esac
    shift
done

mkdir -p "$APPIMAGE_ROOT"

resolve_tool_path() {
    local path="$1"
    if [ -z "$path" ]; then
        return 0
    fi
    readlink -f "$path"
}

RELEASE_ARGS=()
if [ "$DO_CLEAN" -eq 1 ]; then
    RELEASE_ARGS+=(--clean)
fi
if [ "$DO_TESTS" -eq 0 ]; then
    RELEASE_ARGS+=(--no-tests)
fi

"$PROJECT_DIR/scripts/build_release.sh" "${RELEASE_ARGS[@]}"

rm -rf "$APPDIR"
mkdir -p "$APPDIR_BIN"
cp -a "$RELEASE_DIR"/. "$APPDIR_BIN"/

mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/scalable/apps"
mkdir -p "$APPDIR/usr/share/metainfo"
mkdir -p "$APPDIR/usr/translations"
cp "$PROJECT_DIR/resources/linux/deltaq.desktop" \
   "$APPDIR/usr/share/applications/$DESKTOP_ID"
cp "$PROJECT_DIR/resources/linux/deltaq.svg" \
   "$APPDIR/usr/share/icons/hicolor/scalable/apps/deltaq.svg"
cp "$PROJECT_DIR/resources/linux/deltaq.appdata.xml" \
   "$APPDIR/usr/share/metainfo/org.deltaq.deltaq.appdata.xml"

ln -sfn "usr/share/applications/$DESKTOP_ID" "$APPDIR/$DESKTOP_ID"
ln -sfn usr/share/icons/hicolor/scalable/apps/deltaq.svg "$APPDIR/.DirIcon"

cat > "$APPDIR/AppRun" <<'EOF'
#!/bin/sh
HERE="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
exec "$HERE/usr/bin/deltaq" "$@"
EOF
chmod +x "$APPDIR/AppRun"

"$PROJECT_DIR/scripts/verify_appdir.sh" "$APPDIR"

if [ "$APPDIR_ONLY" -eq 1 ]; then
    echo "AppDir prepared: $APPDIR"
    exit 0
fi

mkdir -p "$TOOLS_DIR"

if [ -n "$LINUXDEPLOY_BIN" ]; then
    LINUXDEPLOY_BIN="$(resolve_tool_path "$LINUXDEPLOY_BIN")"
    chmod +x "$LINUXDEPLOY_BIN"
fi
if [ -n "$LINUXDEPLOY_QT_PLUGIN" ]; then
    LINUXDEPLOY_QT_PLUGIN="$(resolve_tool_path "$LINUXDEPLOY_QT_PLUGIN")"
    chmod +x "$LINUXDEPLOY_QT_PLUGIN"
    ln -sfn "$LINUXDEPLOY_QT_PLUGIN" "$TOOLS_DIR/linuxdeploy-plugin-qt"
fi
if [ -n "$APPIMAGETOOL_BIN" ]; then
    APPIMAGETOOL_BIN="$(resolve_tool_path "$APPIMAGETOOL_BIN")"
    chmod +x "$APPIMAGETOOL_BIN"
fi
if [ -n "$RUNTIME_FILE" ]; then
    RUNTIME_FILE="$(resolve_tool_path "$RUNTIME_FILE")"
fi

if [ -z "$LINUXDEPLOY_BIN" ]; then
    LINUXDEPLOY_BIN="$(command -v linuxdeploy || true)"
fi
if [ -z "$APPIMAGETOOL_BIN" ]; then
    APPIMAGETOOL_BIN="$(command -v appimagetool || true)"
fi

[ -n "$LINUXDEPLOY_BIN" ] || {
    echo "linuxdeploy was not provided and was not found in PATH" >&2
    exit 1
}
[ -n "$APPIMAGETOOL_BIN" ] || {
    echo "appimagetool was not provided and was not found in PATH" >&2
    exit 1
}
[ -n "$LINUXDEPLOY_QT_PLUGIN" ] || {
    echo "linuxdeploy Qt plugin path is required for AppImage generation" >&2
    exit 1
}

if command -v qmake6 >/dev/null 2>&1; then
    QMAKE_BIN="$(command -v qmake6)"
elif [ -x /usr/lib/qt6/bin/qmake ]; then
    QMAKE_BIN="/usr/lib/qt6/bin/qmake"
elif command -v qmake >/dev/null 2>&1; then
    QMAKE_BIN="$(command -v qmake)"
else
    echo "qmake was not found; Qt deployment cannot proceed" >&2
    exit 1
fi

VERSION="$(grep -A1 'project(DeltaQ' "$PROJECT_DIR/CMakeLists.txt" | grep -oP 'VERSION \K[0-9.]+')"
ARCH_NAME="$(uname -m)"
APPIMAGE_OUT="$BUILD_DIR/package/DeltaQ-${VERSION}-${ARCH_NAME}.AppImage"

mkdir -p "$BUILD_DIR/package"
rm -f "$APPIMAGE_OUT"
APPSTREAMCLI_BIN="$(command -v appstreamcli || true)"
export PATH="$TOOLS_DIR:$PATH"
export ARCH="$ARCH_NAME"

if [ -n "$APPSTREAMCLI_BIN" ]; then
    cat > "$TOOLS_DIR/appstreamcli" <<EOF
#!/bin/sh
cmd="\$1"
shift
exec "$APPSTREAMCLI_BIN" "\$cmd" --no-net "\$@"
EOF
    chmod +x "$TOOLS_DIR/appstreamcli"
else
    rm -f "$TOOLS_DIR/appstreamcli"
fi

APPIMAGE_EXTRACT_AND_RUN=1 QMAKE="$QMAKE_BIN" "$LINUXDEPLOY_BIN" \
    --appdir "$APPDIR" \
    --plugin qt \
    -e "$APPDIR/usr/bin/deltaq" \
    -d "$APPDIR/usr/share/applications/$DESKTOP_ID" \
    -i "$APPDIR/usr/share/icons/hicolor/scalable/apps/deltaq.svg"

QT_PLUGINS_DIR="$("$QMAKE_BIN" -query QT_INSTALL_PLUGINS)"
OFFSCREEN_PLUGIN="$QT_PLUGINS_DIR/platforms/libqoffscreen.so"
[ -f "$OFFSCREEN_PLUGIN" ] || {
    echo "Qt offscreen platform plugin was not found: $OFFSCREEN_PLUGIN" >&2
    exit 1
}
mkdir -p "$APPDIR/usr/plugins/platforms"
cp "$OFFSCREEN_PLUGIN" "$APPDIR/usr/plugins/platforms/libqoffscreen.so"

APPIMAGETOOL_ARGS=("$APPDIR" "$APPIMAGE_OUT")
if [ -n "$RUNTIME_FILE" ]; then
    APPIMAGETOOL_ARGS=(--runtime-file "$RUNTIME_FILE" "${APPIMAGETOOL_ARGS[@]}")
fi

APPIMAGE_EXTRACT_AND_RUN=1 "$APPIMAGETOOL_BIN" "${APPIMAGETOOL_ARGS[@]}"

(
    cd "$BUILD_DIR/package"
    sha256sum *.tar.gz *.AppImage > SHA256SUMS
)

"$PROJECT_DIR/scripts/verify_appimage_file.sh" "$APPIMAGE_OUT" "$BUILD_DIR/package/SHA256SUMS"

echo "AppImage created: $APPIMAGE_OUT"
