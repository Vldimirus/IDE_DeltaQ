#!/bin/bash

set -euo pipefail

usage()
{
    echo "Usage: $0 <bundle_dir> [--expect-text <text>] [--timeout <seconds>]" >&2
    exit 1
}

fail()
{
    echo "Project export clean-environment verification failed: $1" >&2
    if [ -f "${STDOUT_LOG:-}" ]; then
        echo "--- STDOUT ---" >&2
        cat "$STDOUT_LOG" >&2
    fi
    if [ -f "${STDERR_LOG:-}" ]; then
        echo "--- STDERR ---" >&2
        cat "$STDERR_LOG" >&2
    fi
    exit 1
}

append_ro_bind_if_exists()
{
    local path="$1"
    if [ -e "$path" ]; then
        BWRAP_ARGS+=(--ro-bind "$path" "$path")
    fi
}

[ "$#" -ge 1 ] || usage

BUNDLE_DIR="$1"
shift

EXPECT_TEXT=""
TIMEOUT_SECONDS="20"

while [ "$#" -gt 0 ]; do
    case "$1" in
        --expect-text)
            [ "$#" -ge 2 ] || usage
            EXPECT_TEXT="$2"
            shift 2
            ;;
        --timeout)
            [ "$#" -ge 2 ] || usage
            TIMEOUT_SECONDS="$2"
            shift 2
            ;;
        *)
            usage
            ;;
    esac
done

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUNDLE_DIR="$(realpath "$BUNDLE_DIR")"

[ -d "$BUNDLE_DIR" ] || fail "bundle directory not found: $BUNDLE_DIR"
command -v bwrap >/dev/null 2>&1 || fail "bwrap is required for clean-environment verification"

bash "$SCRIPT_DIR/verify_project_export_bundle.sh" "$BUNDLE_DIR" >/dev/null

BUNDLE_NAME="$(basename "$BUNDLE_DIR")"
LAUNCHER_PATH="$BUNDLE_DIR/$BUNDLE_NAME"
MANIFEST_PATH="$BUNDLE_DIR/EXPORT_INFO.txt"

[ -x "$LAUNCHER_PATH" ] || fail "launcher not found: $LAUNCHER_PATH"
[ -f "$MANIFEST_PATH" ] || fail "manifest not found: $MANIFEST_PATH"

PROJECT_TYPE="$(sed -n 's/^Project type:[[:space:]]*//p' "$MANIFEST_PATH" | head -n 1)"
[ -n "$PROJECT_TYPE" ] || fail "project type missing in manifest"

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

STDOUT_LOG="$TMP_DIR/stdout.log"
STDERR_LOG="$TMP_DIR/stderr.log"

HOST_BUNDLE_PARENT="$(dirname "$BUNDLE_DIR")"
SANDBOX_BUNDLE_PARENT="/bundle-root"
SANDBOX_LAUNCHER="$SANDBOX_BUNDLE_PARENT/$BUNDLE_NAME/$BUNDLE_NAME"

BWRAP_ARGS=(
    --die-with-parent
    --unshare-user-try
    --proc /proc
    --dev /dev
    --tmpfs /tmp
    --dir /run
    --dir /var
    --dir /home
    --dir /tmp/home
    --dir /tmp/runtime
    --chdir /tmp
    --setenv PATH /usr/bin:/bin
    --setenv HOME /tmp/home
    --setenv XDG_RUNTIME_DIR /tmp/runtime
)

append_ro_bind_if_exists /usr
append_ro_bind_if_exists /bin
append_ro_bind_if_exists /sbin
append_ro_bind_if_exists /lib
append_ro_bind_if_exists /lib64
append_ro_bind_if_exists /etc

if [ "$PROJECT_TYPE" = "desktop" ]; then
    BWRAP_ARGS+=(
        --setenv SDL_VIDEODRIVER "${SDL_VIDEODRIVER:-dummy}"
        --setenv SDL_RENDER_DRIVER "${SDL_RENDER_DRIVER:-software}"
        --setenv DQ_DESKTOP_UI_FLOW_AUTOCLOSE_MS "${DQ_DESKTOP_UI_FLOW_AUTOCLOSE_MS:-1200}"
    )
fi

RUN_CMD=(
    bwrap
    "${BWRAP_ARGS[@]}"
    --ro-bind "$HOST_BUNDLE_PARENT" "$SANDBOX_BUNDLE_PARENT"
    "$SANDBOX_LAUNCHER"
)

RUN_STATUS=0
if command -v timeout >/dev/null 2>&1; then
    timeout "${TIMEOUT_SECONDS}s" "${RUN_CMD[@]}" >"$STDOUT_LOG" 2>"$STDERR_LOG" || RUN_STATUS=$?
else
    "${RUN_CMD[@]}" >"$STDOUT_LOG" 2>"$STDERR_LOG" || RUN_STATUS=$?
fi

[ "$RUN_STATUS" -eq 0 ] || fail "sandboxed launcher exited with code $RUN_STATUS"

if [ -n "$EXPECT_TEXT" ]; then
    grep -Fq "$EXPECT_TEXT" "$STDOUT_LOG" || fail "expected text was not found in launcher stdout: $EXPECT_TEXT"
fi

echo "Project export clean-environment verification OK: $BUNDLE_DIR"
