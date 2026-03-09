#!/bin/bash

set -euo pipefail

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
    echo "Usage: $0 <file.AppImage> [SHA256SUMS]" >&2
    exit 1
fi

APPIMAGE_PATH="$(realpath "$1")"
CHECKSUM_FILE="${2:-}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

[ -f "$APPIMAGE_PATH" ] || {
    echo "AppImage file not found: $APPIMAGE_PATH" >&2
    exit 1
}

if [ -n "$CHECKSUM_FILE" ]; then
    CHECKSUM_PATH="$(realpath "$CHECKSUM_FILE")"
    [ -f "$CHECKSUM_PATH" ] || {
        echo "Checksum file not found: $CHECKSUM_PATH" >&2
        exit 1
    }
    CHECKSUM_ENTRY="$(grep -F "  $(basename "$APPIMAGE_PATH")" "$CHECKSUM_PATH" || true)"
    [ -n "$CHECKSUM_ENTRY" ] || {
        echo "Checksum entry not found for: $APPIMAGE_PATH" >&2
        exit 1
    }
    (
        cd "$(dirname "$CHECKSUM_PATH")"
        printf '%s\n' "$CHECKSUM_ENTRY" | sha256sum -c --status
    ) || {
        echo "Checksum verification failed for: $APPIMAGE_PATH" >&2
        exit 1
    }
fi

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

(
    cd "$TMP_DIR"
    chmod +x "$APPIMAGE_PATH"
    "$APPIMAGE_PATH" --appimage-extract >/dev/null
)

"$SCRIPT_DIR/verify_appdir.sh" "$TMP_DIR/squashfs-root" >/dev/null

echo "AppImage verification OK: $APPIMAGE_PATH"
