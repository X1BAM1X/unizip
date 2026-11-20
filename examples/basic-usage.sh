#!/bin/sh

# Basic uz usage demo.
# Usage: ./basic-usage.sh <archive> [destination]

set -eu

if [ "$#" -lt 1 ]; then
    echo "Usage: $0 <archive> [destination]" >&2
    exit 1
fi

archive=$1
dest=${2:-./extracted}

if ! command -v uz >/dev/null 2>&1; then
    echo "uz: command not found in PATH" >&2
    exit 1
fi

echo "Archive: $archive"
echo "Destination: $dest"

echo
echo "--- detected format and contents ---"
uz -v -l "$archive"

echo
echo "--- integrity test ---"
uz -t "$archive"

echo
echo "--- extract ---"
mkdir -p "$dest"
uz -f -o "$dest" "$archive"

echo
echo "Done: $dest"
