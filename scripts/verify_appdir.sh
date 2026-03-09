#!/bin/bash

set -euo pipefail

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <AppDir>" >&2
    exit 1
fi

APPDIR="$(realpath "$1")"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

fail()
{
    echo "AppDir verification failed: $1" >&2
    exit 1
}

[ -d "$APPDIR" ] || fail "missing AppDir: $APPDIR"
[ -x "$APPDIR/AppRun" ] || fail "missing executable AppRun"
[ -e "$APPDIR/.DirIcon" ] || fail "missing .DirIcon"
[ -f "$APPDIR/usr/share/applications/org.deltaq.deltaq.desktop" ] \
    || fail "missing desktop entry"
[ -f "$APPDIR/usr/share/icons/hicolor/scalable/apps/deltaq.svg" ] \
    || fail "missing app icon"
[ -f "$APPDIR/usr/share/metainfo/org.deltaq.deltaq.appdata.xml" ] \
    || fail "missing AppStream metadata"

grep -q "^Exec=deltaq$" "$APPDIR/usr/share/applications/org.deltaq.deltaq.desktop" \
    || fail "desktop entry Exec is not deltaq"
grep -q "^Icon=deltaq$" "$APPDIR/usr/share/applications/org.deltaq.deltaq.desktop" \
    || fail "desktop entry Icon is not deltaq"

"$SCRIPT_DIR/verify_release_bundle.sh" "$APPDIR/usr/bin" --expect-version-file >/dev/null
"$SCRIPT_DIR/smoke_linux_first_run.sh" "$APPDIR/usr/bin" >/dev/null
"$SCRIPT_DIR/smoke_linux_example_build_run.sh" "$APPDIR/usr/bin" >/dev/null

echo "AppDir verification OK: $APPDIR"
