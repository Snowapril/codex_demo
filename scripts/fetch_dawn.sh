#!/bin/sh
set -e

DAWN_DIR=${DAWN_DIR:-third_party/dawn}
DAWN_URL=${DAWN_URL:-https://dawn.googlesource.com/dawn}

if [ -d "$DAWN_DIR" ]; then
  echo "Dawn already exists at $DAWN_DIR"
  exit 0
fi

mkdir -p "$(dirname "$DAWN_DIR")"

git clone "$DAWN_URL" "$DAWN_DIR"

echo "Dawn cloned to $DAWN_DIR"
