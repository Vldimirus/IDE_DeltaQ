#!/bin/bash

set -euo pipefail

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
    echo "Usage: $0 <archive.tar.gz> [SHA256SUMS]" >&2
    exit 1
fi

ARCHIVE_PATH="$(realpath "$1")"
CHECKSUM_FILE="${2:-}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

[ -f "$ARCHIVE_PATH" ] || {
    echo "Package archive not found: $ARCHIVE_PATH" >&2
    exit 1
}

if [ -n "$CHECKSUM_FILE" ]; then
    CHECKSUM_PATH="$(realpath "$CHECKSUM_FILE")"
    [ -f "$CHECKSUM_PATH" ] || {
        echo "Checksum file not found: $CHECKSUM_PATH" >&2
        exit 1
    }
    CHECKSUM_ENTRY="$(grep -F "  $(basename "$ARCHIVE_PATH")" "$CHECKSUM_PATH" || true)"
    [ -n "$CHECKSUM_ENTRY" ] || {
        echo "Checksum entry not found for: $ARCHIVE_PATH" >&2
        exit 1
    }
    (
        cd "$(dirname "$CHECKSUM_PATH")"
        printf '%s\n' "$CHECKSUM_ENTRY" | sha256sum -c --status
    ) || {
        echo "Checksum verification failed for: $ARCHIVE_PATH" >&2
        exit 1
    }
fi

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

tar -xzf "$ARCHIVE_PATH" -C "$TMP_DIR"

ENTRY_COUNT=$(find "$TMP_DIR" -mindepth 1 -maxdepth 1 | wc -l)
[ "$ENTRY_COUNT" -eq 1 ] || {
    echo "Unexpected archive layout: expected one top-level entry, got $ENTRY_COUNT" >&2
    exit 1
}

BUNDLE_DIR="$(find "$TMP_DIR" -mindepth 1 -maxdepth 1 -type d | head -1)"
[ -n "$BUNDLE_DIR" ] || {
    echo "Archive did not unpack into a directory" >&2
    exit 1
}

"$SCRIPT_DIR/verify_release_bundle.sh" "$BUNDLE_DIR" --expect-version-file >/dev/null

echo "Package archive verification OK: $ARCHIVE_PATH"
