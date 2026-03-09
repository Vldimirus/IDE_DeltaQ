#!/bin/bash

set -euo pipefail

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
    echo "Usage: $0 <bundle_dir> [--expect-version-file]" >&2
    exit 1
fi

BUNDLE_DIR="$1"
EXPECT_VERSION_FILE=0

if [ "${2:-}" = "--expect-version-file" ]; then
    EXPECT_VERSION_FILE=1
fi

fail()
{
    echo "Bundle verification failed: $1" >&2
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

require_exec "$BUNDLE_DIR/deltaq"
require_file "$BUNDLE_DIR/LICENSE"
require_file "$BUNDLE_DIR/README.md"
require_file "$BUNDLE_DIR/README_RU.md"

require_dir "$BUNDLE_DIR/modules"
require_file "$BUNDLE_DIR/modules/core/pack.json"

require_dir "$BUNDLE_DIR/templates"
require_file "$BUNDLE_DIR/templates/README.md"
require_file "$BUNDLE_DIR/templates/console/template.json"
require_file "$BUNDLE_DIR/templates/desktop/template.json"

require_dir "$BUNDLE_DIR/examples"
require_file "$BUNDLE_DIR/examples/minimal_console_flow/minimal_console_flow.dqproj"
require_file "$BUNDLE_DIR/examples/desktop_ui_flow/desktop_ui_flow.dqproj"

require_dir "$BUNDLE_DIR/translations"
require_file "$BUNDLE_DIR/translations/deltaq_ru.qm"

if [ "$EXPECT_VERSION_FILE" -eq 1 ]; then
    require_file "$BUNDLE_DIR/VERSION"
fi

echo "Bundle verification OK: $BUNDLE_DIR"
