#!/bin/bash

set -euo pipefail

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <bundle_dir>" >&2
    exit 1
fi

BUNDLE_DIR="$(realpath "$1")"

fail()
{
    echo "Project export bundle verification failed: $1" >&2
    exit 1
}

require_file()
{
    local path="$1"
    [ -f "$path" ] || fail "missing file: $path"
}

require_dir()
{
    local path="$1"
    [ -d "$path" ] || fail "missing directory: $path"
}

require_exec()
{
    local path="$1"
    [ -x "$path" ] || fail "missing executable: $path"
}

is_allowlisted_library()
{
    local name="$1"
    case "$name" in
        ld-linux-x86-64.so.2|ld-linux.so.2|libc.so.6|libm.so.6|libpthread.so.0|libdl.so.2|librt.so.1|libgcc_s.so.1|libstdc++.so.6|libresolv.so.2|libutil.so.1)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

[ -d "$BUNDLE_DIR" ] || fail "bundle directory not found: $BUNDLE_DIR"

BUNDLE_NAME="$(basename "$BUNDLE_DIR")"
LAUNCHER_PATH="$BUNDLE_DIR/$BUNDLE_NAME"
BIN_PATH="$BUNDLE_DIR/bin/$BUNDLE_NAME.bin"
MANIFEST_PATH="$BUNDLE_DIR/EXPORT_INFO.txt"

require_exec "$LAUNCHER_PATH"
require_exec "$BIN_PATH"
require_file "$MANIFEST_PATH"
require_dir "$BUNDLE_DIR/bin"
require_dir "$BUNDLE_DIR/lib"
require_dir "$BUNDLE_DIR/assets"
require_dir "$BUNDLE_DIR/assets/fonts"

PROJECT_TYPE="$(sed -n 's/^Project type:[[:space:]]*//p' "$MANIFEST_PATH" | head -n 1)"
[ -n "$PROJECT_TYPE" ] || fail "project type missing in manifest"

if [ "$PROJECT_TYPE" = "desktop" ]; then
    require_file "$BUNDLE_DIR/assets/fonts/default.ttf"
    find "$BUNDLE_DIR/lib" -mindepth 1 -maxdepth 1 -type f | grep -q . || \
        fail "desktop bundle has empty lib/ payload"
fi

LDD_OUTPUT="$(env -i PATH="$PATH" LD_LIBRARY_PATH="$BUNDLE_DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" ldd "$BIN_PATH" 2>&1)" || \
    fail "ldd failed for $BIN_PATH"

echo "$LDD_OUTPUT" | grep -q "not found" && fail "unresolved library dependency detected"

while IFS= read -r line; do
    trimmed="$(printf '%s' "$line" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
    [ -n "$trimmed" ] || continue
    case "$trimmed" in
        linux-vdso*|statically\ linked)
            continue
            ;;
    esac

    resolved_path=""
    if [[ "$trimmed" == *"=>"* ]]; then
        resolved_path="${trimmed#*=> }"
        resolved_path="${resolved_path%% (*}"
    else
        resolved_path="${trimmed%% (*}"
    fi

    [[ "$resolved_path" == /* ]] || continue
    library_name="$(basename "$resolved_path")"
    if is_allowlisted_library "$library_name"; then
        continue
    fi

    case "$resolved_path" in
        "$BUNDLE_DIR"/lib/*)
            ;;
        *)
            fail "non-allowlisted library resolved outside bundle: $library_name -> $resolved_path"
            ;;
    esac
done <<< "$LDD_OUTPUT"

echo "Project export bundle verification OK: $BUNDLE_DIR"
