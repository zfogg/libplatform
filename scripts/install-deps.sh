#!/usr/bin/env bash
set -euo pipefail

case "${1:-}" in
  linux)
    sudo apt-get update
    sudo apt-get install -y cmake ninja-build gcc musl-tools pkg-config zip binutils libfontconfig1-dev
    ;;
  macos)
    brew install cmake ninja pkg-config
    ;;
  *)
    echo "usage: $0 linux|macos" >&2
    exit 2
    ;;
esac
