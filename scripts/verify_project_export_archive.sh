#!/bin/bash

set -euo pipefail

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <archive.tar.gz>" >&2
    exit 1
fi

ARCHIVE_PATH="$(realpath "$1")"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

[ -f "$ARCHIVE_PATH" ] || {
    echo "Project export archive not found: $ARCHIVE_PATH" >&2
    exit 1
}

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

tar -xzf "$ARCHIVE_PATH" -C "$TMP_DIR"

ENTRY_COUNT=$(find "$TMP_DIR" -mindepth 1 -maxdepth 1 | wc -l)
[ "$ENTRY_COUNT" -eq 1 ] || {
    echo "Unexpected project export archive layout: expected one top-level entry, got $ENTRY_COUNT" >&2
    exit 1
}

BUNDLE_DIR="$(find "$TMP_DIR" -mindepth 1 -maxdepth 1 -type d | head -1)"
[ -n "$BUNDLE_DIR" ] || {
    echo "Project export archive did not unpack into a directory" >&2
    exit 1
}

bash "$SCRIPT_DIR/verify_project_export_bundle.sh" "$BUNDLE_DIR" >/dev/null

echo "Project export archive verification OK: $ARCHIVE_PATH"
