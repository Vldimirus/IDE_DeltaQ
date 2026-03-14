#!/bin/bash

set -euo pipefail

if [ "$#" -gt 2 ]; then
    echo "Usage: $0 [bundle_dir] [example_name]" >&2
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUNDLE_DIR="${1:-$SCRIPT_DIR/../build/release/DeltaQ}"
EXAMPLE_NAME="${2:-minimal_console_flow}"
BUNDLE_DIR="$(realpath "$BUNDLE_DIR")"

fail()
{
    echo "Linux example export smoke failed: $1" >&2
    exit 1
}

example_project_file()
{
    local project_dir="$1"
    local dqproj
    dqproj="$(find "$project_dir" -maxdepth 1 -type f -name '*.dqproj' | head -n 1)"
    [ -n "$dqproj" ] || return 1
    printf '%s\n' "$dqproj"
}

project_name_from_file()
{
    local dqproj="$1"
    sed -n 's/^[[:space:]]*"name":[[:space:]]*"\([^"]*\)".*/\1/p' "$dqproj" | head -n 1
}

[ -d "$BUNDLE_DIR" ] || fail "bundle directory not found: $BUNDLE_DIR"
[ -d "$BUNDLE_DIR/examples/$EXAMPLE_NAME" ] \
    || fail "example not found in bundle: $EXAMPLE_NAME"

"$SCRIPT_DIR/verify_release_bundle.sh" "$BUNDLE_DIR" --expect-version-file >/dev/null

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

WORKSPACE_DIR="$TMP_DIR/workspace"
DELTAQ_HOME_DIR="$TMP_DIR/deltaq-home"
PROJECT_DIR="$WORKSPACE_DIR/$EXAMPLE_NAME"
STDOUT_LOG="$TMP_DIR/stdout.log"
STDERR_LOG="$TMP_DIR/stderr.log"

mkdir -p "$WORKSPACE_DIR" "$DELTAQ_HOME_DIR"
cp -a "$BUNDLE_DIR/examples/$EXAMPLE_NAME" "$PROJECT_DIR"

PROJECT_FILE="$(example_project_file "$PROJECT_DIR")" \
    || fail "failed to locate .dqproj inside copied example"
PROJECT_NAME="$(project_name_from_file "$PROJECT_FILE")"
[ -n "$PROJECT_NAME" ] || fail "failed to read project name from $PROJECT_FILE"

export DELTAQ_HOME="$DELTAQ_HOME_DIR"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"

AUTOMATION_ARGS=(
    --automation-project "$PROJECT_FILE"
    --automation-export
    --automation-quit
)

if command -v timeout >/dev/null 2>&1; then
    timeout 240s "$BUNDLE_DIR/deltaq" "${AUTOMATION_ARGS[@]}" >"$STDOUT_LOG" 2>"$STDERR_LOG" \
        || fail "IDE export automation failed or timed out (stderr: $STDERR_LOG)"
else
    "$BUNDLE_DIR/deltaq" "${AUTOMATION_ARGS[@]}" >"$STDOUT_LOG" 2>"$STDERR_LOG" \
        || fail "IDE export automation failed (stderr: $STDERR_LOG)"
fi

EXPORT_BUNDLE_DIR="$PROJECT_DIR/dist/$PROJECT_NAME"
EXPORT_ARCHIVE_PATH="$PROJECT_DIR/dist/$PROJECT_NAME.tar.gz"

[ -d "$PROJECT_DIR/build" ] || fail "project build directory was not created"
[ -d "$PROJECT_DIR/dist" ] || fail "project dist directory was not created"
[ -d "$EXPORT_BUNDLE_DIR" ] || fail "export bundle directory was not created: $EXPORT_BUNDLE_DIR"
[ -f "$EXPORT_ARCHIVE_PATH" ] || fail "export archive was not created: $EXPORT_ARCHIVE_PATH"

"$SCRIPT_DIR/verify_project_export_bundle.sh" "$EXPORT_BUNDLE_DIR" >/dev/null
"$SCRIPT_DIR/verify_project_export_archive.sh" "$EXPORT_ARCHIVE_PATH" >/dev/null

echo "Linux example export smoke OK: $EXAMPLE_NAME"
echo "  bundle: $BUNDLE_DIR"
echo "  project: $PROJECT_FILE"
echo "  exported bundle: $EXPORT_BUNDLE_DIR"
echo "  exported archive: $EXPORT_ARCHIVE_PATH"
