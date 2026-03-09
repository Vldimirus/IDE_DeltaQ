#!/bin/bash

set -euo pipefail

if [ "$#" -gt 1 ]; then
    echo "Usage: $0 [bundle_dir]" >&2
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUNDLE_DIR="${1:-$SCRIPT_DIR/../build/release/DeltaQ}"
BUNDLE_DIR="$(realpath "$BUNDLE_DIR")"

fail()
{
    echo "Linux first-run smoke failed: $1" >&2
    exit 1
}

[ -d "$BUNDLE_DIR" ] || fail "bundle directory not found: $BUNDLE_DIR"

"$SCRIPT_DIR/verify_release_bundle.sh" "$BUNDLE_DIR" --expect-version-file >/dev/null

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

DELTAQ_HOME_DIR="$TMP_DIR/deltaq-home"
mkdir -p "$DELTAQ_HOME_DIR"

STDOUT_LOG="$TMP_DIR/stdout.log"
STDERR_LOG="$TMP_DIR/stderr.log"

export DELTAQ_HOME="$DELTAQ_HOME_DIR"
export DELTAQ_SMOKE_EXIT_MS="${DELTAQ_SMOKE_EXIT_MS:-1500}"

# Smoke-check должен быть детерминированным и не зависеть от наличия рабочего GUI-сеанса.
# При необходимости пользователь может явно переопределить платформенный backend через QT_QPA_PLATFORM.
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"

if command -v timeout >/dev/null 2>&1; then
    timeout 20s "$BUNDLE_DIR/deltaq" >"$STDOUT_LOG" 2>"$STDERR_LOG" \
        || fail "application launch failed or timed out (stderr: $STDERR_LOG)"
else
    "$BUNDLE_DIR/deltaq" >"$STDOUT_LOG" 2>"$STDERR_LOG" \
        || fail "application launch failed (stderr: $STDERR_LOG)"
fi

[ -d "$DELTAQ_HOME_DIR/config" ] || fail "config dir was not created in DELTAQ_HOME"
[ -f "$DELTAQ_HOME_DIR/config/settings.ini" ] || fail "settings.ini was not created on first launch"
[ -d "$DELTAQ_HOME_DIR/modules/core" ] || fail "core modules dir was not created in DELTAQ_HOME"
[ -f "$DELTAQ_HOME_DIR/modules/core/pack.json" ] || fail "core pack was not installed into DELTAQ_HOME"

echo "Linux first-run smoke OK: $BUNDLE_DIR"
echo "  DELTAQ_HOME: $DELTAQ_HOME_DIR"
