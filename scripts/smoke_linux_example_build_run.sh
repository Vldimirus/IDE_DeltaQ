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
    echo "Linux example build/run smoke failed: $1" >&2
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

export DELTAQ_HOME="$DELTAQ_HOME_DIR"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"

AUTOMATION_ARGS=(
    --automation-project "$PROJECT_FILE"
    --automation-build
    --automation-run
    --automation-quit
)

case "$EXAMPLE_NAME" in
    minimal_console_flow)
        AUTOMATION_ARGS+=(
            --automation-stdin 'DeltaQ\n'
            --automation-expect-output 'Hello from DeltaQ!\nDeltaQ'
        )
        ;;
    imported_pack_sensor_console)
        AUTOMATION_ARGS+=(
            --automation-expect-output 'READ=48\nSCALED=144\nSHUTDOWN=done\nFAILED_SCALE=-1\nLAST_ERROR=scale factor must be positive\nSHUTDOWN=done'
        )
        ;;
    imported_pack_checksum_console)
        AUTOMATION_ARGS+=(
            --automation-expect-output 'TEXT=DeltaQ\nCHECKSUM=2115045471\nHEX=7E11085F\nSHUTDOWN=done\nEXPECTED=00000000\nACTUAL=7E11085F\nMATCH=0\nSHUTDOWN=done'
        )
        ;;
    *)
        ;;
esac

if command -v timeout >/dev/null 2>&1; then
    timeout 180s "$BUNDLE_DIR/deltaq" "${AUTOMATION_ARGS[@]}" >"$STDOUT_LOG" 2>"$STDERR_LOG" \
        || fail "IDE automation failed or timed out (stderr: $STDERR_LOG)"
else
    "$BUNDLE_DIR/deltaq" "${AUTOMATION_ARGS[@]}" >"$STDOUT_LOG" 2>"$STDERR_LOG" \
        || fail "IDE automation failed (stderr: $STDERR_LOG)"
fi

[ -d "$PROJECT_DIR/build" ] || fail "project build directory was not created"

echo "Linux example build/run smoke OK: $EXAMPLE_NAME"
echo "  bundle: $BUNDLE_DIR"
echo "  project: $PROJECT_FILE"
echo "  DELTAQ_HOME: $DELTAQ_HOME_DIR"
